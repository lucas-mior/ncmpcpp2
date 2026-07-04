#include "screens/nc_server_info.h"


void
nc_server_info_screen_init(NcServerInfoScreen *screen,
                           NcScreenCallbacks callbacks, void *user,
                           int64 cols, int64 lines, int64 main_height) {
    nc_screen_init(&screen->base, callbacks, user, 0);
    nc_server_info_screen_set_dimensions(screen, cols, lines, main_height);
    return;
}

void
nc_server_info_screen_set_dimensions(NcServerInfoScreen *screen,
                                     int64 cols, int64 lines,
                                     int64 main_height) {
    int64 max_height;

    screen->width = cols*6 / 10;
    max_height = lines*7 / 10;
    if (max_height < main_height) {
        screen->height = max_height;
    } else {
        screen->height = main_height;
    }
    return;
}

int64
nc_server_info_screen_width(NcServerInfoScreen *screen) {
    return screen->width;
}

int64
nc_server_info_screen_height(NcServerInfoScreen *screen) {
    return screen->height;
}

int64
nc_server_info_screen_start_x(NcServerInfoScreen *screen, int64 cols) {
    return (cols - screen->width)/2;
}

int64
nc_server_info_screen_start_y(NcServerInfoScreen *screen,
                              int64 main_start_y, int64 main_height) {
    return (main_height - screen->height)/2 + main_start_y;
}
