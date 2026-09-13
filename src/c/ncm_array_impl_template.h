/*
 * Include-based implementation template for Ncm arrays.
 *
 * Required definitions:
 *   NCM_ARRAY_TYPE
 *   NCM_ARRAY_ITEM_TYPE
 *   NCM_ARRAY_PREFIX
 *
 * Optional item operation definitions:
 *   NCM_ARRAY_ITEM_INIT
 *   NCM_ARRAY_ITEM_DESTROY
 *   NCM_ARRAY_ITEM_COPY
 *   NCM_ARRAY_ITEM_MOVE
 *
 * Optional operation markers:
 *   NCM_ARRAY_COPY
 *   NCM_ARRAY_MOVE
 *   NCM_ARRAY_SWAP
 *   NCM_ARRAY_APPEND_COPY
 *   NCM_ARRAY_APPEND_MOVE
 *   NCM_ARRAY_REMOVE_ORDERED
 */

#include "cbase.h"

#if defined(__INCLUDE_LEVEL__) && (__INCLUDE_LEVEL__ == 0)
#define NCM_ARRAY_DUMMY_DEFINES 1
#include "ncm_array_decl_template.h"
#endif

#if !defined(NCM_ARRAY_TYPE)
#error "NCM_ARRAY_TYPE must be defined"
#endif
#if !defined(NCM_ARRAY_ITEM_TYPE)
#error "NCM_ARRAY_ITEM_TYPE must be defined"
#endif
#if !defined(NCM_ARRAY_PREFIX)
#error "NCM_ARRAY_PREFIX must be defined"
#endif
#if defined(NCM_ARRAY_COPY) && !defined(NCM_ARRAY_APPEND_COPY)
#error "NCM_ARRAY_COPY requires NCM_ARRAY_APPEND_COPY"
#endif

#define NCM_ARRAY_FUNCTION(SUFFIX) CAT(NCM_ARRAY_PREFIX, SUFFIX)

void
NCM_ARRAY_FUNCTION(_clear)(NCM_ARRAY_TYPE *array) {
    if (array == NULL) {
        return;
    }
#if defined(NCM_ARRAY_ITEM_DESTROY)
    for (int32 i = 0; i < array->len; i += 1) {
        NCM_ARRAY_ITEM_DESTROY(&array->items[i]);
    }
#endif
    array->len = 0;
    return;
}

void
NCM_ARRAY_FUNCTION(_destroy)(NCM_ARRAY_TYPE *array) {
    if (array == NULL) {
        return;
    }
    NCM_ARRAY_FUNCTION(_clear)(array);
    free2(array->items, array->cap*SIZEOF(*array->items));
    *array = (NCM_ARRAY_TYPE){0};
    return;
}

#if defined(NCM_ARRAY_COPY)
int32
NCM_ARRAY_FUNCTION(_copy)(NCM_ARRAY_TYPE *dest, NCM_ARRAY_TYPE *source) {
    NCM_ARRAY_TYPE replacement = {0};
    int32 err;

    if (dest == NULL) {
        return -EINVAL;
    }
    if (dest == source) {
        return dest->len;
    }

    if (source) {
        if ((err = NCM_ARRAY_FUNCTION(_reserve)(
                 &replacement, source->len)) < 0) {
            NCM_ARRAY_FUNCTION(_destroy)(&replacement);
            return err;
        }
        for (int32 i = 0; i < source->len; i += 1) {
            if ((err = NCM_ARRAY_FUNCTION(_append_copy)(
                     &replacement, &source->items[i])) < 0) {
                NCM_ARRAY_FUNCTION(_destroy)(&replacement);
                return err;
            }
        }
    }

    NCM_ARRAY_FUNCTION(_destroy)(dest);
    *dest = replacement;
    return dest->len;
}
#endif

#if defined(NCM_ARRAY_MOVE)
void
NCM_ARRAY_FUNCTION(_move)(NCM_ARRAY_TYPE *dest, NCM_ARRAY_TYPE *source) {
    if (dest == NULL) {
        return;
    }
    if (dest == source) {
        return;
    }

    NCM_ARRAY_FUNCTION(_destroy)(dest);
    if (source == NULL) {
        *dest = (NCM_ARRAY_TYPE){0};
        return;
    }
    *dest = *source;
    *source = (NCM_ARRAY_TYPE){0};
    return;
}
#endif

#if defined(NCM_ARRAY_SWAP)
void
NCM_ARRAY_FUNCTION(_swap)(NCM_ARRAY_TYPE *left, NCM_ARRAY_TYPE *right) {
    NCM_ARRAY_TYPE temp;

    if (left == NULL) {
        return;
    }
    if (right == NULL) {
        return;
    }
    temp = *left;
    *left = *right;
    *right = temp;
    return;
}
#endif

int32
NCM_ARRAY_FUNCTION(_reserve)(NCM_ARRAY_TYPE *array, int32 extra) {
    int64 needed;
    int32 old_cap;
    int32 new_cap;

    if (array == NULL) {
        return -EINVAL;
    }
    if (extra < 0) {
        return -EINVAL;
    }
    if (extra == 0) {
        return array->cap;
    }

    needed = (int64)array->len + extra;
    if (needed <= array->cap) {
        return array->cap;
    }
    if (needed >= MAXOF(array->cap)) {
        error("Array only supports fewer than 2GB items.\n");
        fatal(EXIT_FAILURE);
    }

    old_cap = array->cap;
    new_cap = array->cap;
    if (new_cap <= 0) {
        new_cap = 8;
    }
    if (needed >= (MAXOF(new_cap)/2)) {
        new_cap = (int32)needed;
    } else {
        while (new_cap < needed) {
            new_cap *= 2;
        }
    }

    array->items = realloc2(
        array->items, old_cap, new_cap, SIZEOF(*array->items));
    array->cap = new_cap;
    return array->cap;
}

NCM_ARRAY_ITEM_TYPE *
NCM_ARRAY_FUNCTION(_append)(NCM_ARRAY_TYPE *array) {
    NCM_ARRAY_ITEM_TYPE *item;

    if (NCM_ARRAY_FUNCTION(_reserve)(array, 1) < 0) {
        return NULL;
    }
    item = &array->items[array->len];
    array->len += 1;
#if defined(NCM_ARRAY_ITEM_INIT)
    NCM_ARRAY_ITEM_INIT(item);
#else
    {
        char *bytes = (char *)item;
        for (int32 i = 0; i < (int32)SIZEOF(*item); i += 1) {
            bytes[i] = 0;
        }
    }
#endif
    return item;
}

#if !defined(NCM_ARRAY_ITEM_DESTROY)
#define NCM_ARRAY_ITEM_DESTROY(A)
#endif

#if defined(NCM_ARRAY_APPEND_COPY)
int32
NCM_ARRAY_FUNCTION(_append_copy)(NCM_ARRAY_TYPE *array,
                                 NCM_ARRAY_ITEM_TYPE *item) {
    NCM_ARRAY_ITEM_TYPE *dest;
    int32 err;
    int32 index;

    if ((array == NULL) || (item == NULL)) {
        return -EINVAL;
    }
    if ((err = NCM_ARRAY_FUNCTION(_reserve)(array, 1)) < 0) {
        return err;
    }
    index = array->len;
    dest = &array->items[index];
    array->len += 1;
#if defined(NCM_ARRAY_ITEM_INIT)
    NCM_ARRAY_ITEM_INIT(dest);
#else
    *dest = (NCM_ARRAY_ITEM_TYPE){0};
#endif
#if defined(NCM_ARRAY_ITEM_COPY)
    if ((err = NCM_ARRAY_ITEM_COPY(dest, item)) < 0) {
        array->len -= 1;
        NCM_ARRAY_ITEM_DESTROY(dest);
        return err;
    }
#else
    *dest = *item;
#endif
    return index;
}
#endif

#if !defined(NCM_ARRAY_ITEM_MOVE)
#define NCM_ARRAY_ITEM_MOVE(A, B) *(A) = *(B)
#endif

#if defined(NCM_ARRAY_APPEND_MOVE)
void
NCM_ARRAY_FUNCTION(_append_move)(NCM_ARRAY_TYPE *array,
                                 NCM_ARRAY_ITEM_TYPE *item) {
    NCM_ARRAY_ITEM_TYPE *dest;

    if (item == NULL) {
        return;
    }
    dest = NCM_ARRAY_FUNCTION(_append)(array);
    if (dest == NULL) {
        return;
    }

    NCM_ARRAY_ITEM_MOVE(dest, item);
    return;
}
#endif

#if defined(NCM_ARRAY_REMOVE_ORDERED)
void
NCM_ARRAY_FUNCTION(_remove_ordered)(NCM_ARRAY_TYPE *array, int32 idx) {
    if (array == NULL) {
        return;
    }
    if ((idx < 0) || (idx >= array->len)) {
        return;
    }

    NCM_ARRAY_ITEM_DESTROY(&array->items[idx]);
    if (idx + 1 < array->len) {
        memmove64(&array->items[idx],
                  &array->items[idx + 1],
                  (array->len - idx - 1)*SIZEOF(*array->items));
    }
    array->len -= 1;
    return;
}
#endif

#undef NCM_ARRAY_FUNCTION
#undef NCM_ARRAY_TYPE
#undef NCM_ARRAY_ITEM_TYPE
#undef NCM_ARRAY_PREFIX
#undef NCM_ARRAY_ITEM_INIT
#undef NCM_ARRAY_ITEM_DESTROY
#undef NCM_ARRAY_ITEM_COPY
#undef NCM_ARRAY_ITEM_MOVE
#undef NCM_ARRAY_COPY
#undef NCM_ARRAY_MOVE
#undef NCM_ARRAY_SWAP
#undef NCM_ARRAY_APPEND_COPY
#undef NCM_ARRAY_APPEND_MOVE
#undef NCM_ARRAY_REMOVE_ORDERED
