#if !defined(NCM_DEFS_H)
#define NCM_DEFS_H

#include <stdbool.h>

#include "cbase/primitives.h"

#if defined(__cplusplus)
#define NCM_EXTERN_C_BEGIN extern "C" {
#define NCM_EXTERN_C_END }
#else
#define NCM_EXTERN_C_BEGIN
#define NCM_EXTERN_C_END
#endif

NCM_EXTERN_C_BEGIN

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

NCM_EXTERN_C_END

#endif /* NCM_DEFS_H */
