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
static int32 lyrics_screen_start_foreground_fetch(
    LyricsScreen *screen,
    NcmSong *song,
    NcmLyricsFetcherDef *fetcher,
    StrBuilder *filename,
    NcmError *ncm_error);
static bool lyrics_screen_update_sync_line_force(
    LyricsScreen *screen,
    bool force);
static int32 lyrics_lrc_buffer_position(void *user);
static void lyrics_lrc_buffer_append(void *user,
                                     char *data, int32 data_len);
static void lyrics_screen_clear_lyrics_state(
    LyricsScreen *screen,
    LyricsMode mode);
static void lyrics_remove_extension(StrBuilder *buffer);
static int32 lyrics_filename_from_song_with_extension(
    StrBuilder *filename,
    NcmSong *song,
    char *music_dir, int32 music_dir_len,
    char *lyrics_dir, int32 lyrics_dir_len,
    bool store_in_song_dir,
    bool win32_filename,
    char *extension, int32 extension_len);
static int32 lyrics_filename_from_song(StrBuilder *filename,
                                      NcmSong *song,
                                      char *music_dir,
                                      int32 music_dir_len,
                                      char *lyrics_dir,
                                      int32 lyrics_dir_len,
                                      bool store_in_song_dir,
                                      bool win32_filename);
static int32 lyrics_preferred_filename_from_song(StrBuilder *filename,
                                                NcmSong *song,
                                                char *music_dir,
                                                int32 music_dir_len,
                                                char *lyrics_dir,
                                                int32 lyrics_dir_len,
                                                bool store_in_song_dir,
                                                bool win32_filename);
static LyricsJob *lyrics_job_create(LyricsScreen *screen,
                                    NcmSong *song,
                                    NcmLyricsFetcherDef *fetcher,
                                    bool notify,
                                    bool background);
static void lyrics_append_fetching(NcBuffer *buffer,
                                   NcmLyricsFetcherDef *fetcher);
static void lyrics_job_append_fetch_error(LyricsJob *job,
                                          NcmLyricsResult *result);
static bool lyrics_job_is_current(LyricsJob *job);
static int32 lyrics_job_run(void *user, NcmError *ncm_error);
static void lyrics_job_complete(int32 status, NcmError *ncm_error,
                                void *user);
static void lyrics_job_destroy(void *user);
static int32 lyrics_start_next_background(LyricsScreen *screen,
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

int32
nc_lyrics_screen_take_refresh_request(NcLyricsScreen *screen) {
    bool result;

    if (screen == NULL) {
        return -EINVAL;
    }

    result = screen->refresh_window;
    screen->refresh_window = false;
    if (result) {
        return 1;
    }
    return 0;
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
        lyrics_screen_update_sync_line_force(screen, true);
    }
    nc_scrollpad_flush(&screen->scrollpad,
                       &screen->window,
                       &screen->display);
    nc_lyrics_screen_request_refresh(&screen->screen);
    return;
}

int32
lyrics_screen_build_filename(LyricsScreen *screen,
                             NcmSong *song,
                             char *music_dir, int32 music_dir_len,
                             char *lyrics_dir, int32 lyrics_dir_len,
                             bool store_in_song_dir,
                             bool win32_filename) {
    if ((screen == NULL) || (song == NULL) || ncm_song_is_empty(song)) {
        return -EINVAL;
    }

    return lyrics_preferred_filename_from_song(&screen->filename,
                                               song,
                                               music_dir,
                                               music_dir_len,
                                               lyrics_dir,
                                               lyrics_dir_len,
                                               store_in_song_dir,
                                               win32_filename);
}

int32
lyrics_screen_load_file(LyricsScreen *screen,
                        char *filename, int32 filename_len,
                        NcmError *ncm_error) {
    FILE *file;
    StrBuilder raw = {0};
    char line[1024];
    int close_err;
    int32 line_len;
    int32 status;
    bool first;
    bool lrc_file;

    if ((screen == NULL) || (filename == NULL) || (filename_len <= 0)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing lyrics file"));
    }

    lrc_file = (filename_len > STRLIT_LEN(".lrc"))
               && ENDS_WITH(filename, filename_len, ".lrc");
    if ((file = fopen(filename, "rb")) == NULL) {
        lyrics_screen_clear_lyrics_state(screen, LYRICS_MODE_FETCH_LOG);
        return ncm_error_set_status(ncm_error, -errno,
                                    STRLIT("failed to open lyrics"));
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
        nc_buffer_append_data(&screen->display, line, line_len);
        first = false;
    }

    if ((close_err = XFCLOSE(file, filename)) < 0) {
        sb_free(&raw);
        lyrics_screen_clear_lyrics_state(
            screen, LYRICS_MODE_FETCH_LOG);
        return ncm_error_set_status(ncm_error, close_err,
                                    STRLIT("failed to close lyrics"));
    }
    if (lrc_file) {
        status = ncm_lrc_parse(&screen->lrc, raw.data, raw.len, ncm_error);
        if (status < 0) {
            sb_free(&raw);
            lyrics_screen_clear_lyrics_state(
                screen, LYRICS_MODE_FETCH_LOG);
            return status;
        }
    }

    if (lrc_file) {
        NcmLrcRenderTarget target = {0};

        nc_buffer_clear(&screen->display);
        target.user = screen;
        target.position = lyrics_lrc_buffer_position;
        target.append = lyrics_lrc_buffer_append;
        ncm_lrc_document_render_plain(&screen->lrc, &target);
        ncm_error_clear(ncm_error);
        screen->mode = LYRICS_MODE_SYNCHRONIZED;
    } else {
        ncm_lrc_document_clear(&screen->lrc);
        screen->mode = LYRICS_MODE_PLAIN;
    }
    sb_free(&raw);
    nc_lyrics_screen_request_refresh(&screen->screen);
    ncm_error_clear(ncm_error);

    return 0;
}

int32
lyrics_screen_save_file(LyricsScreen *screen,
                        char *filename, int32 filename_len,
                        char *lyrics, int32 lyrics_len,
                        NcmError *ncm_error) {
    FILE *file;
    int32 error_code;
    int32 written;
    int32 close_result;

    (void)screen;
    if ((filename == NULL) || (filename_len <= 0)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing lyrics file"));
    }
    if ((lyrics_len < 0) || ((lyrics == NULL) && (lyrics_len > 0))) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing lyrics buffer"));
    }

    if ((file = fopen(filename, "wb")) == NULL) {
        return ncm_error_set_status(ncm_error, -errno,
                                    STRLIT("failed to write lyrics"));
    }

    written = 0;
    if (lyrics && (lyrics_len > 0)) {
        written = (int32)fwrite64(lyrics, 1, lyrics_len, file);
    }
    close_result = fclose(file);
    if ((written != lyrics_len) || (close_result != 0)) {
        error_code = errno;
        if (error_code == 0) {
            error_code = EIO;
        }
        return ncm_error_set_status(ncm_error, -error_code,
                                    STRLIT("failed to save lyrics"));
    }

    ncm_error_clear(ncm_error);
    return 0;
}

int32
lyrics_screen_fetch(LyricsScreen *screen,
                    NcmSong *song,
                    NcmLyricsFetcherDef *fetcher,
                    NcmError *ncm_error) {
    StrBuilder next_filename = {0};
    StrBuilder lrc_filename = {0};
    StrBuilder txt_filename = {0};
    int32 status;
    bool changed_song;
    bool changed_filename;
    bool changed;
    bool lrc_found;
    bool txt_found;
    bool win32_filename;

    if ((screen == NULL) || (song == NULL) || ncm_song_is_empty(song)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing song"));
    }

    win32_filename = Config.generate_win32_compatible_filenames;
    status = lyrics_filename_from_song_with_extension(
        &lrc_filename,
        song,
        Config.mpd_music_dir,
        Config.mpd_music_dir_len,
        Config.lyrics_directory,
        Config.lyrics_directory_len,
        Config.store_lyrics_in_song_dir,
        win32_filename,
        STRLIT(".lrc"));
    if (status < 0) {
        sb_free(&lrc_filename);
        return ncm_error_set_status(
            ncm_error, status, STRLIT("failed to build lyrics filename"));
    }
    status = lyrics_filename_from_song(&txt_filename,
                                       song,
                                       Config.mpd_music_dir,
                                       Config.mpd_music_dir_len,
                                       Config.lyrics_directory,
                                       Config.lyrics_directory_len,
                                       Config.store_lyrics_in_song_dir,
                                       win32_filename);
    if (status < 0) {
        sb_free(&txt_filename);
        sb_free(&lrc_filename);
        return ncm_error_set_status(
            ncm_error, status, STRLIT("failed to build lyrics filename"));
    }
    lrc_found = ncm_fs_path_is_existing(lrc_filename.data, lrc_filename.len);
    txt_found = ncm_fs_path_is_existing(txt_filename.data, txt_filename.len);
    if ((lrc_filename.len > 0) && (txt_filename.len > 0)) {
        StrBuilder message = {0};
        char *lrc_status;
        char *txt_status;
        int32 lrc_start;
        int32 txt_start;

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

        lrc_start = ncm_string_basename_start(lrc_filename.data,
                                              lrc_filename.len);
        txt_start = ncm_string_basename_start(txt_filename.data,
                                              txt_filename.len);
        SB_APPEND(&message, lrc_filename.data + lrc_start,
                  lrc_filename.len - lrc_start);
        SB_APPEND(&message, " ");
        SB_APPEND(&message, lrc_status, optional_strlen32(lrc_status));
        SB_APPEND(&message, "; ");
        SB_APPEND(&message, txt_filename.data + txt_start,
                  txt_filename.len - txt_start);
        SB_APPEND(&message, " ");
        SB_APPEND(&message, txt_status, optional_strlen32(txt_status));
        ncm_statusbar_print(Config.message_delay_time,
                            message.data, message.len);
        sb_free(&message);
    }
    if (lrc_found) {
        sb_copy(&next_filename, &lrc_filename);
    } else {
        sb_copy(&next_filename, &txt_filename);
    }
    if (next_filename.len <= 0) {
        sb_free(&next_filename);
        sb_free(&txt_filename);
        sb_free(&lrc_filename);
        return ncm_error_set_status(
            ncm_error, -NCM_ERROR_NOT_FOUND,
            STRLIT("failed to build lyrics filename"));
    }

    changed_song = !screen->has_song || !ncm_song_is_equal(&screen->song, song);
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
        return 0;
    }

    status = lyrics_screen_load_file(screen,
                                     lrc_filename.data,
                                     lrc_filename.len,
                                     ncm_error);
    if (status >= 0) {
        sb_copy(&screen->filename, &lrc_filename);
        sb_free(&txt_filename);
        sb_free(&lrc_filename);
        ncm_error_clear(ncm_error);
        return 0;
    }
    status = lyrics_screen_load_file(screen,
                                     txt_filename.data,
                                     txt_filename.len,
                                     ncm_error);
    if (status >= 0) {
        sb_copy(&screen->filename, &txt_filename);
        sb_free(&txt_filename);
        sb_free(&lrc_filename);
        ncm_error_clear(ncm_error);
        return 0;
    }

    status = lyrics_screen_start_foreground_fetch(screen,
                                                  song,
                                                  fetcher,
                                                  &txt_filename,
                                                  ncm_error);
    if (status < 0) {
        sb_free(&txt_filename);
        sb_free(&lrc_filename);
        return status;
    }

    sb_free(&txt_filename);
    sb_free(&lrc_filename);
    ncm_error_clear(ncm_error);
    return 0;
}

int32
lyrics_screen_fetch_in_background(LyricsScreen *screen,
                                  NcmSong *song,
                                  bool notify,
                                  NcmError *ncm_error) {
    LyricsQueuedSong *queued;
    int32 new_cap;
    int32 status;

    if ((screen == NULL) || (song == NULL) || ncm_song_is_empty(song)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing song"));
    }
    if (screen->queued_songs_len >= screen->queued_songs_cap) {
        new_cap = screen->queued_songs_cap;
        if (new_cap <= 0) {
            new_cap = 8;
        } else {
            new_cap *= 2;
        }
        screen->queued_songs = realloc2(screen->queued_songs,
                                        screen->queued_songs_cap, new_cap,
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
    status = lyrics_start_next_background(screen, ncm_error);
    if (status < 0) {
        return status;
    }
    ncm_error_clear(ncm_error);
    return 0;
}

int32
lyrics_screen_dispatch_jobs(LyricsScreen *screen) {
    int32 result = ncm_job_queue_dispatch_completed(&screen->jobs);

    lyrics_start_next_background(screen, NULL);
    return result;
}

void
lyrics_screen_update(LyricsScreen *screen) {
    LyricsJob *job = screen->foreground_job;

    if (job && lyrics_job_is_current(job)) {
        NcBuffer copy = {0};
        bool log_dirty = false;

        pthread_mutex_lock(&job->log_mutex);
        if (job->log_dirty) {
            nc_buffer_copy(&copy, &job->log);
            job->log_dirty = false;
            log_dirty = true;
        }
        pthread_mutex_unlock(&job->log_mutex);

        if (log_dirty) {
            nc_buffer_destroy(&screen->display);
            nc_buffer_move(&screen->display, &copy);
            ncm_lrc_document_clear(&screen->lrc);
            screen->active_lrc_line = LYRICS_NO_ACTIVE_LINE;
            screen->mode = LYRICS_MODE_FETCH_LOG;
            nc_lyrics_screen_request_refresh(&screen->screen);
        } else {
            nc_buffer_destroy(&copy);
        }
    }

    lyrics_screen_dispatch_jobs(screen);
    if (lyrics_screen_update_sync_line_force(screen, false)) {
        nc_lyrics_screen_request_refresh(&screen->screen);
    }
    if (nc_lyrics_screen_take_refresh_request(&screen->screen) > 0) {
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
    int32 status;
    bool win32_filename;

    if (!screen->has_song) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("no current song"));
        return;
    }

    win32_filename = Config.generate_win32_compatible_filenames;
    status = lyrics_filename_from_song(&filename,
                                       &screen->song,
                                       Config.mpd_music_dir,
                                       Config.mpd_music_dir_len,
                                       Config.lyrics_directory,
                                       Config.lyrics_directory_len,
                                       Config.store_lyrics_in_song_dir,
                                       win32_filename);
    if (status < 0) {
        ncm_error_set_status(ncm_error, status,
                             STRLIT("failed to build lyrics filename"));
        sb_free(&filename);
        return;
    }
    if (ncm_fs_unlink(filename.data, filename.len, ncm_error) < 0) {
        StrBuilder output = {0};
        char *message = "unknown error";

        if (ncm_error && (ncm_error->code != 0)) {
            message = strerror(ncm_error->code);
        }
        SB_APPEND(&output, "Couldn't remove \"");
        SB_APPEND(&output, filename.data, filename.len);
        SB_APPEND(&output, "\": ");
        SB_APPEND(&output, message, optional_strlen32(message));
        ncm_statusbar_print(Config.message_delay_time,
                            output.data, output.len);
        sb_free(&output);
        sb_free(&filename);
        return;
    }

    status = lyrics_screen_start_foreground_fetch(screen,
                                                  &screen->song,
                                                  screen->fetcher,
                                                  &filename,
                                                  ncm_error);
    if (status < 0) {
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

int32
lyrics_screen_try_take_consumer_message(LyricsScreen *screen,
                                        StrBuilder *message) {
    if ((screen == NULL) || (message == NULL)) {
        return -EINVAL;
    }
    if (screen->consumer_message.len <= 0) {
        return 0;
    }
    sb_copy(message, &screen->consumer_message);
    sb_clear(&screen->consumer_message);
    return 1;
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

int32
lyrics_buffer_find(NcBuffer *buffer,
                   char *pattern, int32 pattern_len, NcmError *ncm_error) {
    LyricsFindState state;
    NcmRegex regex;
    char *data;
    int32 match_count;
    int32 status;

    if (buffer == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing lyrics buffer"));
    }

    nc_buffer_remove_properties(buffer, LYRICS_SEARCH_PROPERTY_ID);
    if ((pattern == NULL) || (pattern_len <= 0)) {
        return ncm_error_ok(ncm_error);
    }

    regex = (NcmRegex){0};
    if ((status = ncm_regex_compile(&regex, pattern, pattern_len,
                                    Config.regex_flags, ncm_error)) < 0) {
        ncm_regex_destroy(&regex);
        return status;
    }

    state.buffer = buffer;
    data = nc_buffer_data(buffer);
    match_count = ncm_regex_for_each_match(
        &regex, data, buffer->len, lyrics_find_match_callback, &state);
    ncm_regex_destroy(&regex);
    if (match_count > 0) {
        return 1;
    }
    return 0;
}

int32
lyrics_screen_find(LyricsScreen *screen,
                   char *pattern, int32 pattern_len,
                   NcmError *ncm_error) {
    int32 result;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing lyrics screen"));
    }

    result = lyrics_buffer_find(&screen->display, pattern,
                                pattern_len, ncm_error);
    if (result < 0) {
        nc_scrollpad_flush(&screen->scrollpad, &screen->window,
                           &screen->display);
        lyrics_display(screen);
        return result;
    }

    if ((pattern == NULL) || (pattern_len <= 0)) {
        sb_clear(&screen->search_constraint);
    } else {
        sb_set(&screen->search_constraint, pattern, pattern_len);
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
    title = nc_screen_title(screen);
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
lyrics_screen_update_sync_line_force(
    LyricsScreen *screen,
    bool force
) {
    NcmLrcEntry *entry;
    int32 active_line;

    if (screen->mode != LYRICS_MODE_SYNCHRONIZED) {
        if (screen->active_lrc_line == LYRICS_NO_ACTIVE_LINE) {
            return false;
        }
        screen->active_lrc_line = LYRICS_NO_ACTIVE_LINE;
        lyrics_buffer_clear_sync_highlight(&screen->display);
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

static int32
lyrics_screen_start_foreground_fetch(
    LyricsScreen *screen,
    NcmSong *song,
    NcmLyricsFetcherDef *fetcher,
    StrBuilder *filename,
    NcmError *ncm_error
) {
    LyricsJob *job;
    NcmLyricsFetcherDef *active_fetcher;
    int32 status;

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

    active_fetcher = fetcher;
    if (active_fetcher == NULL) {
        active_fetcher = screen->fetcher;
    }
    if (active_fetcher) {
        lyrics_append_fetching(&screen->display, active_fetcher);
    } else if (Config.lyrics_fetchers.fetchers.len > 0) {
        lyrics_append_fetching(
            &screen->display, &Config.lyrics_fetchers.fetchers.items[0]);
    }
    nc_lyrics_screen_request_refresh(&screen->screen);

    status = ncm_job_queue_start(&screen->jobs, ncm_error);
    if (status < 0) {
        return status;
    }

    job = lyrics_job_create(screen, song, active_fetcher, false, false);
    screen->foreground_job = job;
    status = ncm_job_queue_push(&screen->jobs,
                                (NcmJob){
                                    .run = lyrics_job_run,
                                    .complete = lyrics_job_complete,
                                    .destroy = lyrics_job_destroy,
                                    .user = job,
                                },
                                ncm_error);
    if (status < 0) {
        screen->foreground_job = NULL;
        lyrics_job_destroy(job);
        return status;
    }

    ncm_error_clear(ncm_error);
    return 0;
}

static int32
lyrics_window_timeout_callback(NcScreen *screen) {
    LyricsScreen *lyrics = lyrics_from_screen(screen);
    int32 next_line;
    int64 elapsed_ms;
    int64 remaining_ms;

    if (lyrics->mode != LYRICS_MODE_SYNCHRONIZED) {
        return LYRICS_DEFAULT_TIMEOUT_MS;
    }
    if (ncm_status_state_player() != NCM_STATUS_PLAYER_PLAY) {
        return LYRICS_DEFAULT_TIMEOUT_MS;
    }

    elapsed_ms = ncm_status_state_elapsed_time_ms();
    next_line = ncm_lrc_document_next_entry_after_time(&lyrics->lrc,
                                                       elapsed_ms);
    if (next_line < 0) {
        return LYRICS_DEFAULT_TIMEOUT_MS;
    }
    remaining_ms = (int64)lyrics->lrc.entries[next_line].time_ms
                   - elapsed_ms;
    remaining_ms = CLAMP(remaining_ms, LYRICS_SYNC_TIMEOUT_MIN_MS,
                         LYRICS_SYNC_TIMEOUT_MAX_MS);
    return (int32)remaining_ms;
}

static char *
lyrics_title_callback(NcScreen *screen) {
    StrBuilder song_title = {0};
    StrBuilder scroll_buffer = {0};
    NcmStringView artist_view = {0};
    NcmStringView title_view = {0};
    NcmStringView name_view = {0};
    int32 scroll_begin;
    int32 scroll_width;
    char separator[] = " ** ";
    LyricsScreen *lyrics = lyrics_from_screen(screen);

    sb_clear(&lyrics->title);
    SB_APPEND(&lyrics->title, STRLIT(LYRICS_TITLE));
    if (!lyrics->has_song || ncm_song_is_empty(&lyrics->song)) {
        return lyrics->title.data;
    }

    sb_clear(&song_title);
    if (ncm_song_has_tag_view(&lyrics->song, MPD_TAG_ARTIST, 0, &artist_view)
        && ncm_song_has_tag_view(&lyrics->song, MPD_TAG_TITLE, 0,
                                 &title_view)) {
        SB_APPEND(&song_title, artist_view.data, artist_view.len);
        SB_APPEND(&song_title, " - ");
        SB_APPEND(&song_title, title_view.data, title_view.len);
    } else if (ncm_song_has_name_view(&lyrics->song, 0, &name_view)) {
        SB_APPEND(&song_title, name_view.data, name_view.len);
    }
    if (song_title.len <= 0) {
        sb_free(&song_title);
        return lyrics->title.data;
    }

    SB_APPEND(&lyrics->title, ": ");
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

    if (ncm_song_has_tag_view(song, MPD_TAG_ARTIST, 0, &artist_view)
        && ncm_song_has_tag_view(song, MPD_TAG_TITLE, 0, &title_view)) {
        SB_APPEND(artist, artist_view.data, artist_view.len);
        SB_APPEND(title, title_view.data, title_view.len);
        return true;
    }

    if (ncm_song_has_name_view(song, 0, &name_view)) {
        SB_APPEND(&fallback, name_view.data, name_view.len);
    } else if (ncm_song_has_uri_view(song, 0, &name_view)) {
        SB_APPEND(&fallback, name_view.data, name_view.len);
    }

    lyrics_remove_extension(&fallback);
    sb_copy(title, &fallback);
    sb_free(&fallback);
    return title->len > 0;
}

static int32
lyrics_lrc_buffer_position(void *user) {
    LyricsScreen *screen = user;

    return screen->display.len;
}

static void
lyrics_lrc_buffer_append(void *user,
                         char *data, int32 data_len) {
    LyricsScreen *screen = user;

    nc_buffer_append_data(&screen->display, data, data_len);
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

static int32
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
        if (ncm_song_has_uri_view(song, 0, &uri)) {
            SB_APPEND(filename, uri.data, uri.len);
        }
        lyrics_remove_extension(filename);
    } else {
        lyrics_song_artist_title(song, &artist, &title);
        if (lyrics_dir_len > 0) {
            SB_APPEND(filename, lyrics_dir, lyrics_dir_len);
            sb_append_byte_if_not(filename, '/');
        }
        basename_start = filename->len;
        if ((artist.len > 0) && (title.len > 0)) {
            SB_APPEND(filename, artist.data, artist.len);
            SB_APPEND(filename, " - ");
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

    if (filename->len <= extension_len) {
        return -NCM_ERROR_NOT_FOUND;
    }

    return 0;
}

static int32
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

static int32
lyrics_preferred_filename_from_song(StrBuilder *filename,
                                    NcmSong *song,
                                    char *music_dir,
                                    int32 music_dir_len,
                                    char *lyrics_dir,
                                    int32 lyrics_dir_len,
                                    bool store_in_song_dir,
                                    bool win32_filename) {
    StrBuilder lrc_filename = {0};
    int32 status;

    status = lyrics_filename_from_song_with_extension(&lrc_filename,
                                                      song,
                                                      music_dir,
                                                      music_dir_len,
                                                      lyrics_dir,
                                                      lyrics_dir_len,
                                                      store_in_song_dir,
                                                      win32_filename,
                                                      STRLIT(".lrc"));
    if (status < 0) {
        sb_free(&lrc_filename);
        return status;
    }
    if (ncm_fs_path_is_existing(lrc_filename.data, lrc_filename.len)) {
        sb_copy(filename, &lrc_filename);
        sb_free(&lrc_filename);
        return 0;
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

static LyricsJob *
lyrics_job_create(LyricsScreen *screen,
                  NcmSong *song,
                  NcmLyricsFetcherDef *fetcher,
                  bool notify,
                  bool background) {
    bool win32_filename;
    LyricsJob *job = malloc2(SIZEOF(*job));

    *job = (LyricsJob){0};
    job->screen = screen;
    ncm_song_copy(&job->song, song);
    pthread_mutex_init(&job->log_mutex, NULL);
    job->log_dirty = false;

    win32_filename = Config.generate_win32_compatible_filenames;

    lyrics_filename_from_song(&job->filename,
                              song,
                              Config.mpd_music_dir,
                              Config.mpd_music_dir_len,
                              Config.lyrics_directory,
                              Config.lyrics_directory_len,
                              Config.store_lyrics_in_song_dir,
                              win32_filename);

    job->fetcher = fetcher;
    job->notify = notify;
    job->background = background;

    return job;
}

static int32
lyrics_job_fetch_one(LyricsJob *job, NcmLyricsFetcherDef *fetcher,
                     StrBuilder *artist, StrBuilder *title) {
    int32 status;

    ASSERT(job != NULL);

    pthread_mutex_lock(&job->log_mutex);
    lyrics_append_fetching(&job->log, fetcher);
    job->log_dirty = true;
    pthread_mutex_unlock(&job->log_mutex);

    status = ncm_lyrics_fetcher_fetch(fetcher,
                                      &job->result,
                                      artist->data,
                                      artist->len,
                                      title->data,
                                      title->len);
    if (status < 0) {
        lyrics_job_append_fetch_error(job, &job->result);
        return status;
    }
    if (!job->result.success) {
        lyrics_job_append_fetch_error(job, &job->result);
        return 0;
    }
    return 1;
}

static int32
lyrics_job_run(void *user, NcmError *ncm_error) {
    StrBuilder artist = {0};
    StrBuilder title = {0};
    int32 status;
    LyricsJob *job = user;

    if (!lyrics_song_artist_title(&job->song, &artist, &title)) {
        sb_free(&title);
        sb_free(&artist);
        return ncm_error_set_status(ncm_error, -NCM_ERROR_NOT_FOUND,
                                    STRLIT("missing song metadata"));
    }
    if (artist.len <= 0) {
        for (int32 i = 0; i < title.len; i += 1) {
            if ((title.data[i] == '-') || (title.data[i] == '_')) {
                title.data[i] = ' ';
            }
        }
    }

    if (job->fetcher) {
        status = lyrics_job_fetch_one(job, job->fetcher, &artist, &title);
    } else {
        int32 fetch_status;

        status = 0;
        for (int32 i = 0; i < Config.lyrics_fetchers.fetchers.len; i += 1) {
            fetch_status = lyrics_job_fetch_one(
                job, &Config.lyrics_fetchers.fetchers.items[i],
                &artist, &title);
            if (fetch_status > 0) {
                status = fetch_status;
                break;
            }
        }
    }
    sb_free(&title);
    sb_free(&artist);
    if (status < 0) {
        ncm_error_set_status(ncm_error, status, STRLIT("lyrics fetch failed"));
        return status;
    }
    if ((status == 0) || !job->result.success) {
        return ncm_error_set_status(ncm_error, -NCM_ERROR_NOT_FOUND,
                                    STRLIT("lyrics not found"));
    }
    return ncm_error_ok(ncm_error);
}

static void
lyrics_job_complete(int32 status, NcmError *ncm_error, void *user) {
    LyricsJob *job = user;
    LyricsScreen *screen = job->screen;

    (void)status;
    (void)ncm_error;

    if (!job->background) {
        if (screen->foreground_job == job) {
            screen->foreground_job = NULL;
        }
        if (!lyrics_job_is_current(job)) {
            return;
        }

        ncm_lyrics_result_clear(&screen->result);
        ncm_lyrics_result_set(&screen->result,
                              job->result.success,
                              job->result.text,
                              job->result.text_len);
        if (job->result.success) {
            NcmError save_error = {0};

            lyrics_screen_clear_lyrics_state(
                screen, LYRICS_MODE_PLAIN);
            nc_buffer_append_data(&screen->display,
                                 job->result.text,
                                 job->result.text_len);
            sb_copy(&screen->filename, &job->filename);
            if (lyrics_screen_save_file(screen,
                                        job->filename.data,
                                        job->filename.len,
                                        job->result.text,
                                        job->result.text_len,
                                        &save_error) < 0) {
                StrBuilder output = {0};
                char *message = "unknown error";

                if (save_error.code != 0) {
                    message = strerror(save_error.code);
                }
                SB_APPEND(&output, "Couldn't save lyrics as \"");
                SB_APPEND(&output, job->filename.data, job->filename.len);
                SB_APPEND(&output, "\": ");
                SB_APPEND(&output, message, optional_strlen32(message));
                ncm_statusbar_print(Config.message_delay_time,
                                    output.data, output.len);
                sb_free(&output);
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

            lyrics_screen_save_file(screen,
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
    ncm_song_destroy(&job->song);
    sb_free(&job->filename);
    ncm_lyrics_result_destroy(&job->result);
    nc_buffer_destroy(&job->log);
    pthread_mutex_destroy(&job->log_mutex);
    free2(job, SIZEOF(*job));

    return;
}

static void
lyrics_append_fetching(NcBuffer *buffer, NcmLyricsFetcherDef *fetcher) {
    char *name;
    int32 name_len;
    int32 fetcher_position;

    name = ncm_lyrics_fetcher_name(fetcher);
    name_len = ncm_lyrics_fetcher_name_len(fetcher);

    nc_buffer_append_cstring(buffer, "Fetching lyrics from ");
    fetcher_position = buffer->len;
    nc_buffer_add_format(buffer, fetcher_position, NC_FORMAT_BOLD,
                         LYRICS_FETCH_PROPERTY_ID);
    nc_buffer_append_data(buffer, name, name_len);
    nc_buffer_add_format(buffer, buffer->len, NC_FORMAT_NO_BOLD,
                         LYRICS_FETCH_PROPERTY_ID);
    nc_buffer_append_cstring(buffer, "... ");

    return;
}

static void
lyrics_job_append_fetch_error(LyricsJob *job,
                              NcmLyricsResult *result) {
    NcColor red = nc_color_make(COLOR_RED, NC_COLOR_CURRENT, false, false);

    pthread_mutex_lock(&job->log_mutex);
    nc_buffer_add_color(&job->log, job->log.len, red,
                        LYRICS_FETCH_PROPERTY_ID);
    nc_buffer_append_data(&job->log, result->text, result->text_len);
    nc_buffer_add_color(&job->log, job->log.len, nc_color_end(),
                        LYRICS_FETCH_PROPERTY_ID);
    nc_buffer_append_char(&job->log, '\n');
    job->log_dirty = true;
    pthread_mutex_unlock(&job->log_mutex);

    return;
}

static bool
lyrics_job_is_current(LyricsJob *job) {
    LyricsScreen *screen;

    screen = job->screen;
    if (!screen->has_song) {
        return false;
    }
    if (!ncm_song_is_equal(&screen->song, &job->song)) {
        return false;
    }
    return STREQUAL(screen->filename.data, screen->filename.len,
                    job->filename.data, job->filename.len);
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

static int32
lyrics_start_next_background(LyricsScreen *screen,
                             NcmError *ncm_error) {
    LyricsQueuedSong *queued;
    LyricsJob *job;
    StrBuilder filename = {0};
    int32 status;
    bool win32_filename;
    bool found_job;

    if (ncm_job_queue_pending_count(&screen->jobs) > 0) {
        ncm_error_clear(ncm_error);
        return 0;
    }
    if (ncm_job_queue_completed_count(&screen->jobs) > 0) {
        ncm_error_clear(ncm_error);
        return 0;
    }

    found_job = false;
    queued = NULL;
    win32_filename = Config.generate_win32_compatible_filenames;
    while (!found_job) {
        if (screen->queued_songs_len <= 0) {
            queued = NULL;
        } else {
            queued = malloc2(SIZEOF(*queued));
            *queued = (LyricsQueuedSong){0};
            lyrics_queued_song_move(queued, &screen->queued_songs[0]);
            for (int32 i = 1; i < screen->queued_songs_len; i += 1) {
                lyrics_queued_song_move(&screen->queued_songs[i - 1],
                                        &screen->queued_songs[i]);
            }
            screen->queued_songs_len -= 1;
        }
        if (queued == NULL) {
            sb_free(&filename);
            ncm_error_clear(ncm_error);
            return 0;
        }

        if (ncm_song_is_stream(&queued->song)) {
            lyrics_queued_song_destroy(queued);
            free2(queued, SIZEOF(*queued));
            queued = NULL;
            continue;
        }

        status = lyrics_preferred_filename_from_song(
            &filename,
            &queued->song,
            Config.mpd_music_dir,
            Config.mpd_music_dir_len,
            Config.lyrics_directory,
            Config.lyrics_directory_len,
            Config.store_lyrics_in_song_dir,
            win32_filename);
        if (status < 0) {
            lyrics_queued_song_destroy(queued);
            free2(queued, SIZEOF(*queued));
            queued = NULL;
            continue;
        }

        if (ncm_fs_path_is_existing(filename.data, filename.len)) {
            lyrics_queued_song_destroy(queued);
            free2(queued, SIZEOF(*queued));
            queued = NULL;
            continue;
        }

        found_job = true;
    }

    status = ncm_job_queue_start(&screen->jobs, ncm_error);
    if (status < 0) {
        sb_free(&filename);
        lyrics_queued_song_destroy(queued);
        free2(queued, SIZEOF(*queued));
        return status;
    }

    if (queued->notify) {
        StrBuilder formatted = ncm_format_render_string(
            &Config.song_status_format, &queued->song);

        sb_clear(&screen->consumer_message);
        SB_APPEND(&screen->consumer_message, "Fetching lyrics for \"");
        SB_APPEND(&screen->consumer_message, formatted.data, formatted.len);
        SB_APPEND(&screen->consumer_message, "\"...");
        sb_free(&formatted);
    }
    job = lyrics_job_create(screen,
                            &queued->song,
                            screen->fetcher,
                            queued->notify,
                            true);
    status = ncm_job_queue_push(&screen->jobs,
                                (NcmJob){
                                    .run = lyrics_job_run,
                                    .complete = lyrics_job_complete,
                                    .destroy = lyrics_job_destroy,
                                    .user = job,
                                },
                                ncm_error);
    if (status < 0) {
        lyrics_job_destroy(job);
        sb_free(&filename);
        lyrics_queued_song_destroy(queued);
        free2(queued, SIZEOF(*queued));
        return status;
    }

    sb_free(&filename);
    lyrics_queued_song_destroy(queued);
    free2(queued, SIZEOF(*queued));
    ncm_error_clear(ncm_error);
    return 0;
}

#endif /* NCMPCPP_NC_LYRICS_C */
