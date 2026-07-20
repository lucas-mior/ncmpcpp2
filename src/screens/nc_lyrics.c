#if !defined(NCMPCPP_NC_LYRICS_C)
#define NCMPCPP_NC_LYRICS_C

#include "screens/nc_lyrics.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "c/ncm_base.h"
#include "c/ncm_charset.h"
#include "c/ncm_format.h"
#include "c/ncm_regex.h"
#include "c/ncm_string.h"
#include "cbase/utf8.c"
#include "cbase/base_macros.h"
#include "cbase/util.c"
#include "curses/nc_cyclic_buffer.h"
#include "global.h"
#include "settings.h"
#include "screens/screen_switcher.h"
#include "statusbar.h"
#include "title.h"
#include "ui_state.h"

#define NATIVE_LYRICS_TITLE "Lyrics"
#define NATIVE_LYRICS_PROPERTY_ID 0x4c59524943534649ULL

typedef struct NativeLyricsJob {
    NativeLyricsScreen *screen;
    NcmSong song;
    NcmBuffer filename;
    NcmLyricsFetcherDef *fetcher;
    NcmLyricsResult result;
    NcBuffer log;
    bool notify;
    bool background;
} NativeLyricsJob;

typedef struct NativeLyricsFindState {
    NcBuffer *buffer;
} NativeLyricsFindState;

static NcWindow *lyrics_active_window_callback(NcScreen *screen);
static void lyrics_refresh_callback(NcScreen *screen);
static void lyrics_refresh_window_callback(NcScreen *screen);
static void lyrics_scroll_callback(NcScreen *screen, enum NcScroll where);
static void lyrics_switch_to_callback(NcScreen *screen);
static void lyrics_resize_callback(NcScreen *screen);
static int32 lyrics_window_timeout_callback(NcScreen *screen);
static char *lyrics_title_callback(NcScreen *screen);
static void lyrics_update_callback(NcScreen *screen);
static void lyrics_mouse_button_pressed_callback(NcScreen *screen,
                                                 MEVENT event);
static bool lyrics_is_lockable_callback(NcScreen *screen);
static bool lyrics_is_mergable_callback(NcScreen *screen);
static void lyrics_destroy_callback(NcScreen *screen);
static void native_lyrics_title_song_string(NcmSong *song,
                                            NcmBuffer *title);
static void native_lyrics_replace_search_separators(NcmBuffer *buffer);
static void native_lyrics_append_locale(NcBuffer *buffer, char *data,
                                        int32 data_len);
static void native_lyrics_report_unlink_error(NcmBuffer *filename,
                                              NcmError *error);
static void native_lyrics_remove_extension(NcmBuffer *buffer);
static bool native_lyrics_filename_from_song(NcmBuffer *filename,
                                             NcmSong *song,
                                             char *music_dir,
                                             int32 music_dir_len,
                                             char *lyrics_dir,
                                             int32 lyrics_dir_len,
                                             bool store_in_song_dir,
                                             bool win32_filename);
static bool native_lyrics_queue_song(NativeLyricsScreen *screen,
                                     NcmSong *song, bool notify);
static NativeLyricsJob *native_lyrics_job_create(NativeLyricsScreen *screen,
                                                 NcmSong *song,
                                                 NcmLyricsFetcherDef *fetcher,
                                                 bool notify,
                                                 bool background);
static NcmLyricsFetcherDef *native_lyrics_active_fetcher(
    NativeLyricsScreen *screen, NcmLyricsFetcherDef *fetcher);
static void native_lyrics_append_fetching(NcBuffer *buffer,
                                          NcmLyricsFetcherDef *fetcher);
static void native_lyrics_append_fetch_error(NcBuffer *buffer,
                                             NcmLyricsResult *result);
static bool native_lyrics_job_is_current(NativeLyricsJob *job);
static bool native_lyrics_job_run(void *user, NcmError *error);
static void native_lyrics_job_complete(bool success, NcmError *error,
                                       void *user);
static void native_lyrics_job_destroy(void *user);
static bool native_lyrics_start_next_background(NativeLyricsScreen *screen,
                                                NcmError *error);
static bool native_lyrics_find_match_callback(int32 start, int32 len,
                                              void *user);
static void native_lyrics_mouse_scroll(NativeLyricsScreen *screen,
                                       enum NcScroll where);
static void native_lyrics_display(NativeLyricsScreen *screen);

void
nc_lyrics_screen_init(NcLyricsScreen *screen,
                      NcScreenCallbacks callbacks, void *user,
                      int32 start_x, int32 width,
                      int32 main_start_y, int32 main_height) {
    nc_scrollpad_screen_init(&screen->scrollpad_screen,
                             callbacks,
                             user,
                             NC_SCREEN_TYPE_LYRICS,
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
                              int32 start_x, int32 width,
                              int32 main_start_y, int32 main_height) {
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

int32
nc_lyrics_screen_start_x(NcLyricsScreen *screen) {
    return nc_scrollpad_screen_start_x(&screen->scrollpad_screen);
}

int32
nc_lyrics_screen_start_y(NcLyricsScreen *screen) {
    return nc_scrollpad_screen_start_y(&screen->scrollpad_screen);
}

int32
nc_lyrics_screen_width(NcLyricsScreen *screen) {
    return nc_scrollpad_screen_width(&screen->scrollpad_screen);
}

int32
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

int32
nc_lyrics_screen_scroll_begin(NcLyricsScreen *screen) {
    return screen->scroll_begin;
}

void
nc_lyrics_screen_set_scroll_begin(NcLyricsScreen *screen,
                                  int32 scroll_begin) {
    screen->scroll_begin = scroll_begin;
    return;
}

void
native_lyrics_queued_song_init(NativeLyricsQueuedSong *queued) {
    ncm_song_init(&queued->song);
    queued->notify = false;
    return;
}

void
native_lyrics_queued_song_destroy(NativeLyricsQueuedSong *queued) {
    ncm_song_destroy(&queued->song);
    queued->notify = false;
    return;
}

void
native_lyrics_queued_song_move(NativeLyricsQueuedSong *dest,
                               NativeLyricsQueuedSong *source) {
    ncm_song_move(&dest->song, &source->song);
    dest->notify = source->notify;
    source->notify = false;
    return;
}

static NcScreenCallbacks
native_lyrics_callbacks(void) {
    NcScreenCallbacks callbacks = {0};

    callbacks.active_window = lyrics_active_window_callback;
    callbacks.refresh = lyrics_refresh_callback;
    callbacks.refresh_window = lyrics_refresh_window_callback;
    callbacks.scroll = lyrics_scroll_callback;
    callbacks.switch_to = lyrics_switch_to_callback;
    callbacks.resize = lyrics_resize_callback;
    callbacks.window_timeout = lyrics_window_timeout_callback;
    callbacks.title = lyrics_title_callback;
    callbacks.update = lyrics_update_callback;
    callbacks.mouse_button_pressed = lyrics_mouse_button_pressed_callback;
    callbacks.is_lockable = lyrics_is_lockable_callback;
    callbacks.is_mergable = lyrics_is_mergable_callback;
    callbacks.destroy = lyrics_destroy_callback;
    return callbacks;
}

void
native_lyrics_screen_init(NativeLyricsScreen *screen,
                          int32 start_x, int32 width,
                          int32 main_start_y, int32 main_height,
                          NcColor color, NcBorder border,
                          int32 lines_scrolled) {
    nc_lyrics_screen_init(&screen->screen,
                          native_lyrics_callbacks(),
                          screen,
                          start_x,
                          width,
                          main_start_y,
                          main_height);
    nc_window_init(&screen->window,
                   nc_lyrics_screen_start_x(&screen->screen),
                   nc_lyrics_screen_start_y(&screen->screen),
                   nc_lyrics_screen_width(&screen->screen),
                   nc_lyrics_screen_height(&screen->screen),
                   STRLIT_ARGS(""),
                   color,
                   border);
    nc_scrollpad_init(&screen->scrollpad,
                      nc_lyrics_screen_height(&screen->screen));
    nc_buffer_init(&screen->display);
    ncm_buffer_init(&screen->search_constraint);
    ncm_buffer_init(&screen->title);
    ncm_song_init(&screen->song);
    ncm_buffer_init(&screen->filename);
    ncm_lyrics_result_init(&screen->result);
    ncm_job_queue_init(&screen->jobs);
    screen->queued_songs = NULL;
    screen->queued_songs_len = 0;
    screen->queued_songs_cap = 0;
    ncm_buffer_init(&screen->consumer_message);
    screen->fetcher = NULL;
    screen->has_song = false;
    screen->initialized = true;
    nc_window_set_timeout(&screen->window, lines_scrolled);
    return;
}

void
native_lyrics_screen_destroy(NativeLyricsScreen *screen) {
    if (!screen->initialized) {
        return;
    }

    ncm_job_queue_destroy(&screen->jobs);
    for (int32 i = 0; i < screen->queued_songs_len; i += 1) {
        native_lyrics_queued_song_destroy(&screen->queued_songs[i]);
    }
    if (screen->queued_songs) {
        free2(screen->queued_songs,
            screen->queued_songs_cap*SIZEOF(*screen->queued_songs));
    }
    ncm_buffer_destroy(&screen->consumer_message);
    ncm_lyrics_result_destroy(&screen->result);
    ncm_buffer_destroy(&screen->filename);
    ncm_song_destroy(&screen->song);
    ncm_buffer_destroy(&screen->title);
    ncm_buffer_destroy(&screen->search_constraint);
    nc_buffer_destroy(&screen->display);
    nc_window_destroy(&screen->window);
    screen->queued_songs = NULL;
    screen->queued_songs_len = 0;
    screen->queued_songs_cap = 0;
    screen->fetcher = NULL;
    screen->has_song = false;
    screen->initialized = false;
    return;
}

NcScreen *
native_lyrics_screen_base(NativeLyricsScreen *screen) {
    return nc_lyrics_screen_base(&screen->screen);
}

NcWindow *
native_lyrics_screen_window(NativeLyricsScreen *screen) {
    return &screen->window;
}

void
native_lyrics_screen_set_geometry(NativeLyricsScreen *screen,
                                  int32 start_x, int32 width,
                                  int32 main_start_y, int32 main_height) {
    nc_lyrics_screen_set_geometry(&screen->screen,
                                  start_x,
                                  width,
                                  main_start_y,
                                  main_height);
    nc_window_resize(&screen->window,
                     nc_lyrics_screen_width(&screen->screen),
                     nc_lyrics_screen_height(&screen->screen));
    nc_window_move_to(&screen->window,
                      nc_lyrics_screen_start_x(&screen->screen),
                      nc_lyrics_screen_start_y(&screen->screen));
    nc_scrollpad_resize(&screen->scrollpad,
                        &screen->window,
                        nc_lyrics_screen_width(&screen->screen),
                        nc_lyrics_screen_height(&screen->screen));
    return;
}

bool
native_lyrics_screen_build_filename(NativeLyricsScreen *screen,
                                    NcmSong *song,
                                    char *music_dir,
                                    int32 music_dir_len,
                                    char *lyrics_dir,
                                    int32 lyrics_dir_len,
                                    bool store_in_song_dir,
                                    bool win32_filename) {
    (void)screen;
    return native_lyrics_filename_from_song(&screen->filename,
                                            song,
                                            music_dir,
                                            music_dir_len,
                                            lyrics_dir,
                                            lyrics_dir_len,
                                            store_in_song_dir,
                                            win32_filename);
}

bool
native_lyrics_screen_load_file(NativeLyricsScreen *screen,
                               char *filename, int32 filename_len,
                               NcmError *error) {
    FILE *file;
    char line[1024];
    int32 line_len;
    bool first;

    if ((filename == NULL) || (filename_len <= 0)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing lyrics file"));
        return false;
    }

    if ((file = fopen(filename, "rb")) == NULL) {
        ncm_error_set(error, errno, STRLIT_ARGS("failed to open lyrics"));
        return false;
    }

    nc_buffer_clear(&screen->display);
    first = true;
    while (fgets(line, SIZEOF(line), file)) {
        line_len = strlen32(line);
        ncm_string_remove_chars(line, &line_len, STRLIT_ARGS("\r\n"));
        if (!first) {
            nc_buffer_append_char(&screen->display, '\n');
        }
        native_lyrics_append_locale(&screen->display, line, line_len);
        first = false;
    }
    fclose(file);
    nc_lyrics_screen_request_refresh(&screen->screen);
    ncm_error_clear(error);
    return true;
}

bool
native_lyrics_screen_save_file(NativeLyricsScreen *screen,
                               char *filename, int32 filename_len,
                               char *lyrics, int32 lyrics_len,
                               NcmError *error) {
    FILE *file;
    int32 written;

    (void)screen;
    if ((filename == NULL) || (filename_len <= 0)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing lyrics file"));
        return false;
    }

    if ((file = fopen(filename, "wb")) == NULL) {
        ncm_error_set(error, errno, STRLIT_ARGS("failed to write lyrics"));
        return false;
    }

    written = 0;
    if (lyrics && (lyrics_len > 0)) {
        written = (int32)fwrite64(lyrics, 1, lyrics_len, file);
    }
    if ((written != lyrics_len) || (fclose(file) != 0)) {
        ncm_error_set(error, errno, STRLIT_ARGS("failed to save lyrics"));
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
native_lyrics_screen_fetch(NativeLyricsScreen *screen,
                           NcmSong *song,
                           NcmLyricsFetcherDef *fetcher,
                           NcmError *error) {
    NativeLyricsJob *job;
    NcmLyricsFetcherDef *active_fetcher;
    NcmBuffer next_filename;
    bool changed_song;
    bool changed_filename;
    bool changed;
    bool win32_filename;

    if ((screen == NULL) || (song == NULL) || ncm_song_empty(song)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing song"));
        return false;
    }

    ncm_buffer_init(&next_filename);
    win32_filename = Config.generate_win32_compatible_filenames;
    if (!native_lyrics_filename_from_song(&next_filename,
                                          song,
                                          Config.mpd_music_dir,
                                          Config.mpd_music_dir_len,
                                          Config.lyrics_directory,
                                          Config.lyrics_directory_len,
                                          Config.store_lyrics_in_song_dir,
                                          win32_filename)) {
        ncm_buffer_destroy(&next_filename);
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("failed to build lyrics filename"));
        return false;
    }

    changed_song = !screen->has_song || !ncm_song_equal(&screen->song, song);
    changed_filename = !STREQUAL(screen->filename.data,
                                         screen->filename.len,
                                         next_filename.data,
                                         next_filename.len);
    changed = changed_song || changed_filename;
    if (changed) {
        nc_buffer_clear(&screen->display);
        nc_scrollpad_reset(&screen->scrollpad);
        nc_lyrics_screen_reset_scroll_begin(&screen->screen);
        ncm_lyrics_result_clear(&screen->result);
        ncm_song_copy(&screen->song, song);
        screen->has_song = true;
        ncm_buffer_copy(&screen->filename, &next_filename);
    }
    ncm_buffer_destroy(&next_filename);

    if (!changed) {
        ncm_error_clear(error);
        return true;
    }

    if (native_lyrics_screen_load_file(screen,
                                       screen->filename.data,
                                       screen->filename.len,
                                       error)) {
        ncm_error_clear(error);
        return true;
    }

    nc_buffer_clear(&screen->display);
    if ((active_fetcher = native_lyrics_active_fetcher(screen, fetcher))) {
        native_lyrics_append_fetching(&screen->display, active_fetcher);
    } else if (Config.lyrics_fetchers.fetchers.len > 0) {
        native_lyrics_append_fetching(
            &screen->display, &Config.lyrics_fetchers.fetchers.items[0]);
    }
    nc_lyrics_screen_request_refresh(&screen->screen);

    if (!ncm_job_queue_start(&screen->jobs, error)) {
        return false;
    }

    job = native_lyrics_job_create(screen, song, active_fetcher, false, false);
    if (!ncm_job_queue_push(&screen->jobs,
                            (NcmJob){
                                .run = native_lyrics_job_run,
                                .complete = native_lyrics_job_complete,
                                .destroy = native_lyrics_job_destroy,
                                .user = job,
                            },
                            error)) {
        native_lyrics_job_destroy(job);
        return false;
    }
    ncm_error_clear(error);
    return true;
}

bool
native_lyrics_screen_fetch_in_background(NativeLyricsScreen *screen,
                                         NcmSong *song,
                                         bool notify,
                                         NcmError *error) {
    if ((screen == NULL) || (song == NULL) || ncm_song_empty(song)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing song"));
        return false;
    }
    if (!native_lyrics_queue_song(screen, song, notify)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("failed to queue song"));
        return false;
    }
    if (!native_lyrics_start_next_background(screen, error)) {
        return false;
    }
    ncm_error_clear(error);
    return true;
}

int32
native_lyrics_screen_dispatch_jobs(NativeLyricsScreen *screen) {
    int32 result;

    result = ncm_job_queue_dispatch_completed(&screen->jobs);
    (void)native_lyrics_start_next_background(screen, NULL);
    return result;
}

void
native_lyrics_screen_update(NativeLyricsScreen *screen) {
    native_lyrics_screen_dispatch_jobs(screen);
    if (nc_lyrics_screen_take_refresh_request(&screen->screen)) {
        nc_scrollpad_flush(&screen->scrollpad,
                           &screen->window,
                           &screen->display);
        native_lyrics_display(screen);
    }
    return;
}

void
native_lyrics_screen_refetch_current(NativeLyricsScreen *screen,
                                     NcmError *error) {
    if (!screen->has_song) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("no current song"));
        return;
    }
    if ((screen->filename.len > 0)
        && !ncm_fs_unlink(screen->filename.data,
                          screen->filename.len,
                          error)) {
        native_lyrics_report_unlink_error(&screen->filename, error);
        return;
    }
    screen->has_song = false;
    (void)native_lyrics_screen_fetch(screen,
                                     &screen->song,
                                     screen->fetcher,
                                     error);
    return;
}

NcmLyricsFetcherDef *
native_lyrics_screen_toggle_fetcher(NativeLyricsScreen *screen,
                                    NcmLyricsFetcherRegistry *registry) {
    int32 idx;

    if ((registry == NULL) || (registry->fetchers.len <= 0)) {
        screen->fetcher = NULL;
        return NULL;
    }
    if (screen->fetcher == NULL) {
        screen->fetcher = &registry->fetchers.items[0];
        return screen->fetcher;
    }

    idx = -1;
    for (int32 i = 0; i < registry->fetchers.len; i += 1) {
        if (&registry->fetchers.items[i] == screen->fetcher) {
            idx = i;
            break;
        }
    }
    idx += 1;
    if ((idx >= 0) && (idx < registry->fetchers.len)) {
        screen->fetcher = &registry->fetchers.items[idx];
    } else {
        screen->fetcher = NULL;
    }
    return screen->fetcher;
}

bool
native_lyrics_screen_try_take_consumer_message(NativeLyricsScreen *screen,
                                               NcmBuffer *message) {
    if ((screen == NULL) || (message == NULL)) {
        return false;
    }
    if (screen->consumer_message.len <= 0) {
        return false;
    }
    ncm_buffer_copy(message, &screen->consumer_message);
    ncm_buffer_clear(&screen->consumer_message);
    return true;
}

NcmSong *
native_lyrics_screen_song(NativeLyricsScreen *screen) {
    if (!screen->has_song) {
        return NULL;
    }
    return &screen->song;
}

NcmBuffer *
native_lyrics_screen_filename(NativeLyricsScreen *screen) {
    return &screen->filename;
}

bool
native_lyrics_buffer_find(NcBuffer *buffer, char *pattern,
                          int32 pattern_len, NcmError *error) {
    NativeLyricsFindState state;
    NcmRegex regex;
    char *data;
    bool result;

    if (buffer == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing lyrics buffer"));
        return false;
    }

    nc_buffer_remove_properties(buffer, NATIVE_LYRICS_PROPERTY_ID);
    if ((pattern == NULL) || (pattern_len <= 0)) {
        ncm_error_clear(error);
        return true;
    }

    ncm_regex_init(&regex);
    if (!ncm_regex_compile(&regex, pattern, pattern_len, Config.regex_flags,
                           error)) {
        ncm_regex_destroy(&regex);
        return false;
    }

    state.buffer = buffer;
    data = nc_buffer_data(buffer);
    result = ncm_regex_for_each_match(&regex,
                                      data,
                                      buffer->len,
                                      native_lyrics_find_match_callback,
                                      &state);
    ncm_regex_destroy(&regex);
    return result;
}

bool
native_lyrics_screen_find(NativeLyricsScreen *screen,
                          char *pattern, int32 pattern_len,
                          NcmError *error) {
    bool result;

    if (screen == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing lyrics screen"));
        return false;
    }

    result = native_lyrics_buffer_find(&screen->display, pattern,
                                       pattern_len, error);
    if ((pattern == NULL) || (pattern_len <= 0)) {
        ncm_buffer_clear(&screen->search_constraint);
    } else if (!ncm_error_is_set(error)) {
        (void)ncm_buffer_set(&screen->search_constraint, pattern,
                             pattern_len);
    }
    nc_scrollpad_flush(&screen->scrollpad, &screen->window,
                       &screen->display);
    native_lyrics_display(screen);
    return result;
}

static NativeLyricsScreen *
lyrics_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcWindow *
lyrics_active_window_callback(NcScreen *screen) {
    return native_lyrics_screen_window(lyrics_from_screen(screen));
}

static void
lyrics_refresh_callback(NcScreen *screen) {
    native_lyrics_display(lyrics_from_screen(screen));
    return;
}

static void
lyrics_refresh_window_callback(NcScreen *screen) {
    native_lyrics_display(lyrics_from_screen(screen));
    return;
}

static void
lyrics_scroll_callback(NcScreen *screen, enum NcScroll where) {
    NativeLyricsScreen *lyrics;

    lyrics = lyrics_from_screen(screen);
    nc_scrollpad_scroll(&lyrics->scrollpad, &lyrics->window, where);
    return;
}

static void
lyrics_switch_to_callback(NcScreen *screen) {
    char *title;

    nc_lyrics_screen_reset_scroll_begin(&lyrics_from_screen(screen)->screen);
    if ((title = nc_screen_title(screen)) == NULL) {
        title = "";
    }
    ncm_title_draw_header(title, strlen32(title));
    return;
}

static void
lyrics_resize_callback(NcScreen *screen) {
    NativeLyricsScreen *lyrics;
    int32 x;
    int32 width;

    lyrics = lyrics_from_screen(screen);
    nc_screen_switcher_get_resize_params(screen, &x, &width, true);
    native_lyrics_screen_set_geometry(lyrics,
                                      x,
                                      width,
                                      ui_state_main_start_y(),
                                      ui_state_main_height());
    nc_screen_clear_resize_request(screen);
    return;
}

static int32
lyrics_window_timeout_callback(NcScreen *screen) {
    (void)screen;
    return 500;
}

static char *
lyrics_title_callback(NcScreen *screen) {
    NativeLyricsScreen *lyrics;
    NcmBuffer song_title;
    NcmBuffer scroll_buffer;
    int32 scroll_begin;
    int32 scroll_width;
    char separator[] = " ** ";

    lyrics = lyrics_from_screen(screen);
    ncm_buffer_clear(&lyrics->title);
    ncm_buffer_append(&lyrics->title, STRLIT_ARGS(NATIVE_LYRICS_TITLE));
    if (!lyrics->has_song || ncm_song_empty(&lyrics->song)) {
        return lyrics->title.data;
    }

    ncm_buffer_init(&song_title);
    native_lyrics_title_song_string(&lyrics->song, &song_title);
    if (song_title.len <= 0) {
        ncm_buffer_destroy(&song_title);
        return lyrics->title.data;
    }

    ncm_buffer_append(&lyrics->title, STRLIT_ARGS(": "));
    scroll_begin = nc_lyrics_screen_scroll_begin(&lyrics->screen);
    scroll_width = COLS - utf8_width(lyrics->title.data,
                                         lyrics->title.len);
    if (Config.design == NCM_DESIGN_ALTERNATIVE) {
        scroll_width -= 2;
    } else {
        scroll_width -= global_volume_state_len();
    }

    ncm_buffer_init(&scroll_buffer);
    nc_cyclic_text_write(&scroll_buffer, song_title.data, song_title.len,
                         &scroll_begin, scroll_width, separator,
                         SIZEOF(separator) - 1,
                         Config.header_text_scrolling);
    ncm_buffer_append(&lyrics->title, scroll_buffer.data, scroll_buffer.len);
    nc_lyrics_screen_set_scroll_begin(&lyrics->screen, scroll_begin);
    ncm_buffer_destroy(&scroll_buffer);
    ncm_buffer_destroy(&song_title);
    return lyrics->title.data;
}

static void
lyrics_update_callback(NcScreen *screen) {
    native_lyrics_screen_update(lyrics_from_screen(screen));
    return;
}

static void
lyrics_mouse_button_pressed_callback(NcScreen *screen, MEVENT event) {
    NativeLyricsScreen *lyrics;

    lyrics = lyrics_from_screen(screen);
    if ((event.bstate & BUTTON5_PRESSED) != 0) {
        native_lyrics_mouse_scroll(lyrics, NC_SCROLL_DOWN);
    } else if ((event.bstate & BUTTON4_PRESSED) != 0) {
        native_lyrics_mouse_scroll(lyrics, NC_SCROLL_UP);
    }
    return;
}

static bool
lyrics_is_lockable_callback(NcScreen *screen) {
    (void)screen;
    return true;
}

static bool
lyrics_is_mergable_callback(NcScreen *screen) {
    (void)screen;
    return true;
}

static void
lyrics_destroy_callback(NcScreen *screen) {
    native_lyrics_screen_destroy(lyrics_from_screen(screen));
    return;
}

static void
native_lyrics_title_song_string(NcmSong *song, NcmBuffer *title) {
    NcmStringView artist_view;
    NcmStringView title_view;
    NcmStringView name_view;

    ncm_string_view_init(&artist_view);
    ncm_string_view_init(&title_view);
    ncm_string_view_init(&name_view);
    ncm_buffer_clear(title);
    if (ncm_song_tag_view(song, MPD_TAG_ARTIST, 0, &artist_view)
        && ncm_song_tag_view(song, MPD_TAG_TITLE, 0, &title_view)) {
        ncm_buffer_append(title, artist_view.data, artist_view.len);
        ncm_buffer_append(title, STRLIT_ARGS(" - "));
        ncm_buffer_append(title, title_view.data, title_view.len);
        return;
    }

    if (ncm_song_name_view(song, 0, &name_view)) {
        ncm_buffer_append(title, name_view.data, name_view.len);
    }
    return;
}

static bool
native_lyrics_song_artist_title(NcmSong *song,
                                NcmBuffer *artist,
                                NcmBuffer *title) {
    NcmBuffer fallback;
    NcmStringView artist_view;
    NcmStringView title_view;
    NcmStringView name_view;

    ncm_string_view_init(&artist_view);
    ncm_string_view_init(&title_view);
    ncm_string_view_init(&name_view);
    ncm_buffer_clear(artist);
    ncm_buffer_clear(title);
    if (ncm_song_tag_view(song, MPD_TAG_ARTIST, 0, &artist_view)
        && ncm_song_tag_view(song, MPD_TAG_TITLE, 0, &title_view)) {
        ncm_buffer_append(artist, artist_view.data, artist_view.len);
        ncm_buffer_append(title, title_view.data, title_view.len);
        return true;
    }

    ncm_buffer_init(&fallback);
    if (ncm_song_name_view(song, 0, &name_view)) {
        ncm_buffer_append(&fallback, name_view.data, name_view.len);
    } else if (ncm_song_uri_view(song, 0, &name_view)) {
        ncm_buffer_append(&fallback, name_view.data, name_view.len);
    }
    native_lyrics_remove_extension(&fallback);
    ncm_buffer_copy(title, &fallback);
    ncm_buffer_destroy(&fallback);
    return title->len > 0;
}

static bool
native_lyrics_fetch_artist_title(NcmSong *song, NcmBuffer *artist,
                                 NcmBuffer *title) {
    if (!native_lyrics_song_artist_title(song, artist, title)) {
        return false;
    }
    if (artist->len <= 0) {
        native_lyrics_replace_search_separators(title);
    }
    return true;
}

static void
native_lyrics_replace_search_separators(NcmBuffer *buffer) {
    for (int32 i = 0; i < buffer->len; i += 1) {
        if ((buffer->data[i] == '-') || (buffer->data[i] == '_')) {
            buffer->data[i] = ' ';
        }
    }
    return;
}

static void
native_lyrics_append_locale(NcBuffer *buffer, char *data, int32 data_len) {
    NcmBuffer converted;

    converted = ncm_charset_utf8_to_locale(data, data_len);
    nc_buffer_append_data(buffer, converted.data, converted.len);
    ncm_buffer_destroy(&converted);
    return;
}

static void
native_lyrics_report_save_error(NcmBuffer *filename, NcmError *error) {
    NcmStringFormatArg args[2];
    char *message;

    message = "unknown error";
    if (error && (error->code != 0)) {
        message = strerror(error->code);
    }
    args[0] = ncm_string_format_arg_string(filename->data, filename->len);
    args[1] = ncm_string_format_arg_cstring(message);
    ncm_statusbar_format(Config.message_delay_time,
                         STRLIT_ARGS("Couldn't save lyrics as \"%1%\": %2%"),
                         args, NCM_ARRAY_LEN(args));
    return;
}

static void
native_lyrics_report_unlink_error(NcmBuffer *filename, NcmError *error) {
    NcmStringFormatArg args[2];
    char *message;

    message = "unknown error";
    if (error && (error->code != 0)) {
        message = strerror(error->code);
    }
    args[0] = ncm_string_format_arg_string(filename->data, filename->len);
    args[1] = ncm_string_format_arg_cstring(message);
    ncm_statusbar_format(Config.message_delay_time,
                         STRLIT_ARGS("Couldn't remove \"%1%\": %2%"),
                         args, NCM_ARRAY_LEN(args));
    return;
}

static void
native_lyrics_remove_extension(NcmBuffer *buffer) {
    for (int32 i = buffer->len - 1; i >= 0; i -= 1) {
        if (buffer->data[i] == '/') {
            break;
        }
        if (buffer->data[i] == '.') {
            buffer->len = i;
            buffer->data[i] = '\0';
            break;
        }
    }
    return;
}

static bool
native_lyrics_filename_from_song(NcmBuffer *filename,
                                 NcmSong *song,
                                 char *music_dir,
                                 int32 music_dir_len,
                                 char *lyrics_dir,
                                 int32 lyrics_dir_len,
                                 bool store_in_song_dir,
                                 bool win32_filename) {
    NcmBuffer artist;
    NcmBuffer title;
    NcmStringView uri;
    int32 basename_start;
    int32 basename_len;

    ncm_buffer_init(&artist);
    ncm_buffer_init(&title);
    ncm_string_view_init(&uri);
    ncm_buffer_clear(filename);
    if (store_in_song_dir && !ncm_song_is_stream(song)) {
        if (ncm_song_is_from_database(song) && (music_dir_len > 0)) {
            ncm_buffer_append(filename, music_dir, music_dir_len);
            if ((filename->len > 0)
                && (filename->data[filename->len - 1] != '/')) {
                ncm_buffer_append_byte(filename, '/');
            }
        }
        if (ncm_song_uri_view(song, 0, &uri)) {
            ncm_buffer_append(filename, uri.data, uri.len);
        }
        native_lyrics_remove_extension(filename);
    } else {
        (void)native_lyrics_song_artist_title(song, &artist, &title);
        if (lyrics_dir_len > 0) {
            ncm_buffer_append(filename, lyrics_dir, lyrics_dir_len);
            if ((filename->len > 0)
                && (filename->data[filename->len - 1] != '/')) {
                ncm_buffer_append_byte(filename, '/');
            }
        }
        basename_start = filename->len;
        if ((artist.len > 0) && (title.len > 0)) {
            ncm_buffer_append(filename, artist.data, artist.len);
            ncm_buffer_append(filename, STRLIT_ARGS(" - "));
            ncm_buffer_append(filename, title.data, title.len);
        } else {
            ncm_buffer_append(filename, title.data, title.len);
        }
        basename_len = filename->len - basename_start;
        if (basename_len > 0) {
            ncm_string_remove_invalid_filename_chars(
                filename->data + basename_start,
                &basename_len,
                win32_filename);
            filename->len = basename_start + basename_len;
            filename->data[filename->len] = '\0';
        }
    }
    ncm_buffer_append(filename, STRLIT_ARGS(".txt"));
    ncm_buffer_destroy(&title);
    ncm_buffer_destroy(&artist);
    return filename->len > 4;
}

static bool
native_lyrics_queue_song(NativeLyricsScreen *screen,
                         NcmSong *song, bool notify) {
    NativeLyricsQueuedSong *queued;
    int32 new_cap;

    if ((song == NULL) || ncm_song_empty(song)) {
        return false;
    }
    if (screen->queued_songs_len >= screen->queued_songs_cap) {
        new_cap = screen->queued_songs_cap;
        if (new_cap <= 0) {
            new_cap = 8;
        } else {
            new_cap *= 2;
        }
        screen->queued_songs = realloc2(
            screen->queued_songs,
            screen->queued_songs_cap,
            new_cap,
            SIZEOF(*screen->queued_songs));
        for (int32 i = screen->queued_songs_cap; i < new_cap; i += 1) {
            native_lyrics_queued_song_init(&screen->queued_songs[i]);
        }
        screen->queued_songs_cap = new_cap;
    }
    queued = &screen->queued_songs[screen->queued_songs_len];
    ncm_song_copy(&queued->song, song);
    queued->notify = notify;
    screen->queued_songs_len += 1;
    return true;
}

static NativeLyricsQueuedSong *
native_lyrics_dequeue_song(NativeLyricsScreen *screen) {
    NativeLyricsQueuedSong *queued;

    if (screen->queued_songs_len <= 0) {
        return NULL;
    }
    queued = malloc2(SIZEOF(*queued));
    native_lyrics_queued_song_init(queued);
    native_lyrics_queued_song_move(queued, &screen->queued_songs[0]);
    for (int32 i = 1; i < screen->queued_songs_len; i += 1) {
        native_lyrics_queued_song_move(&screen->queued_songs[i - 1],
                                       &screen->queued_songs[i]);
    }
    screen->queued_songs_len -= 1;
    return queued;
}

static NativeLyricsJob *
native_lyrics_job_create(NativeLyricsScreen *screen,
                         NcmSong *song,
                         NcmLyricsFetcherDef *fetcher,
                         bool notify,
                         bool background) {
    NativeLyricsJob *job;
    bool win32_filename;

    job = malloc2(SIZEOF(*job));
    job->screen = screen;
    ncm_song_init(&job->song);
    ncm_song_copy(&job->song, song);
    ncm_buffer_init(&job->filename);
    nc_buffer_init(&job->log);
    win32_filename = Config.generate_win32_compatible_filenames;
    (void)native_lyrics_filename_from_song(&job->filename,
                                           song,
                                           Config.mpd_music_dir,
                                           Config.mpd_music_dir_len,
                                           Config.lyrics_directory,
                                           Config.lyrics_directory_len,
                                           Config.store_lyrics_in_song_dir,
                                           win32_filename);
    job->fetcher = fetcher;
    ncm_lyrics_result_init(&job->result);
    job->notify = notify;
    job->background = background;
    return job;
}

static bool
native_lyrics_job_fetch_one(NativeLyricsJob *job,
                            NcmLyricsFetcherDef *fetcher,
                            NcmBuffer *artist,
                            NcmBuffer *title) {
    if (fetcher == NULL) {
        return false;
    }

    native_lyrics_append_fetching(&job->log, fetcher);
    if (!ncm_lyrics_fetcher_fetch(fetcher,
                                  &job->result,
                                  artist->data,
                                  artist->len,
                                  title->data,
                                  title->len)) {
        native_lyrics_append_fetch_error(&job->log, &job->result);
        return false;
    }
    if (!job->result.success) {
        native_lyrics_append_fetch_error(&job->log, &job->result);
        return false;
    }
    return true;
}

static bool
native_lyrics_job_fetch(NativeLyricsJob *job,
                        NcmBuffer *artist,
                        NcmBuffer *title) {
    if (job->fetcher) {
        return native_lyrics_job_fetch_one(job,
                                           job->fetcher,
                                           artist,
                                           title);
    }

    for (int32 i = 0; i < Config.lyrics_fetchers.fetchers.len; i += 1) {
        if (native_lyrics_job_fetch_one(
                job, &Config.lyrics_fetchers.fetchers.items[i],
                artist, title)) {
            return true;
        }
    }
    return false;
}

static bool
native_lyrics_job_run(void *user, NcmError *error) {
    NativeLyricsJob *job;
    NcmBuffer artist;
    NcmBuffer title;
    bool success;

    job = user;
    ncm_buffer_init(&artist);
    ncm_buffer_init(&title);
    if (!native_lyrics_fetch_artist_title(&job->song, &artist, &title)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing song metadata"));
        ncm_buffer_destroy(&title);
        ncm_buffer_destroy(&artist);
        return false;
    }
    success = native_lyrics_job_fetch(job, &artist, &title);
    ncm_buffer_destroy(&title);
    ncm_buffer_destroy(&artist);
    if (!success || !job->result.success) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("lyrics not found"));
    }
    return success && job->result.success;
}

static void
native_lyrics_job_complete(bool success, NcmError *error, void *user) {
    NativeLyricsJob *job;
    NativeLyricsScreen *screen;

    (void)success;
    (void)error;
    job = user;
    screen = job->screen;
    if (!job->background) {
        if (!native_lyrics_job_is_current(job)) {
            return;
        }

        ncm_lyrics_result_clear(&screen->result);
        (void)ncm_lyrics_result_set(&screen->result,
                                    job->result.success,
                                    job->result.text,
                                    job->result.text_len);
        nc_buffer_clear(&screen->display);
        if (job->result.success) {
            NcmError save_error = {0};

            native_lyrics_append_locale(&screen->display,
                                        job->result.text,
                                        job->result.text_len);
            ncm_buffer_copy(&screen->filename, &job->filename);
            if (!native_lyrics_screen_save_file(screen,
                                                job->filename.data,
                                                job->filename.len,
                                                job->result.text,
                                                job->result.text_len,
                                                &save_error)) {
                native_lyrics_report_save_error(&job->filename, &save_error);
            }
            ncm_error_clear(&save_error);
        } else {
            nc_buffer_destroy(&screen->display);
            nc_buffer_copy(&screen->display, &job->log);
            nc_buffer_append_cstring(&screen->display,
                                     "\nLyrics were not found.\n");
        }
        nc_lyrics_screen_request_refresh(&screen->screen);
    } else {
        if (job->result.success) {
            NcmError save_error = {0};

            (void)native_lyrics_screen_save_file(screen,
                                                 job->filename.data,
                                                 job->filename.len,
                                                 job->result.text,
                                                 job->result.text_len,
                                                 &save_error);
            ncm_error_clear(&save_error);
        }
    }
    return;
}

static void
native_lyrics_job_destroy(void *user) {
    NativeLyricsJob *job;

    job = user;
    if (job == NULL) {
        return;
    }
    ncm_song_destroy(&job->song);
    ncm_buffer_destroy(&job->filename);
    ncm_lyrics_result_destroy(&job->result);
    nc_buffer_destroy(&job->log);
    free2(job, SIZEOF(*job));
    return;
}

static NcmLyricsFetcherDef *
native_lyrics_active_fetcher(NativeLyricsScreen *screen,
                             NcmLyricsFetcherDef *fetcher) {
    if (fetcher) {
        return fetcher;
    }
    return screen->fetcher;
}

static void
native_lyrics_append_fetching(NcBuffer *buffer,
                              NcmLyricsFetcherDef *fetcher) {
    char *name;
    int32 name_len;
    int32 fetcher_position;

    if (fetcher == NULL) {
        return;
    }

    name = ncm_lyrics_fetcher_name(fetcher);
    name_len = ncm_lyrics_fetcher_name_len(fetcher);
    nc_buffer_append_cstring(buffer, "Fetching lyrics from ");
    fetcher_position = nc_buffer_len(buffer);
    nc_buffer_add_format(buffer,
                         fetcher_position,
                         NC_FORMAT_BOLD,
                         NATIVE_LYRICS_PROPERTY_ID);
    nc_buffer_append_data(buffer, name, name_len);
    nc_buffer_add_format(buffer,
                         nc_buffer_len(buffer),
                         NC_FORMAT_NO_BOLD,
                         NATIVE_LYRICS_PROPERTY_ID);
    nc_buffer_append_cstring(buffer, "... ");
    return;
}

static void
native_lyrics_append_fetch_error(NcBuffer *buffer,
                                 NcmLyricsResult *result) {
    NcColor red;

    red = nc_color_make(COLOR_RED, NC_COLOR_CURRENT, false, false);
    nc_buffer_add_color(buffer,
                        nc_buffer_len(buffer),
                        red,
                        NATIVE_LYRICS_PROPERTY_ID);
    nc_buffer_append_data(buffer, result->text, result->text_len);
    nc_buffer_add_color(buffer,
                        nc_buffer_len(buffer),
                        nc_color_end(),
                        NATIVE_LYRICS_PROPERTY_ID);
    nc_buffer_append_char(buffer, '\n');
    return;
}

static bool
native_lyrics_job_is_current(NativeLyricsJob *job) {
    NativeLyricsScreen *screen;

    if (job == NULL) {
        return false;
    }
    screen = job->screen;
    if ((screen == NULL) || !screen->has_song) {
        return false;
    }
    if (!ncm_song_equal(&screen->song, &job->song)) {
        return false;
    }
    return STREQUAL(screen->filename.data,
                            screen->filename.len,
                            job->filename.data,
                            job->filename.len);
}

static void
native_lyrics_set_consumer_fetch_message(NativeLyricsScreen *screen,
                                         NcmSong *song) {
    NcmBuffer formatted;

    formatted = ncm_format_render_string(&Config.song_status_format, song);
    ncm_buffer_clear(&screen->consumer_message);
    ncm_buffer_append(&screen->consumer_message,
                      STRLIT_ARGS("Fetching lyrics for \""));
    ncm_buffer_append(&screen->consumer_message,
                      formatted.data,
                      formatted.len);
    ncm_buffer_append(&screen->consumer_message, STRLIT_ARGS("\"..."));
    ncm_buffer_destroy(&formatted);
    return;
}

static bool
native_lyrics_find_match_callback(int32 start, int32 len, void *user) {
    NativeLyricsFindState *state;

    if (len <= 0) {
        return true;
    }

    state = user;
    nc_buffer_add_format(state->buffer, start, NC_FORMAT_REVERSE,
                         NATIVE_LYRICS_PROPERTY_ID);
    nc_buffer_add_format(state->buffer, start + len, NC_FORMAT_NO_REVERSE,
                         NATIVE_LYRICS_PROPERTY_ID);
    return true;
}

static void
native_lyrics_mouse_scroll(NativeLyricsScreen *screen,
                           enum NcScroll where) {
    for (int32 i = 0; i < Config.lines_scrolled; i += 1) {
        nc_scrollpad_scroll(&screen->scrollpad, &screen->window, where);
    }
    return;
}

static void
native_lyrics_display(NativeLyricsScreen *screen) {
    nc_window_refresh_border(&screen->window);
    nc_scrollpad_refresh(&screen->scrollpad, &screen->window);
    return;
}

static bool
native_lyrics_start_next_background(NativeLyricsScreen *screen,
                                    NcmError *error) {
    NativeLyricsQueuedSong *queued;
    NativeLyricsJob *job;
    NcmBuffer filename;
    bool win32_filename;
    bool found_job;

    if (ncm_job_queue_pending_count(&screen->jobs) > 0) {
        ncm_error_clear(error);
        return true;
    }
    if (ncm_job_queue_completed_count(&screen->jobs) > 0) {
        ncm_error_clear(error);
        return true;
    }

    ncm_buffer_init(&filename);
    found_job = false;
    queued = NULL;
    win32_filename = Config.generate_win32_compatible_filenames;
    while (!found_job) {
        if ((queued = native_lyrics_dequeue_song(screen)) == NULL) {
            ncm_buffer_destroy(&filename);
            ncm_error_clear(error);
            return true;
        }

        if (ncm_song_is_stream(&queued->song)) {
            native_lyrics_queued_song_destroy(queued);
            free2(queued, SIZEOF(*queued));
            queued = NULL;
            continue;
        }

        if (!native_lyrics_filename_from_song(&filename,
                                              &queued->song,
                                              Config.mpd_music_dir,
                                              Config.mpd_music_dir_len,
                                              Config.lyrics_directory,
                                              Config.lyrics_directory_len,
                                              Config.store_lyrics_in_song_dir,
                                              win32_filename)) {
            native_lyrics_queued_song_destroy(queued);
            free2(queued, SIZEOF(*queued));
            queued = NULL;
            continue;
        }

        if (ncm_fs_exists(filename.data, filename.len)) {
            native_lyrics_queued_song_destroy(queued);
            free2(queued, SIZEOF(*queued));
            queued = NULL;
            continue;
        }

        found_job = true;
    }

    if (!ncm_job_queue_start(&screen->jobs, error)) {
        ncm_buffer_destroy(&filename);
        native_lyrics_queued_song_destroy(queued);
        free2(queued, SIZEOF(*queued));
        return false;
    }

    if (queued->notify) {
        native_lyrics_set_consumer_fetch_message(screen, &queued->song);
    }
    job = native_lyrics_job_create(screen,
                                   &queued->song,
                                   screen->fetcher,
                                   queued->notify,
                                   true);
    if (!ncm_job_queue_push(&screen->jobs,
                            (NcmJob){
                                .run = native_lyrics_job_run,
                                .complete = native_lyrics_job_complete,
                                .destroy = native_lyrics_job_destroy,
                                .user = job,
                            },
                            error)) {
        native_lyrics_job_destroy(job);
        ncm_buffer_destroy(&filename);
        native_lyrics_queued_song_destroy(queued);
        free2(queued, SIZEOF(*queued));
        return false;
    }

    ncm_buffer_destroy(&filename);
    native_lyrics_queued_song_destroy(queued);
    free2(queued, SIZEOF(*queued));
    ncm_error_clear(error);
    return true;
}

#endif /* NCMPCPP_NC_LYRICS_C */
