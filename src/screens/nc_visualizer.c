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
#include "cbase/utf8.c"
#include "cbase/base_macros.h"
#include "cbase/util.c"
#include "screens/screen_switcher.h"
#include "status.h"
#include "statusbar.h"
#include "title.h"
#include "ui_state.h"

#define NATIVE_VISUALIZER_TITLE "Music visualizer"
#define NATIVE_VISUALIZER_DEFAULT_RATE 44100
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
#define NATIVE_VISUALIZER_DEFAULT_CHARS "●▮"

#if defined(HAVE_FFTW3_H)
#define NATIVE_VISUALIZER_SMOOTH_CHAR_COUNT 8

static char *visualizer_smooth_chars[NATIVE_VISUALIZER_SMOOTH_CHAR_COUNT] = {
    "▁",
    "▂",
    "▃",
    "▄",
    "▅",
    "▆",
    "▇",
    "█",
};

static int32 visualizer_smooth_char_lens[
    NATIVE_VISUALIZER_SMOOTH_CHAR_COUNT] = {
    STRLIT_LEN("▁"),
    STRLIT_LEN("▂"),
    STRLIT_LEN("▃"),
    STRLIT_LEN("▄"),
    STRLIT_LEN("▅"),
    STRLIT_LEN("▆"),
    STRLIT_LEN("▇"),
    STRLIT_LEN("█"),
};

static char *visualizer_smooth_flipped_chars[
    NATIVE_VISUALIZER_SMOOTH_CHAR_COUNT] = {
    "▔",
    "🮂",
    "🮃",
    "🮄",
    "🬎",
    "🮅",
    "🮆",
    "█",
};

static int32 visualizer_smooth_flipped_char_lens[
    NATIVE_VISUALIZER_SMOOTH_CHAR_COUNT] = {
    STRLIT_LEN("▔"),
    STRLIT_LEN("🮂"),
    STRLIT_LEN("🮃"),
    STRLIT_LEN("🮄"),
    STRLIT_LEN("🬎"),
    STRLIT_LEN("🮅"),
    STRLIT_LEN("🮆"),
    STRLIT_LEN("█"),
};
#endif

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
static bool visualizer_system_disable_output(void *user, uint32 id,
                                             NcmError *error);
static bool visualizer_system_enable_output(void *user, uint32 id,
                                            NcmError *error);
static void visualizer_system_sleep_microseconds(void *user,
                                                 int32 microseconds);
static bool visualizer_reset_output(NativeVisualizerScreen *screen);
static int32 visualizer_read_samples(NativeVisualizerScreen *screen);
static void visualizer_prepare_drawing(NativeVisualizerScreen *screen);
static void visualizer_copy_characters(NativeVisualizerScreen *screen,
                                       char *characters,
                                       int32 characters_len);
static void visualizer_copy_colors(NativeVisualizerScreen *screen,
                                   NcFormattedColor *colors,
                                   int32 colors_len);
static void visualizer_destroy_colors(NativeVisualizerScreen *screen);
static NcFormattedColor *visualizer_color(NativeVisualizerScreen *screen,
                                          double number, double max,
                                          bool wrap);
static void visualizer_draw_character(NativeVisualizerScreen *screen,
                                      int32 x, int32 y,
                                      NcFormattedColor *color,
                                      bool reverse, char *character,
                                      int32 character_len);
static void visualizer_draw_wave(NativeVisualizerScreen *screen,
                                 int16 *samples, int32 samples_len,
                                 int32 y_offset, int32 height);
static void visualizer_draw_wave_filled(NativeVisualizerScreen *screen,
                                        int16 *samples,
                                        int32 samples_len,
                                        int32 y_offset, int32 height);
static void visualizer_draw_ellipse(NativeVisualizerScreen *screen,
                                    int16 *samples, int32 samples_len,
                                    int32 height);
static void visualizer_draw_ellipse_stereo(
    NativeVisualizerScreen *screen, int16 *left, int16 *right,
    int32 samples_len, int32 half_height);
#if defined(HAVE_FFTW3_H)
static void visualizer_fft_init(NativeVisualizerScreen *screen,
                                uint32 dft_size, double gain,
                                double hz_min, double hz_max);
static void visualizer_fft_destroy(NativeVisualizerScreen *screen);
static void visualizer_fft_reserve_frequency_space(
    NativeVisualizerScreen *screen, int32 capacity);
static void visualizer_fft_reserve_bar_heights(
    NativeVisualizerScreen *screen, int32 capacity);
static void visualizer_generate_frequency_space(
    NativeVisualizerScreen *screen);
static void visualizer_apply_fft_window(NativeVisualizerScreen *screen,
                                        int16 *samples,
                                        int32 samples_len);
static double visualizer_bin_to_hz(NativeVisualizerScreen *screen,
                                   int32 bin);
static double visualizer_interpolate_cubic(
    NativeVisualizerScreen *screen, int32 x, int32 height_index);
static double visualizer_interpolate_linear(
    NativeVisualizerScreen *screen, int32 x, int32 height_index);
static void visualizer_draw_frequency(NativeVisualizerScreen *screen,
                                      int16 *samples, int32 samples_len,
                                      int32 y_offset, int32 height);
#endif

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
    hooks.disable_output = visualizer_system_disable_output;
    hooks.enable_output = visualizer_system_enable_output;
    hooks.sleep_microseconds = visualizer_system_sleep_microseconds;
    hooks.user = client;
    return hooks;
}

static void
visualizer_copy_characters(NativeVisualizerScreen *screen,
                           char *characters, int32 characters_len) {
    int32 next;

    ncm_buffer_set(&screen->visualizer_chars, characters, characters_len);
    next = utf8_next_position(screen->visualizer_chars.data,
                                  screen->visualizer_chars.len, 0);
    screen->point_char_offset = 0;
    screen->point_char_len = next;
    screen->bar_char_offset = next;
    screen->bar_char_len = screen->visualizer_chars.len - next;
    return;
}

static void
visualizer_destroy_colors(NativeVisualizerScreen *screen) {
    if (screen->visualizer_colors != NULL) {
        for (int32 i = 0; i < screen->visualizer_colors_len; i += 1) {
            nc_formatted_color_destroy(&screen->visualizer_colors[i]);
        }
        free2(screen->visualizer_colors,
            screen->visualizer_colors_cap
            *SIZEOF(*screen->visualizer_colors));
    }
    screen->visualizer_colors = NULL;
    screen->visualizer_colors_len = 0;
    screen->visualizer_colors_cap = 0;
    return;
}

static void
visualizer_copy_colors(NativeVisualizerScreen *screen,
                       NcFormattedColor *colors, int32 colors_len) {
    visualizer_destroy_colors(screen);
    if ((colors == NULL) || (colors_len <= 0)) {
        screen->visualizer_colors_cap = 1;
        screen->visualizer_colors = malloc2(
            SIZEOF(*screen->visualizer_colors));
        nc_formatted_color_init(&screen->visualizer_colors[0]);
        screen->visualizer_colors_len = 1;
        return;
    }

    screen->visualizer_colors_cap = colors_len;
    screen->visualizer_colors = malloc2(
        colors_len*SIZEOF(*screen->visualizer_colors));
    for (int32 i = 0; i < colors_len; i += 1) {
        nc_formatted_color_copy(&screen->visualizer_colors[i],
                                &colors[i]);
    }
    screen->visualizer_colors_len = colors_len;
    return;
}

static NcFormattedColor *
visualizer_color(NativeVisualizerScreen *screen, double number,
                 double max, bool wrap) {
    int64 index;

    if ((screen->visualizer_colors == NULL)
        || (screen->visualizer_colors_len <= 0)) {
        return NULL;
    }
    if ((max <= 0.0) || !isfinite(number) || !isfinite(max)) {
        return &screen->visualizer_colors[0];
    }
    if (number < 0.0) {
        number = -number;
    }

    index = (int64)(number*(double)screen->visualizer_colors_len/max);
    if (wrap) {
        index %= screen->visualizer_colors_len;
    } else if (index >= screen->visualizer_colors_len) {
        index = screen->visualizer_colors_len - 1;
    }
    if (index < 0) {
        index = 0;
    }
    return &screen->visualizer_colors[index];
}

static void
visualizer_draw_character(NativeVisualizerScreen *screen,
                          int32 x, int32 y, NcFormattedColor *color,
                          bool reverse, char *character,
                          int32 character_len) {
    enum NcFormat *formats;
    int32 count;
    int64 width;
    int64 height;

    width = nc_window_width(&screen->window);
    height = nc_window_height(&screen->window);
    if ((x < 0) || (y < 0) || (x >= width) || (y >= height)
        || (character == NULL) || (character_len <= 0)) {
        return;
    }

    nc_window_go_to_xy(&screen->window, x, y);
    formats = NULL;
    count = 0;
    if (color != NULL) {
        nc_window_push_color(&screen->window, color->color);
        formats = nc_formatted_color_formats(color);
        count = nc_formatted_color_format_count(color);
        for (int32 i = 0; i < count; i += 1) {
            nc_window_apply_format(&screen->window, formats[i]);
        }
    }
    if (reverse) {
        nc_window_apply_format(&screen->window, NC_FORMAT_REVERSE);
    }

    nc_window_print_data(&screen->window, character, character_len);

    if (reverse) {
        nc_window_apply_format(&screen->window, NC_FORMAT_NO_REVERSE);
    }
    if (color != NULL) {
        for (int32 i = count - 1; i >= 0; i -= 1) {
            nc_window_apply_format(&screen->window,
                                   nc_format_reverse(formats[i]));
        }
        if (!nc_color_is_default(color->color)) {
            nc_window_push_color(&screen->window, nc_color_end());
        }
    }
    return;
}

#if defined(HAVE_FFTW3_H)
static void
visualizer_fft_init(NativeVisualizerScreen *screen, uint32 dft_size,
                    double gain, double hz_min, double hz_max) {
    NativeVisualizerFftState *fft;

    fft = &screen->fft;
    *fft = (NativeVisualizerFftState){0};
    fft->dft_nonzero_size = NATIVE_VISUALIZER_DFT_BASE_SIZE
                            *(2*(int32)dft_size
                              + NATIVE_VISUALIZER_DFT_PADDING);
    fft->dft_total_size = NATIVE_VISUALIZER_DFT_TOTAL_SIZE;
    fft->results_len = fft->dft_total_size / 2 + 1;
    fft->dynamic_range = 100.0 - gain;
    fft->hz_min = hz_min;
    fft->hz_max = hz_max;
    fft->gain = gain;

    fft->frequency_magnitudes_len = fft->results_len;
    fft->frequency_magnitudes_cap = fft->results_len;
    fft->dft_frequency_space_cap = NATIVE_VISUALIZER_FREQ_SPACE_CAP;
    fft->bar_heights_cap = NATIVE_VISUALIZER_BAR_HEIGHTS_CAP;
    fft->frequency_magnitudes = malloc2(
        fft->frequency_magnitudes_cap
        *SIZEOF(*fft->frequency_magnitudes));
    fft->dft_frequency_space = malloc2(
        fft->dft_frequency_space_cap
        *SIZEOF(*fft->dft_frequency_space));
    fft->bar_heights = malloc2(
        fft->bar_heights_cap*SIZEOF(*fft->bar_heights));
    memset64(fft->frequency_magnitudes, 0,
         fft->frequency_magnitudes_cap
         *SIZEOF(*fft->frequency_magnitudes));

    fft->input = fftw_malloc(
        (size_t)(fft->dft_total_size*SIZEOF(*fft->input)));
    fft->output = fftw_malloc(
        (size_t)(fft->results_len*SIZEOF(*fft->output)));
    if ((fft->input == NULL) || (fft->output == NULL)) {
        visualizer_fft_destroy(screen);
        return;
    }
    memset64(fft->input, 0,
         fft->dft_total_size*SIZEOF(*fft->input));
    fft->plan = fftw_plan_dft_r2c_1d(fft->dft_total_size,
                                     fft->input, fft->output,
                                     FFTW_ESTIMATE);
    if (fft->plan == NULL) {
        visualizer_fft_destroy(screen);
    }
    return;
}

static void
visualizer_fft_destroy(NativeVisualizerScreen *screen) {
    NativeVisualizerFftState *fft;

    fft = &screen->fft;
    if (fft->plan != NULL) {
        fftw_destroy_plan(fft->plan);
    }
    if (fft->output != NULL) {
        fftw_free(fft->output);
    }
    if (fft->input != NULL) {
        fftw_free(fft->input);
    }
    if (fft->bar_heights != NULL) {
        free2(fft->bar_heights,
            fft->bar_heights_cap*SIZEOF(*fft->bar_heights));
    }
    if (fft->dft_frequency_space != NULL) {
        free2(fft->dft_frequency_space,
            fft->dft_frequency_space_cap
            *SIZEOF(*fft->dft_frequency_space));
    }
    if (fft->frequency_magnitudes != NULL) {
        free2(fft->frequency_magnitudes,
            fft->frequency_magnitudes_cap
            *SIZEOF(*fft->frequency_magnitudes));
    }
    *fft = (NativeVisualizerFftState){0};
    return;
}

static void
visualizer_fft_reserve_frequency_space(NativeVisualizerScreen *screen,
                                       int32 capacity) {
    NativeVisualizerFftState *fft;
    int32 old_cap;
    int32 new_cap;

    fft = &screen->fft;
    if (capacity <= fft->dft_frequency_space_cap) {
        return;
    }
    old_cap = fft->dft_frequency_space_cap;
    new_cap = old_cap;
    if (new_cap <= 0) {
        new_cap = NATIVE_VISUALIZER_FREQ_SPACE_CAP;
    }
    while (new_cap < capacity) {
        if (new_cap > INT32_MAX / 2) {
            new_cap = capacity;
            break;
        }
        new_cap *= 2;
    }
    fft->dft_frequency_space = realloc2(
        fft->dft_frequency_space, old_cap, new_cap,
        SIZEOF(*fft->dft_frequency_space));
    fft->dft_frequency_space_cap = new_cap;
    return;
}

static void
visualizer_fft_reserve_bar_heights(NativeVisualizerScreen *screen,
                                   int32 capacity) {
    NativeVisualizerFftState *fft;
    int32 old_cap;
    int32 new_cap;

    fft = &screen->fft;
    if (capacity <= fft->bar_heights_cap) {
        return;
    }
    old_cap = fft->bar_heights_cap;
    new_cap = old_cap;
    if (new_cap <= 0) {
        new_cap = NATIVE_VISUALIZER_BAR_HEIGHTS_CAP;
    }
    while (new_cap < capacity) {
        if (new_cap > INT32_MAX / 2) {
            new_cap = capacity;
            break;
        }
        new_cap *= 2;
    }
    fft->bar_heights = realloc2(
        fft->bar_heights, old_cap, new_cap,
        SIZEOF(*fft->bar_heights));
    fft->bar_heights_cap = new_cap;
    return;
}
#endif

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
        location = "";
    }
    port = screen->source_port.data;
    if (port == NULL) {
        port = "";
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
        if (STREQUAL(screen->output_name.data,
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
    char *visualizer_chars;
    NcFormattedColor *visualizer_colors;
    int32 source_location_len;
    int32 output_name_len;
    int32 visualizer_chars_len;
    int32 visualizer_colors_len;
    int32 fps;
#if defined(HAVE_FFTW3_H)
    uint32 spectrum_dft_size;
    double spectrum_gain;
    double spectrum_hz_min;
    double spectrum_hz_max;
#endif
    NativeVisualizerDataSourceHooks data_source_hooks;
    enum NativeVisualizerType visualization_type;
    bool autoscale;
    bool stereo;
    bool spectrum_smooth_look;
    bool spectrum_smooth_look_legacy_chars;
    bool spectrum_log_scale_x;
    bool spectrum_log_scale_y;

    source_location = NULL;
    output_name = NULL;
    visualizer_chars = (char *)NATIVE_VISUALIZER_DEFAULT_CHARS;
    visualizer_colors = NULL;
    source_location_len = 0;
    output_name_len = 0;
    visualizer_chars_len = STRLIT_LEN(NATIVE_VISUALIZER_DEFAULT_CHARS);
    visualizer_colors_len = 0;
    fps = NATIVE_VISUALIZER_DEFAULT_FPS;
    data_source_hooks = native_visualizer_data_source_system_hooks(NULL);
#if defined(HAVE_FFTW3_H)
    spectrum_dft_size = NATIVE_VISUALIZER_DEFAULT_DFT_SIZE;
    spectrum_gain = NATIVE_VISUALIZER_DEFAULT_SPECTRUM_GAIN;
    spectrum_hz_min = NATIVE_VISUALIZER_DEFAULT_SPECTRUM_HZ_MIN;
    spectrum_hz_max = NATIVE_VISUALIZER_DEFAULT_SPECTRUM_HZ_MAX;
#endif
    visualization_type = NATIVE_VISUALIZER_WAVE;
    autoscale = false;
    stereo = false;
    spectrum_smooth_look = true;
    spectrum_smooth_look_legacy_chars = true;
    spectrum_log_scale_x = true;
    spectrum_log_scale_y = true;
    if (config != NULL) {
        source_location = config->source_location;
        output_name = config->output_name;
        visualizer_chars = config->visualizer_chars;
        visualizer_colors = config->visualizer_colors;
        source_location_len = config->source_location_len;
        output_name_len = config->output_name_len;
        visualizer_chars_len = config->visualizer_chars_len;
        visualizer_colors_len = config->visualizer_colors_len;
        fps = config->fps;
        if (config->data_source_hooks.open_fifo
            || config->data_source_hooks.open_udp
            || config->data_source_hooks.read_source
            || config->data_source_hooks.close_source
            || config->data_source_hooks.get_outputs
            || config->data_source_hooks.disable_output
            || config->data_source_hooks.enable_output
            || config->data_source_hooks.sleep_microseconds) {
            data_source_hooks = config->data_source_hooks;
        }
#if defined(HAVE_FFTW3_H)
        spectrum_dft_size = config->spectrum_dft_size;
        spectrum_gain = config->spectrum_gain;
        spectrum_hz_min = config->spectrum_hz_min;
        spectrum_hz_max = config->spectrum_hz_max;
#endif
        visualization_type = config->visualization_type;
        autoscale = config->autoscale;
        stereo = config->stereo;
        spectrum_smooth_look = config->spectrum_smooth_look;
        spectrum_smooth_look_legacy_chars =
            config->spectrum_smooth_look_legacy_chars;
        spectrum_log_scale_x = config->spectrum_log_scale_x;
        spectrum_log_scale_y = config->spectrum_log_scale_y;
    }
    if ((source_location == NULL) || (source_location_len < 0)) {
        source_location = NULL;
        source_location_len = 0;
    }
    if ((output_name == NULL) || (output_name_len < 0)) {
        output_name = NULL;
        output_name_len = 0;
    }
    if ((visualizer_chars == NULL)
        || (visualizer_chars_len <= 0)
        || (utf8_characters(visualizer_chars,
                                visualizer_chars_len) != 2)) {
        visualizer_chars = (char *)NATIVE_VISUALIZER_DEFAULT_CHARS;
        visualizer_chars_len = STRLIT_LEN(NATIVE_VISUALIZER_DEFAULT_CHARS);
    }
    if ((visualizer_colors == NULL) || (visualizer_colors_len < 0)) {
        visualizer_colors = NULL;
        visualizer_colors_len = 0;
    }
    if (fps <= 0) {
        fps = NATIVE_VISUALIZER_DEFAULT_FPS;
    }
    if ((visualization_type < 0)
        || (visualization_type >= NATIVE_VISUALIZER_TYPE_LAST)) {
        visualization_type = NATIVE_VISUALIZER_WAVE;
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
    ncm_buffer_init(&screen->visualizer_chars);
    screen->visualizer_colors = NULL;
    screen->visualizer_colors_len = 0;
    screen->visualizer_colors_cap = 0;
    native_visualizer_screen_init_data_source(screen,
                                              source_location,
                                              source_location_len);
    ncm_buffer_set(&screen->output_name, output_name, output_name_len);
    visualizer_copy_characters(screen, visualizer_chars,
                               visualizer_chars_len);
    visualizer_copy_colors(screen, visualizer_colors,
                           visualizer_colors_len);
    screen->data_source_hooks = data_source_hooks;

    ncm_sample_buffer_init(&screen->incoming_samples);
    ncm_sample_buffer_init(&screen->buffered_samples);
    ncm_sample_buffer_init(&screen->rendered_samples);
    ncm_sample_buffer_init(&screen->left_channel);
    ncm_sample_buffer_init(&screen->right_channel);

#if defined(HAVE_FFTW3_H)
    visualizer_fft_init(screen, spectrum_dft_size, spectrum_gain,
                        spectrum_hz_min, spectrum_hz_max);
#endif

    screen->auto_scale_multiplier = 1.0;
    screen->visualization_type = visualization_type;
    screen->source_fd = -1;
    screen->output_id = -1;
    screen->fps = fps;
    screen->sample_rate = NATIVE_VISUALIZER_DEFAULT_RATE;
    screen->sample_consumption_rate = 5;
    screen->sample_consumption_rate_up_ctr = 0;
    screen->sample_consumption_rate_dn_ctr = 0;
    screen->reset_output = false;
    screen->autoscale = autoscale;
    screen->stereo = stereo;
    screen->spectrum_smooth_look = spectrum_smooth_look;
    screen->spectrum_smooth_look_legacy_chars =
        spectrum_smooth_look_legacy_chars;
    screen->spectrum_log_scale_x = spectrum_log_scale_x;
    screen->spectrum_log_scale_y = spectrum_log_scale_y;
    screen->initialized = true;
    native_visualizer_screen_init_visualization(screen);
    return;
}

void
native_visualizer_screen_destroy(NativeVisualizerScreen *screen) {
    if ((screen == NULL) || !screen->initialized) {
        return;
    }

    native_visualizer_screen_close_data_source(screen);

#if defined(HAVE_FFTW3_H)
    visualizer_fft_destroy(screen);
#endif

    ncm_sample_buffer_destroy(&screen->right_channel);
    ncm_sample_buffer_destroy(&screen->left_channel);
    ncm_sample_buffer_destroy(&screen->rendered_samples);
    ncm_sample_buffer_destroy(&screen->buffered_samples);
    ncm_sample_buffer_destroy(&screen->incoming_samples);
    visualizer_destroy_colors(screen);
    ncm_buffer_destroy(&screen->visualizer_chars);
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
    native_visualizer_screen_init_visualization(screen);
    return;
}

void
native_visualizer_screen_init_visualization(
    NativeVisualizerScreen *screen) {
    double samples_per_column;
    int64 rendered_samples;
    int64 incoming_samples;
    int64 width;
    int32 rendered_cap;
    int32 incoming_cap;
    int32 channel_cap;

    if ((screen == NULL) || !screen->initialized) {
        return;
    }

    width = nc_window_width(&screen->window);
    if (width <= 0) {
        width = 1;
    }

    rendered_samples = 0;
    switch (screen->visualization_type) {
    case NATIVE_VISUALIZER_WAVE:
    case NATIVE_VISUALIZER_WAVE_FILLED:
        samples_per_column = ceil(
            (double)screen->sample_rate / (double)screen->fps
            / (double)width);
        rendered_samples = (int64)samples_per_column*width*10;
        break;
#if defined(HAVE_FFTW3_H)
    case NATIVE_VISUALIZER_FREQUENCY:
        rendered_samples = screen->fft.dft_nonzero_size;
        break;
#endif
    case NATIVE_VISUALIZER_ELLIPSE:
        rendered_samples = screen->sample_rate / 30;
        break;
    case NATIVE_VISUALIZER_TYPE_LAST:
    default:
        screen->visualization_type = NATIVE_VISUALIZER_WAVE;
        samples_per_column = ceil(
            (double)screen->sample_rate / (double)screen->fps
            / (double)width);
        rendered_samples = (int64)samples_per_column*width*10;
        break;
    }

    incoming_samples = screen->sample_rate / 2;
    if (screen->stereo) {
        rendered_samples *= 2;
        incoming_samples *= 2;
    }
    if (rendered_samples < 1) {
        rendered_samples = 1;
    }
    if (incoming_samples < 1) {
        incoming_samples = 1;
    }
    if ((rendered_samples > INT32_MAX)
        || (incoming_samples > INT32_MAX)) {
        return;
    }

    rendered_cap = (int32)rendered_samples;
    incoming_cap = (int32)incoming_samples;
    channel_cap = 0;
    if (screen->stereo) {
        channel_cap = rendered_cap / 2;
    }

    ncm_sample_buffer_resize(&screen->incoming_samples, incoming_cap);
    ncm_sample_buffer_resize(&screen->buffered_samples, incoming_cap);
    ncm_sample_buffer_resize(&screen->rendered_samples, rendered_cap);
    ncm_sample_buffer_resize(&screen->left_channel, channel_cap);
    ncm_sample_buffer_resize(&screen->right_channel, channel_cap);
    memset64(screen->rendered_samples.data,
         0,
         rendered_cap*SIZEOF(*screen->rendered_samples.data));
    return;
}

void
native_visualizer_screen_clear(NativeVisualizerScreen *screen) {
    ncm_sample_buffer_clear(&screen->buffered_samples);
    ncm_sample_buffer_clear(&screen->rendered_samples);
    ncm_sample_buffer_clear(&screen->left_channel);
    ncm_sample_buffer_clear(&screen->right_channel);
    if (screen->rendered_samples.cap > 0) {
        memset64(screen->rendered_samples.data,
             0,
             screen->rendered_samples.cap
             *SIZEOF(*screen->rendered_samples.data));
    }
    nc_window_clear(&screen->window);
    native_visualizer_screen_drain_data_source(screen);
    return;
}

void
native_visualizer_screen_reset_auto_scale_multiplier(
    NativeVisualizerScreen *screen) {
    screen->auto_scale_multiplier = 1.0;
    return;
}

void
native_visualizer_screen_toggle_type(NativeVisualizerScreen *screen) {
    screen->visualization_type = native_visualizer_next_type(
        screen->visualization_type);
    native_visualizer_screen_init_visualization(screen);
    return;
}

int32
native_visualizer_screen_requested_samples(NativeVisualizerScreen *screen) {
    double rate;
    int32 result;

    if ((screen == NULL) || (screen->fps <= 0)) {
        return 0;
    }

    rate = (double)screen->sample_rate / (double)screen->fps;
    rate *= pow(1.1, (double)screen->sample_consumption_rate);
    if (screen->stereo) {
        rate *= 2.0;
    }
    if (!isfinite(rate) || (rate >= (double)INT32_MAX)) {
        return INT32_MAX;
    }

    result = (int32)rate;
    if (result < 1) {
        result = 1;
    }
    return result;
}

bool
native_visualizer_screen_push_samples(NativeVisualizerScreen *screen,
                                      int16 *samples,
                                      int32 samples_len) {
    if (screen == NULL) {
        return false;
    }
    if (samples_len <= 0) {
        return true;
    }
    if (samples == NULL) {
        return false;
    }

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

    if ((screen == NULL) || (dest == NULL) || (dest_len <= 0)) {
        return 0;
    }

    requested = native_visualizer_screen_requested_samples(screen);
    result = ncm_sample_buffer_get_clamped(&screen->buffered_samples,
                                           requested,
                                           dest,
                                           dest_len);
    if (result <= 0) {
        return 0;
    }

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

    if ((screen == NULL) || (samples == NULL) || (samples_len <= 1)) {
        return 0;
    }

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

void
native_visualizer_screen_apply_auto_scale(NativeVisualizerScreen *screen,
                                          int16 *samples,
                                          int32 samples_len) {
    double scale;
    int64 scaled;

    if ((screen == NULL) || !screen->autoscale
        || (samples == NULL) || (samples_len <= 0)) {
        return;
    }

    screen->auto_scale_multiplier += 1.0 / (double)screen->fps;
    for (int32 i = 0; i < samples_len; i += 1) {
        if (samples[i] == 0) {
            continue;
        }
        scale = fabs((double)NATIVE_VISUALIZER_MIN_SAMPLE
                     / (double)samples[i]);
        if (scale < screen->auto_scale_multiplier) {
            screen->auto_scale_multiplier = scale;
        }
    }

    if (screen->auto_scale_multiplier > 50.0) {
        return;
    }
    for (int32 i = 0; i < samples_len; i += 1) {
        scaled = (int64)((double)samples[i]
                         *screen->auto_scale_multiplier);
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

static void
visualizer_draw_wave(NativeVisualizerScreen *screen, int16 *samples,
                     int32 samples_len, int32 y_offset, int32 height) {
    char *character;
    int32 character_len;
    int32 width;
    int32 half_height;
    int32 base_y;
    int32 samples_per_column;
    int32 previous_y;

    width = (int32)nc_window_width(&screen->window);
    if ((samples == NULL) || (samples_len <= 0)
        || (width <= 0) || (height <= 0)) {
        return;
    }
    samples_per_column = samples_len / width;
    if (samples_per_column <= 0) {
        return;
    }

    character = screen->visualizer_chars.data + screen->point_char_offset;
    character_len = screen->point_char_len;
    half_height = height / 2;
    base_y = y_offset + half_height;
    previous_y = 0;
    for (int32 x = 0; x < width; x += 1) {
        int64 sum;
        int32 point_y;
        int32 half;

        sum = 0;
        for (int32 j = 0; j < samples_per_column; j += 1) {
            sum += samples[x*samples_per_column + j];
        }
        point_y = (int32)(sum / samples_per_column);
        point_y = (int32)((double)point_y*(double)height/65536.0);
        visualizer_draw_character(
            screen, x, base_y + point_y,
            visualizer_color(screen, point_y, half_height, false),
            false, character, character_len);

        if ((x > 0)
            && (((previous_y - point_y) > 1)
                || ((point_y - previous_y) > 1))) {
            half = (previous_y + point_y) / 2;
            if (previous_y < point_y) {
                for (int32 y = previous_y; y < point_y; y += 1) {
                    visualizer_draw_character(
                        screen, x - (y < half), base_y + y,
                        visualizer_color(screen, y, half_height, false),
                        false, character, character_len);
                }
            } else {
                for (int32 y = previous_y; y > point_y; y -= 1) {
                    visualizer_draw_character(
                        screen, x - (y > half), base_y + y,
                        visualizer_color(screen, y, half_height, false),
                        false, character, character_len);
                }
            }
        }
        previous_y = point_y;
    }
    return;
}

static void
visualizer_draw_wave_filled(NativeVisualizerScreen *screen,
                            int16 *samples, int32 samples_len,
                            int32 y_offset, int32 height) {
    char *character;
    int32 character_len;
    int32 width;
    int32 samples_per_column;
    bool flipped;

    width = (int32)nc_window_width(&screen->window);
    if ((samples == NULL) || (samples_len <= 0)
        || (width <= 0) || (height <= 0)) {
        return;
    }
    samples_per_column = samples_len / width;
    if (samples_per_column <= 0) {
        return;
    }

    character = screen->visualizer_chars.data + screen->bar_char_offset;
    character_len = screen->bar_char_len;
    flipped = y_offset > 0;
    for (int32 x = 0; x < width; x += 1) {
        int64 sum;
        int64 magnitude;
        int32 point_y;

        sum = 0;
        for (int32 j = 0; j < samples_per_column; j += 1) {
            sum += samples[x*samples_per_column + j];
        }
        magnitude = sum / samples_per_column;
        if (magnitude < 0) {
            magnitude = -magnitude;
        }
        point_y = (int32)((double)magnitude*(double)height/32768.0);
        if (point_y > height) {
            point_y = height;
        }

        for (int32 j = 0; j < point_y; j += 1) {
            int32 y;

            if (flipped) {
                y = y_offset + j;
            } else {
                y = y_offset + height - j - 1;
            }
            visualizer_draw_character(
                screen, x, y,
                visualizer_color(screen, j, height, false),
                false, character, character_len);
        }
    }
    return;
}

static void
visualizer_draw_ellipse(NativeVisualizerScreen *screen, int16 *samples,
                        int32 samples_len, int32 height) {
    char *character;
    int32 character_len;
    int32 width;
    int32 half_width;
    int32 half_height;
    double angle_multiplier;

    width = (int32)nc_window_width(&screen->window);
    if ((samples == NULL) || (samples_len <= 0)
        || (width <= 0) || (height <= 0)) {
        return;
    }

    character = screen->visualizer_chars.data + screen->point_char_offset;
    character_len = screen->point_char_len;
    half_width = width / 2;
    half_height = height / 2;
    angle_multiplier = 2.0*NATIVE_VISUALIZER_PI/(double)samples_len;
    for (int32 i = 0; i < samples_len; i += 1) {
        double angle;
        double max_radius;
        double radius;
        int32 x;
        int32 y;

        angle = (double)i*angle_multiplier;
        x = (int32)((double)half_width*cos(angle));
        y = (int32)((double)half_height*sin(angle));
        max_radius = sqrt((double)x*(double)x + (double)y*(double)y);
        radius = fabs((double)samples[i])/32768.0;
        x = (int32)((double)x*radius);
        y = (int32)((double)y*radius);
        visualizer_draw_character(
            screen, half_width + x, half_height + y,
            visualizer_color(screen,
                             sqrt((double)x*(double)x
                                  + (double)y*(double)y),
                             max_radius, false),
            false, character, character_len);
    }
    return;
}

static void
visualizer_draw_ellipse_stereo(NativeVisualizerScreen *screen,
                               int16 *left, int16 *right,
                               int32 samples_len, int32 half_height) {
    char *character;
    int32 character_len;
    int32 width;
    int32 height;
    int32 left_half_width;
    int32 right_half_width;
    int32 top_half_height;
    int32 bottom_half_height;
    int32 radius;

    width = (int32)nc_window_width(&screen->window);
    height = (int32)nc_window_height(&screen->window);
    if ((left == NULL) || (right == NULL) || (samples_len <= 0)
        || (width <= 0) || (height <= 0)) {
        return;
    }

    character = screen->visualizer_chars.data + screen->point_char_offset;
    character_len = screen->point_char_len;
    left_half_width = width / 2;
    right_half_width = width - left_half_width;
    top_half_height = half_height;
    bottom_half_height = height - half_height;
    radius = 2*screen->visualizer_colors_len;
    for (int32 i = 0; i < samples_len; i += 1) {
        double distance;
        int32 x;
        int32 y;

        if (left[i] < 0) {
            x = (int32)((double)left[i]/32768.0
                        *(double)left_half_width);
        } else {
            x = (int32)((double)left[i]/32768.0
                        *(double)right_half_width);
        }
        if (right[i] < 0) {
            y = (int32)((double)right[i]/32768.0
                        *(double)top_half_height);
        } else {
            y = (int32)((double)right[i]/32768.0
                        *(double)bottom_half_height);
        }
        distance = sqrt((double)x*(double)x
                        + 4.0*(double)y*(double)y);
        visualizer_draw_character(
            screen, left_half_width + x, top_half_height + y,
            visualizer_color(screen, distance, radius, true),
            false, character, character_len);
    }
    return;
}

#if defined(HAVE_FFTW3_H)
static void
visualizer_generate_frequency_space(NativeVisualizerScreen *screen) {
    NativeVisualizerFftState *fft;
    double left_bins_value;
    double scale;
    int32 left_bins;
    int32 width;

    fft = &screen->fft;
    width = (int32)nc_window_width(&screen->window);
    fft->dft_frequency_space_len = 0;
    if ((width <= 0) || (fft->hz_min <= 0.0)
        || (fft->hz_max <= fft->hz_min)) {
        return;
    }
    visualizer_fft_reserve_frequency_space(screen, width);

    if (screen->spectrum_log_scale_x) {
        double min_log;
        double max_log;
        double denominator;

        min_log = log10(fft->hz_min);
        max_log = log10(fft->hz_max);
        denominator = min_log - max_log;
        if (denominator == 0.0) {
            return;
        }
        left_bins_value = (min_log - (double)width*min_log)
                          /denominator;
        if (left_bins_value < 0.0) {
            left_bins_value = 0.0;
        }
        left_bins = (int32)left_bins_value;
        denominator = (double)left_bins + (double)width - 1.0;
        if (denominator <= 0.0) {
            fft->dft_frequency_space[0] = fft->hz_min;
            fft->dft_frequency_space_len = 1;
            return;
        }
        scale = max_log/denominator;
        for (int32 i = 0; i < width; i += 1) {
            fft->dft_frequency_space[i] = pow(
                10.0, (double)(left_bins + i)*scale);
        }
    } else {
        double denominator;

        denominator = fft->hz_min - fft->hz_max;
        if (denominator == 0.0) {
            return;
        }
        left_bins_value = (fft->hz_min
                           - (double)width*fft->hz_min)
                          /denominator;
        if (left_bins_value < 0.0) {
            left_bins_value = 0.0;
        }
        left_bins = (int32)left_bins_value;
        denominator = (double)left_bins + (double)width - 1.0;
        if (denominator <= 0.0) {
            fft->dft_frequency_space[0] = fft->hz_min;
            fft->dft_frequency_space_len = 1;
            return;
        }
        scale = fft->hz_max/denominator;
        for (int32 i = 0; i < width; i += 1) {
            fft->dft_frequency_space[i] =
                (double)(left_bins + i)*scale;
        }
    }
    fft->dft_frequency_space_len = width;
    return;
}

static void
visualizer_apply_fft_window(NativeVisualizerScreen *screen,
                            int16 *samples, int32 samples_len) {
    NativeVisualizerFftState *fft;
    double alpha;
    double a0;
    double a1;
    double a2;
    int32 used_samples;

    fft = &screen->fft;
    memset64(fft->input, 0,
         fft->dft_total_size*SIZEOF(*fft->input));
    used_samples = samples_len;
    if (used_samples > fft->dft_nonzero_size) {
        used_samples = fft->dft_nonzero_size;
    }
    if (used_samples > fft->dft_total_size) {
        used_samples = fft->dft_total_size;
    }
    if (used_samples <= 0) {
        return;
    }

    alpha = 0.16;
    a0 = (1.0 - alpha)/2.0;
    a1 = 0.5;
    a2 = alpha/2.0;
    for (int32 i = 0; i < used_samples; i += 1) {
        double window;
        double denominator;

        denominator = (double)(fft->dft_nonzero_size - 1);
        if (denominator <= 0.0) {
            denominator = 1.0;
        }
        window = a0
                 - a1*cos(2.0*NATIVE_VISUALIZER_PI*(double)i
                          /denominator)
                 + a2*cos(4.0*NATIVE_VISUALIZER_PI*(double)i
                          /denominator);
        fft->input[i] = window*(double)samples[i]
                        /(double)NATIVE_VISUALIZER_MAX_SAMPLE;
    }
    return;
}

static double
visualizer_bin_to_hz(NativeVisualizerScreen *screen, int32 bin) {
    return (double)bin*(double)screen->sample_rate
           /(double)screen->fft.dft_total_size;
}

static double
visualizer_interpolate_cubic(NativeVisualizerScreen *screen,
                             int32 x, int32 height_index) {
    NativeVisualizerFftState *fft;
    double x_next;
    double h_next;
    double delta;

    fft = &screen->fft;
    x_next = fft->bar_heights[height_index].column;
    h_next = fft->bar_heights[height_index].height;
    delta = 0.0;
    if (height_index == 0) {
        if (height_index < fft->bar_heights_len - 1) {
            double x_next2;
            double h_next2;

            x_next2 = fft->bar_heights[height_index + 1].column;
            h_next2 = fft->bar_heights[height_index + 1].height;
            if (x_next2 != x_next) {
                delta = (h_next2 - h_next)/(x_next2 - x_next);
            }
        }
        return h_next - delta*(x_next - x);
    }
    if (height_index == 1) {
        double x_previous;
        double h_previous;

        x_previous = fft->bar_heights[height_index - 1].column;
        h_previous = fft->bar_heights[height_index - 1].height;
        if (x_next != x_previous) {
            delta = (h_next - h_previous)/(x_next - x_previous);
        }
        return h_next - delta*(x_next - x);
    }
    if (height_index < fft->bar_heights_len - 1) {
        double x_previous2;
        double h_previous2;
        double x_previous;
        double h_previous;
        double x_next2;
        double h_next2;
        double m0;
        double m1;
        double t;
        double h00;
        double h10;
        double h01;
        double h11;

        x_previous2 = fft->bar_heights[height_index - 2].column;
        h_previous2 = fft->bar_heights[height_index - 2].height;
        x_previous = fft->bar_heights[height_index - 1].column;
        h_previous = fft->bar_heights[height_index - 1].height;
        x_next2 = fft->bar_heights[height_index + 1].column;
        h_next2 = fft->bar_heights[height_index + 1].height;
        if ((x_previous == x_previous2) || (x_next2 == x_next)
            || (x_next == x_previous)) {
            return h_next;
        }

        m0 = (h_previous - h_previous2)/(x_previous - x_previous2);
        m1 = (h_next2 - h_next)/(x_next2 - x_next);
        t = ((double)x - x_previous)/(x_next - x_previous);
        h00 = 2.0*t*t*t - 3.0*t*t + 1.0;
        h10 = t*t*t - 2.0*t*t + t;
        h01 = -2.0*t*t*t + 3.0*t*t;
        h11 = t*t*t - t*t;
        return h00*h_previous
               + h10*(x_next - x_previous)*m0
               + h01*h_next
               + h11*(x_next - x_previous)*m1;
    }
    return h_next;
}

static double
visualizer_interpolate_linear(NativeVisualizerScreen *screen,
                              int32 x, int32 height_index) {
    NativeVisualizerFftState *fft;
    double x_next;
    double h_next;
    double delta;

    fft = &screen->fft;
    x_next = fft->bar_heights[height_index].column;
    h_next = fft->bar_heights[height_index].height;
    delta = 0.0;
    if (height_index == 0) {
        if (fft->bar_heights_len > 1) {
            double x_next2;
            double h_next2;

            x_next2 = fft->bar_heights[height_index + 1].column;
            h_next2 = fft->bar_heights[height_index + 1].height;
            if (x_next2 != x_next) {
                delta = (h_next2 - h_next)/(x_next2 - x_next);
            }
        }
        return h_next - delta*(x_next - x);
    }
    if (height_index < fft->bar_heights_len) {
        double x_previous;
        double h_previous;
        double slope;

        x_previous = fft->bar_heights[height_index - 1].column;
        h_previous = fft->bar_heights[height_index - 1].height;
        if (x_next == x_previous) {
            return h_next;
        }
        slope = (h_next - h_previous)/(x_next - x_previous);
        return h_previous + slope*((double)x - x_previous);
    }
    return h_next;
}

static void
visualizer_draw_frequency(NativeVisualizerScreen *screen,
                          int16 *samples, int32 samples_len,
                          int32 y_offset, int32 height) {
    NativeVisualizerFftState *fft;
    int32 width;
    int32 current_bin;
    bool flipped;

    fft = &screen->fft;
    width = (int32)nc_window_width(&screen->window);
    if ((samples == NULL) || (samples_len <= 0)
        || (width <= 0) || (height <= 0) || (fft->plan == NULL)
        || (fft->input == NULL) || (fft->output == NULL)) {
        return;
    }

    flipped = y_offset > 0;
    visualizer_apply_fft_window(screen, samples, samples_len);
    fftw_execute(fft->plan);
    for (int32 i = 0; i < fft->results_len; i += 1) {
        double real;
        double imaginary;

        real = fft->output[i][0];
        imaginary = fft->output[i][1];
        fft->frequency_magnitudes[i] =
            sqrt(real*real + imaginary*imaginary)
            /(double)fft->dft_nonzero_size;
    }

    if (fft->dft_frequency_space_len != width) {
        visualizer_generate_frequency_space(screen);
    }
    if (fft->dft_frequency_space_len <= 0) {
        return;
    }
    visualizer_fft_reserve_bar_heights(screen, width);
    fft->bar_heights_len = 0;

    current_bin = 0;
    while ((current_bin < fft->results_len)
           && (visualizer_bin_to_hz(screen, current_bin)
               < fft->dft_frequency_space[0])) {
        current_bin += 1;
    }
    for (int32 x = 0; x < width; x += 1) {
        double bar_height;
        int32 count;

        bar_height = 0.0;
        count = 0;
        while ((current_bin < fft->results_len)
               && (visualizer_bin_to_hz(screen, current_bin)
                   < fft->dft_frequency_space[x])) {
            if ((x == 0)
                || (visualizer_bin_to_hz(screen, current_bin)
                    >= fft->dft_frequency_space[x - 1])) {
                bar_height += fft->frequency_magnitudes[current_bin];
                count += 1;
            }
            current_bin += 1;
        }
        if (count <= 0) {
            continue;
        }

        bar_height /= count;
        if (screen->spectrum_log_scale_y) {
            double dynamic_range;

            dynamic_range = fft->dynamic_range;
            if (dynamic_range == 0.0) {
                dynamic_range = 1.0;
            }
            if (bar_height > 0.0) {
                bar_height = (20.0*log10(bar_height)
                              + dynamic_range + fft->gain)
                             /dynamic_range;
            } else {
                bar_height = 0.0;
            }
        } else {
            bar_height *= pow(10.0, 1.8 + fft->gain/20.0);
            bar_height *= log2(2.0 + x)*80.0/(double)width;
            if (bar_height > 0.0) {
                bar_height = pow(bar_height, 0.65);
            }
        }
        if (bar_height > 0.0) {
            bar_height *= height;
        } else {
            bar_height = 0.0;
        }
        if (bar_height > height) {
            bar_height = height;
        }

        fft->bar_heights[fft->bar_heights_len].column = x;
        fft->bar_heights[fft->bar_heights_len].height = bar_height;
        fft->bar_heights_len += 1;
    }
    if (fft->bar_heights_len <= 0) {
        return;
    }

    {
        int32 height_index;

        height_index = 0;
        for (int32 x = 0; x < width; x += 1) {
            double h;
            int32 data_column;

            data_column = fft->bar_heights[height_index].column;
            if (x == data_column) {
                h = fft->bar_heights[height_index].height;
                if (height_index < fft->bar_heights_len - 1) {
                    height_index += 1;
                }
            } else if (screen->spectrum_log_scale_x) {
                h = visualizer_interpolate_cubic(screen, x,
                                                  height_index);
            } else {
                h = visualizer_interpolate_linear(screen, x,
                                                   height_index);
                if (h > height) {
                    h = height;
                }
            }
            if (h < 0.0) {
                h = 0.0;
            }
            if (h > height) {
                h = height;
            }

            for (int32 j = 0; (double)j < h; j += 1) {
                NcFormattedColor *color;
                char *character;
                int32 character_len;
                int32 y;
                bool reverse;

                if (flipped) {
                    y = y_offset + j;
                } else {
                    y = y_offset + height - j - 1;
                }
                color = visualizer_color(screen, j, height, false);
                character = screen->visualizer_chars.data
                            + screen->bar_char_offset;
                character_len = screen->bar_char_len;
                reverse = false;
                if (screen->spectrum_smooth_look) {
                    int32 index;

                    index = (int32)((int64)(
                        NATIVE_VISUALIZER_SMOOTH_CHAR_COUNT*h)
                        %NATIVE_VISUALIZER_SMOOTH_CHAR_COUNT);
                    if (((double)j < h - 1.0)
                        || (index
                            == NATIVE_VISUALIZER_SMOOTH_CHAR_COUNT - 1)) {
                        index = NATIVE_VISUALIZER_SMOOTH_CHAR_COUNT - 1;
                        character = visualizer_smooth_chars[index];
                        character_len = visualizer_smooth_char_lens[index];
                    } else if (flipped) {
                        if (screen->spectrum_smooth_look_legacy_chars) {
                            character =
                                visualizer_smooth_flipped_chars[index];
                            character_len =
                                visualizer_smooth_flipped_char_lens[index];
                        } else {
                            index = NATIVE_VISUALIZER_SMOOTH_CHAR_COUNT
                                    - index - 2;
                            character = visualizer_smooth_chars[index];
                            character_len =
                                visualizer_smooth_char_lens[index];
                            reverse = true;
                        }
                    } else {
                        character = visualizer_smooth_chars[index];
                        character_len = visualizer_smooth_char_lens[index];
                    }
                }
                visualizer_draw_character(screen, x, y, color, reverse,
                                          character, character_len);
            }
        }
    }
    return;
}
#endif

bool
native_visualizer_screen_draw(NativeVisualizerScreen *screen,
                              int16 *samples, int32 samples_len) {
    int32 height;
    int32 half_height;

    if ((screen == NULL) || !screen->initialized
        || (samples == NULL) || (samples_len <= 0)) {
        return false;
    }
    height = (int32)nc_window_height(&screen->window);
    if ((nc_window_width(&screen->window) <= 0) || (height <= 0)) {
        return false;
    }

    nc_window_clear(&screen->window);
    if (screen->stereo) {
        int32 channel_samples;

        channel_samples = native_visualizer_screen_split_stereo(
            screen, samples, samples_len);
        if (channel_samples <= 0) {
            return false;
        }
        half_height = height / 2;
        switch (screen->visualization_type) {
        case NATIVE_VISUALIZER_WAVE:
            visualizer_draw_wave(screen, screen->left_channel.data,
                                 channel_samples, 0, half_height);
            visualizer_draw_wave(screen, screen->right_channel.data,
                                 channel_samples, half_height,
                                 height - half_height);
            break;
        case NATIVE_VISUALIZER_WAVE_FILLED:
            visualizer_draw_wave_filled(
                screen, screen->left_channel.data, channel_samples,
                0, half_height);
            visualizer_draw_wave_filled(
                screen, screen->right_channel.data, channel_samples,
                half_height, height - half_height);
            break;
#if defined(HAVE_FFTW3_H)
        case NATIVE_VISUALIZER_FREQUENCY:
            visualizer_draw_frequency(
                screen, screen->left_channel.data, channel_samples,
                0, half_height);
            visualizer_draw_frequency(
                screen, screen->right_channel.data, channel_samples,
                half_height, height - half_height);
            break;
#endif
        case NATIVE_VISUALIZER_ELLIPSE:
            visualizer_draw_ellipse_stereo(
                screen, screen->left_channel.data,
                screen->right_channel.data, channel_samples,
                half_height);
            break;
        case NATIVE_VISUALIZER_TYPE_LAST:
        default:
            return false;
        }
        return true;
    }

    switch (screen->visualization_type) {
    case NATIVE_VISUALIZER_WAVE:
        visualizer_draw_wave(screen, samples, samples_len, 0, height);
        break;
    case NATIVE_VISUALIZER_WAVE_FILLED:
        visualizer_draw_wave_filled(screen, samples, samples_len,
                                    0, height);
        break;
#if defined(HAVE_FFTW3_H)
    case NATIVE_VISUALIZER_FREQUENCY:
        visualizer_draw_frequency(screen, samples, samples_len,
                                  0, height);
        break;
#endif
    case NATIVE_VISUALIZER_ELLIPSE:
        visualizer_draw_ellipse(screen, samples, samples_len, height);
        break;
    case NATIVE_VISUALIZER_TYPE_LAST:
    default:
        return false;
    }
    return true;
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

static bool
visualizer_system_disable_output(void *user, uint32 id,
                                 NcmError *error) {
    NcmMpdClient *client;

    client = user;
    if (client == NULL) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("MPD client is not configured"));
        return false;
    }
    return ncm_mpd_client_disable_output(client, id, error);
}

static bool
visualizer_system_enable_output(void *user, uint32 id,
                                NcmError *error) {
    NcmMpdClient *client;

    client = user;
    if (client == NULL) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("MPD client is not configured"));
        return false;
    }
    return ncm_mpd_client_enable_output(client, id, error);
}

static void
visualizer_system_sleep_microseconds(void *user, int32 microseconds) {
    (void)user;
    if (microseconds > 0) {
        usleep((useconds_t)microseconds);
    }
    return;
}

static bool
visualizer_reset_output(NativeVisualizerScreen *screen) {
    NcmError error;

    if (!screen->reset_output || (screen->output_id < 0)) {
        return true;
    }
    if ((screen->data_source_hooks.disable_output == NULL)
        || (screen->data_source_hooks.enable_output == NULL)) {
        return false;
    }

    ncm_error_clear(&error);
    if (!screen->data_source_hooks.disable_output(
            screen->data_source_hooks.user,
            (uint32)screen->output_id,
            &error)) {
        NcmStringFormatArg arg;

        arg = ncm_string_format_arg_cstring(error.message);
        ncm_statusbar_format(
            ncm_statusbar_message_delay_time(),
            STRLIT_ARGS("Could not disable visualizer output: %1"),
            &arg,
            1);
        return false;
    }
    if (screen->data_source_hooks.sleep_microseconds != NULL) {
        screen->data_source_hooks.sleep_microseconds(
            screen->data_source_hooks.user, 50000);
    }

    ncm_error_clear(&error);
    if (!screen->data_source_hooks.enable_output(
            screen->data_source_hooks.user,
            (uint32)screen->output_id,
            &error)) {
        NcmStringFormatArg arg;

        arg = ncm_string_format_arg_cstring(error.message);
        ncm_statusbar_format(
            ncm_statusbar_message_delay_time(),
            STRLIT_ARGS("Could not enable visualizer output: %1"),
            &arg,
            1);
        return false;
    }

    screen->reset_output = false;
    return true;
}

static int32
visualizer_read_samples(NativeVisualizerScreen *screen) {
    int64 buffer_size;
    int64 bytes_read;
    int32 samples_read;

    if ((screen->data_source_hooks.read_source == NULL)
        || (screen->incoming_samples.data == NULL)) {
        return 0;
    }
    buffer_size = screen->incoming_samples.cap
                  *SIZEOF(*screen->incoming_samples.data);
    if (buffer_size <= 0) {
        return 0;
    }

    bytes_read = screen->data_source_hooks.read_source(
        screen->data_source_hooks.user,
        screen->source_fd,
        screen->incoming_samples.data,
        buffer_size);
    if (bytes_read <= 0) {
        return 0;
    }
    if (bytes_read > buffer_size) {
        bytes_read = buffer_size;
    }

    samples_read = (int32)(bytes_read
                           / SIZEOF(*screen->incoming_samples.data));
    if (samples_read <= 0) {
        return 0;
    }
    if (!native_visualizer_screen_push_samples(
            screen, screen->incoming_samples.data, samples_read)) {
        return 0;
    }
    return samples_read;
}

static void
visualizer_prepare_drawing(NativeVisualizerScreen *screen) {
#if defined(HAVE_FFTW3_H)
    int64 width;

    width = nc_window_width(&screen->window);
    if ((width > 0) && (width <= INT32_MAX)) {
        visualizer_generate_frequency_space(screen);
        visualizer_fft_reserve_bar_heights(screen, (int32)width);
    }
#else
    (void)screen;
#endif
    return;
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
    NativeVisualizerScreen *visualizer;

    visualizer = visualizer_from_screen(screen);
    if (visualizer->source_fd < 0) {
        if (native_visualizer_screen_open_data_source(visualizer)) {
            (void)native_visualizer_screen_find_output_id(visualizer);
        }
    }
    native_visualizer_screen_clear(visualizer);
    visualizer->reset_output = true;
    ncm_title_draw_header(STRLIT_ARGS(NATIVE_VISUALIZER_TITLE));
    visualizer_prepare_drawing(visualizer);
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
    visualizer_prepare_drawing(visualizer);
    nc_screen_clear_resize_request(screen);
    return;
}

static int32
visualizer_window_timeout_callback(NcScreen *screen) {
    NativeVisualizerScreen *visualizer;

    visualizer = visualizer_from_screen(screen);
    if ((visualizer->source_fd >= 0)
        && (visualizer->fps > 0)
        && (ncm_status_state_player() == NCM_STATUS_PLAYER_PLAY)) {
        return 1000 / visualizer->fps;
    }
    return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
}

static char *
visualizer_title_callback(NcScreen *screen) {
    (void)screen;
    return (char *)NATIVE_VISUALIZER_TITLE;
}

static void
visualizer_update_callback(NcScreen *screen) {
    NativeVisualizerScreen *visualizer;
    int32 new_samples;

    visualizer = visualizer_from_screen(screen);
    if (visualizer->source_fd < 0) {
        return;
    }
    if (!visualizer_reset_output(visualizer)) {
        return;
    }

    visualizer_read_samples(visualizer);
    new_samples = native_visualizer_screen_take_render_samples(
        visualizer,
        visualizer->rendered_samples.data,
        visualizer->rendered_samples.cap);
    if (new_samples <= 0) {
        return;
    }

    if (!native_visualizer_screen_draw(
            visualizer,
            visualizer->rendered_samples.data,
            visualizer->rendered_samples.cap)) {
        return;
    }
    nc_window_refresh(&visualizer->window);
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
    return true;
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
