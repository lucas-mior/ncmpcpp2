#if !defined(NCM_STRING_C)
#define NCM_STRING_C

#include "cbase.h"

#include "c/ncm_c.h"

void
stupid_string_free(char **data, int32 *len) {
    free2(*data, *len + 1);
    *data = NULL;
    *len = 0;
    return;
}

void
stupid_string_set(char **dst, int32 *dst_len, char *src, int32 src_len) {
    char *copy;

    ASSERT(dst != NULL);
    ASSERT(dst_len != NULL);
    ASSERT_NON_NEGATIVE(src_len);
    ASSERT((src != NULL) || (src_len == 0));

    free2(*dst, *dst_len + 1);
    *dst = NULL;
    *dst_len = 0;

    if (src_len == 0) {
        return;
    }
    copy = malloc2(src_len + 1);
    memcpy64(copy, src, src_len);
    copy[src_len] = '\0';
    *dst = copy;
    *dst_len = src_len;
    return;
}

StringView
ncm_string_view(char *data, int32 len) {
    StringView result;

    result.data = data;
    result.len = len;
    if (data == NULL) {
        result.len = 0;
    }
    if (result.len < 0) {
        result.len = 0;
    }
    return result;
}

void
ncm_string_view_set(StringView *view, char *data, int32 len) {
    if (view == NULL) {
        return;
    }

    *view = ncm_string_view(data, len);
    return;
}

void
ncm_string_view_clear(StringView *view) {
    if (view) {
        *view = (StringView){0};
    }
    return;
}

void
ncm_string_lowercase_ascii(char *string, int32 string_len) {
    for (int32 i = 0; i < string_len; i += 1) {
        if ((string[i] >= 'A') && (string[i] <= 'Z')) {
            string[i] += 'a' - 'A';
        }
    }
    return;
}

int32
ncm_string_find_char(char *string, int32 string_len, char needle) {
    if (string_len <= 0) {
        return -1;
    }

    for (int32 i = 0; i < string_len; i += 1) {
        if (string[i] == needle) {
            return i;
        }
    }

    return -1;
}

bool
ncm_string_contains_char(char *string, int32 string_len, char needle) {
    return ncm_string_find_char(string, string_len, needle) >= 0;
}

StrBuilder
ncm_string_shared_directory(char *left, int32 left_len,
                            char *right, int32 right_len) {
    StrBuilder result = {0};
    int32 min_len;
    int32 common;
    int32 slash;

    if ((left == NULL) || (right == NULL)) {
        SB_APPEND(&result, "/");
        return result;
    }
    if (left_len < 0) {
        left_len = 0;
    }
    if (right_len < 0) {
        right_len = 0;
    }

    min_len = left_len;
    if (right_len < min_len) {
        min_len = right_len;
    }

    common = 0;
    while ((common < min_len) && (left[common] == right[common])) {
        common += 1;
    }

    slash = -1;
    for (int32 i = 0; (i <= common) && (i < left_len); i += 1) {
        if (left[i] == '/') {
            slash = i;
        }
    }

    if (slash < 0) {
        SB_APPEND(&result, "/");
    } else if (slash > 0) {
        SB_APPEND(&result, left, slash);
    }

    return result;
}

StrBuilder
ncm_string_get_enclosed(char *string, int32 string_len, char open, char close,
                        int32 start, int32 *pos) {
    StrBuilder result = {0};
    int32 i;

    if (pos) {
        *pos = -1;
    }
    if (string == NULL) {
        return result;
    }
    if (string_len < 0) {
        string_len = 0;
    }
    if (start < 0) {
        start = 0;
    }
    if (start > string_len) {
        start = string_len;
    }

    i = start;
    while ((i < string_len) && (string[i] != open)) {
        i += 1;
    }
    if (i >= string_len) {
        return result;
    }

    i += 1;
    while ((i < string_len) && (string[i] != close)) {
        if ((string[i] == '\\') && (i + 1 < string_len)
            && ((string[i + 1] == '\\') || (string[i + 1] == close))) {
            i += 1;
        }
        sb_append_byte(&result, string[i]);
        i += 1;
    }

    if (i < string_len) {
        i += 1;
    } else {
        sb_clear(&result);
    }
    if (pos) {
        *pos = i;
    }

    return result;
}

void
ncm_string_remove_chars(char *string, int32 *string_len,
                        char *chars, int32 chars_len) {
    int32 len;
    int32 out;

    if ((string == NULL) || (string_len == NULL)) {
        return;
    }
    if (chars == NULL) {
        return;
    }

    len = *string_len;
    if (len < 0) {
        *string_len = 0;
        string[0] = '\0';
        return;
    }
    out = 0;
    for (int32 i = 0; i < len; i += 1) {
        if (!ncm_string_contains_char(chars, chars_len, string[i])) {
            string[out++] = string[i];
        }
    }

    *string_len = out;
    if (out >= 0) {
        string[out] = '\0';
    }
    return;
}

void
ncm_string_remove_invalid_filename_chars(char *filename, int32 *filename_len,
                                         bool win32_compatible) {
    char win32_unallowed_chars[] = "\"*/:<>?\\|";
    char unix_unallowed_chars[] = "/";
    char *unallowed_chars;
    int32 unallowed_chars_len;

    if (win32_compatible) {
        unallowed_chars = win32_unallowed_chars;
        unallowed_chars_len = LENGTH(win32_unallowed_chars) - 1;
    } else {
        unallowed_chars = unix_unallowed_chars;
        unallowed_chars_len = LENGTH(unix_unallowed_chars) - 1;
    }

    ncm_string_remove_chars(filename, filename_len,
                            unallowed_chars, unallowed_chars_len);
    return;
}

void
ncm_string_append_shell_escaped_single_quotes(StrBuilder *buffer, char *string,
                                              int32 string_len) {
    for (int32 i = 0; i < string_len; i += 1) {
        if (string[i] == '\'') {
            SB_APPEND(buffer, "'\\''");
        } else {
            sb_append_byte(buffer, string[i]);
        }
    }
    return;
}

int32
ncm_string_parent_directory_len(char *path, int32 path_len) {
    return ncm_path_parent_directory_len(path, path_len);
}

#endif /* NCM_STRING_C */
