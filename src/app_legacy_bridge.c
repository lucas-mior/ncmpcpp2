#include "app_legacy_bridge.h"

#include "actions_legacy_runtime.h"
#include "global.h"
#include "screens/native_c_screens.h"

/*
 * Temporary app runtime bridge.
 *
 * Keep the public C entry points used by ncmpcpp.c in this C module.  The
 * pieces that still depend on legacy C++ state are forwarded to the private
 * legacy runtime helpers.  As those dependencies are ported, replace the
 * forwarding calls here with direct C implementations.
 */

bool
ncmpcpp_legacy_configure(int32 argc, char **argv) {
    return actions_legacy_runtime_configure(argc, argv);
}

void
ncmpcpp_legacy_init_screen(bool enable_colors, bool enable_mouse) {
    actions_legacy_runtime_init_screen(enable_colors, enable_mouse);
    return;
}

void
ncmpcpp_legacy_destroy_screen(void) {
    actions_legacy_runtime_destroy_screen();
    return;
}

void
ncmpcpp_legacy_set_statusbar_visibility_baseline(bool visible) {
    actions_legacy_runtime_set_statusbar_visibility_baseline(visible);
    return;
}

void
ncmpcpp_legacy_set_windows_dimensions(void) {
    actions_legacy_runtime_set_windows_dimensions();
    return;
}

int64
ncmpcpp_legacy_header_height(void) {
    return actions_legacy_runtime_header_height();
}

int64
ncmpcpp_legacy_footer_height(void) {
    return actions_legacy_runtime_footer_height();
}

int64
ncmpcpp_legacy_footer_start_y(void) {
    return actions_legacy_runtime_footer_start_y();
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
    actions_legacy_runtime_playlist_enable_highlighting_if_current();
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
    actions_legacy_runtime_set_noidle_status_callback();
    return;
}

bool
ncmpcpp_legacy_mpd_connected(void) {
    return ncm_mpd_client_connected(&global_mpd);
}

void
ncmpcpp_legacy_connect_or_report(void) {
    actions_legacy_runtime_connect_or_report();
    return;
}

void
ncmpcpp_legacy_status_clear(void) {
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
