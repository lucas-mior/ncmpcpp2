#include "c/ncm_directory.h"

#include <mpd/client.h>

#include "c/ncm_base.h"

static int32 ncm_directory_cstring_len(char *string);

static int32
ncm_directory_cstring_len(char *string) {
    int32 len;

    if (string == NULL) {
        return 0;
    }

    len = 0;
    while (string[len] != '\0') {
        len += 1;
    }

    return len;
}

void
ncm_directory_init(NcmDirectory *directory) {
    directory->path = NULL;
    directory->path_len = 0;
    directory->last_modified = 0;
    return;
}

void
ncm_directory_destroy(NcmDirectory *directory) {
    if (directory->path) {
        ncm_free(directory->path, directory->path_len + 1);
    }

    directory->path = NULL;
    directory->path_len = 0;
    directory->last_modified = 0;
    return;
}

bool
ncm_directory_set(NcmDirectory *directory, char *path,
                  int32 path_len, time_t last_modified) {
    NcmDirectory replacement;

    if (directory == NULL) {
        return false;
    }
    if (path == NULL) {
        return false;
    }
    if (path_len < 0) {
        return false;
    }

    ncm_directory_init(&replacement);
    replacement.path = ncm_malloc(path_len + 1);
    replacement.path_len = path_len;
    replacement.last_modified = last_modified;
    ncm_memcpy(replacement.path, path, path_len);
    replacement.path[path_len] = '\0';

    ncm_directory_destroy(directory);
    *directory = replacement;
    return true;
}

bool
ncm_directory_copy(NcmDirectory *dest, NcmDirectory *source) {
    if (source == NULL) {
        return false;
    }
    if (source->path == NULL) {
        return false;
    }

    return ncm_directory_set(dest, source->path, source->path_len,
                             source->last_modified);
}

bool
ncm_directory_from_mpd_directory(NcmDirectory *dest,
                                 struct mpd_directory *source) {
    char *path;
    int32 path_len;
    time_t last_modified;

    if (source == NULL) {
        return false;
    }

    path = (char *)mpd_directory_get_path(source);
    if (path == NULL) {
        return false;
    }

    path_len = ncm_directory_cstring_len(path);
    last_modified = mpd_directory_get_last_modified(source);
    return ncm_directory_set(dest, path, path_len, last_modified);
}
