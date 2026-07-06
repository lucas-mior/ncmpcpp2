#if !defined(NCM_MPD_ITEM_H)
#define NCM_MPD_ITEM_H

#include "c/ncm_defs.h"

#if defined(__cplusplus)
#include <new>
#endif
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

typedef union NcmMpdItemValue {
    NcmSong song;
    NcmDirectory directory;
    NcmPlaylist playlist;

#if defined(__cplusplus)
    NcmMpdItemValue() { }
    ~NcmMpdItemValue() { }
#endif
} NcmMpdItemValue;


typedef struct NcmMpdItem {
    enum NcmMpdItemKind kind;
    NcmMpdItemValue value;

#if defined(__cplusplus)
    NcmMpdItem();
    NcmMpdItem(const NcmMpdItem &rhs);
    NcmMpdItem(NcmMpdItem &&rhs) noexcept;
    NcmMpdItem &operator=(const NcmMpdItem &rhs);
    NcmMpdItem &operator=(NcmMpdItem &&rhs) noexcept;
    ~NcmMpdItem();
#endif
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

#if defined(__cplusplus)
inline NcmMpdItem::NcmMpdItem()
{
    ncm_mpd_item_init(this);
}

inline NcmMpdItem::NcmMpdItem(const NcmMpdItem &rhs)
{
    ncm_mpd_item_init(this);
    if (!ncm_mpd_item_copy(this, const_cast<NcmMpdItem *>(&rhs)))
    {
        throw std::bad_alloc();
    }
}

inline NcmMpdItem::NcmMpdItem(NcmMpdItem &&rhs) noexcept
{
    ncm_mpd_item_init(this);
    ncm_mpd_item_move(this, &rhs);
}

inline NcmMpdItem &NcmMpdItem::operator=(const NcmMpdItem &rhs)
{
    if (this != &rhs)
    {
        if (!ncm_mpd_item_copy(this, const_cast<NcmMpdItem *>(&rhs)))
        {
            throw std::bad_alloc();
        }
    }
    return *this;
}

inline NcmMpdItem &NcmMpdItem::operator=(NcmMpdItem &&rhs) noexcept
{
    if (this != &rhs)
    {
        ncm_mpd_item_move(this, &rhs);
    }
    return *this;
}

inline NcmMpdItem::~NcmMpdItem()
{
    ncm_mpd_item_destroy(this);
}
#endif

#endif /* NCM_MPD_ITEM_H */
