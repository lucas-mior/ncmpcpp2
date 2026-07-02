#include "c/ncm_string.h"

#include "cbase/cbase.h"

void
ncm_string_lowercase_ascii(char *string, int32 string_len) {
    cbase_string_lowercase_ascii(string, string_len);
    return;
}

bool
ncm_string_equal(char *left, int32 left_len,
                 char *right, int32 right_len) {
    if (left_len != right_len) {
        return false;
    }

    return ncm_string_starts_with(left, left_len, right, right_len);
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

int32
ncm_string_basename_start(char *path, int32 path_len) {
    int32 slash;
    int32 result;

    if (path_len <= 0) {
        return 0;
    }

    slash = cbase_string_last_index_of(path, path_len, '/');
    if (slash < 0) {
        result = 0;
    } else {
        result = slash + 1;
    }

    return result;
}

int32
ncm_string_parent_directory_len(char *path, int32 path_len) {
    int32 slash;
    int32 result;

    if (path_len <= 0) {
        return 0;
    }

    slash = cbase_string_last_index_of(path, path_len, '/');
    if (slash < 0) {
        result = 0;
    } else {
        result = slash;
    }

    return result;
}
