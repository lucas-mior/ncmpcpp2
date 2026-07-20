#if !defined(NCMPCPP_APP_LEGACY_BRIDGE_H)
#define NCMPCPP_APP_LEGACY_BRIDGE_H

#include <stdbool.h>

#include "actions.h"
#include "bindings.h"
#include "c/ncm_defs.h"
#include "curses/nc_window.h"
#include "screens/screen_type.h"

void ncmpcpp_legacy_init_screen(bool enable_colors, bool enable_mouse);
void ncmpcpp_legacy_destroy_screen(void);

void ncmpcpp_legacy_set_statusbar_visibility_baseline(bool visible);
void ncmpcpp_legacy_set_windows_dimensions(void);
int32 ncmpcpp_legacy_header_height(void);
int32 ncmpcpp_legacy_footer_height(void);
int32 ncmpcpp_legacy_footer_start_y(void);

NcWindow *ncmpcpp_legacy_window_create(int32 start_x, int32 start_y,
                                       int32 width, int32 height,
                                       NcColor color);
void ncmpcpp_legacy_window_display(NcWindow *window);
void ncmpcpp_legacy_window_destroy(NcWindow *window);

void ncmpcpp_legacy_initialize_screens(void);
void ncmpcpp_legacy_resize_screen(bool reload_main_window);
void ncmpcpp_legacy_playlist_switch_to(void);
void ncmpcpp_legacy_playlist_enable_highlighting_if_current(void);
bool ncmpcpp_legacy_switch_to_screen_type(enum ScreenType screen_type);
bool ncmpcpp_legacy_lock_current_screen(void);
enum ScreenType ncmpcpp_legacy_current_screen_type(void);

void ncmpcpp_legacy_set_noidle_status_callback(void);
bool ncmpcpp_legacy_mpd_connected(void);
void ncmpcpp_legacy_connect_or_report(void);
void ncmpcpp_legacy_status_clear(void);
bool ncmpcpp_legacy_update_environment(bool update_timer, bool refresh_window,
                                       bool mpd_sync);
bool ncmpcpp_legacy_execute_binding(NcmBinding *binding);
bool ncmpcpp_legacy_execute_action(enum NcmActionType type);
bool ncmpcpp_legacy_exit_requested(void);

#endif /* NCMPCPP_APP_LEGACY_BRIDGE_H */
