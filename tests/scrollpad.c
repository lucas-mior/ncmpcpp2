#if !defined(NCMPCPP_TESTS_SCROLLPAD_C)
#define NCMPCPP_TESTS_SCROLLPAD_C

#define CBASE_IMPLEMENT
#include "cbase.h"

#include "curses/nc_scrollpad.h"

static int32 scrollpad_test_x;
static int32 scrollpad_test_y;

int32
prefresh(WINDOW *pad, int32 pminrow, int32 pmincol,
         int32 sminrow, int32 smincol,
         int32 smaxrow, int32 smaxcol) {
    (void)pad;
    (void)pminrow;
    (void)pmincol;
    (void)sminrow;
    (void)smincol;
    (void)smaxrow;
    (void)smaxcol;
    return 0;
}

int32
werase(WINDOW *win) {
    (void)win;
    return 0;
}

int32
wclrtoeol(WINDOW *win) {
    (void)win;
    return 0;
}

int32
mvwhline(WINDOW *win, int32 y, int32 x, chtype ch, int32 n) {
    (void)win;
    (void)y;
    (void)x;
    (void)ch;
    (void)n;
    return 0;
}

NcColor
nc_color_default(void) {
    NcColor color;

    color.foreground = 0;
    color.background = 0;
    color.is_default = true;
    color.is_end = false;
    return color;
}

NcColor
nc_color_end(void) {
    NcColor color;

    color.foreground = 0;
    color.background = 0;
    color.is_default = false;
    color.is_end = true;
    return color;
}

bool
nc_color_is_default(NcColor color) {
    return color.is_default;
}

void
nc_window_adjust_dimensions(NcWindow *window,
                            int32 width, int32 height) {
    window->width = width;
    window->height = height;
    return;
}

void
nc_window_recreate(NcWindow *window, int32 width, int32 height) {
    (void)window;
    (void)width;
    (void)height;
    scrollpad_test_x = 0;
    scrollpad_test_y = 0;
    return;
}

void
nc_window_apply_format(NcWindow *window, enum NcFormat format) {
    (void)window;
    (void)format;
    return;
}

void
nc_window_push_color(NcWindow *window, NcColor color) {
    (void)window;
    (void)color;
    return;
}

void
nc_window_print_char(NcWindow *window, char ch) {
    if (ch == '\n') {
        scrollpad_test_x = 0;
        scrollpad_test_y += 1;
        return;
    }

    if (scrollpad_test_x >= window->width) {
        scrollpad_test_x = 0;
        scrollpad_test_y += 1;
    }
    scrollpad_test_x += 1;
    return;
}

void
nc_window_go_to_xy(NcWindow *window, int32 x, int32 y) {
    (void)window;
    scrollpad_test_x = x;
    scrollpad_test_y = y;
    return;
}

int32
nc_window_get_x(NcWindow *window) {
    (void)window;
    return scrollpad_test_x;
}

int32
nc_window_get_y(NcWindow *window) {
    (void)window;
    return scrollpad_test_y;
}

#include "curses/nc_formatted_color.c"
#include "curses/nc_buffer.c"
#include "curses/nc_scrollpad.c"

static void
scrollpad_test_init_buffer(NcBuffer *buffer, char *data, int32 data_len) {
    nc_buffer_init(buffer);
    nc_buffer_append_data(buffer, data, data_len);
    return;
}

static void
scrollpad_test_short_lines(void) {
    NcBuffer buffer;
    char data[] = "one\ntwo\nthree";

    scrollpad_test_init_buffer(&buffer, data, strlen32(data));
    ASSERT_ZERO(nc_scrollpad_buffer_position_row(&buffer, 20, 0));
    ASSERT_EQUAL(nc_scrollpad_buffer_position_row(&buffer, 20, 4), 1);
    ASSERT_EQUAL(nc_scrollpad_buffer_position_row(&buffer, 20, 8), 2);
    ASSERT_EQUAL(nc_scrollpad_buffer_position_row(&buffer, 20, 13), 2);
    nc_buffer_destroy(&buffer);
    return;
}

static void
scrollpad_test_wraps_words(void) {
    NcBuffer buffer;
    char data[] = "one three";

    scrollpad_test_init_buffer(&buffer, data, strlen32(data));
    ASSERT_ZERO(nc_scrollpad_buffer_position_row(&buffer, 5, 0));
    ASSERT_ZERO(nc_scrollpad_buffer_position_row(&buffer, 5, 3));
    ASSERT_EQUAL(nc_scrollpad_buffer_position_row(&buffer, 5, 4), 1);
    ASSERT_EQUAL(nc_scrollpad_buffer_position_row(&buffer, 5, 8), 1);
    nc_buffer_destroy(&buffer);
    return;
}

static void
scrollpad_test_wraps_long_words(void) {
    NcBuffer buffer;
    char data[] = "abcdef";

    scrollpad_test_init_buffer(&buffer, data, strlen32(data));
    ASSERT_ZERO(nc_scrollpad_buffer_position_row(&buffer, 4, 0));
    ASSERT_ZERO(nc_scrollpad_buffer_position_row(&buffer, 4, 3));
    ASSERT_EQUAL(nc_scrollpad_buffer_position_row(&buffer, 4, 4), 1);
    ASSERT_EQUAL(nc_scrollpad_buffer_position_row(&buffer, 4, 5), 1);
    nc_buffer_destroy(&buffer);
    return;
}

static void
scrollpad_test_empty_lines(void) {
    NcBuffer buffer;
    char data[] = "\n\nx";

    scrollpad_test_init_buffer(&buffer, data, strlen32(data));
    ASSERT_ZERO(nc_scrollpad_buffer_position_row(&buffer, 10, 0));
    ASSERT_EQUAL(nc_scrollpad_buffer_position_row(&buffer, 10, 1), 1);
    ASSERT_EQUAL(nc_scrollpad_buffer_position_row(&buffer, 10, 2), 2);
    ASSERT_EQUAL(nc_scrollpad_buffer_position_row(&buffer, 10, 3), 2);
    nc_buffer_destroy(&buffer);
    return;
}

static void
scrollpad_test_narrow_window(void) {
    NcBuffer buffer;
    char data[] = "a bc";

    scrollpad_test_init_buffer(&buffer, data, strlen32(data));
    ASSERT_ZERO(nc_scrollpad_buffer_position_row(&buffer, 1, 0));
    ASSERT_EQUAL(nc_scrollpad_buffer_position_row(&buffer, 1, 1), 1);
    ASSERT_EQUAL(nc_scrollpad_buffer_position_row(&buffer, 1, 2), 2);
    ASSERT_EQUAL(nc_scrollpad_buffer_position_row(&buffer, 1, 3), 3);
    nc_buffer_destroy(&buffer);
    return;
}

static void
scrollpad_test_buffer_height(void) {
    NcBuffer buffer;
    char data[] = "a bc";

    scrollpad_test_init_buffer(&buffer, data, strlen32(data));
    ASSERT_EQUAL(nc_scrollpad_buffer_height(&buffer, 1), 4);
    ASSERT_EQUAL(nc_scrollpad_buffer_height(&buffer, 10), 1);
    ASSERT_EQUAL(nc_scrollpad_buffer_height(&buffer, 0), 1);
    nc_buffer_destroy(&buffer);
    return;
}

static void
scrollpad_test_center_on_buffer_position(void) {
    NcBuffer buffer;
    NcScrollpad scrollpad;
    NcWindow window = {0};
    char data[] = "zero\none\ntwo\nthree\nfour\nfive";

    scrollpad_test_init_buffer(&buffer, data, strlen32(data));
    nc_scrollpad_init(&scrollpad, 3);
    window.width = 20;
    window.height = 3;

    nc_scrollpad_center_on_buffer_position(&scrollpad, &window, &buffer, 0);
    ASSERT_ZERO(scrollpad.beginning);
    ASSERT_EQUAL(scrollpad.real_height, 6);

    nc_scrollpad_center_on_buffer_position(&scrollpad, &window, &buffer, 13);
    ASSERT_EQUAL(scrollpad.beginning, 2);

    nc_scrollpad_center_on_buffer_position(&scrollpad, &window, &buffer, 24);
    ASSERT_EQUAL(scrollpad.beginning, 3);
    ASSERT_EQUAL(nc_scrollpad_max_beginning(&scrollpad, &window), 3);

    nc_buffer_destroy(&buffer);
    return;
}

static void
scrollpad_test_clamps_positions(void) {
    NcBuffer buffer;
    char data[] = "abc";

    scrollpad_test_init_buffer(&buffer, data, strlen32(data));
    ASSERT_ZERO(nc_scrollpad_buffer_position_row(&buffer, 10, -10));
    ASSERT_ZERO(nc_scrollpad_buffer_position_row(&buffer, 10, 100));
    ASSERT_ZERO(nc_scrollpad_buffer_position_row(&buffer, 0, 0));
    nc_buffer_destroy(&buffer);
    return;
}

int
main(void) {
    scrollpad_test_short_lines();
    scrollpad_test_wraps_words();
    scrollpad_test_wraps_long_words();
    scrollpad_test_empty_lines();
    scrollpad_test_narrow_window();
    scrollpad_test_buffer_height();
    scrollpad_test_center_on_buffer_position();
    scrollpad_test_clamps_positions();
    exit(EXIT_SUCCESS);
}

#endif /* NCMPCPP_TESTS_SCROLLPAD_C */
