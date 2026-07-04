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

bool
cbase_strequal(const char *s1, const char *s2) {
    bool result;

    result = strequal((char *)s1, (char *)s2);
    return result;
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

void *
cbase_malloc(int64 size) {
    return malloc2(size);
}

void *
cbase_realloc_array(void *old, int64 old_capacity,
                    int64 new_capacity, int64 obj_size) {
    void *result;

    if (new_capacity <= 0) {
        cbase_free(old, old_capacity*obj_size);
        return NULL;
    }

    result = realloc2(old, old_capacity, new_capacity, obj_size);
    return result;
}

void
cbase_free(void *pointer, int64 size) {
    free2(pointer, size);
    return;
}

void
cbase_memcpy(void *dest, void *source, int64 n) {
    memcpy64(dest, source, n);
    return;
}

void
cbase_memmove(void *dest, void *source, int64 n) {
    memmove64(dest, source, n);
    return;
}
