#if !defined(NCMPCPP_NC_SCROLLPAD_SCREEN_C)
#define NCMPCPP_NC_SCROLLPAD_SCREEN_C

#include "screens/nc_scrollpad_screen.h"

void
nc_scrollpad_screen_init(NcScrollpadScreen *screen,
                         NcScreenCallbacks callbacks, void *user,
                         int32 type, int32 start_x, int32 start_y,
                         int32 width, int32 height) {
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
                                 int32 start_x, int32 start_y,
                                 int32 width, int32 height) {
    screen->start_x = start_x;
    screen->start_y = start_y;
    screen->width = width;
    screen->height = height;
    return;
}

void
nc_scrollpad_screen_set_main_area(NcScrollpadScreen *screen,
                                  int32 start_x, int32 width,
                                  int32 main_start_y, int32 main_height) {
    nc_scrollpad_screen_set_geometry(screen,
                                     start_x,
                                     main_start_y,
                                     width,
                                     main_height);
    return;
}

void
nc_scrollpad_screen_set_centered_box(NcScrollpadScreen *screen,
                                     int32 cols, int32 lines,
                                     int32 main_start_y,
                                     int32 main_height,
                                     int32 width_num,
                                     int32 width_den,
                                     int32 height_num,
                                     int32 height_den) {
    int32 max_height;
    int32 width;
    int32 height;
    int32 start_x;
    int32 start_y;

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

int32
nc_scrollpad_screen_start_x(NcScrollpadScreen *screen) {
    return screen->start_x;
}

int32
nc_scrollpad_screen_start_y(NcScrollpadScreen *screen) {
    return screen->start_y;
}

int32
nc_scrollpad_screen_width(NcScrollpadScreen *screen) {
    return screen->width;
}

int32
nc_scrollpad_screen_height(NcScrollpadScreen *screen) {
    return screen->height;
}

#endif /* NCMPCPP_NC_SCROLLPAD_SCREEN_C */
