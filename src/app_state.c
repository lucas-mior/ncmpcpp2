#include "app_state.h"

#include <stddef.h>

static NcScreenRegistry screen_registry;
static bool last_switch_changed_screen;

void
app_state_init(void) {
    nc_screen_registry_init(&screen_registry);
    last_switch_changed_screen = false;
    return;
}

NcScreen *
app_state_get_screen(void) {
    return nc_screen_registry_current(&screen_registry);
}

NcScreen *
app_state_get_previous_screen(void) {
    return nc_screen_registry_previous(&screen_registry);
}

NcScreen *
app_state_get_locked_screen(void) {
    return nc_screen_registry_locked(&screen_registry);
}

NcScreen *
app_state_get_inactive_screen(void) {
    return nc_screen_registry_inactive(&screen_registry);
}

bool
app_state_last_switch_changed_screen(void) {
    return last_switch_changed_screen;
}

bool
app_state_register_screen(NcScreen *screen) {
    return nc_screen_registry_register(&screen_registry, screen);
}

bool
app_state_unregister_screen(NcScreen *screen) {
    return nc_screen_registry_unregister(&screen_registry, screen);
}

NcScreen *
app_state_find_screen_type(int32 type) {
    return nc_screen_registry_find(&screen_registry, type);
}

bool
app_state_switch_to_screen(NcScreen *screen) {
    last_switch_changed_screen = screen_registry.current_screen != screen;
    return nc_screen_registry_switch_to(&screen_registry, screen);
}

bool
app_state_switch_to_screen_type(int32 type) {
    NcScreen *screen;

    screen = nc_screen_registry_find(&screen_registry, type);
    last_switch_changed_screen = screen_registry.current_screen != screen;
    return nc_screen_registry_switch_to_type(&screen_registry, type);
}

bool
app_state_lock_current_screen(void) {
    return nc_screen_registry_lock_current(&screen_registry);
}

void
app_state_unlock_screen(void) {
    last_switch_changed_screen = (screen_registry.inactive_screen != NULL)
                                 && (screen_registry.inactive_screen
                                     != screen_registry.locked_screen);
    nc_screen_registry_unlock(&screen_registry);
    return;
}

bool
app_state_can_show_locked_screen(void) {
    return (screen_registry.current_screen != NULL)
           && (screen_registry.locked_screen != NULL)
           && (screen_registry.inactive_screen != NULL)
           && (screen_registry.locked_screen != screen_registry.current_screen)
           && (nc_screen_is_mergable(screen_registry.current_screen));
}

bool
app_state_show_locked_screen(void) {
    if (!app_state_can_show_locked_screen()) {
        return false;
    }

    screen_registry.inactive_screen = screen_registry.current_screen;
    screen_registry.current_screen = screen_registry.locked_screen;
    return true;
}

bool
app_state_can_show_inactive_screen(void) {
    return (screen_registry.current_screen != NULL)
           && (screen_registry.locked_screen != NULL)
           && (screen_registry.inactive_screen != NULL)
           && (screen_registry.locked_screen == screen_registry.current_screen)
           && (nc_screen_is_mergable(screen_registry.current_screen));
}

bool
app_state_show_inactive_screen(void) {
    if (!app_state_can_show_inactive_screen()) {
        return false;
    }

    screen_registry.current_screen = screen_registry.inactive_screen;
    screen_registry.inactive_screen = screen_registry.locked_screen;
    return true;
}

bool
app_state_is_screen_registered(NcScreen *screen) {
    return nc_screen_registry_is_registered(&screen_registry, screen);
}

bool
app_state_is_screen_visible(NcScreen *screen) {
    return nc_screen_registry_is_visible(&screen_registry, screen);
}

void
app_state_each_visible_screen(NcScreenEachCallback callback, void *user) {
    nc_screen_registry_each_visible(&screen_registry, callback, user);
    return;
}

void
app_state_update_current_screen(void) {
    nc_screen_registry_update_current(&screen_registry);
    return;
}

void
app_state_update_visible_screens(void) {
    nc_screen_registry_update_visible(&screen_registry);
    return;
}

void
app_state_update_all_screens(void) {
    nc_screen_registry_update_all(&screen_registry);
    return;
}

void
app_state_resize_current_screen(void) {
    nc_screen_registry_resize_current(&screen_registry);
    return;
}

void
app_state_resize_visible_screens(void) {
    nc_screen_registry_resize_visible(&screen_registry);
    return;
}

void
app_state_resize_all_screens(void) {
    nc_screen_registry_resize_all(&screen_registry);
    return;
}
