#if !defined(NCM_SONG_H)
#define NCM_SONG_H

#include "c/ncm_defs.h"

struct mpd_song;

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcmSong {
    struct mpd_song *song;
} NcmSong;

void ncm_song_init(NcmSong *song);
void ncm_song_destroy(NcmSong *song);
bool ncm_song_copy(NcmSong *dest, NcmSong *source);
bool ncm_song_from_mpd_song(NcmSong *dest, struct mpd_song *source);
bool ncm_song_empty(NcmSong *song);
struct mpd_song *ncm_song_mpd_song(NcmSong *song);
struct mpd_song *ncm_song_dup_mpd_song(NcmSong *song);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_SONG_H */
