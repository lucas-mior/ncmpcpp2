#include "c/ncm_time.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cbase/base_macros.h"

static void ncm_time_set_errno_error(NcmError *error, int32 code,
                                     char *operation);

static void
ncm_time_set_errno_error(NcmError *error, int32 code, char *operation) {
    char message[256];
    int32 message_len;

    message_len = snprintf(message, SIZEOF(message), "%s: %s",
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
        ncm_time_set_errno_error(error, errno, (char *)"clock_gettime");
        return false;
    }

    point->ns = (int64)timespec.tv_sec*1000000000ll;
    point->ns += (int64)timespec.tv_nsec;
    ncm_error_clear(error);
    return true;
}

bool
ncm_time_sleep_ms(int64 milliseconds, NcmError *error) {
    struct timespec request;
    struct timespec remaining;
    int32 saved_errno;

    if (milliseconds < 0) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("negative sleep interval"));
        return false;
    }

    request.tv_sec = (time_t)(milliseconds / 1000);
    request.tv_nsec = (long)((milliseconds % 1000)*1000000ll);
    while (nanosleep(&request, &remaining) != 0) {
        saved_errno = errno;
        if (saved_errno != EINTR) {
            ncm_time_set_errno_error(error, saved_errno, (char *)"nanosleep");
            return false;
        }
        request = remaining;
    }

    ncm_error_clear(error);
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

double
ncm_time_elapsed_seconds(NcmTimePoint start, NcmTimePoint end) {
    return (double)ncm_time_elapsed_ns(start, end) / 1000000000.0;
}

bool
ncm_time_since_ms(NcmTimePoint start, int64 *milliseconds,
                  NcmError *error) {
    NcmTimePoint now;

    if (milliseconds == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing elapsed output"));
        return false;
    }
    if (!ncm_time_monotonic_now(&now, error)) {
        return false;
    }

    *milliseconds = ncm_time_elapsed_ms(start, now);
    return true;
}
