#if !defined(NCM_CONVERSION_C)
#define NCM_CONVERSION_C

#include "cbase.h"
#include "ncmpcpp2.h"

#include "c/ncm_c.h"

static int32
ncm_conversion_set_parse_error(NcmError *ncm_error,
                               char *source, int32 source_len) {
    char message[256];
    int32 len;

    len = SNPRINTF(message, "conversion failed for '%.*s'", source_len, source);
    ncm_error_set_status(ncm_error, -NCM_ERROR_PARSE, message, len);
    return -NCM_ERROR_PARSE;
}

int32
ncm_parse_double(char *source, int32 source_len, double *out,
                 NcmError *ncm_error) {
    String buffer = {0};
    char *end;
    double value;
    bool ok;
    int32 status;

    if (out == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    SL("missing conversion output"));
    }

    if (source == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    SL("missing conversion source"));
    }
    if (source_len < 0) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    SL("negative source length"));
    }

    str_clear(&buffer);
    STR_APPEND(&buffer, source, source_len);

    if (buffer.len <= 0) {
        status = ncm_conversion_set_parse_error(ncm_error, source, source_len);
        str_free(&buffer);
        return status;
    }

    errno = 0;
    value = strtod(buffer.data, &end);
    ok = end != buffer.data;
    while (ok && (*end != '\0')) {
        if (!is_space(*end)) {
            ok = false;
        }
        end += 1;
    }
    ok = ok && (errno != ERANGE);
    if (ok) {
        *out = value;
        status = ncm_error_ok(ncm_error);
    } else {
        status = ncm_conversion_set_parse_error(ncm_error, source, source_len);
    }

    str_free(&buffer);
    return status;
}

int32
ncm_bounds_check_i64(int64 value, int64 lbound, int64 ubound,
                     NcmError *ncm_error) {
    if ((value < lbound) || (value > ubound)) {
        char message[256];
        int32 len;

        len = SNPRINTF(message,
                       "value is out of bounds ([%lld, %lld] expected, "
                       "%lld given)",
                       lbound, ubound, value);
        return ncm_error_set_status(ncm_error, -ERANGE, message, len);
    }

    return ncm_error_ok(ncm_error);
}

int32
ncm_bounds_check_f64(double value, double lbound, double ubound,
                     NcmError *ncm_error) {
    if ((value < lbound) || (value > ubound)) {
        char message[256];
        int32 len;

        len = SNPRINTF(message,
                       "value is out of bounds ([%g, %g] expected, %g given)",
                       lbound, ubound, value);
        return ncm_error_set_status(ncm_error, -ERANGE, message, len);
    }

    return ncm_error_ok(ncm_error);
}

int32
ncm_lower_bound_check_f64(double value, double lbound, NcmError *ncm_error) {
    if (value < lbound) {
        char message[256];
        int32 len;

        len = SNPRINTF(message,
                       "value is out of bounds ([%g, ->) expected, %g given)",
                       lbound, value);
        return ncm_error_set_status(ncm_error, -ERANGE, message, len);
    }

    return ncm_error_ok(ncm_error);
}

#endif /* NCM_CONVERSION_C */
