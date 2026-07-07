#include "screens/nc_visualizer.h"

#include <math.h>
#include <string.h>

#include "app_controller.h"
#include "c/ncm_base.h"
#include "cbase/base_macros.h"
#include "screens/screen_switcher.h"
#include "title.h"
#include "ui_state.h"

#define NATIVE_VISUALIZER_TITLE "Visualizer"
#define NATIVE_VISUALIZER_DEFAULT_RATE 44100
#define NATIVE_VISUALIZER_DEFAULT_CAP 32768
#define NATIVE_VISUALIZER_MIN_SAMPLE (-32768)
#define NATIVE_VISUALIZER_MAX_SAMPLE 32767

static NcWindow *visualizer_active_window_callback(NcScreen *screen);
static void visualizer_refresh_callback(NcScreen *screen);
static void visualizer_refresh_window_callback(NcScreen *screen);
static void visualizer_scroll_callback(NcScreen *screen,
                                       enum NcScroll where);
static void visualizer_switch_to_callback(NcScreen *screen);
static void visualizer_resize_callback(NcScreen *screen);
static int32 visualizer_window_timeout_callback(NcScreen *screen);
static char *visualizer_title_callback(NcScreen *screen);
static void visualizer_update_callback(NcScreen *screen);
static void visualizer_mouse_button_pressed_callback(NcScreen *screen,
                                                     MEVENT event);
static bool visualizer_is_lockable_callback(NcScreen *screen);
static bool visualizer_is_mergable_callback(NcScreen *screen);
static void visualizer_destroy_callback(NcScreen *screen);
static NcScreenCallbacks native_visualizer_callbacks(void);
static NativeVisualizerScreen *visualizer_from_screen(NcScreen *screen);
static int32 native_visualizer_next_type(int32 type);

static NcScreenCallbacks
native_visualizer_callbacks(void) {
    NcScreenCallbacks callbacks = {0};

    callbacks.active_window = visualizer_active_window_callback;
    callbacks.refresh = visualizer_refresh_callback;
    callbacks.refresh_window = visualizer_refresh_window_callback;
    callbacks.scroll = visualizer_scroll_callback;
    callbacks.switch_to = visualizer_switch_to_callback;
    callbacks.resize = visualizer_resize_callback;
    callbacks.window_timeout = visualizer_window_timeout_callback;
    callbacks.title = visualizer_title_callback;
    callbacks.update = visualizer_update_callback;
    callbacks.mouse_button_pressed = visualizer_mouse_button_pressed_callback;
    callbacks.is_lockable = visualizer_is_lockable_callback;
    callbacks.is_mergable = visualizer_is_mergable_callback;
    callbacks.destroy = visualizer_destroy_callback;
    return callbacks;
}

void
native_visualizer_screen_init(NativeVisualizerScreen *screen,
                              int64 start_x, int64 start_y,
                              int64 width, int64 height,
                              NcColor color, NcBorder border,
                              int32 fps, bool stereo) {
    nc_screen_init(&screen->screen,
                   native_visualizer_callbacks(),
                   screen,
                   NC_SCREEN_TYPE_VISUALIZER);
    nc_window_init(&screen->window,
                   start_x,
                   start_y,
                   width,
                   height,
                   STRLIT_ARGS(""),
                   color,
                   border);
    ncm_sample_buffer_init(&screen->buffered_samples);
    ncm_sample_buffer_init(&screen->rendered_samples);
    ncm_sample_buffer_init(&screen->left_channel);
    ncm_sample_buffer_init(&screen->right_channel);
    ncm_sample_buffer_resize(&screen->buffered_samples,
                             NATIVE_VISUALIZER_DEFAULT_CAP);
    ncm_sample_buffer_resize(&screen->rendered_samples,
                             NATIVE_VISUALIZER_DEFAULT_CAP);
    ncm_sample_buffer_resize(&screen->left_channel,
                             NATIVE_VISUALIZER_DEFAULT_CAP / 2);
    ncm_sample_buffer_resize(&screen->right_channel,
                             NATIVE_VISUALIZER_DEFAULT_CAP / 2);
    screen->auto_scale_multiplier = 1.0;
    screen->visualization_type = NATIVE_VISUALIZER_WAVE;
    screen->fps = fps;
    if (screen->fps <= 0) {
        screen->fps = 30;
    }
    screen->sample_rate = NATIVE_VISUALIZER_DEFAULT_RATE;
    screen->sample_consumption_rate = 0;
    screen->sample_consumption_rate_up_ctr = 0;
    screen->sample_consumption_rate_dn_ctr = 0;
    screen->stereo = stereo;
    screen->initialized = true;
    return;
}

void
native_visualizer_screen_destroy(NativeVisualizerScreen *screen) {
    if (!screen->initialized) {
        return;
    }

    ncm_sample_buffer_destroy(&screen->right_channel);
    ncm_sample_buffer_destroy(&screen->left_channel);
    ncm_sample_buffer_destroy(&screen->rendered_samples);
    ncm_sample_buffer_destroy(&screen->buffered_samples);
    nc_window_destroy(&screen->window);
    screen->initialized = false;
    return;
}

NcScreen *
native_visualizer_screen_base(NativeVisualizerScreen *screen) {
    return &screen->screen;
}

NcWindow *
native_visualizer_screen_window(NativeVisualizerScreen *screen) {
    return &screen->window;
}

void
native_visualizer_screen_set_geometry(NativeVisualizerScreen *screen,
                                      int64 start_x, int64 start_y,
                                      int64 width, int64 height) {
    nc_window_resize(&screen->window, width, height);
    nc_window_move_to(&screen->window, start_x, start_y);
    return;
}

void
native_visualizer_screen_clear(NativeVisualizerScreen *screen) {
    ncm_sample_buffer_clear(&screen->buffered_samples);
    ncm_sample_buffer_clear(&screen->rendered_samples);
    ncm_sample_buffer_clear(&screen->left_channel);
    ncm_sample_buffer_clear(&screen->right_channel);
    nc_window_clear(&screen->window);
    return;
}

void
native_visualizer_screen_reset_auto_scale_multiplier(
    NativeVisualizerScreen *screen) {
    screen->auto_scale_multiplier = 1.0;
    screen->sample_consumption_rate = 0;
    screen->sample_consumption_rate_up_ctr = 0;
    screen->sample_consumption_rate_dn_ctr = 0;
    return;
}

void
native_visualizer_screen_set_type(NativeVisualizerScreen *screen,
                                  enum NativeVisualizerType type) {
    if ((type < 0) || (type >= NATIVE_VISUALIZER_TYPE_LAST)) {
        type = NATIVE_VISUALIZER_WAVE;
    }
    screen->visualization_type = type;
    return;
}

void
native_visualizer_screen_toggle_type(NativeVisualizerScreen *screen) {
    screen->visualization_type = native_visualizer_next_type(
        screen->visualization_type);
    native_visualizer_screen_clear(screen);
    return;
}

enum NativeVisualizerType
native_visualizer_screen_type(NativeVisualizerScreen *screen) {
    return screen->visualization_type;
}

void
native_visualizer_screen_set_stereo(NativeVisualizerScreen *screen,
                                    bool stereo) {
    screen->stereo = stereo;
    return;
}

void
native_visualizer_screen_set_fps(NativeVisualizerScreen *screen,
                                 int32 fps) {
    if (fps <= 0) {
        fps = 30;
    }
    screen->fps = fps;
    return;
}

int32
native_visualizer_screen_requested_samples(NativeVisualizerScreen *screen) {
    double rate;
    int32 result;

    rate = (double)screen->sample_rate / (double)screen->fps;
    rate *= pow(1.1, (double)screen->sample_consumption_rate);
    result = (int32)rate;
    if (result < 1) {
        result = 1;
    }
    if (screen->stereo) {
        result *= 2;
    }
    return result;
}

bool
native_visualizer_screen_push_samples(NativeVisualizerScreen *screen,
                                      int16 *samples,
                                      int32 samples_len) {
    native_visualizer_screen_apply_auto_scale(screen, samples, samples_len);
    return ncm_sample_buffer_put(&screen->buffered_samples,
                                 samples,
                                 samples_len);
}

int32
native_visualizer_screen_take_render_samples(NativeVisualizerScreen *screen,
                                             int16 *dest, int32 dest_len) {
    int32 requested;
    int32 result;

    requested = native_visualizer_screen_requested_samples(screen);
    result = ncm_sample_buffer_get_clamped(&screen->buffered_samples,
                                           requested,
                                           dest,
                                           dest_len);
    if (ncm_sample_buffer_size(&screen->buffered_samples) > 0) {
        screen->sample_consumption_rate_up_ctr += 1;
        if (screen->sample_consumption_rate_up_ctr > 8) {
            screen->sample_consumption_rate_up_ctr = 0;
            screen->sample_consumption_rate += 1;
        }
    } else if (screen->sample_consumption_rate > 0) {
        screen->sample_consumption_rate_dn_ctr += 1;
        if (screen->sample_consumption_rate_dn_ctr > 4) {
            screen->sample_consumption_rate_dn_ctr = 0;
            screen->sample_consumption_rate -= 1;
        }
        screen->sample_consumption_rate_up_ctr = 0;
    }
    return result;
}

int32
native_visualizer_screen_split_stereo(NativeVisualizerScreen *screen,
                                      int16 *samples, int32 samples_len) {
    int32 pairs;

    ncm_sample_buffer_clear(&screen->left_channel);
    ncm_sample_buffer_clear(&screen->right_channel);
    pairs = samples_len / 2;
    if (pairs > ncm_sample_buffer_capacity(&screen->left_channel)) {
        ncm_sample_buffer_resize(&screen->left_channel, pairs);
    }
    if (pairs > ncm_sample_buffer_capacity(&screen->right_channel)) {
        ncm_sample_buffer_resize(&screen->right_channel, pairs);
    }
    for (int32 i = 0; i < pairs; i += 1) {
        screen->left_channel.data[screen->left_channel.len++] =
            samples[i*2];
        screen->right_channel.data[screen->right_channel.len++] =
            samples[i*2 + 1];
    }
    return pairs;
}

int32
native_visualizer_screen_left_len(NativeVisualizerScreen *screen) {
    return screen->left_channel.len;
}

int32
native_visualizer_screen_right_len(NativeVisualizerScreen *screen) {
    return screen->right_channel.len;
}

int16 *
native_visualizer_screen_left_data(NativeVisualizerScreen *screen) {
    return screen->left_channel.data;
}

int16 *
native_visualizer_screen_right_data(NativeVisualizerScreen *screen) {
    return screen->right_channel.data;
}

void
native_visualizer_screen_apply_auto_scale(NativeVisualizerScreen *screen,
                                          int16 *samples,
                                          int32 samples_len) {
    int64 scaled;

    if (screen->auto_scale_multiplier > 50.0) {
        return;
    }
    for (int32 i = 0; i < samples_len; i += 1) {
        scaled = (int64)((double)samples[i]*screen->auto_scale_multiplier);
        samples[i] = native_visualizer_clamp_sample(scaled);
    }
    return;
}

int16
native_visualizer_clamp_sample(int64 sample) {
    if (sample < NATIVE_VISUALIZER_MIN_SAMPLE) {
        return NATIVE_VISUALIZER_MIN_SAMPLE;
    }
    if (sample > NATIVE_VISUALIZER_MAX_SAMPLE) {
        return NATIVE_VISUALIZER_MAX_SAMPLE;
    }
    return (int16)sample;
}

static NativeVisualizerScreen *
visualizer_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcWindow *
visualizer_active_window_callback(NcScreen *screen) {
    return native_visualizer_screen_window(visualizer_from_screen(screen));
}

static void
visualizer_refresh_callback(NcScreen *screen) {
    nc_window_display(native_visualizer_screen_window(
        visualizer_from_screen(screen)));
    return;
}

static void
visualizer_refresh_window_callback(NcScreen *screen) {
    nc_window_display(native_visualizer_screen_window(
        visualizer_from_screen(screen)));
    return;
}

static void
visualizer_scroll_callback(NcScreen *screen, enum NcScroll where) {
    (void)screen;
    (void)where;
    return;
}

static void
visualizer_switch_to_callback(NcScreen *screen) {
    (void)app_controller_switch_to_screen(screen);
    ncm_title_draw_header(nc_screen_title(screen),
                          (int32)strlen(nc_screen_title(screen)));
    return;
}

static void
visualizer_resize_callback(NcScreen *screen) {
    NativeVisualizerScreen *visualizer;
    int64 x;
    int64 width;

    visualizer = visualizer_from_screen(screen);
    nc_screen_switcher_get_resize_params(screen, &x, &width, true);
    native_visualizer_screen_set_geometry(visualizer,
                                          x,
                                          ui_state_main_start_y(),
                                          width,
                                          ui_state_main_height());
    nc_screen_clear_resize_request(screen);
    return;
}

static int32
visualizer_window_timeout_callback(NcScreen *screen) {
    NativeVisualizerScreen *visualizer;

    visualizer = visualizer_from_screen(screen);
    if (visualizer->fps <= 0) {
        return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
    }
    return 1000 / visualizer->fps;
}

static char *
visualizer_title_callback(NcScreen *screen) {
    (void)screen;
    return (char *)NATIVE_VISUALIZER_TITLE;
}

static void
visualizer_update_callback(NcScreen *screen) {
    nc_window_refresh(native_visualizer_screen_window(
        visualizer_from_screen(screen)));
    return;
}

static void
visualizer_mouse_button_pressed_callback(NcScreen *screen, MEVENT event) {
    (void)screen;
    (void)event;
    return;
}

static bool
visualizer_is_lockable_callback(NcScreen *screen) {
    (void)screen;
    return false;
}

static bool
visualizer_is_mergable_callback(NcScreen *screen) {
    (void)screen;
    return true;
}

static void
visualizer_destroy_callback(NcScreen *screen) {
    native_visualizer_screen_destroy(visualizer_from_screen(screen));
    return;
}

static int32
native_visualizer_next_type(int32 type) {
    type += 1;
    if (type >= NATIVE_VISUALIZER_TYPE_LAST) {
        type = 0;
    }
    return type;
}
