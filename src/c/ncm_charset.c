#if !defined(NCM_CHARSET_C)
#define NCM_CHARSET_C

#include "cbase.h"

#include "c/ncm_c.h"

static StrBuilder
ncm_charset_copy(char *string, int32 string_len) {
    StrBuilder result = {0};

    SB_APPEND(&result, string, string_len);
    return result;
}

StrBuilder
ncm_charset_utf8_to_locale(char *string, int32 string_len) {
    return ncm_charset_copy(string, string_len);
}

#endif /* NCM_CHARSET_C */
