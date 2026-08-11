#if !defined(NCMPCPP_NC_LYRICS_C)
#define NCMPCPP_NC_LYRICS_C

#include "cbase.h"

#include "c/ncm_base.h"
#include "c/ncm_charset.h"
#include "c/ncm_format.h"
#include "c/ncm_regex.h"
#include "c/ncm_string.h"
#include "curses/nc_cyclic_buffer.h"
#include "global.h"
#include "screens/nc_lyrics.h"
#include "screens/screen_switcher.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "title.h"
#include "ui_state.h"

#define NATIVE_LYRICS_TITLE "Lyrics"
#define NATIVE_LYRICS_FETCH_PROPERTY_ID ((int64)0x4c59524645544348LL)
#define NATIVE_LYRICS_SEARCH_PROPERTY_ID ((int64)0x4c59525345415243LL)
#define NATIVE_LYRICS_SYNC_PROPERTY_ID ((int64)0x4c595253594e4321LL)
#define NATIVE_LYRICS_NO_ACTIVE_LINE (-1)
#define NATIVE_LYRICS_DEFAULT_TIMEOUT_MS NC_SCREEN_DEFAULT_WINDOW_TIMEOUT
#define NATIVE_LYRICS_SYNC_TIMEOUT_MIN_MS 25
#define NATIVE_LYRICS_SYNC_TIMEOUT_MAX_MS 100

struct NativeLyricsJob {
    NativeLyricsScreen *screen;
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

typedef struct NativeLyricsFindState {
    NcBuffer *buffer;
} NativeLyricsFindState;

static void lyrics_switch_to_callback(NcScreen *screen);
static void lyrics_resize_callback(NcScreen *screen);
static int32 lyrics_window_timeout_callback(NcScreen *screen);
static char *lyrics_title_callback(NcScreen *screen);
static void lyrics_update_callback(NcScreen *screen);
static void lyrics_mouse_button_pressed_callback(NcScreen *screen,
                                                 MEVENT event);
static void native_lyrics_title_song_string(NcmSong *song, StrBuilder *title);
static void native_lyrics_replace_search_separators(StrBuilder *buffer);
static void native_lyrics_append_locale(NcBuffer *buffer, char *data,
                                        int32 data_len);
static bool native_lyrics_screen_render_lrc(NativeLyricsScreen *screen,
                                            NcmError *error);
static bool native_lyrics_screen_update_sync_line(NativeLyricsScreen *screen);
static bool native_lyrics_screen_start_foreground_fetch(
    NativeLyricsScreen *screen,
    NcmSong *song,
    NcmLyricsFetcherDef *fetcher,
    StrBuilder *filename,
    NcmError *error);
static bool native_lyrics_screen_update_sync_line_force(
    NativeLyricsScreen *screen,
    bool force);
static void native_lyrics_screen_clear_sync_line(NativeLyricsScreen *screen);
static int32 native_lyrics_screen_sync_timeout(NativeLyricsScreen *screen);
static int32 native_lyrics_lrc_buffer_position(void *user);
static void native_lyrics_lrc_buffer_append(void *user,
                                            char *data, int32 data_len);
static void native_lyrics_report_sidecar_status(StrBuilder *lrc_filename,
                                                bool lrc_found,
                                                StrBuilder *txt_filename,
                                                bool txt_found);
static void native_lyrics_report_unlink_error(StrBuilder *filename,
                                              NcmError *error);
static void native_lyrics_screen_clear_lyrics_state(
    NativeLyricsScreen *screen,
    NativeLyricsMode mode);
static void native_lyrics_remove_extension(StrBuilder *buffer);
static bool native_lyrics_filename_from_song_with_extension(
    StrBuilder *filename,
    NcmSong *song,
    char *music_dir, int32 music_dir_len,
    char *lyrics_dir, int32 lyrics_dir_len,
    bool store_in_song_dir,
    bool win32_filename,
    char *extension, int32 extension_len);
static bool native_lyrics_filename_from_song(StrBuilder *filename,
                                             NcmSong *song,
                                             char *music_dir,
                                             int32 music_dir_len,
                                             char *lyrics_dir,
                                             int32 lyrics_dir_len,
                                             bool store_in_song_dir,
                                             bool win32_filename);
static bool native_lyrics_preferred_filename_from_song(StrBuilder *filename,
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
static void native_lyrics_job_append_fetching(NativeLyricsJob *job,
                                              NcmLyricsFetcherDef *fetcher);
static void native_lyrics_job_append_fetch_error(NativeLyricsJob *job,
                                                 NcmLyricsResult *result);
static bool native_lyrics_job_take_log(NativeLyricsJob *job,
                                       NcBuffer *buffer);
static void native_lyrics_screen_update_progress(NativeLyricsScreen *screen);
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

#define NC_SCREEN_IMPL_TYPE NativeLyricsScreen
#define NC_SCREEN_IMPL_PREFIX lyrics
#define NC_SCREEN_IMPL_PUBLIC_PREFIX native_lyrics_screen
#define NC_SCREEN_IMPL_BASE_FIELD screen
#define NC_SCREEN_IMPL_SCROLLPAD_BASE screen.scrollpad_screen
#define NC_SCREEN_IMPL_NO_GEOMETRY_ACCESSORS
#define NC_SCREEN_IMPL_WINDOW_FIELD window
#define NC_SCREEN_IMPL_SCROLLPAD_FIELD scrollpad
#define NC_SCREEN_IMPL_REFRESH_CALLBACK native_lyrics_display
#define NC_SCREEN_IMPL_SWITCH_TO_CALLBACK lyrics_switch_to_callback
#define NC_SCREEN_IMPL_RESIZE_CALLBACK lyrics_resize_callback
#define NC_SCREEN_IMPL_TITLE_CALLBACK lyrics_title_callback
#define NC_SCREEN_IMPL_WINDOW_TIMEOUT_CALLBACK lyrics_window_timeout_callback
#define NC_SCREEN_IMPL_UPDATE_CALLBACK lyrics_update_callback
#define NC_SCREEN_IMPL_MOUSE_CALLBACK lyrics_mouse_button_pressed_callback
#define NC_SCREEN_IMPL_DESTROY_TYPED_CALLBACK native_lyrics_screen_destroy
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

void
native_lyrics_screen_init(NativeLyricsScreen *screen,
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
    nc_buffer_init(&screen->display);
    sb_init(&screen->search_constraint);
    sb_init(&screen->title);
    ncm_song_init(&screen->song);
    sb_init(&screen->filename);
    ncm_lrc_document_init(&screen->lrc);
    ncm_lyrics_result_init(&screen->result);
    ncm_job_queue_init(&screen->jobs);
    screen->foreground_job = NULL;
    screen->queued_songs = NULL;
    screen->queued_songs_len = 0;
    screen->queued_songs_cap = 0;
    screen->active_lrc_line = NATIVE_LYRICS_NO_ACTIVE_LINE;

    sb_init(&screen->consumer_message);

    screen->fetcher = NULL;
    screen->mode = NATIVE_LYRICS_MODE_PLAIN;
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
    screen->mode = NATIVE_LYRICS_MODE_PLAIN;
    screen->has_song = false;
    screen->initialized = false;

    return;
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
    if (screen->mode == NATIVE_LYRICS_MODE_SYNCHRONIZED) {
        (void)native_lyrics_screen_update_sync_line_force(screen, true);
    }
    nc_scrollpad_flush(&screen->scrollpad,
                       &screen->window,
                       &screen->display);
    nc_lyrics_screen_request_refresh(&screen->screen);
    return;
}

bool
native_lyrics_screen_build_filename(NativeLyricsScreen *screen,
                                    NcmSong *song,
                                    char *music_dir, int32 music_dir_len,
                                    char *lyrics_dir, int32 lyrics_dir_len,
                                    bool store_in_song_dir,
                                    bool win32_filename) {
    return native_lyrics_preferred_filename_from_song(&screen->filename,
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
    StrBuilder raw = {0};
    char line[1024];
    int32 line_len;
    bool first;
    bool lrc_file;

    if ((filename == NULL) || (filename_len <= 0)) {
        ncm_error_set(error, EINVAL, STRLIT("missing lyrics file"));
        return false;
    }

    lrc_file = (filename_len > STRLIT_LEN(".lrc"))
               && STREQUAL(filename + filename_len - STRLIT_LEN(".lrc"),
                           STRLIT_LEN(".lrc"),
                           STRLIT(".lrc"));
    if ((file = fopen(filename, "rb")) == NULL) {
        native_lyrics_screen_clear_lyrics_state(
            screen, NATIVE_LYRICS_MODE_FETCH_LOG);
        ncm_error_set(error, errno, STRLIT("failed to open lyrics"));
        return false;
    }

    nc_buffer_clear(&screen->display);
    nc_scrollpad_reset(&screen->scrollpad);
    screen->active_lrc_line = NATIVE_LYRICS_NO_ACTIVE_LINE;
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
        native_lyrics_append_locale(&screen->display, line, line_len);
        first = false;
    }

    XFCLOSE(file, filename);
    if (lrc_file
        && !ncm_lrc_parse(&screen->lrc, raw.data, raw.len, error)) {
        sb_free(&raw);
        native_lyrics_screen_clear_lyrics_state(
            screen, NATIVE_LYRICS_MODE_FETCH_LOG);
        return false;
    }

    if (lrc_file) {
        nc_buffer_clear(&screen->display);
        if (!native_lyrics_screen_render_lrc(screen, error)) {
            sb_free(&raw);
            native_lyrics_screen_clear_lyrics_state(
                screen, NATIVE_LYRICS_MODE_FETCH_LOG);
            return false;
        }
        screen->mode = NATIVE_LYRICS_MODE_SYNCHRONIZED;
    } else {
        ncm_lrc_document_clear(&screen->lrc);
        screen->mode = NATIVE_LYRICS_MODE_PLAIN;
    }
    sb_free(&raw);
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
    int32 close_result;

    (void)screen;
    if ((filename == NULL) || (filename_len <= 0)) {
        ncm_error_set(error, EINVAL, STRLIT("missing lyrics file"));
        return false;
    }
    if ((lyrics_len < 0) || ((lyrics == NULL) && (lyrics_len > 0))) {
        ncm_error_set(error, EINVAL, STRLIT("missing lyrics buffer"));
        return false;
    }

    if ((file = fopen(filename, "wb")) == NULL) {
        ncm_error_set(error, errno, STRLIT("failed to write lyrics"));
        return false;
    }

    written = 0;
    if (lyrics && (lyrics_len > 0)) {
        written = (int32)fwrite64(lyrics, 1, lyrics_len, file);
    }
    close_result = fclose(file);
    if ((written != lyrics_len) || (close_result != 0)) {
        ncm_error_set(error, errno, STRLIT("failed to save lyrics"));
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
        ncm_error_set(error, EINVAL, STRLIT("missing song"));
        return false;
    }

    win32_filename = Config.generate_win32_compatible_filenames;
    if (!native_lyrics_filename_from_song_with_extension(
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
        ncm_error_set(error, EINVAL,
                      STRLIT("failed to build lyrics filename"));
        return false;
    }
    if (!native_lyrics_filename_from_song(&txt_filename,
                                          song,
                                          Config.mpd_music_dir,
                                          Config.mpd_music_dir_len,
                                          Config.lyrics_directory,
                                          Config.lyrics_directory_len,
                                          Config.store_lyrics_in_song_dir,
                                          win32_filename)) {
        sb_free(&txt_filename);
        sb_free(&lrc_filename);
        ncm_error_set(error, EINVAL,
                      STRLIT("failed to build lyrics filename"));
        return false;
    }
    lrc_found = ncm_fs_exists(lrc_filename.data, lrc_filename.len);
    txt_found = ncm_fs_exists(txt_filename.data, txt_filename.len);
    native_lyrics_report_sidecar_status(&lrc_filename, lrc_found,
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
        ncm_error_set(error, EINVAL,
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
        native_lyrics_screen_clear_lyrics_state(
            screen, NATIVE_LYRICS_MODE_FETCH_LOG);
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
        ncm_error_clear(error);
        return true;
    }

    if (native_lyrics_screen_load_file(screen,
                                       lrc_filename.data,
                                       lrc_filename.len,
                                       error)) {
        sb_copy(&screen->filename, &lrc_filename);
        sb_free(&txt_filename);
        sb_free(&lrc_filename);
        ncm_error_clear(error);
        return true;
    }
    if (native_lyrics_screen_load_file(screen,
                                       txt_filename.data,
                                       txt_filename.len,
                                       error)) {
        sb_copy(&screen->filename, &txt_filename);
        sb_free(&txt_filename);
        sb_free(&lrc_filename);
        ncm_error_clear(error);
        return true;
    }

    if (!native_lyrics_screen_start_foreground_fetch(screen,
                                                     song,
                                                     fetcher,
                                                     &txt_filename,
                                                     error)) {
        sb_free(&txt_filename);
        sb_free(&lrc_filename);
        return false;
    }

    sb_free(&txt_filename);
    sb_free(&lrc_filename);
    ncm_error_clear(error);
    return true;
}

bool
native_lyrics_screen_fetch_in_background(NativeLyricsScreen *screen,
                                         NcmSong *song,
                                         bool notify,
                                         NcmError *error) {
    if ((screen == NULL) || (song == NULL) || ncm_song_empty(song)) {
        ncm_error_set(error, EINVAL, STRLIT("missing song"));
        return false;
    }
    if (!native_lyrics_queue_song(screen, song, notify)) {
        ncm_error_set(error, EINVAL, STRLIT("failed to queue song"));
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
    int32 result = ncm_job_queue_dispatch_completed(&screen->jobs);
    (void)native_lyrics_start_next_background(screen, NULL);
    return result;
}

void
native_lyrics_screen_update(NativeLyricsScreen *screen) {
    native_lyrics_screen_update_progress(screen);
    native_lyrics_screen_dispatch_jobs(screen);
    if (native_lyrics_screen_update_sync_line(screen)) {
        nc_lyrics_screen_request_refresh(&screen->screen);
    }
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
    StrBuilder filename = {0};
    bool win32_filename;

    if (!screen->has_song) {
        ncm_error_set(error, EINVAL, STRLIT("no current song"));
        return;
    }

    win32_filename = Config.generate_win32_compatible_filenames;
    if (!native_lyrics_filename_from_song(&filename,
                                          &screen->song,
                                          Config.mpd_music_dir,
                                          Config.mpd_music_dir_len,
                                          Config.lyrics_directory,
                                          Config.lyrics_directory_len,
                                          Config.store_lyrics_in_song_dir,
                                          win32_filename)) {
        ncm_error_set(error, EINVAL,
                      STRLIT("failed to build lyrics filename"));
        sb_free(&filename);
        return;
    }
    if (!ncm_fs_unlink(filename.data, filename.len, error)) {
        native_lyrics_report_unlink_error(&filename, error);
        sb_free(&filename);
        return;
    }

    if (!native_lyrics_screen_start_foreground_fetch(screen,
                                                     &screen->song,
                                                     screen->fetcher,
                                                     &filename,
                                                     error)) {
        sb_free(&filename);
        return;
    }

    sb_free(&filename);
    ncm_error_clear(error);
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
native_lyrics_screen_song(NativeLyricsScreen *screen) {
    if (!screen->has_song) {
        return NULL;
    }
    return &screen->song;
}

StrBuilder *
native_lyrics_screen_filename(NativeLyricsScreen *screen) {
    return &screen->filename;
}

NativeLyricsMode
native_lyrics_screen_mode(NativeLyricsScreen *screen) {
    return screen->mode;
}

NcmLrcDocument *
native_lyrics_screen_lrc(NativeLyricsScreen *screen) {
    return &screen->lrc;
}

int32
native_lyrics_screen_active_lrc_line(NativeLyricsScreen *screen) {
    if (screen == NULL) {
        return NATIVE_LYRICS_NO_ACTIVE_LINE;
    }
    return screen->active_lrc_line;
}

bool
native_lyrics_buffer_find(NcBuffer *buffer,
                          char *pattern, int32 pattern_len, NcmError *error) {
    NativeLyricsFindState state;
    NcmRegex regex;
    char *data;
    bool result;

    if (buffer == NULL) {
        ncm_error_set(error, EINVAL, STRLIT("missing lyrics buffer"));
        return false;
    }

    nc_buffer_remove_properties(buffer, NATIVE_LYRICS_SEARCH_PROPERTY_ID);
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
        ncm_error_set(error, EINVAL, STRLIT("missing lyrics screen"));
        return false;
    }

    result = native_lyrics_buffer_find(&screen->display, pattern,
                                       pattern_len, error);
    if ((pattern == NULL) || (pattern_len <= 0)) {
        sb_clear(&screen->search_constraint);
    } else if (!ncm_error_is_set(error)) {
        (void)sb_set(&screen->search_constraint, pattern,
                             pattern_len);
    }
    nc_scrollpad_flush(&screen->scrollpad, &screen->window,
                       &screen->display);
    native_lyrics_display(screen);
    return result;
}

void
native_lyrics_buffer_clear_sync_highlight(NcBuffer *buffer) {
    if (buffer == NULL) {
        return;
    }

    nc_buffer_remove_properties(buffer, NATIVE_LYRICS_SYNC_PROPERTY_ID);
    return;
}

void
native_lyrics_buffer_highlight_sync_line(NcBuffer *buffer,
                                         int32 start, int32 end) {
    NcFormattedColor highlight;

    if (buffer == NULL) {
        return;
    }

    native_lyrics_buffer_clear_sync_highlight(buffer);
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
                                  NATIVE_LYRICS_SYNC_PROPERTY_ID);
    nc_buffer_add_formatted_color_end(buffer, end, &highlight,
                                      NATIVE_LYRICS_SYNC_PROPERTY_ID);
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

static bool
native_lyrics_screen_update_sync_line(NativeLyricsScreen *screen) {
    return native_lyrics_screen_update_sync_line_force(screen, false);
}

static bool
native_lyrics_screen_update_sync_line_force(
    NativeLyricsScreen *screen,
    bool force
) {
    NcmLrcEntry *entry;
    int32 active_line;

    if (screen == NULL) {
        return false;
    }
    if (screen->mode != NATIVE_LYRICS_MODE_SYNCHRONIZED) {
        if (screen->active_lrc_line == NATIVE_LYRICS_NO_ACTIVE_LINE) {
            return false;
        }
        native_lyrics_screen_clear_sync_line(screen);
        return true;
    }

    active_line = ncm_lrc_document_entry_at_time(
        &screen->lrc, ncm_status_state_elapsed_time_ms());
    if (!force && (active_line == screen->active_lrc_line)) {
        return false;
    }

    screen->active_lrc_line = active_line;
    if (active_line == NATIVE_LYRICS_NO_ACTIVE_LINE) {
        native_lyrics_buffer_clear_sync_highlight(&screen->display);
        nc_scrollpad_reset(&screen->scrollpad);
        return true;
    }
    if (active_line >= screen->lrc.entries_len) {
        native_lyrics_buffer_clear_sync_highlight(&screen->display);
        nc_scrollpad_reset(&screen->scrollpad);
        return true;
    }

    entry = &screen->lrc.entries[active_line];
    native_lyrics_buffer_highlight_sync_line(&screen->display,
                                             entry->buffer_start,
                                             entry->buffer_end);
    nc_scrollpad_center_on_buffer_position(&screen->scrollpad,
                                           &screen->window,
                                           &screen->display,
                                           entry->buffer_start);
    return true;
}

static void
native_lyrics_screen_clear_sync_line(NativeLyricsScreen *screen) {
    if (screen == NULL) {
        return;
    }

    screen->active_lrc_line = NATIVE_LYRICS_NO_ACTIVE_LINE;
    native_lyrics_buffer_clear_sync_highlight(&screen->display);
    return;
}

static bool
native_lyrics_screen_start_foreground_fetch(
    NativeLyricsScreen *screen,
    NcmSong *song,
    NcmLyricsFetcherDef *fetcher,
    StrBuilder *filename,
    NcmError *error
) {
    NativeLyricsJob *job;
    NcmLyricsFetcherDef *active_fetcher;

    native_lyrics_screen_clear_lyrics_state(
        screen, NATIVE_LYRICS_MODE_FETCH_LOG);
    nc_scrollpad_reset(&screen->scrollpad);
    nc_lyrics_screen_reset_scroll_begin(&screen->screen);
    ncm_lyrics_result_clear(&screen->result);
    if (song != &screen->song) {
        ncm_song_copy(&screen->song, song);
    }
    screen->has_song = true;
    sb_copy(&screen->filename, filename);

    active_fetcher = native_lyrics_active_fetcher(screen, fetcher);
    if (active_fetcher) {
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
    screen->foreground_job = job;
    if (!ncm_job_queue_push(&screen->jobs,
                            (NcmJob){
                                .run = native_lyrics_job_run,
                                .complete = native_lyrics_job_complete,
                                .destroy = native_lyrics_job_destroy,
                                .user = job,
                            },
                            error)) {
        screen->foreground_job = NULL;
        native_lyrics_job_destroy(job);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

static int32
native_lyrics_screen_sync_timeout(NativeLyricsScreen *screen) {
    int32 next_line;
    int64 elapsed_ms;
    int64 remaining_ms;

    if (screen == NULL) {
        return NATIVE_LYRICS_DEFAULT_TIMEOUT_MS;
    }
    if (screen->mode != NATIVE_LYRICS_MODE_SYNCHRONIZED) {
        return NATIVE_LYRICS_DEFAULT_TIMEOUT_MS;
    }
    if (ncm_status_state_player() != NCM_STATUS_PLAYER_PLAY) {
        return NATIVE_LYRICS_DEFAULT_TIMEOUT_MS;
    }

    elapsed_ms = ncm_status_state_elapsed_time_ms();
    next_line = ncm_lrc_document_next_entry_after_time(&screen->lrc,
                                                       elapsed_ms);
    if (next_line < 0) {
        return NATIVE_LYRICS_DEFAULT_TIMEOUT_MS;
    }
    if (next_line >= screen->lrc.entries_len) {
        return NATIVE_LYRICS_DEFAULT_TIMEOUT_MS;
    }

    remaining_ms = (int64)screen->lrc.entries[next_line].time_ms
                   - elapsed_ms;
    remaining_ms = CLAMP(remaining_ms,
                         NATIVE_LYRICS_SYNC_TIMEOUT_MIN_MS,
                         NATIVE_LYRICS_SYNC_TIMEOUT_MAX_MS);
    return (int32)remaining_ms;
}

static int32
lyrics_window_timeout_callback(NcScreen *screen) {
    return native_lyrics_screen_sync_timeout(lyrics_from_screen(screen));
}

static char *
lyrics_title_callback(NcScreen *screen) {
    StrBuilder song_title = {0};
    StrBuilder scroll_buffer = {0};
    int32 scroll_begin;
    int32 scroll_width;
    char separator[] = " ** ";
    NativeLyricsScreen *lyrics = lyrics_from_screen(screen);

    sb_clear(&lyrics->title);
    SB_APPEND(&lyrics->title, STRLIT(NATIVE_LYRICS_TITLE));
    if (!lyrics->has_song || ncm_song_empty(&lyrics->song)) {
        return lyrics->title.data;
    }

    native_lyrics_title_song_string(&lyrics->song, &song_title);
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
    native_lyrics_screen_update(lyrics_from_screen(screen));
    return;
}

static void
lyrics_mouse_button_pressed_callback(NcScreen *screen, MEVENT event) {
    NativeLyricsScreen *lyrics = lyrics_from_screen(screen);

    if ((event.bstate & BUTTON5_PRESSED) != 0) {
        native_lyrics_mouse_scroll(lyrics, NC_SCROLL_DOWN);
    } else if ((event.bstate & BUTTON4_PRESSED) != 0) {
        native_lyrics_mouse_scroll(lyrics, NC_SCROLL_UP);
    }

    return;
}

static void
native_lyrics_title_song_string(NcmSong *song, StrBuilder *title) {
    NcmStringView artist_view;
    NcmStringView title_view;
    NcmStringView name_view;

    ncm_string_view_init(&artist_view);
    ncm_string_view_init(&title_view);
    ncm_string_view_init(&name_view);

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
native_lyrics_song_artist_title(NcmSong *song,
                                StrBuilder *artist, StrBuilder *title) {
    StrBuilder fallback = {0};
    NcmStringView artist_view;
    NcmStringView title_view;
    NcmStringView name_view;

    ncm_string_view_init(&artist_view);
    ncm_string_view_init(&title_view);
    ncm_string_view_init(&name_view);

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

    native_lyrics_remove_extension(&fallback);
    sb_copy(title, &fallback);
    sb_free(&fallback);

    return title->len > 0;
}

static bool
native_lyrics_fetch_artist_title(NcmSong *song,
                                 StrBuilder *artist, StrBuilder *title) {
    if (!native_lyrics_song_artist_title(song, artist, title)) {
        return false;
    }
    if (artist->len <= 0) {
        native_lyrics_replace_search_separators(title);
    }
    return true;
}

static void
native_lyrics_replace_search_separators(StrBuilder *buffer) {
    for (int32 i = 0; i < buffer->len; i += 1) {
        if ((buffer->data[i] == '-') || (buffer->data[i] == '_')) {
            buffer->data[i] = ' ';
        }
    }
    return;
}

static void
native_lyrics_append_locale(NcBuffer *buffer, char *data, int32 data_len) {
    StrBuilder converted = ncm_charset_utf8_to_locale(data, data_len);
    nc_buffer_append_data(buffer, converted.data, converted.len);
    sb_free(&converted);
    return;
}

static bool
native_lyrics_screen_render_lrc(NativeLyricsScreen *screen,
                                NcmError *error) {
    NcmLrcRenderTarget target = {0};

    target.user = screen;
    target.position = native_lyrics_lrc_buffer_position;
    target.append = native_lyrics_lrc_buffer_append;

    if (!ncm_lrc_document_render_plain(&screen->lrc, &target)) {
        ncm_error_set(error, EINVAL, STRLIT("failed to render LRC"));
        return false;
    }

    ncm_error_clear(error);
    return true;
}

static int32
native_lyrics_lrc_buffer_position(void *user) {
    NativeLyricsScreen *screen = user;

    if (screen == NULL) {
        return 0;
    }

    return nc_buffer_len(&screen->display);
}

static void
native_lyrics_lrc_buffer_append(void *user,
                                char *data, int32 data_len) {
    NativeLyricsScreen *screen = user;

    if (screen == NULL) {
        return;
    }

    native_lyrics_append_locale(&screen->display, data, data_len);
    return;
}

static void
native_lyrics_report_sidecar_status(StrBuilder *lrc_filename,
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
native_lyrics_report_save_error(StrBuilder *filename, NcmError *error) {
    NcmStringFormatArg args[2];
    char *message = "unknown error";

    if (error && (error->code != 0)) {
        message = strerror(error->code);
    }

    args[0] = ncm_string_format_arg_string(filename->data, filename->len);
    args[1] = ncm_string_format_arg_cstring(message);
    ncm_statusbar_format(Config.message_delay_time,
                         STRLIT("Couldn't save lyrics as \"%1%\": %2%"),
                         args, LENGTH(args));

    return;
}

static void
native_lyrics_report_unlink_error(StrBuilder *filename, NcmError *error) {
    NcmStringFormatArg args[2];
    char *message = "unknown error";

    if (error && (error->code != 0)) {
        message = strerror(error->code);
    }
    args[0] = ncm_string_format_arg_string(filename->data, filename->len);
    args[1] = ncm_string_format_arg_cstring(message);
    ncm_statusbar_format(Config.message_delay_time,
                         STRLIT("Couldn't remove \"%1%\": %2%"),
                         args, LENGTH(args));
    return;
}

static void
native_lyrics_screen_clear_lyrics_state(NativeLyricsScreen *screen,
                                        NativeLyricsMode mode) {
    nc_buffer_clear(&screen->display);
    ncm_lrc_document_clear(&screen->lrc);
    screen->active_lrc_line = NATIVE_LYRICS_NO_ACTIVE_LINE;
    screen->mode = mode;
    return;
}

static void
native_lyrics_remove_extension(StrBuilder *buffer) {
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
native_lyrics_filename_from_song_with_extension(
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

    ncm_string_view_init(&uri);
    sb_clear(filename);

    if (store_in_song_dir && !ncm_song_is_stream(song)) {
        if (ncm_song_is_from_database(song) && (music_dir_len > 0)) {
            SB_APPEND(filename, music_dir, music_dir_len);
            sb_append_byte_if_not(filename, '/');
        }
        if (ncm_song_uri_view(song, 0, &uri)) {
            SB_APPEND(filename, uri.data, uri.len);
        }
        native_lyrics_remove_extension(filename);
    } else {
        (void)native_lyrics_song_artist_title(song, &artist, &title);
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
native_lyrics_filename_from_song(StrBuilder *filename, NcmSong *song,
                                 char *music_dir, int32 music_dir_len,
                                 char *lyrics_dir, int32 lyrics_dir_len,
                                 bool store_in_song_dir, bool win32_filename) {
    return native_lyrics_filename_from_song_with_extension(filename,
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
native_lyrics_preferred_filename_from_song(StrBuilder *filename,
                                           NcmSong *song,
                                           char *music_dir,
                                           int32 music_dir_len,
                                           char *lyrics_dir,
                                           int32 lyrics_dir_len,
                                           bool store_in_song_dir,
                                           bool win32_filename) {
    StrBuilder lrc_filename = {0};
    bool success;

    success = native_lyrics_filename_from_song_with_extension(&lrc_filename,
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

    return native_lyrics_filename_from_song(filename,
                                           song,
                                           music_dir,
                                           music_dir_len,
                                           lyrics_dir,
                                           lyrics_dir_len,
                                           store_in_song_dir,
                                           win32_filename);
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
    bool win32_filename;
    NativeLyricsJob *job = malloc2(SIZEOF(*job));

    job->screen = screen;
    ncm_song_init(&job->song);
    ncm_song_copy(&job->song, song);
    sb_init(&job->filename);
    nc_buffer_init(&job->log);
    pthread_mutex_init(&job->log_mutex, NULL);
    job->log_dirty = false;

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
native_lyrics_job_fetch_one(NativeLyricsJob *job, NcmLyricsFetcherDef *fetcher,
                            StrBuilder *artist, StrBuilder *title) {
    if (fetcher == NULL) {
        return false;
    }

    native_lyrics_job_append_fetching(job, fetcher);
    if (!ncm_lyrics_fetcher_fetch(fetcher,
                                  &job->result,
                                  artist->data,
                                  artist->len,
                                  title->data,
                                  title->len)) {
        native_lyrics_job_append_fetch_error(job, &job->result);
        return false;
    }
    if (!job->result.success) {
        native_lyrics_job_append_fetch_error(job, &job->result);
        return false;
    }
    return true;
}

static bool
native_lyrics_job_fetch(NativeLyricsJob *job,
                        StrBuilder *artist, StrBuilder *title) {
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
    StrBuilder artist = {0};
    StrBuilder title = {0};
    bool success;
    NativeLyricsJob *job = user;


    if (!native_lyrics_fetch_artist_title(&job->song, &artist, &title)) {
        ncm_error_set(error, EINVAL, STRLIT("missing song metadata"));
        sb_free(&title);
        sb_free(&artist);
        return false;
    }

    success = native_lyrics_job_fetch(job, &artist, &title);
    sb_free(&title);
    sb_free(&artist);
    if (!success || !job->result.success) {
        ncm_error_set(error, EINVAL, STRLIT("lyrics not found"));
    }

    return success && job->result.success;
}

static void
native_lyrics_job_complete(bool success, NcmError *error, void *user) {
    NativeLyricsJob *job = user;
    NativeLyricsScreen *screen = job->screen;

    (void)success;
    (void)error;

    if (!job->background) {
        if (screen->foreground_job == job) {
            screen->foreground_job = NULL;
        }
        if (!native_lyrics_job_is_current(job)) {
            return;
        }

        ncm_lyrics_result_clear(&screen->result);
        (void)ncm_lyrics_result_set(&screen->result,
                                    job->result.success,
                                    job->result.text,
                                    job->result.text_len);
        if (job->result.success) {
            NcmError save_error = {0};

            native_lyrics_screen_clear_lyrics_state(
                screen, NATIVE_LYRICS_MODE_PLAIN);
            native_lyrics_append_locale(&screen->display,
                                        job->result.text,
                                        job->result.text_len);
            sb_copy(&screen->filename, &job->filename);
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
            native_lyrics_screen_clear_lyrics_state(
                screen, NATIVE_LYRICS_MODE_FETCH_LOG);
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
    sb_free(&job->filename);
    ncm_lyrics_result_destroy(&job->result);
    nc_buffer_destroy(&job->log);
    pthread_mutex_destroy(&job->log_mutex);
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
native_lyrics_append_fetching(NcBuffer *buffer, NcmLyricsFetcherDef *fetcher) {
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
                         NATIVE_LYRICS_FETCH_PROPERTY_ID);
    nc_buffer_append_data(buffer, name, name_len);
    nc_buffer_add_format(buffer,
                         nc_buffer_len(buffer),
                         NC_FORMAT_NO_BOLD,
                         NATIVE_LYRICS_FETCH_PROPERTY_ID);
    nc_buffer_append_cstring(buffer, "... ");

    return;
}

static void
native_lyrics_append_fetch_error(NcBuffer *buffer, NcmLyricsResult *result) {
    NcColor red = nc_color_make(COLOR_RED, NC_COLOR_CURRENT, false, false);
    nc_buffer_add_color(buffer,
                        nc_buffer_len(buffer),
                        red,
                        NATIVE_LYRICS_FETCH_PROPERTY_ID);
    nc_buffer_append_data(buffer, result->text, result->text_len);
    nc_buffer_add_color(buffer,
                        nc_buffer_len(buffer),
                        nc_color_end(),
                        NATIVE_LYRICS_FETCH_PROPERTY_ID);
    nc_buffer_append_char(buffer, '\n');

    return;
}

static void
native_lyrics_job_append_fetching(NativeLyricsJob *job,
                                  NcmLyricsFetcherDef *fetcher) {
    if ((job == NULL) || (fetcher == NULL)) {
        return;
    }

    pthread_mutex_lock(&job->log_mutex);
    native_lyrics_append_fetching(&job->log, fetcher);
    job->log_dirty = true;
    pthread_mutex_unlock(&job->log_mutex);

    return;
}

static void
native_lyrics_job_append_fetch_error(NativeLyricsJob *job,
                                     NcmLyricsResult *result) {
    if ((job == NULL) || (result == NULL)) {
        return;
    }

    pthread_mutex_lock(&job->log_mutex);
    native_lyrics_append_fetch_error(&job->log, result);
    job->log_dirty = true;
    pthread_mutex_unlock(&job->log_mutex);

    return;
}

static bool
native_lyrics_job_take_log(NativeLyricsJob *job, NcBuffer *buffer) {
    NcBuffer copy;
    bool result = false;

    if ((job == NULL) || (buffer == NULL)) {
        return false;
    }

    nc_buffer_init(&copy);
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
native_lyrics_screen_update_progress(NativeLyricsScreen *screen) {
    NativeLyricsJob *job;

    if (screen == NULL) {
        return;
    }

    job = screen->foreground_job;
    if (job == NULL) {
        return;
    }
    if (!native_lyrics_job_is_current(job)) {
        return;
    }
    if (native_lyrics_job_take_log(job, &screen->display)) {
        ncm_lrc_document_clear(&screen->lrc);
        screen->active_lrc_line = NATIVE_LYRICS_NO_ACTIVE_LINE;
        screen->mode = NATIVE_LYRICS_MODE_FETCH_LOG;
        nc_lyrics_screen_request_refresh(&screen->screen);
    }

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
    return STREQUAL(screen->filename.data, screen->filename.len,
                    job->filename.data, job->filename.len);
}

static void
native_lyrics_set_consumer_fetch_message(NativeLyricsScreen *screen,
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
native_lyrics_find_match_callback(int32 start, int32 len, void *user) {
    NativeLyricsFindState *state = user;

    if (len <= 0) {
        return true;
    }

    nc_buffer_add_format(state->buffer, start,
                         NC_FORMAT_REVERSE, NATIVE_LYRICS_SEARCH_PROPERTY_ID);
    nc_buffer_add_format(state->buffer, start + len,
                         NC_FORMAT_NO_REVERSE,
                         NATIVE_LYRICS_SEARCH_PROPERTY_ID);

    return true;
}

static void
native_lyrics_mouse_scroll(NativeLyricsScreen *screen, enum NcScroll where) {
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
    StrBuilder filename = {0};
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

    found_job = false;
    queued = NULL;
    win32_filename = Config.generate_win32_compatible_filenames;
    while (!found_job) {
        if ((queued = native_lyrics_dequeue_song(screen)) == NULL) {
            sb_free(&filename);
            ncm_error_clear(error);
            return true;
        }

        if (ncm_song_is_stream(&queued->song)) {
            native_lyrics_queued_song_destroy(queued);
            free2(queued, SIZEOF(*queued));
            queued = NULL;
            continue;
        }

        if (!native_lyrics_preferred_filename_from_song(
                &filename,
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
        sb_free(&filename);
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
        sb_free(&filename);
        native_lyrics_queued_song_destroy(queued);
        free2(queued, SIZEOF(*queued));
        return false;
    }

    sb_free(&filename);
    native_lyrics_queued_song_destroy(queued);
    free2(queued, SIZEOF(*queued));
    ncm_error_clear(error);
    return true;
}

#endif /* NCMPCPP_NC_LYRICS_C */
