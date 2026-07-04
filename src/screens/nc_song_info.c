#include "screens/nc_song_info.h"

void
nc_song_info_screen_init(NcSongInfoScreen *screen,
                         NcScreenCallbacks callbacks, void *user,
                         int64 start_x, int64 width,
                         int64 main_start_y, int64 main_height) {
    nc_screen_init(&screen->base, callbacks, user, 0);
    nc_song_info_screen_set_geometry(screen,
                                     start_x,
                                     width,
                                     main_start_y,
                                     main_height);
    return;
}

void
nc_song_info_screen_set_geometry(NcSongInfoScreen *screen,
                                 int64 start_x, int64 width,
                                 int64 main_start_y, int64 main_height) {
    screen->start_x = start_x;
    screen->start_y = main_start_y;
    screen->width = width;
    screen->height = main_height;
    return;
}

int64
nc_song_info_screen_start_x(NcSongInfoScreen *screen) {
    return screen->start_x;
}

int64
nc_song_info_screen_start_y(NcSongInfoScreen *screen) {
    return screen->start_y;
}

int64
nc_song_info_screen_width(NcSongInfoScreen *screen) {
    return screen->width;
}

int64
nc_song_info_screen_height(NcSongInfoScreen *screen) {
    return screen->height;
}
