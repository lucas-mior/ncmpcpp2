#include "c/ncm_iterator.h"


#include <stddef.h>
static bool ncm_any_iterator_same_range(NcmAnyIterator *left,
                                        NcmAnyIterator *right);

static bool
ncm_any_iterator_same_range(NcmAnyIterator *left, NcmAnyIterator *right) {
    if ((left == NULL) || (right == NULL)) {
        return false;
    }

    return (left->items == right->items)
           && (left->items_len == right->items_len)
           && (left->item_size == right->item_size);
}

void
ncm_any_iterator_init(NcmAnyIterator *iterator,
                      void *items, int32 items_len,
                      int32 item_size, int32 index) {
    if (iterator == NULL) {
        return;
    }

    iterator->items = items;
    iterator->items_len = items_len;
    iterator->item_size = item_size;
    iterator->index = index;
    return;
}

NcmAnyIterator
ncm_any_iterator_begin(void *items, int32 items_len, int32 item_size) {
    NcmAnyIterator result;

    ncm_any_iterator_init(&result, items, items_len, item_size, 0);
    return result;
}

NcmAnyIterator
ncm_any_iterator_end(void *items, int32 items_len, int32 item_size) {
    NcmAnyIterator result;

    ncm_any_iterator_init(&result, items, items_len, item_size, items_len);
    return result;
}

bool
ncm_any_iterator_valid(NcmAnyIterator *iterator) {
    if (iterator == NULL) {
        return false;
    }
    if ((iterator->items == NULL) || (iterator->item_size <= 0)) {
        return false;
    }

    return (iterator->index >= 0)
           && (iterator->index < iterator->items_len);
}

void *
ncm_any_iterator_deref(NcmAnyIterator *iterator) {
    if (!ncm_any_iterator_valid(iterator)) {
        return NULL;
    }

    return iterator->items + iterator->index*iterator->item_size;
}

void
ncm_any_iterator_advance(NcmAnyIterator *iterator, int32 offset) {
    if (iterator == NULL) {
        return;
    }

    iterator->index += offset;
    return;
}

void
ncm_any_iterator_next(NcmAnyIterator *iterator) {
    ncm_any_iterator_advance(iterator, 1);
    return;
}

int32
ncm_any_iterator_distance(NcmAnyIterator *left, NcmAnyIterator *right) {
    if (!ncm_any_iterator_same_range(left, right)) {
        return 0;
    }

    return left->index - right->index;
}

void
ncm_transform_iterator_init(NcmTransformIterator *iterator,
                            NcmAnyIterator base,
                            NcmTransformIteratorFunction transform,
                            void *user) {
    if (iterator == NULL) {
        return;
    }

    iterator->iterator = base;
    iterator->transform = transform;
    iterator->user = user;
    return;
}

void *
ncm_transform_iterator_deref(NcmTransformIterator *iterator) {
    void *item;

    if (iterator == NULL) {
        return NULL;
    }

    item = ncm_any_iterator_deref(&iterator->iterator);
    if (item == NULL) {
        return NULL;
    }
    if (iterator->transform == NULL) {
        return item;
    }

    return iterator->transform(item, iterator->user);
}
