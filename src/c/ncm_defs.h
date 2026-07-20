#if !defined(NCM_DEFS_H)
#define NCM_DEFS_H

#include <stdbool.h>

#include "cbase/base_macros.h"
#include "cbase/primitives.h"

typedef struct NcmStringView {
    char *data;
    int32 len;
} NcmStringView;

typedef struct NcmBuffer {
    char *data;
    int32 len;
    int32 cap;
} NcmBuffer;

#endif /* NCM_DEFS_H */
