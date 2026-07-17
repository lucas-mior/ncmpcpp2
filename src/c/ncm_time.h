#if !defined(NCM_TIME_H)
#define NCM_TIME_H

#include "c/ncm_error.h"

typedef struct NcmTimePoint {
    int64 ns;
} NcmTimePoint;

bool ncm_time_monotonic_now(NcmTimePoint *point, NcmError *error);
int64 ncm_time_elapsed_ns(NcmTimePoint start, NcmTimePoint end);
int64 ncm_time_elapsed_ms(NcmTimePoint start, NcmTimePoint end);

#endif /* NCM_TIME_H */
