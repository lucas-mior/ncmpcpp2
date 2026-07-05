#include "screens/nc_lastfm.h"

void
nc_lastfm_screen_init(NcLastfmScreen *screen,
                      NcScreenCallbacks callbacks, void *user,
                      int64 start_x, int64 width,
                      int64 main_start_y, int64 main_height) {
    nc_scrollpad_screen_init(&screen->scrollpad_screen,
                             callbacks,
                             user,
                             NC_SCREEN_TYPE_LASTFM,
                             0,
                             0,
                             0,
                             0);
    nc_lastfm_screen_set_geometry(screen,
                                  start_x,
                                  width,
                                  main_start_y,
                                  main_height);
    return;
}

void
nc_lastfm_screen_set_geometry(NcLastfmScreen *screen,
                              int64 start_x, int64 width,
                              int64 main_start_y,
                              int64 main_height) {
    nc_scrollpad_screen_set_main_area(&screen->scrollpad_screen,
                                      start_x,
                                      width,
                                      main_start_y,
                                      main_height);
    return;
}

NcScreen *
nc_lastfm_screen_base(NcLastfmScreen *screen) {
    return nc_scrollpad_screen_base(&screen->scrollpad_screen);
}

int64
nc_lastfm_screen_start_x(NcLastfmScreen *screen) {
    return nc_scrollpad_screen_start_x(&screen->scrollpad_screen);
}

int64
nc_lastfm_screen_start_y(NcLastfmScreen *screen) {
    return nc_scrollpad_screen_start_y(&screen->scrollpad_screen);
}

int64
nc_lastfm_screen_width(NcLastfmScreen *screen) {
    return nc_scrollpad_screen_width(&screen->scrollpad_screen);
}

int64
nc_lastfm_screen_height(NcLastfmScreen *screen) {
    return nc_scrollpad_screen_height(&screen->scrollpad_screen);
}
