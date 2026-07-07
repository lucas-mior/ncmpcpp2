#if !defined(NCM_PLAYLIST_H)
#define NCM_PLAYLIST_H

#include <time.h>

#include "c/ncm_defs.h"

struct mpd_playlist;

NCM_EXTERN_C_BEGIN

typedef struct NcmPlaylist {
    char *path;
    int32 path_len;
    time_t last_modified;
} NcmPlaylist;

void ncm_playlist_init(NcmPlaylist *playlist);
void ncm_playlist_destroy(NcmPlaylist *playlist);
void ncm_playlist_move(NcmPlaylist *dest, NcmPlaylist *source);
bool ncm_playlist_set(NcmPlaylist *playlist, char *path,
                      int32 path_len, time_t last_modified);
bool ncm_playlist_copy(NcmPlaylist *dest, NcmPlaylist *source);
bool ncm_playlist_path_view(NcmPlaylist *playlist, NcmStringView *view);
time_t ncm_playlist_last_modified(NcmPlaylist *playlist);
bool ncm_playlist_equal(NcmPlaylist *a, NcmPlaylist *b);
bool ncm_playlist_from_mpd_playlist(NcmPlaylist *dest,
                                    struct mpd_playlist *source);

NCM_EXTERN_C_END

#endif /* NCM_PLAYLIST_H */
