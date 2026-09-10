#if !defined(NC_SCREEN_C)
#define NC_SCREEN_C

#include "cbase.h"

#include "screens/nc_screens.h"
#include "settings.h"
#include "ui_state.h"

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

static bool
nc_screen_run_current_is_available(NcScreen *screen) {
    return screen->ops->run_current != nc_screen_default_run_current;
}

void
nc_screen_init_ops(NcScreen *screen, NcScreenOps ops,
                   void *user, enum NcScreenType type) {
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

    screen->ops_storage = ops;
    screen->ops = &screen->ops_storage;
    screen->user = user;
    screen->type = type;
    screen->has_to_be_resized = false;
    screen->has_to_be_updated = false;

    return;
}

static bool
nc_screen_has_capability(NcScreen *screen,
                         enum NcScreenCapabilityFlag capability) {
    if (screen == NULL) {
        return false;
    }
    return (screen->ops->capabilities & capability) != 0;
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

enum NcScreenType
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

void
nc_screen_set_has_to_be_resized(NcScreen *screen, bool has_to_be_resized) {
    screen->has_to_be_resized = has_to_be_resized;
    return;
}

void
nc_screen_set_has_to_be_updated(NcScreen *screen, bool has_to_be_updated) {
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

NcMenu *
nc_screen_current_menu(NcScreen *screen) {
    if (!nc_screen_has_capability(screen, NC_SCREEN_CAPABILITY_MENU)
        || (screen->ops->current_menu == NULL)) {
        return NULL;
    }
    return screen->ops->current_menu(screen);
}

int32
nc_screen_current_menu_height(NcScreen *screen) {
    if (!nc_screen_has_capability(screen, NC_SCREEN_CAPABILITY_MENU)
        || (screen->ops->current_menu_height == NULL)) {
        return ui_state_main_height();
    }
    return screen->ops->current_menu_height(screen);
}

bool
nc_screen_can_filter(NcScreen *screen) {
    if (!nc_screen_has_capability(screen, NC_SCREEN_CAPABILITY_FILTER)
        || (screen->ops->apply_filter == NULL)) {
        return false;
    }
    if (screen->ops->can_filter) {
        return screen->ops->can_filter(screen);
    }
    return true;
}

StringView
nc_screen_current_filter(NcScreen *screen) {
    if (!nc_screen_has_capability(screen, NC_SCREEN_CAPABILITY_FILTER)
        || (screen->ops->current_filter == NULL)) {
        return ncm_string_view(NULL, 0);
    }
    return screen->ops->current_filter(screen);
}

int32
nc_screen_apply_filter(NcScreen *screen, char *pattern, int32 pattern_len,
                       uint32 regex_flags, NcmError *ncm_error) {
    if (!nc_screen_can_filter(screen)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    return screen->ops->apply_filter(screen, pattern, pattern_len,
                                     regex_flags, ncm_error);
}

bool
nc_screen_can_search(NcScreen *screen) {
    if (!nc_screen_has_capability(screen, NC_SCREEN_CAPABILITY_SEARCH)
        || (screen->ops->search == NULL)) {
        return false;
    }
    if (screen->ops->can_search) {
        return screen->ops->can_search(screen);
    }
    return true;
}

bool
nc_screen_can_find(NcScreen *screen) {
    if (!nc_screen_can_search(screen)) {
        return false;
    }
    return nc_screen_has_capability(screen, NC_SCREEN_CAPABILITY_FIND);
}

StringView
nc_screen_current_search_constraint(NcScreen *screen) {
    if (!nc_screen_has_capability(screen, NC_SCREEN_CAPABILITY_SEARCH)
        || (screen->ops->current_search_constraint == NULL)) {
        return ncm_string_view(NULL, 0);
    }
    return screen->ops->current_search_constraint(screen);
}

void
nc_screen_clear_search_constraint(NcScreen *screen) {
    if (!nc_screen_has_capability(screen, NC_SCREEN_CAPABILITY_SEARCH)
        || (screen->ops->clear_search_constraint == NULL)) {
        return;
    }
    screen->ops->clear_search_constraint(screen);
    return;
}

int32
nc_screen_search(NcScreen *screen, enum SearchDirection direction,
                 char *pattern, int32 pattern_len, uint32 regex_flags,
                 bool wrap, bool skip_current, NcmError *ncm_error) {
    if (!nc_screen_can_search(screen)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    return screen->ops->search(screen, direction, pattern, pattern_len,
                               regex_flags, wrap, skip_current, ncm_error);
}

int32
nc_screen_current_song(NcScreen *screen, NcmSong *song) {
    if (!nc_screen_has_capability(screen, NC_SCREEN_CAPABILITY_SONGS)
        || (screen->ops->current_song == NULL)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    return screen->ops->current_song(screen, song);
}

int32
nc_screen_selected_songs(NcScreen *screen, NcmSongArray *songs) {
    if (!nc_screen_has_capability(screen, NC_SCREEN_CAPABILITY_SONGS)
        || (screen->ops->selected_songs == NULL)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    return screen->ops->selected_songs(screen, songs);
}

bool
nc_screen_previous_column_available(NcScreen *screen) {
    if (!nc_screen_has_capability(screen, NC_SCREEN_CAPABILITY_COLUMNS)
        || (screen->ops->previous_column_available == NULL)) {
        return false;
    }
    return screen->ops->previous_column_available(screen);
}

bool
nc_screen_next_column_available(NcScreen *screen) {
    if (!nc_screen_has_capability(screen, NC_SCREEN_CAPABILITY_COLUMNS)
        || (screen->ops->next_column_available == NULL)) {
        return false;
    }
    return screen->ops->next_column_available(screen);
}

int32
nc_screen_previous_column(NcScreen *screen) {
    if (!nc_screen_previous_column_available(screen)
        || (screen->ops->previous_column == NULL)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    return screen->ops->previous_column(screen);
}

int32
nc_screen_next_column(NcScreen *screen) {
    if (!nc_screen_next_column_available(screen)
        || (screen->ops->next_column == NULL)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    return screen->ops->next_column(screen);
}

NcMenu *
nc_screen_tag_menu(NcScreen *screen) {
    if (!nc_screen_has_capability(screen, NC_SCREEN_CAPABILITY_TAGS)
        || (screen->ops->tag_menu == NULL)) {
        return NULL;
    }
    return screen->ops->tag_menu(screen);
}

int32
nc_screen_song_tag_at(NcScreen *screen, int32 pos, enum SongGetter getter,
                      StrBuilder *tag) {
    if (!nc_screen_has_capability(screen, NC_SCREEN_CAPABILITY_TAGS)
        || (screen->ops->song_tag_at == NULL)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    return screen->ops->song_tag_at(screen, pos, getter, tag);
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
nc_screen_get_resize_params(NcScreen *screen, int32 *x_offset, int32 *width) {
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
    color_set((int16)nc_color_pair_number(Config.main_window_color), NULL);
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
nc_screen_registry_index_of(NcScreenRegistry *registry, NcScreen *screen) {
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
nc_screen_registry_unregister(NcScreenRegistry *registry, NcScreen *screen) {
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
nc_screen_registry_find(NcScreenRegistry *registry, enum NcScreenType type) {
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
nc_screen_registry_is_registered(NcScreenRegistry *registry, NcScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return nc_screen_registry_is_registered_unchecked(registry, screen);
}

bool
nc_screen_registry_is_current(NcScreenRegistry *registry, NcScreen *screen) {
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
nc_screen_registry_resize_params(NcScreenRegistry *registry, NcScreen *screen,
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
nc_screen_registry_switch_to(NcScreenRegistry *registry, NcScreen *screen) {
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

    if (screen->has_to_be_resized || is_screen_mergable) {
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
nc_screen_registry_is_visible(NcScreenRegistry *registry, NcScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (!nc_screen_registry_is_registered_unchecked(registry, screen)) {
        return false;
    }
    if (registry->locked_screen && registry->current_screen
        && (nc_screen_is_mergable(registry->current_screen))) {
        return (screen == registry->current_screen)
               || (screen == registry->inactive_screen)
               || (screen == registry->locked_screen);
    }
    return screen == registry->current_screen;
}

static void
nc_screen_registry_each_visible_unchecked(NcScreenRegistry *registry,
                                          NcScreenEachCallback *callback,
                                          void *user) {
    ASSERT(callback != NULL);

    if (registry->locked_screen && registry->current_screen
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
                                NcScreenEachCallback *callback, void *user) {
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
    nc_screen_registry_each_visible_unchecked(registry,
                                          nc_screen_registry_update_one, NULL);
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
    nc_screen_registry_each_visible_unchecked(registry,
                                          nc_screen_registry_resize_one, NULL);
    return;
}

#endif /* NC_SCREEN_C */
