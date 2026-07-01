#include "cbase_config.h"

#include <wchar.h>

#include "cbase.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#include "util.c"
#include "sort.c"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

void
cbase_string_lowercase_ascii(char *string, int32 string_len) {
    for (int32 i = 0; i < string_len; i += 1) {
        if ((string[i] >= 'A') && (string[i] <= 'Z')) {
            string[i] += 'a' - 'A';
        }
    }
    return;
}

int32
cbase_string_last_index_of(char *string, int32 string_len, char needle) {
    char *found;
    int32 result;

    if (string_len <= 0) {
        return -1;
    }

    found = memrchr64(string, needle, string_len);
    if (found == NULL) {
        result = -1;
    } else {
        result = (int32)(found - string);
    }

    return result;
}

int32
cbase_utf8_decode_rune(char *string, uint32 *rune, int32 string_len) {
    int32 result;

    result = utf8_decode(string, rune, string_len);
    return result;
}

int32
cbase_utf8_encode_rune(uint32 rune, char *buffer, int32 buffer_capacity) {
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
cbase_utf8_char_width(uint32 rune) {
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

void
cbase_application_mode_link_anchor(void) {
    generic_functions_sink();
    assert_functions_sink();
    arena_functions_sink();
    hash_functions_sink_alloc_map();
    memory_functions_sink();
    minmax_functions_sink();
    utf8_functions_sink();
    util_functions_sink();
    sort_functions_sink();
    return;
}
