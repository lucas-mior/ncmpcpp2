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

NcScreen *
nc_screen_switcher_locked(void) {
    return app_controller_locked_screen();
}

NcScreen *
nc_screen_switcher_inactive(void) {
    return app_controller_inactive_screen();
}

bool
nc_screen_switcher_is_current(NcScreen *screen) {
    return app_controller_is_current_screen(screen);
}

bool
nc_screen_switcher_is_previous(NcScreen *screen) {
    return app_controller_is_previous_screen(screen);
}

bool
nc_screen_switcher_is_visible(NcScreen *screen) {
    return app_controller_is_screen_visible(screen);
}

void
nc_screen_switcher_each_visible(NcScreenEachCallback callback, void *user) {
    app_controller_each_visible_screen(callback, user);
    return;
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
nc_screen_switcher_switch_to_type(int32 type,
                                  bool has_to_be_resized) {
    NcScreen *screen;

    screen = app_controller_find_screen_type(type);
    if (screen == NULL) {
        return false;
    }
    return nc_screen_switcher_switch_to(screen, has_to_be_resized);
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
nc_screen_switcher_get_resize_params(NcScreen *screen, int64 *x_offset,
                                     int64 *width,
                                     bool adjust_locked_screen) {
    NcScreenResizeParams params;

    params = nc_screen_switcher_resize_params(screen, adjust_locked_screen);
    if (x_offset != NULL) {
        *x_offset = params.x_offset;
    }
    if (width != NULL) {
        *width = params.width;
    }
    return;
}

bool
nc_screen_switcher_request_resize(NcScreen *screen) {
    return app_controller_request_screen_resize(screen);
}

bool
nc_screen_switcher_request_update(NcScreen *screen) {
    return app_controller_request_screen_update(screen);
}

void
nc_screen_switcher_request_visible_resize(void) {
    app_controller_request_visible_screens_resize();
    return;
}

void
nc_screen_switcher_request_visible_update(void) {
    app_controller_request_visible_screens_update();
    return;
}
