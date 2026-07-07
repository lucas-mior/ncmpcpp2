#include "app_controller.h"

#include <stddef.h>

#include "app_state.h"

static void app_controller_refresh_one(NcScreen *screen, void *user);

void
app_controller_init(void) {
    app_state_init();
    return;
}

NcScreen *
app_controller_current_screen(void) {
    return app_state_get_screen();
}

NcScreen *
app_controller_previous_screen(void) {
    return app_state_get_previous_screen();
}

NcScreen *
app_controller_locked_screen(void) {
    return app_state_get_locked_screen();
}

NcScreen *
app_controller_inactive_screen(void) {
    return app_state_get_inactive_screen();
}

bool
app_controller_last_switch_changed_screen(void) {
    return app_state_last_switch_changed_screen();
}

bool
app_controller_register_screen(NcScreen *screen) {
    return app_state_register_screen(screen);
}

bool
app_controller_unregister_screen(NcScreen *screen) {
    return app_state_unregister_screen(screen);
}

NcScreen *
app_controller_find_screen_type(int32 type) {
    return app_state_find_screen_type(type);
}

bool
app_controller_is_screen_registered(NcScreen *screen) {
    return app_state_is_screen_registered(screen);
}

bool
app_controller_is_screen_visible(NcScreen *screen) {
    return app_state_is_screen_visible(screen);
}

bool
app_controller_is_current_screen(NcScreen *screen) {
    return app_state_is_current_screen(screen);
}

bool
app_controller_is_previous_screen(NcScreen *screen) {
    return app_state_is_previous_screen(screen);
}

NcScreenResizeParams
app_controller_screen_resize_params(NcScreen *screen,
                                    bool adjust_locked_screen) {
    return app_state_screen_resize_params(screen, adjust_locked_screen);
}

bool
app_controller_request_screen_resize(NcScreen *screen) {
    return app_state_request_screen_resize(screen);
}

bool
app_controller_request_screen_update(NcScreen *screen) {
    return app_state_request_screen_update(screen);
}

void
app_controller_request_current_screen_resize(void) {
    app_state_request_current_screen_resize();
    return;
}

void
app_controller_request_visible_screens_resize(void) {
    app_state_request_visible_screens_resize();
    return;
}

void
app_controller_request_all_screens_resize(void) {
    app_state_request_all_screens_resize();
    return;
}

void
app_controller_request_current_screen_update(void) {
    app_state_request_current_screen_update();
    return;
}

void
app_controller_request_visible_screens_update(void) {
    app_state_request_visible_screens_update();
    return;
}

void
app_controller_request_all_screens_update(void) {
    app_state_request_all_screens_update();
    return;
}

void
app_controller_each_visible_screen(NcScreenEachCallback callback,
                                   void *user) {
    app_state_each_visible_screen(callback, user);
    return;
}

bool
app_controller_switch_to_screen(NcScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return app_state_switch_to_screen(screen);
}

bool
app_controller_switch_to_screen_type(int32 type) {
    return app_state_switch_to_screen_type(type);
}

bool
app_controller_lock_current_screen(void) {
    return app_state_lock_current_screen();
}

void
app_controller_unlock_screen(void) {
    app_state_unlock_screen();
    return;
}

bool
app_controller_can_show_locked_screen(void) {
    return app_state_can_show_locked_screen();
}

bool
app_controller_show_locked_screen(void) {
    return app_state_show_locked_screen();
}

bool
app_controller_can_show_inactive_screen(void) {
    return app_state_can_show_inactive_screen();
}

bool
app_controller_show_inactive_screen(void) {
    return app_state_show_inactive_screen();
}

NcWindow *
app_controller_active_window(void) {
    NcScreen *screen;

    screen = app_controller_current_screen();
    if (screen == NULL) {
        return NULL;
    }
    return nc_screen_active_window(screen);
}

int32
app_controller_current_window_timeout(void) {
    NcScreen *screen;

    screen = app_controller_current_screen();
    if (screen == NULL) {
        return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
    }
    return nc_screen_window_timeout(screen);
}

void
app_controller_update_current_screen(void) {
    app_state_update_current_screen();
    return;
}

void
app_controller_update_visible_screens(void) {
    app_state_update_visible_screens();
    return;
}

void
app_controller_update_all_screens(void) {
    app_state_update_all_screens();
    return;
}

void
app_controller_refresh_current_screen(void) {
    NcScreen *screen;

    screen = app_controller_current_screen();
    if (screen != NULL) {
        nc_screen_refresh(screen);
    }
    return;
}

void
app_controller_refresh_current_window(void) {
    NcScreen *screen;

    screen = app_controller_current_screen();
    if (screen != NULL) {
        nc_screen_refresh_window(screen);
    }
    return;
}

void
app_controller_refresh_visible_screens(void) {
    app_state_each_visible_screen(app_controller_refresh_one, NULL);
    return;
}

void
app_controller_resize_current_screen(void) {
    app_state_resize_current_screen();
    return;
}

void
app_controller_resize_visible_screens(void) {
    app_state_resize_visible_screens();
    return;
}

void
app_controller_resize_all_screens(void) {
    app_state_resize_all_screens();
    return;
}

void
app_controller_scroll_current_screen(enum NcScroll where) {
    NcScreen *screen;

    screen = app_controller_current_screen();
    if (screen != NULL) {
        nc_screen_scroll(screen, where);
    }
    return;
}

void
app_controller_mouse_button_pressed_current(MEVENT event) {
    NcScreen *screen;

    screen = app_controller_current_screen();
    if (screen != NULL) {
        nc_screen_mouse_button_pressed(screen, event);
    }
    return;
}

static void
app_controller_refresh_one(NcScreen *screen, void *user) {
    (void)user;
    nc_screen_refresh(screen);
    return;
}
