#if !defined(NCMPCPP_NC_LYRICS_C)
#define NCMPCPP_NC_LYRICS_C

#include "cbase.h"

#include "c/ncm_c.h"
#include "curses/nc_curses.h"
#include "global.h"
#include "screens/nc_screens.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "title.h"
#include "ui_state.h"

#define LYRICS_TITLE "Lyrics"
#define LYRICS_FETCH_PROPERTY_ID ((int64)0x4c59524645544348LL)
#define LYRICS_SEARCH_PROPERTY_ID ((int64)0x4c59525345415243LL)
#define LYRICS_SYNC_PROPERTY_ID ((int64)0x4c595253594e4321LL)
#define LYRICS_NO_ACTIVE_LINE (-1)
#define LYRICS_DEFAULT_TIMEOUT_MS NC_SCREEN_DEFAULT_WINDOW_TIMEOUT
#define LYRICS_SYNC_TIMEOUT_MIN_MS 25
#define LYRICS_SYNC_TIMEOUT_MAX_MS 100

struct LyricsJob {
    LyricsScreen *screen;
    NcmSong song;
    StrBuilder filename;
    NcmLyricsFetcherDef *fetcher;
    NcmLyricsResult result;
    NcBuffer log;
    pthread_mutex_t log_mutex;
    bool log_dirty;
    bool notify;
    bool background;
};

typedef struct LyricsFindState {
    NcBuffer *buffer;
} LyricsFindState;

static void lyrics_switch_to_callback(NcScreen *screen);
static void lyrics_resize_callback(NcScreen *screen);
static int32 lyrics_window_timeout_callback(NcScreen *screen);
static char *lyrics_title_callback(NcScreen *screen);
static void lyrics_update_callback(NcScreen *screen);
static void lyrics_mouse_button_pressed_callback(NcScreen *screen,
                                                 MEVENT event);
static void lyrics_title_song_string(NcmSong *song, StrBuilder *title);
static void lyrics_replace_search_separators(StrBuilder *buffer);
static void lyrics_append_locale(NcBuffer *buffer, char *data,
                                 int32 data_len);
static bool lyrics_screen_render_lrc(LyricsScreen *screen,
                                     NcmError *ncm_error);
static bool lyrics_screen_update_sync_line(LyricsScreen *screen);
static bool lyrics_screen_start_foreground_fetch(
    LyricsScreen *screen,
    NcmSong *song,
    NcmLyricsFetcherDef *fetcher,
    StrBuilder *filename,
    NcmError *ncm_error);
static bool lyrics_screen_update_sync_line_force(
    LyricsScreen *screen,
    bool force);
static void lyrics_screen_clear_sync_line(LyricsScreen *screen);
static int32 lyrics_screen_sync_timeout(LyricsScreen *screen);
static int32 lyrics_lrc_buffer_position(void *user);
static void lyrics_lrc_buffer_append(void *user,
                                     char *data, int32 data_len);
static void lyrics_report_sidecar_status(StrBuilder *lrc_filename,
                                         bool lrc_found,
                                         StrBuilder *txt_filename,
                                         bool txt_found);
static void lyrics_report_unlink_error(StrBuilder *filename,
                                       NcmError *ncm_error);
static void lyrics_screen_clear_lyrics_state(
    LyricsScreen *screen,
    LyricsMode mode);
static void lyrics_remove_extension(StrBuilder *buffer);
static bool lyrics_filename_from_song_with_extension(
    StrBuilder *filename,
    NcmSong *song,
    char *music_dir, int32 music_dir_len,
    char *lyrics_dir, int32 lyrics_dir_len,
    bool store_in_song_dir,
    bool win32_filename,
    char *extension, int32 extension_len);
static bool lyrics_filename_from_song(StrBuilder *filename,
                                      NcmSong *song,
                                      char *music_dir,
                                      int32 music_dir_len,
                                      char *lyrics_dir,
                                      int32 lyrics_dir_len,
                                      bool store_in_song_dir,
                                      bool win32_filename);
static bool lyrics_preferred_filename_from_song(StrBuilder *filename,
                                                NcmSong *song,
                                                char *music_dir,
                                                int32 music_dir_len,
                                                char *lyrics_dir,
                                                int32 lyrics_dir_len,
                                                bool store_in_song_dir,
                                                bool win32_filename);
static bool lyrics_queue_song(LyricsScreen *screen,
                              NcmSong *song, bool notify);
static LyricsJob *lyrics_job_create(LyricsScreen *screen,
                                    NcmSong *song,
                                    NcmLyricsFetcherDef *fetcher,
                                    bool notify,
                                    bool background);
static NcmLyricsFetcherDef *lyrics_active_fetcher(
    LyricsScreen *screen, NcmLyricsFetcherDef *fetcher);
static void lyrics_append_fetching(NcBuffer *buffer,
                                   NcmLyricsFetcherDef *fetcher);
static void lyrics_append_fetch_error(NcBuffer *buffer,
                                      NcmLyricsResult *result);
static void lyrics_job_append_fetching(LyricsJob *job,
                                       NcmLyricsFetcherDef *fetcher);
static void lyrics_job_append_fetch_error(LyricsJob *job,
                                          NcmLyricsResult *result);
static bool lyrics_job_take_log(LyricsJob *job,
                                NcBuffer *buffer);
static void lyrics_screen_update_progress(LyricsScreen *screen);
static bool lyrics_job_is_current(LyricsJob *job);
static bool lyrics_job_run(void *user, NcmError *ncm_error);
static void lyrics_job_complete(bool success, NcmError *ncm_error,
                                void *user);
static void lyrics_job_destroy(void *user);
static bool lyrics_start_next_background(LyricsScreen *screen,
                                         NcmError *ncm_error);
static bool lyrics_find_match_callback(int32 start, int32 len,
                                       void *user);
static void lyrics_mouse_scroll(LyricsScreen *screen,
                                enum NcScroll where);
static void lyrics_display(LyricsScreen *screen);

#define NC_SCREEN_IMPL_TYPE LyricsScreen
#define NC_SCREEN_IMPL_PREFIX lyrics
#define NC_SCREEN_IMPL_PUBLIC_PREFIX lyrics_screen
#define NC_SCREEN_IMPL_BASE_FIELD screen
#define NC_SCREEN_IMPL_SCROLLPAD_BASE screen.scrollpad_screen
#define NC_SCREEN_IMPL_NO_GEOMETRY_ACCESSORS
#define NC_SCREEN_IMPL_WINDOW_FIELD window
#define NC_SCREEN_IMPL_SCROLLPAD_FIELD scrollpad
#define NC_SCREEN_IMPL_REFRESH_CALLBACK lyrics_display
#define NC_SCREEN_IMPL_SWITCH_TO_CALLBACK lyrics_switch_to_callback
#define NC_SCREEN_IMPL_RESIZE_CALLBACK lyrics_resize_callback
#define NC_SCREEN_IMPL_TITLE_CALLBACK lyrics_title_callback
#define NC_SCREEN_IMPL_WINDOW_TIMEOUT_CALLBACK lyrics_window_timeout_callback
#define NC_SCREEN_IMPL_UPDATE_CALLBACK lyrics_update_callback
#define NC_SCREEN_IMPL_MOUSE_CALLBACK lyrics_mouse_button_pressed_callback
#define NC_SCREEN_IMPL_DESTROY_TYPED_CALLBACK lyrics_screen_destroy
#define NC_SCREEN_IMPL_LOCKABLE true
#define NC_SCREEN_IMPL_MERGABLE true
#include "screens/nc_screen_impl_template.h"

void
nc_lyrics_screen_init(NcLyricsScreen *screen,
                      NcScreenOps callbacks, void *user,
                      int32 start_x, int32 width,
                      int32 main_start_y, int32 main_height) {
    nc_scrollpad_screen_init(&screen->scrollpad_screen,
                             callbacks,
                             user,
                             NC_SCREEN_TYPE_LYRICS,
                             0, 0, 0, 0);
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
lyrics_queued_song_destroy(LyricsQueuedSong *queued) {
    ncm_song_destroy(&queued->song);
    queued->notify = false;
    return;
}

void
lyrics_queued_song_move(LyricsQueuedSong *dest,
                        LyricsQueuedSong *source) {
    ncm_song_move(&dest->song, &source->song);
    dest->notify = source->notify;
    source->notify = false;
    return;
}

void
lyrics_screen_init(LyricsScreen *screen,
                   int32 start_x, int32 width,
                   int32 main_start_y, int32 main_height,
                   NcColor color, NcBorder border,
                   int32 lines_scrolled) {
    nc_lyrics_screen_init(&screen->screen,
                          lyrics_ops,
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
                   STRLIT(""),
                   color,
                   border);
    nc_scrollpad_init(&screen->scrollpad,
                      nc_lyrics_screen_height(&screen->screen));
    screen->display = (NcBuffer){0};

    screen->search_constraint = (StrBuilder){0};
    screen->title = (StrBuilder){0};
    screen->song = (NcmSong){0};
    screen->filename = (StrBuilder){0};

    screen->lrc = (NcmLrcDocument){0};
    screen->result = (NcmLyricsResult){0};
    ncm_job_queue_init(&screen->jobs);
    screen->foreground_job = NULL;
    screen->queued_songs = NULL;
    screen->queued_songs_len = 0;
    screen->queued_songs_cap = 0;
    screen->active_lrc_line = LYRICS_NO_ACTIVE_LINE;

    screen->consumer_message = (StrBuilder){0};

    screen->fetcher = NULL;
    screen->mode = LYRICS_MODE_PLAIN;
    screen->has_song = false;
    screen->initialized = true;
    nc_window_set_timeout(&screen->window, lines_scrolled);

    return;
}

void
lyrics_screen_destroy(LyricsScreen *screen) {
    if (!screen->initialized) {
        return;
    }

    ncm_job_queue_destroy(&screen->jobs);
    for (int32 i = 0; i < screen->queued_songs_len; i += 1) {
        lyrics_queued_song_destroy(&screen->queued_songs[i]);
    }
    free2(screen->queued_songs,
          screen->queued_songs_cap*SIZEOF(*screen->queued_songs));

    sb_free(&screen->consumer_message);
    ncm_lyrics_result_destroy(&screen->result);
    ncm_lrc_document_destroy(&screen->lrc);
    sb_free(&screen->filename);
    ncm_song_destroy(&screen->song);
    sb_free(&screen->title);
    sb_free(&screen->search_constraint);
    nc_buffer_destroy(&screen->display);
    nc_window_destroy(&screen->window);

    screen->foreground_job = NULL;
    screen->queued_songs = NULL;
    screen->queued_songs_len = 0;
    screen->queued_songs_cap = 0;
    screen->fetcher = NULL;
    screen->mode = LYRICS_MODE_PLAIN;
    screen->has_song = false;
    screen->initialized = false;

    return;
}

NcWindow *
lyrics_screen_window(LyricsScreen *screen) {
    return &screen->window;
}

void
lyrics_screen_set_geometry(LyricsScreen *screen,
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
    if (screen->mode == LYRICS_MODE_SYNCHRONIZED) {
        (void)lyrics_screen_update_sync_line_force(screen, true);
    }
    nc_scrollpad_flush(&screen->scrollpad,
                       &screen->window,
                       &screen->display);
    nc_lyrics_screen_request_refresh(&screen->screen);
    return;
}

bool
lyrics_screen_build_filename(LyricsScreen *screen,
                             NcmSong *song,
                             char *music_dir, int32 music_dir_len,
                             char *lyrics_dir, int32 lyrics_dir_len,
                             bool store_in_song_dir,
                             bool win32_filename) {
    return lyrics_preferred_filename_from_song(&screen->filename,
                                               song,
                                               music_dir,
                                               music_dir_len,
                                               lyrics_dir,
                                               lyrics_dir_len,
                                               store_in_song_dir,
                                               win32_filename);
}

bool
lyrics_screen_load_file(LyricsScreen *screen,
                        char *filename, int32 filename_len,
                        NcmError *ncm_error) {
    FILE *file;
    StrBuilder raw = {0};
    char line[1024];
    int close_err;
    int32 line_len;
    bool first;
    bool lrc_file;

    if ((filename == NULL) || (filename_len <= 0)) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing lyrics file"));
        return false;
    }

    lrc_file = (filename_len > STRLIT_LEN(".lrc"))
               && STREQUAL(filename + filename_len - STRLIT_LEN(".lrc"),
                           STRLIT_LEN(".lrc"),
                           STRLIT(".lrc"));
    if ((file = fopen(filename, "rb")) == NULL) {
        lyrics_screen_clear_lyrics_state(screen, LYRICS_MODE_FETCH_LOG);
        ncm_error_set(ncm_error, errno, STRLIT("failed to open lyrics"));
        return false;
    }

    nc_buffer_clear(&screen->display);
    nc_scrollpad_reset(&screen->scrollpad);
    screen->active_lrc_line = LYRICS_NO_ACTIVE_LINE;
    first = true;
    while (fgets(line, SIZEOF(line), file)) {
        line_len = strlen32(line);
        if (lrc_file) {
            SB_APPEND(&raw, line, line_len);
            continue;
        }

        ncm_string_remove_chars(line, &line_len, STRLIT("\r\n"));
        if (!first) {
            nc_buffer_append_char(&screen->display, '\n');
        }
        lyrics_append_locale(&screen->display, line, line_len);
        first = false;
    }

    if ((close_err = XFCLOSE(file, filename)) < 0) {
        sb_free(&raw);
        lyrics_screen_clear_lyrics_state(
            screen, LYRICS_MODE_FETCH_LOG);
        ncm_error_set(ncm_error, -close_err, STRLIT("failed to close lyrics"));
        return false;
    }
    if (lrc_file
        && !ncm_lrc_parse(&screen->lrc, raw.data, raw.len, ncm_error)) {
        sb_free(&raw);
        lyrics_screen_clear_lyrics_state(
            screen, LYRICS_MODE_FETCH_LOG);
        return false;
    }

    if (lrc_file) {
        nc_buffer_clear(&screen->display);
        if (!lyrics_screen_render_lrc(screen, ncm_error)) {
            sb_free(&raw);
            lyrics_screen_clear_lyrics_state(
                screen, LYRICS_MODE_FETCH_LOG);
            return false;
        }
        screen->mode = LYRICS_MODE_SYNCHRONIZED;
    } else {
        ncm_lrc_document_clear(&screen->lrc);
        screen->mode = LYRICS_MODE_PLAIN;
    }
    sb_free(&raw);
    nc_lyrics_screen_request_refresh(&screen->screen);
    ncm_error_clear(ncm_error);

    return true;
}

bool
lyrics_screen_save_file(LyricsScreen *screen,
                        char *filename, int32 filename_len,
                        char *lyrics, int32 lyrics_len,
                        NcmError *ncm_error) {
    FILE *file;
    int32 written;
    int32 close_result;

    (void)screen;
    if ((filename == NULL) || (filename_len <= 0)) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing lyrics file"));
        return false;
    }
    if ((lyrics_len < 0) || ((lyrics == NULL) && (lyrics_len > 0))) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing lyrics buffer"));
        return false;
    }

    if ((file = fopen(filename, "wb")) == NULL) {
        ncm_error_set(ncm_error, errno, STRLIT("failed to write lyrics"));
        return false;
    }

    written = 0;
    if (lyrics && (lyrics_len > 0)) {
        written = (int32)fwrite64(lyrics, 1, lyrics_len, file);
    }
    close_result = fclose(file);
    if ((written != lyrics_len) || (close_result != 0)) {
        ncm_error_set(ncm_error, errno, STRLIT("failed to save lyrics"));
        return false;
    }

    ncm_error_clear(ncm_error);
    return true;
}

bool
lyrics_screen_fetch(LyricsScreen *screen,
                    NcmSong *song,
                    NcmLyricsFetcherDef *fetcher,
                    NcmError *ncm_error) {
    StrBuilder next_filename = {0};
    StrBuilder lrc_filename = {0};
    StrBuilder txt_filename = {0};
    bool changed_song;
    bool changed_filename;
    bool changed;
    bool lrc_found;
    bool txt_found;
    bool win32_filename;

    if ((screen == NULL) || (song == NULL) || ncm_song_empty(song)) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing song"));
        return false;
    }

    win32_filename = Config.generate_win32_compatible_filenames;
    if (!lyrics_filename_from_song_with_extension(
        &lrc_filename,
        song,
        Config.mpd_music_dir,
        Config.mpd_music_dir_len,
        Config.lyrics_directory,
        Config.lyrics_directory_len,
        Config.store_lyrics_in_song_dir,
        win32_filename,
        STRLIT(".lrc"))) {
        sb_free(&lrc_filename);
        ncm_error_set(ncm_error, EINVAL,
                      STRLIT("failed to build lyrics filename"));
        return false;
    }
    if (!lyrics_filename_from_song(&txt_filename,
                                   song,
                                   Config.mpd_music_dir,
                                   Config.mpd_music_dir_len,
                                   Config.lyrics_directory,
                                   Config.lyrics_directory_len,
                                   Config.store_lyrics_in_song_dir,
                                   win32_filename)) {
        sb_free(&txt_filename);
        sb_free(&lrc_filename);
        ncm_error_set(ncm_error, EINVAL,
                      STRLIT("failed to build lyrics filename"));
        return false;
    }
    lrc_found = ncm_fs_exists(lrc_filename.data, lrc_filename.len);
    txt_found = ncm_fs_exists(txt_filename.data, txt_filename.len);
    lyrics_report_sidecar_status(&lrc_filename, lrc_found,
                                 &txt_filename, txt_found);
    if (lrc_found) {
        sb_copy(&next_filename, &lrc_filename);
    } else {
        sb_copy(&next_filename, &txt_filename);
    }
    if (next_filename.len <= 0) {
        sb_free(&next_filename);
        sb_free(&txt_filename);
        sb_free(&lrc_filename);
        ncm_error_set(ncm_error, EINVAL,
                      STRLIT("failed to build lyrics filename"));
        return false;
    }

    changed_song = !screen->has_song || !ncm_song_equal(&screen->song, song);
    changed_filename = !STREQUAL(screen->filename.data,
                                 screen->filename.len,
                                 next_filename.data,
                                 next_filename.len);
    changed = changed_song || changed_filename;
    if (changed) {
        lyrics_screen_clear_lyrics_state(
            screen, LYRICS_MODE_FETCH_LOG);
        nc_scrollpad_reset(&screen->scrollpad);
        nc_lyrics_screen_reset_scroll_begin(&screen->screen);
        ncm_lyrics_result_clear(&screen->result);
        ncm_song_copy(&screen->song, song);
        screen->has_song = true;
        sb_copy(&screen->filename, &next_filename);
    }
    sb_free(&next_filename);

    if (!changed) {
        sb_free(&txt_filename);
        sb_free(&lrc_filename);
        ncm_error_clear(ncm_error);
        return true;
    }

    if (lyrics_screen_load_file(screen,
                                lrc_filename.data,
                                lrc_filename.len,
                                ncm_error)) {
        sb_copy(&screen->filename, &lrc_filename);
        sb_free(&txt_filename);
        sb_free(&lrc_filename);
        ncm_error_clear(ncm_error);
        return true;
    }
    if (lyrics_screen_load_file(screen,
                                txt_filename.data,
                                txt_filename.len,
                                ncm_error)) {
        sb_copy(&screen->filename, &txt_filename);
        sb_free(&txt_filename);
        sb_free(&lrc_filename);
        ncm_error_clear(ncm_error);
        return true;
    }

    if (!lyrics_screen_start_foreground_fetch(screen,
                                              song,
                                              fetcher,
                                              &txt_filename,
                                              ncm_error)) {
        sb_free(&txt_filename);
        sb_free(&lrc_filename);
        return false;
    }

    sb_free(&txt_filename);
    sb_free(&lrc_filename);
    ncm_error_clear(ncm_error);
    return true;
}

bool
lyrics_screen_fetch_in_background(LyricsScreen *screen,
                                  NcmSong *song,
                                  bool notify,
                                  NcmError *ncm_error) {
    if ((screen == NULL) || (song == NULL) || ncm_song_empty(song)) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing song"));
        return false;
    }
    if (!lyrics_queue_song(screen, song, notify)) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("failed to queue song"));
        return false;
    }
    if (!lyrics_start_next_background(screen, ncm_error)) {
        return false;
    }
    ncm_error_clear(ncm_error);
    return true;
}

int32
lyrics_screen_dispatch_jobs(LyricsScreen *screen) {
    int32 result = ncm_job_queue_dispatch_completed(&screen->jobs);
    (void)lyrics_start_next_background(screen, NULL);
    return result;
}

void
lyrics_screen_update(LyricsScreen *screen) {
    lyrics_screen_update_progress(screen);
    lyrics_screen_dispatch_jobs(screen);
    if (lyrics_screen_update_sync_line(screen)) {
        nc_lyrics_screen_request_refresh(&screen->screen);
    }
    if (nc_lyrics_screen_take_refresh_request(&screen->screen)) {
        nc_scrollpad_flush(&screen->scrollpad,
                           &screen->window,
                           &screen->display);
        lyrics_display(screen);
    }
    return;
}

void
lyrics_screen_refetch_current(LyricsScreen *screen,
                              NcmError *ncm_error) {
    StrBuilder filename = {0};
    bool win32_filename;

    if (!screen->has_song) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("no current song"));
        return;
    }

    win32_filename = Config.generate_win32_compatible_filenames;
    if (!lyrics_filename_from_song(&filename,
                                   &screen->song,
                                   Config.mpd_music_dir,
                                   Config.mpd_music_dir_len,
                                   Config.lyrics_directory,
                                   Config.lyrics_directory_len,
                                   Config.store_lyrics_in_song_dir,
                                   win32_filename)) {
        ncm_error_set(ncm_error, EINVAL,
                      STRLIT("failed to build lyrics filename"));
        sb_free(&filename);
        return;
    }
    if (!ncm_fs_unlink(filename.data, filename.len, ncm_error)) {
        lyrics_report_unlink_error(&filename, ncm_error);
        sb_free(&filename);
        return;
    }

    if (!lyrics_screen_start_foreground_fetch(screen,
                                              &screen->song,
                                              screen->fetcher,
                                              &filename,
                                              ncm_error)) {
        sb_free(&filename);
        return;
    }

    sb_free(&filename);
    ncm_error_clear(ncm_error);
    return;
}

NcmLyricsFetcherDef *
lyrics_screen_toggle_fetcher(LyricsScreen *screen,
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
lyrics_screen_try_take_consumer_message(LyricsScreen *screen,
                                        StrBuilder *message) {
    if ((screen == NULL) || (message == NULL)) {
        return false;
    }
    if (screen->consumer_message.len <= 0) {
        return false;
    }
    sb_copy(message, &screen->consumer_message);
    sb_clear(&screen->consumer_message);
    return true;
}

NcmSong *
lyrics_screen_song(LyricsScreen *screen) {
    if (!screen->has_song) {
        return NULL;
    }
    return &screen->song;
}

StrBuilder *
lyrics_screen_filename(LyricsScreen *screen) {
    return &screen->filename;
}

LyricsMode
lyrics_screen_mode(LyricsScreen *screen) {
    return screen->mode;
}

NcmLrcDocument *
lyrics_screen_lrc(LyricsScreen *screen) {
    return &screen->lrc;
}

int32
lyrics_screen_active_lrc_line(LyricsScreen *screen) {
    if (screen == NULL) {
        return LYRICS_NO_ACTIVE_LINE;
    }
    return screen->active_lrc_line;
}

bool
lyrics_buffer_find(NcBuffer *buffer,
                   char *pattern, int32 pattern_len, NcmError *ncm_error) {
    LyricsFindState state;
    NcmRegex regex;
    char *data;
    bool result;

    if (buffer == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing lyrics buffer"));
        return false;
    }

    nc_buffer_remove_properties(buffer, LYRICS_SEARCH_PROPERTY_ID);
    if ((pattern == NULL) || (pattern_len <= 0)) {
        ncm_error_clear(ncm_error);
        return true;
    }

    regex = (NcmRegex){0};
    if (!ncm_regex_compile(&regex, pattern, pattern_len, Config.regex_flags,
                           ncm_error)) {
        ncm_regex_destroy(&regex);
        return false;
    }

    state.buffer = buffer;
    data = nc_buffer_data(buffer);
    result = ncm_regex_for_each_match(&regex,
                                      data,
                                      buffer->len,
                                      lyrics_find_match_callback,
                                      &state);
    ncm_regex_destroy(&regex);
    return result;
}

bool
lyrics_screen_find(LyricsScreen *screen,
                   char *pattern, int32 pattern_len,
                   NcmError *ncm_error) {
    bool result;

    if (screen == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing lyrics screen"));
        return false;
    }

    result = lyrics_buffer_find(&screen->display, pattern,
                                pattern_len, ncm_error);
    if ((pattern == NULL) || (pattern_len <= 0)) {
        sb_clear(&screen->search_constraint);
    } else if (!ncm_error_is_set(ncm_error)) {
        (void)sb_set(&screen->search_constraint, pattern,
                     pattern_len);
    }
    nc_scrollpad_flush(&screen->scrollpad, &screen->window,
                       &screen->display);
    lyrics_display(screen);
    return result;
}

void
lyrics_buffer_clear_sync_highlight(NcBuffer *buffer) {
    if (buffer == NULL) {
        return;
    }

    nc_buffer_remove_properties(buffer, LYRICS_SYNC_PROPERTY_ID);
    return;
}

void
lyrics_buffer_highlight_sync_line(NcBuffer *buffer,
                                  int32 start, int32 end) {
    NcFormattedColor highlight;

    if (buffer == NULL) {
        return;
    }

    lyrics_buffer_clear_sync_highlight(buffer);
    if ((start < 0) || (end <= start) || (start >= buffer->len)) {
        return;
    }
    if (end > buffer->len) {
        end = buffer->len;
    }

    nc_formatted_color_init_color(
        &highlight, nc_color_make(COLOR_WHITE, COLOR_BLACK, false, false));
    nc_formatted_color_add_format(&highlight, NC_FORMAT_BOLD);
    nc_buffer_add_formatted_color(buffer, start, &highlight,
                                  LYRICS_SYNC_PROPERTY_ID);
    nc_buffer_add_formatted_color_end(buffer, end, &highlight,
                                      LYRICS_SYNC_PROPERTY_ID);
    nc_formatted_color_destroy(&highlight);
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
    LyricsScreen *lyrics;
    int32 x;
    int32 width;

    lyrics = lyrics_from_screen(screen);
    nc_screen_switcher_get_resize_params(screen, &x, &width, true);
    lyrics_screen_set_geometry(lyrics,
                               x,
                               width,
                               ui_state_main_start_y(),
                               ui_state_main_height());
    nc_screen_clear_resize_request(screen);
    return;
}

static bool
lyrics_screen_update_sync_line(LyricsScreen *screen) {
    return lyrics_screen_update_sync_line_force(screen, false);
}

static bool
lyrics_screen_update_sync_line_force(
    LyricsScreen *screen,
    bool force
) {
    NcmLrcEntry *entry;
    int32 active_line;

    if (screen == NULL) {
        return false;
    }
    if (screen->mode != LYRICS_MODE_SYNCHRONIZED) {
        if (screen->active_lrc_line == LYRICS_NO_ACTIVE_LINE) {
            return false;
        }
        lyrics_screen_clear_sync_line(screen);
        return true;
    }

    active_line = ncm_lrc_document_entry_at_time(
        &screen->lrc, ncm_status_state_elapsed_time_ms());
    if (!force && (active_line == screen->active_lrc_line)) {
        return false;
    }

    screen->active_lrc_line = active_line;
    if (active_line == LYRICS_NO_ACTIVE_LINE) {
        lyrics_buffer_clear_sync_highlight(&screen->display);
        nc_scrollpad_reset(&screen->scrollpad);
        return true;
    }
    if (active_line >= screen->lrc.entries_len) {
        lyrics_buffer_clear_sync_highlight(&screen->display);
        nc_scrollpad_reset(&screen->scrollpad);
        return true;
    }

    entry = &screen->lrc.entries[active_line];
    lyrics_buffer_highlight_sync_line(&screen->display,
                                      entry->buffer_start,
                                      entry->buffer_end);
    nc_scrollpad_center_on_buffer_position(&screen->scrollpad,
                                           &screen->window,
                                           &screen->display,
                                           entry->buffer_start);
    return true;
}

static void
lyrics_screen_clear_sync_line(LyricsScreen *screen) {
    ASSERT(screen != NULL);

    screen->active_lrc_line = LYRICS_NO_ACTIVE_LINE;
    lyrics_buffer_clear_sync_highlight(&screen->display);
    return;
}

static bool
lyrics_screen_start_foreground_fetch(
    LyricsScreen *screen,
    NcmSong *song,
    NcmLyricsFetcherDef *fetcher,
    StrBuilder *filename,
    NcmError *ncm_error
) {
    LyricsJob *job;
    NcmLyricsFetcherDef *active_fetcher;

    lyrics_screen_clear_lyrics_state(
        screen, LYRICS_MODE_FETCH_LOG);
    nc_scrollpad_reset(&screen->scrollpad);
    nc_lyrics_screen_reset_scroll_begin(&screen->screen);
    ncm_lyrics_result_clear(&screen->result);
    if (song != &screen->song) {
        ncm_song_copy(&screen->song, song);
    }
    screen->has_song = true;
    sb_copy(&screen->filename, filename);

    active_fetcher = lyrics_active_fetcher(screen, fetcher);
    if (active_fetcher) {
        lyrics_append_fetching(&screen->display, active_fetcher);
    } else if (Config.lyrics_fetchers.fetchers.len > 0) {
        lyrics_append_fetching(
            &screen->display, &Config.lyrics_fetchers.fetchers.items[0]);
    }
    nc_lyrics_screen_request_refresh(&screen->screen);

    if (!ncm_job_queue_start(&screen->jobs, ncm_error)) {
        return false;
    }

    job = lyrics_job_create(screen, song, active_fetcher, false, false);
    screen->foreground_job = job;
    if (!ncm_job_queue_push(&screen->jobs,
                            (NcmJob){
                                .run = lyrics_job_run,
                                .complete = lyrics_job_complete,
                                .destroy = lyrics_job_destroy,
                                .user = job,
                            },
                            ncm_error)) {
        screen->foreground_job = NULL;
        lyrics_job_destroy(job);
        return false;
    }

    ncm_error_clear(ncm_error);
    return true;
}

static int32
lyrics_screen_sync_timeout(LyricsScreen *screen) {
    int32 next_line;
    int64 elapsed_ms;
    int64 remaining_ms;

    if (screen == NULL) {
        return LYRICS_DEFAULT_TIMEOUT_MS;
    }
    if (screen->mode != LYRICS_MODE_SYNCHRONIZED) {
        return LYRICS_DEFAULT_TIMEOUT_MS;
    }
    if (ncm_status_state_player() != NCM_STATUS_PLAYER_PLAY) {
        return LYRICS_DEFAULT_TIMEOUT_MS;
    }

    elapsed_ms = ncm_status_state_elapsed_time_ms();
    next_line = ncm_lrc_document_next_entry_after_time(&screen->lrc,
                                                       elapsed_ms);
    if (next_line < 0) {
        return LYRICS_DEFAULT_TIMEOUT_MS;
    }
    if (next_line >= screen->lrc.entries_len) {
        return LYRICS_DEFAULT_TIMEOUT_MS;
    }

    remaining_ms = (int64)screen->lrc.entries[next_line].time_ms
                   - elapsed_ms;
    remaining_ms = CLAMP(remaining_ms, LYRICS_SYNC_TIMEOUT_MIN_MS,
                         LYRICS_SYNC_TIMEOUT_MAX_MS);
    return (int32)remaining_ms;
}

static int32
lyrics_window_timeout_callback(NcScreen *screen) {
    return lyrics_screen_sync_timeout(lyrics_from_screen(screen));
}

static char *
lyrics_title_callback(NcScreen *screen) {
    StrBuilder song_title = {0};
    StrBuilder scroll_buffer = {0};
    int32 scroll_begin;
    int32 scroll_width;
    char separator[] = " ** ";
    LyricsScreen *lyrics = lyrics_from_screen(screen);

    sb_clear(&lyrics->title);
    SB_APPEND(&lyrics->title, STRLIT(LYRICS_TITLE));
    if (!lyrics->has_song || ncm_song_empty(&lyrics->song)) {
        return lyrics->title.data;
    }

    lyrics_title_song_string(&lyrics->song, &song_title);
    if (song_title.len <= 0) {
        sb_free(&song_title);
        return lyrics->title.data;
    }

    SB_APPEND(&lyrics->title, STRLIT(": "));
    scroll_begin = nc_lyrics_screen_scroll_begin(&lyrics->screen);
    scroll_width = COLS - utf8_width(lyrics->title.data,
                                     lyrics->title.len);
    if (Config.design == NCM_DESIGN_ALTERNATIVE) {
        scroll_width -= 2;
    } else {
        scroll_width -= global_volume_state_len();
    }

    nc_cyclic_text_write(&scroll_buffer, song_title.data, song_title.len,
                         &scroll_begin, scroll_width, separator,
                         SIZEOF(separator) - 1,
                         Config.header_text_scrolling);
    SB_APPEND(&lyrics->title, scroll_buffer.data, scroll_buffer.len);
    nc_lyrics_screen_set_scroll_begin(&lyrics->screen, scroll_begin);
    sb_free(&scroll_buffer);
    sb_free(&song_title);
    return lyrics->title.data;
}

static void
lyrics_update_callback(NcScreen *screen) {
    lyrics_screen_update(lyrics_from_screen(screen));
    return;
}

static void
lyrics_mouse_button_pressed_callback(NcScreen *screen, MEVENT event) {
    LyricsScreen *lyrics = lyrics_from_screen(screen);

    if ((event.bstate & BUTTON5_PRESSED) != 0) {
        lyrics_mouse_scroll(lyrics, NC_SCROLL_DOWN);
    } else if ((event.bstate & BUTTON4_PRESSED) != 0) {
        lyrics_mouse_scroll(lyrics, NC_SCROLL_UP);
    }

    return;
}

static void
lyrics_title_song_string(NcmSong *song, StrBuilder *title) {
    NcmStringView artist_view;
    NcmStringView title_view;
    NcmStringView name_view;

    artist_view = (NcmStringView){0};
    title_view = (NcmStringView){0};
    name_view = (NcmStringView){0};

    sb_clear(title);

    if (ncm_song_tag_view(song, MPD_TAG_ARTIST, 0, &artist_view)
        && ncm_song_tag_view(song, MPD_TAG_TITLE, 0, &title_view)) {
        SB_APPEND(title, artist_view.data, artist_view.len);
        SB_APPEND(title, STRLIT(" - "));
        SB_APPEND(title, title_view.data, title_view.len);
        return;
    }

    if (ncm_song_name_view(song, 0, &name_view)) {
        SB_APPEND(title, name_view.data, name_view.len);
    }
    return;
}

static bool
lyrics_song_artist_title(NcmSong *song,
                         StrBuilder *artist, StrBuilder *title) {
    StrBuilder fallback = {0};
    NcmStringView artist_view;
    NcmStringView title_view;
    NcmStringView name_view;

    artist_view = (NcmStringView){0};
    title_view = (NcmStringView){0};
    name_view = (NcmStringView){0};

    sb_clear(artist);
    sb_clear(title);

    if (ncm_song_tag_view(song, MPD_TAG_ARTIST, 0, &artist_view)
        && ncm_song_tag_view(song, MPD_TAG_TITLE, 0, &title_view)) {
        SB_APPEND(artist, artist_view.data, artist_view.len);
        SB_APPEND(title, title_view.data, title_view.len);
        return true;
    }

    if (ncm_song_name_view(song, 0, &name_view)) {
        SB_APPEND(&fallback, name_view.data, name_view.len);
    } else if (ncm_song_uri_view(song, 0, &name_view)) {
        SB_APPEND(&fallback, name_view.data, name_view.len);
    }

    lyrics_remove_extension(&fallback);
    sb_copy(title, &fallback);
    sb_free(&fallback);

    return title->len > 0;
}

static bool
lyrics_fetch_artist_title(NcmSong *song,
                          StrBuilder *artist, StrBuilder *title) {
    if (!lyrics_song_artist_title(song, artist, title)) {
        return false;
    }
    if (artist->len <= 0) {
        lyrics_replace_search_separators(title);
    }
    return true;
}

static void
lyrics_replace_search_separators(StrBuilder *buffer) {
    for (int32 i = 0; i < buffer->len; i += 1) {
        if ((buffer->data[i] == '-') || (buffer->data[i] == '_')) {
            buffer->data[i] = ' ';
        }
    }
    return;
}

static void
lyrics_append_locale(NcBuffer *buffer, char *data, int32 data_len) {
    StrBuilder converted = ncm_charset_utf8_to_locale(data, data_len);
    nc_buffer_append_data(buffer, converted.data, converted.len);
    sb_free(&converted);
    return;
}

static bool
lyrics_screen_render_lrc(LyricsScreen *screen,
                         NcmError *ncm_error) {
    NcmLrcRenderTarget target = {0};

    target.user = screen;
    target.position = lyrics_lrc_buffer_position;
    target.append = lyrics_lrc_buffer_append;

    if (!ncm_lrc_document_render_plain(&screen->lrc, &target)) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("failed to render LRC"));
        return false;
    }

    ncm_error_clear(ncm_error);
    return true;
}

static int32
lyrics_lrc_buffer_position(void *user) {
    LyricsScreen *screen = user;

    if (screen == NULL) {
        return 0;
    }

    return nc_buffer_len(&screen->display);
}

static void
lyrics_lrc_buffer_append(void *user,
                         char *data, int32 data_len) {
    LyricsScreen *screen = user;

    if (screen == NULL) {
        return;
    }

    lyrics_append_locale(&screen->display, data, data_len);
    return;
}

static void
lyrics_report_sidecar_status(StrBuilder *lrc_filename,
                             bool lrc_found,
                             StrBuilder *txt_filename,
                             bool txt_found) {
    NcmStringFormatArg args[4];
    char *lrc_status;
    char *txt_status;
    int32 lrc_start;
    int32 txt_start;

    if ((lrc_filename == NULL) || (txt_filename == NULL)) {
        return;
    }
    if ((lrc_filename->len <= 0) || (txt_filename->len <= 0)) {
        return;
    }

    if (lrc_found) {
        lrc_status = "found";
    } else {
        lrc_status = "not found";
    }
    if (txt_found) {
        txt_status = "found";
    } else {
        txt_status = "not found";
    }

    lrc_start = ncm_string_basename_start(lrc_filename->data,
                                          lrc_filename->len);
    txt_start = ncm_string_basename_start(txt_filename->data,
                                          txt_filename->len);
    args[0] = ncm_string_format_arg_string(
        lrc_filename->data + lrc_start, lrc_filename->len - lrc_start);
    args[1] = ncm_string_format_arg_cstring(lrc_status);
    args[2] = ncm_string_format_arg_string(
        txt_filename->data + txt_start, txt_filename->len - txt_start);
    args[3] = ncm_string_format_arg_cstring(txt_status);

    ncm_statusbar_format(Config.message_delay_time,
                         STRLIT("%1% %2%; %3% %4%"),
                         args, LENGTH(args));
    return;
}

static void
lyrics_report_save_error(StrBuilder *filename, NcmError *ncm_error) {
    NcmStringFormatArg args[2];
    char *message = "unknown error";

    if (ncm_error && (ncm_error->code != 0)) {
        message = strerror(ncm_error->code);
    }

    args[0] = ncm_string_format_arg_string(filename->data, filename->len);
    args[1] = ncm_string_format_arg_cstring(message);
    ncm_statusbar_format(Config.message_delay_time,
                         STRLIT("Couldn't save lyrics as \"%1%\": %2%"),
                         args, LENGTH(args));

    return;
}

static void
lyrics_report_unlink_error(StrBuilder *filename, NcmError *ncm_error) {
    NcmStringFormatArg args[2];
    char *message = "unknown error";

    if (ncm_error && (ncm_error->code != 0)) {
        message = strerror(ncm_error->code);
    }
    args[0] = ncm_string_format_arg_string(filename->data, filename->len);
    args[1] = ncm_string_format_arg_cstring(message);
    ncm_statusbar_format(Config.message_delay_time,
                         STRLIT("Couldn't remove \"%1%\": %2%"),
                         args, LENGTH(args));
    return;
}

static void
lyrics_screen_clear_lyrics_state(LyricsScreen *screen,
                                 LyricsMode mode) {
    nc_buffer_clear(&screen->display);
    ncm_lrc_document_clear(&screen->lrc);
    screen->active_lrc_line = LYRICS_NO_ACTIVE_LINE;
    screen->mode = mode;
    return;
}

static void
lyrics_remove_extension(StrBuilder *buffer) {
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
lyrics_filename_from_song_with_extension(
    StrBuilder *filename,
    NcmSong *song,
    char *music_dir, int32 music_dir_len,
    char *lyrics_dir, int32 lyrics_dir_len,
    bool store_in_song_dir,
    bool win32_filename,
    char *extension, int32 extension_len
) {
    StrBuilder artist = {0};
    StrBuilder title = {0};
    NcmStringView uri;
    int32 basename_start;
    int32 basename_len;

    uri = (NcmStringView){0};
    sb_clear(filename);

    if (store_in_song_dir && !ncm_song_is_stream(song)) {
        if (ncm_song_is_from_database(song) && (music_dir_len > 0)) {
            SB_APPEND(filename, music_dir, music_dir_len);
            sb_append_byte_if_not(filename, '/');
        }
        if (ncm_song_uri_view(song, 0, &uri)) {
            SB_APPEND(filename, uri.data, uri.len);
        }
        lyrics_remove_extension(filename);
    } else {
        (void)lyrics_song_artist_title(song, &artist, &title);
        if (lyrics_dir_len > 0) {
            SB_APPEND(filename, lyrics_dir, lyrics_dir_len);
            sb_append_byte_if_not(filename, '/');
        }
        basename_start = filename->len;
        if ((artist.len > 0) && (title.len > 0)) {
            SB_APPEND(filename, artist.data, artist.len);
            SB_APPEND(filename, STRLIT(" - "));
            SB_APPEND(filename, title.data, title.len);
        } else {
            SB_APPEND(filename, title.data, title.len);
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

    SB_APPEND(filename, extension, extension_len);
    sb_free(&title);
    sb_free(&artist);

    return filename->len > extension_len;
}

static bool
lyrics_filename_from_song(StrBuilder *filename, NcmSong *song,
                          char *music_dir, int32 music_dir_len,
                          char *lyrics_dir, int32 lyrics_dir_len,
                          bool store_in_song_dir, bool win32_filename) {
    return lyrics_filename_from_song_with_extension(filename,
                                                    song,
                                                    music_dir,
                                                    music_dir_len,
                                                    lyrics_dir,
                                                    lyrics_dir_len,
                                                    store_in_song_dir,
                                                    win32_filename,
                                                    STRLIT(".txt"));
}

static bool
lyrics_preferred_filename_from_song(StrBuilder *filename,
                                    NcmSong *song,
                                    char *music_dir,
                                    int32 music_dir_len,
                                    char *lyrics_dir,
                                    int32 lyrics_dir_len,
                                    bool store_in_song_dir,
                                    bool win32_filename) {
    StrBuilder lrc_filename = {0};
    bool success;

    success = lyrics_filename_from_song_with_extension(&lrc_filename,
                                                       song,
                                                       music_dir,
                                                       music_dir_len,
                                                       lyrics_dir,
                                                       lyrics_dir_len,
                                                       store_in_song_dir,
                                                       win32_filename,
                                                       STRLIT(".lrc"));
    if (!success) {
        sb_free(&lrc_filename);
        return false;
    }
    if (ncm_fs_exists(lrc_filename.data, lrc_filename.len)) {
        sb_copy(filename, &lrc_filename);
        sb_free(&lrc_filename);
        return true;
    }
    sb_free(&lrc_filename);

    return lyrics_filename_from_song(filename,
                                     song,
                                     music_dir,
                                     music_dir_len,
                                     lyrics_dir,
                                     lyrics_dir_len,
                                     store_in_song_dir,
                                     win32_filename);
}

static bool
lyrics_queue_song(LyricsScreen *screen,
                  NcmSong *song, bool notify) {
    LyricsQueuedSong *queued;
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
            screen->queued_songs[i] = (LyricsQueuedSong){0};
        }
        screen->queued_songs_cap = new_cap;
    }

    queued = &screen->queued_songs[screen->queued_songs_len];
    ncm_song_copy(&queued->song, song);
    queued->notify = notify;
    screen->queued_songs_len += 1;

    return true;
}

static LyricsQueuedSong *
lyrics_dequeue_song(LyricsScreen *screen) {
    LyricsQueuedSong *queued;

    if (screen->queued_songs_len <= 0) {
        return NULL;
    }
    queued = malloc2(SIZEOF(*queued));
    *queued = (LyricsQueuedSong){0};
    lyrics_queued_song_move(queued, &screen->queued_songs[0]);
    for (int32 i = 1; i < screen->queued_songs_len; i += 1) {
        lyrics_queued_song_move(&screen->queued_songs[i - 1],
                                &screen->queued_songs[i]);
    }
    screen->queued_songs_len -= 1;
    return queued;
}

static LyricsJob *
lyrics_job_create(LyricsScreen *screen,
                  NcmSong *song,
                  NcmLyricsFetcherDef *fetcher,
                  bool notify,
                  bool background) {
    bool win32_filename;
    LyricsJob *job = malloc2(SIZEOF(*job));

    job->screen = screen;
    job->song = (NcmSong){0};
    ncm_song_copy(&job->song, song);
    job->filename = (StrBuilder){0};
    job->log = (NcBuffer){0};
    pthread_mutex_init(&job->log_mutex, NULL);
    job->log_dirty = false;

    win32_filename = Config.generate_win32_compatible_filenames;

    (void)lyrics_filename_from_song(&job->filename,
                                    song,
                                    Config.mpd_music_dir,
                                    Config.mpd_music_dir_len,
                                    Config.lyrics_directory,
                                    Config.lyrics_directory_len,
                                    Config.store_lyrics_in_song_dir,
                                    win32_filename);

    job->fetcher = fetcher;
    job->result = (NcmLyricsResult){0};
    job->notify = notify;
    job->background = background;

    return job;
}

static bool
lyrics_job_fetch_one(LyricsJob *job, NcmLyricsFetcherDef *fetcher,
                     StrBuilder *artist, StrBuilder *title) {
    ASSERT(job != NULL);
    ASSERT(fetcher != NULL);
    ASSERT(artist != NULL);
    ASSERT(title != NULL);

    lyrics_job_append_fetching(job, fetcher);
    if (!ncm_lyrics_fetcher_fetch(fetcher,
                                  &job->result,
                                  artist->data,
                                  artist->len,
                                  title->data,
                                  title->len)) {
        lyrics_job_append_fetch_error(job, &job->result);
        return false;
    }
    if (!job->result.success) {
        lyrics_job_append_fetch_error(job, &job->result);
        return false;
    }
    return true;
}

static bool
lyrics_job_fetch(LyricsJob *job,
                 StrBuilder *artist, StrBuilder *title) {
    if (job->fetcher) {
        return lyrics_job_fetch_one(job,
                                    job->fetcher,
                                    artist,
                                    title);
    }

    for (int32 i = 0; i < Config.lyrics_fetchers.fetchers.len; i += 1) {
        if (lyrics_job_fetch_one(
            job, &Config.lyrics_fetchers.fetchers.items[i],
            artist, title)) {
            return true;
        }
    }
    return false;
}

static bool
lyrics_job_run(void *user, NcmError *ncm_error) {
    StrBuilder artist = {0};
    StrBuilder title = {0};
    bool success;
    LyricsJob *job = user;


    if (!lyrics_fetch_artist_title(&job->song, &artist, &title)) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing song metadata"));
        sb_free(&title);
        sb_free(&artist);
        return false;
    }

    success = lyrics_job_fetch(job, &artist, &title);
    sb_free(&title);
    sb_free(&artist);
    if (!success || !job->result.success) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("lyrics not found"));
    }

    return success && job->result.success;
}

static void
lyrics_job_complete(bool success, NcmError *ncm_error, void *user) {
    LyricsJob *job = user;
    LyricsScreen *screen = job->screen;

    (void)success;
    (void)ncm_error;

    if (!job->background) {
        if (screen->foreground_job == job) {
            screen->foreground_job = NULL;
        }
        if (!lyrics_job_is_current(job)) {
            return;
        }

        ncm_lyrics_result_clear(&screen->result);
        (void)ncm_lyrics_result_set(&screen->result,
                                    job->result.success,
                                    job->result.text,
                                    job->result.text_len);
        if (job->result.success) {
            NcmError save_error = {0};

            lyrics_screen_clear_lyrics_state(
                screen, LYRICS_MODE_PLAIN);
            lyrics_append_locale(&screen->display,
                                 job->result.text,
                                 job->result.text_len);
            sb_copy(&screen->filename, &job->filename);
            if (!lyrics_screen_save_file(screen,
                                         job->filename.data,
                                         job->filename.len,
                                         job->result.text,
                                         job->result.text_len,
                                         &save_error)) {
                lyrics_report_save_error(&job->filename, &save_error);
            }
            ncm_error_clear(&save_error);
        } else {
            lyrics_screen_clear_lyrics_state(
                screen, LYRICS_MODE_FETCH_LOG);
            nc_buffer_destroy(&screen->display);
            nc_buffer_copy(&screen->display, &job->log);
            nc_buffer_append_cstring(&screen->display,
                                     "\nLyrics were not found.\n");
        }
        nc_lyrics_screen_request_refresh(&screen->screen);
    } else {
        if (job->result.success) {
            NcmError save_error = {0};

            (void)lyrics_screen_save_file(screen,
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
lyrics_job_destroy(void *user) {
    LyricsJob *job;

    job = user;
    if (job == NULL) {
        return;
    }

    ncm_song_destroy(&job->song);
    sb_free(&job->filename);
    ncm_lyrics_result_destroy(&job->result);
    nc_buffer_destroy(&job->log);
    pthread_mutex_destroy(&job->log_mutex);
    free2(job, SIZEOF(*job));

    return;
}

static NcmLyricsFetcherDef *
lyrics_active_fetcher(LyricsScreen *screen,
                      NcmLyricsFetcherDef *fetcher) {
    if (fetcher) {
        return fetcher;
    }
    return screen->fetcher;
}

static void
lyrics_append_fetching(NcBuffer *buffer, NcmLyricsFetcherDef *fetcher) {
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
    nc_buffer_add_format(buffer, fetcher_position, NC_FORMAT_BOLD,
                         LYRICS_FETCH_PROPERTY_ID);
    nc_buffer_append_data(buffer, name, name_len);
    nc_buffer_add_format(buffer, nc_buffer_len(buffer), NC_FORMAT_NO_BOLD,
                         LYRICS_FETCH_PROPERTY_ID);
    nc_buffer_append_cstring(buffer, "... ");

    return;
}

static void
lyrics_append_fetch_error(NcBuffer *buffer, NcmLyricsResult *result) {
    NcColor red = nc_color_make(COLOR_RED, NC_COLOR_CURRENT, false, false);
    nc_buffer_add_color(buffer, nc_buffer_len(buffer), red,
                        LYRICS_FETCH_PROPERTY_ID);
    nc_buffer_append_data(buffer, result->text, result->text_len);
    nc_buffer_add_color(buffer, nc_buffer_len(buffer), nc_color_end(),
                        LYRICS_FETCH_PROPERTY_ID);
    nc_buffer_append_char(buffer, '\n');

    return;
}

static void
lyrics_job_append_fetching(LyricsJob *job,
                           NcmLyricsFetcherDef *fetcher) {
    ASSERT(job != NULL);
    ASSERT(fetcher != NULL);

    pthread_mutex_lock(&job->log_mutex);
    lyrics_append_fetching(&job->log, fetcher);
    job->log_dirty = true;
    pthread_mutex_unlock(&job->log_mutex);

    return;
}

static void
lyrics_job_append_fetch_error(LyricsJob *job,
                              NcmLyricsResult *result) {
    ASSERT(job != NULL);
    ASSERT(result != NULL);

    pthread_mutex_lock(&job->log_mutex);
    lyrics_append_fetch_error(&job->log, result);
    job->log_dirty = true;
    pthread_mutex_unlock(&job->log_mutex);

    return;
}

static bool
lyrics_job_take_log(LyricsJob *job, NcBuffer *buffer) {
    NcBuffer copy;
    bool result = false;

    if ((job == NULL) || (buffer == NULL)) {
        return false;
    }

    copy = (NcBuffer){0};
    pthread_mutex_lock(&job->log_mutex);
    if (job->log_dirty) {
        nc_buffer_copy(&copy, &job->log);
        job->log_dirty = false;
        result = true;
    }
    pthread_mutex_unlock(&job->log_mutex);

    if (result) {
        nc_buffer_destroy(buffer);
        nc_buffer_move(buffer, &copy);
    } else {
        nc_buffer_destroy(&copy);
    }

    return result;
}

static void
lyrics_screen_update_progress(LyricsScreen *screen) {
    LyricsJob *job;

    if (screen == NULL) {
        return;
    }

    job = screen->foreground_job;
    if (job == NULL) {
        return;
    }
    if (!lyrics_job_is_current(job)) {
        return;
    }
    if (lyrics_job_take_log(job, &screen->display)) {
        ncm_lrc_document_clear(&screen->lrc);
        screen->active_lrc_line = LYRICS_NO_ACTIVE_LINE;
        screen->mode = LYRICS_MODE_FETCH_LOG;
        nc_lyrics_screen_request_refresh(&screen->screen);
    }

    return;
}

static bool
lyrics_job_is_current(LyricsJob *job) {
    LyricsScreen *screen;

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
    return STREQUAL(screen->filename.data, screen->filename.len,
                    job->filename.data, job->filename.len);
}

static void
lyrics_set_consumer_fetch_message(LyricsScreen *screen,
                                  NcmSong *song) {
    StrBuilder formatted = ncm_format_render_string(&Config.song_status_format,
                                                    song);
    sb_clear(&screen->consumer_message);

    SB_APPEND(&screen->consumer_message,
              STRLIT("Fetching lyrics for \""));
    SB_APPEND(&screen->consumer_message,
              formatted.data,
              formatted.len);
    SB_APPEND(&screen->consumer_message, STRLIT("\"..."));

    sb_free(&formatted);

    return;
}

static bool
lyrics_find_match_callback(int32 start, int32 len, void *user) {
    LyricsFindState *state = user;

    if (len <= 0) {
        return true;
    }

    nc_buffer_add_format(state->buffer, start,
                         NC_FORMAT_REVERSE, LYRICS_SEARCH_PROPERTY_ID);
    nc_buffer_add_format(state->buffer, start + len,
                         NC_FORMAT_NO_REVERSE,
                         LYRICS_SEARCH_PROPERTY_ID);

    return true;
}

static void
lyrics_mouse_scroll(LyricsScreen *screen, enum NcScroll where) {
    for (int32 i = 0; i < Config.lines_scrolled; i += 1) {
        nc_scrollpad_scroll(&screen->scrollpad, &screen->window, where);
    }
    return;
}

static void
lyrics_display(LyricsScreen *screen) {
    nc_window_refresh_border(&screen->window);
    nc_scrollpad_refresh(&screen->scrollpad, &screen->window);
    return;
}

static bool
lyrics_start_next_background(LyricsScreen *screen,
                             NcmError *ncm_error) {
    LyricsQueuedSong *queued;
    LyricsJob *job;
    StrBuilder filename = {0};
    bool win32_filename;
    bool found_job;

    if (ncm_job_queue_pending_count(&screen->jobs) > 0) {
        ncm_error_clear(ncm_error);
        return true;
    }
    if (ncm_job_queue_completed_count(&screen->jobs) > 0) {
        ncm_error_clear(ncm_error);
        return true;
    }

    found_job = false;
    queued = NULL;
    win32_filename = Config.generate_win32_compatible_filenames;
    while (!found_job) {
        if ((queued = lyrics_dequeue_song(screen)) == NULL) {
            sb_free(&filename);
            ncm_error_clear(ncm_error);
            return true;
        }

        if (ncm_song_is_stream(&queued->song)) {
            lyrics_queued_song_destroy(queued);
            free2(queued, SIZEOF(*queued));
            queued = NULL;
            continue;
        }

        if (!lyrics_preferred_filename_from_song(
            &filename,
            &queued->song,
            Config.mpd_music_dir,
            Config.mpd_music_dir_len,
            Config.lyrics_directory,
            Config.lyrics_directory_len,
            Config.store_lyrics_in_song_dir,
            win32_filename)) {
            lyrics_queued_song_destroy(queued);
            free2(queued, SIZEOF(*queued));
            queued = NULL;
            continue;
        }

        if (ncm_fs_exists(filename.data, filename.len)) {
            lyrics_queued_song_destroy(queued);
            free2(queued, SIZEOF(*queued));
            queued = NULL;
            continue;
        }

        found_job = true;
    }

    if (!ncm_job_queue_start(&screen->jobs, ncm_error)) {
        sb_free(&filename);
        lyrics_queued_song_destroy(queued);
        free2(queued, SIZEOF(*queued));
        return false;
    }

    if (queued->notify) {
        lyrics_set_consumer_fetch_message(screen, &queued->song);
    }
    job = lyrics_job_create(screen,
                            &queued->song,
                            screen->fetcher,
                            queued->notify,
                            true);
    if (!ncm_job_queue_push(&screen->jobs,
                            (NcmJob){
                                .run = lyrics_job_run,
                                .complete = lyrics_job_complete,
                                .destroy = lyrics_job_destroy,
                                .user = job,
                            },
                            ncm_error)) {
        lyrics_job_destroy(job);
        sb_free(&filename);
        lyrics_queued_song_destroy(queued);
        free2(queued, SIZEOF(*queued));
        return false;
    }

    sb_free(&filename);
    lyrics_queued_song_destroy(queued);
    free2(queued, SIZEOF(*queued));
    ncm_error_clear(ncm_error);
    return true;
}

#endif /* NCMPCPP_NC_LYRICS_C */
