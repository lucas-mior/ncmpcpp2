#include "c/ncm_wide_string.h"

#include <stdint.h>

#include "c/ncm_base.h"
#include "cbase/utf8.c"
#include "cbase/base_macros.h"
#include "cbase/util.c"

#define NCM_WIDE_REPLACEMENT_CHARACTER 0xfffdu

void
ncm_wide_string_init(NcmWideString *string) {
    string->data = NULL;
    string->len = 0;
    string->cap = 0;
    return;
}

void
ncm_wide_string_destroy(NcmWideString *string) {
    if (string == NULL) {
        return;
    }
    if (string->data) {
        free2(string->data, string->cap*SIZEOF(*string->data));
    }

    string->data = NULL;
    string->len = 0;
    string->cap = 0;
    return;
}

void
ncm_wide_string_clear(NcmWideString *string) {
    if (string == NULL) {
        return;
    }

    string->len = 0;
    if (string->data) {
        string->data[0] = L'\0';
    }
    return;
}

void
ncm_wide_string_reserve(NcmWideString *string, int32 extra) {
    int32 needed;
    int32 old_cap;
    int32 new_cap;

    if ((string == NULL) || (extra <= 0)) {
        return;
    }

    needed = string->len + extra + 1;
    if (needed <= string->cap) {
        return;
    }

    old_cap = string->cap;
    new_cap = string->cap;
    if (new_cap <= 0) {
        new_cap = 16;
    }
    while (new_cap < needed) {
        new_cap *= 2;
    }

    string->data = realloc2(string->data, old_cap, new_cap,
                          SIZEOF(*string->data));
    string->cap = new_cap;
    return;
}

void
ncm_wide_string_append_char(NcmWideString *string, wchar_t ch) {
    if (string == NULL) {
        return;
    }

    ncm_wide_string_reserve(string, 1);
    string->data[string->len] = ch;
    string->len += 1;
    string->data[string->len] = L'\0';
    return;
}

bool
ncm_wide_string_from_utf8(NcmWideString *out,
                          char *string, int32 string_len) {
    int32 byte;

    if (out == NULL) {
        return false;
    }

    ncm_wide_string_clear(out);
    if ((string == NULL) || (string_len <= 0)) {
        return true;
    }

    byte = 0;
    while (byte < string_len) {
        uint32 rune;
        int32 length;
        wchar_t ch;

        length = utf8_decode(string + byte, string_len - byte, &rune);
        if ((uint64)rune > (uint64)WCHAR_MAX) {
            rune = NCM_WIDE_REPLACEMENT_CHARACTER;
        }

        ch = (wchar_t)rune;
        ncm_wide_string_append_char(out, ch);
        byte += length;
    }

    return true;
}
