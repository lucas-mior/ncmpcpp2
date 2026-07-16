#include "app_legacy_bridge.h"

#include "actions_legacy_runtime.h"
#include "app_binding_migration.h"
#include "app_controller.h"
#include "bindings.h"
#include "c/ncm_base.h"
#include "global.h"
#include "screens/native_c_screens.h"
#include "settings.h"
#include "settings_legacy_runtime.h"
#include "status.h"
#include "statusbar.h"
#include "title.h"
#include "cbase/base_macros.h"
#include "ui_state.h"

#if defined(__GNUC__)
extern bool settings_legacy_runtime_sync_configuration(void)
    __attribute__((weak));
extern bool actions_legacy_runtime_execute_binding(NcmBinding *binding)
    __attribute__((weak));
extern bool actions_legacy_runtime_execute_action(enum NcmActionType type)
    __attribute__((weak));
extern bool actions_legacy_runtime_exit_requested(void)
    __attribute__((weak));
#endif

/*
 * Temporary app runtime bridge.
 *
 * Keep the public C entry points used by ncmpcpp.c in this C module.  The
 * pieces that still depend on legacy C++ state are forwarded to the private
 * legacy runtime helpers.  As those dependencies are ported, replace the
 * forwarding calls here with direct C implementations.
 */

static void app_legacy_bridge_request_media_library_database_update(
    void *user);
static void app_legacy_bridge_refresh_playlist_related_inactive_columns(
    void *user);
static void app_legacy_bridge_set_status_observers(void);
static void app_legacy_bridge_set_resize_flags(void);
static void app_legacy_bridge_dispatch_lyrics_jobs(void);
static void app_legacy_bridge_refresh_header_if_due(void);
static bool app_legacy_bridge_sync_legacy_configuration(void);
static bool app_legacy_bridge_execute_legacy_binding(NcmBinding *binding);
static bool app_legacy_bridge_execute_legacy_action(enum NcmActionType type);
static bool app_legacy_bridge_legacy_exit_requested(void);

static NcmTimePoint app_legacy_bridge_header_refresh_time;


static void
app_legacy_bridge_request_media_library_database_update(void *user) {
    (void)user;
    native_media_library_screen_request_database_update(
        native_c_screen_media_library());
    return;
}

static void
app_legacy_bridge_refresh_playlist_related_inactive_columns(
    void *user) {
    (void)user;
    if (app_controller_is_screen_visible(
            native_c_screen_media_library_native())) {
        native_media_library_screen_refresh_inactive_songs(
            native_c_screen_media_library());
    }

    if (app_controller_is_screen_visible(
            native_c_screen_playlist_editor_native())) {
        nc_screen_refresh(native_c_screen_playlist_editor_native());
    }
    return;
}

static void
app_legacy_bridge_set_status_observers(void) {
    ncm_status_set_database_update_observer(
        app_legacy_bridge_request_media_library_database_update, NULL);
    ncm_status_set_playlist_update_observer(
        app_legacy_bridge_refresh_playlist_related_inactive_columns, NULL);
    return;
}

static void
app_legacy_bridge_set_resize_flags(void) {
    native_c_screens_request_registered_resize();
    native_c_screen_lyrics_set_resize();
    return;
}

static void
app_legacy_bridge_dispatch_lyrics_jobs(void) {
    NcmBuffer message;

    native_lyrics_screen_dispatch_jobs(native_c_screen_lyrics());
    ncm_buffer_init(&message);
    if (native_lyrics_screen_try_take_consumer_message(
            native_c_screen_lyrics(), &message)) {
        ncm_statusbar_print((int32)Config.message_delay_time,
                            message.data, message.len);
    }
    ncm_buffer_destroy(&message);
    return;
}

static void
app_legacy_bridge_refresh_header_if_due(void) {
    bool current_screen_uses_header_timer;

    current_screen_uses_header_timer = native_c_screen_playlist_is_current()
        || native_c_screen_browser_is_current()
        || native_c_screen_lyrics_is_current();
    if (!current_screen_uses_header_timer) {
        return;
    }
    if (global_timer_elapsed_ms(app_legacy_bridge_header_refresh_time)
        <= 500) {
        return;
    }

    ncm_title_draw_current_header();
    app_legacy_bridge_header_refresh_time = global_timer;
    return;
}

static bool
app_legacy_bridge_sync_legacy_configuration(void) {
#if defined(__GNUC__)
    if (settings_legacy_runtime_sync_configuration == NULL) {
        return true;
    }
#endif
    return settings_legacy_runtime_sync_configuration();
}

static bool
app_legacy_bridge_execute_legacy_binding(NcmBinding *binding) {
#if defined(__GNUC__)
    if (actions_legacy_runtime_execute_binding == NULL) {
        return false;
    }
#endif
    return actions_legacy_runtime_execute_binding(binding);
}

static bool
app_legacy_bridge_execute_legacy_action(enum NcmActionType type) {
#if defined(__GNUC__)
    if (actions_legacy_runtime_execute_action == NULL) {
        return false;
    }
#endif
    return actions_legacy_runtime_execute_action(type);
}

static bool
app_legacy_bridge_legacy_exit_requested(void) {
#if defined(__GNUC__)
    if (actions_legacy_runtime_exit_requested == NULL) {
        return false;
    }
#endif
    return actions_legacy_runtime_exit_requested();
}

bool
ncmpcpp_legacy_sync_configuration(void) {
    return app_legacy_bridge_sync_legacy_configuration();
}

static bool
app_legacy_bridge_execute_binding_action_hybrid(
    NcmBindingAction *action) {
    NcmBindingRuntime *runtime;

    runtime = ncm_binding_default_runtime();
    switch (action->kind) {
    case NCM_BINDING_ACTION_NORMAL:
        if (app_binding_migration_action_is_c_safe(action->type)) {
            return ncm_action_runtime_run(NULL, action->type);
        }
        return app_legacy_bridge_execute_legacy_action(action->type);
    case NCM_BINDING_ACTION_REQUIRE_RUNNABLE:
        return ncm_binding_action_can_run(action, runtime);
    case NCM_BINDING_ACTION_PUSH_CHARACTERS:
    case NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND:
    case NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND:
        return ncm_binding_action_run(action, runtime);
    case NCM_BINDING_ACTION_REQUIRE_SCREEN:
        return false;
    }

    return false;
}

static bool
app_legacy_bridge_execute_binding_hybrid(NcmBinding *binding) {
    if (!app_binding_migration_binding_is_hybrid_safe(binding)) {
        return false;
    }

    for (int32 i = 0; i < binding->actions_len; i += 1) {
        if (!app_legacy_bridge_execute_binding_action_hybrid(
                binding->actions + i)) {
            return false;
        }
    }

    return true;
}

static void
app_legacy_bridge_noidle_status_update(int32 flags, void *user) {
    NcmError error;

    (void)user;
    ncm_error_clear(&error);
    (void)ncm_status_update(&global_mpd, flags, &error);
    return;
}

static char *
app_legacy_bridge_mpd_error_message(NcmError *error) {
    char *message;

    if ((error != NULL) && (error->message[0] != '\0')) {
        return error->message;
    }

    message = ncm_mpd_client_error_message(&global_mpd);
    if ((message != NULL) && (message[0] != '\0')) {
        return message;
    }

    return (char *)"MPD command failed";
}

static void
app_legacy_bridge_report_mpd_error(NcmError *error) {
    NcmStringFormatArg arg;
    char *message;

    message = app_legacy_bridge_mpd_error_message(error);
    arg = ncm_string_format_arg_cstring(message);

    if ((ncm_mpd_client_error_code(&global_mpd) == MPD_ERROR_SERVER)
        || ((error != NULL) && (error->code == MPD_ERROR_SERVER))) {
        ncm_statusbar_format((int32)Config.message_delay_time,
                             STRLIT_ARGS("MPD: %1%"), &arg, 1);
    } else {
        ncm_statusbar_format((int32)Config.message_delay_time,
                             STRLIT_ARGS("ncmpcpp: %1%"), &arg, 1);
    }
    return;
}

void
ncmpcpp_legacy_init_screen(bool enable_colors, bool enable_mouse) {
    nc_init_screen(enable_colors, enable_mouse);
    nc_init_readline();
    return;
}

void
ncmpcpp_legacy_destroy_screen(void) {
    nc_destroy_screen();
    return;
}

void
ncmpcpp_legacy_set_statusbar_visibility_baseline(bool visible) {
    ui_state_set_statusbar_visibility_baseline(visible);
    return;
}

void
ncmpcpp_legacy_set_windows_dimensions(void) {
    int64 main_start_y;
    int64 main_height;
    int64 header_height;
    int64 footer_height;
    int64 footer_start_y;

    ui_state_set_screen_size(COLS, LINES);

    if (Config.design == NCM_DESIGN_ALTERNATIVE) {
        main_start_y = 5;
        main_height = LINES - 7;
    } else {
        main_start_y = 2;
        main_height = LINES - 4;
    }

    if (main_height < 0) {
        main_height = 0;
    }

    if (!Config.header_visibility) {
        main_start_y -= 2;
        main_height += 2;
    }

    if (!Config.statusbar_visibility) {
        main_height += 1;
    }

    if (Config.design == NCM_DESIGN_ALTERNATIVE) {
        if (Config.header_visibility) {
            header_height = 5;
        } else {
            header_height = 3;
        }
    } else {
        header_height = 2;
    }

    if (Config.statusbar_visibility) {
        footer_height = 2;
    } else {
        footer_height = 1;
    }
    footer_start_y = LINES - footer_height;

    ui_state_set_main_geometry(main_start_y, main_height);
    ui_state_set_header_height(header_height);
    ui_state_set_footer_height(footer_height);
    ui_state_set_footer_start_y(footer_start_y);
    return;
}

int64
ncmpcpp_legacy_header_height(void) {
    return ui_state_header_height();
}

int64
ncmpcpp_legacy_footer_height(void) {
    return ui_state_footer_height();
}

int64
ncmpcpp_legacy_footer_start_y(void) {
    return ui_state_footer_start_y();
}

NcWindow *
ncmpcpp_legacy_window_create(int64 start_x, int64 start_y,
                             int64 width, int64 height,
                             NcColor color) {
    NcWindow *window;

    window = ncm_malloc(SIZEOF(*window));
    nc_window_init(window, start_x, start_y, width, height,
                   STRLIT_ARGS(""), color, nc_border_none());
    return window;
}

void
ncmpcpp_legacy_window_display(NcWindow *window) {
    if (window == NULL) {
        return;
    }

    nc_window_display(window);
    nc_window_refresh(window);
    return;
}

void
ncmpcpp_legacy_window_destroy(NcWindow *window) {
    if (window == NULL) {
        return;
    }

    nc_window_destroy(window);
    ncm_free(window, SIZEOF(*window));
    return;
}

void
ncmpcpp_legacy_window_clear_fd_callbacks(NcWindow *window) {
    if (window == NULL) {
        return;
    }

    nc_window_clear_fd_callbacks(window);
    return;
}

void
ncmpcpp_legacy_initialize_screens(void) {
    app_controller_init();
    native_c_screens_init_all();
    app_legacy_bridge_set_status_observers();
    native_c_screens_register_native_only();
    native_c_screen_lyrics_register();
    return;
}

void
ncmpcpp_legacy_resize_screen(bool reload_main_window) {
    NcWindow *header;
    NcWindow *footer;

    if (reload_main_window) {
        nc_resize_readline_terminal();
        endwin();
        refresh();
        getch();
    }

    ncmpcpp_legacy_set_windows_dimensions();
    app_legacy_bridge_set_resize_flags();
    app_controller_resize_visible_screens();

    header = ui_state_header_window();
    if ((header != NULL)
        && (Config.header_visibility
            || (Config.design == NCM_DESIGN_ALTERNATIVE))) {
        nc_window_resize(header, COLS, ncmpcpp_legacy_header_height());
    }

    footer = ui_state_footer_window();
    if (footer != NULL) {
        nc_window_move_to(footer, 0, ncmpcpp_legacy_footer_start_y());
        nc_window_resize(footer, COLS, ncmpcpp_legacy_footer_height());
    }

    app_controller_refresh_visible_screens();
    ncm_status_changes_elapsed_time(false);
    ncm_status_changes_player_state();
    ncm_status_changes_flags();
    ncm_title_draw_current_header();
    if (footer != NULL) {
        nc_window_refresh(footer);
    }
    refresh();
    return;
}

void
ncmpcpp_legacy_playlist_switch_to(void) {
    (void)native_c_screens_switch_to_type(NCM_SCREEN_TYPE_PLAYLIST);
    return;
}

void
ncmpcpp_legacy_playlist_enable_highlighting_if_current(void) {
    if (native_c_screen_playlist_is_current()) {
        native_playlist_screen_request_highlighting(
            native_c_screen_playlist());
    }
    return;
}

bool
ncmpcpp_legacy_switch_to_screen_type(enum ScreenType screen_type) {
    return native_c_screens_switch_to_type(screen_type);
}

bool
ncmpcpp_legacy_lock_current_screen(void) {
    return native_c_screens_lock_current();
}

enum ScreenType
ncmpcpp_legacy_current_screen_type(void) {
    return native_c_screens_current_type();
}

void
ncmpcpp_legacy_set_noidle_status_callback(void) {
    ncm_mpd_client_set_noidle_callback(
        &global_mpd, app_legacy_bridge_noidle_status_update, NULL);
    return;
}

bool
ncmpcpp_legacy_mpd_connected(void) {
    return ncm_mpd_client_connected(&global_mpd);
}

void
ncmpcpp_legacy_connect_or_report(void) {
    NcmError error;

    ncm_error_clear(&error);
    if (!ncm_mpd_client_connect(&global_mpd, &error)) {
        app_legacy_bridge_report_mpd_error(&error);
        return;
    }

    if (ncm_mpd_client_version(&global_mpd) < 16) {
        ncm_mpd_client_disconnect(&global_mpd);
        ncm_error_set(&error, MPD_ERROR_STATE,
                      STRLIT_ARGS("MPD < 0.16.0 is not supported"));
        app_legacy_bridge_report_mpd_error(&error);
    }
    return;
}

void
ncmpcpp_legacy_status_clear(void) {
    ncm_status_clear();
    return;
}

bool
ncmpcpp_legacy_update_environment(bool update_timer,
                                  bool refresh_window,
                                  bool mpd_sync) {
    NcmError error;

    app_legacy_bridge_set_status_observers();
    ncm_error_clear(&error);
    ncm_status_trace(&global_mpd, update_timer, true, &error);
    app_legacy_bridge_dispatch_lyrics_jobs();
    app_legacy_bridge_refresh_header_if_due();

    if (refresh_window) {
        app_controller_refresh_current_window();
    }

    if (mpd_sync) {
        ncm_error_clear(&error);
        (void)ncm_status_update_from_noidle(&global_mpd, NULL, &error);
    }
    return true;
}

bool
ncmpcpp_legacy_execute_binding(NcmBinding *binding) {
    enum ScreenType screen_type;

    screen_type = native_c_screens_current_type();
    if (app_binding_migration_binding_is_c_safe_for_screen(binding,
                                                           screen_type)) {
        if (!ncm_binding_can_execute_default(binding)) {
            return false;
        }
        return ncm_binding_execute_default(binding);
    }
    if (app_binding_migration_screen_is_c_only(screen_type)) {
        return false;
    }
    if (app_binding_migration_binding_is_hybrid_safe(binding)) {
        return app_legacy_bridge_execute_binding_hybrid(binding);
    }

    return app_legacy_bridge_execute_legacy_binding(binding);
}

bool
ncmpcpp_legacy_execute_action(enum NcmActionType type) {
    enum ScreenType screen_type;

    screen_type = native_c_screens_current_type();
    if (app_binding_migration_action_is_c_safe_for_screen(
            type, screen_type)) {
        return ncm_action_runtime_run(NULL, type);
    }
    if (app_binding_migration_screen_is_c_only(screen_type)) {
        return false;
    }

    return app_legacy_bridge_execute_legacy_action(type);
}

bool
ncmpcpp_legacy_exit_requested(void) {
    return app_legacy_bridge_legacy_exit_requested()
        || ncm_action_runtime_exit_requested(NULL);
}
