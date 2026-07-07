#if !defined(NCM_FUNCTIONAL_H)
#define NCM_FUNCTIONAL_H

#include "c/ncm_defs.h"

NCM_EXTERN_C_BEGIN

typedef bool (*NcmFindPredicate)(void *item, void *user);
typedef void (*NcmMapFunction)(void *item, void *user);

int32 ncm_find_map_first(void *items, int32 items_len, int32 item_size,
                         NcmFindPredicate predicate, void *predicate_user,
                         NcmMapFunction map, void *map_user);
int32 ncm_find_map_all(void *items, int32 items_len, int32 item_size,
                       NcmFindPredicate predicate, void *predicate_user,
                       NcmMapFunction map, void *map_user);

NCM_EXTERN_C_END

#endif /* NCM_FUNCTIONAL_H */
