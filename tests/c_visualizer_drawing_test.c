#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "c/ncm_base.h"
#include "cbase/base_macros.h"
#include "screens/nc_visualizer.h"

#define TEST_DRAW_CAPACITY 8192
#define TEST_CHARACTER_CAPACITY 8

typedef struct TestDraw {
    char character[TEST_CHARACTER_CAPACITY];
    int32 character_len;
    int32 x;
    int32 y;
} TestDraw;

static TestDraw test_draws[TEST_DRAW_CAPACITY];
static int32 test_draws_len;
static int32 test_cursor_x;
static int32 test_cursor_y;
static int32 test_clear_calls;
static int32 test_reverse_on_calls;
static int32 test_reverse_off_calls;

static void test_reset_draws(void);
static NativeVisualizerScreenConfig test_config(
    NcFormattedColor *colors, int32 colors_len);
static void test_assert_character(char *character, int32 character_len);
static void test_wave_drawing(void);
static void test_filled_wave_drawing(void);
static void test_ellipse_drawing(void);
static void test_stereo_ellipse_drawing(void);
#if defined(HAVE_FFTW3_H)
static void test_frequency_drawing(void);
#endif

void
__wrap_nc_window_init(NcWindow *window, int64 start_x, int64 start_y,
                      int64 width, int64 height, char *title,
                      int32 title_len, NcColor color, NcBorder border) {
    (void)title;
    (void)title_len;
    *window = (NcWindow){0};
    window->start_x = start_x;
    window->start_y = start_y;
    window->width = width;
    window->height = height;
    window->color = color;
    window->base_color = color;
    window->border = border;
    return;
}

void
__wrap_nc_window_destroy(NcWindow *window) {
    *window = (NcWindow){0};
    return;
}

int64
__wrap_nc_window_width(NcWindow *window) {
    return window->width;
}

int64
__wrap_nc_window_height(NcWindow *window) {
    return window->height;
}

void
__wrap_nc_window_resize(NcWindow *window, int64 width, int64 height) {
    window->width = width;
    window->height = height;
    return;
}

void
__wrap_nc_window_move_to(NcWindow *window, int64 x, int64 y) {
    window->start_x = x;
    window->start_y = y;
    return;
}

void
__wrap_nc_window_clear(NcWindow *window) {
    (void)window;
    test_clear_calls += 1;
    return;
}

void
__wrap_nc_window_go_to_xy(NcWindow *window, int32 x, int32 y) {
    assert(x >= 0);
    assert(y >= 0);
    assert(x < window->width);
    assert(y < window->height);
    test_cursor_x = x;
    test_cursor_y = y;
    return;
}

void
__wrap_nc_window_push_color(NcWindow *window, NcColor color) {
    window->color = color;
    return;
}

void
__wrap_nc_window_apply_format(NcWindow *window, enum NcFormat format) {
    (void)window;
    if (format == NC_FORMAT_REVERSE) {
        test_reverse_on_calls += 1;
    } else if (format == NC_FORMAT_NO_REVERSE) {
        test_reverse_off_calls += 1;
    }
    return;
}

void
__wrap_nc_window_print_data(NcWindow *window, char *string,
                            int32 string_len) {
    TestDraw *draw;

    (void)window;
    assert(test_draws_len < TEST_DRAW_CAPACITY);
    assert(string_len > 0);
    assert(string_len < TEST_CHARACTER_CAPACITY);
    draw = &test_draws[test_draws_len++];
    draw->x = test_cursor_x;
    draw->y = test_cursor_y;
    draw->character_len = string_len;
    memcpy(draw->character, string, (size_t)string_len);
    draw->character[string_len] = '\0';
    test_cursor_x += 1;
    return;
}

static void
test_reset_draws(void) {
    test_draws_len = 0;
    test_cursor_x = 0;
    test_cursor_y = 0;
    test_clear_calls = 0;
    test_reverse_on_calls = 0;
    test_reverse_off_calls = 0;
    memset(test_draws, 0, SIZEOF(test_draws));
    return;
}

static NativeVisualizerScreenConfig
test_config(NcFormattedColor *colors, int32 colors_len) {
    NativeVisualizerScreenConfig config = {0};

    config.source_location = (char *)"/tmp/test-visualizer.fifo";
    config.source_location_len = STRLIT_LEN("/tmp/test-visualizer.fifo");
    config.visualizer_chars = (char *)"xo";
    config.visualizer_chars_len = STRLIT_LEN("xo");
    config.visualizer_colors = colors;
    config.visualizer_colors_len = colors_len;
    config.fps = 60;
    config.spectrum_dft_size = 1;
    config.spectrum_gain = 10.0;
    config.spectrum_hz_min = 20.0;
    config.spectrum_hz_max = 20000.0;
    config.visualization_type = NATIVE_VISUALIZER_WAVE;
    config.spectrum_smooth_look = true;
    config.spectrum_smooth_look_legacy_chars = true;
    config.spectrum_log_scale_x = true;
    config.spectrum_log_scale_y = true;
    return config;
}

static void
test_assert_character(char *character, int32 character_len) {
    for (int32 i = 0; i < test_draws_len; i += 1) {
        assert(test_draws[i].character_len == character_len);
        assert(memcmp(test_draws[i].character, character,
                      (size_t)character_len) == 0);
    }
    return;
}

static void
test_wave_drawing(void) {
    NcFormattedColor colors[2];
    NativeVisualizerScreen screen = {0};
    NativeVisualizerScreenConfig config;
    int16 samples[8] = {
        0,
        0,
        32767,
        32767,
        -32768,
        -32768,
        0,
        0,
    };

    nc_formatted_color_init_color(&colors[0],
                                  nc_color_make(1, 0, false, false));
    nc_formatted_color_init_color(&colors[1],
                                  nc_color_make(2, 0, false, false));
    config = test_config(colors, NCM_ARRAY_LEN(colors));
    native_visualizer_screen_init(&screen, 0, 0, 4, 4,
                                  nc_color_default(), nc_border_none(),
                                  &config);
    nc_formatted_color_destroy(&colors[0]);
    nc_formatted_color_destroy(&colors[1]);

    test_reset_draws();
    assert(native_visualizer_screen_draw(&screen, samples,
                                         NCM_ARRAY_LEN(samples)));
    assert(test_clear_calls == 1);
    assert(test_draws_len >= 4);
    test_assert_character((char *)"x", STRLIT_LEN("x"));
    native_visualizer_screen_destroy(&screen);
    return;
}

static void
test_filled_wave_drawing(void) {
    NativeVisualizerScreen screen = {0};
    NativeVisualizerScreenConfig config;
    int16 samples[4] = {
        32767,
        32767,
        32767,
        32767,
    };

    config = test_config(NULL, 0);
    config.visualization_type = NATIVE_VISUALIZER_WAVE_FILLED;
    native_visualizer_screen_init(&screen, 0, 0, 4, 4,
                                  nc_color_default(), nc_border_none(),
                                  &config);

    test_reset_draws();
    assert(native_visualizer_screen_draw(&screen, samples,
                                         NCM_ARRAY_LEN(samples)));
    assert(test_draws_len == 12);
    test_assert_character((char *)"o", STRLIT_LEN("o"));
    native_visualizer_screen_destroy(&screen);
    return;
}

static void
test_ellipse_drawing(void) {
    NativeVisualizerScreen screen = {0};
    NativeVisualizerScreenConfig config;
    int16 samples[4] = {
        32767,
        32767,
        32767,
        32767,
    };

    config = test_config(NULL, 0);
    config.visualization_type = NATIVE_VISUALIZER_ELLIPSE;
    native_visualizer_screen_init(&screen, 0, 0, 8, 6,
                                  nc_color_default(), nc_border_none(),
                                  &config);

    test_reset_draws();
    assert(native_visualizer_screen_draw(&screen, samples,
                                         NCM_ARRAY_LEN(samples)));
    assert(test_draws_len == 4);
    test_assert_character((char *)"x", STRLIT_LEN("x"));
    native_visualizer_screen_destroy(&screen);
    return;
}

static void
test_stereo_ellipse_drawing(void) {
    NativeVisualizerScreen screen = {0};
    NativeVisualizerScreenConfig config;
    int16 samples[8] = {
        -32768,
        0,
        0,
        -32768,
        32767,
        0,
        0,
        32767,
    };

    config = test_config(NULL, 0);
    config.visualization_type = NATIVE_VISUALIZER_ELLIPSE;
    config.stereo = true;
    native_visualizer_screen_init(&screen, 0, 0, 8, 6,
                                  nc_color_default(), nc_border_none(),
                                  &config);

    test_reset_draws();
    assert(native_visualizer_screen_draw(&screen, samples,
                                         NCM_ARRAY_LEN(samples)));
    assert(test_draws_len == 4);
    test_assert_character((char *)"x", STRLIT_LEN("x"));
    assert(native_visualizer_screen_left_len(&screen) == 4);
    assert(native_visualizer_screen_right_len(&screen) == 4);
    native_visualizer_screen_destroy(&screen);
    return;
}

#if defined(HAVE_FFTW3_H)
static void
test_frequency_drawing(void) {
    NativeVisualizerScreen screen = {0};
    NativeVisualizerScreenConfig config;
    int16 *samples;
    int32 samples_len;

    config = test_config(NULL, 0);
    config.visualization_type = NATIVE_VISUALIZER_FREQUENCY;
    config.stereo = true;
    config.spectrum_smooth_look_legacy_chars = false;
    native_visualizer_screen_init(&screen, 0, 0, 16, 8,
                                  nc_color_default(), nc_border_none(),
                                  &config);
    samples_len = 2*screen.fft.dft_nonzero_size;
    samples = ncm_malloc(samples_len*SIZEOF(*samples));
    for (int32 i = 0; i < samples_len / 2; i += 1) {
        double angle;
        int16 sample;

        angle = 2.0*NATIVE_VISUALIZER_PI*440.0*(double)i/44100.0;
        sample = (int16)(sin(angle)*30000.0);
        samples[2*i] = sample;
        samples[2*i + 1] = sample;
    }

    test_reset_draws();
    assert(native_visualizer_screen_draw(&screen, samples, samples_len));
    assert(screen.fft.dft_frequency_space_len == 16);
    assert(screen.fft.bar_heights_len > 0);
    assert(test_draws_len > 0);
    assert(test_reverse_on_calls > 0);
    assert(test_reverse_on_calls == test_reverse_off_calls);

    ncm_free(samples, samples_len*SIZEOF(*samples));
    native_visualizer_screen_destroy(&screen);
    return;
}
#endif

int
main(void) {
    test_wave_drawing();
    test_filled_wave_drawing();
    test_ellipse_drawing();
    test_stereo_ellipse_drawing();
#if defined(HAVE_FFTW3_H)
    test_frequency_drawing();
#endif
    exit(EXIT_SUCCESS);
}
