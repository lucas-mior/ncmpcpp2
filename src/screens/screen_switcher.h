#if !defined(NCMPCPP_SCREEN_SWITCHER_H)
#define NCMPCPP_SCREEN_SWITCHER_H

#include <stdbool.h>

#include "screens/nc_screen.h"

NcScreen *nc_screen_switcher_current(void);
NcScreen *nc_screen_switcher_previous(void);
NcScreen *nc_screen_switcher_locked(void);
NcScreen *nc_screen_switcher_inactive(void);
bool nc_screen_switcher_is_current(NcScreen *screen);
bool nc_screen_switcher_is_previous(NcScreen *screen);
bool nc_screen_switcher_is_visible(NcScreen *screen);
void nc_screen_switcher_each_visible(NcScreenEachCallback callback,
                                     void *user);
bool nc_screen_switcher_switch_to(NcScreen *screen,
                                  bool has_to_be_resized);
bool nc_screen_switcher_switch_to_type(int32 type,
                                       bool has_to_be_resized);
bool nc_screen_switcher_finish_switch(NcScreen *screen);
NcScreenResizeParams nc_screen_switcher_resize_params(
    NcScreen *screen, bool adjust_locked_screen);
void nc_screen_switcher_get_resize_params(NcScreen *screen,
                                          int64 *x_offset, int64 *width,
                                          bool adjust_locked_screen);
bool nc_screen_switcher_request_resize(NcScreen *screen);
bool nc_screen_switcher_request_update(NcScreen *screen);
void nc_screen_switcher_request_visible_resize(void);
void nc_screen_switcher_request_visible_update(void);

#endif /* NCMPCPP_SCREEN_SWITCHER_H */
