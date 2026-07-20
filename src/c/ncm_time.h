#if !defined(NCM_TIME_H)
#define NCM_TIME_H

#include "c/ncm_error.h"

typedef struct NcmTimePoint {
    int32 ns;
} NcmTimePoint;

bool ncm_time_monotonic_now(NcmTimePoint *point, NcmError *error);
int32 ncm_time_elapsed_ns(NcmTimePoint start, NcmTimePoint end);
int32 ncm_time_elapsed_ms(NcmTimePoint start, NcmTimePoint end);

#endif /* NCM_TIME_H */
