#if !defined(NCM_MPD_ITEM_H)
#define NCM_MPD_ITEM_H

#include "c/ncm_defs.h"
#include "c/ncm_directory.h"
#include "c/ncm_playlist.h"
#include "c/ncm_song.h"

struct mpd_entity;

enum NcmMpdItemKind {
    NCM_MPD_ITEM_UNKNOWN,
    NCM_MPD_ITEM_SONG,
    NCM_MPD_ITEM_DIRECTORY,
    NCM_MPD_ITEM_PLAYLIST,
};

typedef union NcmMpdItemValue {
    NcmSong song;
    NcmDirectory directory;
    NcmPlaylist playlist;
} NcmMpdItemValue;


typedef struct NcmMpdItem {
    enum NcmMpdItemKind kind;
    NcmMpdItemValue value;
} NcmMpdItem;

void ncm_mpd_item_init(NcmMpdItem *item);
void ncm_mpd_item_destroy(NcmMpdItem *item);
void ncm_mpd_item_move(NcmMpdItem *dest, NcmMpdItem *source);
bool ncm_mpd_item_copy(NcmMpdItem *dest, NcmMpdItem *source);
bool ncm_mpd_item_set_song(NcmMpdItem *item, NcmSong *source);
bool ncm_mpd_item_set_directory(NcmMpdItem *item, NcmDirectory *source);
bool ncm_mpd_item_from_entity_copy(NcmMpdItem *item,
                                   struct mpd_entity *entity);
enum NcmMpdItemKind ncm_mpd_item_kind(NcmMpdItem *item);
NcmSong *ncm_mpd_item_song(NcmMpdItem *item);
NcmDirectory *ncm_mpd_item_directory(NcmMpdItem *item);
NcmPlaylist *ncm_mpd_item_playlist(NcmMpdItem *item);

#endif /* NCM_MPD_ITEM_H */
