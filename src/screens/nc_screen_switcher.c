#if !defined(NC_SCREEN_SWITCHER_C)
#define NC_SCREEN_SWITCHER_C

#include "cbase.h"

#include "app_controller.h"
#include "screens/nc_screens.h"
#include "title.h"

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

int32
nc_screen_switcher_switch_to(NcScreen *screen, bool has_to_be_resized) {
    int32 status;

    if (screen == NULL) {
        return -EINVAL;
    }
    nc_screen_set_has_to_be_resized(screen, has_to_be_resized);
    status = app_controller_switch_to_screen(screen);
    if ((status == 0) && app_controller_last_switch_has_changed_screen()) {
        ncm_title_draw_current_header();
    }
    return status;
}

void
nc_screen_switcher_finish_switch(NcScreen *screen) {
    (void)screen;
    return;
}

void
nc_screen_switcher_get_resize_params(NcScreen *screen, int32 *x_offset,
                                     int32 *width,
                                     bool adjust_locked_screen) {
    NcScreenResizeParams params;

    params = app_controller_screen_resize_params(screen, adjust_locked_screen);
    *x_offset = params.x_offset;
    *width = params.width;
    return;
}

#endif /* NC_SCREEN_SWITCHER_C */
