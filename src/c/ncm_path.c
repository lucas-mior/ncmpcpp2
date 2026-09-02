#if !defined(NCM_PATH_C)
#define NCM_PATH_C

#include "cbase.h"

#include "c/ncm_c.h"

static int32
ncm_path_last_index_of(char *path, int32 path_len, char needle) {
    char *found;

    if (path_len <= 0) {
        return -1;
    }

    if ((found = memrchr64(path, needle, path_len)) == NULL) {
        return -1;
    }

    return (int32)(found - path);
}

int32
ncm_path_expand_home(StrBuilder *path, NcmError *ncm_error) {
    char *home;
    int32 home_len;
    int32 tilde;
    int32 old_len;

    if (path == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing path buffer"));
    }
    if ((path->len < 0)
        || ((path->data == NULL) && (path->len > 0))) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("invalid path buffer"));
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
        return ncm_error_ok(ncm_error);
    }

    if (((home = getenv("HOME")) == NULL) || (home[0] == '\0')) {
        return ncm_error_set_status(
            ncm_error, -ENOENT,
            STRLIT("HOME environment variable is not set"));
    }

    home_len = 0;
    while (home[home_len] != '\0') {
        if (home_len == INT32_MAX) {
            return ncm_error_set_status(
                ncm_error, -ENAMETOOLONG,
                STRLIT("HOME path is too long"));
        }
        home_len += 1;
    }

    old_len = path->len;
    if (home_len > (INT32_MAX - (old_len - 1))) {
        return ncm_error_set_status(
            ncm_error, -ENAMETOOLONG,
            STRLIT("expanded path is too long"));
    }
    sb_reserve(path, home_len - 1);
    memmove64(path->data + tilde + home_len,
              path->data + tilde + 1, old_len - tilde);
    memcpy64(path->data + tilde, home, home_len);
    path->len = old_len - 1 + home_len;
    return ncm_error_ok(ncm_error);
}

int32
ncm_path_basename_start(char *path, int32 path_len) {
    int32 slash;
    int32 result;

    if (path_len <= 0) {
        return 0;
    }

    slash = ncm_path_last_index_of(path, path_len, '/');
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

    slash = ncm_path_last_index_of(path, path_len, '/');
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
    dot = ncm_path_last_index_of(path, path_len, '.');
    if (dot <= basename) {
        return -1;
    }

    return dot + 1;
}

#endif /* NCM_PATH_C */
