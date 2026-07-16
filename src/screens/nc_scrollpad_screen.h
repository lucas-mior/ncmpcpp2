#if !defined(NCMPCPP_NC_SCROLLPAD_SCREEN_H)
#define NCMPCPP_NC_SCROLLPAD_SCREEN_H

#include "screens/nc_screen.h"

typedef struct NcScrollpadScreen {
    NcScreen base;

    int64 start_x;
    int64 start_y;
    int64 width;
    int64 height;
} NcScrollpadScreen;

void nc_scrollpad_screen_init(NcScrollpadScreen *screen,
                              NcScreenCallbacks callbacks, void *user,
                              int32 type, int64 start_x, int64 start_y,
                              int64 width, int64 height);
void nc_scrollpad_screen_set_geometry(NcScrollpadScreen *screen,
                                      int64 start_x, int64 start_y,
                                      int64 width, int64 height);
void nc_scrollpad_screen_set_main_area(NcScrollpadScreen *screen,
                                       int64 start_x, int64 width,
                                       int64 main_start_y,
                                       int64 main_height);
void nc_scrollpad_screen_set_centered_box(NcScrollpadScreen *screen,
                                          int64 cols, int64 lines,
                                          int64 main_start_y,
                                          int64 main_height,
                                          int64 width_num,
                                          int64 width_den,
                                          int64 height_num,
                                          int64 height_den);
NcScreen *nc_scrollpad_screen_base(NcScrollpadScreen *screen);
int64 nc_scrollpad_screen_start_x(NcScrollpadScreen *screen);
int64 nc_scrollpad_screen_start_y(NcScrollpadScreen *screen);
int64 nc_scrollpad_screen_width(NcScrollpadScreen *screen);
int64 nc_scrollpad_screen_height(NcScrollpadScreen *screen);

#endif /* NCMPCPP_NC_SCROLLPAD_SCREEN_H */
