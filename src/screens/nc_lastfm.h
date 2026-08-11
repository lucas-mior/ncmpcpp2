#if !defined(NCMPCPP_NC_LASTFM_H)
#define NCMPCPP_NC_LASTFM_H

#include "cbase.h"

#include "c/ncm_job.h"
#include "curses/nc_buffer.h"
#include "curses/nc_scrollpad.h"
#include "curses/nc_window.h"
#include "lastfm_service.h"
#include "screens/nc_scrollpad_screen.h"

typedef struct NcLastfmScreen {
    NcScrollpadScreen scrollpad_screen;
} NcLastfmScreen;

typedef struct LastfmScreen {
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
} LastfmScreen;

void nc_lastfm_screen_init(NcLastfmScreen *screen,
                           NcScreenOps callbacks, void *user,
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

void lastfm_screen_init(LastfmScreen *screen,
                               int32 start_x, int32 width,
                               int32 main_start_y, int32 main_height,
                               NcColor color, NcBorder border,
                               int32 lines_scrolled);
void lastfm_screen_destroy(LastfmScreen *screen);
NcScreen *lastfm_screen_base(LastfmScreen *screen);
NcWindow *lastfm_screen_window(LastfmScreen *screen);
void lastfm_screen_set_geometry(LastfmScreen *screen,
                                       int32 start_x, int32 width,
                                       int32 main_start_y,
                                       int32 main_height);
bool lastfm_screen_queue_artist_info(LastfmScreen *screen,
                                            char *artist, int32 artist_len,
                                            char *lang, int32 lang_len,
                                            NcmError *error);
int32 lastfm_screen_dispatch_jobs(LastfmScreen *screen);
void lastfm_screen_update(LastfmScreen *screen);
char *lastfm_screen_title(LastfmScreen *screen);
bool lastfm_screen_take_refresh_request(LastfmScreen *screen);
bool lastfm_buffer_find(NcBuffer *buffer, char *pattern,
                                int32 pattern_len, NcmError *error);
bool lastfm_screen_find(LastfmScreen *screen,
                               char *pattern, int32 pattern_len,
                               NcmError *error);

#endif /* NCMPCPP_NC_LASTFM_H */
