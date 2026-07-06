#include "c/ncm_functional.h"


#include <stddef.h>
static void *ncm_functional_item(void *items, int32 index, int32 item_size);

static void *
ncm_functional_item(void *items, int32 index, int32 item_size) {
    char *bytes;

    bytes = items;
    return bytes + index*item_size;
}

int32
ncm_find_map_first(void *items, int32 items_len, int32 item_size,
                   NcmFindPredicate predicate, void *predicate_user,
                   NcmMapFunction map, void *map_user) {
    void *item;

    if ((items == NULL) || (items_len <= 0) || (item_size <= 0)) {
        return -1;
    }
    if (predicate == NULL) {
        return -1;
    }

    for (int32 i = 0; i < items_len; i += 1) {
        item = ncm_functional_item(items, i, item_size);
        if (predicate(item, predicate_user)) {
            if (map) {
                map(item, map_user);
            }
            return i;
        }
    }

    return -1;
}

int32
ncm_find_map_all(void *items, int32 items_len, int32 item_size,
                 NcmFindPredicate predicate, void *predicate_user,
                 NcmMapFunction map, void *map_user) {
    void *item;
    int32 result;

    if ((items == NULL) || (items_len <= 0) || (item_size <= 0)) {
        return 0;
    }
    if (predicate == NULL) {
        return 0;
    }

    result = 0;
    for (int32 i = 0; i < items_len; i += 1) {
        item = ncm_functional_item(items, i, item_size);
        if (predicate(item, predicate_user)) {
            if (map) {
                map(item, map_user);
            }
            result += 1;
        }
    }

    return result;
}
