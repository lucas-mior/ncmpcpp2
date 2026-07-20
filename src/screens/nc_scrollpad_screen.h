#if !defined(NCMPCPP_NC_SCROLLPAD_SCREEN_H)
#define NCMPCPP_NC_SCROLLPAD_SCREEN_H

#include "screens/nc_screen.h"

typedef struct NcScrollpadScreen {
    NcScreen base;

    int32 start_x;
    int32 start_y;
    int32 width;
    int32 height;
} NcScrollpadScreen;

void nc_scrollpad_screen_init(NcScrollpadScreen *screen,
                              NcScreenCallbacks callbacks, void *user,
                              int32 type, int32 start_x, int32 start_y,
                              int32 width, int32 height);
void nc_scrollpad_screen_set_geometry(NcScrollpadScreen *screen,
                                      int32 start_x, int32 start_y,
                                      int32 width, int32 height);
void nc_scrollpad_screen_set_main_area(NcScrollpadScreen *screen,
                                       int32 start_x, int32 width,
                                       int32 main_start_y,
                                       int32 main_height);
void nc_scrollpad_screen_set_centered_box(NcScrollpadScreen *screen,
                                          int32 cols, int32 lines,
                                          int32 main_start_y,
                                          int32 main_height,
                                          int32 width_num,
                                          int32 width_den,
                                          int32 height_num,
                                          int32 height_den);
NcScreen *nc_scrollpad_screen_base(NcScrollpadScreen *screen);
int32 nc_scrollpad_screen_start_x(NcScrollpadScreen *screen);
int32 nc_scrollpad_screen_start_y(NcScrollpadScreen *screen);
int32 nc_scrollpad_screen_width(NcScrollpadScreen *screen);
int32 nc_scrollpad_screen_height(NcScrollpadScreen *screen);

#endif /* NCMPCPP_NC_SCROLLPAD_SCREEN_H */
