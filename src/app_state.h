#if !defined(NCMPCPP_APP_STATE_H)
#define NCMPCPP_APP_STATE_H

#include <stdbool.h>

#include "screens/nc_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

void app_state_init(void);
NcScreen *app_state_get_screen(void);
NcScreen *app_state_get_previous_screen(void);
NcScreen *app_state_get_locked_screen(void);
NcScreen *app_state_get_inactive_screen(void);
bool app_state_last_switch_changed_screen(void);
bool app_state_register_screen(NcScreen *screen);
bool app_state_unregister_screen(NcScreen *screen);
bool app_state_switch_to_screen(NcScreen *screen);
bool app_state_switch_to_screen_type(int32 type);
bool app_state_lock_current_screen(void);
void app_state_unlock_screen(void);
bool app_state_can_show_locked_screen(void);
bool app_state_show_locked_screen(void);
bool app_state_can_show_inactive_screen(void);
bool app_state_show_inactive_screen(void);
bool app_state_is_screen_registered(NcScreen *screen);
bool app_state_is_screen_visible(NcScreen *screen);
void app_state_each_visible_screen(NcScreenEachCallback callback, void *user);
void app_state_update_current_screen(void);
void app_state_update_visible_screens(void);
void app_state_update_all_screens(void);
void app_state_resize_current_screen(void);
void app_state_resize_visible_screens(void);
void app_state_resize_all_screens(void);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_APP_STATE_H */
