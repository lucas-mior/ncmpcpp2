#if !defined(NCMPCPP_NC_HELP_H)
#define NCMPCPP_NC_HELP_H

#include "screens/nc_scrollpad_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcHelpScreen {
    NcScrollpadScreen scrollpad_screen;
} NcHelpScreen;

void nc_help_screen_init(NcHelpScreen *screen,
                         NcScreenCallbacks callbacks, void *user,
                         int64 start_x, int64 width,
                         int64 main_start_y, int64 main_height);
void nc_help_screen_set_geometry(NcHelpScreen *screen,
                                 int64 start_x, int64 width,
                                 int64 main_start_y,
                                 int64 main_height);
NcScreen *nc_help_screen_base(NcHelpScreen *screen);
int64 nc_help_screen_start_x(NcHelpScreen *screen);
int64 nc_help_screen_start_y(NcHelpScreen *screen);
int64 nc_help_screen_width(NcHelpScreen *screen);
int64 nc_help_screen_height(NcHelpScreen *screen);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_HELP_H */
