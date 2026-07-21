#if !defined(NCM_CHARSET_C)
#define NCM_CHARSET_C

#include "c/ncm_charset.h"

static StrBuilder
ncm_charset_copy(char *string, int32 string_len) {
    StrBuilder result;

    sb_init(&result);
    if (string_len > 0) {
        sb_append(&result, string, string_len);
    }
    return result;
}

StrBuilder
ncm_charset_utf8_to_locale(char *string, int32 string_len) {
    return ncm_charset_copy(string, string_len);
}

#endif /* NCM_CHARSET_C */
