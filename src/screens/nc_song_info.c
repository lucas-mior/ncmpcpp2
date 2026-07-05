#include "screens/nc_song_info.h"

void
nc_song_info_screen_init(NcSongInfoScreen *screen,
                         NcScreenCallbacks callbacks, void *user,
                         int64 start_x, int64 width,
                         int64 main_start_y, int64 main_height) {
    nc_scrollpad_screen_init(&screen->scrollpad_screen,
                             callbacks,
                             user,
                             NC_SCREEN_TYPE_SONG_INFO,
                             0,
                             0,
                             0,
                             0);
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
    nc_scrollpad_screen_set_main_area(&screen->scrollpad_screen,
                                      start_x,
                                      width,
                                      main_start_y,
                                      main_height);
    return;
}

NcScreen *
nc_song_info_screen_base(NcSongInfoScreen *screen) {
    return nc_scrollpad_screen_base(&screen->scrollpad_screen);
}

int64
nc_song_info_screen_start_x(NcSongInfoScreen *screen) {
    return nc_scrollpad_screen_start_x(&screen->scrollpad_screen);
}

int64
nc_song_info_screen_start_y(NcSongInfoScreen *screen) {
    return nc_scrollpad_screen_start_y(&screen->scrollpad_screen);
}

int64
nc_song_info_screen_width(NcSongInfoScreen *screen) {
    return nc_scrollpad_screen_width(&screen->scrollpad_screen);
}

int64
nc_song_info_screen_height(NcSongInfoScreen *screen) {
    return nc_scrollpad_screen_height(&screen->scrollpad_screen);
}
