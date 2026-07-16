#if !defined(NCM_ITERATOR_H)
#define NCM_ITERATOR_H

#include "c/ncm_defs.h"

typedef struct NcmAnyIterator {
    char *items;
    int32 items_len;
    int32 item_size;
    int32 index;
} NcmAnyIterator;

typedef void *(*NcmTransformIteratorFunction)(void *item, void *user);

typedef struct NcmTransformIterator {
    NcmAnyIterator iterator;
    NcmTransformIteratorFunction transform;
    void *user;
} NcmTransformIterator;

void ncm_any_iterator_init(NcmAnyIterator *iterator,
                           void *items, int32 items_len,
                           int32 item_size, int32 index);
NcmAnyIterator ncm_any_iterator_begin(void *items, int32 items_len,
                                      int32 item_size);
NcmAnyIterator ncm_any_iterator_end(void *items, int32 items_len,
                                    int32 item_size);
bool ncm_any_iterator_valid(NcmAnyIterator *iterator);
void *ncm_any_iterator_deref(NcmAnyIterator *iterator);
void *ncm_any_iterator_at(NcmAnyIterator *iterator, int32 offset);
void ncm_any_iterator_advance(NcmAnyIterator *iterator, int32 offset);
void ncm_any_iterator_next(NcmAnyIterator *iterator);
void ncm_any_iterator_prev(NcmAnyIterator *iterator);
int32 ncm_any_iterator_distance(NcmAnyIterator *left,
                                NcmAnyIterator *right);
bool ncm_any_iterator_equals(NcmAnyIterator *left, NcmAnyIterator *right);
bool ncm_any_iterator_less(NcmAnyIterator *left, NcmAnyIterator *right);

void ncm_transform_iterator_init(NcmTransformIterator *iterator,
                                 NcmAnyIterator base,
                                 NcmTransformIteratorFunction transform,
                                 void *user);
void *ncm_transform_iterator_deref(NcmTransformIterator *iterator);
void *ncm_transform_iterator_at(NcmTransformIterator *iterator,
                                int32 offset);
void ncm_transform_iterator_advance(NcmTransformIterator *iterator,
                                    int32 offset);

#endif /* NCM_ITERATOR_H */
