#if !defined(NCMPCPP_NC_LASTFM_H)
#define NCMPCPP_NC_LASTFM_H

#include "screens/nc_scrollpad_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcLastfmScreen {
    NcScrollpadScreen scrollpad_screen;
} NcLastfmScreen;

void nc_lastfm_screen_init(NcLastfmScreen *screen,
                           NcScreenCallbacks callbacks, void *user,
                           int64 start_x, int64 width,
                           int64 main_start_y, int64 main_height);
void nc_lastfm_screen_set_geometry(NcLastfmScreen *screen,
                                   int64 start_x, int64 width,
                                   int64 main_start_y,
                                   int64 main_height);
NcScreen *nc_lastfm_screen_base(NcLastfmScreen *screen);
int64 nc_lastfm_screen_start_x(NcLastfmScreen *screen);
int64 nc_lastfm_screen_start_y(NcLastfmScreen *screen);
int64 nc_lastfm_screen_width(NcLastfmScreen *screen);
int64 nc_lastfm_screen_height(NcLastfmScreen *screen);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_LASTFM_H */
