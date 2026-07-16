#if !defined(NCMPCPP_NC_SERVER_INFO_H)
#define NCMPCPP_NC_SERVER_INFO_H

#include "curses/nc_buffer.h"
#include "curses/nc_scrollpad.h"
#include "screens/nc_scrollpad_screen.h"

typedef struct NcServerInfoHooks {
    void (*load_lists)(void *user);
    bool (*render)(void *user, NcBuffer *buffer);
    void (*switch_to)(void *user);
    void (*resize_layout)(void *user);
    void (*resize_background)(void *user);
    char *(*title)(void *user);
    void (*destroy)(void *user);
    void *user;
} NcServerInfoHooks;

typedef struct NcServerInfoScreen {
    NcScrollpadScreen scrollpad_screen;
    NcWindow window;
    NcScrollpad scrollpad;
    NcBuffer buffer;
    NcServerInfoHooks hooks;
} NcServerInfoScreen;

void nc_server_info_screen_init(NcServerInfoScreen *screen,
                                NcServerInfoHooks hooks,
                                int64 cols, int64 lines,
                                int64 main_start_y,
                                int64 main_height,
                                NcColor color, NcBorder border);
void nc_server_info_screen_set_dimensions(NcServerInfoScreen *screen,
                                          int64 cols, int64 lines,
                                          int64 main_start_y,
                                          int64 main_height);
NcScreen *nc_server_info_screen_base(NcServerInfoScreen *screen);
int64 nc_server_info_screen_width(NcServerInfoScreen *screen);
int64 nc_server_info_screen_height(NcServerInfoScreen *screen);
int64 nc_server_info_screen_start_x(NcServerInfoScreen *screen);
int64 nc_server_info_screen_start_y(NcServerInfoScreen *screen);

#endif /* NCMPCPP_NC_SERVER_INFO_H */
