#if !defined(NC_MENU_C)
#define NC_MENU_C

#include "cbase.h"

#include "curses/nc_curses.h"

#define NC_MENU_SCROLL_DEPTH_MARGIN 4

static bool menu_is_separator(NcMenu *menu, void *item);
static bool menu_is_selected(NcMenu *menu, void *item);
static bool menu_is_inactive(NcMenu *menu, void *item);
static bool menu_is_position_highlightable(int32 pos, void *user);
static void menu_print_buffer(NcWindow *window, NcBuffer *buffer);
static void menu_copy_buffer(NcBuffer *dest, NcBuffer *source);
static void *menu_construct_item(NcMenu *menu);

static uint32 menu_default_item_flags(void);
static uint32 menu_flags_for_item(NcMenu *menu, void *item);
static uint32 *menu_flags_array(NcMenu *menu,
                                enum NcMenuItemSource source);
static int32 menu_item_index(NcMenu *menu, enum NcMenuItemSource source,
                             void *item);
static void menu_clamp_navigation(NcMenu *menu);
static void menu_set_flags_for_item(NcMenu *menu, void *item,
                                    uint32 flags);

static void **
menu_array(NcMenu *menu, enum NcMenuItemSource source) {
    if (source == NC_MENU_ITEMS_FILTERED) {
        return menu->filtered_items;
    }
    return menu->all_items;
}

static uint32 *
menu_flags_array(NcMenu *menu, enum NcMenuItemSource source) {
    if (source == NC_MENU_ITEMS_FILTERED) {
        return menu->filtered_item_flags;
    }
    return menu->all_item_flags;
}

static int32
menu_array_count(NcMenu *menu, enum NcMenuItemSource source) {
    void **items = menu_array(menu, source);
    return ARRAY_LEN(items);
}

static void *
menu_copy_item(NcMenu *menu, void *source) {
    void *item = menu_construct_item(menu);

    if (menu->item_callbacks.copy) {
        menu->item_callbacks.copy(item, source, menu->item_callbacks.user);
    }
    return item;
}

static void *
menu_construct_item(NcMenu *menu) {
    void *item;

    ASSERT_POSITIVE(menu->item_callbacks.item_size);
    item = malloc2(menu->item_callbacks.item_size);

    if (menu->item_callbacks.construct) {
        menu->item_callbacks.construct(item, menu->item_callbacks.user);
    } else {
        memset64(item, 0, menu->item_callbacks.item_size);
    }

    return item;
}

static void
menu_destroy_item(NcMenu *menu, void *item) {
    if (menu->item_callbacks.destroy) {
        menu->item_callbacks.destroy(item, menu->item_callbacks.user);
    }
    free2(item, menu->item_callbacks.item_size);
    return;
}

static bool
menu_is_highlightable(NcMenu *menu, int32 pos,
                      NcMenuPositionIsHighlightableFunc *is_highlightable,
                      void *user) {
    if ((pos < 0) || (pos >= menu->item_count)) {
        return false;
    }
    if (is_highlightable == NULL) {
        return true;
    }
    return is_highlightable(pos, user);
}

static bool
menu_position_is_selectable(NcMenu *menu, int32 pos) {
    void *item = nc_menu_active_item_at(menu, pos);
    uint32 flags = menu_flags_for_item(menu, item);

    if (!(flags & NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    if (menu_is_separator(menu, item)) {
        return false;
    }
    if (menu_is_inactive(menu, item)) {
        return false;
    }
    return true;
}

static bool
menu_position_is_separator(NcMenu *menu, int32 pos) {
    return menu_is_separator(menu, nc_menu_active_item_at(menu, pos));
}

static bool
menu_position_is_inactive(NcMenu *menu, int32 pos) {
    return menu_is_inactive(menu, nc_menu_active_item_at(menu, pos));
}

static bool
menu_position_is_selected(NcMenu *menu, int32 pos) {
    return menu_is_selected(menu, nc_menu_active_item_at(menu, pos));
}

static void
menu_set_position_selected(NcMenu *menu, int32 pos, bool selected) {
    void *item = nc_menu_active_item_at(menu, pos);
    uint32 flags;

    if (menu->action_callbacks.set_selected) {
        menu->action_callbacks.set_selected(item, selected,
                                            menu->action_callbacks.user);
    }

    flags = menu_flags_for_item(menu, item);
    if (selected) {
        flags |= NC_MENU_ITEM_SELECTED;
    } else {
        flags &= ~NC_MENU_ITEM_SELECTED;
    }
    menu_set_flags_for_item(menu, item, flags);
    return;
}

static void
scroll_internal(NcMenu *menu, int32 height, enum NcScroll where,
                NcMenuPositionIsHighlightableFunc *is_highlightable, void *user,
                int32 depth) {
    int32 max_beginning;
    int32 max_highlight;
    int32 max_visible_highlight;

    if (menu->item_count <= 0) {
        return;
    }
    if (height <= 0) {
        return;
    }
    if (depth > menu->item_count + height + NC_MENU_SCROLL_DEPTH_MARGIN) {
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
        menu->beginning = MIN(menu->beginning, max_beginning);
        menu->highlight = MIN(menu->highlight, max_highlight);
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
    case NC_SCROLL_COUNT:
    default:
        break;
    }

    if (menu->autocenter_cursor) {
        nc_menu_highlight_position(menu, menu->highlight, height);
    }

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
    dest->all_items = NULL;
    dest->filtered_items = NULL;
    dest->all_item_flags = NULL;
    dest->filtered_item_flags = NULL;
    dest->active_items = NC_MENU_ITEMS_ALL;
    dest->item_callbacks = source->item_callbacks;
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
    dest->highlight_disabled = source->highlight_disabled;
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
    ASSERT_NON_POSITIVE(menu_array_count(menu, NC_MENU_ITEMS_ALL));
    ASSERT_NON_POSITIVE(menu_array_count(menu, NC_MENU_ITEMS_FILTERED));
    menu->item_callbacks = callbacks;
    return;
}

void
nc_menu_set_display_callbacks(NcMenu *menu, NcMenuDisplayCallbacks callbacks) {
    menu->display_callbacks = callbacks;
    return;
}

void
nc_menu_set_action_callbacks(NcMenu *menu, NcMenuActionCallbacks callbacks) {
    menu->action_callbacks = callbacks;
    return;
}

void
nc_menu_sync_item_count(NcMenu *menu) {
    menu->item_count = menu_array_count(menu, menu->active_items);
    menu_clamp_navigation(menu);
    return;
}

int32
nc_menu_item_count(NcMenu *menu) {
    nc_menu_sync_item_count(menu);
    return menu->item_count;
}

int32
nc_menu_all_item_count(NcMenu *menu) {
    return menu_array_count(menu, NC_MENU_ITEMS_ALL);
}

int32
nc_menu_filtered_item_count(NcMenu *menu) {
    return menu_array_count(menu, NC_MENU_ITEMS_FILTERED);
}

int32
nc_menu_highlight(NcMenu *menu) {
    return menu->highlight;
}

bool
nc_menu_highlight_is_enabled(NcMenu *menu) {
    return !menu->highlight_disabled;
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
    menu->highlight_disabled = !state;
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

int32
nc_menu_goto_selectable(NcMenu *menu, int32 y) {
    int32 pos = menu->beginning + y;

    if (!menu_is_highlightable(menu, pos, menu_is_position_highlightable,
                               menu)) {
        return -NCM_ERROR_NOT_FOUND;
    }
    menu->highlight = pos;
    return 0;
}

int32
nc_menu_search_selectable(NcMenu *menu, int32 height, bool forward,
                          bool wrap, bool skip_current,
                          NcMenuPositionMatchesFunc *matches, void *user,
                          int32 *found_pos) {
    int32 count;
    int32 current;
    int32 start;
    int32 step;

    if ((menu == NULL) || (matches == NULL)) {
        return -EINVAL;
    }

    count = nc_menu_item_count(menu);
    if (count <= 0) {
        return -NCM_ERROR_NOT_FOUND;
    }

    current = nc_menu_highlight(menu);
    if ((current < 0) || (current >= count)) {
        current = -1;
        if (forward) {
            start = 0;
        } else {
            start = count - 1;
        }
    } else {
        start = current;
        if (skip_current) {
            if (forward) {
                start += 1;
            } else {
                start -= 1;
            }
        }
    }

    if (forward) {
        step = 1;
    } else {
        step = -1;
    }

    for (int32 i = 0; i < count; i += 1) {
        int32 pos = start + step*i;

        if (wrap) {
            pos %= count;
            if (pos < 0) {
                pos += count;
            }
        } else if ((pos < 0) || (pos >= count)) {
            break;
        }
        if (skip_current && (current >= 0) && (pos == current)) {
            continue;
        }
        if (!menu_position_is_selectable(menu, pos)) {
            continue;
        }
        if (!matches(menu, pos, user)) {
            continue;
        }
        if (!menu_position_is_selectable(menu, pos)) {
            continue;
        }
        nc_menu_highlight_position(menu, pos, height);
        if (found_pos != NULL) {
            *found_pos = pos;
        }
        return 0;
    }

    return -NCM_ERROR_NOT_FOUND;
}

void
nc_menu_prepare_refresh(NcMenu *menu, int32 height,
                        NcMenuPositionIsHighlightableFunc *is_highlightable,
                        void *user) {
    int32 max_beginning;
    int32 max_visible_highlight;

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
    if (menu->beginning > max_beginning) {
        menu->beginning = max_beginning;
    }

    max_visible_highlight = menu->beginning + height - 1;
    if (menu->highlight > max_visible_highlight) {
        menu->highlight = max_visible_highlight;
    }
    if (menu->highlight >= menu->item_count) {
        menu->highlight = menu->item_count - 1;
    }
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
nc_menu_refresh(NcMenu *menu, NcWindow *window, int32 width, int32 height) {
    int32 end;
    int32 line;

    nc_menu_sync_item_count(menu);
    if (menu->item_count <= 0) {
        nc_window_clear(window);
        nc_window_refresh(window);
        return;
    }

    nc_menu_prepare_refresh(menu, height, menu_is_position_highlightable, menu);

    line = 0;
    end = menu->beginning + height;
    for (int32 pos = menu->beginning; pos < end; pos += 1) {
        void *item;
        bool highlighted;
        bool selected;

        menu->drawn_position = pos;
        nc_window_go_to_xy(window, 0, line);
        if (pos >= menu->item_count) {
            for (; line < height; line += 1) {
                mvwhline(nc_window_raw(window), line, 0, NC_KEY_SPACE, width);
            }
            break;
        }

        item = nc_menu_active_item_at(menu, pos);
        if (menu_is_separator(menu, item)) {
            mvwhline(nc_window_raw(window), line, 0, 0, width);
            line += 1;
            continue;
        }

        highlighted = !menu->highlight_disabled && (pos == menu->highlight);
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
nc_menu_scroll(NcMenu *menu, int32 height, enum NcScroll where,
               NcMenuPositionIsHighlightableFunc *is_highlightable,
               void *user) {
    nc_menu_sync_item_count(menu);
    scroll_internal(menu, height, where, is_highlightable, user, 0);
    return;
}

void
nc_menu_scroll_selectable(NcMenu *menu, int32 height,
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
nc_menu_highlight_position(NcMenu *menu, int32 pos, int32 height) {
    int32 half_height;

    nc_menu_sync_item_count(menu);
    ASSERT_NON_NEGATIVE(pos);
    ASSERT_LESS(pos, menu->item_count);

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
nc_menu_add_item(NcMenu *menu, void *item) {
    nc_menu_add_item_with_flags(menu, item, menu_default_item_flags());
    return;
}

void
nc_menu_add_item_with_flags(NcMenu *menu, void *item, uint32 flags) {
    ARRAY_PUSH(menu->all_items, menu_copy_item(menu, item));
    ARRAY_PUSH(menu->all_item_flags, flags);
    nc_menu_sync_item_count(menu);
    return;
}

void
nc_menu_add_separator(NcMenu *menu) {
    void *item = menu_construct_item(menu);
    ARRAY_PUSH(menu->all_items, item);
    ARRAY_PUSH(menu->all_item_flags, NC_MENU_ITEM_SEPARATOR);
    nc_menu_sync_item_count(menu);
    return;
}

void
nc_menu_insert_item_with_flags(NcMenu *menu, int32 pos, void *item,
                               uint32 flags) {
    int32 count;
    void *new_item;

    count = menu_array_count(menu, NC_MENU_ITEMS_ALL);
    ASSERT_NON_NEGATIVE(pos);
    ASSERT_LESS_EQUAL(pos, count);

    new_item = menu_copy_item(menu, item);
    ARRAY_PUSH(menu->all_items, NULL);
    ARRAY_PUSH(menu->all_item_flags, NC_MENU_ITEM_NONE);
    if (pos < count) {
        memmove64(&menu->all_items[pos + 1], &menu->all_items[pos],
                  (count - pos)*SIZEOF(*menu->all_items));
        memmove64(&menu->all_item_flags[pos + 1],
                  &menu->all_item_flags[pos],
                  (count - pos)*SIZEOF(*menu->all_item_flags));
    }
    menu->all_items[pos] = new_item;
    menu->all_item_flags[pos] = flags;
    nc_menu_clear_filtered_items(menu);
    nc_menu_sync_item_count(menu);
    return;
}

int32
nc_menu_remove_item(NcMenu *menu, enum NcMenuItemSource source,
                    int32 pos) {
    void **items = menu_array(menu, source);
    uint32 *flags = menu_flags_array(menu, source);
    int32 count = menu_array_count(menu, source);

    if ((pos < 0) || (pos >= count)) {
        return -ERANGE;
    }

    if (source == NC_MENU_ITEMS_ALL) {
        menu_destroy_item(menu, items[pos]);
    }
    if (pos < count - 1) {
        memmove64(&items[pos], &items[pos + 1],
                  (count - pos - 1)*SIZEOF(*items));
        memmove64(&flags[pos], &flags[pos + 1],
                  (count - pos - 1)*SIZEOF(*flags));
    }
    ARRAY_HEADER(items)->count -= 1;
    ARRAY_HEADER(flags)->count -= 1;
    if (source == NC_MENU_ITEMS_ALL) {
        nc_menu_clear_filtered_items(menu);
    }
    nc_menu_sync_item_count(menu);
    return 0;
}

int32
nc_menu_replace_item(NcMenu *menu, enum NcMenuItemSource source,
                     int32 pos, void *item) {
    void *old_item;
    void *new_item;
    int32 all_pos;

    if ((pos < 0) || (pos >= menu_array_count(menu, source))) {
        return -ERANGE;
    }

    old_item = menu_array(menu, source)[pos];
    new_item = menu_copy_item(menu, item);
    all_pos = menu_item_index(menu, NC_MENU_ITEMS_ALL, old_item);
    ASSERT_NON_NEGATIVE(all_pos);
    menu->all_items[all_pos] = new_item;
    for (int32 i = 0; i < menu_array_count(menu, NC_MENU_ITEMS_FILTERED);
         i += 1) {
        if (menu->filtered_items[i] == old_item) {
            menu->filtered_items[i] = new_item;
        }
    }
    menu_destroy_item(menu, old_item);
    return 0;
}

void
nc_menu_clear_items(NcMenu *menu) {
    enum NcMenuItemSource active_items = menu->active_items;
    int32 count = menu_array_count(menu, NC_MENU_ITEMS_ALL);

    for (int32 i = 0; i < count; i += 1) {
        menu_destroy_item(menu, menu->all_items[i]);
    }

    ARRAY_FREE(menu->all_items);
    ARRAY_FREE(menu->filtered_items);
    ARRAY_FREE(menu->all_item_flags);
    ARRAY_FREE(menu->filtered_item_flags);

    menu->active_items = active_items;
    menu->item_count = 0;
    menu_clamp_navigation(menu);

    return;
}

void
nc_menu_clear_filtered_items(NcMenu *menu) {
    ARRAY_FREE(menu->filtered_items);
    ARRAY_FREE(menu->filtered_item_flags);
    nc_menu_sync_item_count(menu);
    return;
}

void
nc_menu_apply_filter(NcMenu *menu) {
    nc_menu_clear_filtered_items(menu);
    for (int32 i = 0; i < menu_array_count(menu, NC_MENU_ITEMS_ALL); i += 1) {
        void *item = menu->all_items[i];

        if (menu->display_callbacks.matches_filter
            && menu->display_callbacks.matches_filter(
                menu, item, menu->display_callbacks.user)) {
            ASSERT(item);
            ARRAY_PUSH(menu->filtered_items, item);
            ARRAY_PUSH(menu->filtered_item_flags,
                       menu_flags_for_item(menu, item));
            nc_menu_sync_item_count(menu);
        }
    }
    menu->active_items = NC_MENU_ITEMS_FILTERED;
    nc_menu_sync_item_count(menu);
    return;
}

void
nc_menu_show_all_items(NcMenu *menu) {
    menu->active_items = NC_MENU_ITEMS_ALL;
    nc_menu_sync_item_count(menu);
    return;
}

bool
nc_menu_is_filtered(NcMenu *menu) {
    return menu->active_items == NC_MENU_ITEMS_FILTERED;
}

bool
nc_menu_is_empty(NcMenu *menu) {
    return nc_menu_item_count(menu) <= 0;
}

bool
nc_menu_position_is_selectable(NcMenu *menu, int32 pos) {
    if ((pos < 0) || (pos >= nc_menu_item_count(menu))) {
        return false;
    }
    return menu_position_is_selectable(menu, pos);
}

bool
nc_menu_position_is_separator(NcMenu *menu, int32 pos) {
    if ((pos < 0) || (pos >= nc_menu_item_count(menu))) {
        return false;
    }
    return menu_position_is_separator(menu, pos);
}

bool
nc_menu_position_is_inactive(NcMenu *menu, int32 pos) {
    if ((pos < 0) || (pos >= nc_menu_item_count(menu))) {
        return false;
    }
    return menu_position_is_inactive(menu, pos);
}

bool
nc_menu_position_is_selected(NcMenu *menu, int32 pos) {
    if ((pos < 0) || (pos >= nc_menu_item_count(menu))) {
        return false;
    }
    return menu_position_is_selected(menu, pos);
}

int32
nc_menu_set_position_selected(NcMenu *menu, int32 pos, bool selected) {
    if ((pos < 0) || (pos >= nc_menu_item_count(menu))) {
        return -ERANGE;
    }
    if (!menu_position_is_selectable(menu, pos)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    menu_set_position_selected(menu, pos, selected);
    return 0;
}

void
nc_menu_clear_selection(NcMenu *menu) {
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (menu_position_is_selectable(menu, i)) {
            menu_set_position_selected(menu, i, false);
        }
    }
    return;
}

bool
nc_menu_has_selected(NcMenu *menu) {
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (menu_position_is_selected(menu, i)) {
            return true;
        }
    }
    return false;
}

int32
nc_menu_selected_count(NcMenu *menu) {
    int32 count = 0;

    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (menu_position_is_selected(menu, i)) {
            count += 1;
        }
    }
    return count;
}

bool
nc_menu_current_is_selectable(NcMenu *menu) {
    return nc_menu_position_is_selectable(menu, nc_menu_highlight(menu));
}

int32
nc_menu_set_current_selected(NcMenu *menu, bool selected) {
    return nc_menu_set_position_selected(menu, nc_menu_highlight(menu),
                                         selected);
}

int32
nc_menu_toggle_current_selected(NcMenu *menu) {
    int32 pos = nc_menu_highlight(menu);
    bool selected = nc_menu_position_is_selected(menu, pos);

    return nc_menu_set_position_selected(menu, pos, !selected);
}

int32
nc_menu_activate_current(NcMenu *menu) {
    int32 pos = nc_menu_highlight(menu);
    void *item;

    if (menu->action_callbacks.activate == NULL) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if ((pos < 0) || (pos >= nc_menu_item_count(menu))) {
        return -ERANGE;
    }
    if (!menu_position_is_selectable(menu, pos)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    item = nc_menu_active_item_at(menu, pos);
    menu->action_callbacks.activate(menu, item, pos,
                                    menu->action_callbacks.user);
    return 0;
}

uint32
nc_menu_item_flags_at(NcMenu *menu, enum NcMenuItemSource source,
                      int32 pos) {
    uint32 *flags = menu_flags_array(menu, source);
    int32 count = menu_array_count(menu, source);

    ASSERT_NON_NEGATIVE(pos);
    ASSERT_LESS(pos, count);

    return flags[pos];
}

int32
nc_menu_set_item_flags_at(NcMenu *menu, enum NcMenuItemSource source,
                          int32 pos, uint32 flags) {
    void *item;

    if ((pos < 0) || (pos >= menu_array_count(menu, source))) {
        return -ERANGE;
    }
    item = menu_array(menu, source)[pos];
    menu_set_flags_for_item(menu, item, flags);
    return 0;
}

void *
nc_menu_item_at(NcMenu *menu, enum NcMenuItemSource source, int32 pos) {
    void **items = menu_array(menu, source);
    int32 count = menu_array_count(menu, source);

    ASSERT_NON_NEGATIVE(pos);
    ASSERT_LESS(pos, count);

    return items[pos];
}

void *
nc_menu_active_item_at(NcMenu *menu, int32 pos) {
    return nc_menu_item_at(menu, menu->active_items, pos);
}

void *
nc_menu_current_item(NcMenu *menu) {
    if (nc_menu_is_empty(menu)) {
        return NULL;
    }
    return nc_menu_active_item_at(menu, nc_menu_highlight(menu));
}

void
nc_menu_swap_item_slots(NcMenu *menu, enum NcMenuItemSource source,
                        int32 left, int32 right) {
    uint32 *flags;
    void **items;
    void *temp;
    uint32 temp_flags;
    int32 count;

    items = menu_array(menu, source);
    flags = menu_flags_array(menu, source);
    count = menu_array_count(menu, source);

    ASSERT_NON_NEGATIVE(left);
    ASSERT_NON_NEGATIVE(right);
    ASSERT_LESS(left, count);
    ASSERT_LESS(right, count);

    temp = items[left];
    items[left] = items[right];
    items[right] = temp;

    temp_flags = flags[left];
    flags[left] = flags[right];
    flags[right] = temp_flags;
    return;
}

static bool
menu_is_separator(NcMenu *menu, void *item) {
    if (menu_flags_for_item(menu, item) & NC_MENU_ITEM_SEPARATOR) {
        return true;
    }
    if (menu->display_callbacks.is_separator == NULL) {
        return false;
    }
    return menu->display_callbacks.is_separator(
        item, menu->display_callbacks.user);
}

static bool
menu_is_selected(NcMenu *menu, void *item) {
    if (menu_flags_for_item(menu, item) & NC_MENU_ITEM_SELECTED) {
        return true;
    }
    if (menu->display_callbacks.is_selected == NULL) {
        return false;
    }
    return menu->display_callbacks.is_selected(item,
                                               menu->display_callbacks.user);
}

static bool
menu_is_inactive(NcMenu *menu, void *item) {
    if (menu_flags_for_item(menu, item) & NC_MENU_ITEM_INACTIVE) {
        return true;
    }
    if (menu->display_callbacks.is_inactive == NULL) {
        return false;
    }
    return menu->display_callbacks.is_inactive(item,
                                               menu->display_callbacks.user);
}

static uint32
menu_default_item_flags(void) {
    return NC_MENU_ITEM_SELECTABLE;
}

static uint32
menu_flags_for_item(NcMenu *menu, void *item) {
    int32 pos = menu_item_index(menu, NC_MENU_ITEMS_ALL, item);
    ASSERT_NON_NEGATIVE(pos);
    return menu->all_item_flags[pos];
}

static int32
menu_item_index(NcMenu *menu, enum NcMenuItemSource source, void *item) {
    void **items = menu_array(menu, source);
    int32 count = menu_array_count(menu, source);

    for (int32 i = 0; i < count; i += 1) {
        if (items[i] == item) {
            return i;
        }
    }
    return -1;
}

static void
menu_clamp_navigation(NcMenu *menu) {
    if (menu->item_count <= 0) {
        menu->highlight = 0;
        menu->beginning = 0;
        return;
    }

    if (menu->highlight < 0) {
        menu->highlight = 0;
    }
    if (menu->highlight >= menu->item_count) {
        menu->highlight = menu->item_count - 1;
    }
    if (menu->beginning < 0) {
        menu->beginning = 0;
    }
    if (menu->beginning >= menu->item_count) {
        menu->beginning = menu->item_count - 1;
    }
    return;
}

static void
menu_set_flags_for_item(NcMenu *menu, void *item, uint32 flags) {
    int32 pos = menu_item_index(menu, NC_MENU_ITEMS_ALL, item);

    ASSERT_NON_NEGATIVE(pos);
    menu->all_item_flags[pos] = flags;

    if ((pos = menu_item_index(menu, NC_MENU_ITEMS_FILTERED, item)) >= 0) {
        menu->filtered_item_flags[pos] = flags;
    }
    return;
}

static bool
menu_is_position_highlightable(int32 pos, void *user) {
    NcMenu *menu = user;
    void *item = nc_menu_active_item_at(menu, pos);

    return !menu_is_separator(menu, item) && !menu_is_inactive(menu, item);
}

static void
menu_print_buffer(NcWindow *window, NcBuffer *buffer) {
    NcBufferProperty *properties = nc_buffer_properties(buffer);
    char *data = nc_buffer_data(buffer);
    int32 len = nc_buffer_len(buffer);
    int32 property_count = ARRAY_LEN(buffer->properties);
    int32 property_index = 0;

    for (int32 i = 0; i <= len; i += 1) {
        while ((property_index < property_count)
               && (properties[property_index].position == i)) {
            nc_buffer_apply_property(window, &properties[property_index]);
            property_index += 1;
        }
        if (i >= len) {
            break;
        }
        nc_window_print_char(window, data[i]);
    }
    return;
}

static void
menu_copy_buffer(NcBuffer *dest, NcBuffer *source) {
    nc_buffer_destroy(dest);
    if (source) {
        nc_buffer_copy(dest, source);
    } else {
        *dest = (NcBuffer){0};
    }
    return;
}

#endif /* NC_MENU_C */
