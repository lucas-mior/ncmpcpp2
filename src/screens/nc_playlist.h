#if !defined(NCMPCPP_NC_PLAYLIST_H)
#define NCMPCPP_NC_PLAYLIST_H

#include <stdbool.h>

#include "curses/nc_menu.h"
#include "screens/nc_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

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

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_PLAYLIST_H */
