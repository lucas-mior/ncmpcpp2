#include "screens/nc_visualizer.h"

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "c/ncm_base.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "cbase/cbase.h"
#include "screens/screen_switcher.h"
#include "statusbar.h"
#include "title.h"
#include "ui_state.h"

#define NATIVE_VISUALIZER_TITLE "Visualizer"
#define NATIVE_VISUALIZER_DEFAULT_RATE 44100
#define NATIVE_VISUALIZER_DEFAULT_CAP 32768
#define NATIVE_VISUALIZER_DEFAULT_FPS 30
#define NATIVE_VISUALIZER_DEFAULT_DFT_SIZE 2
#define NATIVE_VISUALIZER_MAX_DFT_SIZE 5
#define NATIVE_VISUALIZER_DEFAULT_SPECTRUM_GAIN 10.0
#define NATIVE_VISUALIZER_DEFAULT_SPECTRUM_HZ_MIN 20.0
#define NATIVE_VISUALIZER_DEFAULT_SPECTRUM_HZ_MAX 20000.0
#define NATIVE_VISUALIZER_DFT_TOTAL_SIZE (1 << 15)
#define NATIVE_VISUALIZER_DFT_BASE_SIZE 2048
#define NATIVE_VISUALIZER_DFT_PADDING 4
#define NATIVE_VISUALIZER_FREQ_SPACE_CAP 500
#define NATIVE_VISUALIZER_BAR_HEIGHTS_CAP 100
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
static int32 visualizer_system_open_fifo(void *user, char *location,
                                         int32 location_len);
static int32 visualizer_system_open_udp(void *user, char *location,
                                        int32 location_len, char *port,
                                        int32 port_len);
static int64 visualizer_system_read_source(void *user, int32 fd,
                                           void *buffer,
                                           int64 buffer_size);
static void visualizer_system_close_source(void *user, int32 fd);
static bool visualizer_system_get_outputs(void *user,
                                          NcmMpdOutputList *outputs,
                                          NcmError *error);

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

NativeVisualizerDataSourceHooks
native_visualizer_data_source_system_hooks(NcmMpdClient *client) {
    NativeVisualizerDataSourceHooks hooks = {0};

    hooks.open_fifo = visualizer_system_open_fifo;
    hooks.open_udp = visualizer_system_open_udp;
    hooks.read_source = visualizer_system_read_source;
    hooks.close_source = visualizer_system_close_source;
    hooks.get_outputs = visualizer_system_get_outputs;
    hooks.user = client;
    return hooks;
}

void
native_visualizer_screen_init_data_source(
    NativeVisualizerScreen *screen, char *source_location,
    int32 source_location_len) {
    int32 colon;

    if (screen == NULL) {
        return;
    }
    if ((source_location == NULL) || (source_location_len <= 0)) {
        source_location = NULL;
        source_location_len = 0;
    }

    colon = -1;
    for (int32 i = source_location_len - 1; i >= 0; i -= 1) {
        if (source_location[i] == ':') {
            colon = i;
            break;
        }
    }

    ncm_buffer_clear(&screen->source_location);
    ncm_buffer_clear(&screen->source_port);
    if ((source_location_len > 0) && (source_location[0] != '/')
        && (colon >= 0)) {
        ncm_buffer_set(&screen->source_location, source_location, colon);
        ncm_buffer_set(&screen->source_port,
                       source_location + colon + 1,
                       source_location_len - colon - 1);
    } else {
        ncm_buffer_set(&screen->source_location,
                       source_location,
                       source_location_len);
    }
    return;
}

bool
native_visualizer_screen_open_data_source(
    NativeVisualizerScreen *screen) {
    char *location;
    char *port;
    int32 fd;

    if (screen == NULL) {
        return false;
    }
    if (screen->source_fd >= 0) {
        return true;
    }

    location = screen->source_location.data;
    if (location == NULL) {
        location = (char *)"";
    }
    port = screen->source_port.data;
    if (port == NULL) {
        port = (char *)"";
    }

    fd = -1;
    if (screen->source_port.len > 0) {
        if (screen->data_source_hooks.open_udp == NULL) {
            return false;
        }
        fd = screen->data_source_hooks.open_udp(
            screen->data_source_hooks.user,
            location,
            screen->source_location.len,
            port,
            screen->source_port.len);
    } else {
        if (screen->data_source_hooks.open_fifo == NULL) {
            return false;
        }
        fd = screen->data_source_hooks.open_fifo(
            screen->data_source_hooks.user,
            location,
            screen->source_location.len);
    }

    screen->source_fd = fd;
    return fd >= 0;
}

void
native_visualizer_screen_close_data_source(
    NativeVisualizerScreen *screen) {
    int32 fd;

    if ((screen == NULL) || (screen->source_fd < 0)) {
        return;
    }

    fd = screen->source_fd;
    screen->source_fd = -1;
    if (screen->data_source_hooks.close_source) {
        screen->data_source_hooks.close_source(
            screen->data_source_hooks.user, fd);
    }
    return;
}

int64
native_visualizer_screen_drain_data_source(
    NativeVisualizerScreen *screen) {
    int64 buffer_size;
    int64 bytes_read;
    int64 total_read;

    if ((screen == NULL) || (screen->source_fd < 0)
        || (screen->data_source_hooks.read_source == NULL)) {
        return 0;
    }
    if (screen->incoming_samples.data == NULL) {
        return 0;
    }

    buffer_size = ncm_sample_buffer_capacity(&screen->incoming_samples)
                  *SIZEOF(*screen->incoming_samples.data);
    if (buffer_size <= 0) {
        return 0;
    }

    total_read = 0;
    do {
        bytes_read = screen->data_source_hooks.read_source(
            screen->data_source_hooks.user,
            screen->source_fd,
            screen->incoming_samples.data,
            buffer_size);
        if (bytes_read > 0) {
            total_read += bytes_read;
        }
    } while (bytes_read > 0);
    return total_read;
}

bool
native_visualizer_screen_find_output_id(
    NativeVisualizerScreen *screen) {
    NcmMpdOutputList outputs;
    NcmError error;
    bool found;

    if (screen == NULL) {
        return false;
    }

    screen->output_id = -1;
    if ((screen->output_name.len <= 0) || (screen->source_port.len > 0)) {
        return true;
    }
    if (screen->data_source_hooks.get_outputs == NULL) {
        return false;
    }

    ncm_error_clear(&error);
    ncm_mpd_output_list_init(&outputs);
    if (!screen->data_source_hooks.get_outputs(
            screen->data_source_hooks.user, &outputs, &error)) {
        NcmStringFormatArg arg;

        arg = ncm_string_format_arg_cstring(error.message);
        ncm_statusbar_format(ncm_statusbar_message_delay_time(),
                             STRLIT_ARGS("Could not fetch outputs: %1"),
                             &arg,
                             1);
        ncm_mpd_output_list_destroy(&outputs);
        return false;
    }

    found = false;
    for (int32 i = 0; i < outputs.count; i += 1) {
        NcmMpdOutput *output;

        output = outputs.items + i;
        if (ncm_string_equal(screen->output_name.data,
                             screen->output_name.len,
                             output->name,
                             output->name_len)) {
            screen->output_id = (int32)output->id;
            found = true;
            break;
        }
    }
    ncm_mpd_output_list_destroy(&outputs);

    if (!found) {
        NcmStringFormatArg arg;

        arg = ncm_string_format_arg_string(screen->output_name.data,
                                            screen->output_name.len);
        ncm_statusbar_format(ncm_statusbar_message_delay_time(),
                             STRLIT_ARGS("There is no output named \"%1\""),
                             &arg,
                             1);
    }
    return found;
}

void
native_visualizer_screen_init(NativeVisualizerScreen *screen,
                              int64 start_x, int64 start_y,
                              int64 width, int64 height,
                              NcColor color, NcBorder border,
                              NativeVisualizerScreenConfig *config) {
    char *source_location;
    char *output_name;
    int32 source_location_len;
    int32 output_name_len;
    int32 incoming_cap;
    int32 rendered_cap;
    int32 fps;
#if defined(HAVE_FFTW3_H)
    uint32 spectrum_dft_size;
    double spectrum_gain;
    double spectrum_hz_min;
    double spectrum_hz_max;
#endif
    NativeVisualizerDataSourceHooks data_source_hooks;
    bool stereo;

    source_location = NULL;
    output_name = NULL;
    source_location_len = 0;
    output_name_len = 0;
    fps = NATIVE_VISUALIZER_DEFAULT_FPS;
    data_source_hooks = native_visualizer_data_source_system_hooks(NULL);
#if defined(HAVE_FFTW3_H)
    spectrum_dft_size = NATIVE_VISUALIZER_DEFAULT_DFT_SIZE;
    spectrum_gain = NATIVE_VISUALIZER_DEFAULT_SPECTRUM_GAIN;
    spectrum_hz_min = NATIVE_VISUALIZER_DEFAULT_SPECTRUM_HZ_MIN;
    spectrum_hz_max = NATIVE_VISUALIZER_DEFAULT_SPECTRUM_HZ_MAX;
#endif
    stereo = false;
    if (config != NULL) {
        source_location = config->source_location;
        output_name = config->output_name;
        source_location_len = config->source_location_len;
        output_name_len = config->output_name_len;
        fps = config->fps;
        if (config->data_source_hooks.open_fifo
            || config->data_source_hooks.open_udp
            || config->data_source_hooks.read_source
            || config->data_source_hooks.close_source
            || config->data_source_hooks.get_outputs) {
            data_source_hooks = config->data_source_hooks;
        }
#if defined(HAVE_FFTW3_H)
        spectrum_dft_size = config->spectrum_dft_size;
        spectrum_gain = config->spectrum_gain;
        spectrum_hz_min = config->spectrum_hz_min;
        spectrum_hz_max = config->spectrum_hz_max;
#endif
        stereo = config->stereo;
    }
    if ((source_location == NULL) || (source_location_len < 0)) {
        source_location = NULL;
        source_location_len = 0;
    }
    if ((output_name == NULL) || (output_name_len < 0)) {
        output_name = NULL;
        output_name_len = 0;
    }
    if (fps <= 0) {
        fps = NATIVE_VISUALIZER_DEFAULT_FPS;
    }
#if defined(HAVE_FFTW3_H)
    if ((spectrum_dft_size == 0)
        || (spectrum_dft_size > NATIVE_VISUALIZER_MAX_DFT_SIZE)) {
        spectrum_dft_size = NATIVE_VISUALIZER_DEFAULT_DFT_SIZE;
    }
#endif

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
    ncm_buffer_init(&screen->source_location);
    ncm_buffer_init(&screen->source_port);
    ncm_buffer_init(&screen->output_name);
    native_visualizer_screen_init_data_source(screen,
                                              source_location,
                                              source_location_len);
    ncm_buffer_set(&screen->output_name, output_name, output_name_len);
    screen->data_source_hooks = data_source_hooks;

    ncm_sample_buffer_init(&screen->incoming_samples);
    ncm_sample_buffer_init(&screen->buffered_samples);
    ncm_sample_buffer_init(&screen->rendered_samples);
    ncm_sample_buffer_init(&screen->left_channel);
    ncm_sample_buffer_init(&screen->right_channel);

    incoming_cap = NATIVE_VISUALIZER_DEFAULT_RATE / 2;
    if (stereo) {
        incoming_cap *= 2;
    }
    rendered_cap = NATIVE_VISUALIZER_DEFAULT_CAP;
#if defined(HAVE_FFTW3_H)
    screen->fft.input = NULL;
    screen->fft.output = NULL;
    screen->fft.plan = NULL;
    screen->fft.frequency_magnitudes = NULL;
    screen->fft.dft_frequency_space = NULL;
    screen->fft.bar_heights = NULL;

    screen->fft.dft_nonzero_size = NATIVE_VISUALIZER_DFT_BASE_SIZE
                                   *(2*(int32)spectrum_dft_size
                                     + NATIVE_VISUALIZER_DFT_PADDING);
    screen->fft.dft_total_size = NATIVE_VISUALIZER_DFT_TOTAL_SIZE;
    screen->fft.results_len = screen->fft.dft_total_size / 2 + 1;
    screen->fft.dynamic_range = 100.0 - spectrum_gain;
    screen->fft.hz_min = spectrum_hz_min;
    screen->fft.hz_max = spectrum_hz_max;
    screen->fft.gain = spectrum_gain;

    screen->fft.frequency_magnitudes_len = screen->fft.results_len;
    screen->fft.frequency_magnitudes_cap = screen->fft.results_len;
    screen->fft.dft_frequency_space_len = 0;
    screen->fft.dft_frequency_space_cap =
        NATIVE_VISUALIZER_FREQ_SPACE_CAP;
    screen->fft.bar_heights_len = 0;
    screen->fft.bar_heights_cap = NATIVE_VISUALIZER_BAR_HEIGHTS_CAP;

    screen->fft.frequency_magnitudes = ncm_malloc(
        screen->fft.frequency_magnitudes_cap
        *SIZEOF(*screen->fft.frequency_magnitudes));
    cbase_memset(screen->fft.frequency_magnitudes,
             0,
             screen->fft.frequency_magnitudes_cap
             *SIZEOF(*screen->fft.frequency_magnitudes));
    screen->fft.dft_frequency_space = ncm_malloc(
        screen->fft.dft_frequency_space_cap
        *SIZEOF(*screen->fft.dft_frequency_space));
    screen->fft.bar_heights = ncm_malloc(
        screen->fft.bar_heights_cap
        *SIZEOF(*screen->fft.bar_heights));

    screen->fft.input = fftw_malloc(
        (size_t)(screen->fft.dft_total_size
                 *SIZEOF(*screen->fft.input)));
    screen->fft.output = fftw_malloc(
        (size_t)(screen->fft.results_len
                 *SIZEOF(*screen->fft.output)));
    if (screen->fft.input != NULL) {
        cbase_memset(screen->fft.input,
                 0,
                 screen->fft.dft_total_size
                 *SIZEOF(*screen->fft.input));
    }
    if ((screen->fft.input != NULL) && (screen->fft.output != NULL)) {
        screen->fft.plan = fftw_plan_dft_r2c_1d(
            screen->fft.dft_total_size,
            screen->fft.input,
            screen->fft.output,
            FFTW_ESTIMATE);
    }

    if (stereo) {
        if (screen->fft.dft_nonzero_size*2 > rendered_cap) {
            rendered_cap = screen->fft.dft_nonzero_size*2;
        }
    } else if (screen->fft.dft_nonzero_size > rendered_cap) {
        rendered_cap = screen->fft.dft_nonzero_size;
    }
#endif

    ncm_sample_buffer_resize(&screen->incoming_samples, incoming_cap);
    ncm_sample_buffer_resize(&screen->buffered_samples, incoming_cap);
    ncm_sample_buffer_resize(&screen->rendered_samples, rendered_cap);
    ncm_sample_buffer_resize(&screen->left_channel, rendered_cap / 2);
    ncm_sample_buffer_resize(&screen->right_channel, rendered_cap / 2);

    screen->auto_scale_multiplier = 1.0;
    screen->visualization_type = NATIVE_VISUALIZER_WAVE;
    screen->source_fd = -1;
    screen->output_id = -1;
    screen->fps = fps;
    screen->sample_rate = NATIVE_VISUALIZER_DEFAULT_RATE;
    screen->sample_consumption_rate = 0;
    screen->sample_consumption_rate_up_ctr = 0;
    screen->sample_consumption_rate_dn_ctr = 0;
    screen->reset_output = false;
    screen->stereo = stereo;
    screen->initialized = true;
    return;
}

void
native_visualizer_screen_destroy(NativeVisualizerScreen *screen) {
    if ((screen == NULL) || !screen->initialized) {
        return;
    }

    native_visualizer_screen_close_data_source(screen);

#if defined(HAVE_FFTW3_H)
    if (screen->fft.plan != NULL) {
        fftw_destroy_plan(screen->fft.plan);
        screen->fft.plan = NULL;
    }
    if (screen->fft.output != NULL) {
        fftw_free(screen->fft.output);
        screen->fft.output = NULL;
    }
    if (screen->fft.input != NULL) {
        fftw_free(screen->fft.input);
        screen->fft.input = NULL;
    }
    if (screen->fft.bar_heights != NULL) {
        ncm_free(screen->fft.bar_heights,
                 screen->fft.bar_heights_cap
                 *SIZEOF(*screen->fft.bar_heights));
        screen->fft.bar_heights = NULL;
    }
    if (screen->fft.dft_frequency_space != NULL) {
        ncm_free(screen->fft.dft_frequency_space,
                 screen->fft.dft_frequency_space_cap
                 *SIZEOF(*screen->fft.dft_frequency_space));
        screen->fft.dft_frequency_space = NULL;
    }
    if (screen->fft.frequency_magnitudes != NULL) {
        ncm_free(screen->fft.frequency_magnitudes,
                 screen->fft.frequency_magnitudes_cap
                 *SIZEOF(*screen->fft.frequency_magnitudes));
        screen->fft.frequency_magnitudes = NULL;
    }
    screen->fft.results_len = 0;
    screen->fft.dft_nonzero_size = 0;
    screen->fft.dft_total_size = 0;
    screen->fft.frequency_magnitudes_len = 0;
    screen->fft.frequency_magnitudes_cap = 0;
    screen->fft.dft_frequency_space_len = 0;
    screen->fft.dft_frequency_space_cap = 0;
    screen->fft.bar_heights_len = 0;
    screen->fft.bar_heights_cap = 0;
#endif

    ncm_sample_buffer_destroy(&screen->right_channel);
    ncm_sample_buffer_destroy(&screen->left_channel);
    ncm_sample_buffer_destroy(&screen->rendered_samples);
    ncm_sample_buffer_destroy(&screen->buffered_samples);
    ncm_sample_buffer_destroy(&screen->incoming_samples);
    ncm_buffer_destroy(&screen->output_name);
    ncm_buffer_destroy(&screen->source_port);
    ncm_buffer_destroy(&screen->source_location);
    nc_window_destroy(&screen->window);

    screen->output_id = -1;
    screen->reset_output = false;
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
    native_visualizer_screen_drain_data_source(screen);
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

static int32
visualizer_system_open_fifo(void *user, char *location,
                            int32 location_len) {
    int32 fd;

    (void)user;
    fd = open(location, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        NcmStringFormatArg args[2];

        args[0] = ncm_string_format_arg_string(location, location_len);
        args[1] = ncm_string_format_arg_cstring(strerror(errno));
        ncm_statusbar_format(
            ncm_statusbar_message_delay_time(),
            STRLIT_ARGS("Couldn't open \"%1\" for reading PCM data: %2"),
            args,
            2);
    }
    return fd;
}

static int32
visualizer_system_open_udp(void *user, char *location,
                           int32 location_len, char *port,
                           int32 port_len) {
    struct addrinfo hints = {0};
    struct addrinfo *addresses;
    struct addrinfo *address;
    int32 error_code;
    int32 fd;

    (void)user;
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    addresses = NULL;
    error_code = getaddrinfo(location, port, &hints, &addresses);
    if (error_code != 0) {
        NcmStringFormatArg args[3];

        args[0] = ncm_string_format_arg_string(location, location_len);
        args[1] = ncm_string_format_arg_string(port, port_len);
        args[2] = ncm_string_format_arg_cstring(
            (char *)gai_strerror(error_code));
        ncm_statusbar_format(
            ncm_statusbar_message_delay_time(),
            STRLIT_ARGS("Couldn't resolve \"%1:%2\": %3"),
            args,
            3);
        return -1;
    }

    fd = -1;
    for (address = addresses; address; address = address->ai_next) {
        int32 socket_flags;

        fd = socket(address->ai_family,
                    address->ai_socktype,
                    address->ai_protocol);
        if (fd < 0) {
            fprintf(stderr, "Creation of socket failed: %s\n",
                    strerror(errno));
            continue;
        }

        socket_flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, socket_flags | O_NONBLOCK);
        error_code = bind(fd, address->ai_addr, address->ai_addrlen);
        if (error_code < 0) {
            fprintf(stderr, "Binding a socket failed: %s\n",
                    strerror(errno));
            close(fd);
            fd = -1;
            continue;
        }
        break;
    }

    freeaddrinfo(addresses);
    return fd;
}

static int64
visualizer_system_read_source(void *user, int32 fd, void *buffer,
                              int64 buffer_size) {
    (void)user;
    return (int64)read(fd, buffer, (size_t)buffer_size);
}

static void
visualizer_system_close_source(void *user, int32 fd) {
    (void)user;
    close(fd);
    return;
}

static bool
visualizer_system_get_outputs(void *user, NcmMpdOutputList *outputs,
                              NcmError *error) {
    NcmMpdClient *client;

    client = user;
    if (client == NULL) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("MPD client is not configured"));
        return false;
    }
    return ncm_mpd_client_get_outputs(client, outputs, error);
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
