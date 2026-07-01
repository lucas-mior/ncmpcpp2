#include "c/ncm_utf8.h"

#include "cbase/cbase.h"

int32
ncm_utf8_decode(char *string, int32 string_len, uint32 *rune) {
    int32 result;

    result = cbase_utf8_decode_rune(string, rune, string_len);
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
    int32 result;

    result = cbase_utf8_encode_rune(rune, buffer, buffer_capacity);
    return result;
}

int32
ncm_utf8_char_width(uint32 rune) {
    int32 result;

    result = cbase_utf8_char_width(rune);
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
