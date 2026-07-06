#include "c/ncm_scoped_value.h"


#include <stddef.h>
#include "c/ncm_base.h"

void
ncm_scoped_value_init(NcmScopedValue *scope) {
    scope->target = NULL;
    scope->saved = NULL;
    scope->size = 0;
    return;
}

bool
ncm_scoped_value_begin(NcmScopedValue *scope, void *target,
                       void *temporary_value, int32 size) {
    if ((scope == NULL) || (target == NULL)
        || (temporary_value == NULL) || (size <= 0)) {
        return false;
    }

    ncm_scoped_value_discard(scope);
    scope->saved = ncm_malloc(size);
    scope->target = target;
    scope->size = size;
    ncm_memcpy(scope->saved, target, size);
    ncm_memcpy(target, temporary_value, size);
    return true;
}

void
ncm_scoped_value_restore(NcmScopedValue *scope) {
    if ((scope == NULL) || (scope->saved == NULL)
        || (scope->target == NULL) || (scope->size <= 0)) {
        return;
    }

    ncm_memcpy(scope->target, scope->saved, scope->size);
    ncm_scoped_value_discard(scope);
    return;
}

void
ncm_scoped_value_discard(NcmScopedValue *scope) {
    if (scope == NULL) {
        return;
    }
    if (scope->saved) {
        ncm_free(scope->saved, scope->size);
    }

    scope->target = NULL;
    scope->saved = NULL;
    scope->size = 0;
    return;
}
