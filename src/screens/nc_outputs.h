#if !defined(NCMPCPP_NC_OUTPUTS_H)
#define NCMPCPP_NC_OUTPUTS_H

#include "screens/nc_scrollpad_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcOutputsScreen {
    NcScrollpadScreen menu_screen;
} NcOutputsScreen;

void nc_outputs_screen_init(NcOutputsScreen *screen,
                            NcScreenCallbacks callbacks, void *user,
                            int64 start_x, int64 width,
                            int64 main_start_y, int64 main_height);
void nc_outputs_screen_set_geometry(NcOutputsScreen *screen,
                                    int64 start_x, int64 width,
                                    int64 main_start_y, int64 main_height);
NcScreen *nc_outputs_screen_base(NcOutputsScreen *screen);
int64 nc_outputs_screen_start_x(NcOutputsScreen *screen);
int64 nc_outputs_screen_start_y(NcOutputsScreen *screen);
int64 nc_outputs_screen_width(NcOutputsScreen *screen);
int64 nc_outputs_screen_height(NcOutputsScreen *screen);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_OUTPUTS_H */
