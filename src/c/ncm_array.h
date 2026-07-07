#if !defined(NCM_ARRAY_H)
#define NCM_ARRAY_H

#include <stddef.h>

#include "c/ncm_base.h"
#include "cbase/base_macros.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef void (*NcmArrayItemInitCallback)(void *item);
typedef void (*NcmArrayItemDestroyCallback)(void *item);
typedef bool (*NcmArrayItemCopyCallback)(void *dest, void *source);
typedef void (*NcmArrayItemMoveCallback)(void *dest, void *source);

typedef struct NcmArrayItemCallbacks {
    NcmArrayItemInitCallback init;
    NcmArrayItemDestroyCallback destroy;
    NcmArrayItemCopyCallback copy;
    NcmArrayItemMoveCallback move;
} NcmArrayItemCallbacks;

#define NCM_ARRAY_DECLARE(PREFIX, ARRAY_TYPE, ITEM_TYPE)                  \
    typedef struct ARRAY_TYPE {                                           \
        ITEM_TYPE *items;                                                 \
        int32 len;                                                        \
        int32 cap;                                                        \
    } ARRAY_TYPE;                                                         \
    void PREFIX##_init(ARRAY_TYPE *array);                                \
    void PREFIX##_destroy(ARRAY_TYPE *array);                             \
    void PREFIX##_clear(ARRAY_TYPE *array);                               \
    bool PREFIX##_copy(ARRAY_TYPE *dest, ARRAY_TYPE *source);             \
    void PREFIX##_move(ARRAY_TYPE *dest, ARRAY_TYPE *source);             \
    void PREFIX##_swap(ARRAY_TYPE *left, ARRAY_TYPE *right);              \
    bool PREFIX##_reserve(ARRAY_TYPE *array, int32 extra);                \
    ITEM_TYPE *PREFIX##_append(ARRAY_TYPE *array);                        \
    bool PREFIX##_append_copy(ARRAY_TYPE *array, ITEM_TYPE *item);        \
    void PREFIX##_append_move(ARRAY_TYPE *array, ITEM_TYPE *item);        \
    void PREFIX##_remove_ordered(ARRAY_TYPE *array, int32 idx);

#define NCM_ARRAY_DEFINE(PREFIX, ARRAY_TYPE, ITEM_TYPE, CALLBACKS)        \
    void                                                                 \
    PREFIX##_init(ARRAY_TYPE *array) {                                    \
        array->items = NULL;                                              \
        array->len = 0;                                                   \
        array->cap = 0;                                                   \
        return;                                                           \
    }                                                                    \
                                                                         \
    void                                                                 \
    PREFIX##_clear(ARRAY_TYPE *array) {                                   \
        NcmArrayItemCallbacks *callbacks;                                 \
                                                                         \
        if (array == NULL) {                                              \
            return;                                                       \
        }                                                                \
        callbacks = CALLBACKS;                                            \
        if ((callbacks != NULL) && (callbacks->destroy != NULL)) {        \
            for (int32 i = 0; i < array->len; i += 1) {                  \
                callbacks->destroy(&array->items[i]);                    \
            }                                                            \
        }                                                                \
        array->len = 0;                                                   \
        return;                                                           \
    }                                                                    \
                                                                         \
    void                                                                 \
    PREFIX##_destroy(ARRAY_TYPE *array) {                                 \
        if (array == NULL) {                                              \
            return;                                                       \
        }                                                                \
        PREFIX##_clear(array);                                            \
        if (array->items != NULL) {                                       \
            ncm_free(array->items,                                        \
                     array->cap*SIZEOF(*array->items));                  \
        }                                                                \
        PREFIX##_init(array);                                             \
        return;                                                           \
    }                                                                    \
                                                                         \
    bool                                                                 \
    PREFIX##_copy(ARRAY_TYPE *dest, ARRAY_TYPE *source) {                 \
        ARRAY_TYPE replacement;                                           \
                                                                         \
        if (dest == NULL) {                                               \
            return false;                                                 \
        }                                                                \
        if (dest == source) {                                             \
            return true;                                                  \
        }                                                                \
                                                                         \
        PREFIX##_init(&replacement);                                      \
        if (source != NULL) {                                             \
            if (!PREFIX##_reserve(&replacement, source->len)) {           \
                PREFIX##_destroy(&replacement);                           \
                return false;                                             \
            }                                                            \
            for (int32 i = 0; i < source->len; i += 1) {                 \
                if (!PREFIX##_append_copy(&replacement,                   \
                                          &source->items[i])) {           \
                    PREFIX##_destroy(&replacement);                       \
                    return false;                                         \
                }                                                        \
            }                                                            \
        }                                                                \
                                                                         \
        PREFIX##_destroy(dest);                                           \
        *dest = replacement;                                              \
        return true;                                                      \
    }                                                                    \
                                                                         \
    void                                                                 \
    PREFIX##_move(ARRAY_TYPE *dest, ARRAY_TYPE *source) {                 \
        if (dest == NULL) {                                               \
            return;                                                       \
        }                                                                \
        if (dest == source) {                                             \
            return;                                                       \
        }                                                                \
                                                                         \
        PREFIX##_destroy(dest);                                           \
        if (source == NULL) {                                             \
            PREFIX##_init(dest);                                          \
            return;                                                       \
        }                                                                \
        *dest = *source;                                                  \
        PREFIX##_init(source);                                            \
        return;                                                           \
    }                                                                    \
                                                                         \
    void                                                                 \
    PREFIX##_swap(ARRAY_TYPE *left, ARRAY_TYPE *right) {                  \
        ARRAY_TYPE temp;                                                  \
                                                                         \
        if (left == NULL) {                                               \
            return;                                                       \
        }                                                                \
        if (right == NULL) {                                              \
            return;                                                       \
        }                                                                \
        temp = *left;                                                     \
        *left = *right;                                                   \
        *right = temp;                                                    \
        return;                                                           \
    }                                                                    \
                                                                         \
    bool                                                                 \
    PREFIX##_reserve(ARRAY_TYPE *array, int32 extra) {                    \
        int32 needed;                                                     \
        int32 old_cap;                                                    \
        int32 new_cap;                                                    \
                                                                         \
        if (array == NULL) {                                              \
            return false;                                                 \
        }                                                                \
        if (extra <= 0) {                                                 \
            return true;                                                  \
        }                                                                \
                                                                         \
        needed = array->len + extra;                                      \
        if (needed <= array->cap) {                                       \
            return true;                                                  \
        }                                                                \
                                                                         \
        old_cap = array->cap;                                             \
        new_cap = array->cap;                                             \
        if (new_cap <= 0) {                                               \
            new_cap = 8;                                                  \
        }                                                                \
        while (new_cap < needed) {                                        \
            new_cap *= 2;                                                 \
        }                                                                \
                                                                         \
        array->items = ncm_realloc_array(array->items, old_cap, new_cap,  \
                                         SIZEOF(*array->items));          \
        array->cap = new_cap;                                             \
        return true;                                                      \
    }                                                                    \
                                                                         \
    ITEM_TYPE *                                                          \
    PREFIX##_append(ARRAY_TYPE *array) {                                  \
        ITEM_TYPE *item;                                                  \
        NcmArrayItemCallbacks *callbacks;                                 \
                                                                         \
        if (!PREFIX##_reserve(array, 1)) {                                \
            return NULL;                                                  \
        }                                                                \
        callbacks = CALLBACKS;                                            \
        item = &array->items[array->len];                                 \
        array->len += 1;                                                  \
        if ((callbacks != NULL) && (callbacks->init != NULL)) {           \
            callbacks->init(item);                                        \
        } else {                                                          \
            char *bytes = (char *)item;                                   \
            for (int32 i = 0; i < (int32)SIZEOF(*item); i += 1) {        \
                bytes[i] = 0;                                             \
            }                                                            \
        }                                                                \
        return item;                                                      \
    }                                                                    \
                                                                         \
    bool                                                                 \
    PREFIX##_append_copy(ARRAY_TYPE *array, ITEM_TYPE *item) {            \
        ITEM_TYPE *dest;                                                  \
        NcmArrayItemCallbacks *callbacks;                                 \
                                                                         \
        if (item == NULL) {                                               \
            return false;                                                 \
        }                                                                \
        dest = PREFIX##_append(array);                                    \
        if (dest == NULL) {                                               \
            return false;                                                 \
        }                                                                \
        callbacks = CALLBACKS;                                            \
        if ((callbacks != NULL) && (callbacks->copy != NULL)) {           \
            if (!callbacks->copy(dest, item)) {                           \
                array->len -= 1;                                          \
                if (callbacks->destroy != NULL) {                         \
                    callbacks->destroy(dest);                             \
                }                                                        \
                return false;                                             \
            }                                                            \
        } else {                                                          \
            *dest = *item;                                                \
        }                                                                \
        return true;                                                      \
    }                                                                    \
                                                                         \
    void                                                                 \
    PREFIX##_append_move(ARRAY_TYPE *array, ITEM_TYPE *item) {            \
        ITEM_TYPE *dest;                                                  \
        NcmArrayItemCallbacks *callbacks;                                 \
                                                                         \
        if (item == NULL) {                                               \
            return;                                                       \
        }                                                                \
        dest = PREFIX##_append(array);                                    \
        if (dest == NULL) {                                               \
            return;                                                       \
        }                                                                \
        callbacks = CALLBACKS;                                            \
        if ((callbacks != NULL) && (callbacks->move != NULL)) {           \
            callbacks->move(dest, item);                                  \
        } else {                                                          \
            *dest = *item;                                                \
        }                                                                \
        return;                                                           \
    }                                                                    \
                                                                         \
    void                                                                 \
    PREFIX##_remove_ordered(ARRAY_TYPE *array, int32 idx) {               \
        NcmArrayItemCallbacks *callbacks;                                 \
                                                                         \
        if (array == NULL) {                                              \
            return;                                                       \
        }                                                                \
        if ((idx < 0) || (idx >= array->len)) {                           \
            return;                                                       \
        }                                                                \
                                                                         \
        callbacks = CALLBACKS;                                            \
        if ((callbacks != NULL) && (callbacks->destroy != NULL)) {        \
            callbacks->destroy(&array->items[idx]);                      \
        }                                                                \
        if (idx + 1 < array->len) {                                       \
            ncm_memmove(&array->items[idx], &array->items[idx + 1],       \
                        (array->len - idx - 1)*SIZEOF(*array->items));    \
        }                                                                \
        array->len -= 1;                                                  \
        return;                                                           \
    }

#if defined(__cplusplus)
}
#endif

#endif /* NCM_ARRAY_H */
