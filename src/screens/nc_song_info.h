#if !defined(NCMPCPP_NC_SONG_INFO_H)
#define NCMPCPP_NC_SONG_INFO_H

#include "curses/nc_buffer.h"
#include "curses/nc_scrollpad.h"
#include "screens/nc_scrollpad_screen.h"

typedef struct NcSongInfoScreen NcSongInfoScreen;

typedef struct NcSongInfoHooks {
    bool (*render)(void *user, NcSongInfoScreen *screen, NcBuffer *buffer);
    void (*switch_to)(void *user, NcSongInfoScreen *screen);
    void (*resize_layout)(void *user, NcSongInfoScreen *screen);
    void (*destroy)(void *user);
    void *user;
} NcSongInfoHooks;

struct NcSongInfoScreen {
    NcScrollpadScreen scrollpad_screen;
    NcWindow window;
    NcScrollpad scrollpad;
    NcBuffer buffer;
    NcSongInfoHooks hooks;

    int32 lines_scrolled;
};

void nc_song_info_screen_init(NcSongInfoScreen *screen,
                              NcSongInfoHooks hooks,
                              int32 start_x, int32 width,
                              int32 main_start_y, int32 main_height,
                              NcColor color, NcBorder border,
                              int32 lines_scrolled);
void nc_song_info_screen_set_geometry(NcSongInfoScreen *screen,
                                      int32 start_x, int32 width,
                                      int32 main_start_y,
                                      int32 main_height);
bool nc_song_info_screen_prepare_current(NcSongInfoScreen *screen);
NcScreen *nc_song_info_screen_base(NcSongInfoScreen *screen);
int32 nc_song_info_screen_start_x(NcSongInfoScreen *screen);
int32 nc_song_info_screen_start_y(NcSongInfoScreen *screen);
int32 nc_song_info_screen_width(NcSongInfoScreen *screen);
int32 nc_song_info_screen_height(NcSongInfoScreen *screen);

#endif /* NCMPCPP_NC_SONG_INFO_H */
