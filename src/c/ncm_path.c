#include "c/ncm_path.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include "cbase/base_macros.h"
#include "cbase/cbase.h"
#include "c/ncm_base.h"
#include "c/ncm_string.h"

bool
ncm_path_expand_home(NcmBuffer *path, NcmError *error) {
    char *home;
    int32 home_len;
    int32 tilde;
    int32 old_len;

    if (path == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing path buffer"));
        return false;
    }
    if ((path->len < 0)
        || ((path->data == NULL) && (path->len > 0))) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("invalid path buffer"));
        return false;
    }

    tilde = -1;
    for (int32 i = 0; i < path->len; i += 1) {
        if ((path->data[i] == '~')
            && ((i == 0) || (path->data[i - 1] == '@'))) {
            tilde = i;
            break;
        }
    }
    if (tilde < 0) {
        ncm_error_clear(error);
        return true;
    }

    home = getenv("HOME");
    if ((home == NULL) || (home[0] == '\0')) {
        ncm_error_set(error, ENOENT,
                      STRLIT_ARGS("HOME environment variable is not set"));
        return false;
    }

    home_len = 0;
    while (home[home_len] != '\0') {
        if (home_len == INT32_MAX) {
            ncm_error_set(error, ENAMETOOLONG,
                          STRLIT_ARGS("HOME path is too long"));
            return false;
        }
        home_len += 1;
    }

    old_len = path->len;
    if (home_len > (INT32_MAX - (old_len - 1))) {
        ncm_error_set(error, ENAMETOOLONG,
                      STRLIT_ARGS("expanded path is too long"));
        return false;
    }
    ncm_buffer_reserve(path, home_len - 1);
    ncm_memmove(path->data + tilde + home_len,
                path->data + tilde + 1, old_len - tilde);
    ncm_memcpy(path->data + tilde, home, home_len);
    path->len = old_len - 1 + home_len;
    ncm_error_clear(error);
    return true;
}

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
