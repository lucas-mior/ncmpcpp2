#if !defined(NCM_STRING_FORMAT_C)
#define NCM_STRING_FORMAT_C

#include "c/ncm_string_format.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>

#include "cbase/base_macros.h"
#include "cbase/util.c"

static void
ncm_string_format_append_number(NcmBuffer *out, char *format,
                                NcmStringFormatArg *arg) {
    char buffer[128];
    int32 len;

    switch (arg->type) {
    case NCM_STRING_FORMAT_ARG_I64:
        len = SNPRINTF(buffer, format, arg->value.i64);
        break;
    case NCM_STRING_FORMAT_ARG_U64:
        len = SNPRINTF(buffer, format, arg->value.u64);
        break;
    case NCM_STRING_FORMAT_ARG_F64:
        len = SNPRINTF(buffer, format, arg->value.f64);
        break;
    case NCM_STRING_FORMAT_ARG_STRING:
    case NCM_STRING_FORMAT_ARG_CHAR:
    case NCM_STRING_FORMAT_ARG_BOOL:
    case NCM_STRING_FORMAT_ARG_LAST:
    default:
        return;
    }

    if (len < 0) {
        return;
    }
    if (len >= SIZEOF(buffer)) {
        len = SIZEOF(buffer) - 1;
    }
    ncm_buffer_append(out, buffer, len);
    return;
}

static void
ncm_string_format_append_arg(NcmBuffer *out, NcmStringFormatArg *arg) {
    char ch;

    if (arg == NULL) {
        return;
    }

    switch (arg->type) {
    case NCM_STRING_FORMAT_ARG_STRING:
        ncm_buffer_append(out, arg->value.string.data,
                          arg->value.string.len);
        break;
    case NCM_STRING_FORMAT_ARG_I64:
        ncm_string_format_append_number(out, "%" PRId64, arg);
        break;
    case NCM_STRING_FORMAT_ARG_U64:
        ncm_string_format_append_number(out, "%" PRIu64, arg);
        break;
    case NCM_STRING_FORMAT_ARG_F64:
        ncm_string_format_append_number(out, "%g", arg);
        break;
    case NCM_STRING_FORMAT_ARG_CHAR:
        ch = arg->value.ch;
        ncm_buffer_append(out, &ch, 1);
        break;
    case NCM_STRING_FORMAT_ARG_BOOL:
        if (arg->value.boolean) {
            ncm_buffer_append(out, STRLIT_ARGS("1"));
        } else {
            ncm_buffer_append(out, STRLIT_ARGS("0"));
        }
        break;
    case NCM_STRING_FORMAT_ARG_LAST:
    default:
        break;
    }
    return;
}

NcmStringFormatArg
ncm_string_format_arg_string(char *data, int32 len) {
    NcmStringFormatArg result;

    result.type = NCM_STRING_FORMAT_ARG_STRING;
    result.value.string.data = data;
    result.value.string.len = len;
    if (data == NULL) {
        result.value.string.len = 0;
    }
    return result;
}

NcmStringFormatArg
ncm_string_format_arg_cstring(char *data) {
    return ncm_string_format_arg_string(
        data, optional_strlen32(data));
}

NcmStringFormatArg
ncm_string_format_arg_i64(int32 value) {
    NcmStringFormatArg result;

    result.type = NCM_STRING_FORMAT_ARG_I64;
    result.value.i64 = value;
    return result;
}

NcmStringFormatArg
ncm_string_format_arg_u64(uint64 value) {
    NcmStringFormatArg result;

    result.type = NCM_STRING_FORMAT_ARG_U64;
    result.value.u64 = value;
    return result;
}

void
ncm_string_format_apply(NcmBuffer *out, char *format, int32 format_len,
                        NcmStringFormatArg *args, int32 args_len) {
    int32 next_arg;
    int32 i;

    if ((out == NULL) || (format == NULL) || (format_len <= 0)) {
        return;
    }

    next_arg = 0;
    i = 0;
    while (i < format_len) {
        uint8 next;

        if ((format[i] != '%') || (i + 1 >= format_len)) {
            ncm_buffer_append_byte(out, format[i]);
            i += 1;
            continue;
        }

        next = (uint8)format[i + 1];
        if (next == '%') {
            ncm_buffer_append_byte(out, '%');
            i += 2;
            continue;
        }
        if (next == 's') {
            if (next_arg < args_len) {
                ncm_string_format_append_arg(out, &args[next_arg]);
            }
            next_arg += 1;
            i += 2;
            continue;
        }
        if (isdigit(next)) {
            int32 idx;
            int32 j;

            idx = 0;
            j = i + 1;
            while ((j < format_len)
                   && isdigit((uint8)format[j])) {
                idx = idx*10 + format[j] - '0';
                j += 1;
            }
            if ((idx > 0) && (j < format_len) && (format[j] == '%')) {
                if (idx <= args_len) {
                    ncm_string_format_append_arg(out, &args[idx - 1]);
                }
                i = j + 1;
                continue;
            }
        }

        ncm_buffer_append_byte(out, format[i]);
        i += 1;
    }
    return;
}

NcmBuffer
ncm_string_format_make(char *format, int32 format_len,
                       NcmStringFormatArg *args, int32 args_len) {
    NcmBuffer result;

    ncm_buffer_init(&result);
    ncm_string_format_apply(&result, format, format_len, args, args_len);
    return result;
}

#endif /* NCM_STRING_FORMAT_C */
