#if !defined(NCMPCPP_NC_LYRICS_H)
#define NCMPCPP_NC_LYRICS_H

#include "cbase.h"

#include "c/ncm_app_arrays.h"
#include "c/ncm_fs.h"
#include "c/ncm_job.h"
#include "c/ncm_lrc.h"
#include "c/ncm_song.h"
#include "curses/nc_buffer.h"
#include "curses/nc_scrollpad.h"
#include "curses/nc_window.h"
#include "lyrics_fetcher.h"
#include "screens/nc_scrollpad_screen.h"

typedef struct LyricsJob LyricsJob;

typedef enum LyricsMode {
    LYRICS_MODE_PLAIN,
    LYRICS_MODE_SYNCHRONIZED,
    LYRICS_MODE_FETCH_LOG,
} LyricsMode;

typedef struct NcLyricsScreen {
    NcScrollpadScreen scrollpad_screen;

    int32 scroll_begin;
    bool refresh_window;
} NcLyricsScreen;

typedef struct LyricsQueuedSong {
    NcmSong song;
    bool notify;
} LyricsQueuedSong;

typedef struct LyricsScreen {
    NcLyricsScreen screen;
    NcWindow window;
    NcScrollpad scrollpad;
    NcBuffer display;
    StrBuilder search_constraint;

    StrBuilder title;
    NcmSong song;
    StrBuilder filename;
    NcmLrcDocument lrc;
    NcmLyricsResult result;
    NcmJobQueue jobs;
    LyricsJob *foreground_job;
    LyricsQueuedSong *queued_songs;
    StrBuilder consumer_message;

    NcmLyricsFetcherDef *fetcher;
    int32 queued_songs_len;
    int32 queued_songs_cap;
    int32 active_lrc_line;
    LyricsMode mode;

    bool has_song;
    bool initialized;
} LyricsScreen;

void nc_lyrics_screen_init(NcLyricsScreen *screen,
                           NcScreenOps callbacks, void *user,
                           int32 start_x, int32 width,
                           int32 main_start_y, int32 main_height);
void nc_lyrics_screen_set_geometry(NcLyricsScreen *screen,
                                   int32 start_x, int32 width,
                                   int32 main_start_y, int32 main_height);
NcScreen *nc_lyrics_screen_base(NcLyricsScreen *screen);
int32 nc_lyrics_screen_start_x(NcLyricsScreen *screen);
int32 nc_lyrics_screen_start_y(NcLyricsScreen *screen);
int32 nc_lyrics_screen_width(NcLyricsScreen *screen);
int32 nc_lyrics_screen_height(NcLyricsScreen *screen);
void nc_lyrics_screen_request_refresh(NcLyricsScreen *screen);
bool nc_lyrics_screen_take_refresh_request(NcLyricsScreen *screen);
void nc_lyrics_screen_reset_scroll_begin(NcLyricsScreen *screen);
int32 nc_lyrics_screen_scroll_begin(NcLyricsScreen *screen);
void nc_lyrics_screen_set_scroll_begin(NcLyricsScreen *screen,
                                       int32 scroll_begin);

void lyrics_queued_song_init(LyricsQueuedSong *queued);
void lyrics_queued_song_destroy(LyricsQueuedSong *queued);
void lyrics_queued_song_move(LyricsQueuedSong *dest,
                                    LyricsQueuedSong *source);

void lyrics_screen_init(LyricsScreen *screen,
                               int32 start_x, int32 width,
                               int32 main_start_y, int32 main_height,
                               NcColor color, NcBorder border,
                               int32 lines_scrolled);
void lyrics_screen_destroy(LyricsScreen *screen);
NcScreen *lyrics_screen_base(LyricsScreen *screen);
NcWindow *lyrics_screen_window(LyricsScreen *screen);
void lyrics_screen_set_geometry(LyricsScreen *screen,
                                       int32 start_x, int32 width,
                                       int32 main_start_y,
                                       int32 main_height);
bool lyrics_screen_build_filename(LyricsScreen *screen,
                                         NcmSong *song,
                                         char *music_dir,
                                         int32 music_dir_len,
                                         char *lyrics_dir,
                                         int32 lyrics_dir_len,
                                         bool store_in_song_dir,
                                         bool win32_filename);
bool lyrics_screen_load_file(LyricsScreen *screen,
                                    char *filename, int32 filename_len,
                                    NcmError *ncm_error);
bool lyrics_screen_save_file(LyricsScreen *screen,
                                    char *filename, int32 filename_len,
                                    char *lyrics, int32 lyrics_len,
                                    NcmError *ncm_error);
bool lyrics_screen_fetch(LyricsScreen *screen,
                                NcmSong *song,
                                NcmLyricsFetcherDef *fetcher,
                                NcmError *ncm_error);
bool lyrics_screen_fetch_in_background(LyricsScreen *screen,
                                              NcmSong *song,
                                              bool notify,
                                              NcmError *ncm_error);
int32 lyrics_screen_dispatch_jobs(LyricsScreen *screen);
void lyrics_screen_update(LyricsScreen *screen);
void lyrics_screen_refetch_current(LyricsScreen *screen,
                                          NcmError *ncm_error);
NcmLyricsFetcherDef *lyrics_screen_toggle_fetcher(
    LyricsScreen *screen, NcmLyricsFetcherRegistry *registry);
bool lyrics_screen_try_take_consumer_message(
    LyricsScreen *screen, StrBuilder *message);
NcmSong *lyrics_screen_song(LyricsScreen *screen);
StrBuilder *lyrics_screen_filename(LyricsScreen *screen);
LyricsMode lyrics_screen_mode(LyricsScreen *screen);
NcmLrcDocument *lyrics_screen_lrc(LyricsScreen *screen);
int32 lyrics_screen_active_lrc_line(LyricsScreen *screen);
bool lyrics_buffer_find(NcBuffer *buffer, char *pattern,
                               int32 pattern_len, NcmError *ncm_error);
void lyrics_buffer_clear_sync_highlight(NcBuffer *buffer);
void lyrics_buffer_highlight_sync_line(NcBuffer *buffer,
                                              int32 start, int32 end);
bool lyrics_screen_find(LyricsScreen *screen,
                               char *pattern, int32 pattern_len,
                               NcmError *ncm_error);

#endif /* NCMPCPP_NC_LYRICS_H */
