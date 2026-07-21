#if !defined(NCMPCPP_NC_LASTFM_H)
#define NCMPCPP_NC_LASTFM_H

#include <stdbool.h>

#include "c/ncm_job.h"
#include "curses/nc_buffer.h"
#include "curses/nc_scrollpad.h"
#include "curses/nc_window.h"
#include "lastfm_service.h"
#include "screens/nc_scrollpad_screen.h"

typedef struct NcLastfmScreen {
    NcScrollpadScreen scrollpad_screen;
} NcLastfmScreen;

typedef struct NativeLastfmScreen {
    NcLastfmScreen screen;
    NcWindow window;
    NcScrollpad scrollpad;
    NcBuffer buffer;
    StrBuilder search_constraint;

    NcmLastfmService service;
    NcmLastfmResult result;
    NcmJobQueue jobs;

    char *title;
    int32 title_len;
    int32 title_cap;

    bool has_service;
    bool refresh_window;
    bool initialized;
} NativeLastfmScreen;

void nc_lastfm_screen_init(NcLastfmScreen *screen,
                           NcScreenCallbacks callbacks, void *user,
                           int32 start_x, int32 width,
                           int32 main_start_y, int32 main_height);
void nc_lastfm_screen_set_geometry(NcLastfmScreen *screen,
                                   int32 start_x, int32 width,
                                   int32 main_start_y,
                                   int32 main_height);
NcScreen *nc_lastfm_screen_base(NcLastfmScreen *screen);
int32 nc_lastfm_screen_start_x(NcLastfmScreen *screen);
int32 nc_lastfm_screen_start_y(NcLastfmScreen *screen);
int32 nc_lastfm_screen_width(NcLastfmScreen *screen);
int32 nc_lastfm_screen_height(NcLastfmScreen *screen);

void native_lastfm_screen_init(NativeLastfmScreen *screen,
                               int32 start_x, int32 width,
                               int32 main_start_y, int32 main_height,
                               NcColor color, NcBorder border,
                               int32 lines_scrolled);
void native_lastfm_screen_destroy(NativeLastfmScreen *screen);
NcScreen *native_lastfm_screen_base(NativeLastfmScreen *screen);
NcWindow *native_lastfm_screen_window(NativeLastfmScreen *screen);
void native_lastfm_screen_set_geometry(NativeLastfmScreen *screen,
                                       int32 start_x, int32 width,
                                       int32 main_start_y,
                                       int32 main_height);
bool native_lastfm_screen_queue_artist_info(NativeLastfmScreen *screen,
                                            char *artist, int32 artist_len,
                                            char *lang, int32 lang_len,
                                            NcmError *error);
int32 native_lastfm_screen_dispatch_jobs(NativeLastfmScreen *screen);
void native_lastfm_screen_update(NativeLastfmScreen *screen);
char *native_lastfm_screen_title(NativeLastfmScreen *screen);
bool native_lastfm_screen_take_refresh_request(NativeLastfmScreen *screen);
bool native_lastfm_buffer_find(NcBuffer *buffer, char *pattern,
                                int32 pattern_len, NcmError *error);
bool native_lastfm_screen_find(NativeLastfmScreen *screen,
                               char *pattern, int32 pattern_len,
                               NcmError *error);

#endif /* NCMPCPP_NC_LASTFM_H */
