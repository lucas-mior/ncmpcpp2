#if !defined(NCMPCPP_NC_PLAYLIST_EDITOR_H)
#define NCMPCPP_NC_PLAYLIST_EDITOR_H

#include "cbase.h"

#include "c/ncm_app_arrays.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_regex.h"
#include "c/ncm_time.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#define ENUM_NAME PlaylistEditorColumn
#define ENUM_PREFIX_ PLAYLIST_EDITOR_COLUMN_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(PLAYLIST_EDITOR_COLUMN_PLAYLISTS) \
    X(PLAYLIST_EDITOR_COLUMN_CONTENT)
#include "cbase/xenums.c"

#define PLAYLIST_EDITOR_FETCH_DELAY_MS 250

#define ENUM_NAME PlaylistEditorCommandType
#define ENUM_PREFIX_ PLAYLIST_EDITOR_COMMAND_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(PLAYLIST_EDITOR_COMMAND_NONE) \
    X(PLAYLIST_EDITOR_COMMAND_LOAD) \
    X(PLAYLIST_EDITOR_COMMAND_SAVE) \
    X(PLAYLIST_EDITOR_COMMAND_RENAME) \
    X(PLAYLIST_EDITOR_COMMAND_DELETE)
#include "cbase/xenums.c"

typedef struct PlaylistEditorCommand {
    enum PlaylistEditorCommandType type;
    char *playlist;
    char *target;
    int32 playlist_len;
    int32 playlist_cap;
    int32 target_len;
    int32 target_cap;
} PlaylistEditorCommand;

typedef struct PlaylistEditorScreen {
    NcScreen screen;
    NcPlaylistEntryMenu playlists;
    NcSongMenu content;
    NcWindow playlists_window;
    NcWindow content_window;
    StrBuilder playlist_filter_constraint;
    StrBuilder content_filter_constraint;
    StrBuilder playlist_search_constraint;
    StrBuilder content_search_constraint;
    StrBuilder playlists_title;
    StrBuilder content_title;
    StrBuilder displayed_playlist_path;
    StrBuilder observed_playlist_path;

    NcmRegex playlist_filter_regex;
    NcmRegex content_filter_regex;
    NcmRegex playlist_search_regex;
    NcmRegex content_search_regex;

    NcmTimePoint timer;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;
    int32 left_width;
    int32 right_start_x;
    int32 right_width;
    int32 column_ratio_left;
    int32 column_ratio_right;
    int32 fetching_delay_ms;
    int32 last_playlist_highlight;
    int32 last_known_content_count;
    int32 window_timeout_ms;
    int32 active_column;

    bool playlists_update_requested;
    bool content_update_requested;
    bool playlist_filter_enabled;
    bool content_filter_enabled;
    bool playlist_search_enabled;
    bool content_search_enabled;
    bool displayed_playlist_valid;
    bool observed_playlist_valid;
    bool registered;
} PlaylistEditorScreen;

void playlist_editor_screen_init(PlaylistEditorScreen *screen,
                                        int32 start_x, int32 width,
                                        int32 main_start_y,
                                        int32 main_height,
                                        NcColor color, NcBorder border);
void playlist_editor_screen_destroy(PlaylistEditorScreen *screen);
NcScreen *playlist_editor_screen_base(
    PlaylistEditorScreen *screen);
NcPlaylistEntryMenu *playlist_editor_screen_playlists(
    PlaylistEditorScreen *screen);
NcSongMenu *playlist_editor_screen_content(
    PlaylistEditorScreen *screen);
NcMenu *playlist_editor_screen_active_menu(
    PlaylistEditorScreen *screen);
NcWindow *playlist_editor_screen_active_window(
    PlaylistEditorScreen *screen);
void playlist_editor_screen_set_geometry(
    PlaylistEditorScreen *screen, int32 start_x, int32 width,
    int32 main_start_y, int32 main_height);
void playlist_editor_screen_set_column_ratio(
    PlaylistEditorScreen *screen, int32 left, int32 right);
bool playlist_editor_screen_previous_column_available(
    PlaylistEditorScreen *screen);
bool playlist_editor_screen_next_column_available(
    PlaylistEditorScreen *screen);
void playlist_editor_screen_previous_column(
    PlaylistEditorScreen *screen);
void playlist_editor_screen_next_column(
    PlaylistEditorScreen *screen);
bool playlist_editor_screen_load_playlists(
    PlaylistEditorScreen *screen, NcmMpdPlaylistList *playlists);
bool playlist_editor_screen_reload_playlists_from_mpd(
    PlaylistEditorScreen *screen, NcmMpdClient *client,
    NcmError *error);
bool playlist_editor_screen_load_content(
    PlaylistEditorScreen *screen, NcmMpdSongList *songs);
bool playlist_editor_screen_reload_content_from_mpd(
    PlaylistEditorScreen *screen, NcmMpdClient *client,
    NcmError *error);
bool playlist_editor_screen_locate_playlist(
    PlaylistEditorScreen *screen, NcmMpdClient *client,
    char *path, int32 path_len, NcmError *error);
bool playlist_editor_screen_locate_song(
    PlaylistEditorScreen *screen, NcmMpdClient *client,
    NcmSong *song, NcmError *error);
bool playlist_editor_screen_current_playlist(
    PlaylistEditorScreen *screen, NcmPlaylist *playlist);
bool playlist_editor_screen_current_song(
    PlaylistEditorScreen *screen, NcmSong *song);
bool playlist_editor_screen_current_content_song(
    PlaylistEditorScreen *screen, NcmSong *song);
int32 playlist_editor_screen_selected_playlist_count(
    PlaylistEditorScreen *screen);
bool playlist_editor_screen_selected_songs(
    PlaylistEditorScreen *screen, NcmSongArray *songs);
bool playlist_editor_screen_apply_active_filter(
    PlaylistEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, NcmError *error);
bool playlist_editor_screen_search_active(
    PlaylistEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, bool forward, bool wrap, bool skip_current,
    NcmError *error);
void playlist_editor_screen_request_playlists_update(
    PlaylistEditorScreen *screen);
void playlist_editor_screen_request_content_update(
    PlaylistEditorScreen *screen);

#endif /* NCMPCPP_NC_PLAYLIST_EDITOR_H */
