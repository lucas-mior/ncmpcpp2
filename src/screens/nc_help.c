#include "screens/nc_help.h"

void
nc_help_screen_init(NcHelpScreen *screen,
                    NcScreenCallbacks callbacks, void *user,
                    int64 start_x, int64 width,
                    int64 main_start_y, int64 main_height) {
    nc_scrollpad_screen_init(&screen->scrollpad_screen,
                             callbacks,
                             user,
                             NC_SCREEN_TYPE_HELP,
                             0,
                             0,
                             0,
                             0);
    nc_help_screen_set_geometry(screen,
                                start_x,
                                width,
                                main_start_y,
                                main_height);
    return;
}

void
nc_help_screen_set_geometry(NcHelpScreen *screen,
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
nc_help_screen_base(NcHelpScreen *screen) {
    return nc_scrollpad_screen_base(&screen->scrollpad_screen);
}

int64
nc_help_screen_start_x(NcHelpScreen *screen) {
    return nc_scrollpad_screen_start_x(&screen->scrollpad_screen);
}

int64
nc_help_screen_start_y(NcHelpScreen *screen) {
    return nc_scrollpad_screen_start_y(&screen->scrollpad_screen);
}

int64
nc_help_screen_width(NcHelpScreen *screen) {
    return nc_scrollpad_screen_width(&screen->scrollpad_screen);
}

int64
nc_help_screen_height(NcHelpScreen *screen) {
    return nc_scrollpad_screen_height(&screen->scrollpad_screen);
}
