#if !defined(NCMPCPP_NC_PLAYLIST_H)
#define NCMPCPP_NC_PLAYLIST_H

#include <stdbool.h>

#include "c/ncm_mutable_song.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_time.h"
#include "c/ncm_regex.h"
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

typedef struct NativePlaylistScreen {
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
} NativePlaylistScreen;

void nc_playlist_screen_init(NcPlaylistScreen *screen,
                             NcScreenCallbacks callbacks, void *user,
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

void native_playlist_screen_init(NativePlaylistScreen *screen,
                                 int32 start_x, int32 width,
                                 int32 main_start_y, int32 main_height,
                                 NcColor color, NcBorder border);
void native_playlist_screen_destroy(NativePlaylistScreen *screen);
bool native_playlist_screen_unregister(NativePlaylistScreen *screen);
NcScreen *native_playlist_screen_base(NativePlaylistScreen *screen);
NcPlaylistScreen *native_playlist_screen_playlist(NativePlaylistScreen *screen);
NcSongMenu *native_playlist_screen_song_menu(NativePlaylistScreen *screen);
NcMenu *native_playlist_screen_menu(NativePlaylistScreen *screen);
NcWindow *native_playlist_screen_window(NativePlaylistScreen *screen);
void native_playlist_screen_update_column_title(
    NativePlaylistScreen *screen);
void native_playlist_screen_set_geometry(NativePlaylistScreen *screen,
                                         int32 start_x, int32 width,
                                         int32 main_start_y,
                                         int32 main_height);
void native_playlist_screen_set_mouse_config(NativePlaylistScreen *screen,
                                             int32 lines_scrolled,
                                             bool scroll_whole_page);
void native_playlist_screen_set_highlighting(NativePlaylistScreen *screen,
                                             bool enabled);
bool native_playlist_screen_highlighting(NativePlaylistScreen *screen);
void native_playlist_screen_request_highlighting(NativePlaylistScreen *screen);
void native_playlist_screen_clear(NativePlaylistScreen *screen);
bool native_playlist_screen_reload_from_mpd(NativePlaylistScreen *screen,
                                            NcmMpdClient *client,
                                            int32 version,
                                            int32 playlist_length,
                                            NcmError *error);
int32 native_playlist_screen_song_count(NativePlaylistScreen *screen);
bool native_playlist_screen_empty(NativePlaylistScreen *screen);
bool native_playlist_screen_current_song(NativePlaylistScreen *screen,
                                         NcmSong *song);
bool native_playlist_screen_update_current_mutable_song(
    NativePlaylistScreen *screen, NcmMutableSong *song);
bool native_playlist_screen_now_playing_song(NativePlaylistScreen *screen,
                                             int32 position,
                                             NcmSong *song);
bool native_playlist_screen_locate_position(NativePlaylistScreen *screen,
                                            int32 position);
bool native_playlist_screen_selected_songs(NativePlaylistScreen *screen,
                                           NcmSongArray *songs);
bool native_playlist_screen_has_sortable_range(
    NativePlaylistScreen *screen);
bool native_playlist_screen_copy_sort_range(
    NativePlaylistScreen *screen, NcmSongArray *songs,
    int32 *start_position, NcmError *error);
bool native_playlist_screen_apply_filter(NativePlaylistScreen *screen,
                                         char *pattern, int32 pattern_len,
                                         NcmError *error);
void native_playlist_screen_clear_filter(NativePlaylistScreen *screen);
bool native_playlist_screen_search(NativePlaylistScreen *screen,
                                   char *pattern, int32 pattern_len,
                                   bool forward, bool wrap,
                                   bool skip_current, NcmError *error);
bool native_playlist_screen_set_selected_priority(NativePlaylistScreen *screen,
                                                  NcmMpdClient *client,
                                                  int32 priority,
                                                  NcmError *error);
void native_playlist_screen_reload_total_length(NativePlaylistScreen *screen);
void native_playlist_screen_reload_remaining(NativePlaylistScreen *screen);

#endif /* NCMPCPP_NC_PLAYLIST_H */
