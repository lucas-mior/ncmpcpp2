#if !defined(NCMPCPP_NC_MEDIA_LIBRARY_H)
#define NCMPCPP_NC_MEDIA_LIBRARY_H

#include <stdbool.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_regex.h"
#include "c/ncm_time.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define NATIVE_MEDIA_LIBRARY_FETCH_DELAY_MS 250

enum NativeMediaLibraryMode {
    NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS,
    NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS,
    NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY,
    NATIVE_MEDIA_LIBRARY_MODE_LAST,
};

enum NativeMediaLibraryColumn {
    NATIVE_MEDIA_LIBRARY_COLUMN_TAGS,
    NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS,
    NATIVE_MEDIA_LIBRARY_COLUMN_SONGS,
    NATIVE_MEDIA_LIBRARY_COLUMN_LAST,
};

typedef struct NativeMediaLibraryAlbumItem {
    NcMediaLibraryAlbumRow row;
    uint32 menu_flags;
} NativeMediaLibraryAlbumItem;

NCM_ARRAY_DECLARE(native_media_library_tag_array,
                  NativeMediaLibraryTagArray,
                  NcMediaLibraryTagRow)
NCM_ARRAY_DECLARE(native_media_library_album_array,
                  NativeMediaLibraryAlbumArray,
                  NativeMediaLibraryAlbumItem)

typedef struct NativeMediaLibrarySongQuery {
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
} NativeMediaLibrarySongQuery;

typedef struct NativeMediaLibraryColumnState {
    NcmBuffer filter_constraint;
    NcmBuffer search_constraint;
    NcmRegex filter_regex;
    NcmRegex search_regex;
    bool filter_enabled;
    bool search_enabled;
} NativeMediaLibraryColumnState;

typedef struct NativeMediaLibraryHooks {
    bool (*list_tags)(void *user, enum mpd_tag_type tag_type,
                      NcmMpdStringList *tags, NcmError *error);
    bool (*list_all_songs)(void *user, NcmMpdSongList *songs,
                           NcmError *error);
    bool (*search_songs)(void *user,
                         NativeMediaLibrarySongQuery *query,
                         NcmMpdSongList *songs, NcmError *error);
    bool (*add_songs)(void *user, NcmSongArray *songs, bool play,
                      NcmError *error);
    void (*destroy)(void *user);
    void *user;
} NativeMediaLibraryHooks;

typedef struct NativeMediaLibraryScreen {
    NcScreen screen;
    NcMediaLibraryTagMenu tags;
    NcMediaLibraryAlbumMenu albums;
    NcMediaLibrarySongMenu songs;
    NcWindow tags_window;
    NcWindow albums_window;
    NcWindow songs_window;
    NativeMediaLibraryHooks hooks;

    NativeMediaLibraryColumnState column_state[
        NATIVE_MEDIA_LIBRARY_COLUMN_LAST];
    NcmBuffer tags_title;
    NcmBuffer albums_title;
    NcmBuffer songs_title;
    NcmTimePoint update_timer;
    NcMediaLibraryTagRow observed_tag;
    NcMediaLibraryAlbumRow observed_album;

    int64 start_x;
    int64 width;
    int64 main_start_y;
    int64 main_height;
    int64 fetching_delay_ms;
    int32 window_timeout_ms;

    enum NativeMediaLibraryMode mode;
    enum NativeMediaLibraryColumn active_column;

    bool tags_update_request;
    bool albums_update_request;
    bool songs_update_request;
    bool sort_by_mtime;
    bool observed_tag_valid;
    bool observed_album_valid;
    bool registered;
} NativeMediaLibraryScreen;

NativeMediaLibraryHooks native_media_library_mpd_hooks(
    NcmMpdClient *client);
void native_media_library_screen_init(NativeMediaLibraryScreen *screen,
                                      NativeMediaLibraryHooks hooks,
                                      int64 start_x, int64 width,
                                      int64 main_start_y,
                                      int64 main_height, NcColor color,
                                      NcBorder border);
void native_media_library_screen_destroy(NativeMediaLibraryScreen *screen);
NcScreen *native_media_library_screen_base(NativeMediaLibraryScreen *screen);
NcMediaLibraryTagMenu *native_media_library_screen_tags(
    NativeMediaLibraryScreen *screen);
NcMediaLibraryAlbumMenu *native_media_library_screen_albums(
    NativeMediaLibraryScreen *screen);
NcMediaLibrarySongMenu *native_media_library_screen_songs(
    NativeMediaLibraryScreen *screen);
NcMenu *native_media_library_screen_active_menu(
    NativeMediaLibraryScreen *screen);
NcWindow *native_media_library_screen_active_window(
    NativeMediaLibraryScreen *screen);
void native_media_library_screen_set_geometry(
    NativeMediaLibraryScreen *screen, int64 start_x, int64 width,
    int64 main_start_y, int64 main_height);

enum NativeMediaLibraryMode native_media_library_screen_mode(
    NativeMediaLibraryScreen *screen);
int32 native_media_library_screen_column_count(
    NativeMediaLibraryScreen *screen);
bool native_media_library_screen_set_mode(
    NativeMediaLibraryScreen *screen, enum NativeMediaLibraryMode mode);
enum NativeMediaLibraryMode native_media_library_screen_toggle_mode(
    NativeMediaLibraryScreen *screen);
enum NativeMediaLibraryColumn native_media_library_screen_active_column(
    NativeMediaLibraryScreen *screen);
bool native_media_library_screen_item_available(
    NativeMediaLibraryScreen *screen);
bool native_media_library_screen_set_active_column(
    NativeMediaLibraryScreen *screen,
    enum NativeMediaLibraryColumn column);
bool native_media_library_screen_column_visible(
    NativeMediaLibraryScreen *screen,
    enum NativeMediaLibraryColumn column);
NativeMediaLibraryColumnState *native_media_library_screen_column_state(
    NativeMediaLibraryScreen *screen,
    enum NativeMediaLibraryColumn column);
NcmBuffer *native_media_library_screen_active_filter_constraint(
    NativeMediaLibraryScreen *screen);
NcmBuffer *native_media_library_screen_active_search_constraint(
    NativeMediaLibraryScreen *screen);
NcMediaLibraryTagRow *native_media_library_screen_current_tag(
    NativeMediaLibraryScreen *screen);
NcMediaLibraryAlbumRow *native_media_library_screen_current_album(
    NativeMediaLibraryScreen *screen);
bool native_media_library_screen_current_primary_tag_value(
    NativeMediaLibraryScreen *screen, char **value, int32 *value_len);
bool native_media_library_screen_current_album_value(
    NativeMediaLibraryScreen *screen, char **album, int32 *album_len);
bool native_media_library_screen_current_album_date(
    NativeMediaLibraryScreen *screen, char **date, int32 *date_len);
bool native_media_library_screen_current_album_is_all_tracks(
    NativeMediaLibraryScreen *screen);
int64 native_media_library_screen_visible_song_count(
    NativeMediaLibraryScreen *screen);
NcmSong *native_media_library_screen_visible_song_at(
    NativeMediaLibraryScreen *screen, int64 pos);
void native_media_library_screen_format_tag_row(
    NativeMediaLibraryScreen *screen, NcMediaLibraryTagRow *row,
    NcmBuffer *output);
void native_media_library_screen_format_album_row(
    NativeMediaLibraryScreen *screen, NcMediaLibraryAlbumRow *row,
    NcmBuffer *output);
void native_media_library_screen_format_song_row(
    NativeMediaLibraryScreen *screen, NcmSong *song, NcBuffer *output);

bool native_media_library_tags_from_strings(
    NativeMediaLibraryTagArray *tags, NcmMpdStringList *strings);
bool native_media_library_tags_from_songs(
    NativeMediaLibraryTagArray *tags, NcmMpdSongList *songs,
    enum mpd_tag_type primary_tag);
bool native_media_library_albums_from_songs(
    NativeMediaLibraryAlbumArray *albums, NcmMpdSongList *songs,
    enum NativeMediaLibraryMode mode, enum mpd_tag_type primary_tag,
    char *selected_tag, int32 selected_tag_len);
bool native_media_library_songs_from_list(
    NcmSongArray *songs, NcmMpdSongList *source);

NcmTimePoint native_media_library_screen_update_timer(
    NativeMediaLibraryScreen *screen);
void native_media_library_screen_set_update_timer(
    NativeMediaLibraryScreen *screen, NcmTimePoint timer);
int64 native_media_library_screen_fetching_delay_ms(
    NativeMediaLibraryScreen *screen);
int32 native_media_library_screen_window_timeout_ms(
    NativeMediaLibraryScreen *screen);
bool native_media_library_screen_sort_by_mtime(
    NativeMediaLibraryScreen *screen);
bool native_media_library_screen_toggle_sort_mode(
    NativeMediaLibraryScreen *screen);
bool native_media_library_screen_set_primary_tag_type(
    NativeMediaLibraryScreen *screen, enum mpd_tag_type tag_type);
enum NativeMediaLibraryMode native_media_library_screen_toggle_columns_mode(
    NativeMediaLibraryScreen *screen);
void native_media_library_screen_request_database_update(
    NativeMediaLibraryScreen *screen);
bool native_media_library_screen_refresh_inactive_songs(
    NativeMediaLibraryScreen *screen);

bool native_media_library_screen_previous_column_available(
    NativeMediaLibraryScreen *screen);
bool native_media_library_screen_next_column_available(
    NativeMediaLibraryScreen *screen);
void native_media_library_screen_previous_column(
    NativeMediaLibraryScreen *screen);
void native_media_library_screen_next_column(NativeMediaLibraryScreen *screen);
void native_media_library_screen_clear(NativeMediaLibraryScreen *screen);
bool native_media_library_screen_add_tag(
    NativeMediaLibraryScreen *screen, char *tag, int32 tag_len,
    time_t mtime);
bool native_media_library_screen_add_album(
    NativeMediaLibraryScreen *screen, char *tag, int32 tag_len,
    char *album, int32 album_len, char *date, int32 date_len,
    time_t mtime, bool all_tracks_entry);
bool native_media_library_screen_add_song_copy(
    NativeMediaLibraryScreen *screen, NcmSong *song);
bool native_media_library_screen_group_tags_from_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs,
    enum mpd_tag_type tag_type);
bool native_media_library_screen_current_song(
    NativeMediaLibraryScreen *screen, NcmSong *song);
bool native_media_library_screen_current_selected_song(
    NativeMediaLibraryScreen *screen, NcmSong *song);
bool native_media_library_screen_selected_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs);
bool native_media_library_screen_selected_songs_checked(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs, NcmError *error);
bool native_media_library_screen_current_tag_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *error);
bool native_media_library_screen_current_album_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs, NcmError *error);
bool native_media_library_screen_copy_visible_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *error);
bool native_media_library_screen_apply_filter(
    NativeMediaLibraryScreen *screen, char *pattern, int32 pattern_len,
    NcmError *error);
void native_media_library_screen_clear_filter(
    NativeMediaLibraryScreen *screen);
bool native_media_library_screen_search(
    NativeMediaLibraryScreen *screen, char *pattern, int32 pattern_len,
    bool forward, bool wrap, bool skip_current, NcmError *error);
void native_media_library_screen_clear_search(
    NativeMediaLibraryScreen *screen);
void native_media_library_screen_request_tags_update(
    NativeMediaLibraryScreen *screen);
void native_media_library_screen_request_albums_update(
    NativeMediaLibraryScreen *screen);
void native_media_library_screen_request_songs_update(
    NativeMediaLibraryScreen *screen);
void native_media_library_screen_clear_update_requests(
    NativeMediaLibraryScreen *screen);
void native_media_library_screen_finish_list_change(
    NativeMediaLibraryScreen *screen);
bool native_media_library_screen_update(
    NativeMediaLibraryScreen *screen, NcmError *error);

bool native_media_library_screen_list_tags(
    NativeMediaLibraryScreen *screen, enum mpd_tag_type tag_type,
    NcmMpdStringList *tags, NcmError *error);
bool native_media_library_screen_list_all_songs(
    NativeMediaLibraryScreen *screen, NcmMpdSongList *songs,
    NcmError *error);
bool native_media_library_screen_search_songs(
    NativeMediaLibraryScreen *screen,
    NativeMediaLibrarySongQuery *query, NcmMpdSongList *songs,
    NcmError *error);
bool native_media_library_screen_add_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs, bool play,
    NcmError *error);
bool native_media_library_screen_add_item_to_playlist(
    NativeMediaLibraryScreen *screen, bool play, NcmError *error);
bool native_media_library_screen_locate_song(
    NativeMediaLibraryScreen *screen, NcmSong *song, NcmError *error);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_MEDIA_LIBRARY_H */
