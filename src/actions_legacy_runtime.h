#if !defined(NCMPCPP_ACTIONS_LEGACY_RUNTIME_H)
#define NCMPCPP_ACTIONS_LEGACY_RUNTIME_H

#include <stdbool.h>

#include "actions.h"
#include "bindings.h"
#include "c/ncm_defs.h"
#include "curses/nc_window.h"
#include "screens/screen_type.h"

NCM_EXTERN_C_BEGIN

bool actions_legacy_runtime_configure(int32 argc, char **argv);
void actions_legacy_runtime_init_screen(bool enable_colors,
                                        bool enable_mouse);
void actions_legacy_runtime_destroy_screen(void);

void actions_legacy_runtime_set_statusbar_visibility_baseline(
    bool visible);
void actions_legacy_runtime_set_windows_dimensions(void);
int64 actions_legacy_runtime_header_height(void);
int64 actions_legacy_runtime_footer_height(void);
int64 actions_legacy_runtime_footer_start_y(void);

void *actions_legacy_runtime_window_create(int64 start_x,
                                           int64 start_y,
                                           int64 width,
                                           int64 height,
                                           NcColor color);
void actions_legacy_runtime_window_display(void *window);
void actions_legacy_runtime_window_destroy(void *window);
void actions_legacy_runtime_window_set_main_hook(void *window);
void actions_legacy_runtime_window_clear_fd_callbacks(void *window);
NcWindow *actions_legacy_runtime_window_native(void *window);

void actions_legacy_runtime_initialize_screens(void);
void actions_legacy_runtime_resize_screen(bool reload_main_window);
void actions_legacy_runtime_playlist_switch_to(void);
void actions_legacy_runtime_playlist_enable_highlighting_if_current(void);
bool actions_legacy_runtime_switch_to_screen_type(
    enum ScreenType screen_type);
bool actions_legacy_runtime_lock_current_screen(void);
enum ScreenType actions_legacy_runtime_current_screen_type(void);

void actions_legacy_runtime_set_noidle_status_callback(void);
bool actions_legacy_runtime_mpd_connected(void);
void actions_legacy_runtime_connect_or_report(void);
void actions_legacy_runtime_status_clear(void);
bool actions_legacy_runtime_update_environment(bool update_timer,
                                                bool refresh_window,
                                                bool mpd_sync);
bool actions_legacy_runtime_execute_binding(NcmBinding *binding);
bool actions_legacy_runtime_execute_action(enum NcmActionType type);
bool actions_legacy_runtime_exit_requested(void);

NCM_EXTERN_C_END

#endif /* NCMPCPP_ACTIONS_LEGACY_RUNTIME_H */
