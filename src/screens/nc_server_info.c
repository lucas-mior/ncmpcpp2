#include "screens/nc_server_info.h"

void
nc_server_info_screen_init(NcServerInfoScreen *screen,
                           NcScreenCallbacks callbacks, void *user,
                           int64 cols, int64 lines, int64 main_start_y,
                           int64 main_height) {
    nc_scrollpad_screen_init(&screen->scrollpad_screen,
                             callbacks,
                             user,
                             0,
                             0,
                             0,
                             0,
                             0);
    nc_server_info_screen_set_dimensions(screen,
                                         cols,
                                         lines,
                                         main_start_y,
                                         main_height);
    return;
}

void
nc_server_info_screen_set_dimensions(NcServerInfoScreen *screen,
                                     int64 cols, int64 lines,
                                     int64 main_start_y,
                                     int64 main_height) {
    nc_scrollpad_screen_set_centered_box(&screen->scrollpad_screen,
                                         cols,
                                         lines,
                                         main_start_y,
                                         main_height,
                                         6,
                                         10,
                                         7,
                                         10);
    return;
}

NcScreen *
nc_server_info_screen_base(NcServerInfoScreen *screen) {
    return nc_scrollpad_screen_base(&screen->scrollpad_screen);
}

int64
nc_server_info_screen_width(NcServerInfoScreen *screen) {
    return nc_scrollpad_screen_width(&screen->scrollpad_screen);
}

int64
nc_server_info_screen_height(NcServerInfoScreen *screen) {
    return nc_scrollpad_screen_height(&screen->scrollpad_screen);
}

int64
nc_server_info_screen_start_x(NcServerInfoScreen *screen) {
    return nc_scrollpad_screen_start_x(&screen->scrollpad_screen);
}

int64
nc_server_info_screen_start_y(NcServerInfoScreen *screen) {
    return nc_scrollpad_screen_start_y(&screen->scrollpad_screen);
}
