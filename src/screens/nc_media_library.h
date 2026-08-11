#if !defined(NCMPCPP_NC_MEDIA_LIBRARY_H)
#define NCMPCPP_NC_MEDIA_LIBRARY_H

#include "cbase.h"

#include "c/ncm_app_arrays.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_regex.h"
#include "c/ncm_time.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#define MEDIA_LIBRARY_FETCH_DELAY_MS 250

#define ENUM_NAME MediaLibraryMode
#define ENUM_PREFIX_ MEDIA_LIBRARY_MODE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(MEDIA_LIBRARY_MODE_THREE_COLUMNS) \
    X(MEDIA_LIBRARY_MODE_TWO_COLUMNS) \
    X(MEDIA_LIBRARY_MODE_ALBUM_ONLY)
#include "cbase/xenums.c"

#define ENUM_NAME MediaLibraryColumn
#define ENUM_PREFIX_ MEDIA_LIBRARY_COLUMN_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(MEDIA_LIBRARY_COLUMN_TAGS) \
    X(MEDIA_LIBRARY_COLUMN_ALBUMS) \
    X(MEDIA_LIBRARY_COLUMN_SONGS)
#include "cbase/xenums.c"

typedef struct MediaLibraryAlbumItem {
    NcMediaLibraryAlbumRow row;
    uint32 menu_flags;
} MediaLibraryAlbumItem;

NCM_ARRAY_DECLARE_TYPE(MediaLibraryTagArray,
                       NcMediaLibraryTagRow)
NCM_ARRAY_DECLARE_INIT(media_library_tag_array,
                       MediaLibraryTagArray)
NCM_ARRAY_DECLARE_CLEAR(media_library_tag_array,
                        MediaLibraryTagArray)
NCM_ARRAY_DECLARE_DESTROY(media_library_tag_array,
                          MediaLibraryTagArray)
NCM_ARRAY_DECLARE_MOVE(media_library_tag_array,
                       MediaLibraryTagArray)
NCM_ARRAY_DECLARE_RESERVE(media_library_tag_array,
                          MediaLibraryTagArray)
NCM_ARRAY_DECLARE_APPEND(media_library_tag_array,
                         MediaLibraryTagArray,
                         NcMediaLibraryTagRow)
NCM_ARRAY_DECLARE_REMOVE_ORDERED(media_library_tag_array,
                                 MediaLibraryTagArray)

NCM_ARRAY_DECLARE_TYPE(MediaLibraryAlbumArray,
                       MediaLibraryAlbumItem)
NCM_ARRAY_DECLARE_INIT(media_library_album_array,
                       MediaLibraryAlbumArray)
NCM_ARRAY_DECLARE_CLEAR(media_library_album_array,
                        MediaLibraryAlbumArray)
NCM_ARRAY_DECLARE_DESTROY(media_library_album_array,
                          MediaLibraryAlbumArray)
NCM_ARRAY_DECLARE_MOVE(media_library_album_array,
                       MediaLibraryAlbumArray)
NCM_ARRAY_DECLARE_RESERVE(media_library_album_array,
                          MediaLibraryAlbumArray)
NCM_ARRAY_DECLARE_APPEND(media_library_album_array,
                         MediaLibraryAlbumArray,
                         MediaLibraryAlbumItem)
NCM_ARRAY_DECLARE_REMOVE_ORDERED(media_library_album_array,
                                 MediaLibraryAlbumArray)

typedef struct MediaLibrarySongQuery {
    char *primary_value;
    char *album;
    char *date;

    int32 primary_value_len;
    int32 album_len;
    int32 date_len;

    enum mpd_tag_type primary_tag;
    bool match_primary_tag;
    bool match_album;
    bool match_date;
} MediaLibrarySongQuery;

typedef struct MediaLibraryColumnState {
    StrBuilder filter_constraint;
    StrBuilder search_constraint;
    NcmRegex filter_regex;
    NcmRegex search_regex;
    bool filter_enabled;
    bool search_enabled;
} MediaLibraryColumnState;

typedef struct MediaLibraryHooks {
    bool (*list_tags)(void *user, enum mpd_tag_type tag_type,
                      NcmMpdStringList *tags, NcmError *ncm_error);
    bool (*list_all_songs)(void *user, NcmMpdSongList *songs,
                           NcmError *ncm_error);
    bool (*search_songs)(void *user,
                         MediaLibrarySongQuery *query,
                         NcmMpdSongList *songs, NcmError *ncm_error);
    bool (*add_songs)(void *user, NcmSongArray *songs, bool play,
                      NcmError *ncm_error);
    void (*destroy)(void *user);
    void *user;
} MediaLibraryHooks;

typedef struct MediaLibraryScreen {
    NcScreen screen;
    NcMediaLibraryTagMenu tags;
    NcMediaLibraryAlbumMenu albums;
    NcMediaLibrarySongMenu songs;
    NcWindow tags_window;
    NcWindow albums_window;
    NcWindow songs_window;
    MediaLibraryHooks hooks;

    MediaLibraryColumnState column_state[
        MEDIA_LIBRARY_COLUMN_LAST];
    StrBuilder tags_title;
    StrBuilder albums_title;
    StrBuilder songs_title;
    NcmTimePoint update_timer;
    NcMediaLibraryTagRow observed_tag;
    NcMediaLibraryAlbumRow observed_album;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;
    int32 fetching_delay_ms;
    int32 window_timeout_ms;

    enum MediaLibraryMode mode;
    enum MediaLibraryColumn active_column;

    bool tags_update_request;
    bool albums_update_request;
    bool songs_update_request;
    bool sort_by_mtime;
    bool observed_tag_valid;
    bool observed_album_valid;
    bool registered;
} MediaLibraryScreen;

MediaLibraryHooks media_library_mpd_hooks(
    NcmMpdClient *client);
void media_library_screen_init(MediaLibraryScreen *screen,
                                      MediaLibraryHooks hooks,
                                      int32 start_x, int32 width,
                                      int32 main_start_y,
                                      int32 main_height, NcColor color,
                                      NcBorder border);
void media_library_screen_destroy(MediaLibraryScreen *screen);
NcScreen *media_library_screen_base(MediaLibraryScreen *screen);
NcMenu *media_library_screen_active_menu(
    MediaLibraryScreen *screen);
NcWindow *media_library_screen_active_window(
    MediaLibraryScreen *screen);
void media_library_screen_set_geometry(
    MediaLibraryScreen *screen, int32 start_x, int32 width,
    int32 main_start_y, int32 main_height);

int32 media_library_screen_column_count(
    MediaLibraryScreen *screen);
bool media_library_screen_set_mode(
    MediaLibraryScreen *screen, enum MediaLibraryMode mode);
enum MediaLibraryMode media_library_screen_toggle_mode(
    MediaLibraryScreen *screen);
enum MediaLibraryColumn media_library_screen_active_column(
    MediaLibraryScreen *screen);
bool media_library_screen_item_available(
    MediaLibraryScreen *screen);
bool media_library_screen_set_active_column(
    MediaLibraryScreen *screen,
    enum MediaLibraryColumn column);
bool media_library_screen_column_visible(
    MediaLibraryScreen *screen,
    enum MediaLibraryColumn column);
MediaLibraryColumnState *media_library_screen_column_state(
    MediaLibraryScreen *screen,
    enum MediaLibraryColumn column);
StrBuilder *media_library_screen_active_filter_constraint(
    MediaLibraryScreen *screen);
StrBuilder *media_library_screen_active_search_constraint(
    MediaLibraryScreen *screen);
NcMediaLibraryTagRow *media_library_screen_current_tag(
    MediaLibraryScreen *screen);
NcMediaLibraryAlbumRow *media_library_screen_current_album(
    MediaLibraryScreen *screen);
bool media_library_screen_current_primary_tag_value(
    MediaLibraryScreen *screen, char **value, int32 *value_len);
bool media_library_screen_current_album_value(
    MediaLibraryScreen *screen, char **album, int32 *album_len);
void media_library_screen_format_tag_row(
    MediaLibraryScreen *screen, NcMediaLibraryTagRow *row,
    StrBuilder *output);
void media_library_screen_format_album_row(
    MediaLibraryScreen *screen, NcMediaLibraryAlbumRow *row,
    StrBuilder *output);
void media_library_screen_format_song_row(
    MediaLibraryScreen *screen, NcmSong *song, NcBuffer *output);

bool media_library_tags_from_strings(
    MediaLibraryTagArray *tags, NcmMpdStringList *strings);
bool media_library_tags_from_songs(
    MediaLibraryTagArray *tags, NcmMpdSongList *songs,
    enum mpd_tag_type primary_tag);
bool media_library_albums_from_songs(
    MediaLibraryAlbumArray *albums, NcmMpdSongList *songs,
    enum MediaLibraryMode mode, enum mpd_tag_type primary_tag,
    char *selected_tag, int32 selected_tag_len);
bool media_library_songs_from_list(
    NcmSongArray *songs, NcmMpdSongList *source);

bool media_library_screen_toggle_sort_mode(
    MediaLibraryScreen *screen);
bool media_library_screen_set_primary_tag_type(
    MediaLibraryScreen *screen, enum mpd_tag_type tag_type);
void media_library_screen_request_database_update(
    MediaLibraryScreen *screen);
bool media_library_screen_refresh_inactive_songs(
    MediaLibraryScreen *screen);

bool media_library_screen_previous_column_available(
    MediaLibraryScreen *screen);
bool media_library_screen_next_column_available(
    MediaLibraryScreen *screen);
void media_library_screen_previous_column(
    MediaLibraryScreen *screen);
void media_library_screen_next_column(MediaLibraryScreen *screen);
void media_library_screen_clear(MediaLibraryScreen *screen);
bool media_library_screen_current_song(
    MediaLibraryScreen *screen, NcmSong *song);
bool media_library_screen_selected_songs(
    MediaLibraryScreen *screen, NcmSongArray *songs);
bool media_library_screen_selected_songs_checked(
    MediaLibraryScreen *screen, NcmSongArray *songs, NcmError *ncm_error);
bool media_library_screen_copy_visible_songs(
    MediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *ncm_error);
bool media_library_screen_apply_filter(
    MediaLibraryScreen *screen, char *pattern, int32 pattern_len,
    NcmError *ncm_error);
void media_library_screen_clear_filter(
    MediaLibraryScreen *screen);
bool media_library_screen_search(
    MediaLibraryScreen *screen, char *pattern, int32 pattern_len,
    bool forward, bool wrap, bool skip_current, NcmError *ncm_error);
void media_library_screen_clear_search(
    MediaLibraryScreen *screen);
void media_library_screen_request_tags_update(
    MediaLibraryScreen *screen);
void media_library_screen_request_albums_update(
    MediaLibraryScreen *screen);
void media_library_screen_request_songs_update(
    MediaLibraryScreen *screen);
void media_library_screen_finish_list_change(
    MediaLibraryScreen *screen);
bool media_library_screen_update(
    MediaLibraryScreen *screen, NcmError *ncm_error);

bool media_library_screen_list_tags(
    MediaLibraryScreen *screen, enum mpd_tag_type tag_type,
    NcmMpdStringList *tags, NcmError *ncm_error);
bool media_library_screen_list_all_songs(
    MediaLibraryScreen *screen, NcmMpdSongList *songs,
    NcmError *ncm_error);
bool media_library_screen_search_songs(
    MediaLibraryScreen *screen,
    MediaLibrarySongQuery *query, NcmMpdSongList *songs,
    NcmError *ncm_error);
bool media_library_screen_add_songs(
    MediaLibraryScreen *screen, NcmSongArray *songs, bool play,
    NcmError *ncm_error);
bool media_library_screen_add_item_to_playlist(
    MediaLibraryScreen *screen, bool play, NcmError *ncm_error);
bool media_library_screen_locate_song(
    MediaLibraryScreen *screen, NcmSong *song, NcmError *ncm_error);

#endif /* NCMPCPP_NC_MEDIA_LIBRARY_H */
