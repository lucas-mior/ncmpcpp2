#include "app_legacy_bridge.h"

#include "actions_legacy_runtime.h"
#include "app_binding_migration.h"
#include "bindings.h"
#include "configuration.h"
#include "global.h"
#include "screens/native_c_screens.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "ui_state.h"

/*
 * Temporary app runtime bridge.
 *
 * Keep the public C entry points used by ncmpcpp.c in this C module.  The
 * pieces that still depend on legacy C++ state are forwarded to the private
 * legacy runtime helpers.  As those dependencies are ported, replace the
 * forwarding calls here with direct C implementations.
 */


static bool
app_legacy_bridge_can_execute_binding_in_c(NcmBinding *binding) {
    if (!app_binding_migration_binding_is_c_safe(binding)) {
        return false;
    }
    return ncm_binding_can_execute_default(binding);
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

bool
ncmpcpp_legacy_configure(int32 argc, char **argv) {
    if (!actions_legacy_runtime_configure(argc, argv)) {
        return false;
    }

    ncm_bindings_configuration_destroy(&Bindings);
    ncm_bindings_configuration_init(&Bindings);
    return configure(argc, argv);
}

void
ncmpcpp_legacy_init_screen(bool enable_colors, bool enable_mouse) {
    nc_init_screen(enable_colors, enable_mouse);
    actions_legacy_runtime_init_readline();
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

void *
ncmpcpp_legacy_window_create(int64 start_x, int64 start_y,
                             int64 width, int64 height,
                             NcColor color) {
    return actions_legacy_runtime_window_create(start_x, start_y, width,
                                                height, color);
}

void
ncmpcpp_legacy_window_display(void *window) {
    actions_legacy_runtime_window_display(window);
    return;
}

void
ncmpcpp_legacy_window_destroy(void *window) {
    actions_legacy_runtime_window_destroy(window);
    return;
}

void
ncmpcpp_legacy_window_set_main_hook(void *window) {
    actions_legacy_runtime_window_set_main_hook(window);
    return;
}

void
ncmpcpp_legacy_window_clear_fd_callbacks(void *window) {
    actions_legacy_runtime_window_clear_fd_callbacks(window);
    return;
}

NcWindow *
ncmpcpp_legacy_window_native(void *window) {
    return actions_legacy_runtime_window_native(window);
}

void
ncmpcpp_legacy_initialize_screens(void) {
    actions_legacy_runtime_initialize_screens();
    return;
}

void
ncmpcpp_legacy_resize_screen(bool reload_main_window) {
    actions_legacy_runtime_resize_screen(reload_main_window);
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
        native_playlist_screen_sync(native_c_screen_playlist());
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

    /*
     * Keep the legacy status state reset while update_environment still calls
     * the C++ Status::update path.  This extra reset can be removed when the
     * update_environment bridge is fully C-owned.
     */
    actions_legacy_runtime_status_clear();
    return;
}

bool
ncmpcpp_legacy_update_environment(bool update_timer,
                                  bool refresh_window,
                                  bool mpd_sync) {
    return actions_legacy_runtime_update_environment(update_timer,
                                                     refresh_window,
                                                     mpd_sync);
}

bool
ncmpcpp_legacy_execute_binding(NcmBinding *binding) {
    if (app_legacy_bridge_can_execute_binding_in_c(binding)) {
        return ncm_binding_execute_default(binding);
    }

    return actions_legacy_runtime_execute_binding(binding);
}

bool
ncmpcpp_legacy_execute_action(enum NcmActionType type) {
    if (ncm_action_runtime_run(NULL, type)) {
        return true;
    }
    return actions_legacy_runtime_execute_action(type);
}

bool
ncmpcpp_legacy_exit_requested(void) {
    return actions_legacy_runtime_exit_requested()
        || ncm_action_runtime_exit_requested(NULL);
}
