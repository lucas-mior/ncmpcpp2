#if !defined(NCM_COMPARATORS_C)
#define NCM_COMPARATORS_C

#include "cbase.h"

#include "c/ncm_c.h"

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
        uint8 c = (uint8)string[i];

        if (!isdigit(c)) {
            return false;
        }
    }

    return true;
}

static int32
ncm_compare_parse_decimal(char *string, int32 string_len) {
    int32 n;

    n = 0;
    for (int32 i = 0; i < string_len; i += 1) {
        n = 10*n + string[i] - '0';
    }

    return n;
}

static void
ncm_compare_copy_to_buffer(StrBuilder *buffer,
                           char *string, int32 string_len) {
    sb_clear(buffer);
    if ((string == NULL) || (string_len <= 0)) {
        sb_reserve(buffer, 1);
        buffer->data[0] = '\0';
        return;
    }
    SB_APPEND(buffer, string, string_len);
    return;
}

int32
ncm_compare_locale_strings(char *left, int32 left_len,
                           char *right, int32 right_len,
                           bool ignore_the) {
    StrBuilder left_buffer = {0};
    StrBuilder right_buffer = {0};
    int32 left_number;
    int32 right_number;
    int32 left_offset;
    int32 right_offset;
    int32 result;

    static char empty_string[] = "";

    if ((left == NULL) || (left_len <= 0)) {
        left = empty_string;
        left_len = 0;
    }
    if ((right == NULL) || (right_len <= 0)) {
        right = empty_string;
        right_len = 0;
    }

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

    ncm_compare_copy_to_buffer(&left_buffer, left + left_offset,
                               left_len - left_offset);
    ncm_compare_copy_to_buffer(&right_buffer, right + right_offset,
                               right_len - right_offset);

    result = strcoll(left_buffer.data, right_buffer.data);
    sb_free(&left_buffer);
    sb_free(&right_buffer);

    if (result < 0) {
        return -1;
    }
    if (result > 0) {
        return 1;
    }
    return 0;
}

#endif /* NCM_COMPARATORS_C */
