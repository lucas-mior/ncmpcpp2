#if !defined(NCMPCPP_NC_SERVER_INFO_H)
#define NCMPCPP_NC_SERVER_INFO_H

#include "screens/nc_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcServerInfoScreen {
    NcScreen base;
    int64 width;
    int64 height;
} NcServerInfoScreen;

void nc_server_info_screen_init(NcServerInfoScreen *screen,
                                NcScreenCallbacks callbacks, void *user,
                                int64 cols, int64 lines,
                                int64 main_height);
void nc_server_info_screen_set_dimensions(NcServerInfoScreen *screen,
                                          int64 cols, int64 lines,
                                          int64 main_height);
int64 nc_server_info_screen_width(NcServerInfoScreen *screen);
int64 nc_server_info_screen_height(NcServerInfoScreen *screen);
int64 nc_server_info_screen_start_x(NcServerInfoScreen *screen, int64 cols);
int64 nc_server_info_screen_start_y(NcServerInfoScreen *screen,
                                    int64 main_start_y,
                                    int64 main_height);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_SERVER_INFO_H */
