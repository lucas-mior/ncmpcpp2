#include "cbase.h"
#include "ncmpcpp2.h"

void
strview_list_push(StrViewList *list, char *value, int32 value_len) {
    StrView string;

    if (list->arena == NULL) {
        list->arena = arena_create(SIZEMB(2), "mpd_string_list");
    }

    string.data = xarena_push(list->arena, value_len + 1);
    string.len = value_len;
    memcpy64(string.data, value, value_len + 1);
    string.data[value_len] = '\0';
    ARRAY_PUSH(list->items, string);
    return;
}

void
strview_list_destroy(StrViewList *list) {
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
strview_list_clear(StrViewList *list) {
    if (list == NULL) {
        return;
    }

    ARRAY_CLEAR(list->items);
    arena_reset(list->arena);
    return;
}

int32
strview_list_len(StrViewList *list) {
    if (list == NULL) {
        return 0;
    }

    return ARRAY_LEN(list->items);
}

StrView *
strview_list_at(StrViewList *list, int32 idx) {
    if (list == NULL) {
        return NULL;
    }
    if ((idx < 0) || (idx >= ARRAY_LEN(list->items))) {
        return NULL;
    }

    return &list->items[idx];
}

void
strflex_list_push(StrFlexList *list, char *value, int32 value_len) {
    StrFlex *string;

    if (list->arena == NULL) {
        list->arena = arena_create(SIZEMB(2), "mpd_string_list");
    }

    string = xarena_push(list->arena, SIZEOF(*string) + value_len + 1);
    string->len = value_len;
    memcpy64(string->data, value, value_len);
    string->data[value_len] = '\0';
    ARRAY_PUSH(list->items, string);
    return;
}

void
strflex_list_destroy(StrFlexList *list) {
    if (list == NULL) {
        return;
    }

    ARRAY_FREE(list->items);
    if (list->arena) {
        arena_destroy(list->arena);
    }
    *list = (StrFlexList){0};

    return;
}

void
strflex_list_clear(StrFlexList *list) {
    if (list == NULL) {
        return;
    }

    ARRAY_CLEAR(list->items);
    arena_reset(list->arena);
    return;
}

int32
strflex_list_len(StrFlexList *list) {
    if (list == NULL) {
        return 0;
    }

    return ARRAY_LEN(list->items);
}

StrFlex *
strflex_list_at(StrFlexList *list, int32 idx) {
    if (list == NULL) {
        return NULL;
    }
    if ((idx < 0) || (idx >= ARRAY_LEN(list->items))) {
        return NULL;
    }

    return list->items[idx];
}
