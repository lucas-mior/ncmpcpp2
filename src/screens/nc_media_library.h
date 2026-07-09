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

enum NativeMediaLibraryColumn {
    NATIVE_MEDIA_LIBRARY_COLUMN_TAGS,
    NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS,
    NATIVE_MEDIA_LIBRARY_COLUMN_SONGS,
};

typedef struct NativeMediaLibraryScreen {
    NcScreen screen;
    NcMediaLibraryTagMenu tags;
    NcMediaLibraryAlbumMenu albums;
    NcMediaLibrarySongMenu songs;
    NcWindow tags_window;
    NcWindow albums_window;
    NcWindow songs_window;
    NcmBuffer tag_filter_constraint;
    NcmBuffer album_filter_constraint;
    NcmBuffer song_filter_constraint;
    NcmBuffer tag_search_constraint;
    NcmBuffer album_search_constraint;
    NcmBuffer song_search_constraint;
    NcmRegex tag_filter_regex;
    NcmRegex album_filter_regex;
    NcmRegex song_filter_regex;
    NcmTimePoint timer;

    int64 start_x;
    int64 width;
    int64 main_start_y;
    int64 main_height;
    int64 columns;
    int64 active_column;

    bool tags_update_request;
    bool albums_update_request;
    bool songs_update_request;
    bool filter_enabled;
    bool registered;
} NativeMediaLibraryScreen;

void native_media_library_screen_init(NativeMediaLibraryScreen *screen,
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
int64 native_media_library_screen_columns(NativeMediaLibraryScreen *screen);
void native_media_library_screen_set_columns(NativeMediaLibraryScreen *screen,
                                             int64 columns);
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
bool native_media_library_screen_selected_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs);
bool native_media_library_screen_apply_filter(
    NativeMediaLibraryScreen *screen, char *pattern, int32 pattern_len,
    NcmError *error);
void native_media_library_screen_clear_filter(
    NativeMediaLibraryScreen *screen);
bool native_media_library_screen_search(
    NativeMediaLibraryScreen *screen, char *pattern, int32 pattern_len,
    bool forward, bool wrap, bool skip_current, NcmError *error);
void native_media_library_screen_request_tags_update(
    NativeMediaLibraryScreen *screen);
void native_media_library_screen_request_albums_update(
    NativeMediaLibraryScreen *screen);
void native_media_library_screen_request_songs_update(
    NativeMediaLibraryScreen *screen);
void native_media_library_screen_clear_update_requests(
    NativeMediaLibraryScreen *screen);
bool native_media_library_screen_locate_song(NativeMediaLibraryScreen *screen,
                                             NcmSong *song);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_MEDIA_LIBRARY_H */
