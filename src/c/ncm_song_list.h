#if !defined(NCM_SONG_LIST_H)
#define NCM_SONG_LIST_H

#include "c/ncm_app_arrays.h"
#include "c/ncm_defs.h"
#include "c/ncm_song.h"

#if defined(__cplusplus)
extern "C" {
#endif

enum NcmSongListItemFlag {
    NCM_SONG_LIST_ITEM_SELECTABLE = 1 << 0,
    NCM_SONG_LIST_ITEM_SELECTED = 1 << 1,
    NCM_SONG_LIST_ITEM_INACTIVE = 1 << 2,
    NCM_SONG_LIST_ITEM_SEPARATOR = 1 << 3,
};

typedef struct NcmSongListItem {
    NcmSong song;
    uint32 flags;
} NcmSongListItem;

typedef struct NcmSongList {
    NcmSongListItem *items;
    int32 len;
    int32 cap;
    int32 current;
} NcmSongList;

enum NcmSongListSearchDirection {
    NCM_SONG_LIST_SEARCH_FORWARD,
    NCM_SONG_LIST_SEARCH_BACKWARD,
};

typedef bool (*NcmSongListPredicate)(NcmSong *song, void *user);
typedef bool (*NcmSongListEachFunc)(NcmSong *song, int32 idx, void *user);

void ncm_song_list_item_init(NcmSongListItem *item);
void ncm_song_list_item_destroy(NcmSongListItem *item);
bool ncm_song_list_item_copy(NcmSongListItem *dest,
                             NcmSongListItem *source);
void ncm_song_list_item_move(NcmSongListItem *dest,
                             NcmSongListItem *source);

void ncm_song_list_init(NcmSongList *list);
void ncm_song_list_destroy(NcmSongList *list);
void ncm_song_list_clear(NcmSongList *list);
bool ncm_song_list_reserve(NcmSongList *list, int32 extra);
NcmSongListItem *ncm_song_list_append(NcmSongList *list);
bool ncm_song_list_append_copy(NcmSongList *list, NcmSong *song,
                               uint32 flags);
void ncm_song_list_append_move(NcmSongList *list, NcmSong *song,
                               uint32 flags);
NcmSongListItem *ncm_song_list_at(NcmSongList *list, int32 idx);
NcmSongListItem *ncm_song_list_current(NcmSongList *list);
bool ncm_song_list_set_current(NcmSongList *list, int32 idx);
bool ncm_song_list_item_is_selected(NcmSongListItem *item);
bool ncm_song_list_item_is_selectable(NcmSongListItem *item);
void ncm_song_list_item_set_selected(NcmSongListItem *item, bool selected);
bool ncm_song_list_selected_songs(NcmSongList *list, NcmSongArray *songs);
int32 ncm_song_list_selected_count(NcmSongList *list);
bool ncm_song_list_has_selected(NcmSongList *list);
bool ncm_song_list_each_selected_song(NcmSongList *list,
                                 NcmSongListEachFunc each,
                                 void *user);
bool ncm_song_list_select_current_song_if_none_selected(NcmSongList *list);
void ncm_song_list_clear_selection(NcmSongList *list);
void ncm_song_list_reverse_selectable_selection(NcmSongList *list);
bool ncm_song_list_find_position(NcmSongList *list, uint32 position,
                                 int32 *idx);
bool ncm_song_list_wrapped_song_search(NcmSongList *list,
                                  NcmSongListPredicate predicate,
                                  void *user,
                                  enum NcmSongListSearchDirection direction,
                                  bool wrap, bool skip_current);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_SONG_LIST_H */
