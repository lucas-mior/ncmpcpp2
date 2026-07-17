#include "c/ncm_song_list.h"

#include "c/ncm_base.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"

void
ncm_song_list_item_init(NcmSongListItem *item) {
    if (item == NULL) {
        return;
    }

    ncm_song_init(&item->song);
    item->flags = NCM_SONG_LIST_ITEM_SELECTABLE;
    return;
}

void
ncm_song_list_item_destroy(NcmSongListItem *item) {
    if (item == NULL) {
        return;
    }

    ncm_song_destroy(&item->song);
    item->flags = NCM_SONG_LIST_ITEM_SELECTABLE;
    return;
}

void
ncm_song_list_init(NcmSongList *list) {
    if (list == NULL) {
        return;
    }

    list->items = NULL;
    list->len = 0;
    list->cap = 0;
    list->current = 0;
    return;
}

void
ncm_song_list_clear(NcmSongList *list) {
    if (list == NULL) {
        return;
    }

    for (int32 i = 0; i < list->len; i += 1) {
        ncm_song_list_item_destroy(&list->items[i]);
    }
    list->len = 0;
    list->current = 0;
    return;
}

void
ncm_song_list_destroy(NcmSongList *list) {
    if (list == NULL) {
        return;
    }

    ncm_song_list_clear(list);
    if (list->items != NULL) {
        free2(list->items, list->cap*SIZEOF(*list->items));
    }
    ncm_song_list_init(list);
    return;
}

bool
ncm_song_list_reserve(NcmSongList *list, int32 extra) {
    int32 needed;
    int32 old_cap;
    int32 new_cap;

    if (list == NULL) {
        return false;
    }
    if (extra <= 0) {
        return true;
    }

    needed = list->len + extra;
    if (needed <= list->cap) {
        return true;
    }

    old_cap = list->cap;
    new_cap = old_cap;
    if (new_cap <= 0) {
        new_cap = 8;
    }
    while (new_cap < needed) {
        new_cap *= 2;
    }

    list->items = realloc2(list->items, old_cap, new_cap,
                         SIZEOF(*list->items));
    for (int32 i = old_cap; i < new_cap; i += 1) {
        ncm_song_list_item_init(&list->items[i]);
    }
    list->cap = new_cap;
    return true;
}

NcmSongListItem *
ncm_song_list_append(NcmSongList *list) {
    NcmSongListItem *item;

    if (!ncm_song_list_reserve(list, 1)) {
        return NULL;
    }

    item = &list->items[list->len];
    list->len += 1;
    ncm_song_list_item_destroy(item);
    ncm_song_list_item_init(item);
    return item;
}

bool
ncm_song_list_append_copy(NcmSongList *list, NcmSong *song, uint32 flags) {
    NcmSongListItem *item;

    item = ncm_song_list_append(list);
    if (item == NULL) {
        return false;
    }
    if (!ncm_song_copy(&item->song, song)) {
        list->len -= 1;
        ncm_song_list_item_destroy(item);
        return false;
    }
    item->flags = flags;
    return true;
}

void
ncm_song_list_append_move(NcmSongList *list, NcmSong *song, uint32 flags) {
    NcmSongListItem *item;

    item = ncm_song_list_append(list);
    if (item == NULL) {
        return;
    }

    ncm_song_move(&item->song, song);
    item->flags = flags;
    return;
}

NcmSongListItem *
ncm_song_list_at(NcmSongList *list, int32 idx) {
    if (list == NULL) {
        return NULL;
    }
    if ((idx < 0) || (idx >= list->len)) {
        return NULL;
    }

    return &list->items[idx];
}

NcmSongListItem *
ncm_song_list_current(NcmSongList *list) {
    if (list == NULL) {
        return NULL;
    }

    return ncm_song_list_at(list, list->current);
}

bool
ncm_song_list_set_current(NcmSongList *list, int32 idx) {
    if (list == NULL) {
        return false;
    }
    if ((idx < 0) || (idx >= list->len)) {
        return false;
    }

    list->current = idx;
    return true;
}

bool
ncm_song_list_item_is_selected(NcmSongListItem *item) {
    if (item == NULL) {
        return false;
    }

    return (item->flags & NCM_SONG_LIST_ITEM_SELECTED) != 0;
}

bool
ncm_song_list_item_is_selectable(NcmSongListItem *item) {
    if (item == NULL) {
        return false;
    }

    return (item->flags & NCM_SONG_LIST_ITEM_SELECTABLE) != 0;
}

void
ncm_song_list_item_set_selected(NcmSongListItem *item, bool selected) {
    if (item == NULL) {
        return;
    }
    if (!ncm_song_list_item_is_selectable(item)) {
        item->flags &= ~((uint32)NCM_SONG_LIST_ITEM_SELECTED);
        return;
    }

    if (selected) {
        item->flags |= NCM_SONG_LIST_ITEM_SELECTED;
    } else {
        item->flags &= ~((uint32)NCM_SONG_LIST_ITEM_SELECTED);
    }
    return;
}

int32
ncm_song_list_selected_count(NcmSongList *list) {
    int32 count;

    if (list == NULL) {
        return 0;
    }

    count = 0;
    for (int32 i = 0; i < list->len; i += 1) {
        if (ncm_song_list_item_is_selected(&list->items[i])) {
            count += 1;
        }
    }
    return count;
}

bool
ncm_song_list_has_selected(NcmSongList *list) {
    return ncm_song_list_selected_count(list) > 0;
}

bool
ncm_song_list_each_selected_song(NcmSongList *list,
                                NcmSongListEachFunc each, void *user) {
    if (list == NULL) {
        return false;
    }
    if (each == NULL) {
        return false;
    }

    for (int32 i = 0; i < list->len; i += 1) {
        if (ncm_song_list_item_is_selected(&list->items[i])) {
            if (!each(&list->items[i].song, i, user)) {
                return false;
            }
        }
    }
    return true;
}

bool
ncm_song_list_select_current_song_if_none_selected(NcmSongList *list) {
    NcmSongListItem *item;

    if (list == NULL) {
        return false;
    }
    if (ncm_song_list_has_selected(list)) {
        return true;
    }

    item = ncm_song_list_current(list);
    if (item == NULL) {
        return false;
    }
    if (!ncm_song_list_item_is_selectable(item)) {
        return false;
    }
    ncm_song_list_item_set_selected(item, true);
    return true;
}

void
ncm_song_list_clear_selection(NcmSongList *list) {
    if (list == NULL) {
        return;
    }

    for (int32 i = 0; i < list->len; i += 1) {
        ncm_song_list_item_set_selected(&list->items[i], false);
    }
    return;
}

void
ncm_song_list_reverse_selectable_selection(NcmSongList *list) {
    NcmSongListItem *item;

    if (list == NULL) {
        return;
    }

    for (int32 i = 0; i < list->len; i += 1) {
        item = &list->items[i];
        if (ncm_song_list_item_is_selectable(item)) {
            ncm_song_list_item_set_selected(
                item, !ncm_song_list_item_is_selected(item));
        }
    }
    return;
}

bool
ncm_song_list_wrapped_song_search(NcmSongList *list,
                             NcmSongListPredicate predicate, void *user,
                             enum NcmSongListSearchDirection direction,
                             bool wrap, bool skip_current) {
    int32 start;
    int32 step;
    int32 limit;
    int32 idx;

    if (list == NULL) {
        return false;
    }
    if (predicate == NULL) {
        return false;
    }
    if (list->len <= 0) {
        return false;
    }

    start = list->current;
    if ((start < 0) || (start >= list->len)) {
        start = 0;
    }

    step = 1;
    if (direction == NCM_SONG_LIST_SEARCH_BACKWARD) {
        step = -1;
    }
    if (skip_current) {
        start += step;
    }

    limit = list->len;
    idx = start;
    for (int32 i = 0; i < limit; i += 1) {
        if ((idx < 0) || (idx >= list->len)) {
            if (!wrap) {
                return false;
            }
            if (idx < 0) {
                idx = list->len - 1;
            } else {
                idx = 0;
            }
        }

        if (predicate(&list->items[idx].song, user)) {
            list->current = idx;
            return true;
        }
        idx += step;
    }
    return false;
}

bool
ncm_song_list_selected_songs(NcmSongList *list, NcmSongArray *songs) {
    bool found;
    NcmSongListItem *item;

    if (songs == NULL) {
        return false;
    }
    ncm_song_array_clear(songs);
    if (list == NULL) {
        return false;
    }

    found = false;
    for (int32 i = 0; i < list->len; i += 1) {
        item = &list->items[i];
        if (ncm_song_list_item_is_selected(item)) {
            if (!ncm_song_array_append_copy(songs, &item->song)) {
                ncm_song_array_clear(songs);
                return false;
            }
            found = true;
        }
    }

    if (!found && (list->len > 0)) {
        item = ncm_song_list_current(list);
        if (item == NULL) {
            return true;
        }
        if (!ncm_song_array_append_copy(songs, &item->song)) {
            ncm_song_array_clear(songs);
            return false;
        }
    }

    return true;
}
