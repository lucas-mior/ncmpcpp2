#include "curses/nc_app_menus.h"

#include "c/ncm_base.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"

static void nc_menu_owned_string_destroy(char **data, int32 *len,
                                         int32 *cap);
static bool nc_menu_owned_string_copy(char **dest_data, int32 *dest_len,
                                      int32 *dest_cap, char *source_data,
                                      int32 source_len);

static void ncm_song_menu_item_init(void *item, void *user);
static void ncm_song_menu_item_copy(void *dest, void *source, void *user);
static void ncm_song_menu_item_destroy(void *item, void *user);
static void ncm_mutable_song_menu_item_init(void *item, void *user);
static void ncm_mutable_song_menu_item_copy(void *dest, void *source,
                                            void *user);
static void ncm_mutable_song_menu_item_destroy(void *item, void *user);
static void ncm_mpd_item_menu_item_init(void *item, void *user);
static void ncm_mpd_item_menu_item_copy(void *dest, void *source,
                                        void *user);
static void ncm_mpd_item_menu_item_destroy(void *item, void *user);
static void ncm_playlist_menu_item_init(void *item, void *user);
static void ncm_playlist_menu_item_copy(void *dest, void *source,
                                        void *user);
static void ncm_playlist_menu_item_destroy(void *item, void *user);
static void nc_search_row_menu_item_init(void *item, void *user);
static void nc_search_row_menu_item_copy(void *dest, void *source,
                                         void *user);
static void nc_search_row_menu_item_destroy(void *item, void *user);
static void nc_media_library_tag_menu_item_init(void *item, void *user);
static void nc_media_library_tag_menu_item_copy(void *dest, void *source,
                                                void *user);
static void nc_media_library_tag_menu_item_destroy(void *item, void *user);
static void nc_media_library_album_menu_item_init(void *item, void *user);
static void nc_media_library_album_menu_item_copy(void *dest, void *source,
                                                  void *user);
static void nc_media_library_album_menu_item_destroy(void *item, void *user);
static void nc_menu_string_item_init(void *item, void *user);
static void nc_menu_string_item_copy(void *dest, void *source, void *user);
static void nc_menu_string_item_destroy(void *item, void *user);
static void nc_menu_string_pair_item_init(void *item, void *user);
static void nc_menu_string_pair_item_copy(void *dest, void *source,
                                          void *user);
static void nc_menu_string_pair_item_destroy(void *item, void *user);
static void nc_editor_action_menu_item_init(void *item, void *user);
static void nc_editor_action_menu_item_copy(void *dest, void *source,
                                            void *user);
static void nc_editor_action_menu_item_destroy(void *item, void *user);
static void nc_editor_sort_menu_item_init(void *item, void *user);
static void nc_editor_sort_menu_item_copy(void *dest, void *source,
                                          void *user);
static void nc_editor_sort_menu_item_destroy(void *item, void *user);
static void nc_buffer_menu_item_init(void *item, void *user);
static void nc_buffer_menu_item_copy(void *dest, void *source, void *user);
static void nc_buffer_menu_item_destroy(void *item, void *user);

static NcMenuItemCallbacks ncm_song_menu_callbacks = {
    .item_size = SIZEOF(NcmSong),
    .construct = ncm_song_menu_item_init,
    .copy = ncm_song_menu_item_copy,
    .destroy = ncm_song_menu_item_destroy,
};
static NcMenuItemCallbacks ncm_mutable_song_menu_callbacks = {
    .item_size = SIZEOF(NcmMutableSong),
    .construct = ncm_mutable_song_menu_item_init,
    .copy = ncm_mutable_song_menu_item_copy,
    .destroy = ncm_mutable_song_menu_item_destroy,
};
static NcMenuItemCallbacks ncm_mpd_item_menu_callbacks = {
    .item_size = SIZEOF(NcmMpdItem),
    .construct = ncm_mpd_item_menu_item_init,
    .copy = ncm_mpd_item_menu_item_copy,
    .destroy = ncm_mpd_item_menu_item_destroy,
};
static NcMenuItemCallbacks ncm_playlist_menu_callbacks = {
    .item_size = SIZEOF(NcmPlaylist),
    .construct = ncm_playlist_menu_item_init,
    .copy = ncm_playlist_menu_item_copy,
    .destroy = ncm_playlist_menu_item_destroy,
};
static NcMenuItemCallbacks nc_search_row_menu_callbacks = {
    .item_size = SIZEOF(NcSearchRow),
    .construct = nc_search_row_menu_item_init,
    .copy = nc_search_row_menu_item_copy,
    .destroy = nc_search_row_menu_item_destroy,
};
static NcMenuItemCallbacks nc_media_library_tag_menu_callbacks = {
    .item_size = SIZEOF(NcMediaLibraryTagRow),
    .construct = nc_media_library_tag_menu_item_init,
    .copy = nc_media_library_tag_menu_item_copy,
    .destroy = nc_media_library_tag_menu_item_destroy,
};
static NcMenuItemCallbacks nc_media_library_album_menu_callbacks = {
    .item_size = SIZEOF(NcMediaLibraryAlbumRow),
    .construct = nc_media_library_album_menu_item_init,
    .copy = nc_media_library_album_menu_item_copy,
    .destroy = nc_media_library_album_menu_item_destroy,
};
static NcMenuItemCallbacks nc_menu_string_callbacks = {
    .item_size = SIZEOF(NcMenuString),
    .construct = nc_menu_string_item_init,
    .copy = nc_menu_string_item_copy,
    .destroy = nc_menu_string_item_destroy,
};
static NcMenuItemCallbacks nc_menu_string_pair_callbacks = {
    .item_size = SIZEOF(NcMenuStringPair),
    .construct = nc_menu_string_pair_item_init,
    .copy = nc_menu_string_pair_item_copy,
    .destroy = nc_menu_string_pair_item_destroy,
};
static NcMenuItemCallbacks nc_editor_action_menu_callbacks = {
    .item_size = SIZEOF(NcEditorActionRow),
    .construct = nc_editor_action_menu_item_init,
    .copy = nc_editor_action_menu_item_copy,
    .destroy = nc_editor_action_menu_item_destroy,
};
static NcMenuItemCallbacks nc_editor_sort_menu_callbacks = {
    .item_size = SIZEOF(NcEditorSortRow),
    .construct = nc_editor_sort_menu_item_init,
    .copy = nc_editor_sort_menu_item_copy,
    .destroy = nc_editor_sort_menu_item_destroy,
};
static NcMenuItemCallbacks nc_buffer_menu_callbacks = {
    .item_size = SIZEOF(NcBuffer),
    .construct = nc_buffer_menu_item_init,
    .copy = nc_buffer_menu_item_copy,
    .destroy = nc_buffer_menu_item_destroy,
};

static void
nc_menu_owned_string_destroy(char **data, int32 *len, int32 *cap) {
    if (*data != NULL) {
        free2(*data, *cap);
    }
    *data = NULL;
    *len = 0;
    *cap = 0;
    return;
}

static bool
nc_menu_owned_string_copy(char **dest_data, int32 *dest_len,
                          int32 *dest_cap, char *source_data,
                          int32 source_len) {
    char *data;
    int32 cap;

    nc_menu_owned_string_destroy(dest_data, dest_len, dest_cap);
    if (source_data == NULL || source_len <= 0) {
        return true;
    }

    cap = source_len + 1;
    data = malloc2(cap);
    memcpy64(data, source_data, source_len);
    data[source_len] = '\0';

    *dest_data = data;
    *dest_len = source_len;
    *dest_cap = cap;
    return true;
}

#define NC_TYPED_MENU_DEFINE_INIT(TYPE_NAME, PREFIX, CALLBACKS) \
    void \
    PREFIX##_init(TYPE_NAME *menu) { \
        nc_menu_init(&menu->menu); \
        nc_menu_set_item_callbacks(&menu->menu, *(CALLBACKS)); \
        return; \
    }

#define NC_TYPED_MENU_DEFINE_DESTROY(TYPE_NAME, PREFIX) \
    void \
    PREFIX##_destroy(TYPE_NAME *menu) { \
        nc_menu_destroy(&menu->menu); \
        return; \
    }

#define NC_TYPED_MENU_DEFINE_BASE(TYPE_NAME, PREFIX) \
    NcMenu * \
    PREFIX##_base(TYPE_NAME *menu) { \
        return &menu->menu; \
    }

#define NC_TYPED_MENU_DEFINE_ADD(TYPE_NAME, PREFIX, ITEM_TYPE) \
    void \
    PREFIX##_add(TYPE_NAME *menu, ITEM_TYPE *item) { \
        nc_menu_add_item(&menu->menu, item); \
        return; \
    }

#define NC_TYPED_MENU_DEFINE_ADD_WITH_FLAGS(TYPE_NAME, PREFIX, ITEM_TYPE) \
    void \
    PREFIX##_add_with_flags(TYPE_NAME *menu, ITEM_TYPE *item, \
                            uint32 flags) { \
        nc_menu_add_item_with_flags(&menu->menu, item, flags); \
        return; \
    }

#define NC_TYPED_MENU_DEFINE_ADD_SEPARATOR(TYPE_NAME, PREFIX) \
    void \
    PREFIX##_add_separator(TYPE_NAME *menu) { \
        nc_menu_add_separator(&menu->menu); \
        return; \
    }

#define NC_TYPED_MENU_DEFINE_INSERT_WITH_FLAGS(TYPE_NAME, PREFIX, ITEM_TYPE) \
    void \
    PREFIX##_insert_with_flags(TYPE_NAME *menu, int64 pos, \
                               ITEM_TYPE *item, uint32 flags) { \
        nc_menu_insert_item_with_flags(&menu->menu, pos, item, flags); \
        return; \
    }

#define NC_TYPED_MENU_DEFINE_ITEM_AT(TYPE_NAME, PREFIX, ITEM_TYPE) \
    ITEM_TYPE * \
    PREFIX##_item_at(TYPE_NAME *menu, enum NcMenuItemSource source, \
                     int64 pos) { \
        return nc_menu_item_at(&menu->menu, source, pos); \
    }

#define NC_TYPED_MENU_DEFINE_CURRENT(TYPE_NAME, PREFIX, ITEM_TYPE) \
    ITEM_TYPE * \
    PREFIX##_current(TYPE_NAME *menu) { \
        return nc_menu_current_item(&menu->menu); \
    }

#define NC_TYPED_MENU_DEFINE_COMMON(TYPE_NAME, PREFIX, CALLBACKS) \
    NC_TYPED_MENU_DEFINE_INIT(TYPE_NAME, PREFIX, CALLBACKS) \
    NC_TYPED_MENU_DEFINE_DESTROY(TYPE_NAME, PREFIX) \
    NC_TYPED_MENU_DEFINE_BASE(TYPE_NAME, PREFIX)

NC_TYPED_MENU_DEFINE_COMMON(NcSongMenu,
                            nc_song_menu,
                            &ncm_song_menu_callbacks)
NC_TYPED_MENU_DEFINE_ADD(NcSongMenu, nc_song_menu, NcmSong)
NC_TYPED_MENU_DEFINE_ITEM_AT(NcSongMenu, nc_song_menu, NcmSong)
NC_TYPED_MENU_DEFINE_CURRENT(NcSongMenu, nc_song_menu, NcmSong)

NC_TYPED_MENU_DEFINE_COMMON(NcBrowserEntryMenu,
                            nc_browser_entry_menu,
                            &ncm_mpd_item_menu_callbacks)
NC_TYPED_MENU_DEFINE_ADD(NcBrowserEntryMenu,
                         nc_browser_entry_menu,
                         NcmMpdItem)
NC_TYPED_MENU_DEFINE_CURRENT(NcBrowserEntryMenu,
                             nc_browser_entry_menu,
                             NcmMpdItem)

NC_TYPED_MENU_DEFINE_COMMON(NcPlaylistEntryMenu,
                            nc_playlist_entry_menu,
                            &ncm_playlist_menu_callbacks)
NC_TYPED_MENU_DEFINE_ADD(NcPlaylistEntryMenu,
                         nc_playlist_entry_menu,
                         NcmPlaylist)
NC_TYPED_MENU_DEFINE_ITEM_AT(NcPlaylistEntryMenu,
                             nc_playlist_entry_menu,
                             NcmPlaylist)
NC_TYPED_MENU_DEFINE_CURRENT(NcPlaylistEntryMenu,
                             nc_playlist_entry_menu,
                             NcmPlaylist)

NC_TYPED_MENU_DEFINE_COMMON(NcTagRowMenu,
                            nc_tag_row_menu,
                            &ncm_mutable_song_menu_callbacks)
NC_TYPED_MENU_DEFINE_ADD(NcTagRowMenu,
                         nc_tag_row_menu,
                         NcmMutableSong)
NC_TYPED_MENU_DEFINE_CURRENT(NcTagRowMenu,
                             nc_tag_row_menu,
                             NcmMutableSong)

NC_TYPED_MENU_DEFINE_COMMON(NcSearchRowMenu,
                            nc_search_row_menu,
                            &nc_search_row_menu_callbacks)
NC_TYPED_MENU_DEFINE_ADD_WITH_FLAGS(NcSearchRowMenu,
                                    nc_search_row_menu,
                                    NcSearchRow)
NC_TYPED_MENU_DEFINE_INSERT_WITH_FLAGS(NcSearchRowMenu,
                                       nc_search_row_menu,
                                       NcSearchRow)
NC_TYPED_MENU_DEFINE_ITEM_AT(NcSearchRowMenu,
                             nc_search_row_menu,
                             NcSearchRow)
NC_TYPED_MENU_DEFINE_CURRENT(NcSearchRowMenu,
                             nc_search_row_menu,
                             NcSearchRow)

NC_TYPED_MENU_DEFINE_COMMON(NcMediaLibraryTagMenu,
                            nc_media_library_tag_menu,
                            &nc_media_library_tag_menu_callbacks)
NC_TYPED_MENU_DEFINE_ADD(NcMediaLibraryTagMenu,
                         nc_media_library_tag_menu,
                         NcMediaLibraryTagRow)
NC_TYPED_MENU_DEFINE_ITEM_AT(NcMediaLibraryTagMenu,
                             nc_media_library_tag_menu,
                             NcMediaLibraryTagRow)
NC_TYPED_MENU_DEFINE_CURRENT(NcMediaLibraryTagMenu,
                             nc_media_library_tag_menu,
                             NcMediaLibraryTagRow)

NC_TYPED_MENU_DEFINE_COMMON(NcMediaLibraryAlbumMenu,
                            nc_media_library_album_menu,
                            &nc_media_library_album_menu_callbacks)
NC_TYPED_MENU_DEFINE_ADD(NcMediaLibraryAlbumMenu,
                         nc_media_library_album_menu,
                         NcMediaLibraryAlbumRow)
NC_TYPED_MENU_DEFINE_ADD_WITH_FLAGS(NcMediaLibraryAlbumMenu,
                                    nc_media_library_album_menu,
                                    NcMediaLibraryAlbumRow)
NC_TYPED_MENU_DEFINE_ITEM_AT(NcMediaLibraryAlbumMenu,
                             nc_media_library_album_menu,
                             NcMediaLibraryAlbumRow)
NC_TYPED_MENU_DEFINE_CURRENT(NcMediaLibraryAlbumMenu,
                             nc_media_library_album_menu,
                             NcMediaLibraryAlbumRow)

NC_TYPED_MENU_DEFINE_COMMON(NcMediaLibrarySongMenu,
                            nc_media_library_song_menu,
                            &ncm_song_menu_callbacks)
NC_TYPED_MENU_DEFINE_ADD(NcMediaLibrarySongMenu,
                         nc_media_library_song_menu,
                         NcmSong)
NC_TYPED_MENU_DEFINE_ITEM_AT(NcMediaLibrarySongMenu,
                             nc_media_library_song_menu,
                             NcmSong)
NC_TYPED_MENU_DEFINE_CURRENT(NcMediaLibrarySongMenu,
                             nc_media_library_song_menu,
                             NcmSong)

NC_TYPED_MENU_DEFINE_COMMON(NcEditorStringMenu,
                            nc_editor_string_menu,
                            &nc_menu_string_callbacks)
NC_TYPED_MENU_DEFINE_ADD_WITH_FLAGS(NcEditorStringMenu,
                                    nc_editor_string_menu,
                                    NcMenuString)
NC_TYPED_MENU_DEFINE_ADD_SEPARATOR(NcEditorStringMenu,
                                   nc_editor_string_menu)

NC_TYPED_MENU_DEFINE_COMMON(NcEditorPairMenu,
                            nc_editor_pair_menu,
                            &nc_menu_string_pair_callbacks)
NC_TYPED_MENU_DEFINE_ADD(NcEditorPairMenu,
                         nc_editor_pair_menu,
                         NcMenuStringPair)
NC_TYPED_MENU_DEFINE_CURRENT(NcEditorPairMenu,
                             nc_editor_pair_menu,
                             NcMenuStringPair)

NC_TYPED_MENU_DEFINE_COMMON(NcEditorActionMenu,
                            nc_editor_action_menu,
                            &nc_editor_action_menu_callbacks)
NC_TYPED_MENU_DEFINE_ADD(NcEditorActionMenu,
                         nc_editor_action_menu,
                         NcEditorActionRow)
NC_TYPED_MENU_DEFINE_ADD_SEPARATOR(NcEditorActionMenu,
                                   nc_editor_action_menu)
NC_TYPED_MENU_DEFINE_ITEM_AT(NcEditorActionMenu,
                             nc_editor_action_menu,
                             NcEditorActionRow)

NC_TYPED_MENU_DEFINE_COMMON(NcEditorSortMenu,
                            nc_editor_sort_menu,
                            &nc_editor_sort_menu_callbacks)
NC_TYPED_MENU_DEFINE_ADD(NcEditorSortMenu,
                         nc_editor_sort_menu,
                         NcEditorSortRow)
NC_TYPED_MENU_DEFINE_ADD_SEPARATOR(NcEditorSortMenu,
                                   nc_editor_sort_menu)
NC_TYPED_MENU_DEFINE_ITEM_AT(NcEditorSortMenu,
                             nc_editor_sort_menu,
                             NcEditorSortRow)
NC_TYPED_MENU_DEFINE_CURRENT(NcEditorSortMenu,
                             nc_editor_sort_menu,
                             NcEditorSortRow)

NC_TYPED_MENU_DEFINE_COMMON(NcEditorBufferMenu,
                            nc_editor_buffer_menu,
                            &nc_buffer_menu_callbacks)
NC_TYPED_MENU_DEFINE_ADD_WITH_FLAGS(NcEditorBufferMenu,
                                    nc_editor_buffer_menu,
                                    NcBuffer)
NC_TYPED_MENU_DEFINE_ADD_SEPARATOR(NcEditorBufferMenu,
                                   nc_editor_buffer_menu)
NC_TYPED_MENU_DEFINE_ITEM_AT(NcEditorBufferMenu,
                             nc_editor_buffer_menu,
                             NcBuffer)

#undef NC_TYPED_MENU_DEFINE_INIT
#undef NC_TYPED_MENU_DEFINE_DESTROY
#undef NC_TYPED_MENU_DEFINE_BASE
#undef NC_TYPED_MENU_DEFINE_ADD
#undef NC_TYPED_MENU_DEFINE_ADD_WITH_FLAGS
#undef NC_TYPED_MENU_DEFINE_ADD_SEPARATOR
#undef NC_TYPED_MENU_DEFINE_INSERT_WITH_FLAGS
#undef NC_TYPED_MENU_DEFINE_ITEM_AT
#undef NC_TYPED_MENU_DEFINE_CURRENT
#undef NC_TYPED_MENU_DEFINE_COMMON

void
nc_menu_string_init(NcMenuString *string) {
    string->data = NULL;
    string->len = 0;
    string->cap = 0;
    return;
}

void
nc_menu_string_destroy(NcMenuString *string) {
    if (string == NULL) {
        return;
    }
    nc_menu_owned_string_destroy(&string->data, &string->len,
                                 &string->cap);
    return;
}

bool
nc_menu_string_copy(NcMenuString *dest, NcMenuString *source) {
    if (dest == NULL || source == NULL) {
        return false;
    }
    return nc_menu_owned_string_copy(&dest->data, &dest->len, &dest->cap,
                                     source->data, source->len);
}

bool
nc_menu_string_set(NcMenuString *string, char *data, int32 data_len) {
    if (string == NULL) {
        return false;
    }
    return nc_menu_owned_string_copy(&string->data, &string->len,
                                     &string->cap, data, data_len);
}

void
nc_menu_string_pair_init(NcMenuStringPair *pair) {
    pair->first = NULL;
    pair->second = NULL;
    pair->first_len = 0;
    pair->first_cap = 0;
    pair->second_len = 0;
    pair->second_cap = 0;
    return;
}

void
nc_menu_string_pair_destroy(NcMenuStringPair *pair) {
    if (pair == NULL) {
        return;
    }
    nc_menu_owned_string_destroy(&pair->first, &pair->first_len,
                                 &pair->first_cap);
    nc_menu_owned_string_destroy(&pair->second, &pair->second_len,
                                 &pair->second_cap);
    return;
}

bool
nc_menu_string_pair_copy(NcMenuStringPair *dest,
                         NcMenuStringPair *source) {
    NcMenuStringPair tmp;

    if (dest == NULL || source == NULL) {
        return false;
    }

    nc_menu_string_pair_init(&tmp);
    if (!nc_menu_owned_string_copy(&tmp.first, &tmp.first_len,
                                   &tmp.first_cap, source->first,
                                   source->first_len)) {
        nc_menu_string_pair_destroy(&tmp);
        return false;
    }
    if (!nc_menu_owned_string_copy(&tmp.second, &tmp.second_len,
                                   &tmp.second_cap, source->second,
                                   source->second_len)) {
        nc_menu_string_pair_destroy(&tmp);
        return false;
    }

    nc_menu_string_pair_destroy(dest);
    *dest = tmp;
    return true;
}

void
nc_search_row_init(NcSearchRow *row) {
    ncm_song_init(&row->song);
    nc_buffer_init(&row->buffer);
    row->is_song = false;
    return;
}

void
nc_search_row_destroy(NcSearchRow *row) {
    if (row == NULL) {
        return;
    }
    ncm_song_destroy(&row->song);
    nc_buffer_destroy(&row->buffer);
    nc_search_row_init(row);
    return;
}

bool
nc_search_row_copy(NcSearchRow *dest, NcSearchRow *source) {
    NcSearchRow tmp;

    if (dest == NULL || source == NULL) {
        return false;
    }

    nc_search_row_init(&tmp);
    tmp.is_song = source->is_song;
    if (!ncm_song_copy(&tmp.song, &source->song)) {
        nc_search_row_destroy(&tmp);
        return false;
    }
    nc_buffer_copy(&tmp.buffer, &source->buffer);

    nc_search_row_destroy(dest);
    *dest = tmp;
    return true;
}

void
nc_media_library_tag_row_init(NcMediaLibraryTagRow *row) {
    row->tag = NULL;
    row->tag_len = 0;
    row->tag_cap = 0;
    row->mtime = 0;
    return;
}

void
nc_media_library_tag_row_destroy(NcMediaLibraryTagRow *row) {
    if (row == NULL) {
        return;
    }
    nc_menu_owned_string_destroy(&row->tag, &row->tag_len,
                                 &row->tag_cap);
    row->mtime = 0;
    return;
}

bool
nc_media_library_tag_row_copy(NcMediaLibraryTagRow *dest,
                              NcMediaLibraryTagRow *source) {
    NcMediaLibraryTagRow tmp;

    if (dest == NULL || source == NULL) {
        return false;
    }

    nc_media_library_tag_row_init(&tmp);
    if (!nc_menu_owned_string_copy(&tmp.tag, &tmp.tag_len, &tmp.tag_cap,
                                   source->tag, source->tag_len)) {
        nc_media_library_tag_row_destroy(&tmp);
        return false;
    }
    tmp.mtime = source->mtime;

    nc_media_library_tag_row_destroy(dest);
    *dest = tmp;
    return true;
}

void
nc_media_library_album_row_init(NcMediaLibraryAlbumRow *row) {
    row->tag = NULL;
    row->album = NULL;
    row->date = NULL;
    row->tag_len = 0;
    row->tag_cap = 0;
    row->album_len = 0;
    row->album_cap = 0;
    row->date_len = 0;
    row->date_cap = 0;
    row->mtime = 0;
    row->all_tracks_entry = false;
    return;
}

void
nc_media_library_album_row_destroy(NcMediaLibraryAlbumRow *row) {
    if (row == NULL) {
        return;
    }
    nc_menu_owned_string_destroy(&row->tag, &row->tag_len,
                                 &row->tag_cap);
    nc_menu_owned_string_destroy(&row->album, &row->album_len,
                                 &row->album_cap);
    nc_menu_owned_string_destroy(&row->date, &row->date_len,
                                 &row->date_cap);
    row->mtime = 0;
    row->all_tracks_entry = false;
    return;
}

bool
nc_media_library_album_row_copy(NcMediaLibraryAlbumRow *dest,
                                NcMediaLibraryAlbumRow *source) {
    NcMediaLibraryAlbumRow tmp;

    if (dest == NULL || source == NULL) {
        return false;
    }

    nc_media_library_album_row_init(&tmp);
    if (!nc_menu_owned_string_copy(&tmp.tag, &tmp.tag_len, &tmp.tag_cap,
                                   source->tag, source->tag_len)) {
        nc_media_library_album_row_destroy(&tmp);
        return false;
    }
    if (!nc_menu_owned_string_copy(&tmp.album, &tmp.album_len,
                                   &tmp.album_cap, source->album,
                                   source->album_len)) {
        nc_media_library_album_row_destroy(&tmp);
        return false;
    }
    if (!nc_menu_owned_string_copy(&tmp.date, &tmp.date_len, &tmp.date_cap,
                                   source->date, source->date_len)) {
        nc_media_library_album_row_destroy(&tmp);
        return false;
    }
    tmp.mtime = source->mtime;
    tmp.all_tracks_entry = source->all_tracks_entry;

    nc_media_library_album_row_destroy(dest);
    *dest = tmp;
    return true;
}

void
nc_editor_action_row_init(NcEditorActionRow *row) {
    row->label = NULL;
    row->label_len = 0;
    row->label_cap = 0;
    row->run = NULL;
    row->user = NULL;
    return;
}

void
nc_editor_action_row_destroy(NcEditorActionRow *row) {
    if (row == NULL) {
        return;
    }
    nc_menu_owned_string_destroy(&row->label, &row->label_len,
                                 &row->label_cap);
    row->run = NULL;
    row->user = NULL;
    return;
}

bool
nc_editor_action_row_copy(NcEditorActionRow *dest,
                          NcEditorActionRow *source) {
    NcEditorActionRow tmp;

    if (dest == NULL || source == NULL) {
        return false;
    }

    nc_editor_action_row_init(&tmp);
    if (!nc_menu_owned_string_copy(&tmp.label, &tmp.label_len,
                                   &tmp.label_cap, source->label,
                                   source->label_len)) {
        nc_editor_action_row_destroy(&tmp);
        return false;
    }
    tmp.run = source->run;
    tmp.user = source->user;

    nc_editor_action_row_destroy(dest);
    *dest = tmp;
    return true;
}

void
nc_editor_sort_row_init(NcEditorSortRow *row) {
    nc_editor_action_row_init(&row->action);
    row->getter = NCM_SONG_GETTER_NONE;
    return;
}

void
nc_editor_sort_row_destroy(NcEditorSortRow *row) {
    if (row == NULL) {
        return;
    }
    nc_editor_action_row_destroy(&row->action);
    row->getter = NCM_SONG_GETTER_NONE;
    return;
}

bool
nc_editor_sort_row_copy(NcEditorSortRow *dest, NcEditorSortRow *source) {
    NcEditorSortRow tmp;

    if (dest == NULL || source == NULL) {
        return false;
    }

    nc_editor_sort_row_init(&tmp);
    if (!nc_editor_action_row_copy(&tmp.action, &source->action)) {
        nc_editor_sort_row_destroy(&tmp);
        return false;
    }
    tmp.getter = source->getter;

    nc_editor_sort_row_destroy(dest);
    *dest = tmp;
    return true;
}

static void
ncm_song_menu_item_init(void *item, void *user) {
    (void)user;
    ncm_song_init(item);
    return;
}

static void
ncm_song_menu_item_copy(void *dest, void *source, void *user) {
    (void)user;
    ncm_song_copy(dest, source);
    return;
}

static void
ncm_song_menu_item_destroy(void *item, void *user) {
    (void)user;
    ncm_song_destroy(item);
    return;
}

static void
ncm_mutable_song_menu_item_init(void *item, void *user) {
    (void)user;
    ncm_mutable_song_init(item);
    return;
}

static void
ncm_mutable_song_menu_item_copy(void *dest, void *source, void *user) {
    (void)user;
    ncm_mutable_song_copy(dest, source);
    return;
}

static void
ncm_mutable_song_menu_item_destroy(void *item, void *user) {
    (void)user;
    ncm_mutable_song_destroy(item);
    return;
}

static void
ncm_mpd_item_menu_item_init(void *item, void *user) {
    (void)user;
    ncm_mpd_item_init(item);
    return;
}

static void
ncm_mpd_item_menu_item_copy(void *dest, void *source, void *user) {
    (void)user;
    ncm_mpd_item_copy(dest, source);
    return;
}

static void
ncm_mpd_item_menu_item_destroy(void *item, void *user) {
    (void)user;
    ncm_mpd_item_destroy(item);
    return;
}

static void
ncm_playlist_menu_item_init(void *item, void *user) {
    (void)user;
    ncm_playlist_init(item);
    return;
}

static void
ncm_playlist_menu_item_copy(void *dest, void *source, void *user) {
    (void)user;
    ncm_playlist_copy(dest, source);
    return;
}

static void
ncm_playlist_menu_item_destroy(void *item, void *user) {
    (void)user;
    ncm_playlist_destroy(item);
    return;
}

static void
nc_search_row_menu_item_init(void *item, void *user) {
    (void)user;
    nc_search_row_init(item);
    return;
}

static void
nc_search_row_menu_item_copy(void *dest, void *source, void *user) {
    (void)user;
    nc_search_row_copy(dest, source);
    return;
}

static void
nc_search_row_menu_item_destroy(void *item, void *user) {
    (void)user;
    nc_search_row_destroy(item);
    return;
}

static void
nc_media_library_tag_menu_item_init(void *item, void *user) {
    (void)user;
    nc_media_library_tag_row_init(item);
    return;
}

static void
nc_media_library_tag_menu_item_copy(void *dest, void *source, void *user) {
    (void)user;
    nc_media_library_tag_row_copy(dest, source);
    return;
}

static void
nc_media_library_tag_menu_item_destroy(void *item, void *user) {
    (void)user;
    nc_media_library_tag_row_destroy(item);
    return;
}

static void
nc_media_library_album_menu_item_init(void *item, void *user) {
    (void)user;
    nc_media_library_album_row_init(item);
    return;
}

static void
nc_media_library_album_menu_item_copy(void *dest, void *source,
                                      void *user) {
    (void)user;
    nc_media_library_album_row_copy(dest, source);
    return;
}

static void
nc_media_library_album_menu_item_destroy(void *item, void *user) {
    (void)user;
    nc_media_library_album_row_destroy(item);
    return;
}

static void
nc_menu_string_item_init(void *item, void *user) {
    (void)user;
    nc_menu_string_init(item);
    return;
}

static void
nc_menu_string_item_copy(void *dest, void *source, void *user) {
    (void)user;
    nc_menu_string_copy(dest, source);
    return;
}

static void
nc_menu_string_item_destroy(void *item, void *user) {
    (void)user;
    nc_menu_string_destroy(item);
    return;
}

static void
nc_menu_string_pair_item_init(void *item, void *user) {
    (void)user;
    nc_menu_string_pair_init(item);
    return;
}

static void
nc_menu_string_pair_item_copy(void *dest, void *source, void *user) {
    (void)user;
    nc_menu_string_pair_copy(dest, source);
    return;
}

static void
nc_menu_string_pair_item_destroy(void *item, void *user) {
    (void)user;
    nc_menu_string_pair_destroy(item);
    return;
}

static void
nc_editor_action_menu_item_init(void *item, void *user) {
    (void)user;
    nc_editor_action_row_init(item);
    return;
}

static void
nc_editor_action_menu_item_copy(void *dest, void *source, void *user) {
    (void)user;
    nc_editor_action_row_copy(dest, source);
    return;
}

static void
nc_editor_action_menu_item_destroy(void *item, void *user) {
    (void)user;
    nc_editor_action_row_destroy(item);
    return;
}

static void
nc_editor_sort_menu_item_init(void *item, void *user) {
    (void)user;
    nc_editor_sort_row_init(item);
    return;
}

static void
nc_editor_sort_menu_item_copy(void *dest, void *source, void *user) {
    (void)user;
    nc_editor_sort_row_copy(dest, source);
    return;
}

static void
nc_editor_sort_menu_item_destroy(void *item, void *user) {
    (void)user;
    nc_editor_sort_row_destroy(item);
    return;
}

static void
nc_buffer_menu_item_init(void *item, void *user) {
    (void)user;
    nc_buffer_init(item);
    return;
}

static void
nc_buffer_menu_item_copy(void *dest, void *source, void *user) {
    (void)user;
    nc_buffer_copy(dest, source);
    return;
}

static void
nc_buffer_menu_item_destroy(void *item, void *user) {
    (void)user;
    nc_buffer_destroy(item);
    return;
}
