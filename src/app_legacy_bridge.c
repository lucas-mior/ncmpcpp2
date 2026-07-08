#include "app_legacy_bridge.h"

#include "actions_legacy_runtime.h"
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
app_legacy_bridge_action_needs_legacy_screen(enum NcmActionType type) {
    switch (type) {
    case NCM_ACTION_UPDATE_ENVIRONMENT:
    case NCM_ACTION_MOUSE_EVENT:
    case NCM_ACTION_SCROLL_UP:
    case NCM_ACTION_SCROLL_DOWN:
    case NCM_ACTION_SCROLL_UP_ARTIST:
    case NCM_ACTION_SCROLL_UP_ALBUM:
    case NCM_ACTION_SCROLL_DOWN_ARTIST:
    case NCM_ACTION_SCROLL_DOWN_ALBUM:
    case NCM_ACTION_PAGE_UP:
    case NCM_ACTION_PAGE_DOWN:
    case NCM_ACTION_MOVE_HOME:
    case NCM_ACTION_MOVE_END:
    case NCM_ACTION_TOGGLE_INTERFACE:
    case NCM_ACTION_JUMP_TO_PARENT_DIRECTORY:
    case NCM_ACTION_PREVIOUS_COLUMN:
    case NCM_ACTION_NEXT_COLUMN:
    case NCM_ACTION_MASTER_SCREEN:
    case NCM_ACTION_SLAVE_SCREEN:
    case NCM_ACTION_ADD_ITEM_TO_PLAYLIST:
    case NCM_ACTION_PLAY_ITEM:
    case NCM_ACTION_DELETE_PLAYLIST_ITEMS:
    case NCM_ACTION_DELETE_STORED_PLAYLIST:
    case NCM_ACTION_DELETE_BROWSER_ITEMS:
    case NCM_ACTION_MOVE_SORT_ORDER_UP:
    case NCM_ACTION_MOVE_SORT_ORDER_DOWN:
    case NCM_ACTION_MOVE_SELECTED_ITEMS_UP:
    case NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN:
    case NCM_ACTION_MOVE_SELECTED_ITEMS_TO:
    case NCM_ACTION_ADD:
    case NCM_ACTION_LOAD:
    case NCM_ACTION_TOGGLE_DISPLAY_MODE:
    case NCM_ACTION_UPDATE_DATABASE:
    case NCM_ACTION_JUMP_TO_PLAYING_SONG:
    case NCM_ACTION_SAVE_TAG_CHANGES:
    case NCM_ACTION_ENTER_DIRECTORY:
    case NCM_ACTION_EDIT_SONG:
    case NCM_ACTION_EDIT_LIBRARY_TAG:
    case NCM_ACTION_EDIT_LIBRARY_ALBUM:
    case NCM_ACTION_EDIT_DIRECTORY_NAME:
    case NCM_ACTION_EDIT_PLAYLIST_NAME:
    case NCM_ACTION_EDIT_LYRICS:
    case NCM_ACTION_JUMP_TO_BROWSER:
    case NCM_ACTION_JUMP_TO_MEDIA_LIBRARY:
    case NCM_ACTION_JUMP_TO_PLAYLIST_EDITOR:
    case NCM_ACTION_TOGGLE_SCREEN_LOCK:
    case NCM_ACTION_JUMP_TO_TAG_EDITOR:
    case NCM_ACTION_JUMP_TO_POSITION_IN_SONG:
    case NCM_ACTION_SELECT_ITEM:
    case NCM_ACTION_SELECT_RANGE:
    case NCM_ACTION_REVERSE_SELECTION:
    case NCM_ACTION_REMOVE_SELECTION:
    case NCM_ACTION_SELECT_ALBUM:
    case NCM_ACTION_SELECT_FOUND_ITEMS:
    case NCM_ACTION_ADD_SELECTED_ITEMS:
    case NCM_ACTION_CROP_MAIN_PLAYLIST:
    case NCM_ACTION_CROP_PLAYLIST:
    case NCM_ACTION_CLEAR_MAIN_PLAYLIST:
    case NCM_ACTION_CLEAR_PLAYLIST:
    case NCM_ACTION_SORT_PLAYLIST:
    case NCM_ACTION_REVERSE_PLAYLIST:
    case NCM_ACTION_APPLY_FILTER:
    case NCM_ACTION_FIND:
    case NCM_ACTION_FIND_ITEM_FORWARD:
    case NCM_ACTION_FIND_ITEM_BACKWARD:
    case NCM_ACTION_NEXT_FOUND_ITEM:
    case NCM_ACTION_PREVIOUS_FOUND_ITEM:
    case NCM_ACTION_TOGGLE_FIND_MODE:
    case NCM_ACTION_TOGGLE_BROWSER_SORT_MODE:
    case NCM_ACTION_TOGGLE_LIBRARY_TAG_TYPE:
    case NCM_ACTION_TOGGLE_MEDIA_LIBRARY_SORT_MODE:
    case NCM_ACTION_FETCH_LYRICS_IN_BACKGROUND:
    case NCM_ACTION_REFETCH_LYRICS:
    case NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY:
    case NCM_ACTION_TOGGLE_OUTPUT:
    case NCM_ACTION_TOGGLE_VISUALIZATION_TYPE:
    case NCM_ACTION_SHOW_SONG_INFO:
    case NCM_ACTION_SHOW_ARTIST_INFO:
    case NCM_ACTION_SHOW_LYRICS:
    case NCM_ACTION_NEXT_SCREEN:
    case NCM_ACTION_PREVIOUS_SCREEN:
    case NCM_ACTION_SHOW_HELP:
    case NCM_ACTION_SHOW_PLAYLIST:
    case NCM_ACTION_SHOW_BROWSER:
    case NCM_ACTION_CHANGE_BROWSE_MODE:
    case NCM_ACTION_SHOW_SEARCH_ENGINE:
    case NCM_ACTION_RESET_SEARCH_ENGINE:
    case NCM_ACTION_SHOW_MEDIA_LIBRARY:
    case NCM_ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE:
    case NCM_ACTION_SHOW_PLAYLIST_EDITOR:
    case NCM_ACTION_SHOW_TAG_EDITOR:
    case NCM_ACTION_SHOW_OUTPUTS:
    case NCM_ACTION_SHOW_VISUALIZER:
    case NCM_ACTION_SHOW_SERVER_INFO:
        return true;
    default:
        return false;
    }
}

static bool
app_legacy_bridge_binding_needs_legacy_screen(NcmBinding *binding) {
    if (binding == NULL) {
        return true;
    }

    for (int32 i = 0; i < binding->actions_len; i += 1) {
        NcmBindingAction *action;

        action = binding->actions + i;
        if ((action->kind == NCM_BINDING_ACTION_NORMAL)
            || (action->kind == NCM_BINDING_ACTION_REQUIRE_RUNNABLE)) {
            if (app_legacy_bridge_action_needs_legacy_screen(action->type)) {
                return true;
            }
        }
        if (action->kind == NCM_BINDING_ACTION_REQUIRE_SCREEN) {
            return true;
        }
    }

    return false;
}

static bool
app_legacy_bridge_can_execute_binding_in_c(NcmBinding *binding) {
    if (app_legacy_bridge_binding_needs_legacy_screen(binding)) {
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
