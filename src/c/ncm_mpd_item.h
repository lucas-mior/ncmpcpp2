#if !defined(NCM_MPD_ITEM_H)
#define NCM_MPD_ITEM_H

#include "c/ncm_defs.h"
#include "c/ncm_directory.h"
#include "c/ncm_playlist.h"
#include "c/ncm_song.h"

struct mpd_entity;

#if defined(__cplusplus)
extern "C" {
#endif

enum NcmMpdItemKind {
    NCM_MPD_ITEM_UNKNOWN,
    NCM_MPD_ITEM_SONG,
    NCM_MPD_ITEM_DIRECTORY,
    NCM_MPD_ITEM_PLAYLIST,
};

typedef struct NcmMpdItem {
    enum NcmMpdItemKind kind;
    union {
        NcmSong song;
        NcmDirectory directory;
        NcmPlaylist playlist;
    } value;
} NcmMpdItem;

void ncm_mpd_item_init(NcmMpdItem *item);
void ncm_mpd_item_destroy(NcmMpdItem *item);
void ncm_mpd_item_move(NcmMpdItem *dest, NcmMpdItem *source);
bool ncm_mpd_item_copy(NcmMpdItem *dest, NcmMpdItem *source);
bool ncm_mpd_item_equal(NcmMpdItem *a, NcmMpdItem *b);
bool ncm_mpd_item_set_song(NcmMpdItem *item, NcmSong *source);
bool ncm_mpd_item_set_directory(NcmMpdItem *item, NcmDirectory *source);
bool ncm_mpd_item_set_playlist(NcmMpdItem *item, NcmPlaylist *source);
bool ncm_mpd_item_from_mpd_song_copy(NcmMpdItem *item,
                                     struct mpd_song *source);
bool ncm_mpd_item_from_mpd_song_borrow(NcmMpdItem *item,
                                       struct mpd_song *source);
bool ncm_mpd_item_from_entity(NcmMpdItem *item, struct mpd_entity *entity);
bool ncm_mpd_item_from_entity_copy(NcmMpdItem *item,
                                   struct mpd_entity *entity);
bool ncm_mpd_item_from_entity_borrow(NcmMpdItem *item,
                                     struct mpd_entity *entity);
enum NcmMpdItemKind ncm_mpd_item_kind(NcmMpdItem *item);
NcmSong *ncm_mpd_item_song(NcmMpdItem *item);
NcmDirectory *ncm_mpd_item_directory(NcmMpdItem *item);
NcmPlaylist *ncm_mpd_item_playlist(NcmMpdItem *item);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_MPD_ITEM_H */
