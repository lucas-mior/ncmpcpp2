#if !defined(NCMPCPP_HELPERS_C)
#define NCMPCPP_HELPERS_C

#include "cbase.h"

#include "helpers.h"

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

static int32
menu_item_count(NcMenu *menu, enum NcMenuItemSource source) {
    ASSERT(menu != NULL);

    switch (source) {
    case NC_MENU_ITEMS_FILTERED:
        return nc_menu_filtered_item_count(menu);
    case NC_MENU_ITEMS_ALL:
        return nc_menu_all_item_count(menu);
    case NC_MENU_ITEMS_COUNT:
    default:
        return 0;
    }
}

static bool
menu_position_is_selected(NcMenu *menu, enum NcMenuItemSource source,
                          int32 pos) {
    uint32 flags;
    void *item;

    ASSERT(menu != NULL);

    flags = nc_menu_item_flags_at(menu, source, pos);
    if (flags & NC_MENU_ITEM_SELECTED) {
        return true;
    }

    if ((item = nc_menu_item_at(menu, source, pos)) == NULL) {
        return false;
    }

    if (menu->display_callbacks.is_selected == NULL) {
        return false;
    }

    return menu->display_callbacks.is_selected(item,
                                               menu->display_callbacks.user);
}

void
ncm_menu_reverse_selection(NcMenu *menu, enum NcMenuItemSource source) {
    bool selected;
    int32 count;

    if (menu == NULL) {
        return;
    }

    count = menu_item_count(menu, source);
    for (int32 i = 0; i < count; i += 1) {
        uint32 flags;
        void *item;

        selected = menu_position_is_selected(menu, source, i);
        if ((item = nc_menu_item_at(menu, source, i)) == NULL) {
            continue;
        }

        if (menu->action_callbacks.set_selected) {
            menu->action_callbacks.set_selected(item, !selected,
                                                menu->action_callbacks.user);
        }

        flags = nc_menu_item_flags_at(menu, source, i);
        if (!selected) {
            flags |= NC_MENU_ITEM_SELECTED;
        } else {
            flags &= ~NC_MENU_ITEM_SELECTED;
        }
        nc_menu_set_item_flags_at(menu, source, i, flags);
    }
    return;
}

int32
ncm_menu_find_selected_range(NcMenu *menu, enum NcMenuItemSource source,
                             int32 *first, int32 *last) {
    int32 range_first;
    int32 count;

    if (first) {
        *first = 0;
    }
    if (menu == NULL) {
        if (last) {
            *last = 0;
        }
        return -EINVAL;
    }

    count = menu_item_count(menu, source);
    if (last) {
        *last = count;
    }

    range_first = count;
    for (int32 i = 0; i < count; i += 1) {
        if (menu_position_is_selected(menu, source, i)) {
            range_first = i;
            break;
        }
    }
    if (range_first >= count) {
        return 0;
    }

    if (first) {
        *first = range_first;
    }
    for (int32 i = count - 1; i >= range_first; i -= 1) {
        if (menu_position_is_selected(menu, source, i)) {
            if (last) {
                *last = i + 1;
            }
            return 1;
        }
    }
    return 0;
}

int32
ncm_menu_find_full_selected_range(NcMenu *menu, enum NcMenuItemSource source,
                                  int32 *first, int32 *last) {
    int32 range_first;
    int32 range_last;
    int32 status;

    status = ncm_menu_find_selected_range(menu, source,
                                          &range_first, &range_last);
    if (status < 0) {
        return status;
    }
    if (status == 0) {
        if (first) {
            *first = 0;
        }
        if (last) {
            *last = menu_item_count(menu, source);
        }
        return 0;
    }
    for (int32 i = range_first; i < range_last; i += 1) {
        if (!menu_position_is_selected(menu, source, i)) {
            return -NCM_ERROR_UNAVAILABLE;
        }
    }
    if (first) {
        *first = range_first;
    }
    if (last) {
        *last = range_last;
    }
    return 0;
}

#endif /* NCMPCPP_HELPERS_C */
