#if !defined(NCM_TIME_C)
#define NCM_TIME_C

#include "c/ncm_time.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cbase/base_macros.h"
#include "cbase/util.c"

static void
ncm_time_set_errno_error(NcmError *error, int32 code, char *operation) {
    char message[256];
    int32 message_len;

    message_len = SNPRINTF(message, "%s: %s",
                           operation, strerror(code));
    ncm_error_set(error, code, message, message_len);
    return;
}

bool
ncm_time_monotonic_now(NcmTimePoint *point, NcmError *error) {
    struct timespec timespec;

    if (point == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing time point"));
        return false;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &timespec) != 0) {
        ncm_time_set_errno_error(error, errno, "clock_gettime");
        return false;
    }

    point->ns = (int32)timespec.tv_sec*1000000000ll;
    point->ns += (int32)timespec.tv_nsec;
    ncm_error_clear(error);
    return true;
}

int32
ncm_time_elapsed_ns(NcmTimePoint start, NcmTimePoint end) {
    return end.ns - start.ns;
}

int32
ncm_time_elapsed_ms(NcmTimePoint start, NcmTimePoint end) {
    return ncm_time_elapsed_ns(start, end) / 1000000ll;
}

#endif /* NCM_TIME_C */
