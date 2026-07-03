#include "c/ncm_path.h"

#include "cbase/cbase.h"
#include "c/ncm_string.h"

int32
ncm_path_basename_start(char *path, int32 path_len) {
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
ncm_path_parent_directory_len(char *path, int32 path_len) {
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

int32
ncm_path_extension_start(char *path, int32 path_len) {
    int32 basename;
    int32 dot;

    if (path_len <= 0) {
        return -1;
    }

    basename = ncm_path_basename_start(path, path_len);
    dot = cbase_string_last_index_of(path, path_len, '.');
    if (dot <= basename) {
        return -1;
    }

    return dot + 1;
}

bool
ncm_path_has_extension(char *path, int32 path_len,
                       char *extension, int32 extension_len) {
    int32 start;

    start = ncm_path_extension_start(path, path_len);
    if (start < 0) {
        return false;
    }

    return ncm_string_equal(path + start, path_len - start,
                            extension, extension_len);
}
