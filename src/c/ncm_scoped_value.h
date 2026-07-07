#if !defined(NCM_SCOPED_VALUE_H)
#define NCM_SCOPED_VALUE_H

#include "c/ncm_defs.h"

NCM_EXTERN_C_BEGIN

typedef struct NcmScopedValue {
    void *target;
    void *saved;
    int32 size;
} NcmScopedValue;

void ncm_scoped_value_init(NcmScopedValue *scope);
bool ncm_scoped_value_begin(NcmScopedValue *scope, void *target,
                            void *temporary_value, int32 size);
void ncm_scoped_value_restore(NcmScopedValue *scope);
void ncm_scoped_value_discard(NcmScopedValue *scope);

NCM_EXTERN_C_END

#endif /* NCM_SCOPED_VALUE_H */
