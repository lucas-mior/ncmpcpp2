#if !defined(NCMPCPP_NC_LYRICS_H)
#define NCMPCPP_NC_LYRICS_H

#include <stdbool.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_fs.h"
#include "c/ncm_job.h"
#include "c/ncm_song.h"
#include "curses/nc_buffer.h"
#include "curses/nc_scrollpad.h"
#include "curses/nc_window.h"
#include "lyrics_fetcher.h"
#include "screens/nc_scrollpad_screen.h"

typedef struct NcLyricsScreen {
    NcScrollpadScreen scrollpad_screen;

    int64 scroll_begin;
    bool refresh_window;
} NcLyricsScreen;

typedef struct NativeLyricsQueuedSong {
    NcmSong song;
    bool notify;
} NativeLyricsQueuedSong;

typedef struct NativeLyricsScreen {
    NcLyricsScreen screen;
    NcWindow window;
    NcScrollpad scrollpad;
    NcBuffer display;
    NcmBuffer search_constraint;

    NcmBuffer title;
    NcmSong song;
    NcmBuffer filename;
    NcmLyricsResult result;
    NcmJobQueue jobs;
    NativeLyricsQueuedSong *queued_songs;
    NcmBuffer consumer_message;

    NcmLyricsFetcherDef *fetcher;
    int32 queued_songs_len;
    int32 queued_songs_cap;

    bool has_song;
    bool initialized;
} NativeLyricsScreen;

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

void native_lyrics_queued_song_init(NativeLyricsQueuedSong *queued);
void native_lyrics_queued_song_destroy(NativeLyricsQueuedSong *queued);
bool native_lyrics_queued_song_copy(NativeLyricsQueuedSong *dest,
                                    NativeLyricsQueuedSong *source);
void native_lyrics_queued_song_move(NativeLyricsQueuedSong *dest,
                                    NativeLyricsQueuedSong *source);

void native_lyrics_screen_init(NativeLyricsScreen *screen,
                               int64 start_x, int64 width,
                               int64 main_start_y, int64 main_height,
                               NcColor color, NcBorder border,
                               int32 lines_scrolled);
void native_lyrics_screen_destroy(NativeLyricsScreen *screen);
NcLyricsScreen *native_lyrics_screen_typed(NativeLyricsScreen *screen);
NcScreen *native_lyrics_screen_base(NativeLyricsScreen *screen);
NcWindow *native_lyrics_screen_window(NativeLyricsScreen *screen);
void native_lyrics_screen_set_geometry(NativeLyricsScreen *screen,
                                       int64 start_x, int64 width,
                                       int64 main_start_y,
                                       int64 main_height);
bool native_lyrics_screen_build_filename(NativeLyricsScreen *screen,
                                         NcmSong *song,
                                         char *music_dir,
                                         int32 music_dir_len,
                                         char *lyrics_dir,
                                         int32 lyrics_dir_len,
                                         bool store_in_song_dir,
                                         bool win32_filename);
bool native_lyrics_screen_load_file(NativeLyricsScreen *screen,
                                    char *filename, int32 filename_len,
                                    NcmError *error);
bool native_lyrics_screen_save_file(NativeLyricsScreen *screen,
                                    char *filename, int32 filename_len,
                                    char *lyrics, int32 lyrics_len,
                                    NcmError *error);
bool native_lyrics_screen_fetch(NativeLyricsScreen *screen,
                                NcmSong *song,
                                NcmLyricsFetcherDef *fetcher,
                                NcmError *error);
bool native_lyrics_screen_fetch_in_background(NativeLyricsScreen *screen,
                                              NcmSong *song,
                                              bool notify,
                                              NcmError *error);
int32 native_lyrics_screen_dispatch_jobs(NativeLyricsScreen *screen);
void native_lyrics_screen_update(NativeLyricsScreen *screen);
void native_lyrics_screen_stop_downloads(NativeLyricsScreen *screen);
void native_lyrics_screen_clear_worker(NativeLyricsScreen *screen);
void native_lyrics_screen_refetch_current(NativeLyricsScreen *screen,
                                          NcmError *error);
NcmLyricsFetcherDef *native_lyrics_screen_toggle_fetcher(
    NativeLyricsScreen *screen, NcmLyricsFetcherRegistry *registry);
bool native_lyrics_screen_try_take_consumer_message(
    NativeLyricsScreen *screen, NcmBuffer *message);
NcmSong *native_lyrics_screen_song(NativeLyricsScreen *screen);
NcmLyricsResult *native_lyrics_screen_result(NativeLyricsScreen *screen);
NcmBuffer *native_lyrics_screen_filename(NativeLyricsScreen *screen);
NcBuffer *native_lyrics_screen_display(NativeLyricsScreen *screen);
bool native_lyrics_buffer_find(NcBuffer *buffer, char *pattern,
                               int32 pattern_len, NcmError *error);
bool native_lyrics_screen_find(NativeLyricsScreen *screen,
                               char *pattern, int32 pattern_len,
                               NcmError *error);
int32 native_lyrics_screen_pending_jobs(NativeLyricsScreen *screen);
int32 native_lyrics_screen_completed_jobs(NativeLyricsScreen *screen);
int32 native_lyrics_screen_queued_count(NativeLyricsScreen *screen);

#endif /* NCMPCPP_NC_LYRICS_H */
