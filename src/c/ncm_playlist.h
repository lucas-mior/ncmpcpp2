#if !defined(NCM_PLAYLIST_H)
#define NCM_PLAYLIST_H

#include <time.h>

#include "c/ncm_defs.h"

struct mpd_playlist;

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcmPlaylist {
    char *path;
    int32 path_len;
    time_t last_modified;
} NcmPlaylist;

void ncm_playlist_init(NcmPlaylist *playlist);
void ncm_playlist_destroy(NcmPlaylist *playlist);
bool ncm_playlist_set(NcmPlaylist *playlist, char *path,
                      int32 path_len, time_t last_modified);
bool ncm_playlist_copy(NcmPlaylist *dest, NcmPlaylist *source);
bool ncm_playlist_from_mpd_playlist(NcmPlaylist *dest,
                                    struct mpd_playlist *source);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_PLAYLIST_H */
