#if !defined(NCMPCPP_APP_CONTROLLER_H)
#define NCMPCPP_APP_CONTROLLER_H

#include <stdbool.h>

#include "screens/nc_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

void app_controller_init(void);
NcScreen *app_controller_current_screen(void);
NcScreen *app_controller_previous_screen(void);
NcScreen *app_controller_locked_screen(void);
NcScreen *app_controller_inactive_screen(void);
bool app_controller_last_switch_changed_screen(void);
bool app_controller_register_screen(NcScreen *screen);
bool app_controller_unregister_screen(NcScreen *screen);
NcScreen *app_controller_find_screen_type(int32 type);
bool app_controller_is_screen_registered(NcScreen *screen);
bool app_controller_is_screen_visible(NcScreen *screen);
void app_controller_each_visible_screen(NcScreenEachCallback callback,
                                        void *user);
bool app_controller_switch_to_screen(NcScreen *screen);
bool app_controller_switch_to_screen_type(int32 type);
bool app_controller_lock_current_screen(void);
void app_controller_unlock_screen(void);
bool app_controller_can_show_locked_screen(void);
bool app_controller_show_locked_screen(void);
bool app_controller_can_show_inactive_screen(void);
bool app_controller_show_inactive_screen(void);
NcWindow *app_controller_active_window(void);
int32 app_controller_current_window_timeout(void);
void app_controller_update_current_screen(void);
void app_controller_update_visible_screens(void);
void app_controller_update_all_screens(void);
void app_controller_refresh_current_screen(void);
void app_controller_refresh_current_window(void);
void app_controller_refresh_visible_screens(void);
void app_controller_resize_current_screen(void);
void app_controller_resize_visible_screens(void);
void app_controller_resize_all_screens(void);
void app_controller_scroll_current_screen(enum NcScroll where);
void app_controller_mouse_button_pressed_current(MEVENT event);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_APP_CONTROLLER_H */
