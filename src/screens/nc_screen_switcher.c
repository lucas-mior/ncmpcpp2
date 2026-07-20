#if !defined(NCMPCPP_NC_SCREEN_SWITCHER_C)
#define NCMPCPP_NC_SCREEN_SWITCHER_C

#include "screens/screen_switcher.h"

#include <stddef.h>

#include "app_controller.h"

NcScreen *
nc_screen_switcher_current(void) {
    return app_controller_current_screen();
}

NcScreen *
nc_screen_switcher_previous(void) {
    return app_controller_previous_screen();
}

bool
nc_screen_switcher_is_current(NcScreen *screen) {
    return app_controller_is_current_screen(screen);
}

bool
nc_screen_switcher_is_visible(NcScreen *screen) {
    return app_controller_is_screen_visible(screen);
}

bool
nc_screen_switcher_switch_to(NcScreen *screen,
                             bool has_to_be_resized) {
    if (screen == NULL) {
        return false;
    }
    nc_screen_set_has_to_be_resized(screen, has_to_be_resized);
    return app_controller_switch_to_screen(screen);
}

bool
nc_screen_switcher_finish_switch(NcScreen *screen) {
    (void)screen;
    return app_controller_last_switch_changed_screen();
}

NcScreenResizeParams
nc_screen_switcher_resize_params(NcScreen *screen,
                                 bool adjust_locked_screen) {
    return app_controller_screen_resize_params(screen, adjust_locked_screen);
}

void
nc_screen_switcher_get_resize_params(NcScreen *screen, int32 *x_offset,
                                     int32 *width,
                                     bool adjust_locked_screen) {
    NcScreenResizeParams params;

    params = nc_screen_switcher_resize_params(screen, adjust_locked_screen);
    if (x_offset) {
        *x_offset = params.x_offset;
    }
    if (width) {
        *width = params.width;
    }
    return;
}

#endif /* NCMPCPP_NC_SCREEN_SWITCHER_C */
