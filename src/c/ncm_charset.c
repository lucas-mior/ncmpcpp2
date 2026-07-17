#include "c/ncm_charset.h"

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
ncm_charset_utf8_to_locale(char *string, int32 string_len) {
    return ncm_charset_copy(string, string_len);
}
