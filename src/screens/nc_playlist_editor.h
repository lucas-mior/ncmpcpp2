#if !defined(NCMPCPP_NC_PLAYLIST_EDITOR_H)
#define NCMPCPP_NC_PLAYLIST_EDITOR_H

#include <stdbool.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_regex.h"
#include "c/ncm_time.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#define ENUM_NAME NativePlaylistEditorColumn
#define ENUM_PREFIX_ NATIVE_PLAYLIST_EDITOR_COLUMN_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS) \
    X(NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT)
#include "cbase/xenums.c"

#define NATIVE_PLAYLIST_EDITOR_FETCH_DELAY_MS 250

#define ENUM_NAME NativePlaylistEditorCommandType
#define ENUM_PREFIX_ NATIVE_PLAYLIST_EDITOR_COMMAND_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(NATIVE_PLAYLIST_EDITOR_COMMAND_NONE) \
    X(NATIVE_PLAYLIST_EDITOR_COMMAND_LOAD) \
    X(NATIVE_PLAYLIST_EDITOR_COMMAND_SAVE) \
    X(NATIVE_PLAYLIST_EDITOR_COMMAND_RENAME) \
    X(NATIVE_PLAYLIST_EDITOR_COMMAND_DELETE)
#include "cbase/xenums.c"

typedef struct NativePlaylistEditorCommand {
    enum NativePlaylistEditorCommandType type;
    char *playlist;
    char *target;
    int32 playlist_len;
    int32 playlist_cap;
    int32 target_len;
    int32 target_cap;
} NativePlaylistEditorCommand;

typedef struct NativePlaylistEditorScreen {
    NcScreen screen;
    NcPlaylistEntryMenu playlists;
    NcSongMenu content;
    NcWindow playlists_window;
    NcWindow content_window;
    NcmBuffer playlist_filter_constraint;
    NcmBuffer content_filter_constraint;
    NcmBuffer playlist_search_constraint;
    NcmBuffer content_search_constraint;
    NcmBuffer playlists_title;
    NcmBuffer content_title;
    NcmBuffer displayed_playlist_path;
    NcmBuffer observed_playlist_path;

    NcmRegex playlist_filter_regex;
    NcmRegex content_filter_regex;
    NcmRegex playlist_search_regex;
    NcmRegex content_search_regex;

    NcmTimePoint timer;

    int64 start_x;
    int64 width;
    int64 main_start_y;
    int32 main_height;
    int64 left_width;
    int64 right_start_x;
    int64 right_width;
    int64 column_ratio_left;
    int64 column_ratio_right;
    int64 fetching_delay_ms;
    int64 last_playlist_highlight;
    int64 last_known_content_count;
    int32 window_timeout_ms;
    int64 active_column;

    bool playlists_update_requested;
    bool content_update_requested;
    bool playlist_filter_enabled;
    bool content_filter_enabled;
    bool playlist_search_enabled;
    bool content_search_enabled;
    bool displayed_playlist_valid;
    bool observed_playlist_valid;
    bool registered;
} NativePlaylistEditorScreen;

void native_playlist_editor_screen_init(NativePlaylistEditorScreen *screen,
                                        int64 start_x, int64 width,
                                        int64 main_start_y,
                                        int64 main_height,
                                        NcColor color, NcBorder border);
void native_playlist_editor_screen_destroy(NativePlaylistEditorScreen *screen);
NcScreen *native_playlist_editor_screen_base(
    NativePlaylistEditorScreen *screen);
NcPlaylistEntryMenu *native_playlist_editor_screen_playlists(
    NativePlaylistEditorScreen *screen);
NcSongMenu *native_playlist_editor_screen_content(
    NativePlaylistEditorScreen *screen);
NcMenu *native_playlist_editor_screen_active_menu(
    NativePlaylistEditorScreen *screen);
NcWindow *native_playlist_editor_screen_active_window(
    NativePlaylistEditorScreen *screen);
void native_playlist_editor_screen_set_geometry(
    NativePlaylistEditorScreen *screen, int64 start_x, int64 width,
    int64 main_start_y, int64 main_height);
void native_playlist_editor_screen_set_column_ratio(
    NativePlaylistEditorScreen *screen, int64 left, int64 right);
bool native_playlist_editor_screen_previous_column_available(
    NativePlaylistEditorScreen *screen);
bool native_playlist_editor_screen_next_column_available(
    NativePlaylistEditorScreen *screen);
void native_playlist_editor_screen_previous_column(
    NativePlaylistEditorScreen *screen);
void native_playlist_editor_screen_next_column(
    NativePlaylistEditorScreen *screen);
bool native_playlist_editor_screen_load_playlists(
    NativePlaylistEditorScreen *screen, NcmMpdPlaylistList *playlists);
bool native_playlist_editor_screen_reload_playlists_from_mpd(
    NativePlaylistEditorScreen *screen, NcmMpdClient *client,
    NcmError *error);
bool native_playlist_editor_screen_load_content(
    NativePlaylistEditorScreen *screen, NcmMpdSongList *songs);
bool native_playlist_editor_screen_reload_content_from_mpd(
    NativePlaylistEditorScreen *screen, NcmMpdClient *client,
    NcmError *error);
bool native_playlist_editor_screen_locate_playlist(
    NativePlaylistEditorScreen *screen, NcmMpdClient *client,
    char *path, int32 path_len, NcmError *error);
bool native_playlist_editor_screen_locate_song(
    NativePlaylistEditorScreen *screen, NcmMpdClient *client,
    NcmSong *song, NcmError *error);
bool native_playlist_editor_screen_current_playlist(
    NativePlaylistEditorScreen *screen, NcmPlaylist *playlist);
bool native_playlist_editor_screen_current_song(
    NativePlaylistEditorScreen *screen, NcmSong *song);
bool native_playlist_editor_screen_current_content_song(
    NativePlaylistEditorScreen *screen, NcmSong *song);
int64 native_playlist_editor_screen_selected_playlist_count(
    NativePlaylistEditorScreen *screen);
bool native_playlist_editor_screen_selected_songs(
    NativePlaylistEditorScreen *screen, NcmSongArray *songs);
bool native_playlist_editor_screen_apply_active_filter(
    NativePlaylistEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, NcmError *error);
bool native_playlist_editor_screen_search_active(
    NativePlaylistEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, bool forward, bool wrap, bool skip_current,
    NcmError *error);
void native_playlist_editor_screen_request_playlists_update(
    NativePlaylistEditorScreen *screen);
void native_playlist_editor_screen_request_content_update(
    NativePlaylistEditorScreen *screen);

#endif /* NCMPCPP_NC_PLAYLIST_EDITOR_H */
