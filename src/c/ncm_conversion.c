#if !defined(NCM_CONVERSION_C)
#define NCM_CONVERSION_C

#include "c/ncm_conversion.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "c/ncm_base.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"

static bool
ncm_conversion_copy_source(NcmBuffer *buffer, char *source,
                           int32 source_len, NcmError *error) {
    ncm_buffer_clear(buffer);

    if (source == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing conversion source"));
        return false;
    }
    if (source_len < 0) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("negative source length"));
        return false;
    }

    ncm_buffer_append(buffer, source, source_len);
    if (buffer->data == NULL) {
        ncm_buffer_append_byte(buffer, '\0');
        buffer->len = 0;
    }

    return true;
}

static bool
ncm_conversion_trailing_space_only(char *cursor) {

    while (*cursor != '\0') {
        uint8 c = (uint8)*cursor;
        if (!isspace(c)) {
            return false;
        }
        cursor += 1;
    }

    return true;
}

static bool
ncm_conversion_is_negative_source(char *source) {
    while (*source != '\0') {
        uint8 c = (uint8)*source;
        if (!isspace(c)) {
            break;
        }
        source += 1;
    }

    return *source == '-';
}

static void
ncm_conversion_set_parse_error(NcmError *error, char *source,
                               int32 source_len) {
    char message[256];
    int32 len;
    len = SNPRINTF(message,
                   "conversion failed for '%.*s'", source_len, source);
    ncm_error_set(error, EINVAL, message, len);
    return;
}

static void
ncm_conversion_set_i64_bounds_error(NcmError *error, int64 value,
                                    int64 lbound, int64 ubound) {
    char message[256];
    int32 len;

    len = SNPRINTF(message,
                   "value is out of bounds ([%lld, %lld] expected, %lld given)",
                   (llong)lbound, (llong)ubound, (llong)value);

    ncm_error_set(error, ERANGE, message, len);
    return;
}

static void
ncm_conversion_set_f64_bounds_error(NcmError *error, double value,
                                    double lbound, double ubound) {
    char message[256];
    int32 len;

    len = SNPRINTF(message,
                   "value is out of bounds ([%g, %g] expected, %g given)",
                   lbound, ubound, value);

    ncm_error_set(error, ERANGE, message, len);
    return;
}

static void
ncm_conversion_set_f64_lower_error(NcmError *error,
                                   double value, double lbound) {
    char message[256];
    int32 len;

    len = SNPRINTF(message,
                   "value is out of bounds ([%g, ->) expected, %g given)",
                   lbound, value);

    ncm_error_set(error, ERANGE, message, len);
    return;
}

bool
ncm_parse_int64(char *source, int32 source_len, int32 *out, NcmError *error) {
    StrBuilder buffer;
    char *end;
    int64 value;
    bool ok;

    if (out == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing conversion output"));
        return false;
    }

    sb_init(&buffer);
    if (!ncm_conversion_copy_source(&buffer, source, source_len, error)) {
        sb_free(&buffer);
        return false;
    }

    errno = 0;
    value = strtoll(buffer.data, &end, 10);
    ok = (end != buffer.data)
         && !ncm_conversion_is_negative_source(buffer.data)
         && ncm_conversion_trailing_space_only(end)
         && (errno != ERANGE)
         && (value <= MAXOF(*out));
    if (ok) {
        *out = (int32)value;
        ncm_error_clear(error);
    } else {
        ncm_conversion_set_parse_error(error, source, source_len);
    }

    sb_free(&buffer);
    return ok;
}

bool
ncm_parse_int32(char *source, int32 source_len, int32 *out, NcmError *error) {
    int32 value;

    if (out == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing conversion output"));
        return false;
    }

    if (!ncm_parse_int64(source, source_len, &value, error)) {
        return false;
    }
    if (value > MAXOF(*out)) {
        fatal(EXIT_FAILURE);
    }

    *out = value;
    ncm_error_clear(error);
    return true;
}

bool
ncm_parse_double(char *source, int32 source_len, double *out, NcmError *error) {
    StrBuilder buffer;
    char *end;
    double value;
    bool ok;

    if (out == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing conversion output"));
        return false;
    }

    sb_init(&buffer);
    if (!ncm_conversion_copy_source(&buffer, source, source_len, error)) {
        sb_free(&buffer);
        return false;
    }

    errno = 0;
    value = strtod(buffer.data, &end);
    ok = (end != buffer.data)
         && ncm_conversion_trailing_space_only(end)
         && (errno != ERANGE);
    if (ok) {
        *out = value;
        ncm_error_clear(error);
    } else {
        ncm_conversion_set_parse_error(error, source, source_len);
    }

    sb_free(&buffer);
    return ok;
}

bool
ncm_bounds_check_i64(int64 value, int64 lbound, int64 ubound, NcmError *error) {
    if ((value < lbound) || (value > ubound)) {
        ncm_conversion_set_i64_bounds_error(error, value, lbound, ubound);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_bounds_check_f64(double value, double lbound, double ubound,
                     NcmError *error) {
    if ((value < lbound) || (value > ubound)) {
        ncm_conversion_set_f64_bounds_error(error, value, lbound, ubound);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_lower_bound_check_f64(double value, double lbound, NcmError *error) {
    if (value < lbound) {
        ncm_conversion_set_f64_lower_error(error, value, lbound);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

#endif /* NCM_CONVERSION_C */
