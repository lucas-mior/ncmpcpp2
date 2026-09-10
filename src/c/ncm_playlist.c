#if !defined(NCM_PLAYLIST_C)
#define NCM_PLAYLIST_C

#include "cbase.h"
#include "ncmpcpp2.h"

#include "c/ncm_c.h"

void
ncm_playlist_destroy(NcmPlaylist *playlist) {
    ASSERT(playlist != NULL);

    free2(playlist->path, playlist->path_len + 1);

    playlist->path = NULL;
    playlist->path_len = 0;
    playlist->last_modified = 0;

    return;
}

int32
ncm_playlist_set(NcmPlaylist *playlist,
                 char *path, int32 path_len,
                 time_t last_modified) {
    NcmPlaylist replacement = {0};

    ASSERT(playlist != NULL);
    ASSERT(path != NULL);
    ASSERT(path_len >= 0);

    replacement.path = malloc2(path_len + 1);
    replacement.path_len = path_len;
    replacement.last_modified = last_modified;
    memcpy64(replacement.path, path, path_len);
    replacement.path[path_len] = '\0';

    ncm_playlist_destroy(playlist);
    *playlist = replacement;
    return 0;
}

int32
ncm_playlist_copy(NcmPlaylist *dest, NcmPlaylist *source) {
    ASSERT(dest != NULL);
    ASSERT(source != NULL);

    if (source->path == NULL) {
        ncm_playlist_destroy(dest);
        return 0;
    }

    return ncm_playlist_set(dest, source->path, source->path_len,
                            source->last_modified);
}

void
ncm_playlist_move(NcmPlaylist *dest, NcmPlaylist *source) {
    ASSERT(dest != NULL);
    ASSERT(source != NULL);

    if (dest == source) {
        return;
    }

    ncm_playlist_destroy(dest);
    *dest = *source;
    *source = (NcmPlaylist){0};
    return;
}

bool
ncm_playlist_has_path_view(NcmPlaylist *playlist, StringView *view) {
    ASSERT(playlist != NULL);

    if (view) {
        view->data = NULL;
        view->len = 0;
    }
    if (playlist->path == NULL) {
        return false;
    }
    if (view) {
        view->data = playlist->path;
        view->len = playlist->path_len;
    }

    return true;
}

time_t
ncm_playlist_last_modified(NcmPlaylist *playlist) {
    ASSERT(playlist != NULL);

    return playlist->last_modified;
}

#endif /* NCM_PLAYLIST_C */
