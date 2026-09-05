#if !defined(NCMPCPP_NC_SCREEN_C)
#define NCMPCPP_NC_SCREEN_C

#include "cbase.h"

#include "screens/nc_screens.h"
#include "settings.h"
#include "ui_state.h"

static NcWindow *nc_screen_callbacks_active_window(NcScreen *);
static void nc_screen_callbacks_refresh(NcScreen *);
static void nc_screen_callbacks_refresh_window(NcScreen *);
static void nc_screen_callbacks_scroll(NcScreen *, enum NcScroll where);
static void nc_screen_callbacks_list_change_finished(NcScreen *);
static bool nc_screen_callbacks_can_run_current(NcScreen *);
static int32 nc_screen_callbacks_run_current(NcScreen *);
static void nc_screen_callbacks_switch_to(NcScreen *);
static void nc_screen_callbacks_resize(NcScreen *);
static int32 nc_screen_callbacks_window_timeout(NcScreen *);
static char *nc_screen_callbacks_title(NcScreen *);
static void nc_screen_callbacks_update(NcScreen *);
static void nc_screen_callbacks_mouse_button_pressed(NcScreen *, MEVENT event);
static bool nc_screen_callbacks_is_lockable(NcScreen *);
static bool nc_screen_callbacks_is_mergable(NcScreen *);
static void nc_screen_callbacks_destroy(NcScreen *);
static bool nc_screen_run_current_is_available(NcScreen *);

const NcScreenOps nc_screen_default_ops = {
    .active_window = nc_screen_default_active_window,
    .refresh = nc_screen_noop_refresh,
    .refresh_window = nc_screen_noop_refresh_window,
    .scroll = nc_screen_noop_scroll,
    .list_change_finished = nc_screen_noop_list_change_finished,
    .can_run_current = nc_screen_default_can_run_current,
    .run_current = nc_screen_default_run_current,
    .switch_to = nc_screen_noop_switch_to,
    .resize = nc_screen_noop_resize,
    .window_timeout = NC_SCREEN_DEFAULT_WINDOW_TIMEOUT,
    .title = nc_screen_default_title,
    .update = nc_screen_noop_update,
    .mouse_button_pressed = nc_screen_noop_mouse_button_pressed,
    .lockable = false,
    .mergable = false,
    .destroy = nc_screen_noop_destroy,
};

static const NcScreenOps nc_screen_callbacks_ops = {
    .active_window = nc_screen_callbacks_active_window,
    .refresh = nc_screen_callbacks_refresh,
    .refresh_window = nc_screen_callbacks_refresh_window,
    .scroll = nc_screen_callbacks_scroll,
    .list_change_finished = nc_screen_callbacks_list_change_finished,
    .can_run_current = nc_screen_callbacks_can_run_current,
    .run_current = nc_screen_callbacks_run_current,
    .switch_to = nc_screen_callbacks_switch_to,
    .resize = nc_screen_callbacks_resize,
    .window_timeout_callback = nc_screen_callbacks_window_timeout,
    .title = nc_screen_callbacks_title,
    .update = nc_screen_callbacks_update,
    .mouse_button_pressed = nc_screen_callbacks_mouse_button_pressed,
    .is_lockable_callback = nc_screen_callbacks_is_lockable,
    .is_mergable_callback = nc_screen_callbacks_is_mergable,
    .destroy = nc_screen_callbacks_destroy,
};

NcWindow *
nc_screen_default_active_window(NcScreen *screen) {
    (void)screen;
    return NULL;
}

void
nc_screen_noop_refresh(NcScreen *screen) {
    (void)screen;
    return;
}

void
nc_screen_noop_refresh_window(NcScreen *screen) {
    (void)screen;
    return;
}

void
nc_screen_noop_scroll(NcScreen *screen, enum NcScroll where) {
    (void)screen;
    (void)where;
    return;
}

void
nc_screen_noop_list_change_finished(NcScreen *screen) {
    (void)screen;
    return;
}

bool
nc_screen_default_can_run_current(NcScreen *screen) {
    (void)screen;
    return false;
}

int32
nc_screen_default_run_current(NcScreen *screen) {
    (void)screen;
    return 0;
}

void
nc_screen_noop_switch_to(NcScreen *screen) {
    (void)screen;
    return;
}

void
nc_screen_noop_resize(NcScreen *screen) {
    (void)screen;
    return;
}

char *
nc_screen_default_title(NcScreen *screen) {
    (void)screen;
    return NULL;
}

void
nc_screen_noop_update(NcScreen *screen) {
    (void)screen;
    return;
}

void
nc_screen_noop_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    (void)screen;
    (void)event;
    return;
}

void
nc_screen_noop_destroy(NcScreen *screen) {
    (void)screen;
    return;
}

void
nc_screen_init(NcScreen *screen, NcScreenCallbacks callbacks,
               void *user, int32 type) {
    nc_screen_init_ops(screen, nc_screen_callbacks_ops, user, type);
    screen->callbacks = callbacks;
    return;
}

void
nc_screen_init_ops(NcScreen *screen, NcScreenOps ops,
                   void *user, int32 type) {
    NcScreenCallbacks callbacks = {0};

    if (ops.active_window == NULL) {
        ops.active_window = nc_screen_default_ops.active_window;
    }
    if (ops.refresh == NULL) {
        ops.refresh = nc_screen_default_ops.refresh;
    }
    if (ops.refresh_window == NULL) {
        ops.refresh_window = nc_screen_default_ops.refresh_window;
    }
    if (ops.scroll == NULL) {
        ops.scroll = nc_screen_default_ops.scroll;
    }
    if (ops.list_change_finished == NULL) {
        ops.list_change_finished = nc_screen_default_ops.list_change_finished;
    }
    if ((ops.can_run_current == NULL) && (ops.run_current != NULL)) {
        ops.can_run_current = nc_screen_run_current_is_available;
    }
    if (ops.can_run_current == NULL) {
        ops.can_run_current = nc_screen_default_ops.can_run_current;
    }
    if (ops.run_current == NULL) {
        ops.run_current = nc_screen_default_ops.run_current;
    }
    if (ops.switch_to == NULL) {
        ops.switch_to = nc_screen_default_ops.switch_to;
    }
    if (ops.resize == NULL) {
        ops.resize = nc_screen_default_ops.resize;
    }
    if (ops.title == NULL) {
        ops.title = nc_screen_default_ops.title;
    }
    if (ops.update == NULL) {
        ops.update = nc_screen_default_ops.update;
    }
    if (ops.mouse_button_pressed == NULL) {
        ops.mouse_button_pressed = nc_screen_default_ops.mouse_button_pressed;
    }
    if (ops.destroy == NULL) {
        ops.destroy = nc_screen_default_ops.destroy;
    }
    if (ops.window_timeout <= 0) {
        ops.window_timeout = nc_screen_default_ops.window_timeout;
    }

    screen->callbacks = callbacks;
    screen->ops_storage = ops;
    screen->ops = &screen->ops_storage;
    screen->user = user;
    screen->type = type;
    screen->has_to_be_resized = false;
    screen->has_to_be_updated = false;

    return;
}

NcWindow *
nc_screen_active_window(NcScreen *screen) {
    return screen->ops->active_window(screen);
}

void
nc_screen_refresh(NcScreen *screen) {
    screen->ops->refresh(screen);
    return;
}

void
nc_screen_refresh_window(NcScreen *screen) {
    screen->ops->refresh_window(screen);
    return;
}

void
nc_screen_scroll(NcScreen *screen, enum NcScroll where) {
    screen->ops->scroll(screen, where);
    nc_screen_finish_list_change(screen);
    return;
}

void
nc_screen_finish_list_change(NcScreen *screen) {
    screen->ops->list_change_finished(screen);
    return;
}

bool
nc_screen_can_run_current(NcScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->ops->can_run_current(screen);
}

int32
nc_screen_run_current(NcScreen *screen) {
    if (screen == NULL) {
        return -EINVAL;
    }
    if (!screen->ops->can_run_current(screen)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    return screen->ops->run_current(screen);
}

void
nc_screen_switch_to(NcScreen *screen) {
    screen->ops->switch_to(screen);
    return;
}

void
nc_screen_resize(NcScreen *screen) {
    screen->ops->resize(screen);
    screen->has_to_be_resized = false;
    return;
}

int32
nc_screen_window_timeout(NcScreen *screen) {
    if (screen->ops->window_timeout_callback) {
        return screen->ops->window_timeout_callback(screen);
    }
    return screen->ops->window_timeout;
}

char *
nc_screen_title(NcScreen *screen) {
    return screen->ops->title(screen);
}

int32
nc_screen_type(NcScreen *screen) {
    return screen->type;
}

void
nc_screen_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    screen->ops->mouse_button_pressed(screen, event);
    return;
}

bool
nc_screen_is_lockable(NcScreen *screen) {
    if (screen->ops->is_lockable_callback) {
        return screen->ops->is_lockable_callback(screen);
    }
    return screen->ops->lockable;
}

bool
nc_screen_is_mergable(NcScreen *screen) {
    if (screen->ops->is_mergable_callback) {
        return screen->ops->is_mergable_callback(screen);
    }
    return screen->ops->mergable;
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
nc_screen_get_resize_params(NcScreen *screen, int32 *x_offset,
                            int32 *width) {
    NcScreenResizeParams params = nc_screen_resize_params(screen);

    if (x_offset) {
        *x_offset = params.x_offset;
    }
    if (width) {
        *width = params.width;
    }
    return;
}

void
nc_screen_draw_vertical_separator(int32 x) {
    color_set((int16)nc_color_pair_number(Config.main_color), NULL);
    mvvline(ui_state_main_start_y(), x, 0, ui_state_main_height());
    standend();
    refresh();
    return;
}

void *
nc_screen_user(NcScreen *screen) {
    return screen->user;
}

static int32
nc_screen_registry_index_of(NcScreenRegistry *registry,
                            NcScreen *screen) {
    ASSERT(screen != NULL);
    for (int32 i = 0; i < registry->screens_len; i += 1) {
        if (registry->screens[i] == screen) {
            return i;
        }
    }
    return -1;
}

static bool
nc_screen_registry_is_registered_unchecked(NcScreenRegistry *registry,
                                           NcScreen *screen) {
    return nc_screen_registry_index_of(registry, screen) >= 0;
}

int32
nc_screen_registry_register(NcScreenRegistry *registry, NcScreen *screen) {
    if ((registry == NULL) || (screen == NULL)) {
        return -EINVAL;
    }
    if (nc_screen_registry_is_registered_unchecked(registry, screen)) {
        return -EEXIST;
    }
    if ((screen->type != NC_SCREEN_TYPE_UNKNOWN)
        && nc_screen_registry_find(registry, screen->type)) {
        return -EEXIST;
    }
    if (registry->screens_len >= NC_SCREEN_REGISTRY_MAX_SCREENS) {
        return -ENOSPC;
    }

    registry->screens[registry->screens_len] = screen;
    registry->screens_len += 1;
    return 0;
}

int32
nc_screen_registry_unregister(NcScreenRegistry *registry,
                              NcScreen *screen) {
    int32 index;

    if ((registry == NULL) || (screen == NULL)) {
        return -EINVAL;
    }
    if ((index = nc_screen_registry_index_of(registry, screen)) < 0) {
        return -ENOENT;
    }

    for (int32 i = index; i < registry->screens_len - 1; i += 1) {
        registry->screens[i] = registry->screens[i + 1];
    }
    registry->screens_len -= 1;
    registry->screens[registry->screens_len] = NULL;

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
    return 0;
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

bool
nc_screen_registry_is_registered(NcScreenRegistry *registry,
                                 NcScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return nc_screen_registry_is_registered_unchecked(registry, screen);
}

bool
nc_screen_registry_is_current(NcScreenRegistry *registry,
                              NcScreen *screen) {
    return registry->current_screen == screen;
}

void
nc_screen_registry_request_resize_current(NcScreenRegistry *registry) {
    if (registry->current_screen) {
        nc_screen_request_resize(registry->current_screen);
    }
    return;
}

void
nc_screen_registry_request_update_current(NcScreenRegistry *registry) {
    if (registry->current_screen) {
        nc_screen_request_update(registry->current_screen);
    }
    return;
}

NcScreenResizeParams
nc_screen_registry_resize_params(NcScreenRegistry *registry,
                                 NcScreen *screen,
                                 bool adjust_locked_screen) {
    NcScreenResizeParams params = nc_screen_resize_params(screen);
    NcScreen *locked_screen;
    NcScreen *inactive_screen;
    int32 locked_width;

    if (registry == NULL) {
        return params;
    }

    locked_screen = registry->locked_screen;
    inactive_screen = registry->inactive_screen;
    if ((locked_screen == NULL) || (inactive_screen == NULL)) {
        return params;
    }
    if (params.width <= 0) {
        return params;
    }

    locked_width = (int32)(
        (double)params.width*Config.locked_screen_width_part);
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

int32
nc_screen_registry_switch_to(NcScreenRegistry *registry,
                             NcScreen *screen) {
    bool is_screen_mergable;

    if ((registry == NULL) || (screen == NULL)) {
        return -EINVAL;
    }
    if (!nc_screen_registry_is_registered_unchecked(registry, screen)) {
        return -ENOENT;
    }
    if (registry->current_screen == screen) {
        nc_screen_switch_to(screen);
        return 0;
    }

    is_screen_mergable = registry->locked_screen
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
    return 0;
}

int32
nc_screen_registry_lock_current(NcScreenRegistry *registry) {
    if (registry == NULL) {
        return -EINVAL;
    }
    if (registry->locked_screen) {
        return -EBUSY;
    }
    if (registry->current_screen == NULL) {
        return -ENOENT;
    }
    if (!nc_screen_is_lockable(registry->current_screen)) {
        return -EPERM;
    }
    registry->locked_screen = registry->current_screen;
    return 0;
}

void
nc_screen_registry_unlock(NcScreenRegistry *registry) {
    bool current_changed;

    if (registry->locked_screen == NULL) {
        return;
    }

    current_changed = false;
    if (registry->inactive_screen
        && (registry->inactive_screen != registry->locked_screen)) {
        registry->previous_screen = registry->current_screen;
        registry->current_screen = registry->inactive_screen;
        current_changed = true;
    }
    registry->locked_screen = NULL;
    registry->inactive_screen = NULL;
    if (current_changed && registry->current_screen) {
        nc_screen_switch_to(registry->current_screen);
    }
    return;
}

bool
nc_screen_registry_is_visible(NcScreenRegistry *registry,
                              NcScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (!nc_screen_registry_is_registered_unchecked(registry, screen)) {
        return false;
    }
    if (registry->locked_screen
        && registry->current_screen
        && (nc_screen_is_mergable(registry->current_screen))) {
        return (screen == registry->current_screen)
               || (screen == registry->inactive_screen)
               || (screen == registry->locked_screen);
    }
    return screen == registry->current_screen;
}

static void
nc_screen_registry_each_visible_unchecked(
    NcScreenRegistry *registry, NcScreenEachCallback *callback, void *user
) {
    ASSERT(callback != NULL);

    if (registry->locked_screen
        && registry->current_screen
        && nc_screen_is_mergable(registry->current_screen)) {
        if (registry->current_screen == registry->locked_screen) {
            if (registry->inactive_screen) {
                callback(registry->inactive_screen, user);
            }
        } else {
            callback(registry->locked_screen, user);
        }
    }
    if (registry->current_screen) {
        callback(registry->current_screen, user);
    }
    return;
}

void
nc_screen_registry_each_visible(NcScreenRegistry *registry,
                                NcScreenEachCallback *callback,
                                void *user) {
    if (callback == NULL) {
        return;
    }
    nc_screen_registry_each_visible_unchecked(registry, callback, user);
    return;
}

static void
nc_screen_registry_resize_one(NcScreen *screen, void *user) {
    (void)user;
    nc_screen_resize(screen);
    return;
}

static void
nc_screen_registry_update_one(NcScreen *screen, void *user) {
    (void)user;
    screen->ops->update(screen);
    screen->has_to_be_updated = false;
    return;
}

void
nc_screen_registry_update_visible(NcScreenRegistry *registry) {
    nc_screen_registry_each_visible_unchecked(
        registry, nc_screen_registry_update_one, NULL
    );
    return;
}

void
nc_screen_registry_resize_current(NcScreenRegistry *registry) {
    if (registry->current_screen) {
        nc_screen_resize(registry->current_screen);
    }
    return;
}

void
nc_screen_registry_resize_visible(NcScreenRegistry *registry) {
    nc_screen_registry_each_visible_unchecked(
        registry, nc_screen_registry_resize_one, NULL
    );
    return;
}

static NcWindow *
nc_screen_callbacks_active_window(NcScreen *screen) {
    if (screen->callbacks.active_window == NULL) {
        return nc_screen_default_active_window(screen);
    }
    return screen->callbacks.active_window(screen);
}

static void
nc_screen_callbacks_refresh(NcScreen *screen) {
    if (screen->callbacks.refresh == NULL) {
        nc_screen_noop_refresh(screen);
        return;
    }
    screen->callbacks.refresh(screen);
    return;
}

static void
nc_screen_callbacks_refresh_window(NcScreen *screen) {
    if (screen->callbacks.refresh_window == NULL) {
        nc_screen_noop_refresh_window(screen);
        return;
    }
    screen->callbacks.refresh_window(screen);
    return;
}

static void
nc_screen_callbacks_scroll(NcScreen *screen, enum NcScroll where) {
    if (screen->callbacks.scroll == NULL) {
        nc_screen_noop_scroll(screen, where);
        return;
    }
    screen->callbacks.scroll(screen, where);
    return;
}

static void
nc_screen_callbacks_list_change_finished(NcScreen *screen) {
    if (screen->callbacks.list_change_finished == NULL) {
        nc_screen_noop_list_change_finished(screen);
        return;
    }
    screen->callbacks.list_change_finished(screen);
    return;
}

static bool
nc_screen_callbacks_can_run_current(NcScreen *screen) {
    if (screen->callbacks.run_current == NULL) {
        return false;
    }
    if (screen->callbacks.can_run_current == NULL) {
        return true;
    }
    return screen->callbacks.can_run_current(screen);
}

static int32
nc_screen_callbacks_run_current(NcScreen *screen) {
    if (screen->callbacks.run_current == NULL) {
        return nc_screen_default_run_current(screen);
    }
    return screen->callbacks.run_current(screen);
}

static void
nc_screen_callbacks_switch_to(NcScreen *screen) {
    if (screen->callbacks.switch_to == NULL) {
        nc_screen_noop_switch_to(screen);
        return;
    }
    screen->callbacks.switch_to(screen);
    return;
}

static void
nc_screen_callbacks_resize(NcScreen *screen) {
    if (screen->callbacks.resize == NULL) {
        nc_screen_noop_resize(screen);
        return;
    }
    screen->callbacks.resize(screen);
    return;
}

static int32
nc_screen_callbacks_window_timeout(NcScreen *screen) {
    if (screen->callbacks.window_timeout == NULL) {
        return screen->ops->window_timeout;
    }
    return screen->callbacks.window_timeout(screen);
}

static char *
nc_screen_callbacks_title(NcScreen *screen) {
    if (screen->callbacks.title == NULL) {
        return nc_screen_default_title(screen);
    }
    return screen->callbacks.title(screen);
}

static void
nc_screen_callbacks_update(NcScreen *screen) {
    if (screen->callbacks.update == NULL) {
        nc_screen_noop_update(screen);
        return;
    }
    screen->callbacks.update(screen);
    return;
}

static void
nc_screen_callbacks_mouse_button_pressed(NcScreen *screen,
                                         MEVENT event) {
    if (screen->callbacks.mouse_button_pressed == NULL) {
        nc_screen_noop_mouse_button_pressed(screen, event);
        return;
    }
    screen->callbacks.mouse_button_pressed(screen, event);
    return;
}

static bool
nc_screen_callbacks_is_lockable(NcScreen *screen) {
    if (screen->callbacks.is_lockable == NULL) {
        return screen->ops->lockable;
    }
    return screen->callbacks.is_lockable(screen);
}

static bool
nc_screen_callbacks_is_mergable(NcScreen *screen) {
    if (screen->callbacks.is_mergable == NULL) {
        return screen->ops->mergable;
    }
    return screen->callbacks.is_mergable(screen);
}

static void
nc_screen_callbacks_destroy(NcScreen *screen) {
    if (screen->callbacks.destroy == NULL) {
        nc_screen_noop_destroy(screen);
        return;
    }
    screen->callbacks.destroy(screen);
    return;
}

static bool
nc_screen_run_current_is_available(NcScreen *screen) {
    return screen->ops->run_current != nc_screen_default_run_current;
}

#endif /* NCMPCPP_NC_SCREEN_C */
