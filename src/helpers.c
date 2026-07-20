#if !defined(NCMPCPP_HELPERS_C)
#define NCMPCPP_HELPERS_C

#include "helpers.h"

#include <assert.h>
#include <stddef.h>
#include <time.h>

#include "cbase/base_macros.h"

char *
ncm_helpers_with_errors(bool success) {
    if (success) {
        return "";
    }
    return " (with errors)";
}

int32
ncm_helpers_show_song_time(int32 length, char *buffer, int32 buffer_cap) {
    return ncm_song_show_time(length, buffer, buffer_cap);
}

static int64
menu_item_count(NcMenu *menu, enum NcMenuItemSource source) {
    if (menu == NULL) {
        return 0;
    }
    switch (source) {
    case NC_MENU_ITEMS_FILTERED:
        return nc_menu_filtered_item_count(menu);
    case NC_MENU_ITEMS_ALL:
        return nc_menu_all_item_count(menu);
    case NC_MENU_ITEMS_LAST:
    default:
        break;
    }
    return 0;
}

static bool
menu_position_is_selected(NcMenu *menu, enum NcMenuItemSource source,
                          int64 pos) {
    uint32 flags;
    void *item;

    if (menu == NULL) {
        return false;
    }
    flags = nc_menu_item_flags_at(menu, source, pos);
    if (flags & NC_MENU_ITEM_SELECTED) {
        return true;
    }
    item = nc_menu_item_at(menu, source, pos);
    if (item == NULL) {
        return false;
    }
    if (menu->display_callbacks.is_selected == NULL) {
        return false;
    }
    return menu->display_callbacks.is_selected(item,
                                               menu->display_callbacks.user);
}

static bool
menu_set_position_selected(NcMenu *menu, enum NcMenuItemSource source,
                           int64 pos, bool selected) {
    uint32 flags;
    void *item;

    if (menu == NULL) {
        return false;
    }
    item = nc_menu_item_at(menu, source, pos);
    if (item == NULL) {
        return false;
    }
    if (menu->action_callbacks.set_selected) {
        menu->action_callbacks.set_selected(item, selected,
                                            menu->action_callbacks.user);
    }

    flags = nc_menu_item_flags_at(menu, source, pos);
    if (selected) {
        flags |= NC_MENU_ITEM_SELECTED;
    } else {
        flags &= ~NC_MENU_ITEM_SELECTED;
    }
    return nc_menu_set_item_flags_at(menu, source, pos, flags);
}

void
ncm_menu_reverse_selection(NcMenu *menu, enum NcMenuItemSource source) {
    int64 count;
    bool selected;

    count = menu_item_count(menu, source);
    for (int64 i = 0; i < count; i += 1) {
        selected = menu_position_is_selected(menu, source, i);
        menu_set_position_selected(menu, source, i, !selected);
    }
    return;
}

bool
ncm_menu_find_selected_range(NcMenu *menu, enum NcMenuItemSource source,
                             int64 *first, int64 *last) {
    int64 count;
    int64 range_first;

    count = menu_item_count(menu, source);
    if (first) {
        *first = 0;
    }
    if (last) {
        *last = count;
    }

    range_first = count;
    for (int64 i = 0; i < count; i += 1) {
        if (menu_position_is_selected(menu, source, i)) {
            range_first = i;
            break;
        }
    }
    if (range_first >= count) {
        return false;
    }

    if (first) {
        *first = range_first;
    }
    for (int64 i = count - 1; i >= range_first; i -= 1) {
        if (menu_position_is_selected(menu, source, i)) {
            if (last) {
                *last = i + 1;
            }
            return true;
        }
    }
    return false;
}

bool
ncm_menu_find_full_selected_range(NcMenu *menu, enum NcMenuItemSource source,
                                  int64 *first, int64 *last) {
    int64 range_first;
    int64 range_last;

    if (!ncm_menu_find_selected_range(menu, source, &range_first,
                                      &range_last)) {
        if (first) {
            *first = 0;
        }
        if (last) {
            *last = menu_item_count(menu, source);
        }
        return true;
    }
    for (int64 i = range_first; i < range_last; i += 1) {
        if (!menu_position_is_selected(menu, source, i)) {
            return false;
        }
    }
    if (first) {
        *first = range_first;
    }
    if (last) {
        *last = range_last;
    }
    return true;
}

#endif /* NCMPCPP_HELPERS_C */
