#include "curses/nc_menu.h"

#include <assert.h>

#include "cbase/array.h"
#include "cbase/cbase.h"

static int64
min_int64(int64 left, int64 right) {
    if (left < right) {
        return left;
    }
    return right;
}

static bool menu_is_separator(NcMenu *menu, void *item);
static bool menu_is_selected(NcMenu *menu, void *item);
static bool menu_is_inactive(NcMenu *menu, void *item);
static bool menu_is_position_highlightable(int64 pos, void *user);
static void menu_print_buffer(NcWindow *window, NcBuffer *buffer);
static void menu_copy_buffer(NcBuffer *dest, NcBuffer *source);

static void ***
menu_array_pointer(NcMenu *menu, enum NcMenuItemSource source) {
    if (source == NC_MENU_ITEMS_FILTERED) {
        return &menu->filtered_items;
    }
    return &menu->all_items;
}

static void **
menu_array(NcMenu *menu, enum NcMenuItemSource source) {
    return *menu_array_pointer(menu, source);
}

static int64
menu_array_count(NcMenu *menu, enum NcMenuItemSource source) {
    void **items;

    items = menu_array(menu, source);
    return ARRAY_LEN(items);
}

static void *
menu_allocate_item(NcMenu *menu) {
    void *item;

    assert(menu->item_callbacks.item_size > 0);
    item = cbase_malloc(menu->item_callbacks.item_size);
    return item;
}

static void *
menu_copy_item(NcMenu *menu, void *source) {
    void *item;

    item = menu_allocate_item(menu);
    if (menu->item_callbacks.copy) {
        menu->item_callbacks.copy(item, source, menu->item_callbacks.user);
    }
    return item;
}

static void *
menu_construct_item(NcMenu *menu) {
    void *item;

    item = menu_allocate_item(menu);
    if (menu->item_callbacks.construct) {
        menu->item_callbacks.construct(item, menu->item_callbacks.user);
    }
    return item;
}

static void
menu_destroy_item(NcMenu *menu, void *item) {
    if (item == NULL) {
        return;
    }
    if (menu->item_callbacks.destroy) {
        menu->item_callbacks.destroy(item, menu->item_callbacks.user);
    }
    cbase_free(item, menu->item_callbacks.item_size);
    return;
}

static bool
menu_is_highlightable(NcMenu *menu, int64 pos,
                      NcMenuHighlightableFunc is_highlightable,
                      void *user) {
    if ((pos < 0) || (pos >= menu->item_count)) {
        return false;
    }
    if (!is_highlightable) {
        return true;
    }
    return is_highlightable(pos, user);
}

static void
scroll_internal(NcMenu *menu, int64 height, enum NcScroll where,
                NcMenuHighlightableFunc is_highlightable, void *user,
                int64 depth) {
    int64 max_beginning;
    int64 max_highlight;
    int64 max_visible_highlight;

    if (menu->item_count <= 0) {
        return;
    }
    if (height <= 0) {
        return;
    }
    if (depth > menu->item_count + height + 4) {
        return;
    }

    max_highlight = menu->item_count - 1;
    if (menu->item_count < height) {
        max_beginning = 0;
    } else {
        max_beginning = menu->item_count - height;
    }
    max_visible_highlight = menu->beginning + height - 1;

    switch (where) {
    case NC_SCROLL_UP:
        if ((menu->highlight <= menu->beginning) && (menu->highlight > 0)) {
            menu->beginning -= 1;
        }
        if (menu->highlight == 0) {
            if (menu->cyclic_scroll_enabled) {
                scroll_internal(menu, height, NC_SCROLL_END, is_highlightable,
                                user, depth + 1);
                return;
            }
            break;
        }
        menu->highlight -= 1;
        if (!menu_is_highlightable(menu, menu->highlight, is_highlightable,
                                   user)) {
            if ((menu->highlight == 0) && !menu->cyclic_scroll_enabled) {
                scroll_internal(menu, height, NC_SCROLL_DOWN,
                                is_highlightable, user, depth + 1);
            } else {
                scroll_internal(menu, height, NC_SCROLL_UP,
                                is_highlightable, user, depth + 1);
            }
        }
        break;
    case NC_SCROLL_DOWN:
        if ((menu->highlight >= max_visible_highlight)
            && (menu->highlight < max_highlight)) {
            menu->beginning += 1;
        }
        if (menu->highlight == max_highlight) {
            if (menu->cyclic_scroll_enabled) {
                scroll_internal(menu, height, NC_SCROLL_HOME,
                                is_highlightable, user, depth + 1);
                return;
            }
            break;
        }
        menu->highlight += 1;
        if (!menu_is_highlightable(menu, menu->highlight, is_highlightable,
                                   user)) {
            if ((menu->highlight == max_highlight)
                && !menu->cyclic_scroll_enabled) {
                scroll_internal(menu, height, NC_SCROLL_UP,
                                is_highlightable, user, depth + 1);
            } else {
                scroll_internal(menu, height, NC_SCROLL_DOWN,
                                is_highlightable, user, depth + 1);
            }
        }
        break;
    case NC_SCROLL_PAGE_UP:
        if (menu->cyclic_scroll_enabled && (menu->highlight == 0)) {
            scroll_internal(menu, height, NC_SCROLL_END, is_highlightable,
                            user, depth + 1);
            return;
        }
        if (menu->highlight < height) {
            menu->highlight = 0;
        } else {
            menu->highlight -= height;
        }
        if (menu->beginning < height) {
            menu->beginning = 0;
        } else {
            menu->beginning -= height;
        }
        if (!menu_is_highlightable(menu, menu->highlight, is_highlightable,
                                   user)) {
            if ((menu->highlight == 0) && !menu->cyclic_scroll_enabled) {
                scroll_internal(menu, height, NC_SCROLL_DOWN,
                                is_highlightable, user, depth + 1);
            } else {
                scroll_internal(menu, height, NC_SCROLL_UP,
                                is_highlightable, user, depth + 1);
            }
        }
        break;
    case NC_SCROLL_PAGE_DOWN:
        if (menu->cyclic_scroll_enabled
            && (menu->highlight == max_highlight)) {
            scroll_internal(menu, height, NC_SCROLL_HOME, is_highlightable,
                            user, depth + 1);
            return;
        }
        menu->highlight += height;
        menu->beginning += height;
        menu->beginning = min_int64(menu->beginning, max_beginning);
        menu->highlight = min_int64(menu->highlight, max_highlight);
        if (!menu_is_highlightable(menu, menu->highlight, is_highlightable,
                                   user)) {
            if ((menu->highlight == max_highlight)
                && !menu->cyclic_scroll_enabled) {
                scroll_internal(menu, height, NC_SCROLL_UP,
                                is_highlightable, user, depth + 1);
            } else {
                scroll_internal(menu, height, NC_SCROLL_DOWN,
                                is_highlightable, user, depth + 1);
            }
        }
        break;
    case NC_SCROLL_HOME:
        menu->highlight = 0;
        menu->beginning = 0;
        if (!menu_is_highlightable(menu, menu->highlight, is_highlightable,
                                   user)) {
            scroll_internal(menu, height, NC_SCROLL_DOWN, is_highlightable,
                            user, depth + 1);
        }
        break;
    case NC_SCROLL_END:
        menu->highlight = max_highlight;
        menu->beginning = max_beginning;
        if (!menu_is_highlightable(menu, menu->highlight, is_highlightable,
                                   user)) {
            scroll_internal(menu, height, NC_SCROLL_UP, is_highlightable,
                            user, depth + 1);
        }
        break;
    }

    if (menu->autocenter_cursor) {
        nc_menu_highlight_position(menu, menu->highlight, height);
    }

    return;
}

void
nc_menu_init(NcMenu *menu) {
    menu->all_items = NULL;
    menu->filtered_items = NULL;
    menu->active_items = NC_MENU_ITEMS_ALL;
    menu->item_callbacks.item_size = 0;
    menu->item_callbacks.construct = NULL;
    menu->item_callbacks.copy = NULL;
    menu->item_callbacks.destroy = NULL;
    menu->item_callbacks.user = NULL;
    menu->display_callbacks.draw = NULL;
    menu->display_callbacks.filter = NULL;
    menu->display_callbacks.is_separator = NULL;
    menu->display_callbacks.is_selected = NULL;
    menu->display_callbacks.is_inactive = NULL;
    menu->display_callbacks.user = NULL;
    menu->action_callbacks.activate = NULL;
    menu->action_callbacks.set_selected = NULL;
    menu->action_callbacks.user = NULL;
    nc_buffer_init(&menu->highlight_prefix);
    nc_buffer_init(&menu->highlight_suffix);
    nc_buffer_init(&menu->selected_prefix);
    nc_buffer_init(&menu->selected_suffix);
    menu->item_count = 0;
    menu->beginning = 0;
    menu->highlight = 0;
    menu->drawn_position = 0;
    menu->highlight_enabled = true;
    menu->cyclic_scroll_enabled = false;
    menu->autocenter_cursor = false;
    return;
}

void
nc_menu_destroy(NcMenu *menu) {
    nc_menu_clear_items(menu);
    nc_buffer_destroy(&menu->highlight_prefix);
    nc_buffer_destroy(&menu->highlight_suffix);
    nc_buffer_destroy(&menu->selected_prefix);
    nc_buffer_destroy(&menu->selected_suffix);
    return;
}

void
nc_menu_copy(NcMenu *dest, NcMenu *source) {
    dest->active_items = NC_MENU_ITEMS_ALL;
    dest->display_callbacks = source->display_callbacks;
    dest->action_callbacks = source->action_callbacks;
    menu_copy_buffer(&dest->highlight_prefix, &source->highlight_prefix);
    menu_copy_buffer(&dest->highlight_suffix, &source->highlight_suffix);
    menu_copy_buffer(&dest->selected_prefix, &source->selected_prefix);
    menu_copy_buffer(&dest->selected_suffix, &source->selected_suffix);
    dest->item_count = 0;
    dest->beginning = source->beginning;
    dest->highlight = source->highlight;
    dest->drawn_position = source->drawn_position;
    dest->highlight_enabled = source->highlight_enabled;
    dest->cyclic_scroll_enabled = source->cyclic_scroll_enabled;
    dest->autocenter_cursor = source->autocenter_cursor;
    return;
}

void
nc_menu_swap(NcMenu *left, NcMenu *right) {
    NcMenu temp;

    temp = *left;
    *left = *right;
    *right = temp;
    return;
}

void
nc_menu_set_item_callbacks(NcMenu *menu, NcMenuItemCallbacks callbacks) {
    assert(menu_array_count(menu, NC_MENU_ITEMS_ALL) == 0);
    assert(menu_array_count(menu, NC_MENU_ITEMS_FILTERED) == 0);
    menu->item_callbacks = callbacks;
    return;
}

void
nc_menu_set_display_callbacks(NcMenu *menu,
                              NcMenuDisplayCallbacks callbacks) {
    menu->display_callbacks = callbacks;
    return;
}

void
nc_menu_set_action_callbacks(NcMenu *menu,
                             NcMenuActionCallbacks callbacks) {
    menu->action_callbacks = callbacks;
    return;
}

void
nc_menu_sync_item_count(NcMenu *menu) {
    menu->item_count = menu_array_count(menu, menu->active_items);
    return;
}

void
nc_menu_set_item_count(NcMenu *menu, int64 item_count) {
    if (item_count < 0) {
        item_count = 0;
    }
    menu->item_count = item_count;
    return;
}

int64
nc_menu_item_count(NcMenu *menu) {
    nc_menu_sync_item_count(menu);
    return menu->item_count;
}

int64
nc_menu_all_item_count(NcMenu *menu) {
    return menu_array_count(menu, NC_MENU_ITEMS_ALL);
}

int64
nc_menu_filtered_item_count(NcMenu *menu) {
    return menu_array_count(menu, NC_MENU_ITEMS_FILTERED);
}

int64
nc_menu_beginning(NcMenu *menu) {
    return menu->beginning;
}

int64
nc_menu_highlight(NcMenu *menu) {
    return menu->highlight;
}

int64
nc_menu_drawn_position(NcMenu *menu) {
    return menu->drawn_position;
}

void
nc_menu_set_drawn_position(NcMenu *menu, int64 drawn_position) {
    menu->drawn_position = drawn_position;
    return;
}

bool
nc_menu_highlight_enabled(NcMenu *menu) {
    return menu->highlight_enabled;
}

void
nc_menu_set_highlight_prefix(NcMenu *menu, NcBuffer *buffer) {
    menu_copy_buffer(&menu->highlight_prefix, buffer);
    return;
}

void
nc_menu_set_highlight_suffix(NcMenu *menu, NcBuffer *buffer) {
    menu_copy_buffer(&menu->highlight_suffix, buffer);
    return;
}

void
nc_menu_set_selected_prefix(NcMenu *menu, NcBuffer *buffer) {
    menu_copy_buffer(&menu->selected_prefix, buffer);
    return;
}

void
nc_menu_set_selected_suffix(NcMenu *menu, NcBuffer *buffer) {
    menu_copy_buffer(&menu->selected_suffix, buffer);
    return;
}

void
nc_menu_set_highlighting(NcMenu *menu, bool state) {
    menu->highlight_enabled = state;
    return;
}

void
nc_menu_set_cyclic_scrolling(NcMenu *menu, bool state) {
    menu->cyclic_scroll_enabled = state;
    return;
}

void
nc_menu_set_centered_cursor(NcMenu *menu, bool state) {
    menu->autocenter_cursor = state;
    return;
}

bool
nc_menu_goto(NcMenu *menu, int64 y,
             NcMenuHighlightableFunc is_highlightable, void *user) {
    int64 pos;

    pos = menu->beginning + y;
    if (!menu_is_highlightable(menu, pos, is_highlightable, user)) {
        return false;
    }
    menu->highlight = pos;
    return true;
}

bool
nc_menu_goto_selectable(NcMenu *menu, int64 y) {
    return nc_menu_goto(menu, y, menu_is_position_highlightable, menu);
}

void
nc_menu_prepare_refresh(NcMenu *menu, int64 height,
                        NcMenuHighlightableFunc is_highlightable,
                        void *user) {
    int64 max_beginning;
    int64 max_visible_highlight;

    nc_menu_sync_item_count(menu);
    if (menu->item_count <= 0) {
        return;
    }
    if (height <= 0) {
        return;
    }

    max_beginning = 0;
    if (menu->item_count > height) {
        max_beginning = menu->item_count - height;
    }

    if (menu->beginning < 0) {
        menu->beginning = 0;
    }
    menu->beginning = min_int64(menu->beginning, max_beginning);

    max_visible_highlight = menu->beginning + height - 1;
    menu->highlight = min_int64(menu->highlight, max_visible_highlight);
    menu->highlight = min_int64(menu->highlight, menu->item_count - 1);
    if (menu->highlight < 0) {
        menu->highlight = 0;
    }

    if (!menu_is_highlightable(menu, menu->highlight, is_highlightable,
                               user)) {
        nc_menu_scroll(menu, height, NC_SCROLL_UP, is_highlightable, user);
        if (!menu_is_highlightable(menu, menu->highlight, is_highlightable,
                                   user)) {
            nc_menu_scroll(menu, height, NC_SCROLL_DOWN, is_highlightable,
                           user);
        }
    }
    return;
}

void
nc_menu_refresh(NcMenu *menu, NcWindow *window, int64 width,
                int64 height) {
    int64 end;
    int64 line;

    nc_menu_sync_item_count(menu);
    if (menu->item_count == 0) {
        nc_window_clear(window);
        nc_window_refresh(window);
        return;
    }

    nc_menu_prepare_refresh(menu, height, menu_is_position_highlightable,
                            menu);

    line = 0;
    end = menu->beginning + height;
    for (int64 pos = menu->beginning; pos < end; pos += 1) {
        void *item;
        bool highlighted;
        bool selected;

        menu->drawn_position = pos;
        nc_window_go_to_xy(window, 0, (int32)line);
        if (pos >= menu->item_count) {
            for (; line < height; line += 1) {
                mvwhline(nc_window_raw(window), (int32)line, 0,
                         NC_KEY_SPACE, (int32)width);
            }
            break;
        }

        item = nc_menu_active_item_at(menu, pos);
        if (menu_is_separator(menu, item)) {
            mvwhline(nc_window_raw(window), (int32)line, 0, 0,
                     (int32)width);
            line += 1;
            continue;
        }

        highlighted = menu->highlight_enabled && (pos == menu->highlight);
        selected = menu_is_selected(menu, item);
        if (highlighted) {
            menu_print_buffer(window, &menu->highlight_prefix);
        }
        if (selected) {
            menu_print_buffer(window, &menu->selected_prefix);
        }
        nc_window_apply_term_manip(window, NC_TERM_CLEAR_TO_EOL);
        if (menu->display_callbacks.draw) {
            menu->display_callbacks.draw(menu, window, item, pos,
                                         menu->display_callbacks.user);
        }
        if (selected) {
            menu_print_buffer(window, &menu->selected_suffix);
        }
        if (highlighted) {
            menu_print_buffer(window, &menu->highlight_suffix);
        }
        line += 1;
    }

    nc_window_refresh(window);
    return;
}

void
nc_menu_scroll(NcMenu *menu, int64 height, enum NcScroll where,
               NcMenuHighlightableFunc is_highlightable, void *user) {
    nc_menu_sync_item_count(menu);
    scroll_internal(menu, height, where, is_highlightable, user, 0);
    return;
}

void
nc_menu_scroll_selectable(NcMenu *menu, int64 height,
                          enum NcScroll where) {
    nc_menu_scroll(menu, height, where, menu_is_position_highlightable,
                   menu);
    return;
}

void
nc_menu_reset(NcMenu *menu) {
    menu->highlight = 0;
    menu->beginning = 0;
    return;
}

void
nc_menu_highlight_position(NcMenu *menu, int64 pos, int64 height) {
    int64 half_height;

    nc_menu_sync_item_count(menu);
    assert(pos >= 0);
    assert(pos < menu->item_count);

    menu->highlight = pos;
    if (height <= 0) {
        menu->beginning = 0;
        return;
    }

    half_height = height/2;
    if (pos < half_height) {
        menu->beginning = 0;
    } else {
        menu->beginning = pos - half_height;
    }
    return;
}

void
nc_menu_resize_all_items(NcMenu *menu, int64 new_size) {
    int64 old_size;

    assert(new_size >= 0);
    old_size = menu_array_count(menu, NC_MENU_ITEMS_ALL);
    while (old_size > new_size) {
        old_size -= 1;
        menu_destroy_item(menu, menu->all_items[old_size]);
        ARRAY_HEADER(menu->all_items)->count -= 1;
    }
    while (old_size < new_size) {
        ARRAY_PUSH(menu->all_items, menu_construct_item(menu));
        old_size += 1;
    }
    nc_menu_sync_item_count(menu);
    return;
}

void
nc_menu_add_item(NcMenu *menu, void *item) {
    ARRAY_PUSH(menu->all_items, menu_copy_item(menu, item));
    nc_menu_sync_item_count(menu);
    return;
}

void
nc_menu_insert_item(NcMenu *menu, int64 pos, void *item) {
    int64 count;
    void *new_item;

    count = menu_array_count(menu, NC_MENU_ITEMS_ALL);
    assert(pos >= 0);
    assert(pos <= count);

    new_item = menu_copy_item(menu, item);
    ARRAY_PUSH(menu->all_items, NULL);
    if (pos < count) {
        cbase_memmove(&menu->all_items[pos + 1], &menu->all_items[pos],
                      (count - pos)*SIZEOF(*menu->all_items));
    }
    menu->all_items[pos] = new_item;
    nc_menu_sync_item_count(menu);
    return;
}

void
nc_menu_clear_items(NcMenu *menu) {
    enum NcMenuItemSource active_items;
    int64 count;

    active_items = menu->active_items;
    count = menu_array_count(menu, NC_MENU_ITEMS_ALL);
    for (int64 i = 0; i < count; i += 1) {
        menu_destroy_item(menu, menu->all_items[i]);
    }
    ARRAY_FREE(menu->all_items);
    ARRAY_FREE(menu->filtered_items);
    menu->active_items = active_items;
    menu->item_count = 0;
    return;
}

void
nc_menu_clear_filtered_items(NcMenu *menu) {
    ARRAY_FREE(menu->filtered_items);
    nc_menu_sync_item_count(menu);
    return;
}

void
nc_menu_add_filtered_item_ref(NcMenu *menu, void *item) {
    assert(item != NULL);
    ARRAY_PUSH(menu->filtered_items, item);
    nc_menu_sync_item_count(menu);
    return;
}

void
nc_menu_apply_filter(NcMenu *menu) {
    nc_menu_clear_filtered_items(menu);
    for (int64 i = 0; i < menu_array_count(menu, NC_MENU_ITEMS_ALL);
         i += 1) {
        void *item;

        item = menu->all_items[i];
        if (menu->display_callbacks.filter
            && menu->display_callbacks.filter(
                menu, item, menu->display_callbacks.user)) {
            nc_menu_add_filtered_item_ref(menu, item);
        }
    }
    nc_menu_show_filtered_items(menu);
    return;
}

void
nc_menu_show_all_items(NcMenu *menu) {
    menu->active_items = NC_MENU_ITEMS_ALL;
    nc_menu_sync_item_count(menu);
    return;
}

void
nc_menu_show_filtered_items(NcMenu *menu) {
    menu->active_items = NC_MENU_ITEMS_FILTERED;
    nc_menu_sync_item_count(menu);
    return;
}

bool
nc_menu_is_filtered(NcMenu *menu) {
    return menu->active_items == NC_MENU_ITEMS_FILTERED;
}

bool
nc_menu_empty(NcMenu *menu) {
    return nc_menu_item_count(menu) == 0;
}

bool
nc_menu_position_is_selectable(NcMenu *menu, int64 pos) {
    void *item;

    if ((pos < 0) || (pos >= nc_menu_item_count(menu))) {
        return false;
    }

    item = nc_menu_active_item_at(menu, pos);
    if (menu_is_separator(menu, item)) {
        return false;
    }
    if (menu_is_inactive(menu, item)) {
        return false;
    }

    return true;
}

bool
nc_menu_position_is_selected(NcMenu *menu, int64 pos) {
    void *item;

    if ((pos < 0) || (pos >= nc_menu_item_count(menu))) {
        return false;
    }

    item = nc_menu_active_item_at(menu, pos);
    return menu_is_selected(menu, item);
}

bool
nc_menu_set_position_selected(NcMenu *menu, int64 pos, bool selected) {
    void *item;

    if (menu->action_callbacks.set_selected == NULL) {
        return false;
    }
    if ((pos < 0) || (pos >= nc_menu_item_count(menu))) {
        return false;
    }
    if (!nc_menu_position_is_selectable(menu, pos)) {
        return false;
    }

    item = nc_menu_active_item_at(menu, pos);
    menu->action_callbacks.set_selected(item, selected,
                                        menu->action_callbacks.user);
    return true;
}

bool
nc_menu_toggle_position_selected(NcMenu *menu, int64 pos) {
    bool selected;

    selected = nc_menu_position_is_selected(menu, pos);
    return nc_menu_set_position_selected(menu, pos, !selected);
}

bool
nc_menu_current_is_selectable(NcMenu *menu) {
    return nc_menu_position_is_selectable(menu, nc_menu_highlight(menu));
}

bool
nc_menu_current_is_selected(NcMenu *menu) {
    return nc_menu_position_is_selected(menu, nc_menu_highlight(menu));
}

bool
nc_menu_set_current_selected(NcMenu *menu, bool selected) {
    return nc_menu_set_position_selected(menu, nc_menu_highlight(menu),
                                         selected);
}

bool
nc_menu_toggle_current_selected(NcMenu *menu) {
    return nc_menu_toggle_position_selected(menu, nc_menu_highlight(menu));
}

bool
nc_menu_activate_position(NcMenu *menu, int64 pos) {
    void *item;

    if (menu->action_callbacks.activate == NULL) {
        return false;
    }
    if ((pos < 0) || (pos >= nc_menu_item_count(menu))) {
        return false;
    }
    if (!nc_menu_position_is_selectable(menu, pos)) {
        return false;
    }

    item = nc_menu_active_item_at(menu, pos);
    menu->action_callbacks.activate(menu, item, pos,
                                    menu->action_callbacks.user);
    return true;
}

bool
nc_menu_activate_current(NcMenu *menu) {
    return nc_menu_activate_position(menu, nc_menu_highlight(menu));
}

void *
nc_menu_item_at(NcMenu *menu, enum NcMenuItemSource source, int64 pos) {
    void **items;
    int64 count;

    items = menu_array(menu, source);
    count = menu_array_count(menu, source);
    assert(pos >= 0);
    assert(pos < count);
    return items[pos];
}

void *
nc_menu_active_item_at(NcMenu *menu, int64 pos) {
    return nc_menu_item_at(menu, menu->active_items, pos);
}

void *
nc_menu_current_item(NcMenu *menu) {
    if (nc_menu_empty(menu)) {
        return NULL;
    }
    return nc_menu_active_item_at(menu, nc_menu_highlight(menu));
}

void
nc_menu_swap_item_slots(NcMenu *menu, enum NcMenuItemSource source,
                        int64 left, int64 right) {
    void **items;
    void *temp;
    int64 count;

    items = menu_array(menu, source);
    count = menu_array_count(menu, source);
    assert(left >= 0);
    assert(right >= 0);
    assert(left < count);
    assert(right < count);

    temp = items[left];
    items[left] = items[right];
    items[right] = temp;
    return;
}

static bool
menu_is_separator(NcMenu *menu, void *item) {
    if (!menu->display_callbacks.is_separator) {
        return false;
    }
    return menu->display_callbacks.is_separator(
        item, menu->display_callbacks.user);
}

static bool
menu_is_selected(NcMenu *menu, void *item) {
    if (!menu->display_callbacks.is_selected) {
        return false;
    }
    return menu->display_callbacks.is_selected(
        item, menu->display_callbacks.user);
}

static bool
menu_is_inactive(NcMenu *menu, void *item) {
    if (!menu->display_callbacks.is_inactive) {
        return false;
    }
    return menu->display_callbacks.is_inactive(
        item, menu->display_callbacks.user);
}

static bool
menu_is_position_highlightable(int64 pos, void *user) {
    NcMenu *menu;
    void *item;

    menu = user;
    if ((pos < 0) || (pos >= menu->item_count)) {
        return false;
    }

    item = nc_menu_active_item_at(menu, pos);
    return !menu_is_separator(menu, item) && !menu_is_inactive(menu, item);
}

static void
menu_print_buffer(NcWindow *window, NcBuffer *buffer) {
    NcBufferProperty *properties;
    char *data;
    int32 property_count;
    int32 property_index;
    int32 len;

    data = nc_buffer_data(buffer);
    len = nc_buffer_len(buffer);
    properties = nc_buffer_properties(buffer);
    property_count = nc_buffer_property_count(buffer);
    property_index = 0;

    for (int32 i = 0;; i += 1) {
        while ((property_index < property_count)
               && (properties[property_index].position == i)) {
            nc_buffer_apply_property(window, &properties[property_index]);
            property_index += 1;
        }
        if (i < len) {
            nc_window_print_char(window, data[i]);
        } else {
            break;
        }
    }
    return;
}

static void
menu_copy_buffer(NcBuffer *dest, NcBuffer *source) {
    nc_buffer_destroy(dest);
    if (source) {
        nc_buffer_copy(dest, source);
    } else {
        nc_buffer_init(dest);
    }
    return;
}
