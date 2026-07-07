#if !defined(NCM_STRING_FORMAT_H)
#define NCM_STRING_FORMAT_H

#include "c/ncm_base.h"
#include "c/ncm_string.h"

NCM_EXTERN_C_BEGIN

enum NcmStringFormatArgType {
    NCM_STRING_FORMAT_ARG_STRING,
    NCM_STRING_FORMAT_ARG_I64,
    NCM_STRING_FORMAT_ARG_U64,
    NCM_STRING_FORMAT_ARG_F64,
    NCM_STRING_FORMAT_ARG_CHAR,
    NCM_STRING_FORMAT_ARG_BOOL,
};

typedef struct NcmStringFormatArg {
    enum NcmStringFormatArgType type;
    union {
        NcmStringView string;
        int64 i64;
        uint64 u64;
        double f64;
        char ch;
        bool boolean;
    } value;
} NcmStringFormatArg;

NcmStringFormatArg ncm_string_format_arg_string(char *data, int32 len);
NcmStringFormatArg ncm_string_format_arg_cstring(char *data);
NcmStringFormatArg ncm_string_format_arg_i64(int64 value);
NcmStringFormatArg ncm_string_format_arg_u64(uint64 value);
NcmStringFormatArg ncm_string_format_arg_f64(double value);
NcmStringFormatArg ncm_string_format_arg_char(char value);
NcmStringFormatArg ncm_string_format_arg_bool(bool value);

void ncm_string_format_apply(NcmBuffer *out, char *format,
                             int32 format_len,
                             NcmStringFormatArg *args,
                             int32 args_len);
NcmBuffer ncm_string_format_make(char *format, int32 format_len,
                                 NcmStringFormatArg *args,
                                 int32 args_len);

NCM_EXTERN_C_END

#endif /* NCM_STRING_FORMAT_H */
