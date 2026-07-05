#include "screens/nc_playlist.h"

void
nc_playlist_screen_init(NcPlaylistScreen *screen,
                        NcScreenCallbacks callbacks, void *user,
                        NcMenu *menu, int64 start_x, int64 width,
                        int64 main_start_y, int64 main_height) {
    nc_screen_init(&screen->screen, callbacks, user, NC_SCREEN_TYPE_PLAYLIST);
    nc_playlist_screen_set_menu(screen, menu);
    nc_playlist_screen_set_geometry(screen, start_x, width, main_start_y,
                                    main_height);
    return;
}

void
nc_playlist_screen_set_geometry(NcPlaylistScreen *screen,
                                int64 start_x, int64 width,
                                int64 main_start_y, int64 main_height) {
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    return;
}

void
nc_playlist_screen_set_menu(NcPlaylistScreen *screen, NcMenu *menu) {
    screen->menu = menu;
    return;
}

NcScreen *
nc_playlist_screen_base(NcPlaylistScreen *screen) {
    return &screen->screen;
}

NcMenu *
nc_playlist_screen_menu(NcPlaylistScreen *screen) {
    return screen->menu;
}

int64
nc_playlist_screen_start_x(NcPlaylistScreen *screen) {
    return screen->start_x;
}

int64
nc_playlist_screen_start_y(NcPlaylistScreen *screen) {
    return screen->main_start_y;
}

int64
nc_playlist_screen_width(NcPlaylistScreen *screen) {
    return screen->width;
}

int64
nc_playlist_screen_height(NcPlaylistScreen *screen) {
    return screen->main_height;
}
