#if !defined(NCMPCPP_SCREEN_SWITCHER_H)
#define NCMPCPP_SCREEN_SWITCHER_H

#include <stdbool.h>

#include "screens/nc_screen.h"

NcScreen *nc_screen_switcher_current(void);
NcScreen *nc_screen_switcher_previous(void);
bool nc_screen_switcher_is_current(NcScreen *screen);
bool nc_screen_switcher_is_visible(NcScreen *screen);
bool nc_screen_switcher_switch_to(NcScreen *screen,
                                  bool has_to_be_resized);
bool nc_screen_switcher_finish_switch(NcScreen *screen);
NcScreenResizeParams nc_screen_switcher_resize_params(
    NcScreen *screen, bool adjust_locked_screen);
void nc_screen_switcher_get_resize_params(NcScreen *screen,
                                          int32 *x_offset, int32 *width,
                                          bool adjust_locked_screen);

#endif /* NCMPCPP_SCREEN_SWITCHER_H */
