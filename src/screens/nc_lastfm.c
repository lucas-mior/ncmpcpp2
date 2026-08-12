#if !defined(NCMPCPP_NC_LASTFM_C)
#define NCMPCPP_NC_LASTFM_C

#include "cbase.h"

#include "c/ncm_c.h"
#include "screens/nc_screens.h"
#include "settings.h"
#include "statusbar.h"
#include "title.h"
#include "ui_state.h"

#define LASTFM_DEFAULT_TITLE "Last.fm"
#define LASTFM_FETCHING "Fetching information..."
#define LASTFM_PROPERTY_ID ((int64)-2)
#define LASTFM_DEFAULT_PROPERTY_ID ((int64)-1)

typedef struct LastfmJob {
    LastfmScreen *screen;
    NcmLastfmService service;
    NcmLastfmResult result;
} LastfmJob;

typedef struct LastfmFindState {
    NcBuffer *buffer;
} LastfmFindState;

static void lastfm_switch_to_callback(NcScreen *screen);
static void lastfm_resize_callback(NcScreen *screen);
static char *lastfm_title_callback(NcScreen *screen);
static void lastfm_update_callback(NcScreen *screen);
static void lastfm_mouse_button_pressed_callback(NcScreen *screen,
                                                 MEVENT event);
static bool lastfm_set_title(LastfmScreen *screen, char *title,
                             int32 title_len);
static LastfmJob *lastfm_job_create(LastfmScreen *screen,
                                    NcmLastfmService *service);
static bool lastfm_job_run(void *user, NcmError *ncm_error);
static void lastfm_job_complete(bool success, NcmError *ncm_error, void *user);
static void lastfm_job_destroy(void *user);
static void lastfm_copy_result(LastfmScreen *screen, NcmLastfmResult *result);
static void lastfm_render_result(LastfmScreen *screen);
static void lastfm_render_failure(LastfmScreen *screen);
static void lastfm_apply_literal_format(NcBuffer *buffer,
                                        char *needle, int32 needle_len,
                                        enum NcFormat start_format,
                                        enum NcFormat end_format);
static void lastfm_apply_literal_color2(NcBuffer *buffer,
                                        char *needle, int32 needle_len);
static bool lastfm_find_match_callback(int32 start, int32 len, void *user);
static void lastfm_mouse_scroll(LastfmScreen *screen, enum NcScroll where);
static void lastfm_display(LastfmScreen *screen);
static void lastfm_flush(LastfmScreen *screen);

#define NC_SCREEN_IMPL_TYPE LastfmScreen
#define NC_SCREEN_IMPL_PREFIX lastfm
#define NC_SCREEN_IMPL_PUBLIC_PREFIX lastfm_screen
#define NC_SCREEN_IMPL_BASE_FIELD screen
#define NC_SCREEN_IMPL_SCROLLPAD_BASE screen.scrollpad_screen
#define NC_SCREEN_IMPL_NO_GEOMETRY_ACCESSORS
#define NC_SCREEN_IMPL_WINDOW_FIELD window
#define NC_SCREEN_IMPL_SCROLLPAD_FIELD scrollpad
#define NC_SCREEN_IMPL_REFRESH_CALLBACK lastfm_display
#define NC_SCREEN_IMPL_SWITCH_TO_CALLBACK lastfm_switch_to_callback
#define NC_SCREEN_IMPL_RESIZE_CALLBACK lastfm_resize_callback
#define NC_SCREEN_IMPL_TITLE_CALLBACK lastfm_title_callback
#define NC_SCREEN_IMPL_UPDATE_CALLBACK lastfm_update_callback
#define NC_SCREEN_IMPL_MOUSE_CALLBACK lastfm_mouse_button_pressed_callback
#define NC_SCREEN_IMPL_DESTROY_TYPED_CALLBACK lastfm_screen_destroy
#define NC_SCREEN_IMPL_LOCKABLE true
#define NC_SCREEN_IMPL_MERGABLE true
#include "screens/nc_screen_impl_template.h"

void
nc_lastfm_screen_init(NcLastfmScreen *screen,
                      NcScreenOps callbacks, void *user,
                      int32 start_x, int32 width,
                      int32 main_start_y, int32 main_height) {
    nc_scrollpad_screen_init(&screen->scrollpad_screen,
                             callbacks,
                             user,
                             NC_SCREEN_TYPE_LASTFM,
                             0, 0, 0, 0);
    nc_lastfm_screen_set_geometry(screen, start_x, width,
                                  main_start_y, main_height);
    return;
}

void
nc_lastfm_screen_set_geometry(NcLastfmScreen *screen,
                              int32 start_x, int32 width,
                              int32 main_start_y, int32 main_height) {
    nc_scrollpad_screen_set_main_area(&screen->scrollpad_screen,
                                      start_x, width,
                                      main_start_y, main_height);
    return;
}

NcScreen *
nc_lastfm_screen_base(NcLastfmScreen *screen) {
    return nc_scrollpad_screen_base(&screen->scrollpad_screen);
}

int32
nc_lastfm_screen_start_x(NcLastfmScreen *screen) {
    return nc_scrollpad_screen_start_x(&screen->scrollpad_screen);
}

int32
nc_lastfm_screen_start_y(NcLastfmScreen *screen) {
    return nc_scrollpad_screen_start_y(&screen->scrollpad_screen);
}

int32
nc_lastfm_screen_width(NcLastfmScreen *screen) {
    return nc_scrollpad_screen_width(&screen->scrollpad_screen);
}

int32
nc_lastfm_screen_height(NcLastfmScreen *screen) {
    return nc_scrollpad_screen_height(&screen->scrollpad_screen);
}

void
lastfm_screen_init(LastfmScreen *screen,
                   int32 start_x, int32 width,
                   int32 main_start_y, int32 main_height,
                   NcColor color, NcBorder border,
                   int32 lines_scrolled) {
    nc_lastfm_screen_init(&screen->screen,
                          lastfm_ops,
                          screen,
                          start_x, width,
                          main_start_y, main_height);

    nc_window_init(&screen->window,
                   nc_lastfm_screen_start_x(&screen->screen),
                   nc_lastfm_screen_start_y(&screen->screen),
                   nc_lastfm_screen_width(&screen->screen),
                   nc_lastfm_screen_height(&screen->screen),
                   STRLIT(""), color, border);

    nc_scrollpad_init(&screen->scrollpad,
                      nc_lastfm_screen_height(&screen->screen));

    nc_buffer_init(&screen->buffer);
    sb_init(&screen->search_constraint);
    ncm_lastfm_service_init(&screen->service);
    ncm_lastfm_result_init(&screen->result);
    ncm_job_queue_init(&screen->jobs);

    screen->title = NULL;
    screen->title_len = 0;
    screen->title_cap = 0;
    screen->has_service = false;
    screen->refresh_window = false;
    screen->initialized = true;

    nc_window_set_timeout(&screen->window, lines_scrolled);
    (void)lastfm_set_title(screen, STRLIT(LASTFM_DEFAULT_TITLE));
    return;
}

void
lastfm_screen_destroy(LastfmScreen *screen) {
    if (!screen->initialized) {
        return;
    }

    ncm_job_queue_destroy(&screen->jobs);
    ncm_lastfm_service_destroy(&screen->service);
    ncm_lastfm_result_destroy(&screen->result);
    if (screen->title) {
        free2(screen->title, screen->title_cap);
    }
    sb_free(&screen->search_constraint);
    nc_buffer_destroy(&screen->buffer);
    nc_window_destroy(&screen->window);

    screen->title = NULL;
    screen->title_len = 0;
    screen->title_cap = 0;
    screen->has_service = false;
    screen->refresh_window = false;
    screen->initialized = false;

    return;
}

NcWindow *
lastfm_screen_window(LastfmScreen *screen) {
    return &screen->window;
}

void
lastfm_screen_set_geometry(LastfmScreen *screen,
                           int32 start_x, int32 width,
                           int32 main_start_y, int32 main_height) {
    nc_lastfm_screen_set_geometry(&screen->screen, start_x, width,
                                  main_start_y, main_height);
    nc_window_resize(&screen->window,
                     nc_lastfm_screen_width(&screen->screen),
                     nc_lastfm_screen_height(&screen->screen));
    nc_window_move_to(&screen->window,
                      nc_lastfm_screen_start_x(&screen->screen),
                      nc_lastfm_screen_start_y(&screen->screen));
    nc_scrollpad_resize(&screen->scrollpad,
                        &screen->window,
                        nc_lastfm_screen_width(&screen->screen),
                        nc_lastfm_screen_height(&screen->screen));
    return;
}

bool
lastfm_screen_queue_artist_info(LastfmScreen *screen,
                                char *artist, int32 artist_len,
                                char *lang, int32 lang_len,
                                NcmError *ncm_error) {
    LastfmJob *job;
    NcmLastfmService candidate;
    char *title;

    if ((screen == NULL) || (artist == NULL) || (artist_len <= 0)) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing artist"));
        return false;
    }

    ncm_lastfm_service_init(&candidate);
    if (!ncm_lastfm_artist_info_init(&candidate, artist, artist_len,
                                     lang, lang_len)) {
        ncm_lastfm_service_destroy(&candidate);
        ncm_error_set(ncm_error, EINVAL,
                      STRLIT("invalid Last.fm service"));
        return false;
    }
    if (screen->has_service
        && ncm_lastfm_service_equal(&screen->service, &candidate)) {
        ncm_lastfm_service_destroy(&candidate);
        ncm_error_clear(ncm_error);
        return true;
    }

    if (!ncm_job_queue_start(&screen->jobs, ncm_error)) {
        ncm_lastfm_service_destroy(&candidate);
        return false;
    }

    job = lastfm_job_create(screen, &candidate);
    ncm_lastfm_service_destroy(&candidate);
    if (job == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("failed to create job"));
        return false;
    }

    if (!ncm_job_queue_push(&screen->jobs,
                            (NcmJob){
                                .run = lastfm_job_run,
                                .complete = lastfm_job_complete,
                                .destroy = lastfm_job_destroy,
                                .user = job,
                            },
                            ncm_error)) {
        lastfm_job_destroy(job);
        return false;
    }

    ncm_lastfm_service_destroy(&screen->service);
    ncm_lastfm_service_init(&screen->service);
    (void)ncm_lastfm_artist_info_init(&screen->service,
                                      artist, artist_len,
                                      lang, lang_len);
    screen->has_service = true;
    title = ncm_lastfm_service_name(&screen->service);
    (void)lastfm_set_title(screen, title, strlen32(title));
    nc_buffer_clear(&screen->buffer);
    nc_buffer_append_cstring(&screen->buffer,
                             (char *)LASTFM_FETCHING);
    screen->refresh_window = true;
    ncm_error_clear(ncm_error);
    return true;
}

int32
lastfm_screen_dispatch_jobs(LastfmScreen *screen) {
    return ncm_job_queue_dispatch_completed(&screen->jobs);
}

void
lastfm_screen_update(LastfmScreen *screen) {
    lastfm_screen_dispatch_jobs(screen);
    if (lastfm_screen_take_refresh_request(screen)) {
        nc_scrollpad_flush(&screen->scrollpad,
                           &screen->window,
                           &screen->buffer);
        nc_scrollpad_refresh(&screen->scrollpad, &screen->window);
    }
    return;
}

char *
lastfm_screen_title(LastfmScreen *screen) {
    if (screen->title == NULL) {
        return (char *)LASTFM_DEFAULT_TITLE;
    }
    return screen->title;
}

bool
lastfm_screen_take_refresh_request(LastfmScreen *screen) {
    bool result;

    result = screen->refresh_window;
    screen->refresh_window = false;
    return result;
}

bool
lastfm_buffer_find(NcBuffer *buffer, char *pattern,
                   int32 pattern_len, NcmError *ncm_error) {
    LastfmFindState state;
    NcmRegex regex;
    char *data;
    bool result;

    if (buffer == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing Last.fm buffer"));
        return false;
    }

    nc_buffer_remove_properties(buffer, LASTFM_PROPERTY_ID);
    if ((pattern == NULL) || (pattern_len <= 0)) {
        ncm_error_clear(ncm_error);
        return true;
    }

    ncm_regex_init(&regex);
    if (!ncm_regex_compile(&regex,
                           pattern, pattern_len,
                           Config.regex_flags,
                           ncm_error)) {
        ncm_regex_destroy(&regex);
        return false;
    }

    state.buffer = buffer;
    data = nc_buffer_data(buffer);
    result = ncm_regex_for_each_match(&regex,
                                      data, buffer->len,
                                      lastfm_find_match_callback, &state);
    ncm_regex_destroy(&regex);
    return result;
}

bool
lastfm_screen_find(LastfmScreen *screen,
                   char *pattern, int32 pattern_len,
                   NcmError *ncm_error) {
    bool result;

    if (screen == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing Last.fm screen"));
        return false;
    }

    result = lastfm_buffer_find(&screen->buffer, pattern, pattern_len, ncm_error);
    if ((pattern == NULL) || (pattern_len <= 0)) {
        sb_clear(&screen->search_constraint);
    } else if (!ncm_error_is_set(ncm_error)) {
        (void)sb_set(&screen->search_constraint, pattern, pattern_len);
    }
    lastfm_flush(screen);
    return result;
}

static void
lastfm_switch_to_callback(NcScreen *screen) {
    char *title = nc_screen_title(screen);

    ncm_title_draw_header(title, strlen32(title));
    return;
}

static void
lastfm_resize_callback(NcScreen *screen) {
    int32 x;
    int32 width;
    LastfmScreen *lastfm = lastfm_from_screen(screen);

    nc_screen_switcher_get_resize_params(screen, &x, &width, true);
    lastfm_screen_set_geometry(lastfm, x, width,
                               ui_state_main_start_y(),
                               ui_state_main_height());
    nc_screen_clear_resize_request(screen);

    return;
}

static char *
lastfm_title_callback(NcScreen *screen) {
    return lastfm_screen_title(lastfm_from_screen(screen));
}

static void
lastfm_update_callback(NcScreen *screen) {
    lastfm_screen_update(lastfm_from_screen(screen));
    return;
}

static void
lastfm_mouse_button_pressed_callback(NcScreen *screen, MEVENT event) {
    LastfmScreen *lastfm = lastfm_from_screen(screen);

    if (event.bstate & BUTTON5_PRESSED) {
        lastfm_mouse_scroll(lastfm, NC_SCROLL_DOWN);
    } else if (event.bstate & BUTTON4_PRESSED) {
        lastfm_mouse_scroll(lastfm, NC_SCROLL_UP);
    }
    return;
}

static bool
lastfm_set_title(LastfmScreen *screen, char *title, int32 title_len) {
    int32 cap;

    if (title == NULL) {
        title = (char *)LASTFM_DEFAULT_TITLE;
        title_len = STRLIT_LEN(LASTFM_DEFAULT_TITLE);
    }
    cap = title_len + 1;
    if (cap > screen->title_cap) {
        screen->title = realloc2(screen->title,
                                 screen->title_cap,
                                 cap,
                                 SIZEOF(*screen->title));
        screen->title_cap = cap;
    }

    memcpy64(screen->title, title, title_len);
    screen->title[title_len] = '\0';
    screen->title_len = title_len;

    return true;
}

static LastfmJob *
lastfm_job_create(LastfmScreen *screen, NcmLastfmService *service) {
    LastfmJob *job = malloc2(SIZEOF(*job));
    job->screen = screen;

    ncm_lastfm_service_init(&job->service);
    ncm_lastfm_result_init(&job->result);
    (void)ncm_lastfm_artist_info_init(&job->service,
                                      service->artist, service->artist_len,
                                      service->lang, service->lang_len);

    return job;
}

static bool
lastfm_job_run(void *user, NcmError *ncm_error) {
    LastfmJob *job = user;

    (void)ncm_lastfm_service_fetch(&job->service, &job->result);
    if (!job->result.success) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("Last.fm fetch failed"));
    }
    return job->result.success;
}

static void
lastfm_job_complete(bool success, NcmError *ncm_error, void *user) {
    LastfmJob *job = user;
    LastfmScreen *screen;

    (void)success;
    (void)ncm_error;
    if (job == NULL) {
        return;
    }

    if (((screen = job->screen) == NULL)
        || !screen->has_service
        || !ncm_lastfm_service_equal(&job->service, &screen->service)) {
        return;
    }

    lastfm_copy_result(screen, &job->result);
    lastfm_render_result(screen);
    return;
}

static void
lastfm_job_destroy(void *user) {
    LastfmJob *job = user;

    if (job == NULL) {
        return;
    }
    ncm_lastfm_service_destroy(&job->service);
    ncm_lastfm_result_destroy(&job->result);
    free2(job, SIZEOF(*job));
    return;
}

static void
lastfm_copy_result(LastfmScreen *screen, NcmLastfmResult *result) {
    ncm_lastfm_result_clear(&screen->result);
    (void)ncm_lastfm_result_set(&screen->result,
                                result->success,
                                result->text,
                                result->text_len);
    return;
}

static void
lastfm_render_result(LastfmScreen *screen) {
    nc_buffer_clear(&screen->buffer);
    if (screen->result.success) {
        nc_buffer_append_data(&screen->buffer,
                              screen->result.text,
                              screen->result.text_len);
        if (ncm_lastfm_service_type(&screen->service)
            == NCM_LASTFM_SERVICE_ARTIST_INFO) {
            lastfm_apply_literal_format(
                &screen->buffer, STRLIT("\n\nSimilar artists:\n"),
                NC_FORMAT_BOLD, NC_FORMAT_NO_BOLD);
            lastfm_apply_literal_format(
                &screen->buffer, STRLIT("\n\nSimilar tags:\n"),
                NC_FORMAT_BOLD, NC_FORMAT_NO_BOLD);
            lastfm_apply_literal_color2(&screen->buffer,
                                        STRLIT("\n * "));
        }
    } else {
        lastfm_render_failure(screen);
    }
    screen->refresh_window = true;
    return;
}

static void
lastfm_render_failure(LastfmScreen *screen) {
    NcColor red;

    red = nc_color_make(COLOR_RED, NC_COLOR_CURRENT, false, false);
    nc_buffer_append_char(&screen->buffer, ' ');
    nc_buffer_add_color(&screen->buffer, nc_buffer_len(&screen->buffer), red,
                        LASTFM_DEFAULT_PROPERTY_ID);
    nc_buffer_append_data(&screen->buffer,
                          screen->result.text,
                          screen->result.text_len);
    nc_buffer_add_color(&screen->buffer, nc_buffer_len(&screen->buffer),
                        nc_color_end(),
                        LASTFM_DEFAULT_PROPERTY_ID);
    return;
}

static void
lastfm_apply_literal_format(NcBuffer *buffer,
                            char *needle, int32 needle_len,
                            enum NcFormat start_format,
                            enum NcFormat end_format) {
    char *data;
    int32 len;

    data = buffer->data;
    len = buffer->len;
    for (int32 i = 0; i + needle_len <= len; i += 1) {
        if (BEGINS_WITH(data + i, len - i, needle, needle_len)) {
            nc_buffer_add_format(buffer, i, start_format,
                                 LASTFM_PROPERTY_ID);
            nc_buffer_add_format(buffer, i + needle_len, end_format,
                                 LASTFM_PROPERTY_ID);
        }
    }
    return;
}

static void
lastfm_apply_literal_color2(NcBuffer *buffer, char *needle, int32 needle_len) {
    char *data;
    int32 len;

    data = buffer->data;
    len = buffer->len;
    for (int32 i = 0; i + needle_len <= len; i += 1) {
        if (BEGINS_WITH(data + i, len - i, needle, needle_len)) {
            nc_buffer_add_formatted_color(buffer, i, &Config.color2,
                                          LASTFM_PROPERTY_ID);
            nc_buffer_add_formatted_color_end(buffer, i + needle_len,
                                              &Config.color2,
                                              LASTFM_PROPERTY_ID);
        }
    }
    return;
}

static bool
lastfm_find_match_callback(int32 start, int32 len, void *user) {
    LastfmFindState *state = user;

    if (len <= 0) {
        return true;
    }

    nc_buffer_add_format(state->buffer, start, NC_FORMAT_REVERSE,
                         LASTFM_PROPERTY_ID);
    nc_buffer_add_format(state->buffer, start + len, NC_FORMAT_NO_REVERSE,
                         LASTFM_PROPERTY_ID);
    return true;
}

static void
lastfm_mouse_scroll(LastfmScreen *screen, enum NcScroll where) {
    for (int32 i = 0; i < Config.lines_scrolled; i += 1) {
        nc_scrollpad_scroll(&screen->scrollpad, &screen->window, where);
    }
    return;
}

static void
lastfm_display(LastfmScreen *screen) {
    nc_window_refresh_border(&screen->window);
    nc_scrollpad_refresh(&screen->scrollpad, &screen->window);
    return;
}

static void
lastfm_flush(LastfmScreen *screen) {
    nc_scrollpad_flush(&screen->scrollpad, &screen->window, &screen->buffer);
    lastfm_display(screen);
    return;
}

#endif /* NCMPCPP_NC_LASTFM_C */
