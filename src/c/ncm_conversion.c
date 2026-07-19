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
    uint8 c;

    while (*cursor != '\0') {
        c = (uint8)*cursor;
        if (!isspace(c)) {
            return false;
        }
        cursor += 1;
    }

    return true;
}

static bool
ncm_conversion_is_negative_source(char *source) {
    uint8 c;

    while (*source != '\0') {
        c = (uint8)*source;
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
    if (len < 0) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("conversion failed"));
        return;
    }
    if (len >= (int32)SIZEOF(message)) {
        len = (int32)SIZEOF(message) - 1;
    }

    ncm_error_set(error, EINVAL, message, len);
    return;
}

static void
ncm_conversion_set_u64_bounds_error(NcmError *error, uint64 value,
                                    uint64 lbound, uint64 ubound) {
    char message[256];
    int32 len;

    len = SNPRINTF(message,
                   "value is out of bounds ([%" PRIu64 ", %" PRIu64
                   "] expected, %" PRIu64 " given)",
                   lbound, ubound, value);
    if (len < 0) {
        ncm_error_set(error, ERANGE, STRLIT_ARGS("value is out of bounds"));
        return;
    }
    if (len >= (int32)SIZEOF(message)) {
        len = (int32)SIZEOF(message) - 1;
    }

    ncm_error_set(error, ERANGE, message, len);
    return;
}

static void
ncm_conversion_set_u64_upper_error(NcmError *error, uint64 value,
                                   uint64 ubound) {
    char message[256];
    int32 len;

    len = SNPRINTF(message,
                   "value is out of bounds ((<-, %" PRIu64
                   "] expected, %" PRIu64 " given)",
                   ubound, value);
    if (len < 0) {
        ncm_error_set(error, ERANGE, STRLIT_ARGS("value is out of bounds"));
        return;
    }
    if (len >= (int32)SIZEOF(message)) {
        len = (int32)SIZEOF(message) - 1;
    }

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
    if (len < 0) {
        ncm_error_set(error, ERANGE, STRLIT_ARGS("value is out of bounds"));
        return;
    }
    if (len >= (int32)SIZEOF(message)) {
        len = (int32)SIZEOF(message) - 1;
    }

    ncm_error_set(error, ERANGE, message, len);
    return;
}

static void
ncm_conversion_set_f64_lower_error(NcmError *error, double value,
                                   double lbound) {
    char message[256];
    int32 len;

    len = SNPRINTF(message,
                   "value is out of bounds ([%g, ->) expected, %g given)",
                   lbound, value);
    if (len < 0) {
        ncm_error_set(error, ERANGE, STRLIT_ARGS("value is out of bounds"));
        return;
    }
    if (len >= (int32)SIZEOF(message)) {
        len = (int32)SIZEOF(message) - 1;
    }

    ncm_error_set(error, ERANGE, message, len);
    return;
}

bool
ncm_parse_int64(char *source, int32 source_len,
                int64 *out, NcmError *error) {
    NcmBuffer buffer;
    char *end;
    uint64 value;
    bool ok;

    if (out == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing conversion output"));
        return false;
    }

    ncm_buffer_init(&buffer);
    if (!ncm_conversion_copy_source(&buffer, source, source_len, error)) {
        ncm_buffer_destroy(&buffer);
        return false;
    }

    errno = 0;
    value = strtoull(buffer.data, &end, 10);
    ok = (end != buffer.data)
         && !ncm_conversion_is_negative_source(buffer.data)
         && ncm_conversion_trailing_space_only(end)
         && (errno != ERANGE)
         && (value <= (uint64)MAXOF(*out));
    if (ok) {
        *out = (int64)value;
        ncm_error_clear(error);
    } else {
        ncm_conversion_set_parse_error(error, source, source_len);
    }

    ncm_buffer_destroy(&buffer);
    return ok;
}

bool
ncm_parse_int32(char *source, int32 source_len, int32 *out, NcmError *error) {
    int64 value;

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

    *out = (int32)value;
    ncm_error_clear(error);
    return true;
}

bool
ncm_parse_double(char *source, int32 source_len,
                 double *out, NcmError *error) {
    NcmBuffer buffer;
    char *end;
    double value;
    bool ok;

    if (out == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing conversion output"));
        return false;
    }

    ncm_buffer_init(&buffer);
    if (!ncm_conversion_copy_source(&buffer, source, source_len, error)) {
        ncm_buffer_destroy(&buffer);
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

    ncm_buffer_destroy(&buffer);
    return ok;
}

bool
ncm_bounds_check_u64(uint64 value, uint64 lbound, uint64 ubound,
                     NcmError *error) {
    if ((value < lbound) || (value > ubound)) {
        ncm_conversion_set_u64_bounds_error(error, value, lbound, ubound);
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
ncm_lower_bound_check_f64(double value, double lbound,
                          NcmError *error) {
    if (value < lbound) {
        ncm_conversion_set_f64_lower_error(error, value, lbound);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

#endif /* NCM_CONVERSION_C */
