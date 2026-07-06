#include "c/ncm_comparators.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "c/ncm_base.h"
#include "cbase/base_macros.h"

static bool ncm_compare_has_the_word(char *string, int32 string_len);
static bool ncm_compare_is_decimal_number(char *string, int32 string_len);
static int64 ncm_compare_parse_decimal(char *string, int32 string_len);
static int32 ncm_compare_copy_to_buffer(NcmBuffer *buffer,
                                        char *string, int32 string_len);

static bool
ncm_compare_has_the_word(char *string, int32 string_len) {
    if (string_len < 4) {
        return false;
    }
    if ((string[0] != 't') && (string[0] != 'T')) {
        return false;
    }
    if ((string[1] != 'h') && (string[1] != 'H')) {
        return false;
    }
    if ((string[2] != 'e') && (string[2] != 'E')) {
        return false;
    }
    if (string[3] != ' ') {
        return false;
    }

    return true;
}

static bool
ncm_compare_is_decimal_number(char *string, int32 string_len) {
    if (string_len <= 0) {
        return false;
    }

    for (int32 i = 0; i < string_len; i += 1) {
        unsigned char c;

        c = (unsigned char)string[i];
        if (!isdigit(c)) {
            return false;
        }
    }

    return true;
}

static int64
ncm_compare_parse_decimal(char *string, int32 string_len) {
    int64 n;

    n = 0;
    for (int32 i = 0; i < string_len; i += 1) {
        n = 10*n + string[i] - '0';
    }

    return n;
}

static int32
ncm_compare_copy_to_buffer(NcmBuffer *buffer,
                           char *string, int32 string_len) {
    ncm_buffer_clear(buffer);
    if (string_len > 0) {
        ncm_buffer_append(buffer, string, string_len);
    }
    ncm_buffer_append_byte(buffer, '\0');
    return buffer->len - 1;
}

int32
ncm_compare_locale_strings(char *left, int32 left_len,
                           char *right, int32 right_len,
                           bool ignore_the) {
    NcmBuffer left_buffer;
    NcmBuffer right_buffer;
    int64 left_number;
    int64 right_number;
    int32 left_offset;
    int32 right_offset;
    int32 result;

    if (ncm_compare_is_decimal_number(left, left_len)
        && ncm_compare_is_decimal_number(right, right_len)) {
        left_number = ncm_compare_parse_decimal(left, left_len);
        right_number = ncm_compare_parse_decimal(right, right_len);
        if (left_number < right_number) {
            return -1;
        }
        if (left_number > right_number) {
            return 1;
        }
        return 0;
    }

    left_offset = 0;
    right_offset = 0;
    if (ignore_the) {
        if (ncm_compare_has_the_word(left, left_len)) {
            left_offset = 4;
        }
        if (ncm_compare_has_the_word(right, right_len)) {
            right_offset = 4;
        }
    }

    ncm_buffer_init(&left_buffer);
    ncm_buffer_init(&right_buffer);
    left_len = ncm_compare_copy_to_buffer(&left_buffer,
                                          left + left_offset,
                                          left_len - left_offset);
    right_len = ncm_compare_copy_to_buffer(&right_buffer,
                                           right + right_offset,
                                           right_len - right_offset);

    (void)left_len;
    (void)right_len;
    result = strcoll(left_buffer.data, right_buffer.data);
    ncm_buffer_destroy(&left_buffer);
    ncm_buffer_destroy(&right_buffer);

    if (result < 0) {
        return -1;
    }
    if (result > 0) {
        return 1;
    }
    return 0;
}
