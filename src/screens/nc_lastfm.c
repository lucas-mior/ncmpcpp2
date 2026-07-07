#include "screens/nc_lastfm.h"

#include <errno.h>
#include <string.h>

#include "c/ncm_base.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "settings.h"
#include "screens/screen_switcher.h"
#include "statusbar.h"
#include "title.h"
#include "ui_state.h"

#define NATIVE_LASTFM_DEFAULT_TITLE "Last.fm"
#define NATIVE_LASTFM_FETCHING "Fetching information..."

typedef struct NativeLastfmJob {
    NativeLastfmScreen *screen;
    NcmLastfmService service;
    NcmLastfmResult result;
} NativeLastfmJob;

static NcWindow *lastfm_active_window_callback(NcScreen *screen);
static void lastfm_refresh_callback(NcScreen *screen);
static void lastfm_refresh_window_callback(NcScreen *screen);
static void lastfm_scroll_callback(NcScreen *screen, enum NcScroll where);
static void lastfm_switch_to_callback(NcScreen *screen);
static void lastfm_resize_callback(NcScreen *screen);
static int32 lastfm_window_timeout_callback(NcScreen *screen);
static char *lastfm_title_callback(NcScreen *screen);
static void lastfm_update_callback(NcScreen *screen);
static void lastfm_mouse_button_pressed_callback(NcScreen *screen,
                                                 MEVENT event);
static bool lastfm_is_lockable_callback(NcScreen *screen);
static bool lastfm_is_mergable_callback(NcScreen *screen);
static void lastfm_destroy_callback(NcScreen *screen);
static NcScreenCallbacks native_lastfm_callbacks(void);
static NativeLastfmScreen *lastfm_from_screen(NcScreen *screen);
static bool native_lastfm_set_title(NativeLastfmScreen *screen,
                                    char *title, int32 title_len);
static NativeLastfmJob *native_lastfm_job_create(
    NativeLastfmScreen *screen, NcmLastfmService *service);
static bool native_lastfm_job_run(void *user, NcmError *error);
static void native_lastfm_job_complete(bool success, NcmError *error,
                                       void *user);
static void native_lastfm_job_destroy(void *user);
static void native_lastfm_copy_result(NativeLastfmScreen *screen,
                                      NcmLastfmResult *result);
static void native_lastfm_render_result(NativeLastfmScreen *screen);

void
nc_lastfm_screen_init(NcLastfmScreen *screen,
                      NcScreenCallbacks callbacks, void *user,
                      int64 start_x, int64 width,
                      int64 main_start_y, int64 main_height) {
    nc_scrollpad_screen_init(&screen->scrollpad_screen,
                             callbacks,
                             user,
                             NC_SCREEN_TYPE_LASTFM,
                             0,
                             0,
                             0,
                             0);
    nc_lastfm_screen_set_geometry(screen,
                                  start_x,
                                  width,
                                  main_start_y,
                                  main_height);
    return;
}

void
nc_lastfm_screen_set_geometry(NcLastfmScreen *screen,
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
nc_lastfm_screen_base(NcLastfmScreen *screen) {
    return nc_scrollpad_screen_base(&screen->scrollpad_screen);
}

int64
nc_lastfm_screen_start_x(NcLastfmScreen *screen) {
    return nc_scrollpad_screen_start_x(&screen->scrollpad_screen);
}

int64
nc_lastfm_screen_start_y(NcLastfmScreen *screen) {
    return nc_scrollpad_screen_start_y(&screen->scrollpad_screen);
}

int64
nc_lastfm_screen_width(NcLastfmScreen *screen) {
    return nc_scrollpad_screen_width(&screen->scrollpad_screen);
}

int64
nc_lastfm_screen_height(NcLastfmScreen *screen) {
    return nc_scrollpad_screen_height(&screen->scrollpad_screen);
}

static NcScreenCallbacks
native_lastfm_callbacks(void) {
    NcScreenCallbacks callbacks = {0};

    callbacks.active_window = lastfm_active_window_callback;
    callbacks.refresh = lastfm_refresh_callback;
    callbacks.refresh_window = lastfm_refresh_window_callback;
    callbacks.scroll = lastfm_scroll_callback;
    callbacks.switch_to = lastfm_switch_to_callback;
    callbacks.resize = lastfm_resize_callback;
    callbacks.window_timeout = lastfm_window_timeout_callback;
    callbacks.title = lastfm_title_callback;
    callbacks.update = lastfm_update_callback;
    callbacks.mouse_button_pressed = lastfm_mouse_button_pressed_callback;
    callbacks.is_lockable = lastfm_is_lockable_callback;
    callbacks.is_mergable = lastfm_is_mergable_callback;
    callbacks.destroy = lastfm_destroy_callback;
    return callbacks;
}

void
native_lastfm_screen_init(NativeLastfmScreen *screen,
                          int64 start_x, int64 width,
                          int64 main_start_y, int64 main_height,
                          NcColor color, NcBorder border,
                          int32 lines_scrolled) {
    nc_lastfm_screen_init(&screen->screen,
                          native_lastfm_callbacks(),
                          screen,
                          start_x,
                          width,
                          main_start_y,
                          main_height);
    nc_window_init(&screen->window,
                   nc_lastfm_screen_start_x(&screen->screen),
                   nc_lastfm_screen_start_y(&screen->screen),
                   nc_lastfm_screen_width(&screen->screen),
                   nc_lastfm_screen_height(&screen->screen),
                   STRLIT_ARGS(""),
                   color,
                   border);
    nc_scrollpad_init(&screen->scrollpad,
                      nc_lastfm_screen_height(&screen->screen));
    nc_buffer_init(&screen->buffer);
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
    (void)native_lastfm_set_title(screen,
                                  STRLIT_ARGS(NATIVE_LASTFM_DEFAULT_TITLE));
    return;
}

void
native_lastfm_screen_destroy(NativeLastfmScreen *screen) {
    if (!screen->initialized) {
        return;
    }

    ncm_job_queue_destroy(&screen->jobs);
    ncm_lastfm_service_destroy(&screen->service);
    ncm_lastfm_result_destroy(&screen->result);
    if (screen->title != NULL) {
        ncm_free(screen->title, screen->title_cap);
    }
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

NcLastfmScreen *
native_lastfm_screen_typed(NativeLastfmScreen *screen) {
    return &screen->screen;
}

NcScreen *
native_lastfm_screen_base(NativeLastfmScreen *screen) {
    return nc_lastfm_screen_base(&screen->screen);
}

NcWindow *
native_lastfm_screen_window(NativeLastfmScreen *screen) {
    return &screen->window;
}

void
native_lastfm_screen_set_geometry(NativeLastfmScreen *screen,
                                  int64 start_x, int64 width,
                                  int64 main_start_y, int64 main_height) {
    nc_lastfm_screen_set_geometry(&screen->screen,
                                  start_x,
                                  width,
                                  main_start_y,
                                  main_height);
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
native_lastfm_screen_queue_artist_info(NativeLastfmScreen *screen,
                                       char *artist, int32 artist_len,
                                       char *lang, int32 lang_len,
                                       NcmError *error) {
    NativeLastfmJob *job;
    NcmLastfmService candidate;

    if ((screen == NULL) || (artist == NULL) || (artist_len <= 0)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing artist"));
        return false;
    }

    ncm_lastfm_service_init(&candidate);
    if (!ncm_lastfm_artist_info_init(&candidate, artist, artist_len,
                                     lang, lang_len)) {
        ncm_lastfm_service_destroy(&candidate);
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("invalid Last.fm service"));
        return false;
    }
    if (screen->has_service
        && ncm_lastfm_service_equal(&screen->service, &candidate)) {
        ncm_lastfm_service_destroy(&candidate);
        ncm_error_clear(error);
        return true;
    }

    if (!ncm_job_queue_start(&screen->jobs, error)) {
        ncm_lastfm_service_destroy(&candidate);
        return false;
    }

    job = native_lastfm_job_create(screen, &candidate);
    ncm_lastfm_service_destroy(&candidate);
    if (job == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("failed to create job"));
        return false;
    }

    if (!ncm_job_queue_push(&screen->jobs,
                            (NcmJob){
                                .run = native_lastfm_job_run,
                                .complete = native_lastfm_job_complete,
                                .destroy = native_lastfm_job_destroy,
                                .user = job,
                            },
                            error)) {
        native_lastfm_job_destroy(job);
        return false;
    }

    ncm_lastfm_service_destroy(&screen->service);
    ncm_lastfm_service_init(&screen->service);
    (void)ncm_lastfm_artist_info_init(&screen->service,
                                      artist,
                                      artist_len,
                                      lang,
                                      lang_len);
    screen->has_service = true;
    (void)native_lastfm_set_title(screen,
                                  ncm_lastfm_service_name(&screen->service),
                                  (int32)strlen(
                                      ncm_lastfm_service_name(
                                          &screen->service)));
    nc_buffer_clear(&screen->buffer);
    nc_buffer_append_cstring(&screen->buffer,
                             (char *)NATIVE_LASTFM_FETCHING);
    screen->refresh_window = true;
    ncm_error_clear(error);
    return true;
}

int32
native_lastfm_screen_dispatch_jobs(NativeLastfmScreen *screen) {
    return ncm_job_queue_dispatch_completed(&screen->jobs);
}

void
native_lastfm_screen_update(NativeLastfmScreen *screen) {
    native_lastfm_screen_dispatch_jobs(screen);
    if (native_lastfm_screen_take_refresh_request(screen)) {
        nc_scrollpad_flush(&screen->scrollpad,
                           &screen->window,
                           &screen->buffer);
        nc_scrollpad_refresh(&screen->scrollpad, &screen->window);
    }
    return;
}

bool
native_lastfm_screen_has_service(NativeLastfmScreen *screen) {
    return screen->has_service;
}

NcmLastfmResult *
native_lastfm_screen_result(NativeLastfmScreen *screen) {
    return &screen->result;
}

char *
native_lastfm_screen_title(NativeLastfmScreen *screen) {
    if (screen->title == NULL) {
        return (char *)NATIVE_LASTFM_DEFAULT_TITLE;
    }
    return screen->title;
}

int32
native_lastfm_screen_title_len(NativeLastfmScreen *screen) {
    if (screen->title == NULL) {
        return STRLIT_LEN(NATIVE_LASTFM_DEFAULT_TITLE);
    }
    return screen->title_len;
}

bool
native_lastfm_screen_take_refresh_request(NativeLastfmScreen *screen) {
    bool result;

    result = screen->refresh_window;
    screen->refresh_window = false;
    return result;
}

int32
native_lastfm_screen_pending_jobs(NativeLastfmScreen *screen) {
    return ncm_job_queue_pending_count(&screen->jobs);
}

int32
native_lastfm_screen_completed_jobs(NativeLastfmScreen *screen) {
    return ncm_job_queue_completed_count(&screen->jobs);
}

static NativeLastfmScreen *
lastfm_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcWindow *
lastfm_active_window_callback(NcScreen *screen) {
    return native_lastfm_screen_window(lastfm_from_screen(screen));
}

static void
lastfm_refresh_callback(NcScreen *screen) {
    nc_window_display(native_lastfm_screen_window(lastfm_from_screen(screen)));
    return;
}

static void
lastfm_refresh_window_callback(NcScreen *screen) {
    nc_window_display(native_lastfm_screen_window(lastfm_from_screen(screen)));
    return;
}

static void
lastfm_scroll_callback(NcScreen *screen, enum NcScroll where) {
    NativeLastfmScreen *lastfm;

    lastfm = lastfm_from_screen(screen);
    nc_scrollpad_scroll(&lastfm->scrollpad, &lastfm->window, where);
    return;
}

static void
lastfm_switch_to_callback(NcScreen *screen) {
    ncm_title_draw_header(nc_screen_title(screen),
                          (int32)strlen(nc_screen_title(screen)));
    return;
}

static void
lastfm_resize_callback(NcScreen *screen) {
    NativeLastfmScreen *lastfm;
    int64 x;
    int64 width;

    lastfm = lastfm_from_screen(screen);
    nc_screen_switcher_get_resize_params(screen, &x, &width, true);
    native_lastfm_screen_set_geometry(lastfm,
                                      x,
                                      width,
                                      ui_state_main_start_y(),
                                      ui_state_main_height());
    nc_screen_clear_resize_request(screen);
    return;
}

static int32
lastfm_window_timeout_callback(NcScreen *screen) {
    (void)screen;
    return 500;
}

static char *
lastfm_title_callback(NcScreen *screen) {
    return native_lastfm_screen_title(lastfm_from_screen(screen));
}

static void
lastfm_update_callback(NcScreen *screen) {
    native_lastfm_screen_update(lastfm_from_screen(screen));
    return;
}

static void
lastfm_mouse_button_pressed_callback(NcScreen *screen, MEVENT event) {
    (void)screen;
    (void)event;
    return;
}

static bool
lastfm_is_lockable_callback(NcScreen *screen) {
    (void)screen;
    return true;
}

static bool
lastfm_is_mergable_callback(NcScreen *screen) {
    (void)screen;
    return true;
}

static void
lastfm_destroy_callback(NcScreen *screen) {
    native_lastfm_screen_destroy(lastfm_from_screen(screen));
    return;
}

static bool
native_lastfm_set_title(NativeLastfmScreen *screen,
                        char *title, int32 title_len) {
    int32 cap;

    if (title == NULL) {
        title = (char *)NATIVE_LASTFM_DEFAULT_TITLE;
        title_len = STRLIT_LEN(NATIVE_LASTFM_DEFAULT_TITLE);
    }
    cap = title_len + 1;
    if (cap > screen->title_cap) {
        screen->title = ncm_realloc_array(screen->title,
                                          screen->title_cap,
                                          cap,
                                          SIZEOF(*screen->title));
        screen->title_cap = cap;
    }
    ncm_memcpy(screen->title, title, title_len);
    screen->title[title_len] = '\0';
    screen->title_len = title_len;
    return true;
}

static NativeLastfmJob *
native_lastfm_job_create(NativeLastfmScreen *screen,
                         NcmLastfmService *service) {
    NativeLastfmJob *job;

    job = ncm_malloc(SIZEOF(*job));
    job->screen = screen;
    ncm_lastfm_service_init(&job->service);
    ncm_lastfm_result_init(&job->result);
    (void)ncm_lastfm_artist_info_init(&job->service,
                                      service->artist,
                                      service->artist_len,
                                      service->lang,
                                      service->lang_len);
    return job;
}

static bool
native_lastfm_job_run(void *user, NcmError *error) {
    NativeLastfmJob *job;

    job = user;
    (void)ncm_lastfm_service_fetch(&job->service, &job->result);
    if (!job->result.success) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("Last.fm fetch failed"));
    }
    return job->result.success;
}

static void
native_lastfm_job_complete(bool success, NcmError *error, void *user) {
    NativeLastfmJob *job;

    (void)success;
    (void)error;
    job = user;
    native_lastfm_copy_result(job->screen, &job->result);
    native_lastfm_render_result(job->screen);
    return;
}

static void
native_lastfm_job_destroy(void *user) {
    NativeLastfmJob *job;

    job = user;
    if (job == NULL) {
        return;
    }
    ncm_lastfm_service_destroy(&job->service);
    ncm_lastfm_result_destroy(&job->result);
    ncm_free(job, SIZEOF(*job));
    return;
}

static void
native_lastfm_copy_result(NativeLastfmScreen *screen,
                          NcmLastfmResult *result) {
    ncm_lastfm_result_clear(&screen->result);
    (void)ncm_lastfm_result_set(&screen->result,
                                result->success,
                                result->text,
                                result->text_len);
    return;
}

static void
native_lastfm_render_result(NativeLastfmScreen *screen) {
    nc_buffer_clear(&screen->buffer);
    if (screen->result.success) {
        nc_buffer_append_data(&screen->buffer,
                              screen->result.text,
                              screen->result.text_len);
    } else {
        nc_buffer_append_cstring(&screen->buffer,
                                 (char *)"Last.fm request failed");
        if (screen->result.text_len > 0) {
            nc_buffer_append_cstring(&screen->buffer, (char *)": ");
            nc_buffer_append_data(&screen->buffer,
                                  screen->result.text,
                                  screen->result.text_len);
        }
    }
    screen->refresh_window = true;
    return;
}
