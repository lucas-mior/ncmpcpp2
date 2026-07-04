#if !defined(NCMPCPP_NC_SONG_INFO_H)
#define NCMPCPP_NC_SONG_INFO_H

#include "screens/nc_scrollpad_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcSongInfoScreen {
    NcScrollpadScreen scrollpad_screen;
} NcSongInfoScreen;

void nc_song_info_screen_init(NcSongInfoScreen *screen,
                              NcScreenCallbacks callbacks, void *user,
                              int64 start_x, int64 width,
                              int64 main_start_y, int64 main_height);
void nc_song_info_screen_set_geometry(NcSongInfoScreen *screen,
                                      int64 start_x, int64 width,
                                      int64 main_start_y,
                                      int64 main_height);
NcScreen *nc_song_info_screen_base(NcSongInfoScreen *screen);
int64 nc_song_info_screen_start_x(NcSongInfoScreen *screen);
int64 nc_song_info_screen_start_y(NcSongInfoScreen *screen);
int64 nc_song_info_screen_width(NcSongInfoScreen *screen);
int64 nc_song_info_screen_height(NcSongInfoScreen *screen);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_SONG_INFO_H */
