#if !defined(NCM_TIME_C)
#define NCM_TIME_C

#include "cbase.h"

#include "c/ncm_c.h"

int32
ncm_time_monotonic_now(NcmTimePoint *point, NcmError *ncm_error) {
    struct timespec timespec;

    if (point == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing time point"));
    }

    if (clock_gettime(CLOCK_MONOTONIC, &timespec) != 0) {
        char message[256];
        int32 code = errno;
        int32 message_len;

        message_len = SNPRINTF(message, "clock_gettime: %s", strerror(code));
        return ncm_error_set_status(ncm_error, -code, message, message_len);
    }

    point->ns = (int64)timespec.tv_sec*1000000000ll;
    point->ns += (int64)timespec.tv_nsec;
    return ncm_error_ok(ncm_error);
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
