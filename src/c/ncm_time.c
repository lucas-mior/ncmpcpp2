#if !defined(NCM_TIME_C)
#define NCM_TIME_C

#include "cbase.h"

#include "c/ncm_time.h"

static void
ncm_time_set_errno_error(NcmError *ncm_error, int32 code, char *operation) {
    char message[256];
    int32 message_len;

    message_len = SNPRINTF(message, "%s: %s",
                           operation, strerror(code));
    ncm_error_set(ncm_error, code, message, message_len);
    return;
}

bool
ncm_time_monotonic_now(NcmTimePoint *point, NcmError *ncm_error) {
    struct timespec timespec;

    if (point == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing time point"));
        return false;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &timespec) != 0) {
        ncm_time_set_errno_error(ncm_error, errno, "clock_gettime");
        return false;
    }

    point->ns = (int64)timespec.tv_sec*1000000000ll;
    point->ns += (int64)timespec.tv_nsec;
    ncm_error_clear(ncm_error);
    return true;
}

int64
ncm_time_elapsed_ns(NcmTimePoint start, NcmTimePoint end) {
    return end.ns - start.ns;
}

int64
ncm_time_elapsed_ms(NcmTimePoint start, NcmTimePoint end) {
    return ncm_time_elapsed_ns(start, end) / 1000000ll;
}

#endif /* NCM_TIME_C */
