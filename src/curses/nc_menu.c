#include "curses/nc_menu.h"

#include <assert.h>

static int64
min_int64(int64 left, int64 right) {
    if (left < right) {
        return left;
    }
    return right;
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
nc_menu_copy(NcMenu *dest, NcMenu *source) {
    *dest = *source;
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
nc_menu_set_item_count(NcMenu *menu, int64 item_count) {
    if (item_count < 0) {
        item_count = 0;
    }
    menu->item_count = item_count;
    return;
}

int64
nc_menu_item_count(NcMenu *menu) {
    return menu->item_count;
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

void
nc_menu_prepare_refresh(NcMenu *menu, int64 height,
                        NcMenuHighlightableFunc is_highlightable,
                        void *user) {
    int64 max_beginning;
    int64 max_visible_highlight;

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
nc_menu_scroll(NcMenu *menu, int64 height, enum NcScroll where,
               NcMenuHighlightableFunc is_highlightable, void *user) {
    scroll_internal(menu, height, where, is_highlightable, user, 0);
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
