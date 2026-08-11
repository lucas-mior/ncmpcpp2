#if !defined(NCMPCPP_NC_PLAYLIST_H)
#define NCMPCPP_NC_PLAYLIST_H

#include "cbase.h"

#include "c/ncm_mpd_client.h"
#include "c/ncm_mutable_song.h"
#include "c/ncm_regex.h"
#include "c/ncm_time.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_menu.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

typedef struct NcPlaylistScreen {
    NcScreen screen;
    NcMenu *menu;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;
    int32 lines_scrolled;

    bool mouse_list_scroll_whole_page;
} NcPlaylistScreen;

typedef struct PlaylistScreen {
    NcPlaylistScreen screen;
    NcSongMenu songs;
    NcWindow window;
    StrBuilder title_cache;
    StrBuilder column_title;
    StrBuilder filter_constraint;
    StrBuilder search_constraint;
    NcmRegex filter_regex;

    uint64 total_length;
    uint64 remaining_time;
    int32 scroll_begin;
    NcmTimePoint highlight_timer;

    bool reload_total_length;
    bool reload_remaining;
    bool registered;
    bool highlighting_requested;
} PlaylistScreen;

void nc_playlist_screen_init(NcPlaylistScreen *screen,
                             NcScreenOps callbacks, void *user,
                             NcMenu *menu, int32 start_x, int32 width,
                             int32 main_start_y, int32 main_height);
void nc_playlist_screen_set_geometry(NcPlaylistScreen *screen,
                                     int32 start_x, int32 width,
                                     int32 main_start_y,
                                     int32 main_height);
void nc_playlist_screen_set_menu(NcPlaylistScreen *screen, NcMenu *menu);
void nc_playlist_screen_set_mouse_config(NcPlaylistScreen *screen,
                                         int32 lines_scrolled,
                                         bool scroll_whole_page);
NcScreen *nc_playlist_screen_base(NcPlaylistScreen *screen);
NcMenu *nc_playlist_screen_menu(NcPlaylistScreen *screen);
int32 nc_playlist_screen_height(NcPlaylistScreen *screen);
void nc_playlist_screen_scroll(NcPlaylistScreen *screen,
                               enum NcScroll where);
bool nc_playlist_screen_goto_y(NcPlaylistScreen *screen, int32 y);
bool nc_playlist_screen_activate_current(NcPlaylistScreen *screen);
void nc_playlist_screen_mouse_button_pressed(NcPlaylistScreen *screen,
                                             MEVENT event);

void playlist_screen_init(PlaylistScreen *screen,
                                 int32 start_x, int32 width,
                                 int32 main_start_y, int32 main_height,
                                 NcColor color, NcBorder border);
void playlist_screen_destroy(PlaylistScreen *screen);
bool playlist_screen_unregister(PlaylistScreen *screen);
NcScreen *playlist_screen_base(PlaylistScreen *screen);
NcPlaylistScreen *playlist_screen_playlist(PlaylistScreen *screen);
NcSongMenu *playlist_screen_song_menu(PlaylistScreen *screen);
NcMenu *playlist_screen_menu(PlaylistScreen *screen);
NcWindow *playlist_screen_window(PlaylistScreen *screen);
void playlist_screen_update_column_title(
    PlaylistScreen *screen);
void playlist_screen_set_geometry(PlaylistScreen *screen,
                                         int32 start_x, int32 width,
                                         int32 main_start_y,
                                         int32 main_height);
void playlist_screen_set_mouse_config(PlaylistScreen *screen,
                                             int32 lines_scrolled,
                                             bool scroll_whole_page);
void playlist_screen_set_highlighting(PlaylistScreen *screen,
                                             bool enabled);
bool playlist_screen_highlighting(PlaylistScreen *screen);
void playlist_screen_request_highlighting(PlaylistScreen *screen);
void playlist_screen_clear(PlaylistScreen *screen);
bool playlist_screen_reload_from_mpd(PlaylistScreen *screen,
                                            NcmMpdClient *client,
                                            int32 version,
                                            int32 playlist_length,
                                            NcmError *ncm_error);
int32 playlist_screen_song_count(PlaylistScreen *screen);
bool playlist_screen_empty(PlaylistScreen *screen);
bool playlist_screen_current_song(PlaylistScreen *screen,
                                         NcmSong *song);
bool playlist_screen_update_current_mutable_song(
    PlaylistScreen *screen, NcmMutableSong *song);
bool playlist_screen_now_playing_song(PlaylistScreen *screen,
                                             int32 position,
                                             NcmSong *song);
bool playlist_screen_locate_position(PlaylistScreen *screen,
                                            int32 position);
bool playlist_screen_selected_songs(PlaylistScreen *screen,
                                           NcmSongArray *songs);
bool playlist_screen_has_sortable_range(
    PlaylistScreen *screen);
bool playlist_screen_copy_sort_range(
    PlaylistScreen *screen, NcmSongArray *songs,
    int32 *start_position, NcmError *ncm_error);
bool playlist_screen_apply_filter(PlaylistScreen *screen,
                                         char *pattern, int32 pattern_len,
                                         NcmError *ncm_error);
void playlist_screen_clear_filter(PlaylistScreen *screen);
bool playlist_screen_search(PlaylistScreen *screen,
                                   char *pattern, int32 pattern_len,
                                   bool forward, bool wrap,
                                   bool skip_current, NcmError *ncm_error);
bool playlist_screen_set_selected_priority(PlaylistScreen *screen,
                                                  NcmMpdClient *client,
                                                  int32 priority,
                                                  NcmError *ncm_error);
void playlist_screen_reload_total_length(PlaylistScreen *screen);
void playlist_screen_reload_remaining(PlaylistScreen *screen);

#endif /* NCMPCPP_NC_PLAYLIST_H */
