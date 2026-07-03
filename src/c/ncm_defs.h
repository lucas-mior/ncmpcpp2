#if !defined(NCM_DEFS_H)
#define NCM_DEFS_H

#include <stdbool.h>

#include "cbase/primitives.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define NCM_ARRAY_LEN(array) ((int32)(sizeof(array) / sizeof((array)[0])))

typedef struct NcmStringView {
    char *data;
    int32 len;
} NcmStringView;

typedef struct NcmBuffer {
    char *data;
    int32 len;
    int32 cap;
} NcmBuffer;

#if defined(__cplusplus)
}
#endif

#endif /* NCM_DEFS_H */
