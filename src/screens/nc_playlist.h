#if !defined(NCMPCPP_NC_PLAYLIST_H)
#define NCMPCPP_NC_PLAYLIST_H

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
NcScreen *nc_playlist_screen_base(NcPlaylistScreen *screen);
NcMenu *nc_playlist_screen_menu(NcPlaylistScreen *screen);
int64 nc_playlist_screen_start_x(NcPlaylistScreen *screen);
int64 nc_playlist_screen_start_y(NcPlaylistScreen *screen);
int64 nc_playlist_screen_width(NcPlaylistScreen *screen);
int64 nc_playlist_screen_height(NcPlaylistScreen *screen);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_PLAYLIST_H */
