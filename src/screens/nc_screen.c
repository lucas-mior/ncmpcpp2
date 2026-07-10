#include "screens/nc_screen.h"

#include <stddef.h>

#include "settings.h"
#include "ui_state.h"

static int32 nc_screen_registry_index_of(NcScreenRegistry *registry,
                                         NcScreen *screen);
static bool nc_screen_registry_has_type(NcScreenRegistry *registry,
                                        int32 type);
static void nc_screen_registry_clear_refs(NcScreenRegistry *registry,
                                          NcScreen *screen);
static void nc_screen_registry_update_one(NcScreen *screen, void *user);
static void nc_screen_registry_resize_one(NcScreen *screen, void *user);
static void nc_screen_registry_request_resize_one(NcScreen *screen,
                                                  void *user);
static void nc_screen_registry_request_update_one(NcScreen *screen,
                                                  void *user);

void
nc_screen_init(NcScreen *screen, NcScreenCallbacks callbacks,
               void *user, int32 type) {
    screen->callbacks = callbacks;
    screen->user = user;
    screen->type = type;
    screen->has_to_be_resized = false;
    screen->has_to_be_updated = false;
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
    nc_screen_finish_list_change(screen);
    return;
}

void
nc_screen_finish_list_change(NcScreen *screen) {
    if (screen->callbacks.list_change_finished) {
        screen->callbacks.list_change_finished(screen);
    }
    return;
}

bool
nc_screen_is_active_window(NcScreen *screen, NcWindow *window) {
    if (screen == NULL || window == NULL) {
        return false;
    }
    return nc_screen_active_window(screen) == window;
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
    screen->has_to_be_resized = false;
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
nc_screen_set_type(NcScreen *screen, int32 type) {
    screen->type = type;
    return;
}

void
nc_screen_update(NcScreen *screen) {
    if (screen->callbacks.update) {
        screen->callbacks.update(screen);
    }
    screen->has_to_be_updated = false;
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

bool
nc_screen_has_to_be_updated(NcScreen *screen) {
    return screen->has_to_be_updated;
}

void
nc_screen_set_has_to_be_updated(NcScreen *screen,
                                bool has_to_be_updated) {
    screen->has_to_be_updated = has_to_be_updated;
    return;
}

void
nc_screen_request_resize(NcScreen *screen) {
    nc_screen_set_has_to_be_resized(screen, true);
    return;
}

void
nc_screen_request_update(NcScreen *screen) {
    nc_screen_set_has_to_be_updated(screen, true);
    return;
}

void
nc_screen_clear_resize_request(NcScreen *screen) {
    nc_screen_set_has_to_be_resized(screen, false);
    return;
}

void
nc_screen_clear_update_request(NcScreen *screen) {
    nc_screen_set_has_to_be_updated(screen, false);
    return;
}

NcScreenResizeParams
nc_screen_resize_params(NcScreen *screen) {
    NcScreenResizeParams params;

    (void)screen;
    params.x_offset = 0;
    params.width = ui_state_screen_width();
    return params;
}

void
nc_screen_get_resize_params(NcScreen *screen, int64 *x_offset,
                            int64 *width) {
    NcScreenResizeParams params;

    params = nc_screen_resize_params(screen);
    if (x_offset != NULL) {
        *x_offset = params.x_offset;
    }
    if (width != NULL) {
        *width = params.width;
    }
    return;
}

void
nc_screen_draw_vertical_separator(int64 x) {
    color_set(nc_color_pair_number(Config.main_color), NULL);
    mvvline((int32)ui_state_main_start_y(), (int32)x, 0,
            (int32)ui_state_main_height());
    standend();
    refresh();
    return;
}

void
nc_screen_destroy(NcScreen *screen) {
    if (screen->callbacks.destroy) {
        screen->callbacks.destroy(screen);
    }
    return;
}

void *
nc_screen_user(NcScreen *screen) {
    return screen->user;
}

void
nc_screen_registry_init(NcScreenRegistry *registry) {
    for (int32 i = 0; i < NC_SCREEN_REGISTRY_MAX_SCREENS; i += 1) {
        registry->screens[i] = NULL;
    }
    registry->current_screen = NULL;
    registry->previous_screen = NULL;
    registry->locked_screen = NULL;
    registry->inactive_screen = NULL;
    registry->screens_len = 0;
    return;
}

bool
nc_screen_registry_register(NcScreenRegistry *registry, NcScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (nc_screen_registry_is_registered(registry, screen)) {
        return false;
    }
    if (nc_screen_registry_has_type(registry, screen->type)) {
        return false;
    }
    if (registry->screens_len >= NC_SCREEN_REGISTRY_MAX_SCREENS) {
        return false;
    }

    registry->screens[registry->screens_len] = screen;
    registry->screens_len += 1;
    return true;
}

bool
nc_screen_registry_unregister(NcScreenRegistry *registry,
                              NcScreen *screen) {
    int32 index;

    index = nc_screen_registry_index_of(registry, screen);
    if (index < 0) {
        return false;
    }

    for (int32 i = index; i < registry->screens_len - 1; i += 1) {
        registry->screens[i] = registry->screens[i + 1];
    }
    registry->screens_len -= 1;
    registry->screens[registry->screens_len] = NULL;
    nc_screen_registry_clear_refs(registry, screen);
    return true;
}

NcScreen *
nc_screen_registry_find(NcScreenRegistry *registry, int32 type) {
    for (int32 i = 0; i < registry->screens_len; i += 1) {
        if (registry->screens[i]->type == type) {
            return registry->screens[i];
        }
    }
    return NULL;
}

NcScreen *
nc_screen_registry_current(NcScreenRegistry *registry) {
    return registry->current_screen;
}

NcScreen *
nc_screen_registry_previous(NcScreenRegistry *registry) {
    return registry->previous_screen;
}

NcScreen *
nc_screen_registry_locked(NcScreenRegistry *registry) {
    return registry->locked_screen;
}

NcScreen *
nc_screen_registry_inactive(NcScreenRegistry *registry) {
    return registry->inactive_screen;
}

int32
nc_screen_registry_count(NcScreenRegistry *registry) {
    return registry->screens_len;
}

bool
nc_screen_registry_is_registered(NcScreenRegistry *registry,
                                 NcScreen *screen) {
    return nc_screen_registry_index_of(registry, screen) >= 0;
}

bool
nc_screen_registry_is_current(NcScreenRegistry *registry,
                              NcScreen *screen) {
    return registry->current_screen == screen;
}

bool
nc_screen_registry_is_previous(NcScreenRegistry *registry,
                               NcScreen *screen) {
    return registry->previous_screen == screen;
}

bool
nc_screen_registry_request_resize(NcScreenRegistry *registry,
                                  NcScreen *screen) {
    if (!nc_screen_registry_is_registered(registry, screen)) {
        return false;
    }
    nc_screen_request_resize(screen);
    return true;
}

bool
nc_screen_registry_request_update(NcScreenRegistry *registry,
                                  NcScreen *screen) {
    if (!nc_screen_registry_is_registered(registry, screen)) {
        return false;
    }
    nc_screen_request_update(screen);
    return true;
}

void
nc_screen_registry_request_resize_current(NcScreenRegistry *registry) {
    if (registry->current_screen != NULL) {
        nc_screen_request_resize(registry->current_screen);
    }
    return;
}

void
nc_screen_registry_request_resize_visible(NcScreenRegistry *registry) {
    nc_screen_registry_each_visible(
        registry, nc_screen_registry_request_resize_one, NULL);
    return;
}

void
nc_screen_registry_request_resize_all(NcScreenRegistry *registry) {
    for (int32 i = 0; i < registry->screens_len; i += 1) {
        nc_screen_request_resize(registry->screens[i]);
    }
    return;
}

void
nc_screen_registry_request_update_current(NcScreenRegistry *registry) {
    if (registry->current_screen != NULL) {
        nc_screen_request_update(registry->current_screen);
    }
    return;
}

void
nc_screen_registry_request_update_visible(NcScreenRegistry *registry) {
    nc_screen_registry_each_visible(
        registry, nc_screen_registry_request_update_one, NULL);
    return;
}

void
nc_screen_registry_request_update_all(NcScreenRegistry *registry) {
    for (int32 i = 0; i < registry->screens_len; i += 1) {
        nc_screen_request_update(registry->screens[i]);
    }
    return;
}

NcScreenResizeParams
nc_screen_registry_resize_params(NcScreenRegistry *registry,
                                 NcScreen *screen,
                                 bool adjust_locked_screen) {
    NcScreenResizeParams params;
    NcScreen *locked_screen;
    NcScreen *inactive_screen;
    int64 locked_width;

    params = nc_screen_resize_params(screen);
    if (registry == NULL) {
        return params;
    }

    locked_screen = registry->locked_screen;
    inactive_screen = registry->inactive_screen;
    if (locked_screen == NULL || inactive_screen == NULL) {
        return params;
    }
    if (params.width <= 0) {
        return params;
    }

    locked_width = (int64)(params.width * Config.locked_screen_width_part);
    if (locked_width < 0) {
        locked_width = 0;
    }
    if (locked_width >= params.width) {
        locked_width = params.width - 1;
    }

    if (locked_screen == screen) {
        params.width = locked_width;
        return params;
    }

    params.x_offset = locked_width + 1;
    params.width = params.width - locked_width - 1;
    if (adjust_locked_screen) {
        nc_screen_resize(locked_screen);
        nc_screen_refresh(locked_screen);
        nc_screen_draw_vertical_separator(params.x_offset - 1);
    }
    return params;
}

bool
nc_screen_registry_switch_to(NcScreenRegistry *registry,
                             NcScreen *screen) {
    bool is_screen_mergable;

    if (!nc_screen_registry_is_registered(registry, screen)) {
        return false;
    }
    if (registry->current_screen == screen) {
        nc_screen_switch_to(screen);
        return true;
    }

    is_screen_mergable = (registry->locked_screen != NULL)
                         && (nc_screen_is_mergable(screen));
    if (is_screen_mergable) {
        if (registry->locked_screen == screen) {
            registry->inactive_screen = NULL;
        } else {
            registry->inactive_screen = registry->locked_screen;
        }
    }

    if (nc_screen_has_to_be_resized(screen) || is_screen_mergable) {
        nc_screen_resize(screen);
    }

    registry->previous_screen = registry->current_screen;
    registry->current_screen = screen;
    nc_screen_switch_to(screen);
    return true;
}

bool
nc_screen_registry_switch_to_type(NcScreenRegistry *registry,
                                  int32 type) {
    NcScreen *screen;

    screen = nc_screen_registry_find(registry, type);
    if (screen == NULL) {
        return false;
    }
    return nc_screen_registry_switch_to(registry, screen);
}

bool
nc_screen_registry_lock_current(NcScreenRegistry *registry) {
    if (registry->locked_screen != NULL) {
        return false;
    }
    if (registry->current_screen == NULL) {
        return false;
    }
    if (!nc_screen_is_lockable(registry->current_screen)) {
        return false;
    }
    registry->locked_screen = registry->current_screen;
    return true;
}

void
nc_screen_registry_unlock(NcScreenRegistry *registry) {
    bool current_changed;

    if (registry->locked_screen == NULL) {
        return;
    }

    current_changed = false;
    if ((registry->inactive_screen != NULL)
        && (registry->inactive_screen != registry->locked_screen)) {
        registry->previous_screen = registry->current_screen;
        registry->current_screen = registry->inactive_screen;
        current_changed = true;
    }
    registry->locked_screen = NULL;
    registry->inactive_screen = NULL;
    if (current_changed && (registry->current_screen != NULL)) {
        nc_screen_switch_to(registry->current_screen);
    }
    return;
}

bool
nc_screen_registry_is_visible(NcScreenRegistry *registry,
                              NcScreen *screen) {
    if (!nc_screen_registry_is_registered(registry, screen)) {
        return false;
    }
    if ((registry->locked_screen != NULL)
        && (registry->current_screen != NULL)
        && (nc_screen_is_mergable(registry->current_screen))) {
        return (screen == registry->current_screen)
               || (screen == registry->inactive_screen)
               || (screen == registry->locked_screen);
    }
    return screen == registry->current_screen;
}

void
nc_screen_registry_each_visible(NcScreenRegistry *registry,
                                NcScreenEachCallback callback,
                                void *user) {
    if (callback == NULL) {
        return;
    }
    if ((registry->locked_screen != NULL)
        && (registry->current_screen != NULL)
        && (nc_screen_is_mergable(registry->current_screen))) {
        if (registry->current_screen == registry->locked_screen) {
            if (registry->inactive_screen != NULL) {
                callback(registry->inactive_screen, user);
            }
        } else if (registry->locked_screen != NULL) {
            callback(registry->locked_screen, user);
        }
    }
    if (registry->current_screen != NULL) {
        callback(registry->current_screen, user);
    }
    return;
}

void
nc_screen_registry_update_current(NcScreenRegistry *registry) {
    if (registry->current_screen != NULL) {
        nc_screen_update(registry->current_screen);
    }
    return;
}

void
nc_screen_registry_update_visible(NcScreenRegistry *registry) {
    nc_screen_registry_each_visible(registry,
                                    nc_screen_registry_update_one,
                                    NULL);
    return;
}

void
nc_screen_registry_update_all(NcScreenRegistry *registry) {
    for (int32 i = 0; i < registry->screens_len; i += 1) {
        nc_screen_update(registry->screens[i]);
    }
    return;
}

void
nc_screen_registry_resize_current(NcScreenRegistry *registry) {
    if (registry->current_screen != NULL) {
        nc_screen_resize(registry->current_screen);
    }
    return;
}

void
nc_screen_registry_resize_visible(NcScreenRegistry *registry) {
    nc_screen_registry_each_visible(registry,
                                    nc_screen_registry_resize_one,
                                    NULL);
    return;
}

void
nc_screen_registry_resize_all(NcScreenRegistry *registry) {
    for (int32 i = 0; i < registry->screens_len; i += 1) {
        nc_screen_resize(registry->screens[i]);
    }
    return;
}

bool
nc_screen_registry_destroy_screen(NcScreenRegistry *registry,
                                  NcScreen *screen) {
    if (!nc_screen_registry_unregister(registry, screen)) {
        return false;
    }
    nc_screen_destroy(screen);
    return true;
}

bool
nc_screen_registry_destroy_type(NcScreenRegistry *registry,
                                int32 type) {
    NcScreen *screen;

    screen = nc_screen_registry_find(registry, type);
    if (screen == NULL) {
        return false;
    }
    return nc_screen_registry_destroy_screen(registry, screen);
}

void
nc_screen_registry_destroy_all(NcScreenRegistry *registry) {
    while (registry->screens_len > 0) {
        NcScreen *screen;

        screen = registry->screens[registry->screens_len - 1];
        nc_screen_registry_unregister(registry, screen);
        nc_screen_destroy(screen);
    }
    registry->current_screen = NULL;
    registry->previous_screen = NULL;
    registry->locked_screen = NULL;
    registry->inactive_screen = NULL;
    registry->screens_len = 0;
    return;
}

static int32
nc_screen_registry_index_of(NcScreenRegistry *registry,
                            NcScreen *screen) {
    if (screen == NULL) {
        return -1;
    }
    for (int32 i = 0; i < registry->screens_len; i += 1) {
        if (registry->screens[i] == screen) {
            return i;
        }
    }
    return -1;
}

static bool
nc_screen_registry_has_type(NcScreenRegistry *registry, int32 type) {
    if (type == NC_SCREEN_TYPE_UNKNOWN) {
        return false;
    }
    return nc_screen_registry_find(registry, type) != NULL;
}

static void
nc_screen_registry_clear_refs(NcScreenRegistry *registry,
                              NcScreen *screen) {
    if (registry->current_screen == screen) {
        registry->current_screen = NULL;
    }
    if (registry->previous_screen == screen) {
        registry->previous_screen = NULL;
    }
    if (registry->locked_screen == screen) {
        registry->locked_screen = NULL;
    }
    if (registry->inactive_screen == screen) {
        registry->inactive_screen = NULL;
    }
    return;
}

static void
nc_screen_registry_update_one(NcScreen *screen, void *user) {
    (void)user;
    nc_screen_update(screen);
    return;
}

static void
nc_screen_registry_resize_one(NcScreen *screen, void *user) {
    (void)user;
    nc_screen_resize(screen);
    return;
}

static void
nc_screen_registry_request_resize_one(NcScreen *screen,
                                      void *user) {
    (void)user;
    nc_screen_request_resize(screen);
    return;
}

static void
nc_screen_registry_request_update_one(NcScreen *screen,
                                      void *user) {
    (void)user;
    nc_screen_request_update(screen);
    return;
}
