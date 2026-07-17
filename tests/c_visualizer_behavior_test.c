#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "c/ncm_error.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"
#include "screens/nc_screen.h"
#include "screens/nc_visualizer.h"
#include "status.h"
#include "statusbar.h"

#define TEST_FIFO_FD 41
#define TEST_UDP_FD 42
#define TEST_OUTPUT_ID 77
#define TEST_SAMPLE_CAP 512
#define TEST_READ_RESULT_CAP 8

enum TestOutputMode {
    TEST_OUTPUT_MATCH,
    TEST_OUTPUT_MISSING,
    TEST_OUTPUT_ERROR,
};

typedef struct TestVisualizerIo {
    int16 samples[TEST_SAMPLE_CAP];
    int64 read_results[TEST_READ_RESULT_CAP];

    int32 samples_len;
    int32 read_results_len;
    int32 read_result_index;
    int32 fifo_fd;
    int32 udp_fd;
    int32 open_fifo_calls;
    int32 open_udp_calls;
    int32 read_calls;
    int32 close_calls;
    int32 close_fd;
    int32 get_outputs_calls;
    int32 disable_calls;
    int32 enable_calls;
    int32 sleep_calls;
    int32 slept_microseconds;

    enum TestOutputMode output_mode;
    bool disable_ok;
    bool enable_ok;
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
static int32 test_statusbar_calls;
static int32 test_cursor_x;
static int32 test_cursor_y;
static char test_header[64];
static int32 test_header_len;

static void test_io_init(TestVisualizerIo *io);
static NativeVisualizerScreenConfig test_config(TestVisualizerIo *io,
                                                 char *source,
                                                 int32 source_len,
                                                 char *output,
                                                 int32 output_len,
                                                 bool stereo);
static int32 test_open_fifo(void *user, char *location,
                            int32 location_len);
static int32 test_open_udp(void *user, char *location,
                           int32 location_len, char *port,
                           int32 port_len);
static int64 test_read_source(void *user, int32 fd, void *buffer,
                              int64 buffer_size);
static void test_close_source(void *user, int32 fd);
static bool test_get_outputs(void *user, NcmMpdOutputList *outputs,
                             NcmError *error);
static bool test_disable_output(void *user, uint32 id,
                                NcmError *error);
static bool test_enable_output(void *user, uint32 id,
                               NcmError *error);
static void test_sleep_microseconds(void *user, int32 microseconds);
static void test_reset_ui(void);
static void test_set_read_results(TestVisualizerIo *io,
                                  int64 *results, int32 results_len);
static void test_fill_samples(TestVisualizerIo *io, int32 samples_len);
static void test_fifo_session_behavior(void);
static void test_switch_opens_data_source(void);
static void test_udp_stereo_session_behavior(void);
static void test_output_lookup_errors(void);
static void test_output_reset_errors(void);
static void test_open_failure_and_closed_behavior(void);
static void test_resize_rebuilds_rendering_state(void);

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

int32
__wrap_ncm_statusbar_message_delay_time(void) {
    return 5;
}

void
__wrap_ncm_statusbar_format(int32 delay_seconds, char *format,
                            int32 format_len,
                            NcmStringFormatArg *args,
                            int32 args_len) {
    (void)delay_seconds;
    (void)format;
    (void)format_len;
    (void)args;
    (void)args_len;
    test_statusbar_calls += 1;
    return;
}

static void
test_io_init(TestVisualizerIo *io) {
    *io = (TestVisualizerIo){0};
    io->fifo_fd = TEST_FIFO_FD;
    io->udp_fd = TEST_UDP_FD;
    io->close_fd = -1;
    io->output_mode = TEST_OUTPUT_MATCH;
    io->disable_ok = true;
    io->enable_ok = true;
    return;
}

static NativeVisualizerScreenConfig
test_config(TestVisualizerIo *io, char *source, int32 source_len,
            char *output, int32 output_len, bool stereo) {
    NativeVisualizerScreenConfig config = {0};

    config.source_location = source;
    config.output_name = output;
    config.visualizer_chars = "xo";
    config.source_location_len = source_len;
    config.output_name_len = output_len;
    config.visualizer_chars_len = STRLIT_LEN("xo");
    config.fps = 50;
    config.spectrum_dft_size = 1;
    config.spectrum_gain = 10.0;
    config.spectrum_hz_min = 20.0;
    config.spectrum_hz_max = 20000.0;
    config.visualization_type = NATIVE_VISUALIZER_WAVE;
    config.stereo = stereo;
    config.data_source_hooks.open_fifo = test_open_fifo;
    config.data_source_hooks.open_udp = test_open_udp;
    config.data_source_hooks.read_source = test_read_source;
    config.data_source_hooks.close_source = test_close_source;
    config.data_source_hooks.get_outputs = test_get_outputs;
    config.data_source_hooks.disable_output = test_disable_output;
    config.data_source_hooks.enable_output = test_enable_output;
    config.data_source_hooks.sleep_microseconds =
        test_sleep_microseconds;
    config.data_source_hooks.user = io;
    return config;
}

static int32
test_open_fifo(void *user, char *location, int32 location_len) {
    TestVisualizerIo *io;

    io = user;
    assert(location != NULL);
    assert(location_len >= 0);
    io->open_fifo_calls += 1;
    return io->fifo_fd;
}

static int32
test_open_udp(void *user, char *location, int32 location_len,
              char *port, int32 port_len) {
    TestVisualizerIo *io;

    io = user;
    assert(location != NULL);
    assert(location_len > 0);
    assert(port != NULL);
    assert(port_len > 0);
    io->open_udp_calls += 1;
    return io->udp_fd;
}

static int64
test_read_source(void *user, int32 fd, void *buffer,
                 int64 buffer_size) {
    TestVisualizerIo *io;
    int64 result;
    int64 copy_bytes;
    int64 sample_bytes;

    io = user;
    assert((fd == TEST_FIFO_FD) || (fd == TEST_UDP_FD));
    io->read_calls += 1;
    if (io->read_result_index >= io->read_results_len) {
        return 0;
    }

    result = io->read_results[io->read_result_index];
    io->read_result_index += 1;
    if (result <= 0) {
        return result;
    }

    copy_bytes = result;
    if (copy_bytes > buffer_size) {
        copy_bytes = buffer_size;
    }
    memset64(buffer, 0, copy_bytes);
    sample_bytes = io->samples_len*SIZEOF(*io->samples);
    if (sample_bytes > copy_bytes) {
        sample_bytes = copy_bytes;
    }
    if (sample_bytes > 0) {
        memcpy64(buffer, io->samples, sample_bytes);
    }
    return result;
}

static void
test_close_source(void *user, int32 fd) {
    TestVisualizerIo *io;

    io = user;
    io->close_calls += 1;
    io->close_fd = fd;
    return;
}

static bool
test_get_outputs(void *user, NcmMpdOutputList *outputs,
                 NcmError *error) {
    TestVisualizerIo *io;
    NcmMpdOutput output;

    io = user;
    io->get_outputs_calls += 1;
    if (io->output_mode == TEST_OUTPUT_ERROR) {
        ncm_error_set(error, 1, STRLIT_ARGS("output failure"));
        return false;
    }

    ncm_mpd_output_init(&output);
    assert(ncm_mpd_output_set(&output, 3,
                              STRLIT_ARGS("Speakers"), true));
    assert(ncm_mpd_output_list_append_copy(outputs, &output));
    if (io->output_mode == TEST_OUTPUT_MATCH) {
        assert(ncm_mpd_output_set(&output, TEST_OUTPUT_ID,
                                  STRLIT_ARGS("Visualizer feed"),
                                  true));
        assert(ncm_mpd_output_list_append_copy(outputs, &output));
    }
    ncm_mpd_output_destroy(&output);
    return true;
}

static bool
test_disable_output(void *user, uint32 id, NcmError *error) {
    TestVisualizerIo *io;

    io = user;
    assert(id == TEST_OUTPUT_ID);
    io->disable_calls += 1;
    if (!io->disable_ok) {
        ncm_error_set(error, 2, STRLIT_ARGS("disable failure"));
        return false;
    }
    return true;
}

static bool
test_enable_output(void *user, uint32 id, NcmError *error) {
    TestVisualizerIo *io;

    io = user;
    assert(id == TEST_OUTPUT_ID);
    io->enable_calls += 1;
    if (!io->enable_ok) {
        ncm_error_set(error, 3, STRLIT_ARGS("enable failure"));
        return false;
    }
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
test_reset_ui(void) {
    test_clear_calls = 0;
    test_refresh_calls = 0;
    test_draw_calls = 0;
    test_header_calls = 0;
    test_statusbar_calls = 0;
    test_cursor_x = -1;
    test_cursor_y = -1;
    test_header[0] = '\0';
    test_header_len = 0;
    return;
}

static void
test_set_read_results(TestVisualizerIo *io, int64 *results,
                      int32 results_len) {
    assert((results_len >= 0)
           && (results_len <= TEST_READ_RESULT_CAP));
    for (int32 i = 0; i < results_len; i += 1) {
        io->read_results[i] = results[i];
    }
    io->read_results_len = results_len;
    io->read_result_index = 0;
    return;
}

static void
test_fill_samples(TestVisualizerIo *io, int32 samples_len) {
    assert((samples_len >= 0) && (samples_len <= TEST_SAMPLE_CAP));
    for (int32 i = 0; i < samples_len; i += 1) {
        io->samples[i] = (int16)(i*257 - 16000);
    }
    io->samples_len = samples_len;
    return;
}

static void
test_fifo_session_behavior(void) {
    NativeVisualizerScreen screen = {0};
    TestVisualizerIo io;
    NativeVisualizerScreenConfig config;
    int16 old_samples[4] = {
        1,
        2,
        3,
        4,
    };
    int64 drain_results[2] = {
        8,
        0,
    };
    int64 update_results[1] = {
        128,
    };
    int32 read_calls;

    test_io_init(&io);
    test_reset_ui();
    test_player_state = NCM_STATUS_PLAYER_STOP;
    config = test_config(&io,
                         STRLIT_ARGS("/tmp/visualizer.fifo"),
                         STRLIT_ARGS("Visualizer feed"),
                         false);
    native_visualizer_screen_init(&screen, 0, 0, 20, 8,
                                  nc_color_default(), nc_border_none(),
                                  &config);

    assert(strcmp(nc_screen_title(
                      native_visualizer_screen_base(&screen)),
                  "Music visualizer") == 0);
    assert(native_visualizer_screen_find_output_id(&screen));
    assert(screen.output_id == TEST_OUTPUT_ID);
    assert(io.get_outputs_calls == 1);
    assert(native_visualizer_screen_open_data_source(&screen));
    assert(io.open_fifo_calls == 1);
    assert(io.open_udp_calls == 0);
    assert(native_visualizer_screen_push_samples(
        &screen, old_samples, NCM_ARRAY_LEN(old_samples)));
    test_set_read_results(&io, drain_results,
                          NCM_ARRAY_LEN(drain_results));

    nc_screen_switch_to(native_visualizer_screen_base(&screen));
    assert(screen.buffered_samples.len == 0);
    assert(screen.rendered_samples.len == 0);
    assert(screen.reset_output);
    assert(io.read_calls == 2);
    assert(test_clear_calls == 1);
    assert(test_header_calls == 1);
    assert(test_header_len == STRLIT_LEN("Music visualizer"));
    assert(strcmp(test_header, "Music visualizer") == 0);
    assert(nc_screen_is_lockable(
        native_visualizer_screen_base(&screen)));
    assert(nc_screen_is_mergable(
        native_visualizer_screen_base(&screen)));
    assert(nc_screen_window_timeout(
               native_visualizer_screen_base(&screen))
           == NC_SCREEN_DEFAULT_WINDOW_TIMEOUT);

    test_player_state = NCM_STATUS_PLAYER_PLAY;
    assert(nc_screen_window_timeout(
               native_visualizer_screen_base(&screen)) == 20);
    test_fill_samples(&io, 64);
    test_set_read_results(&io, update_results,
                          NCM_ARRAY_LEN(update_results));
    read_calls = io.read_calls;
    test_reset_ui();

    nc_screen_update(native_visualizer_screen_base(&screen));
    assert(io.disable_calls == 1);
    assert(io.enable_calls == 1);
    assert(io.sleep_calls == 1);
    assert(io.slept_microseconds == 50000);
    assert(io.read_calls == read_calls + 1);
    assert(!screen.reset_output);
    assert(test_clear_calls == 1);
    assert(test_draw_calls > 0);
    assert(test_refresh_calls == 1);

    test_reset_ui();
    nc_screen_update(native_visualizer_screen_base(&screen));
    assert(test_clear_calls == 0);
    assert(test_draw_calls == 0);
    assert(test_refresh_calls == 0);

    native_visualizer_screen_close_data_source(&screen);
    assert(io.close_calls == 1);
    assert(io.close_fd == TEST_FIFO_FD);
    assert(nc_screen_window_timeout(
               native_visualizer_screen_base(&screen))
           == NC_SCREEN_DEFAULT_WINDOW_TIMEOUT);
    native_visualizer_screen_destroy(&screen);
    assert(io.close_calls == 1);
    return;
}

static void
test_switch_opens_data_source(void) {
    NativeVisualizerScreen screen = {0};
    TestVisualizerIo io;
    NativeVisualizerScreenConfig config;

    test_io_init(&io);
    test_reset_ui();
    config = test_config(&io,
                         STRLIT_ARGS("/tmp/visualizer.fifo"),
                         STRLIT_ARGS("Visualizer feed"),
                         false);
    native_visualizer_screen_init(&screen, 0, 0, 20, 8,
                                  nc_color_default(), nc_border_none(),
                                  &config);

    assert(screen.source_fd < 0);
    assert(screen.output_id < 0);
    nc_screen_switch_to(native_visualizer_screen_base(&screen));
    assert(screen.source_fd == TEST_FIFO_FD);
    assert(screen.output_id == TEST_OUTPUT_ID);
    assert(io.open_fifo_calls == 1);
    assert(io.get_outputs_calls == 1);

    native_visualizer_screen_destroy(&screen);
    return;
}

static void
test_udp_stereo_session_behavior(void) {
    NativeVisualizerScreen screen = {0};
    TestVisualizerIo io;
    NativeVisualizerScreenConfig config;
    int64 update_results[1] = {
        256,
    };

    test_io_init(&io);
    test_reset_ui();
    test_player_state = NCM_STATUS_PLAYER_PLAY;
    config = test_config(&io,
                         STRLIT_ARGS("127.0.0.1:5555"),
                         STRLIT_ARGS("Visualizer feed"),
                         true);
    native_visualizer_screen_init(&screen, 0, 0, 24, 10,
                                  nc_color_default(), nc_border_none(),
                                  &config);

    assert(native_visualizer_screen_find_output_id(&screen));
    assert(screen.output_id == -1);
    assert(io.get_outputs_calls == 0);
    assert(native_visualizer_screen_open_data_source(&screen));
    assert(io.open_fifo_calls == 0);
    assert(io.open_udp_calls == 1);
    nc_screen_switch_to(native_visualizer_screen_base(&screen));

    test_fill_samples(&io, 128);
    test_set_read_results(&io, update_results,
                          NCM_ARRAY_LEN(update_results));
    test_reset_ui();
    nc_screen_update(native_visualizer_screen_base(&screen));

    assert(io.disable_calls == 0);
    assert(io.enable_calls == 0);
    assert(io.sleep_calls == 0);
    assert(screen.reset_output);
    assert(native_visualizer_screen_left_len(&screen)
           == screen.rendered_samples.cap / 2);
    assert(native_visualizer_screen_right_len(&screen)
           == screen.rendered_samples.cap / 2);
    assert(test_draw_calls > 0);
    assert(test_refresh_calls == 1);
    assert(nc_screen_window_timeout(
               native_visualizer_screen_base(&screen)) == 20);

    native_visualizer_screen_destroy(&screen);
    assert(io.close_calls == 1);
    assert(io.close_fd == TEST_UDP_FD);
    return;
}

static void
test_output_lookup_errors(void) {
    NativeVisualizerScreen screen = {0};
    TestVisualizerIo io;
    NativeVisualizerScreenConfig config;

    test_io_init(&io);
    test_reset_ui();
    config = test_config(&io,
                         STRLIT_ARGS("/tmp/visualizer.fifo"),
                         STRLIT_ARGS("Visualizer feed"),
                         false);
    native_visualizer_screen_init(&screen, 0, 0, 20, 8,
                                  nc_color_default(), nc_border_none(),
                                  &config);

    io.output_mode = TEST_OUTPUT_ERROR;
    assert(!native_visualizer_screen_find_output_id(&screen));
    assert(screen.output_id == -1);
    assert(io.get_outputs_calls == 1);
    assert(test_statusbar_calls == 1);

    io.output_mode = TEST_OUTPUT_MISSING;
    test_statusbar_calls = 0;
    assert(!native_visualizer_screen_find_output_id(&screen));
    assert(screen.output_id == -1);
    assert(io.get_outputs_calls == 2);
    assert(test_statusbar_calls == 1);

    ncm_buffer_clear(&screen.output_name);
    test_statusbar_calls = 0;
    assert(native_visualizer_screen_find_output_id(&screen));
    assert(screen.output_id == -1);
    assert(io.get_outputs_calls == 2);
    assert(test_statusbar_calls == 0);

    native_visualizer_screen_destroy(&screen);
    return;
}

static void
test_output_reset_errors(void) {
    NativeVisualizerScreen screen = {0};
    TestVisualizerIo io;
    NativeVisualizerScreenConfig config;
    int64 update_results[1] = {
        128,
    };
    int32 read_calls;

    test_io_init(&io);
    test_reset_ui();
    config = test_config(&io,
                         STRLIT_ARGS("/tmp/visualizer.fifo"),
                         STRLIT_ARGS("Visualizer feed"),
                         false);
    native_visualizer_screen_init(&screen, 0, 0, 20, 8,
                                  nc_color_default(), nc_border_none(),
                                  &config);
    assert(native_visualizer_screen_open_data_source(&screen));
    screen.output_id = TEST_OUTPUT_ID;
    nc_screen_switch_to(native_visualizer_screen_base(&screen));
    test_fill_samples(&io, 64);
    test_set_read_results(&io, update_results,
                          NCM_ARRAY_LEN(update_results));
    read_calls = io.read_calls;

    io.disable_ok = false;
    test_reset_ui();
    nc_screen_update(native_visualizer_screen_base(&screen));
    assert(screen.reset_output);
    assert(io.disable_calls == 1);
    assert(io.enable_calls == 0);
    assert(io.sleep_calls == 0);
    assert(io.read_calls == read_calls);
    assert(test_statusbar_calls == 1);
    assert(test_draw_calls == 0);
    assert(test_refresh_calls == 0);

    io.disable_ok = true;
    io.enable_ok = false;
    test_reset_ui();
    nc_screen_update(native_visualizer_screen_base(&screen));
    assert(screen.reset_output);
    assert(io.disable_calls == 2);
    assert(io.enable_calls == 1);
    assert(io.sleep_calls == 1);
    assert(io.read_calls == read_calls);
    assert(test_statusbar_calls == 1);
    assert(test_draw_calls == 0);
    assert(test_refresh_calls == 0);

    io.enable_ok = true;
    test_reset_ui();
    nc_screen_update(native_visualizer_screen_base(&screen));
    assert(!screen.reset_output);
    assert(io.disable_calls == 3);
    assert(io.enable_calls == 2);
    assert(io.sleep_calls == 2);
    assert(io.read_calls == read_calls + 1);
    assert(test_statusbar_calls == 0);
    assert(test_draw_calls > 0);
    assert(test_refresh_calls == 1);

    native_visualizer_screen_destroy(&screen);
    return;
}

static void
test_open_failure_and_closed_behavior(void) {
    NativeVisualizerScreen screen = {0};
    TestVisualizerIo io;
    NativeVisualizerScreenConfig config;
    int64 update_results[1] = {
        128,
    };

    test_io_init(&io);
    test_reset_ui();
    test_player_state = NCM_STATUS_PLAYER_PLAY;
    io.fifo_fd = -1;
    config = test_config(&io,
                         STRLIT_ARGS("/tmp/missing.fifo"),
                         NULL, 0, false);
    native_visualizer_screen_init(&screen, 0, 0, 20, 8,
                                  nc_color_default(), nc_border_none(),
                                  &config);

    assert(!native_visualizer_screen_open_data_source(&screen));
    assert(screen.source_fd == -1);
    assert(io.open_fifo_calls == 1);
    nc_screen_switch_to(native_visualizer_screen_base(&screen));
    assert(screen.source_fd == -1);
    assert(io.open_fifo_calls == 2);
    test_fill_samples(&io, 64);
    test_set_read_results(&io, update_results,
                          NCM_ARRAY_LEN(update_results));
    nc_screen_update(native_visualizer_screen_base(&screen));
    assert(io.read_calls == 0);
    assert(test_draw_calls == 0);
    assert(test_refresh_calls == 0);
    assert(nc_screen_window_timeout(
               native_visualizer_screen_base(&screen))
           == NC_SCREEN_DEFAULT_WINDOW_TIMEOUT);
    native_visualizer_screen_close_data_source(&screen);
    assert(io.close_calls == 0);

    native_visualizer_screen_destroy(&screen);
    return;
}

static void
test_resize_rebuilds_rendering_state(void) {
    NativeVisualizerScreen screen = {0};
    TestVisualizerIo io;
    NativeVisualizerScreenConfig config;

    test_io_init(&io);
    config = test_config(&io,
                         STRLIT_ARGS("/tmp/visualizer.fifo"),
                         NULL, 0, false);
    native_visualizer_screen_init(&screen, 0, 0, 20, 8,
                                  nc_color_default(), nc_border_none(),
                                  &config);
    test_resize_x = 7;
    test_resize_width = 31;
    test_main_y = 3;
    test_main_height = 11;

    nc_screen_request_resize(native_visualizer_screen_base(&screen));
    nc_screen_resize(native_visualizer_screen_base(&screen));
    assert(screen.window.start_x == test_resize_x);
    assert(screen.window.start_y == test_main_y);
    assert(screen.window.width == test_resize_width);
    assert(screen.window.height == test_main_height);
    assert(screen.rendered_samples.cap == 8990);
    assert(!nc_screen_has_to_be_resized(
        native_visualizer_screen_base(&screen)));
#if defined(HAVE_FFTW3_H)
    assert(screen.fft.dft_frequency_space_len == test_resize_width);
    assert(screen.fft.bar_heights_cap >= test_resize_width);
#endif

    native_visualizer_screen_set_stereo(&screen, true);
    assert(screen.rendered_samples.cap == 17980);
    assert(screen.left_channel.cap == 8990);
    assert(screen.right_channel.cap == 8990);
    native_visualizer_screen_set_type(&screen,
                                      NATIVE_VISUALIZER_ELLIPSE);
    assert(screen.rendered_samples.cap == 2940);
    assert(screen.left_channel.cap == 1470);
    assert(screen.right_channel.cap == 1470);

    native_visualizer_screen_destroy(&screen);
    return;
}

int
main(void) {
    test_fifo_session_behavior();
    test_switch_opens_data_source();
    test_udp_stereo_session_behavior();
    test_output_lookup_errors();
    test_output_reset_errors();
    test_open_failure_and_closed_behavior();
    test_resize_rebuilds_rendering_state();
    exit(EXIT_SUCCESS);
}
