#include "helpers.h"

#include <assert.h>
#include <stddef.h>
#include <time.h>

#include "cbase/base_macros.h"

static int32 song_list_item_count(NcmSongList *list);
static int64 menu_item_count(NcMenu *menu, enum NcMenuItemSource source);
static bool menu_position_is_selected(NcMenu *menu,
                                      enum NcMenuItemSource source,
                                      int64 pos);
static bool menu_set_position_selected(NcMenu *menu,
                                       enum NcMenuItemSource source,
                                       int64 pos, bool selected);

char *
ncm_helpers_with_errors(bool success) {
    if (success) {
        return "";
    }
    return " (with errors)";
}

int32
ncm_helpers_show_song_time(uint32 length, char *buffer, int32 buffer_cap) {
    return ncm_song_show_time(length, buffer, buffer_cap);
}

bool
ncm_helpers_time_format(NcmBuffer *buffer, char *format, time_t value) {
    struct tm info;
    char result[64];
    int32 len;

    if (buffer == NULL) {
        return false;
    }
    if (format == NULL) {
        return false;
    }
    if (localtime_r(&value, &info) == NULL) {
        return false;
    }

    len = (int32)strftime(result, (size_t)SIZEOF(result), format, &info);
    if (len <= 0) {
        return false;
    }
    ncm_buffer_append(buffer, result, len);
    return true;
}

bool
ncm_helpers_timestamp(NcmBuffer *buffer, time_t value) {
    return ncm_helpers_time_format(buffer, "%x %X", value);
}

static int32
song_list_item_count(NcmSongList *list) {
    if (list == NULL) {
        return 0;
    }
    return list->len;
}

bool
ncm_song_list_each_item(NcmSongList *list, NcmSongListIterFunc func,
                        void *user) {
    if (func == NULL) {
        return false;
    }
    if (list == NULL) {
        return true;
    }

    for (int32 i = 0; i < list->len; i += 1) {
        if (!func(&list->items[i], i, user)) {
            return false;
        }
    }
    return true;
}

bool
ncm_song_list_each_item_reverse(NcmSongList *list,
                                NcmSongListIterFunc func, void *user) {
    if (func == NULL) {
        return false;
    }
    if (list == NULL) {
        return true;
    }

    for (int32 i = list->len - 1; i >= 0; i -= 1) {
        if (!func(&list->items[i], i, user)) {
            return false;
        }
    }
    return true;
}

bool
ncm_song_list_each_selected(NcmSongList *list, NcmSongListIterFunc func,
                            void *user) {
    NcmSongListItem *item;

    if (func == NULL) {
        return false;
    }
    if (list == NULL) {
        return true;
    }

    for (int32 i = 0; i < list->len; i += 1) {
        item = &list->items[i];
        if (ncm_song_list_item_is_selected(item)) {
            if (!func(item, i, user)) {
                return false;
            }
        }
    }
    return true;
}

bool
ncm_song_list_each_selected_or_current(NcmSongList *list,
                                       NcmSongListIterFunc func,
                                       void *user) {
    NcmSongListItem *item;
    bool found;

    if (func == NULL) {
        return false;
    }
    if (list == NULL) {
        return true;
    }

    found = false;
    for (int32 i = 0; i < list->len; i += 1) {
        item = &list->items[i];
        if (ncm_song_list_item_is_selected(item)) {
            found = true;
            if (!func(item, i, user)) {
                return false;
            }
        }
    }
    if (!found) {
        item = ncm_song_list_current(list);
        if (item) {
            return func(item, list->current, user);
        }
    }
    return true;
}

bool
ncm_song_list_has_selected_item(NcmSongList *list) {
    if (list == NULL) {
        return false;
    }

    for (int32 i = 0; i < list->len; i += 1) {
        if (ncm_song_list_item_is_selected(&list->items[i])) {
            return true;
        }
    }
    return false;
}

void
ncm_song_list_select_current_if_none_selected(NcmSongList *list) {
    NcmSongListItem *current;

    if (list == NULL) {
        return;
    }
    if (ncm_song_list_has_selected_item(list)) {
        return;
    }
    if ((current = ncm_song_list_current(list)) == NULL) {
        return;
    }

    ncm_song_list_item_set_selected(current, true);
    return;
}

void
ncm_song_list_reverse_selection(NcmSongList *list) {
    NcmSongListItem *item;
    bool selected;

    if (list == NULL) {
        return;
    }

    for (int32 i = 0; i < list->len; i += 1) {
        item = &list->items[i];
        selected = ncm_song_list_item_is_selected(item);
        ncm_song_list_item_set_selected(item, !selected);
    }
    return;
}

bool
ncm_song_list_find_selected_range(NcmSongList *list, int32 *first,
                                  int32 *last) {
    int32 count;
    int32 range_first;

    count = song_list_item_count(list);
    if (first) {
        *first = 0;
    }
    if (last) {
        *last = count;
    }
    if (list == NULL) {
        return false;
    }

    range_first = count;
    for (int32 i = 0; i < list->len; i += 1) {
        if (ncm_song_list_item_is_selected(&list->items[i])) {
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
    for (int32 i = count - 1; i >= range_first; i -= 1) {
        if (ncm_song_list_item_is_selected(&list->items[i])) {
            if (last) {
                *last = i + 1;
            }
            return true;
        }
    }
    return false;
}

bool
ncm_song_list_find_full_selected_range(NcmSongList *list, int32 *first,
                                       int32 *last) {
    int32 range_first;
    int32 range_last;

    if (!ncm_song_list_find_selected_range(list, &range_first, &range_last)) {
        if (first) {
            *first = 0;
        }
        if (last) {
            *last = song_list_item_count(list);
        }
        return true;
    }

    for (int32 i = range_first; i < range_last; i += 1) {
        if (!ncm_song_list_item_is_selected(&list->items[i])) {
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

int32
ncm_song_list_wrapped_search(NcmSongList *list, int32 current,
                             enum SearchDirection direction, bool wrap,
                             bool skip_current,
                             NcmSongListIterFunc predicate, void *user) {
    int32 count;
    int32 start;

    if ((list == NULL) || (predicate == NULL)) {
        return -1;
    }
    count = list->len;
    if (count <= 0) {
        return -1;
    }
    if ((current < 0) || (current >= count)) {
        current = list->current;
    }
    if ((current < 0) || (current >= count)) {
        return -1;
    }

    switch (direction) {
    case NCM_SEARCH_DIRECTION_FORWARD:
        start = current;
        if (skip_current) {
            start += 1;
        }
        for (int32 i = start; i < count; i += 1) {
            if (predicate(&list->items[i], i, user)) {
                return i;
            }
        }
        if (wrap) {
            for (int32 i = 0; i < start && i < count; i += 1) {
                if (predicate(&list->items[i], i, user)) {
                    return i;
                }
            }
        }
        break;
    case NCM_SEARCH_DIRECTION_BACKWARD:
        start = current;
        if (skip_current) {
            start -= 1;
        }
        for (int32 i = start; i >= 0; i -= 1) {
            if (predicate(&list->items[i], i, user)) {
                return i;
            }
        }
        if (wrap) {
            for (int32 i = count - 1; i > start; i -= 1) {
                if (predicate(&list->items[i], i, user)) {
                    return i;
                }
            }
        }
        break;
    case NCM_SEARCH_DIRECTION_LAST:
        break;
    }
    return -1;
}

bool
ncm_mpd_song_list_each_song(NcmMpdSongList *list,
                            NcmMpdSongListIterFunc func, void *user) {
    if (func == NULL) {
        return false;
    }
    if (list == NULL) {
        return true;
    }

    for (int32 i = 0; i < list->count; i += 1) {
        if (!func(&list->items[i], i, user)) {
            return false;
        }
    }
    return true;
}

bool
ncm_mpd_song_list_each_song_reverse(NcmMpdSongList *list,
                                    NcmMpdSongListIterFunc func,
                                    void *user) {
    if (func == NULL) {
        return false;
    }
    if (list == NULL) {
        return true;
    }

    for (int32 i = list->count - 1; i >= 0; i -= 1) {
        if (!func(&list->items[i], i, user)) {
            return false;
        }
    }
    return true;
}

int32
ncm_mpd_song_list_wrapped_search(NcmMpdSongList *list, int32 current,
                                 enum SearchDirection direction,
                                 bool wrap, bool skip_current,
                                 NcmMpdSongListIterFunc predicate,
                                 void *user) {
    int32 count;
    int32 start;

    if ((list == NULL) || (predicate == NULL)) {
        return -1;
    }
    count = list->count;
    if (count <= 0) {
        return -1;
    }
    if ((current < 0) || (current >= count)) {
        return -1;
    }

    switch (direction) {
    case NCM_SEARCH_DIRECTION_FORWARD:
        start = current;
        if (skip_current) {
            start += 1;
        }
        for (int32 i = start; i < count; i += 1) {
            if (predicate(&list->items[i], i, user)) {
                return i;
            }
        }
        if (wrap) {
            for (int32 i = 0; i < start && i < count; i += 1) {
                if (predicate(&list->items[i], i, user)) {
                    return i;
                }
            }
        }
        break;
    case NCM_SEARCH_DIRECTION_BACKWARD:
        start = current;
        if (skip_current) {
            start -= 1;
        }
        for (int32 i = start; i >= 0; i -= 1) {
            if (predicate(&list->items[i], i, user)) {
                return i;
            }
        }
        if (wrap) {
            for (int32 i = count - 1; i > start; i -= 1) {
                if (predicate(&list->items[i], i, user)) {
                    return i;
                }
            }
        }
        break;
    case NCM_SEARCH_DIRECTION_LAST:
        break;
    }
    return -1;
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
    return menu->display_callbacks.is_selected(
        item,
        menu->display_callbacks.user
    );
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
    if (menu->action_callbacks.set_selected != NULL) {
        menu->action_callbacks.set_selected(
            item, selected, menu->action_callbacks.user);
    }

    flags = nc_menu_item_flags_at(menu, source, pos);
    if (selected) {
        flags |= NC_MENU_ITEM_SELECTED;
    } else {
        flags &= ~NC_MENU_ITEM_SELECTED;
    }
    return nc_menu_set_item_flags_at(menu, source, pos, flags);
}

bool
ncm_menu_each_item(NcMenu *menu, enum NcMenuItemSource source,
                   NcmMenuIterFunc func, void *user) {
    int64 count;
    void *item;

    if (func == NULL) {
        return false;
    }
    count = menu_item_count(menu, source);
    for (int64 i = 0; i < count; i += 1) {
        item = nc_menu_item_at(menu, source, i);
        if (!func(item, i, user)) {
            return false;
        }
    }
    return true;
}

bool
ncm_menu_each_item_reverse(NcMenu *menu, enum NcMenuItemSource source,
                           NcmMenuIterFunc func, void *user) {
    int64 count;
    void *item;

    if (func == NULL) {
        return false;
    }
    count = menu_item_count(menu, source);
    for (int64 i = count - 1; i >= 0; i -= 1) {
        item = nc_menu_item_at(menu, source, i);
        if (!func(item, i, user)) {
            return false;
        }
    }
    return true;
}

bool
ncm_menu_each_selected(NcMenu *menu, enum NcMenuItemSource source,
                       NcmMenuIterFunc func, void *user) {
    int64 count;
    void *item;

    if (func == NULL) {
        return false;
    }
    count = menu_item_count(menu, source);
    for (int64 i = 0; i < count; i += 1) {
        if (menu_position_is_selected(menu, source, i)) {
            item = nc_menu_item_at(menu, source, i);
            if (!func(item, i, user)) {
                return false;
            }
        }
    }
    return true;
}

bool
ncm_menu_each_selected_or_current(NcMenu *menu, enum NcMenuItemSource source,
                                  NcmMenuIterFunc func, void *user) {
    void *item;

    if (func == NULL) {
        return false;
    }
    if (ncm_menu_has_selected_item(menu, source)) {
        return ncm_menu_each_selected(menu, source, func, user);
    }
    item = nc_menu_current_item(menu);
    if (item == NULL) {
        return true;
    }
    return func(item, nc_menu_highlight(menu), user);
}

bool
ncm_menu_has_selected_item(NcMenu *menu, enum NcMenuItemSource source) {
    int64 count;

    count = menu_item_count(menu, source);
    for (int64 i = 0; i < count; i += 1) {
        if (menu_position_is_selected(menu, source, i)) {
            return true;
        }
    }
    return false;
}

void
ncm_menu_select_current_if_none_selected(NcMenu *menu) {
    if (menu == NULL) {
        return;
    }
    if (ncm_menu_has_selected_item(menu, NC_MENU_ITEMS_ALL)) {
        return;
    }
    nc_menu_set_current_selected(menu, true);
    return;
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
ncm_menu_find_full_selected_range(NcMenu *menu,
                                  enum NcMenuItemSource source,
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

int64
ncm_menu_wrapped_search(NcMenu *menu, enum NcMenuItemSource source,
                        int64 current, enum SearchDirection direction,
                        bool wrap, bool skip_current,
                        NcmMenuIterFunc predicate, void *user) {
    int64 count;
    int64 start;
    void *item;

    if ((menu == NULL) || (predicate == NULL)) {
        return -1;
    }
    count = menu_item_count(menu, source);
    if (count <= 0) {
        return -1;
    }
    if ((current < 0) || (current >= count)) {
        current = nc_menu_highlight(menu);
    }
    if ((current < 0) || (current >= count)) {
        return -1;
    }

    switch (direction) {
    case NCM_SEARCH_DIRECTION_FORWARD:
        start = current;
        if (skip_current) {
            start += 1;
        }
        for (int64 i = start; i < count; i += 1) {
            item = nc_menu_item_at(menu, source, i);
            if (predicate(item, i, user)) {
                return i;
            }
        }
        if (wrap) {
            for (int64 i = 0; i < start && i < count; i += 1) {
                item = nc_menu_item_at(menu, source, i);
                if (predicate(item, i, user)) {
                    return i;
                }
            }
        }
        break;
    case NCM_SEARCH_DIRECTION_BACKWARD:
        start = current;
        if (skip_current) {
            start -= 1;
        }
        for (int64 i = start; i >= 0; i -= 1) {
            item = nc_menu_item_at(menu, source, i);
            if (predicate(item, i, user)) {
                return i;
            }
        }
        if (wrap) {
            for (int64 i = count - 1; i > start; i -= 1) {
                item = nc_menu_item_at(menu, source, i);
                if (predicate(item, i, user)) {
                    return i;
                }
            }
        }
        break;
    case NCM_SEARCH_DIRECTION_LAST:
        break;
    }
    return -1;
}
