#if !defined(NCM_TIME_H)
#define NCM_TIME_H

#include "c/ncm_error.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcmTimePoint {
    int64 ns;
} NcmTimePoint;

bool ncm_time_monotonic_now(NcmTimePoint *point, NcmError *error);
bool ncm_time_sleep_ms(int64 milliseconds, NcmError *error);
int64 ncm_time_elapsed_ns(NcmTimePoint start, NcmTimePoint end);
int64 ncm_time_elapsed_ms(NcmTimePoint start, NcmTimePoint end);
double ncm_time_elapsed_seconds(NcmTimePoint start, NcmTimePoint end);
bool ncm_time_since_ms(NcmTimePoint start, int64 *milliseconds,
                       NcmError *error);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_TIME_H */
