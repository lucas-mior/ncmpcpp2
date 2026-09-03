#if !defined(NCM_CONVERSION_C)
#define NCM_CONVERSION_C

#include "cbase.h"

#include "c/ncm_c.h"

static int32
ncm_conversion_copy_source(StrBuilder *buffer, char *source, int32 source_len,
                           NcmError *ncm_error) {
    sb_clear(buffer);

    if (source == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing conversion source"));
    }
    if (source_len < 0) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("negative source length"));
    }

    SB_APPEND(buffer, source, source_len);
    return ncm_error_ok(ncm_error);
}

static bool
ncm_conversion_has_only_trailing_space(char *cursor) {

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

static int32
ncm_conversion_set_parse_error(NcmError *ncm_error, char *source,
                               int32 source_len) {
    char message[256];
    int32 len;

    len = SNPRINTF(message,
                   "conversion failed for '%.*s'", source_len, source);
    return ncm_error_set_status(ncm_error, -NCM_ERROR_PARSE, message, len);
}

static int32
ncm_conversion_set_i64_bounds_error(NcmError *ncm_error, int64 value,
                                    int64 lbound, int64 ubound) {
    char message[256];
    int32 len;

    len = SNPRINTF(message,
                   "value is out of bounds ([%lld, %lld] expected, %lld given)",
                   lbound, ubound, value);

    return ncm_error_set_status(ncm_error, -ERANGE, message, len);
}

static int32
ncm_conversion_set_f64_bounds_error(NcmError *ncm_error, double value,
                                    double lbound, double ubound) {
    char message[256];
    int32 len;

    len = SNPRINTF(message,
                   "value is out of bounds ([%g, %g] expected, %g given)",
                   lbound, ubound, value);

    return ncm_error_set_status(ncm_error, -ERANGE, message, len);
}

static int32
ncm_conversion_set_f64_lower_error(NcmError *ncm_error,
                                   double value, double lbound) {
    char message[256];
    int32 len;

    len = SNPRINTF(message,
                   "value is out of bounds ([%g, ->) expected, %g given)",
                   lbound, value);

    return ncm_error_set_status(ncm_error, -ERANGE, message, len);
}

int32
ncm_parse_int64(char *source, int32 source_len, int32 *out,
                NcmError *ncm_error) {
    StrBuilder buffer = {0};
    char *end;
    int64 value;
    bool ok;
    int32 status;

    if (out == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing conversion output"));
    }

    status = ncm_conversion_copy_source(
        &buffer, source, source_len, ncm_error);
    if (status < 0) {
        sb_free(&buffer);
        return status;
    }

    if (buffer.len <= 0) {
        status = ncm_conversion_set_parse_error(
            ncm_error, source, source_len);
        sb_free(&buffer);
        return status;
    }

    errno = 0;
    value = strtoll(buffer.data, &end, 10);
    ok = (end != buffer.data)
         && !ncm_conversion_is_negative_source(buffer.data)
         && ncm_conversion_has_only_trailing_space(end)
         && (errno != ERANGE)
         && (value <= MAXOF(*out));
    if (ok) {
        *out = (int32)value;
        status = ncm_error_ok(ncm_error);
    } else {
        status = ncm_conversion_set_parse_error(
            ncm_error, source, source_len);
    }

    sb_free(&buffer);
    return status;
}

int32
ncm_parse_int32(char *source, int32 source_len, int32 *out,
                NcmError *ncm_error) {
    int32 value;
    int32 status;

    if (out == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing conversion output"));
    }

    status = ncm_parse_int64(source, source_len, &value, ncm_error);
    if (status < 0) {
        return status;
    }
    if (value > MAXOF(*out)) {
        fatal(EXIT_FAILURE);
    }

    *out = value;
    return ncm_error_ok(ncm_error);
}

int32
ncm_parse_double(char *source, int32 source_len, double *out,
                 NcmError *ncm_error) {
    StrBuilder buffer = {0};
    char *end;
    double value;
    bool ok;
    int32 status;

    if (out == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing conversion output"));
    }

    status = ncm_conversion_copy_source(
        &buffer, source, source_len, ncm_error);
    if (status < 0) {
        sb_free(&buffer);
        return status;
    }

    if (buffer.len <= 0) {
        status = ncm_conversion_set_parse_error(
            ncm_error, source, source_len);
        sb_free(&buffer);
        return status;
    }

    errno = 0;
    value = strtod(buffer.data, &end);
    ok = (end != buffer.data)
         && ncm_conversion_has_only_trailing_space(end)
         && (errno != ERANGE);
    if (ok) {
        *out = value;
        status = ncm_error_ok(ncm_error);
    } else {
        status = ncm_conversion_set_parse_error(
            ncm_error, source, source_len);
    }

    sb_free(&buffer);
    return status;
}

int32
ncm_bounds_check_i64(int64 value, int64 lbound, int64 ubound,
                     NcmError *ncm_error) {
    if ((value < lbound) || (value > ubound)) {
        return ncm_conversion_set_i64_bounds_error(ncm_error, value,
                                                   lbound, ubound);
    }

    return ncm_error_ok(ncm_error);
}

int32
ncm_bounds_check_f64(double value, double lbound, double ubound,
                     NcmError *ncm_error) {
    if ((value < lbound) || (value > ubound)) {
        return ncm_conversion_set_f64_bounds_error(ncm_error, value,
                                                   lbound, ubound);
    }

    return ncm_error_ok(ncm_error);
}

int32
ncm_lower_bound_check_f64(double value, double lbound, NcmError *ncm_error) {
    if (value < lbound) {
        return ncm_conversion_set_f64_lower_error(ncm_error, value, lbound);
    }

    return ncm_error_ok(ncm_error);
}

#endif /* NCM_CONVERSION_C */
