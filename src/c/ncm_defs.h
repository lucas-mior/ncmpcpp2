#if !defined(NCM_DEFS_H)
#define NCM_DEFS_H

#include <stdbool.h>

#include "cbase/base_macros.h"
#include "cbase/util.h"

typedef struct NcmStringView {
    char *data;
    int32 len;
} NcmStringView;

typedef StrBuilder NcmBuffer;

#endif /* NCM_DEFS_H */
