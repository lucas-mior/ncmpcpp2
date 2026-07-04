#include "screens/nc_screen.h"

#include <stddef.h>

void
nc_screen_init(NcScreen *screen, NcScreenCallbacks callbacks,
               void *user, int32 type) {
    screen->callbacks = callbacks;
    screen->user = user;
    screen->type = type;
    screen->has_to_be_resized = false;
    return;
}

NcWindow *
nc_screen_active_window(NcScreen *screen) {
    if (screen->callbacks.active_window == NULL) {
        return NULL;
    }
    return screen->callbacks.active_window(screen);
}

void
nc_screen_refresh(NcScreen *screen) {
    if (screen->callbacks.refresh) {
        screen->callbacks.refresh(screen);
    }
    return;
}

void
nc_screen_refresh_window(NcScreen *screen) {
    if (screen->callbacks.refresh_window) {
        screen->callbacks.refresh_window(screen);
    }
    return;
}

void
nc_screen_scroll(NcScreen *screen, enum NcScroll where) {
    if (screen->callbacks.scroll) {
        screen->callbacks.scroll(screen, where);
    }
    return;
}

void
nc_screen_switch_to(NcScreen *screen) {
    if (screen->callbacks.switch_to) {
        screen->callbacks.switch_to(screen);
    }
    return;
}

void
nc_screen_resize(NcScreen *screen) {
    if (screen->callbacks.resize) {
        screen->callbacks.resize(screen);
    }
    return;
}

int32
nc_screen_window_timeout(NcScreen *screen) {
    if (screen->callbacks.window_timeout == NULL) {
        return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
    }
    return screen->callbacks.window_timeout(screen);
}

char *
nc_screen_title(NcScreen *screen) {
    if (screen->callbacks.title == NULL) {
        return NULL;
    }
    return screen->callbacks.title(screen);
}

int32
nc_screen_type(NcScreen *screen) {
    return screen->type;
}

void
nc_screen_update(NcScreen *screen) {
    if (screen->callbacks.update) {
        screen->callbacks.update(screen);
    }
    return;
}

void
nc_screen_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    if (screen->callbacks.mouse_button_pressed) {
        screen->callbacks.mouse_button_pressed(screen, event);
    }
    return;
}

bool
nc_screen_is_lockable(NcScreen *screen) {
    if (screen->callbacks.is_lockable == NULL) {
        return false;
    }
    return screen->callbacks.is_lockable(screen);
}

bool
nc_screen_is_mergable(NcScreen *screen) {
    if (screen->callbacks.is_mergable == NULL) {
        return false;
    }
    return screen->callbacks.is_mergable(screen);
}

bool
nc_screen_has_to_be_resized(NcScreen *screen) {
    return screen->has_to_be_resized;
}

void
nc_screen_set_has_to_be_resized(NcScreen *screen,
                                bool has_to_be_resized) {
    screen->has_to_be_resized = has_to_be_resized;
    return;
}

void *
nc_screen_user(NcScreen *screen) {
    return screen->user;
}
