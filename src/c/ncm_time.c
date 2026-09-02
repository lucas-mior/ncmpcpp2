#if !defined(NCM_TIME_C)
#define NCM_TIME_C

#include "cbase.h"

#include "c/ncm_c.h"

static int32
ncm_time_set_errno_error(NcmError *ncm_error, int32 code, char *operation) {
    char message[256];
    int32 message_len;

    message_len = SNPRINTF(message, "%s: %s",
                           operation, strerror(code));
    return ncm_error_set_status(ncm_error, -code, message, message_len);
}

int32
ncm_time_monotonic_now(NcmTimePoint *point, NcmError *ncm_error) {
    struct timespec timespec;
    int32 code;

    if (point == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing time point"));
    }

    if (clock_gettime(CLOCK_MONOTONIC, &timespec) != 0) {
        code = errno;
        return ncm_time_set_errno_error(ncm_error, code, "clock_gettime");
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
