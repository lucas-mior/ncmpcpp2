#include "screens/nc_scrollpad_screen.h"

void
nc_scrollpad_screen_init(NcScrollpadScreen *screen,
                         NcScreenCallbacks callbacks, void *user,
                         int32 type, int64 start_x, int64 start_y,
                         int64 width, int64 height) {
    nc_screen_init(&screen->base, callbacks, user, type);
    nc_scrollpad_screen_set_geometry(screen,
                                     start_x,
                                     start_y,
                                     width,
                                     height);
    return;
}

void
nc_scrollpad_screen_set_geometry(NcScrollpadScreen *screen,
                                 int64 start_x, int64 start_y,
                                 int64 width, int64 height) {
    screen->start_x = start_x;
    screen->start_y = start_y;
    screen->width = width;
    screen->height = height;
    return;
}

void
nc_scrollpad_screen_set_main_area(NcScrollpadScreen *screen,
                                  int64 start_x, int64 width,
                                  int64 main_start_y, int64 main_height) {
    nc_scrollpad_screen_set_geometry(screen,
                                     start_x,
                                     main_start_y,
                                     width,
                                     main_height);
    return;
}

void
nc_scrollpad_screen_set_centered_box(NcScrollpadScreen *screen,
                                     int64 cols, int64 lines,
                                     int64 main_start_y,
                                     int64 main_height,
                                     int64 width_num,
                                     int64 width_den,
                                     int64 height_num,
                                     int64 height_den) {
    int64 max_height;
    int64 width;
    int64 height;
    int64 start_x;
    int64 start_y;

    width = cols*width_num / width_den;
    max_height = lines*height_num / height_den;
    if (max_height < main_height) {
        height = max_height;
    } else {
        height = main_height;
    }
    start_x = (cols - width)/2;
    start_y = (main_height - height)/2 + main_start_y;
    nc_scrollpad_screen_set_geometry(screen,
                                     start_x,
                                     start_y,
                                     width,
                                     height);
    return;
}

NcScreen *
nc_scrollpad_screen_base(NcScrollpadScreen *screen) {
    return &screen->base;
}

int64
nc_scrollpad_screen_start_x(NcScrollpadScreen *screen) {
    return screen->start_x;
}

int64
nc_scrollpad_screen_start_y(NcScrollpadScreen *screen) {
    return screen->start_y;
}

int64
nc_scrollpad_screen_width(NcScrollpadScreen *screen) {
    return screen->width;
}

int64
nc_scrollpad_screen_height(NcScrollpadScreen *screen) {
    return screen->height;
}
