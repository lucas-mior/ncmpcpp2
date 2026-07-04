#if !defined(NCMPCPP_NC_SERVER_INFO_H)
#define NCMPCPP_NC_SERVER_INFO_H

#include "screens/nc_scrollpad_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcServerInfoScreen {
    NcScrollpadScreen scrollpad_screen;
} NcServerInfoScreen;

void nc_server_info_screen_init(NcServerInfoScreen *screen,
                                NcScreenCallbacks callbacks, void *user,
                                int64 cols, int64 lines,
                                int64 main_start_y,
                                int64 main_height);
void nc_server_info_screen_set_dimensions(NcServerInfoScreen *screen,
                                          int64 cols, int64 lines,
                                          int64 main_start_y,
                                          int64 main_height);
NcScreen *nc_server_info_screen_base(NcServerInfoScreen *screen);
int64 nc_server_info_screen_width(NcServerInfoScreen *screen);
int64 nc_server_info_screen_height(NcServerInfoScreen *screen);
int64 nc_server_info_screen_start_x(NcServerInfoScreen *screen);
int64 nc_server_info_screen_start_y(NcServerInfoScreen *screen);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_SERVER_INFO_H */
