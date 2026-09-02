#if !defined(NCM_DIRECTORY_C)
#define NCM_DIRECTORY_C

#include "cbase.h"

#include <mpd/client.h>

#include "c/ncm_c.h"

void
ncm_directory_destroy(NcmDirectory *directory) {
    free2(directory->path, directory->path_len + 1);

    directory->path = NULL;
    directory->path_len = 0;
    directory->last_modified = 0;

    return;
}

int32
ncm_directory_set(NcmDirectory *directory, char *path,
                  int32 path_len, time_t last_modified) {
    NcmDirectory replacement;

    if (directory == NULL) {
        return -EINVAL;
    }
    if (path == NULL) {
        return -EINVAL;
    }
    if (path_len < 0) {
        return -EINVAL;
    }

    replacement = (NcmDirectory){0};
    replacement.path = malloc2(path_len + 1);
    replacement.path_len = path_len;
    replacement.last_modified = last_modified;
    memcpy64(replacement.path, path, path_len);
    replacement.path[path_len] = '\0';

    ncm_directory_destroy(directory);
    *directory = replacement;
    return 0;
}

int32
ncm_directory_copy(NcmDirectory *dest, NcmDirectory *source) {
    if (dest == NULL) {
        return -EINVAL;
    }
    if (source == NULL) {
        return -EINVAL;
    }
    if (source->path == NULL) {
        ncm_directory_destroy(dest);
        return 0;
    }

    return ncm_directory_set(dest, source->path, source->path_len,
                             source->last_modified);
}

void
ncm_directory_move(NcmDirectory *dest, NcmDirectory *source) {
    if (dest == NULL) {
        return;
    }
    if (dest == source) {
        return;
    }

    ncm_directory_destroy(dest);
    if (source == NULL) {
        *dest = (NcmDirectory){0};
        return;
    }

    *dest = *source;
    *source = (NcmDirectory){0};
    return;
}

bool
ncm_directory_has_path_view(NcmDirectory *directory, NcmStringView *view) {
    if (view) {
        view->data = NULL;
        view->len = 0;
    }
    if (directory == NULL) {
        return false;
    }
    if (directory->path == NULL) {
        return false;
    }
    if (view) {
        view->data = directory->path;
        view->len = directory->path_len;
    }

    return true;
}

time_t
ncm_directory_last_modified(NcmDirectory *directory) {
    if (directory == NULL) {
        return 0;
    }

    return directory->last_modified;
}

int32
ncm_directory_from_mpd_directory(NcmDirectory *dest,
                                 struct mpd_directory *source) {
    char *path;
    int32 path_len;
    time_t last_modified;

    if (dest == NULL) {
        return -EINVAL;
    }
    if (source == NULL) {
        return -EINVAL;
    }

    if ((path = (char *)mpd_directory_get_path(source)) == NULL) {
        return -NCM_ERROR_NOT_FOUND;
    }

    path_len = optional_strlen32(path);
    last_modified = mpd_directory_get_last_modified(source);
    return ncm_directory_set(dest, path, path_len, last_modified);
}

#endif /* NCM_DIRECTORY_C */
