#if !defined(NCMPCPP_NC_LYRICS_H)
#define NCMPCPP_NC_LYRICS_H

#include <stdbool.h>

#include "screens/nc_scrollpad_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcLyricsScreen {
    NcScrollpadScreen scrollpad_screen;

    int64 scroll_begin;
    bool refresh_window;
} NcLyricsScreen;

void nc_lyrics_screen_init(NcLyricsScreen *screen,
                           NcScreenCallbacks callbacks, void *user,
                           int64 start_x, int64 width,
                           int64 main_start_y, int64 main_height);
void nc_lyrics_screen_set_geometry(NcLyricsScreen *screen,
                                   int64 start_x, int64 width,
                                   int64 main_start_y, int64 main_height);
NcScreen *nc_lyrics_screen_base(NcLyricsScreen *screen);
int64 nc_lyrics_screen_start_x(NcLyricsScreen *screen);
int64 nc_lyrics_screen_start_y(NcLyricsScreen *screen);
int64 nc_lyrics_screen_width(NcLyricsScreen *screen);
int64 nc_lyrics_screen_height(NcLyricsScreen *screen);
void nc_lyrics_screen_request_refresh(NcLyricsScreen *screen);
bool nc_lyrics_screen_take_refresh_request(NcLyricsScreen *screen);
void nc_lyrics_screen_reset_scroll_begin(NcLyricsScreen *screen);
int64 nc_lyrics_screen_scroll_begin(NcLyricsScreen *screen);
void nc_lyrics_screen_set_scroll_begin(NcLyricsScreen *screen,
                                       int64 scroll_begin);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_LYRICS_H */
