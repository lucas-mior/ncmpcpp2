#include <assert.h>
#include <stdlib.h>

#include "c/ncm_error.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"
#include "screens/nc_screen.h"
#include "screens/nc_visualizer.h"
#include "status.h"

#define TEST_SOURCE_FD 17
#define TEST_OUTPUT_ID 23
#define TEST_SAMPLE_CAP 64


typedef struct TestVisualizerIo {
    int16 samples[TEST_SAMPLE_CAP];
    int32 samples_len;

    int32 open_calls;
    int32 read_calls;
    int32 close_calls;
    int32 disable_calls;
    int32 enable_calls;
    int32 sleep_calls;
    int32 slept_microseconds;
} TestVisualizerIo;

static enum NcmStatusPlayerState test_player_state;
static int64 test_resize_x;
static int64 test_resize_width;
static int64 test_main_y;
static int64 test_main_height;
static int32 test_clear_calls;
static int32 test_refresh_calls;
static int32 test_draw_calls;
static int32 test_header_calls;
static char test_header[64];
static int32 test_header_len;
static int32 test_cursor_x;
static int32 test_cursor_y;

static NativeVisualizerScreenConfig test_config(TestVisualizerIo *io);
static int32 test_open_fifo(void *user, char *location,
                            int32 location_len);
static int64 test_read_source(void *user, int32 fd, void *buffer,
                              int64 buffer_size);
static void test_close_source(void *user, int32 fd);
static bool test_disable_output(void *user, uint32 id, NcmError *error);
static bool test_enable_output(void *user, uint32 id, NcmError *error);
static void test_sleep_microseconds(void *user, int32 microseconds);
static void test_reset_window_calls(void);
static void test_fill_samples(TestVisualizerIo *io, int32 samples_len);
static void test_callbacks(void);

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
__wrap_nc_window_refresh(NcWindow *window) {
    (void)window;
    test_refresh_calls += 1;
    return;
}

void
__wrap_nc_window_go_to_xy(NcWindow *window, int32 x, int32 y) {
    assert((x >= 0) && (x < window->width));
    assert((y >= 0) && (y < window->height));
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
    (void)format;
    return;
}

void
__wrap_nc_window_print_data(NcWindow *window, char *string,
                            int32 string_len) {
    (void)window;
    assert(test_cursor_x >= 0);
    assert(test_cursor_y >= 0);
    assert(string != NULL);
    assert(string_len > 0);
    test_draw_calls += 1;
    return;
}

void
__wrap_nc_screen_switcher_get_resize_params(NcScreen *screen,
                                             int64 *x_offset,
                                             int64 *width,
                                             bool adjust_locked_screen) {
    (void)screen;
    (void)adjust_locked_screen;
    *x_offset = test_resize_x;
    *width = test_resize_width;
    return;
}

int64
__wrap_ui_state_main_start_y(void) {
    return test_main_y;
}

int64
__wrap_ui_state_main_height(void) {
    return test_main_height;
}

void
__wrap_ncm_title_draw_header(char *title, int32 title_len) {
    assert(title_len < (int32)SIZEOF(test_header));
    memcpy64(test_header, title, title_len);
    test_header[title_len] = '\0';
    test_header_len = title_len;
    test_header_calls += 1;
    return;
}

enum NcmStatusPlayerState
__wrap_ncm_status_state_player(void) {
    return test_player_state;
}

static NativeVisualizerScreenConfig
test_config(TestVisualizerIo *io) {
    NativeVisualizerScreenConfig config = {0};

    config.source_location = "/tmp/visualizer.fifo";
    config.source_location_len = STRLIT_LEN("/tmp/visualizer.fifo");
    config.visualizer_chars = "xo";
    config.visualizer_chars_len = STRLIT_LEN("xo");
    config.fps = 50;
    config.spectrum_dft_size = 1;
    config.spectrum_gain = 10.0;
    config.spectrum_hz_min = 20.0;
    config.spectrum_hz_max = 20000.0;
    config.visualization_type = NATIVE_VISUALIZER_WAVE;
    config.data_source_hooks.open_fifo = test_open_fifo;
    config.data_source_hooks.read_source = test_read_source;
    config.data_source_hooks.close_source = test_close_source;
    config.data_source_hooks.disable_output = test_disable_output;
    config.data_source_hooks.enable_output = test_enable_output;
    config.data_source_hooks.sleep_microseconds = test_sleep_microseconds;
    config.data_source_hooks.user = io;
    return config;
}

static int32
test_open_fifo(void *user, char *location, int32 location_len) {
    TestVisualizerIo *io;

    io = user;
    assert(location_len == STRLIT_LEN("/tmp/visualizer.fifo"));
    assert(memcmp64(location, "/tmp/visualizer.fifo",
                location_len) == 0);
    io->open_calls += 1;
    return TEST_SOURCE_FD;
}

static int64
test_read_source(void *user, int32 fd, void *buffer,
                 int64 buffer_size) {
    TestVisualizerIo *io;
    int64 bytes;

    io = user;
    assert(fd == TEST_SOURCE_FD);
    io->read_calls += 1;
    if (io->samples_len <= 0) {
        return 0;
    }

    bytes = io->samples_len*SIZEOF(*io->samples);
    if (bytes > buffer_size) {
        bytes = buffer_size;
    }
    memcpy64(buffer, io->samples, bytes);
    io->samples_len = 0;
    return bytes;
}

static void
test_close_source(void *user, int32 fd) {
    TestVisualizerIo *io;

    io = user;
    assert(fd == TEST_SOURCE_FD);
    io->close_calls += 1;
    return;
}

static bool
test_disable_output(void *user, uint32 id, NcmError *error) {
    TestVisualizerIo *io;

    (void)error;
    io = user;
    assert(id == TEST_OUTPUT_ID);
    io->disable_calls += 1;
    return true;
}

static bool
test_enable_output(void *user, uint32 id, NcmError *error) {
    TestVisualizerIo *io;

    (void)error;
    io = user;
    assert(id == TEST_OUTPUT_ID);
    io->enable_calls += 1;
    return true;
}

static void
test_sleep_microseconds(void *user, int32 microseconds) {
    TestVisualizerIo *io;

    io = user;
    io->sleep_calls += 1;
    io->slept_microseconds = microseconds;
    return;
}

static void
test_reset_window_calls(void) {
    test_clear_calls = 0;
    test_refresh_calls = 0;
    test_draw_calls = 0;
    test_header_calls = 0;
    test_header[0] = '\0';
    test_header_len = 0;
    test_cursor_x = -1;
    test_cursor_y = -1;
    return;
}

static void
test_fill_samples(TestVisualizerIo *io, int32 samples_len) {
    assert((samples_len >= 0) && (samples_len <= TEST_SAMPLE_CAP));
    for (int32 i = 0; i < samples_len; i += 1) {
        io->samples[i] = (int16)(i*1000 - 16000);
    }
    io->samples_len = samples_len;
    return;
}

static void
test_callbacks(void) {
    NativeVisualizerScreen screen = {0};
    NativeVisualizerScreenConfig config;
    TestVisualizerIo io = {0};
    int16 buffered_samples[4] = {
        1,
        2,
        3,
        4,
    };

    test_resize_x = 7;
    test_resize_width = 30;
    test_main_y = 3;
    test_main_height = 12;
    test_player_state = NCM_STATUS_PLAYER_STOP;
    test_reset_window_calls();
    config = test_config(&io);
    native_visualizer_screen_init(&screen, 0, 0, 20, 8,
                                  nc_color_default(), nc_border_none(),
                                  &config);
    assert(native_visualizer_screen_open_data_source(&screen));
    assert(io.open_calls == 1);
    assert(native_visualizer_screen_push_samples(&screen,
                                                 buffered_samples,
                                                 NCM_ARRAY_LEN(
                                                     buffered_samples)));
    assert(screen.buffered_samples.len == NCM_ARRAY_LEN(buffered_samples));

    nc_screen_switch_to(native_visualizer_screen_base(&screen));
    assert(screen.buffered_samples.len == 0);
    assert(screen.rendered_samples.len == 0);
    assert(screen.reset_output);
    assert(test_clear_calls == 1);
    assert(test_header_calls == 1);
    assert(test_header_len == STRLIT_LEN("Music visualizer"));
    assert(memcmp64(test_header, "Music visualizer",
                test_header_len) == 0);
#if defined(HAVE_FFTW3_H)
    assert(screen.fft.dft_frequency_space_len == 20);
    assert(screen.fft.bar_heights_cap >= 20);
#endif
    assert(nc_screen_is_lockable(native_visualizer_screen_base(&screen)));
    assert(nc_screen_is_mergable(native_visualizer_screen_base(&screen)));

    nc_screen_request_resize(native_visualizer_screen_base(&screen));
    nc_screen_resize(native_visualizer_screen_base(&screen));
    assert(screen.window.start_x == test_resize_x);
    assert(screen.window.start_y == test_main_y);
    assert(screen.window.width == test_resize_width);
    assert(screen.window.height == test_main_height);
#if defined(HAVE_FFTW3_H)
    assert(screen.fft.dft_frequency_space_len == test_resize_width);
    assert(screen.fft.bar_heights_cap >= test_resize_width);
#endif
    assert(!nc_screen_has_to_be_resized(
        native_visualizer_screen_base(&screen)));

    test_player_state = NCM_STATUS_PLAYER_PLAY;
    assert(nc_screen_window_timeout(
        native_visualizer_screen_base(&screen)) == 20);
    test_player_state = NCM_STATUS_PLAYER_PAUSE;
    assert(nc_screen_window_timeout(
        native_visualizer_screen_base(&screen))
           == NC_SCREEN_DEFAULT_WINDOW_TIMEOUT);

    screen.output_id = TEST_OUTPUT_ID;
    test_fill_samples(&io, TEST_SAMPLE_CAP);
    test_reset_window_calls();
    nc_screen_update(native_visualizer_screen_base(&screen));
    assert(io.disable_calls == 1);
    assert(io.enable_calls == 1);
    assert(io.sleep_calls == 1);
    assert(io.slept_microseconds == 50000);
    assert(!screen.reset_output);
    assert(test_clear_calls == 1);
    assert(test_draw_calls > 0);
    assert(test_refresh_calls == 1);

    test_reset_window_calls();
    nc_screen_update(native_visualizer_screen_base(&screen));
    assert(test_clear_calls == 0);
    assert(test_draw_calls == 0);
    assert(test_refresh_calls == 0);

    native_visualizer_screen_close_data_source(&screen);
    assert(io.close_calls == 1);
    test_player_state = NCM_STATUS_PLAYER_PLAY;
    assert(nc_screen_window_timeout(
        native_visualizer_screen_base(&screen))
           == NC_SCREEN_DEFAULT_WINDOW_TIMEOUT);
    test_fill_samples(&io, TEST_SAMPLE_CAP);
    nc_screen_update(native_visualizer_screen_base(&screen));
    assert(io.samples_len == TEST_SAMPLE_CAP);

    native_visualizer_screen_destroy(&screen);
    return;
}

int
main(void) {
    test_callbacks();
    exit(EXIT_SUCCESS);
}
