#include "c/ncm_playlist.h"

#include <mpd/client.h>

#include "c/ncm_base.h"
#include "cbase/util.c"

static int32 ncm_playlist_cstring_len(char *string);

static int32
ncm_playlist_cstring_len(char *string) {
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
ncm_playlist_init(NcmPlaylist *playlist) {
    playlist->path = NULL;
    playlist->path_len = 0;
    playlist->last_modified = 0;
    return;
}

void
ncm_playlist_destroy(NcmPlaylist *playlist) {
    if (playlist->path) {
        free2(playlist->path, playlist->path_len + 1);
    }

    playlist->path = NULL;
    playlist->path_len = 0;
    playlist->last_modified = 0;
    return;
}

bool
ncm_playlist_set(NcmPlaylist *playlist, char *path,
                 int32 path_len, time_t last_modified) {
    NcmPlaylist replacement;

    if (playlist == NULL) {
        return false;
    }
    if (path == NULL) {
        return false;
    }
    if (path_len < 0) {
        return false;
    }

    ncm_playlist_init(&replacement);
    replacement.path = malloc2(path_len + 1);
    replacement.path_len = path_len;
    replacement.last_modified = last_modified;
    memcpy64(replacement.path, path, path_len);
    replacement.path[path_len] = '\0';

    ncm_playlist_destroy(playlist);
    *playlist = replacement;
    return true;
}

bool
ncm_playlist_copy(NcmPlaylist *dest, NcmPlaylist *source) {
    if (dest == NULL) {
        return false;
    }
    if (source == NULL) {
        return false;
    }
    if (source->path == NULL) {
        ncm_playlist_destroy(dest);
        return true;
    }

    return ncm_playlist_set(dest, source->path, source->path_len,
                            source->last_modified);
}

bool
ncm_playlist_from_mpd_playlist(NcmPlaylist *dest,
                               struct mpd_playlist *source) {
    char *path;
    int32 path_len;
    time_t last_modified;

    if (source == NULL) {
        return false;
    }

    path = (char *)mpd_playlist_get_path(source);
    if (path == NULL) {
        return false;
    }

    path_len = ncm_playlist_cstring_len(path);
    last_modified = mpd_playlist_get_last_modified(source);
    return ncm_playlist_set(dest, path, path_len, last_modified);
}

void
ncm_playlist_move(NcmPlaylist *dest, NcmPlaylist *source) {
    if (dest == NULL) {
        return;
    }
    if (dest == source) {
        return;
    }

    ncm_playlist_destroy(dest);
    if (source == NULL) {
        ncm_playlist_init(dest);
        return;
    }

    *dest = *source;
    ncm_playlist_init(source);
    return;
}

bool
ncm_playlist_path_view(NcmPlaylist *playlist, NcmStringView *view) {
    if (view != NULL) {
        view->data = NULL;
        view->len = 0;
    }
    if (playlist == NULL) {
        return false;
    }
    if (playlist->path == NULL) {
        return false;
    }
    if (view != NULL) {
        view->data = playlist->path;
        view->len = playlist->path_len;
    }

    return true;
}

time_t
ncm_playlist_last_modified(NcmPlaylist *playlist) {
    if (playlist == NULL) {
        return 0;
    }

    return playlist->last_modified;
}

bool
ncm_playlist_equal(NcmPlaylist *a, NcmPlaylist *b) {
    if ((a == NULL) || (b == NULL)) {
        return a == b;
    }
    if (a->last_modified != b->last_modified) {
        return false;
    }
    if (a->path_len != b->path_len) {
        return false;
    }
    if ((a->path == NULL) || (b->path == NULL)) {
        return a->path == b->path;
    }

    for (int32 i = 0; i < a->path_len; i += 1) {
        if (a->path[i] != b->path[i]) {
            return false;
        }
    }

    return true;
}
