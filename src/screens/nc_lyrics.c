#include "screens/nc_lyrics.h"

void
nc_lyrics_screen_init(NcLyricsScreen *screen,
                      NcScreenCallbacks callbacks, void *user,
                      int64 start_x, int64 width,
                      int64 main_start_y, int64 main_height) {
    nc_scrollpad_screen_init(&screen->scrollpad_screen,
                             callbacks,
                             user,
                             0,
                             0,
                             0,
                             0,
                             0);
    screen->scroll_begin = 0;
    screen->refresh_window = false;
    nc_lyrics_screen_set_geometry(screen,
                                  start_x,
                                  width,
                                  main_start_y,
                                  main_height);
    return;
}

void
nc_lyrics_screen_set_geometry(NcLyricsScreen *screen,
                              int64 start_x, int64 width,
                              int64 main_start_y, int64 main_height) {
    nc_scrollpad_screen_set_main_area(&screen->scrollpad_screen,
                                      start_x,
                                      width,
                                      main_start_y,
                                      main_height);
    return;
}

NcScreen *
nc_lyrics_screen_base(NcLyricsScreen *screen) {
    return nc_scrollpad_screen_base(&screen->scrollpad_screen);
}

int64
nc_lyrics_screen_start_x(NcLyricsScreen *screen) {
    return nc_scrollpad_screen_start_x(&screen->scrollpad_screen);
}

int64
nc_lyrics_screen_start_y(NcLyricsScreen *screen) {
    return nc_scrollpad_screen_start_y(&screen->scrollpad_screen);
}

int64
nc_lyrics_screen_width(NcLyricsScreen *screen) {
    return nc_scrollpad_screen_width(&screen->scrollpad_screen);
}

int64
nc_lyrics_screen_height(NcLyricsScreen *screen) {
    return nc_scrollpad_screen_height(&screen->scrollpad_screen);
}

void
nc_lyrics_screen_request_refresh(NcLyricsScreen *screen) {
    screen->refresh_window = true;
    return;
}

bool
nc_lyrics_screen_take_refresh_request(NcLyricsScreen *screen) {
    bool result = screen->refresh_window;

    screen->refresh_window = false;
    return result;
}

void
nc_lyrics_screen_reset_scroll_begin(NcLyricsScreen *screen) {
    screen->scroll_begin = 0;
    return;
}

int64
nc_lyrics_screen_scroll_begin(NcLyricsScreen *screen) {
    return screen->scroll_begin;
}

void
nc_lyrics_screen_set_scroll_begin(NcLyricsScreen *screen,
                                  int64 scroll_begin) {
    screen->scroll_begin = scroll_begin;
    return;
}
