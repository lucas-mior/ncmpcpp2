#if !defined(NCMPCPP_NC_PLAYLIST_H)
#define NCMPCPP_NC_PLAYLIST_H

#include <stdbool.h>

#include "c/ncm_mpd_client.h"
#include "c/ncm_time.h"
#include "c/ncm_song_list.h"
#include "c/ncm_regex.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_menu.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef void NativePlaylistSync(void *user);
typedef NcWindow *NativePlaylistWindowCallback(void *user);
typedef void NativePlaylistVoidCallback(void *user);
typedef void NativePlaylistSongCallback(NcmSong *song, void *user);
typedef bool NativePlaylistUpdateBeginCallback(uint32 playlist_length,
                                               void *user);
typedef bool NativePlaylistUpdateSongCallback(NcmSong *song, void *user);

typedef struct NativePlaylistBridge {
    NativePlaylistWindowCallback *active_window;
    NativePlaylistVoidCallback *refresh;
    NativePlaylistVoidCallback *refresh_window;
    NativePlaylistVoidCallback *resize;
    NativePlaylistSongCallback *register_song;
    NativePlaylistSongCallback *unregister_song;
    NativePlaylistUpdateBeginCallback *begin_update;
    NativePlaylistUpdateSongCallback *update_song;
    NativePlaylistVoidCallback *end_update;
    void *user;
} NativePlaylistBridge;

typedef struct NcPlaylistScreen {
    NcScreen screen;
    NcMenu *menu;

    int64 start_x;
    int64 width;
    int64 main_start_y;
    int64 main_height;
    int64 lines_scrolled;

    bool mouse_list_scroll_whole_page;
} NcPlaylistScreen;

typedef struct NativePlaylistScreen {
    NcPlaylistScreen screen;
    NcSongMenu songs;
    NcWindow window;
    NcmBuffer title_cache;
    NcmBuffer filter_constraint;
    NcmBuffer search_constraint;
    NcmRegex filter_regex;

    uint64 total_length;
    uint64 remaining_time;
    int64 scroll_begin;
    NcmTimePoint highlight_timer;

    NativePlaylistSync *sync;
    void *sync_user;
    NativePlaylistBridge bridge;

    bool reload_total_length;
    bool reload_remaining;
    bool registered;
    bool syncing;
    bool highlighting_requested;
} NativePlaylistScreen;

void nc_playlist_screen_init(NcPlaylistScreen *screen,
                             NcScreenCallbacks callbacks, void *user,
                             NcMenu *menu, int64 start_x, int64 width,
                             int64 main_start_y, int64 main_height);
void nc_playlist_screen_set_geometry(NcPlaylistScreen *screen,
                                     int64 start_x, int64 width,
                                     int64 main_start_y,
                                     int64 main_height);
void nc_playlist_screen_set_menu(NcPlaylistScreen *screen, NcMenu *menu);
void nc_playlist_screen_set_mouse_config(NcPlaylistScreen *screen,
                                         int64 lines_scrolled,
                                         bool scroll_whole_page);
NcScreen *nc_playlist_screen_base(NcPlaylistScreen *screen);
NcMenu *nc_playlist_screen_menu(NcPlaylistScreen *screen);
int64 nc_playlist_screen_start_x(NcPlaylistScreen *screen);
int64 nc_playlist_screen_start_y(NcPlaylistScreen *screen);
int64 nc_playlist_screen_width(NcPlaylistScreen *screen);
int64 nc_playlist_screen_height(NcPlaylistScreen *screen);
void nc_playlist_screen_scroll(NcPlaylistScreen *screen,
                               enum NcScroll where);
bool nc_playlist_screen_goto_y(NcPlaylistScreen *screen, int64 y);
bool nc_playlist_screen_activate_current(NcPlaylistScreen *screen);
bool nc_playlist_screen_activate_position(NcPlaylistScreen *screen,
                                          int64 pos);
bool nc_playlist_screen_set_current_selected(NcPlaylistScreen *screen,
                                             bool selected);
bool nc_playlist_screen_toggle_current_selected(NcPlaylistScreen *screen);
bool nc_playlist_screen_set_position_selected(NcPlaylistScreen *screen,
                                              int64 pos, bool selected);
bool nc_playlist_screen_toggle_position_selected(NcPlaylistScreen *screen,
                                                 int64 pos);
void nc_playlist_screen_mouse_button_pressed(NcPlaylistScreen *screen,
                                             MEVENT event);

void native_playlist_screen_init(NativePlaylistScreen *screen,
                                 int64 start_x, int64 width,
                                 int64 main_start_y, int64 main_height,
                                 NcColor color, NcBorder border);
void native_playlist_screen_destroy(NativePlaylistScreen *screen);
bool native_playlist_screen_register(NativePlaylistScreen *screen);
bool native_playlist_screen_unregister(NativePlaylistScreen *screen);
NcScreen *native_playlist_screen_base(NativePlaylistScreen *screen);
NcPlaylistScreen *native_playlist_screen_playlist(NativePlaylistScreen *screen);
NcSongMenu *native_playlist_screen_song_menu(NativePlaylistScreen *screen);
NcMenu *native_playlist_screen_menu(NativePlaylistScreen *screen);
NcWindow *native_playlist_screen_window(NativePlaylistScreen *screen);
void native_playlist_screen_set_geometry(NativePlaylistScreen *screen,
                                         int64 start_x, int64 width,
                                         int64 main_start_y,
                                         int64 main_height);
void native_playlist_screen_set_mouse_config(NativePlaylistScreen *screen,
                                             int64 lines_scrolled,
                                             bool scroll_whole_page);
void native_playlist_screen_set_sync_callback(NativePlaylistScreen *screen,
                                              NativePlaylistSync *sync,
                                              void *user);
void native_playlist_screen_set_bridge(NativePlaylistScreen *screen,
                                       NativePlaylistBridge bridge);
void native_playlist_screen_set_display_menu(NativePlaylistScreen *screen,
                                             NcMenu *menu);
void native_playlist_screen_sync(NativePlaylistScreen *screen);
void native_playlist_screen_set_highlighting(NativePlaylistScreen *screen,
                                             bool enabled);
bool native_playlist_screen_highlighting(NativePlaylistScreen *screen);
void native_playlist_screen_request_highlighting(NativePlaylistScreen *screen);
bool native_playlist_screen_consume_highlighting_request(
    NativePlaylistScreen *screen);
void native_playlist_screen_clear(NativePlaylistScreen *screen);
bool native_playlist_screen_add_song_copy(NativePlaylistScreen *screen,
                                          NcmSong *song);
void native_playlist_screen_add_song_move(NativePlaylistScreen *screen,
                                          NcmSong *song);
bool native_playlist_screen_load_song_list(NativePlaylistScreen *screen,
                                           NcmMpdSongList *songs);
bool native_playlist_screen_reload_from_mpd(NativePlaylistScreen *screen,
                                            NcmMpdClient *client,
                                            uint32 version,
                                            uint32 playlist_length,
                                            NcmError *error);
int64 native_playlist_screen_song_count(NativePlaylistScreen *screen);
bool native_playlist_screen_empty(NativePlaylistScreen *screen);
bool native_playlist_screen_current_song(NativePlaylistScreen *screen,
                                         NcmSong *song);
bool native_playlist_screen_now_playing_song(NativePlaylistScreen *screen,
                                             int32 position,
                                             NcmSong *song);
bool native_playlist_screen_locate_position(NativePlaylistScreen *screen,
                                            uint32 position);
bool native_playlist_screen_selected_songs(NativePlaylistScreen *screen,
                                           NcmSongArray *songs);
bool native_playlist_screen_copy_sort_range(
    NativePlaylistScreen *screen, NcmSongArray *songs,
    uint32 *start_position, NcmError *error);
bool native_playlist_screen_select_current_if_none_selected(
    NativePlaylistScreen *screen);
bool native_playlist_screen_apply_filter(NativePlaylistScreen *screen,
                                         char *pattern, int32 pattern_len,
                                         NcmError *error);
void native_playlist_screen_clear_filter(NativePlaylistScreen *screen);
bool native_playlist_screen_search(NativePlaylistScreen *screen,
                                   char *pattern, int32 pattern_len,
                                   bool forward, bool wrap,
                                   bool skip_current, NcmError *error);
void native_playlist_screen_reverse_selection(NativePlaylistScreen *screen);
void native_playlist_screen_clear_selection(NativePlaylistScreen *screen);
bool native_playlist_screen_set_selected_priority(NativePlaylistScreen *screen,
                                                  NcmMpdClient *client,
                                                  int32 priority,
                                                  NcmError *error);
void native_playlist_screen_reload_total_length(NativePlaylistScreen *screen);
void native_playlist_screen_reload_remaining(NativePlaylistScreen *screen);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_PLAYLIST_H */
