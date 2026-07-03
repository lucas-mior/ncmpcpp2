#include "c/ncm_playlist.h"

#include <mpd/client.h>

#include "c/ncm_base.h"

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
        ncm_free(playlist->path, playlist->path_len + 1);
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
    replacement.path = ncm_malloc(path_len + 1);
    replacement.path_len = path_len;
    replacement.last_modified = last_modified;
    ncm_memcpy(replacement.path, path, path_len);
    replacement.path[path_len] = '\0';

    ncm_playlist_destroy(playlist);
    *playlist = replacement;
    return true;
}

bool
ncm_playlist_copy(NcmPlaylist *dest, NcmPlaylist *source) {
    if (source == NULL) {
        return false;
    }
    if (source->path == NULL) {
        return false;
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
