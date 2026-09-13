/*
 * Include-based declaration template for Ncm arrays.
 *
 * Required definitions:
 *   NCM_ARRAY_TYPE
 *   NCM_ARRAY_ITEM_TYPE
 *   NCM_ARRAY_PREFIX
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

#if !defined(NCM_ARRAY_DUMMY_DEFINES)
#define NCM_ARRAY_DUMMY_DEFINES 0
#endif

#if NCM_ARRAY_DUMMY_DEFINES \
    || (defined(__INCLUDE_LEVEL__) && (__INCLUDE_LEVEL__ == 0))
#define NCM_ARRAY_TYPE      ncm_unused
#define NCM_ARRAY_ITEM_TYPE int32
#define NCM_ARRAY_PREFIX    ncm_unused2
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

#define NCM_ARRAY_FUNCTION(SUFFIX) CAT(NCM_ARRAY_PREFIX, SUFFIX)

typedef struct NCM_ARRAY_TYPE {
    NCM_ARRAY_ITEM_TYPE *items;
    int32 len;
    int32 cap;
} NCM_ARRAY_TYPE;

void NCM_ARRAY_FUNCTION(_clear)(NCM_ARRAY_TYPE *);
void NCM_ARRAY_FUNCTION(_destroy)(NCM_ARRAY_TYPE *);
int32 NCM_ARRAY_FUNCTION(_reserve)(NCM_ARRAY_TYPE *, int32);
NCM_ARRAY_ITEM_TYPE *NCM_ARRAY_FUNCTION(_append)(NCM_ARRAY_TYPE *);

#if defined(NCM_ARRAY_COPY)
int32 NCM_ARRAY_FUNCTION(_copy)(NCM_ARRAY_TYPE *dest, NCM_ARRAY_TYPE *source);
#endif

#if defined(NCM_ARRAY_MOVE)
void NCM_ARRAY_FUNCTION(_move)(NCM_ARRAY_TYPE *dest, NCM_ARRAY_TYPE *source);
#endif

#if defined(NCM_ARRAY_SWAP)
void NCM_ARRAY_FUNCTION(_swap)(NCM_ARRAY_TYPE *left, NCM_ARRAY_TYPE *right);
#endif

#if defined(NCM_ARRAY_APPEND_COPY)
int32 NCM_ARRAY_FUNCTION(_append_copy)(NCM_ARRAY_TYPE *,
                                       NCM_ARRAY_ITEM_TYPE *);
#endif

#if defined(NCM_ARRAY_APPEND_MOVE)
void NCM_ARRAY_FUNCTION(_append_move)(NCM_ARRAY_TYPE *,
                                      NCM_ARRAY_ITEM_TYPE *);
#endif

#if defined(NCM_ARRAY_REMOVE_ORDERED)
void NCM_ARRAY_FUNCTION(_remove_ordered)(NCM_ARRAY_TYPE *, int32);
#endif

#undef NCM_ARRAY_FUNCTION
#undef NCM_ARRAY_TYPE
#undef NCM_ARRAY_ITEM_TYPE
#undef NCM_ARRAY_PREFIX
#undef NCM_ARRAY_COPY
#undef NCM_ARRAY_MOVE
#undef NCM_ARRAY_SWAP
#undef NCM_ARRAY_APPEND_COPY
#undef NCM_ARRAY_APPEND_MOVE
#undef NCM_ARRAY_REMOVE_ORDERED
