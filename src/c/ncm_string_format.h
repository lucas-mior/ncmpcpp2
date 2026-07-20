#if !defined(NCM_STRING_FORMAT_H)
#define NCM_STRING_FORMAT_H

#include "c/ncm_base.h"
#include "c/ncm_string.h"

#define ENUM_NAME NcmStringFormatArgType
#define ENUM_PREFIX_ NCM_STRING_FORMAT_ARG_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(NCM_STRING_FORMAT_ARG_STRING) \
    X(NCM_STRING_FORMAT_ARG_I64) \
    X(NCM_STRING_FORMAT_ARG_U64) \
    X(NCM_STRING_FORMAT_ARG_F64) \
    X(NCM_STRING_FORMAT_ARG_CHAR) \
    X(NCM_STRING_FORMAT_ARG_BOOL)
#include "cbase/xenums.c"

typedef struct NcmStringFormatArg {
    enum NcmStringFormatArgType type;
    union {
        NcmStringView string;
        int32 i64;
        uint64 u64;
        double f64;
        char ch;
        bool boolean;
    } value;
} NcmStringFormatArg;

NcmStringFormatArg ncm_string_format_arg_string(char *data, int32 len);
NcmStringFormatArg ncm_string_format_arg_cstring(char *data);
NcmStringFormatArg ncm_string_format_arg_i64(int32 value);
NcmStringFormatArg ncm_string_format_arg_u64(uint64 value);

void ncm_string_format_apply(NcmBuffer *out, char *format,
                             int32 format_len,
                             NcmStringFormatArg *args,
                             int32 args_len);
NcmBuffer ncm_string_format_make(char *format, int32 format_len,
                                 NcmStringFormatArg *args,
                                 int32 args_len);

#endif /* NCM_STRING_FORMAT_H */
