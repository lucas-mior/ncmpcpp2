#include "c/ncm_utf8.h"

#include <wchar.h>
#include <wctype.h>

#include "cbase/util.c"
#include "cbase/base_macros.h"

int32
ncm_utf8_decode(char *string, int32 string_len, uint32 *rune) {
    int32 result;

    result = utf8_decode(string, rune, string_len);
    if (result <= 0) {
        result = 1;
        if (rune) {
            *rune = '.';
        }
    }

    return result;
}

int32
ncm_utf8_encode(uint32 rune, char *buffer, int32 buffer_capacity) {
    char encoded[4];
    int32 result;

    result = utf8_encode(rune, encoded);
    if (result > buffer_capacity) {
        return 0;
    }
    if (result > 0) {
        memcpy64(buffer, encoded, result);
    }

    return result;
}

int32
ncm_utf8_char_width(uint32 rune) {
    int width;
    int32 result;

    width = wcwidth((wchar_t)rune);
    if (width < 0) {
        result = 1;
    } else {
        result = (int32)width;
    }

    return result;
}

int32
ncm_utf8_characters(char *string, int32 string_len) {
    int32 result;
    int32 byte;

    result = 0;
    byte = 0;
    while (byte < string_len) {
        byte = ncm_utf8_next_position(string, string_len, byte);
        result += 1;
    }

    return result;
}

int32
ncm_utf8_byte_position(char *string, int32 string_len,
                       int32 character) {
    int32 byte;

    if (character <= 0) {
        return 0;
    }

    byte = 0;
    for (int32 i = 0; (i < character) && (byte < string_len); i += 1) {
        byte = ncm_utf8_next_position(string, string_len, byte);
    }

    return byte;
}

int32
ncm_utf8_next_position(char *string, int32 string_len, int32 byte) {
    int32 length;
    int32 result;
    uint32 rune;

    if (byte >= string_len) {
        return string_len;
    }

    length = ncm_utf8_decode(string + byte, string_len - byte, &rune);
    result = byte + length;
    if (result > string_len) {
        result = string_len;
    }

    return result;
}

int32
ncm_utf8_suffix_width_position(char *string, int32 string_len,
                               int32 max_width) {
    int32 byte;
    int32 remaining_width;

    if (max_width <= 0) {
        return string_len;
    }

    byte = 0;
    remaining_width = ncm_utf8_width(string, string_len);
    while (byte < string_len) {
        int32 length;
        uint32 rune;

        if (remaining_width <= max_width) {
            return byte;
        }

        length = ncm_utf8_decode(string + byte, string_len - byte, &rune);
        remaining_width -= ncm_utf8_char_width(rune);
        byte += length;
    }

    return string_len;
}

int32
ncm_utf8_width(char *string, int32 string_len) {
    int32 result;
    int32 byte;

    result = 0;
    byte = 0;
    while (byte < string_len) {
        int32 length;
        uint32 rune;

        length = ncm_utf8_decode(string + byte, string_len - byte, &rune);
        result += ncm_utf8_char_width(rune);
        byte += length;
    }

    return result;
}

int32
ncm_utf8_cut_width(char *string, int32 string_len, int32 max_width) {
    int32 byte;
    int32 result_width;

    byte = 0;
    result_width = 0;
    while (byte < string_len) {
        int32 length;
        int32 next_width;
        uint32 rune;

        length = ncm_utf8_decode(string + byte, string_len - byte, &rune);
        next_width = result_width + ncm_utf8_char_width(rune);
        if (next_width > max_width) {
            return byte;
        }

        result_width = next_width;
        byte += length;
    }

    return string_len;
}

int32
ncm_utf8_capitalize_first_letters(char *string, int32 string_len,
                                  char *buffer, int32 buffer_capacity) {
    int32 byte;
    int32 result_len;
    wchar_t previous;

    byte = 0;
    previous = 0;
    result_len = 0;
    while (byte < string_len) {
        char encoded[4];
        int32 encoded_len;
        int32 length;
        uint32 rune;
        wchar_t wc;

        length = ncm_utf8_decode(string + byte, string_len - byte, &rune);
        wc = (wchar_t)rune;
        if (!iswalpha(previous) && (previous != L'\'')) {
            wc = towupper(wc);
        }

        encoded_len = ncm_utf8_encode((uint32)wc, encoded,
                                      (int32)SIZEOF(encoded));
        if (encoded_len <= 0) {
            encoded_len = length;
            if ((buffer != NULL)
                && ((result_len + encoded_len) <= buffer_capacity)) {
                memcpy64(buffer + result_len, string + byte,
                     encoded_len);
            }
        } else if ((buffer != NULL)
                   && ((result_len + encoded_len) <= buffer_capacity)) {
            memcpy64(buffer + result_len, encoded, encoded_len);
        }

        result_len += encoded_len;
        previous = wc;
        byte += length;
    }

    return result_len;
}
