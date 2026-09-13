#include "cbase.h"
#include "ncmpcpp2.h"

void
string_list_push(StrViewList *list, char *value, int32 value_len) {
    StrView string;

    if (list->arena == NULL) {
        list->arena = arena_create(SIZEMB(2), "mpd_string_list");
    }

    string.data = xarena_push(list->arena, value_len + 1);
    string.len = value_len;
    memcpy64(string.data, value, value_len + 1);
    ARRAY_PUSH(list->items, string);
    return;
}

void
string_list_destroy(StrViewList *list) {
    if (list == NULL) {
        return;
    }

    ARRAY_FREE(list->items);
    if (list->arena) {
        arena_destroy(list->arena);
    }
    *list = (StrViewList){0};

    return;
}

void
string_list_clear(StrViewList *list) {
    if (list == NULL) {
        return;
    }

    ARRAY_CLEAR(list->items);
    arena_reset(list->arena);
    return;
}

int32
string_list_len(StrViewList *list) {
    if (list == NULL) {
        return 0;
    }

    return ARRAY_LEN(list->items);
}

StrView *
string_list_at(StrViewList *list, int32 idx) {
    if (list == NULL) {
        return NULL;
    }
    if ((idx < 0) || (idx >= ARRAY_LEN(list->items))) {
        return NULL;
    }

    return &list->items[idx];
}
