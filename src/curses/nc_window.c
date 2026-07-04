#include "curses/nc_window.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#include "cbase/array.h"
#include "cbase/base_macros.h"
#include "cbase/cbase.h"

#define NC_COLOR_TRANSPARENT -1
#define NC_COLOR_CURRENT -2

static int32 max_color;
static int32 color_pair_counter;
static int32 *color_pair_map;
static bool mouse_support_enabled;
static struct termios orig_termios;

static void nc_window_assign_title(NcWindow *window,
                                   char *title, int32 title_len);
static void nc_window_copy_colors(NcColor **dest, NcColor *source);
static void nc_window_copy_keys(NcKey **dest, NcKey *source);
static void nc_window_copy_fd_callbacks(NcFdCallback **dest,
                                        NcFdCallback *source);
static int32 nc_i32(int64 value);
static bool nc_window_has_title(NcWindow *window);
static NcKey nc_window_get_input_char(NcWindow *window, int32 key);
static NcKey nc_window_define_mouse_event(NcWindow *window, int32 type);
static NcKey nc_xterm_modifier_key(int32 ch);
static int32 nc_window_parse_number(NcWindow *window, int32 *result);
static void nc_window_bold(NcWindow *window, bool state);
static void nc_window_underline(NcWindow *window, bool state);
static void nc_window_reverse(NcWindow *window, bool state);
static void nc_window_alt_charset(NcWindow *window, bool state);
static void nc_window_italic(NcWindow *window, bool state);
static void nc_window_increase_format(NcWindow *window, int32 *counter,
                                      void (*set)(NcWindow *, bool));
static void nc_window_decrease_format(NcWindow *window, int32 *counter,
                                      void (*set)(NcWindow *, bool));

NcColor
nc_color_make(int16 foreground, int16 background,
              bool is_default, bool is_end) {
    NcColor result;

    result.foreground = foreground;
    result.background = background;
    result.is_default = is_default;
    result.is_end = is_end;

    return result;
}

NcColor
nc_color_default(void) {
    return nc_color_make(0, 0, true, false);
}

NcColor
nc_color_end(void) {
    return nc_color_make(0, 0, false, true);
}

bool
nc_color_equal(NcColor left, NcColor right) {
    if (left.foreground != right.foreground) {
        return false;
    }
    if (left.background != right.background) {
        return false;
    }
    if (left.is_default != right.is_default) {
        return false;
    }
    if (left.is_end != right.is_end) {
        return false;
    }

    return true;
}

bool
nc_color_is_default(NcColor color) {
    return color.is_default;
}

bool
nc_color_is_end(NcColor color) {
    return color.is_end;
}

bool
nc_color_current_background(NcColor color) {
    return color.background == NC_COLOR_CURRENT;
}

int32
nc_color_pair_number(NcColor color) {
    int32 result;

    if (ARRAY_LEN(color_pair_map) == 0) {
        return 0;
    }

    result = 0;
    if (nc_color_is_end(color)) {
        return 0;
    }
    if (!nc_color_is_default(color)) {
        if (!nc_color_current_background(color)) {
            result = (color.background + 1) % nc_color_count();
        }
        result *= 256;
        result += color.foreground % nc_color_count();

        assert(result < ARRAY_LEN(color_pair_map));

        if (!color_pair_map[result]) {
            if (color_pair_counter >= COLOR_PAIRS) {
                result = 0;
            } else {
                init_pair((short)color_pair_counter,
                          (short)color.foreground,
                          (short)color.background);
                color_pair_map[result] = color_pair_counter;
                color_pair_counter += 1;
            }
        }
        result = color_pair_map[result];
    }

    return result;
}

void
nc_mouse_enable(void) {
    if (!mouse_support_enabled) {
        return;
    }

    printf("\033[?1001s");
    printf("\033[?1000h");
    printf("\033[?1015h");
    fflush(stdout);
    return;
}

void
nc_mouse_disable(void) {
    if (!mouse_support_enabled) {
        return;
    }

    printf("\033[?1015l");
    printf("\033[?1000l");
    printf("\033[?1001r");
    fflush(stdout);
    return;
}

void
nc_init_screen(bool enable_colors, bool enable_mouse) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    initscr();
    if (has_colors() && enable_colors) {
        start_color();
        use_default_colors();
        max_color = COLORS;
        if (max_color > 256) {
            max_color = 256;
        }

        ARRAY_FREE(color_pair_map);
        for (int32 i = 0; i < 256*256; i += 1) {
            ARRAY_PUSH(color_pair_map, 0);
        }

        color_pair_counter = 1;
        for (int32 fg = 0; fg < nc_color_count(); fg += 1) {
            init_pair((short)color_pair_counter, (short)fg, -1);
            color_pair_map[fg] = color_pair_counter;
            color_pair_counter += 1;
        }
    }
    raw();
    nonl();
    noecho();
    timeout(0);
    curs_set(0);

    mouse_support_enabled = enable_mouse;
    nc_mouse_enable();
    return;
}

int32
nc_color_count(void) {
    return max_color;
}

void
nc_pause_screen(void) {
    if (mouse_support_enabled) {
        nc_mouse_disable();
    }
    def_prog_mode();
    endwin();
    return;
}

void
nc_unpause_screen(void) {
    if (mouse_support_enabled) {
        nc_mouse_enable();
    }
    refresh();
    return;
}

void
nc_destroy_screen(void) {
    nc_mouse_disable();
    curs_set(1);
    endwin();
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    ARRAY_FREE(color_pair_map);
    return;
}

void
nc_window_init_empty(NcWindow *window) {
    *window = (NcWindow){0};
    window->window_timeout = -1;
    window->color = nc_color_default();
    window->base_color = nc_color_default();
    window->escape_terminal_sequences = true;
    return;
}

void
nc_window_init(NcWindow *window, int64 start_x, int64 start_y,
               int64 width, int64 height, char *title,
               int32 title_len, NcColor color, NcBorder border) {
    nc_window_init_empty(window);

    window->start_x = start_x;
    window->start_y = start_y;
    window->width = width;
    window->height = height;
    window->border = border;
    nc_window_assign_title(window, title, title_len);

    if (window->border.enabled) {
        window->start_x += 1;
        window->start_y += 1;
        window->width -= 2;
        window->height -= 2;
    }
    if (nc_window_has_title(window)) {
        window->start_y += 2;
        window->height -= 2;
    }

    window->window = newpad(nc_i32(window->height), nc_i32(window->width));
    wtimeout(window->window, 0);

    nc_window_set_base_color(window, color);
    nc_window_set_color(window, window->base_color);
    return;
}

void
nc_window_copy(NcWindow *dest, NcWindow *source) {
    *dest = *source;
    if (source->window != NULL) {
        dest->window = dupwin(source->window);
    }

    dest->title = NULL;
    dest->title_len = 0;
    dest->title_cap = 0;
    nc_window_assign_title(dest, source->title, source->title_len);

    dest->color_stack = NULL;
    dest->input_queue = NULL;
    dest->fd_callbacks = NULL;
    nc_window_copy_colors(&dest->color_stack, source->color_stack);
    nc_window_copy_keys(&dest->input_queue, source->input_queue);
    nc_window_copy_fd_callbacks(&dest->fd_callbacks, source->fd_callbacks);

    if (dest->window != NULL) {
        nc_window_set_color(dest, dest->color);
    }
    return;
}

void
nc_window_move(NcWindow *dest, NcWindow *source) {
    *dest = *source;
    nc_window_init_empty(source);
    return;
}

void
nc_window_swap(NcWindow *left, NcWindow *right) {
    NcWindow temp;

    temp = *left;
    *left = *right;
    *right = temp;
    return;
}

void
nc_window_destroy(NcWindow *window) {
    if (window->window != NULL) {
        delwin(window->window);
    }
    if (window->title != NULL) {
        cbase_free(window->title, window->title_cap);
    }
    ARRAY_FREE(window->color_stack);
    ARRAY_FREE(window->input_queue);
    ARRAY_FREE(window->fd_callbacks);
    nc_window_init_empty(window);
    return;
}

WINDOW *
nc_window_raw(NcWindow *window) {
    return window->window;
}

int64
nc_window_width(NcWindow *window) {
    if (window->border.enabled) {
        return window->width + 2;
    }

    return window->width;
}

int64
nc_window_height(NcWindow *window) {
    int64 height;

    height = window->height;
    if (window->border.enabled) {
        height += 2;
    }
    if (nc_window_has_title(window)) {
        height += 2;
    }

    return height;
}

int64
nc_window_start_x(NcWindow *window) {
    if (window->border.enabled) {
        return window->start_x - 1;
    }

    return window->start_x;
}

int64
nc_window_start_y(NcWindow *window) {
    int64 start_y;

    start_y = window->start_y;
    if (window->border.enabled) {
        start_y -= 1;
    }
    if (nc_window_has_title(window)) {
        start_y -= 2;
    }

    return start_y;
}

int32
nc_window_timeout(NcWindow *window) {
    return window->window_timeout;
}

char *
nc_window_title(NcWindow *window) {
    return window->title;
}

int32
nc_window_title_len(NcWindow *window) {
    return window->title_len;
}

NcColor
nc_window_color(NcWindow *window) {
    return window->color;
}

NcColor
nc_window_base_color(NcWindow *window) {
    return window->base_color;
}

NcBorder
nc_window_border(NcWindow *window) {
    return window->border;
}

MEVENT *
nc_window_mouse_event(NcWindow *window) {
    return &window->mouse_event;
}

void
nc_window_set_color(NcWindow *window, NcColor color) {
    if (nc_color_is_default(color)) {
        color = window->base_color;
    }
    if (!nc_color_equal(color, nc_color_default())) {
        assert(!nc_color_current_background(color));
        wcolor_set(window->window, (short)nc_color_pair_number(color), NULL);
    } else {
        wcolor_set(window->window,
                   (short)nc_color_pair_number(window->base_color), NULL);
    }
    window->color = color;
    return;
}

void
nc_window_set_base_color(NcWindow *window, NcColor color) {
    if (nc_color_current_background(color)) {
        window->base_color = nc_color_make(color.foreground,
                                           NC_COLOR_TRANSPARENT,
                                           color.is_default,
                                           color.is_end);
    } else {
        window->base_color = color;
    }
    return;
}

void
nc_window_set_border(NcWindow *window, NcBorder border) {
    if (!border.enabled && window->border.enabled) {
        window->start_x -= 1;
        window->start_y -= 1;
        window->height += 2;
        window->width += 2;
        nc_window_recreate(window, window->width, window->height);
    } else if (border.enabled && !window->border.enabled) {
        window->start_x += 1;
        window->start_y += 1;
        window->height -= 2;
        window->width -= 2;
        nc_window_recreate(window, window->width, window->height);
    }
    window->border = border;
    return;
}

void
nc_window_set_timeout(NcWindow *window, int32 timeout) {
    window->window_timeout = timeout;
    return;
}

void
nc_window_set_title(NcWindow *window, char *title, int32 title_len) {
    bool old_has_title;
    bool new_has_title;

    old_has_title = nc_window_has_title(window);
    new_has_title = title_len > 0;
    if (new_has_title && !old_has_title) {
        window->start_y += 2;
        window->height -= 2;
        nc_window_recreate(window, window->width, window->height);
    } else if (!new_has_title && old_has_title) {
        window->start_y -= 2;
        window->height += 2;
        nc_window_recreate(window, window->width, window->height);
    }
    nc_window_assign_title(window, title, title_len);
    return;
}

void
nc_window_set_escape_terminal_sequences(NcWindow *window, bool enabled) {
    window->escape_terminal_sequences = enabled;
    return;
}

void
nc_window_display(NcWindow *window) {
    nc_window_refresh_border(window);
    nc_window_refresh(window);
    return;
}

void
nc_window_refresh_border(NcWindow *window) {
    int32 start_x;
    int32 start_y;
    int32 width;
    int32 height;

    if (window->border.enabled) {
        start_x = nc_i32(nc_window_start_x(window));
        start_y = nc_i32(nc_window_start_y(window));
        width = nc_i32(nc_window_width(window));
        height = nc_i32(nc_window_height(window));
        color_set((short)nc_color_pair_number(window->border.color), NULL);
        attron(A_ALTCHARSET);
        mvaddch(start_y, start_x, 'l');
        mvaddch(start_y, start_x + width - 1, 'k');
        mvaddch(start_y + height - 1, start_x, 'm');
        mvaddch(start_y + height - 1, start_x + width - 1, 'j');
        mvhline(start_y, start_x + 1, 'q', width - 2);
        mvhline(start_y + height - 1, start_x + 1, 'q', width - 2);
        mvvline(start_y + 1, start_x, 'x', height - 2);
        mvvline(start_y + 1, start_x + width - 1, 'x', height - 2);
        if (nc_window_has_title(window)) {
            mvaddch(start_y + 2, start_x, 't');
            mvaddch(start_y + 2, start_x + width - 1, 'u');
        }
        attroff(A_ALTCHARSET);
    } else {
        color_set((short)nc_color_pair_number(window->base_color), NULL);
    }
    if (nc_window_has_title(window)) {
        mvhline(nc_i32(window->start_y - 2), nc_i32(window->start_x),
                ' ', nc_i32(window->width));
        attron(A_BOLD);
        mvaddnstr(nc_i32(window->start_y - 2), nc_i32(window->start_x),
                  window->title, window->title_len);
        attroff(A_BOLD);
        mvhline(nc_i32(window->start_y - 1), nc_i32(window->start_x),
                0, nc_i32(window->width));
    }
    standend();
    refresh();
    return;
}

void
nc_window_refresh(NcWindow *window) {
    prefresh(window->window, 0, 0,
             nc_i32(window->start_y), nc_i32(window->start_x),
             nc_i32(window->start_y + window->height - 1),
             nc_i32(window->start_x + window->width - 1));
    return;
}

void
nc_window_move_to(NcWindow *window, int64 new_x, int64 new_y) {
    window->start_x = new_x;
    window->start_y = new_y;
    if (window->border.enabled) {
        window->start_x += 1;
        window->start_y += 1;
    }
    if (nc_window_has_title(window)) {
        window->start_y += 2;
    }
    return;
}

void
nc_window_adjust_dimensions(NcWindow *window,
                            int64 width, int64 height) {
    if (window->border.enabled) {
        if (width >= 2) {
            width -= 2;
        }
        if (height >= 2) {
            height -= 2;
        }
    }
    if (nc_window_has_title(window)) {
        if (height >= 2) {
            height -= 2;
        }
    }
    window->height = height;
    window->width = width;
    return;
}

void
nc_window_resize(NcWindow *window, int64 new_width, int64 new_height) {
    nc_window_adjust_dimensions(window, new_width, new_height);
    nc_window_recreate(window, window->width, window->height);
    return;
}

void
nc_window_recreate(NcWindow *window, int64 width, int64 height) {
    if (window->window != NULL) {
        delwin(window->window);
    }
    window->window = newpad(nc_i32(height), nc_i32(width));
    wtimeout(window->window, 0);
    nc_window_set_color(window, window->color);
    return;
}

void
nc_window_clear(NcWindow *window) {
    werase(window->window);
    nc_window_set_color(window, window->base_color);
    return;
}

void
nc_window_add_fd_callback(NcWindow *window,
                          int32 fd, void (*callback)(void)) {
    NcFdCallback item;

    item.callback = callback;
    item.fd = fd;
    ARRAY_PUSH(window->fd_callbacks, item);
    return;
}

void
nc_window_clear_fd_callbacks(NcWindow *window) {
    ARRAY_CLEAR(window->fd_callbacks);
    return;
}

bool
nc_window_fd_callbacks_empty(NcWindow *window) {
    return ARRAY_LEN(window->fd_callbacks) == 0;
}

NcKey
nc_window_read_key(NcWindow *window) {
    NcKey result;
    fd_set fds_read;
    struct timeval timeout;
    struct timeval *tv_addr;
    int32 fd_max;
    int32 res;

    if (window->input_queue_start < ARRAY_LEN(window->input_queue)) {
        result = window->input_queue[window->input_queue_start];
        window->input_queue_start += 1;
        if (window->input_queue_start == ARRAY_LEN(window->input_queue)) {
            ARRAY_CLEAR(window->input_queue);
            window->input_queue_start = 0;
        }
        return result;
    }

    FD_ZERO(&fds_read);
    FD_SET(STDIN_FILENO, &fds_read);
    timeout.tv_sec = window->window_timeout / 1000;
    timeout.tv_usec = (window->window_timeout % 1000)*1000;

    fd_max = STDIN_FILENO;
    for (int32 i = 0; i < ARRAY_LEN(window->fd_callbacks); i += 1) {
        if (window->fd_callbacks[i].fd > fd_max) {
            fd_max = window->fd_callbacks[i].fd;
        }
        FD_SET(window->fd_callbacks[i].fd, &fds_read);
    }

    if (window->window_timeout < 0) {
        tv_addr = NULL;
    } else {
        tv_addr = &timeout;
    }

    res = select(fd_max + 1, &fds_read, NULL, NULL, tv_addr);
    if (res > 0) {
        if (FD_ISSET(STDIN_FILENO, &fds_read)) {
            int32 key;

            key = wgetch(window->window);
            if (key == EOF) {
                result = NC_KEY_EOF;
            } else {
                result = nc_window_get_input_char(window, key);
            }
        } else {
            result = NC_KEY_NONE;
        }

        for (int32 i = 0; i < ARRAY_LEN(window->fd_callbacks); i += 1) {
            if (FD_ISSET(window->fd_callbacks[i].fd, &fds_read)) {
                window->fd_callbacks[i].callback();
            }
        }
    } else {
        result = NC_KEY_NONE;
    }

    return result;
}

void
nc_window_push_key(NcWindow *window, NcKey ch) {
    ARRAY_PUSH(window->input_queue, ch);
    return;
}

void
nc_window_scroll(NcWindow *window, enum NcScroll where) {
    idlok(window->window, 1);
    scrollok(window->window, 1);
    switch (where) {
    case NC_SCROLL_UP:
        wscrl(window->window, 1);
        break;
    case NC_SCROLL_DOWN:
        wscrl(window->window, -1);
        break;
    case NC_SCROLL_PAGE_UP:
        wscrl(window->window, nc_i32(window->width));
        break;
    case NC_SCROLL_PAGE_DOWN:
        wscrl(window->window, -nc_i32(window->width));
        break;
    default:
        break;
    }
    idlok(window->window, 0);
    scrollok(window->window, 0);
    return;
}

void
nc_window_apply_term_manip(NcWindow *window, enum NcTermManip tm) {
    int32 x;
    int32 y;

    switch (tm) {
    case NC_TERM_CLEAR_TO_EOL:
        x = nc_window_get_x(window);
        y = nc_window_get_y(window);
        mvwhline(window->window, y, x, ' ', nc_i32(window->width) - x);
        nc_window_go_to_xy(window, x, y);
        break;
    }
    return;
}

void
nc_window_apply_format(NcWindow *window, enum NcFormat format) {
    switch (format) {
    case NC_FORMAT_BOLD:
        nc_window_increase_format(window, &window->bold_counter,
                                  nc_window_bold);
        break;
    case NC_FORMAT_NO_BOLD:
        nc_window_decrease_format(window, &window->bold_counter,
                                  nc_window_bold);
        break;
    case NC_FORMAT_UNDERLINE:
        nc_window_increase_format(window, &window->underline_counter,
                                  nc_window_underline);
        break;
    case NC_FORMAT_NO_UNDERLINE:
        nc_window_decrease_format(window, &window->underline_counter,
                                  nc_window_underline);
        break;
    case NC_FORMAT_REVERSE:
        nc_window_increase_format(window, &window->reverse_counter,
                                  nc_window_reverse);
        break;
    case NC_FORMAT_NO_REVERSE:
        nc_window_decrease_format(window, &window->reverse_counter,
                                  nc_window_reverse);
        break;
    case NC_FORMAT_ALT_CHARSET:
        nc_window_increase_format(window, &window->alt_charset_counter,
                                  nc_window_alt_charset);
        break;
    case NC_FORMAT_NO_ALT_CHARSET:
        nc_window_decrease_format(window, &window->alt_charset_counter,
                                  nc_window_alt_charset);
        break;
    case NC_FORMAT_ITALIC:
        nc_window_increase_format(window, &window->italic_counter,
                                  nc_window_italic);
        break;
    case NC_FORMAT_NO_ITALIC:
        nc_window_decrease_format(window, &window->italic_counter,
                                  nc_window_italic);
        break;
    }
    return;
}

void
nc_window_push_color(NcWindow *window, NcColor color) {
    if (nc_color_is_default(color)) {
        ARRAY_CLEAR(window->color_stack);
        nc_window_set_color(window, window->base_color);
    } else if (nc_color_is_end(color)) {
        if (ARRAY_LEN(window->color_stack) > 0) {
            ARRAY_HEADER(window->color_stack)->count -= 1;
        }
        if (ARRAY_LEN(window->color_stack) > 0) {
            nc_window_set_color(window,
                                window->color_stack[
                                    ARRAY_LEN(window->color_stack) - 1]);
        } else {
            nc_window_set_color(window, window->base_color);
        }
    } else {
        if (nc_color_current_background(color)) {
            NcColor current_color;
            int16 background;

            if (nc_color_is_default(window->color)) {
                background = NC_COLOR_TRANSPARENT;
            } else {
                background = window->color.background;
            }
            current_color = nc_color_make(color.foreground, background,
                                          false, false);
            nc_window_set_color(window, current_color);
            ARRAY_PUSH(window->color_stack, current_color);
        } else {
            nc_window_set_color(window, color);
            ARRAY_PUSH(window->color_stack, color);
        }
    }
    return;
}

void
nc_window_go_to_xy(NcWindow *window, int32 x, int32 y) {
    wmove(window->window, y, x);
    return;
}

int32
nc_window_get_x(NcWindow *window) {
    return getcurx(window->window);
}

int32
nc_window_get_y(NcWindow *window) {
    return getcury(window->window);
}

bool
nc_window_has_coords(NcWindow *window, int32 *x, int32 *y) {
    return wmouse_trafo(window->window, y, x, 0);
}

void
nc_window_print_cstring(NcWindow *window, char *string) {
    waddstr(window->window, string);
    return;
}

void
nc_window_print_data(NcWindow *window, char *string, int32 string_len) {
    waddnstr(window->window, string, string_len);
    return;
}

void
nc_window_print_char(NcWindow *window, char ch) {
    waddnstr(window->window, &ch, 1);
    return;
}

void
nc_window_print_int32(NcWindow *window, int32 value) {
    wprintw(window->window, "%d", value);
    return;
}

void
nc_window_print_uint64(NcWindow *window, uint64 value) {
    wprintw(window->window, "%llu", (ullong)value);
    return;
}

void
nc_window_print_double(NcWindow *window, double value) {
    wprintw(window->window, "%f", value);
    return;
}

static void
nc_window_assign_title(NcWindow *window, char *title, int32 title_len) {
    if (title == NULL) {
        title_len = 0;
    }
    if (title_len < 0) {
        title_len = 0;
    }
    if ((title_len + 1) > window->title_cap) {
        if (window->title != NULL) {
            cbase_free(window->title, window->title_cap);
        }
        window->title_cap = title_len + 1;
        window->title = cbase_malloc(window->title_cap);
    }
    if (title_len > 0) {
        cbase_memcpy(window->title, title, title_len);
    }
    window->title[title_len] = '\0';
    window->title_len = title_len;
    return;
}

static void
nc_window_copy_colors(NcColor **dest, NcColor *source) {
    for (int32 i = 0; i < ARRAY_LEN(source); i += 1) {
        ARRAY_PUSH(*dest, source[i]);
    }
    return;
}

static void
nc_window_copy_keys(NcKey **dest, NcKey *source) {
    for (int32 i = 0; i < ARRAY_LEN(source); i += 1) {
        ARRAY_PUSH(*dest, source[i]);
    }
    return;
}

static void
nc_window_copy_fd_callbacks(NcFdCallback **dest, NcFdCallback *source) {
    for (int32 i = 0; i < ARRAY_LEN(source); i += 1) {
        ARRAY_PUSH(*dest, source[i]);
    }
    return;
}

static int32
nc_i32(int64 value) {
    return (int32)value;
}

static bool
nc_window_has_title(NcWindow *window) {
    return window->title_len > 0;
}

static NcKey
nc_window_get_input_char(NcWindow *window, int32 key) {
    if (!window->escape_terminal_sequences || (key != NC_KEY_ESCAPE)) {
        return (NcKey)key;
    }

    key = wgetch(window->window);
    switch (key) {
    case '\t':
        return NC_KEY_SHIFT | NC_KEY_TAB;
    case 'O':
        key = wgetch(window->window);
        switch (key) {
        case 'A':
            return NC_KEY_UP;
        case 'B':
            return NC_KEY_DOWN;
        case 'C':
            return NC_KEY_RIGHT;
        case 'D':
            return NC_KEY_LEFT;
        case 'F':
            return NC_KEY_END;
        case 'H':
            return NC_KEY_HOME;
        case 'a':
            return NC_KEY_CTRL | NC_KEY_UP;
        case 'b':
            return NC_KEY_CTRL | NC_KEY_DOWN;
        case 'c':
            return NC_KEY_CTRL | NC_KEY_RIGHT;
        case 'd':
            return NC_KEY_CTRL | NC_KEY_LEFT;
        case 'P':
            return NC_KEY_F1;
        case 'Q':
            return NC_KEY_F2;
        case 'R':
            return NC_KEY_F3;
        case 'S':
            return NC_KEY_F4;
        default:
            return NC_KEY_NONE;
        }
    case '[':
        key = wgetch(window->window);
        switch (key) {
        case 'a':
            return NC_KEY_SHIFT | NC_KEY_UP;
        case 'b':
            return NC_KEY_SHIFT | NC_KEY_DOWN;
        case 'c':
            return NC_KEY_SHIFT | NC_KEY_RIGHT;
        case 'd':
            return NC_KEY_SHIFT | NC_KEY_LEFT;
        case 'A':
            return NC_KEY_UP;
        case 'B':
            return NC_KEY_DOWN;
        case 'C':
            return NC_KEY_RIGHT;
        case 'D':
            return NC_KEY_LEFT;
        case 'F':
            return NC_KEY_END;
        case 'H':
            return NC_KEY_HOME;
        case 'M':
        {
            int32 raw_x;
            int32 raw_y;

            key = wgetch(window->window);
            raw_x = wgetch(window->window);
            raw_y = wgetch(window->window);
            window->mouse_event.x = (raw_x - 33) & 0xff;
            window->mouse_event.y = (raw_y - 33) & 0xff;
            return nc_window_define_mouse_event(window, key);
        }
        case 'P':
            return NC_KEY_DELETE;
        case 'Z':
            return NC_KEY_SHIFT | NC_KEY_TAB;
        case '[':
            key = wgetch(window->window);
            switch (key) {
            case 'A':
                return NC_KEY_F1;
            case 'B':
                return NC_KEY_F2;
            case 'C':
                return NC_KEY_F3;
            case 'D':
                return NC_KEY_F4;
            case 'E':
                return NC_KEY_F5;
            default:
                return NC_KEY_NONE;
            }
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            int32 delim;

            key -= '0';
            delim = nc_window_parse_number(window, &key);
            if ((key >= 2) && (key <= 8)) {
                NcKey modifier;

                switch (delim) {
                case '~':
                    modifier = NC_KEY_NULL;
                    break;
                case '^':
                    modifier = NC_KEY_CTRL;
                    break;
                case '$':
                    modifier = NC_KEY_SHIFT;
                    break;
                case '@':
                    modifier = NC_KEY_CTRL | NC_KEY_SHIFT;
                    break;
                case ';':
                {
                    int32 local_key;

                    local_key = wgetch(window->window);
                    modifier = nc_xterm_modifier_key(local_key);
                    local_key = wgetch(window->window);
                    if ((local_key != '~')
                        || ((key != 2) && (key != 3)
                            && (key != 5) && (key != 6))) {
                        return NC_KEY_NONE;
                    }
                    break;
                }
                default:
                    return NC_KEY_NONE;
                }
                switch (key) {
                case 2:
                    return modifier | NC_KEY_INSERT;
                case 3:
                    return modifier | NC_KEY_DELETE;
                case 4:
                    return modifier | NC_KEY_END;
                case 5:
                    return modifier | NC_KEY_PAGE_UP;
                case 6:
                    return modifier | NC_KEY_PAGE_DOWN;
                case 7:
                    return modifier | NC_KEY_HOME;
                case 8:
                    return modifier | NC_KEY_END;
                default:
                    return NC_KEY_NONE;
                }
            }
            switch (delim) {
            case '~':
                switch (key) {
                case 1:
                    return NC_KEY_HOME;
                case 11:
                    return NC_KEY_F1;
                case 12:
                    return NC_KEY_F2;
                case 13:
                    return NC_KEY_F3;
                case 14:
                    return NC_KEY_F4;
                case 15:
                    return NC_KEY_F5;
                case 17:
                    return NC_KEY_F6;
                case 18:
                    return NC_KEY_F7;
                case 19:
                    return NC_KEY_F8;
                case 20:
                    return NC_KEY_F9;
                case 21:
                    return NC_KEY_F10;
                case 23:
                    return NC_KEY_F11;
                case 24:
                    return NC_KEY_F12;
                default:
                    return NC_KEY_NONE;
                }
            case ';':
                switch (key) {
                case 1:
                {
                    NcKey modifier;

                    key = wgetch(window->window);
                    modifier = nc_xterm_modifier_key(key);
                    if (modifier == NC_KEY_NONE) {
                        return NC_KEY_NONE;
                    }
                    key = wgetch(window->window);
                    switch (key) {
                    case 'A':
                        return modifier | NC_KEY_UP;
                    case 'B':
                        return modifier | NC_KEY_DOWN;
                    case 'C':
                        return modifier | NC_KEY_RIGHT;
                    case 'D':
                        return modifier | NC_KEY_LEFT;
                    case 'F':
                        return modifier | NC_KEY_END;
                    case 'H':
                        return modifier | NC_KEY_HOME;
                    default:
                        return NC_KEY_NONE;
                    }
                }
                default:
                    window->mouse_event.x = 0;
                    delim = nc_window_parse_number(window,
                                                   &window->mouse_event.x);
                    if (delim != ';') {
                        return NC_KEY_NONE;
                    }
                    window->mouse_event.y = 0;
                    delim = nc_window_parse_number(window,
                                                   &window->mouse_event.y);
                    if (delim != 'M') {
                        return NC_KEY_NONE;
                    }
                    window->mouse_event.x -= 1;
                    window->mouse_event.y -= 1;
                    return nc_window_define_mouse_event(window, key);
                }
            default:
                return NC_KEY_NONE;
            }
        }
        default:
            return NC_KEY_NONE;
        }
    case ERR:
        return NC_KEY_ESCAPE;
    default:
    {
        NcKey key_prim;

        key_prim = nc_window_get_input_char(window, key);
        if (key_prim != NC_KEY_NONE) {
            return NC_KEY_ALT | key_prim;
        }
        return NC_KEY_NONE;
    }
    }
}

static NcKey
nc_window_define_mouse_event(NcWindow *window, int32 type) {
    switch (type & ~28) {
    case 32:
        window->mouse_event.bstate = BUTTON1_PRESSED;
        break;
    case 33:
        window->mouse_event.bstate = BUTTON2_PRESSED;
        break;
    case 34:
        window->mouse_event.bstate = BUTTON3_PRESSED;
        break;
    case 96:
        window->mouse_event.bstate = BUTTON4_PRESSED;
        break;
    case 97:
        window->mouse_event.bstate = BUTTON5_PRESSED;
        break;
    default:
        return NC_KEY_NONE;
    }
    if (type & 4) {
        window->mouse_event.bstate |= BUTTON_SHIFT;
    }
    if (type & 8) {
        window->mouse_event.bstate |= BUTTON_ALT;
    }
    if (type & 16) {
        window->mouse_event.bstate |= BUTTON_CTRL;
    }
    if ((window->mouse_event.x < 0) || (window->mouse_event.x >= COLS)) {
        return NC_KEY_NONE;
    }
    if ((window->mouse_event.y < 0) || (window->mouse_event.y >= LINES)) {
        return NC_KEY_NONE;
    }

    return NC_KEY_MOUSE;
}

static NcKey
nc_xterm_modifier_key(int32 ch) {
    switch (ch) {
    case '2':
        return NC_KEY_SHIFT;
    case '3':
        return NC_KEY_ALT;
    case '4':
        return NC_KEY_ALT | NC_KEY_SHIFT;
    case '5':
        return NC_KEY_CTRL;
    case '6':
        return NC_KEY_CTRL | NC_KEY_SHIFT;
    case '7':
        return NC_KEY_ALT | NC_KEY_CTRL;
    case '8':
        return NC_KEY_ALT | NC_KEY_CTRL | NC_KEY_SHIFT;
    default:
        return NC_KEY_NONE;
    }
}

static int32
nc_window_parse_number(NcWindow *window, int32 *result) {
    int32 x;

    while (true) {
        x = wgetch(window->window);
        if (!isdigit(x)) {
            return x;
        }
        *result = *result*10 + x - '0';
    }
}

static void
nc_window_bold(NcWindow *window, bool state) {
    if (state) {
        wattron(window->window, A_BOLD);
    } else {
        wattroff(window->window, A_BOLD);
    }
    return;
}

static void
nc_window_underline(NcWindow *window, bool state) {
    if (state) {
        wattron(window->window, A_UNDERLINE);
    } else {
        wattroff(window->window, A_UNDERLINE);
    }
    return;
}

static void
nc_window_reverse(NcWindow *window, bool state) {
    if (state) {
        wattron(window->window, A_REVERSE);
    } else {
        wattroff(window->window, A_REVERSE);
    }
    return;
}

static void
nc_window_alt_charset(NcWindow *window, bool state) {
    if (state) {
        wattron(window->window, A_ALTCHARSET);
    } else {
        wattroff(window->window, A_ALTCHARSET);
    }
    return;
}

static void
nc_window_italic(NcWindow *window, bool state) {
    if (state) {
        wattron(window->window, A_ITALIC);
    } else {
        wattroff(window->window, A_ITALIC);
    }
    return;
}

static void
nc_window_increase_format(NcWindow *window, int32 *counter,
                          void (*set)(NcWindow *, bool)) {
    *counter += 1;
    set(window, true);
    return;
}

static void
nc_window_decrease_format(NcWindow *window, int32 *counter,
                          void (*set)(NcWindow *, bool)) {
    if (*counter > 0) {
        *counter -= 1;
        if (*counter == 0) {
            set(window, false);
        }
    }
    return;
}
