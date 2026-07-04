#include "screens/nc_outputs.h"

void
nc_outputs_screen_init(NcOutputsScreen *screen,
                       NcScreenCallbacks callbacks, void *user,
                       int64 start_x, int64 width,
                       int64 main_start_y, int64 main_height) {
    nc_scrollpad_screen_init(&screen->menu_screen,
                             callbacks,
                             user,
                             0,
                             0,
                             0,
                             0,
                             0);
    nc_outputs_screen_set_geometry(screen,
                                   start_x,
                                   width,
                                   main_start_y,
                                   main_height);
    return;
}

void
nc_outputs_screen_set_geometry(NcOutputsScreen *screen,
                               int64 start_x, int64 width,
                               int64 main_start_y, int64 main_height) {
    nc_scrollpad_screen_set_main_area(&screen->menu_screen,
                                      start_x,
                                      width,
                                      main_start_y,
                                      main_height);
    return;
}

NcScreen *
nc_outputs_screen_base(NcOutputsScreen *screen) {
    return nc_scrollpad_screen_base(&screen->menu_screen);
}

int64
nc_outputs_screen_start_x(NcOutputsScreen *screen) {
    return nc_scrollpad_screen_start_x(&screen->menu_screen);
}

int64
nc_outputs_screen_start_y(NcOutputsScreen *screen) {
    return nc_scrollpad_screen_start_y(&screen->menu_screen);
}

int64
nc_outputs_screen_width(NcOutputsScreen *screen) {
    return nc_scrollpad_screen_width(&screen->menu_screen);
}

int64
nc_outputs_screen_height(NcOutputsScreen *screen) {
    return nc_scrollpad_screen_height(&screen->menu_screen);
}
