#if !defined(NCM_STRING_C)
#define NCM_STRING_C

#include "c/ncm_string.h"

#include "cbase/base_macros.h"
#include "c/ncm_base.h"
#include "c/ncm_path.h"
#include "cbase/util.c"

void
ncm_string_view_init(NcmStringView *view) {
    if (view == NULL) {
        return;
    }

    view->data = NULL;
    view->len = 0;
    return;
}

NcmStringView
ncm_string_view_make(char *data, int32 len) {
    NcmStringView result;

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
ncm_string_view_set(NcmStringView *view, char *data, int32 len) {
    if (view == NULL) {
        return;
    }

    *view = ncm_string_view_make(data, len);
    return;
}

void
ncm_string_view_clear(NcmStringView *view) {
    ncm_string_view_init(view);
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

bool
ncm_string_starts_with(char *string, int32 string_len,
                       char *prefix, int32 prefix_len) {
    if (prefix_len > string_len) {
        return false;
    }
    if (prefix_len < 0) {
        return false;
    }

    for (int32 i = 0; i < prefix_len; i += 1) {
        if (string[i] != prefix[i]) {
            return false;
        }
    }

    return true;
}

bool
ncm_string_ends_with(char *string, int32 string_len,
                     char *suffix, int32 suffix_len) {
    if (suffix_len > string_len) {
        return false;
    }
    if (suffix_len < 0) {
        return false;
    }

    return STREQUAL(string + string_len - suffix_len, suffix_len,
                            suffix, suffix_len);
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
    StrBuilder result;
    int32 min_len;
    int32 common;
    int32 slash;

    sb_init(&result);
    if ((left == NULL) || (right == NULL)) {
        sb_append(&result, STRLIT_ARGS("/"));
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
        sb_append(&result, STRLIT_ARGS("/"));
    } else if (slash > 0) {
        sb_append(&result, left, slash);
    }

    return result;
}

StrBuilder
ncm_string_get_enclosed(char *string, int32 string_len,
                        char open, char close,
                        int32 start, int32 *pos) {
    StrBuilder result;
    int32 i;

    sb_init(&result);
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
        if ((string[i] == '\\')
            && (i + 1 < string_len)
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
ncm_string_remove_invalid_filename_chars(char *filename,
                                         int32 *filename_len,
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
ncm_string_append_shell_escaped_single_quotes(StrBuilder *buffer,
                                              char *string,
                                              int32 string_len) {
    for (int32 i = 0; i < string_len; i += 1) {
        if (string[i] == '\'') {
            sb_append(buffer, STRLIT_ARGS("'\\''"));
        } else {
            sb_append_byte(buffer, string[i]);
        }
    }
    return;
}

int32
ncm_string_basename_start(char *path, int32 path_len) {
    return ncm_path_basename_start(path, path_len);
}

int32
ncm_string_parent_directory_len(char *path, int32 path_len) {
    return ncm_path_parent_directory_len(path, path_len);
}

#endif /* NCM_STRING_C */
