#if !defined(NCMPCPP_NC_APP_MENUS_H)
#define NCMPCPP_NC_APP_MENUS_H

#include <stdbool.h>
#include <time.h>

#include "c/ncm_mpd_item.h"
#include "c/ncm_mutable_song.h"
#include "c/ncm_playlist.h"
#include "c/ncm_song.h"
#include "curses/nc_buffer.h"
#include "curses/nc_menu.h"

typedef struct NcMenuString {
    char *data;
    int32 len;
    int32 cap;
} NcMenuString;

typedef struct NcMenuStringPair {
    char *first;
    char *second;

    int32 first_len;
    int32 first_cap;
    int32 second_len;
    int32 second_cap;
} NcMenuStringPair;

typedef struct NcSearchRow {
    NcmSong song;
    NcBuffer buffer;
    bool is_song;
} NcSearchRow;

typedef struct NcMediaLibraryTagRow {
    char *tag;
    int32 tag_len;
    int32 tag_cap;
    time_t mtime;
} NcMediaLibraryTagRow;

typedef struct NcMediaLibraryAlbumRow {
    char *tag;
    char *album;
    char *date;

    int32 tag_len;
    int32 tag_cap;
    int32 album_len;
    int32 album_cap;
    int32 date_len;
    int32 date_cap;

    time_t mtime;
    bool all_tracks_entry;
} NcMediaLibraryAlbumRow;

typedef struct NcEditorActionRow {
    char *label;
    int32 label_len;
    int32 label_cap;

    void (*run)(void *user);
    void *user;
} NcEditorActionRow;

typedef struct NcEditorSortRow {
    NcEditorActionRow action;
    enum NcmSongGetter getter;
} NcEditorSortRow;

#define NC_TYPED_MENU_DECLARE_TYPE(TYPE_NAME) \
    typedef struct TYPE_NAME { \
        NcMenu menu; \
    } TYPE_NAME

#define NC_TYPED_MENU_DECLARE_INIT(TYPE_NAME, PREFIX) \
    void PREFIX##_init(TYPE_NAME *menu)

#define NC_TYPED_MENU_DECLARE_DESTROY(TYPE_NAME, PREFIX) \
    void PREFIX##_destroy(TYPE_NAME *menu)

#define NC_TYPED_MENU_DECLARE_BASE(TYPE_NAME, PREFIX) \
    NcMenu *PREFIX##_base(TYPE_NAME *menu)

#define NC_TYPED_MENU_DECLARE_ADD(TYPE_NAME, PREFIX, ITEM_TYPE) \
    void PREFIX##_add(TYPE_NAME *menu, ITEM_TYPE *item)

#define NC_TYPED_MENU_DECLARE_ADD_WITH_FLAGS(TYPE_NAME, PREFIX, ITEM_TYPE) \
    void PREFIX##_add_with_flags(TYPE_NAME *menu, ITEM_TYPE *item, \
                                 uint32 flags)

#define NC_TYPED_MENU_DECLARE_ADD_SEPARATOR(TYPE_NAME, PREFIX) \
    void PREFIX##_add_separator(TYPE_NAME *menu)

#define NC_TYPED_MENU_DECLARE_INSERT_WITH_FLAGS(TYPE_NAME, PREFIX, ITEM_TYPE) \
    void PREFIX##_insert_with_flags(TYPE_NAME *menu, int64 pos, \
                                    ITEM_TYPE *item, uint32 flags)

#define NC_TYPED_MENU_DECLARE_ITEM_AT(TYPE_NAME, PREFIX, ITEM_TYPE) \
    ITEM_TYPE *PREFIX##_item_at(TYPE_NAME *menu, \
                                enum NcMenuItemSource source, \
                                int64 pos)

#define NC_TYPED_MENU_DECLARE_CURRENT(TYPE_NAME, PREFIX, ITEM_TYPE) \
    ITEM_TYPE *PREFIX##_current(TYPE_NAME *menu)

#define NC_TYPED_MENU_DECLARE_COMMON(TYPE_NAME, PREFIX) \
    NC_TYPED_MENU_DECLARE_TYPE(TYPE_NAME); \
    NC_TYPED_MENU_DECLARE_INIT(TYPE_NAME, PREFIX); \
    NC_TYPED_MENU_DECLARE_DESTROY(TYPE_NAME, PREFIX); \
    NC_TYPED_MENU_DECLARE_BASE(TYPE_NAME, PREFIX)

NC_TYPED_MENU_DECLARE_COMMON(NcSongMenu, nc_song_menu);
NC_TYPED_MENU_DECLARE_ADD(NcSongMenu, nc_song_menu, NcmSong);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcSongMenu, nc_song_menu, NcmSong);
NC_TYPED_MENU_DECLARE_CURRENT(NcSongMenu, nc_song_menu, NcmSong);

NC_TYPED_MENU_DECLARE_COMMON(NcBrowserEntryMenu, nc_browser_entry_menu);
NC_TYPED_MENU_DECLARE_ADD(NcBrowserEntryMenu,
                          nc_browser_entry_menu,
                          NcmMpdItem);
NC_TYPED_MENU_DECLARE_CURRENT(NcBrowserEntryMenu,
                              nc_browser_entry_menu,
                              NcmMpdItem);

NC_TYPED_MENU_DECLARE_COMMON(NcPlaylistEntryMenu,
                             nc_playlist_entry_menu);
NC_TYPED_MENU_DECLARE_ADD(NcPlaylistEntryMenu,
                          nc_playlist_entry_menu,
                          NcmPlaylist);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcPlaylistEntryMenu,
                              nc_playlist_entry_menu,
                              NcmPlaylist);
NC_TYPED_MENU_DECLARE_CURRENT(NcPlaylistEntryMenu,
                              nc_playlist_entry_menu,
                              NcmPlaylist);

NC_TYPED_MENU_DECLARE_COMMON(NcTagRowMenu, nc_tag_row_menu);
NC_TYPED_MENU_DECLARE_ADD(NcTagRowMenu,
                          nc_tag_row_menu,
                          NcmMutableSong);
NC_TYPED_MENU_DECLARE_CURRENT(NcTagRowMenu,
                              nc_tag_row_menu,
                              NcmMutableSong);

NC_TYPED_MENU_DECLARE_COMMON(NcSearchRowMenu, nc_search_row_menu);
NC_TYPED_MENU_DECLARE_ADD_WITH_FLAGS(NcSearchRowMenu,
                                     nc_search_row_menu,
                                     NcSearchRow);
NC_TYPED_MENU_DECLARE_INSERT_WITH_FLAGS(NcSearchRowMenu,
                                        nc_search_row_menu,
                                        NcSearchRow);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcSearchRowMenu,
                              nc_search_row_menu,
                              NcSearchRow);
NC_TYPED_MENU_DECLARE_CURRENT(NcSearchRowMenu,
                              nc_search_row_menu,
                              NcSearchRow);

NC_TYPED_MENU_DECLARE_COMMON(NcMediaLibraryTagMenu,
                             nc_media_library_tag_menu);
NC_TYPED_MENU_DECLARE_ADD(NcMediaLibraryTagMenu,
                          nc_media_library_tag_menu,
                          NcMediaLibraryTagRow);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcMediaLibraryTagMenu,
                              nc_media_library_tag_menu,
                              NcMediaLibraryTagRow);
NC_TYPED_MENU_DECLARE_CURRENT(NcMediaLibraryTagMenu,
                              nc_media_library_tag_menu,
                              NcMediaLibraryTagRow);

NC_TYPED_MENU_DECLARE_COMMON(NcMediaLibraryAlbumMenu,
                             nc_media_library_album_menu);
NC_TYPED_MENU_DECLARE_ADD(NcMediaLibraryAlbumMenu,
                          nc_media_library_album_menu,
                          NcMediaLibraryAlbumRow);
NC_TYPED_MENU_DECLARE_ADD_WITH_FLAGS(NcMediaLibraryAlbumMenu,
                                     nc_media_library_album_menu,
                                     NcMediaLibraryAlbumRow);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcMediaLibraryAlbumMenu,
                              nc_media_library_album_menu,
                              NcMediaLibraryAlbumRow);
NC_TYPED_MENU_DECLARE_CURRENT(NcMediaLibraryAlbumMenu,
                              nc_media_library_album_menu,
                              NcMediaLibraryAlbumRow);

NC_TYPED_MENU_DECLARE_COMMON(NcMediaLibrarySongMenu,
                             nc_media_library_song_menu);
NC_TYPED_MENU_DECLARE_ADD(NcMediaLibrarySongMenu,
                          nc_media_library_song_menu,
                          NcmSong);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcMediaLibrarySongMenu,
                              nc_media_library_song_menu,
                              NcmSong);
NC_TYPED_MENU_DECLARE_CURRENT(NcMediaLibrarySongMenu,
                              nc_media_library_song_menu,
                              NcmSong);

NC_TYPED_MENU_DECLARE_COMMON(NcEditorStringMenu,
                             nc_editor_string_menu);
NC_TYPED_MENU_DECLARE_ADD_WITH_FLAGS(NcEditorStringMenu,
                                     nc_editor_string_menu,
                                     NcMenuString);
NC_TYPED_MENU_DECLARE_ADD_SEPARATOR(NcEditorStringMenu,
                                    nc_editor_string_menu);

NC_TYPED_MENU_DECLARE_COMMON(NcEditorPairMenu, nc_editor_pair_menu);
NC_TYPED_MENU_DECLARE_ADD(NcEditorPairMenu,
                          nc_editor_pair_menu,
                          NcMenuStringPair);
NC_TYPED_MENU_DECLARE_CURRENT(NcEditorPairMenu,
                              nc_editor_pair_menu,
                              NcMenuStringPair);

NC_TYPED_MENU_DECLARE_COMMON(NcEditorActionMenu,
                             nc_editor_action_menu);
NC_TYPED_MENU_DECLARE_ADD(NcEditorActionMenu,
                          nc_editor_action_menu,
                          NcEditorActionRow);
NC_TYPED_MENU_DECLARE_ADD_SEPARATOR(NcEditorActionMenu,
                                    nc_editor_action_menu);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcEditorActionMenu,
                              nc_editor_action_menu,
                              NcEditorActionRow);

NC_TYPED_MENU_DECLARE_COMMON(NcEditorSortMenu, nc_editor_sort_menu);
NC_TYPED_MENU_DECLARE_ADD(NcEditorSortMenu,
                          nc_editor_sort_menu,
                          NcEditorSortRow);
NC_TYPED_MENU_DECLARE_ADD_SEPARATOR(NcEditorSortMenu,
                                    nc_editor_sort_menu);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcEditorSortMenu,
                              nc_editor_sort_menu,
                              NcEditorSortRow);
NC_TYPED_MENU_DECLARE_CURRENT(NcEditorSortMenu,
                              nc_editor_sort_menu,
                              NcEditorSortRow);

NC_TYPED_MENU_DECLARE_COMMON(NcEditorBufferMenu,
                             nc_editor_buffer_menu);
NC_TYPED_MENU_DECLARE_ADD_WITH_FLAGS(NcEditorBufferMenu,
                                     nc_editor_buffer_menu,
                                     NcBuffer);
NC_TYPED_MENU_DECLARE_ADD_SEPARATOR(NcEditorBufferMenu,
                                    nc_editor_buffer_menu);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcEditorBufferMenu,
                              nc_editor_buffer_menu,
                              NcBuffer);

#undef NC_TYPED_MENU_DECLARE_TYPE
#undef NC_TYPED_MENU_DECLARE_INIT
#undef NC_TYPED_MENU_DECLARE_DESTROY
#undef NC_TYPED_MENU_DECLARE_BASE
#undef NC_TYPED_MENU_DECLARE_ADD
#undef NC_TYPED_MENU_DECLARE_ADD_WITH_FLAGS
#undef NC_TYPED_MENU_DECLARE_ADD_SEPARATOR
#undef NC_TYPED_MENU_DECLARE_INSERT_WITH_FLAGS
#undef NC_TYPED_MENU_DECLARE_ITEM_AT
#undef NC_TYPED_MENU_DECLARE_CURRENT
#undef NC_TYPED_MENU_DECLARE_COMMON

void nc_menu_string_init(NcMenuString *string);
void nc_menu_string_destroy(NcMenuString *string);
bool nc_menu_string_copy(NcMenuString *dest, NcMenuString *source);
bool nc_menu_string_set(NcMenuString *string, char *data, int32 data_len);

void nc_menu_string_pair_init(NcMenuStringPair *pair);
void nc_menu_string_pair_destroy(NcMenuStringPair *pair);
bool nc_menu_string_pair_copy(NcMenuStringPair *dest,
                              NcMenuStringPair *source);

void nc_search_row_init(NcSearchRow *row);
void nc_search_row_destroy(NcSearchRow *row);
bool nc_search_row_copy(NcSearchRow *dest, NcSearchRow *source);

void nc_media_library_tag_row_init(NcMediaLibraryTagRow *row);
void nc_media_library_tag_row_destroy(NcMediaLibraryTagRow *row);
bool nc_media_library_tag_row_copy(NcMediaLibraryTagRow *dest,
                                   NcMediaLibraryTagRow *source);

void nc_media_library_album_row_init(NcMediaLibraryAlbumRow *row);
void nc_media_library_album_row_destroy(NcMediaLibraryAlbumRow *row);
bool nc_media_library_album_row_copy(NcMediaLibraryAlbumRow *dest,
                                     NcMediaLibraryAlbumRow *source);

void nc_editor_action_row_init(NcEditorActionRow *row);
void nc_editor_action_row_destroy(NcEditorActionRow *row);
bool nc_editor_action_row_copy(NcEditorActionRow *dest,
                               NcEditorActionRow *source);

void nc_editor_sort_row_init(NcEditorSortRow *row);
void nc_editor_sort_row_destroy(NcEditorSortRow *row);
bool nc_editor_sort_row_copy(NcEditorSortRow *dest,
                             NcEditorSortRow *source);

#endif /* NCMPCPP_NC_APP_MENUS_H */
