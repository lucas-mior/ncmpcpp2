#include "c/ncm_charset.h"

static NcmBuffer ncm_charset_copy(char *string, int32 string_len);

static NcmBuffer
ncm_charset_copy(char *string, int32 string_len) {
    NcmBuffer result;

    ncm_buffer_init(&result);
    if (string_len > 0) {
        ncm_buffer_append(&result, string, string_len);
    }
    return result;
}

NcmBuffer
ncm_charset_to_utf8_from(char *string, int32 string_len,
                         char *charset, int32 charset_len) {
    (void)charset;
    (void)charset_len;
    return ncm_charset_copy(string, string_len);
}

NcmBuffer
ncm_charset_from_utf8_to(char *string, int32 string_len,
                         char *charset, int32 charset_len) {
    (void)charset;
    (void)charset_len;
    return ncm_charset_copy(string, string_len);
}

NcmBuffer
ncm_charset_utf8_to_locale(char *string, int32 string_len) {
    return ncm_charset_copy(string, string_len);
}

NcmBuffer
ncm_charset_locale_to_utf8(char *string, int32 string_len) {
    return ncm_charset_copy(string, string_len);
}
