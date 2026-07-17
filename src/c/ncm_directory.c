#include "c/ncm_directory.h"

#include <mpd/client.h>

#include "c/ncm_base.h"
#include "cbase/util.c"

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
        free2(directory->path, directory->path_len + 1);
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
    replacement.path = malloc2(path_len + 1);
    replacement.path_len = path_len;
    replacement.last_modified = last_modified;
    memcpy64(replacement.path, path, path_len);
    replacement.path[path_len] = '\0';

    ncm_directory_destroy(directory);
    *directory = replacement;
    return true;
}

bool
ncm_directory_copy(NcmDirectory *dest, NcmDirectory *source) {
    if (dest == NULL) {
        return false;
    }
    if (source == NULL) {
        return false;
    }
    if (source->path == NULL) {
        ncm_directory_destroy(dest);
        return true;
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
        ncm_directory_init(dest);
        return;
    }

    *dest = *source;
    ncm_directory_init(source);
    return;
}

bool
ncm_directory_path_view(NcmDirectory *directory, NcmStringView *view) {
    if (view != NULL) {
        view->data = NULL;
        view->len = 0;
    }
    if (directory == NULL) {
        return false;
    }
    if (directory->path == NULL) {
        return false;
    }
    if (view != NULL) {
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

    path_len = optional_strlen32(path);
    last_modified = mpd_directory_get_last_modified(source);
    return ncm_directory_set(dest, path, path_len, last_modified);
}
