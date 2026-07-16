#include "screens/nc_help.h"

#include <errno.h>

#include "c/ncm_regex.h"
#include "cbase/base_macros.h"
#include "settings.h"


static NcHelpScreen *nc_help_from_screen(NcScreen *screen);
static NcScreenCallbacks nc_help_callbacks(void);
static NcWindow *nc_help_active_window(NcScreen *screen);
static void nc_help_refresh(NcScreen *screen);
static void nc_help_refresh_window(NcScreen *screen);
static void nc_help_scroll(NcScreen *screen, enum NcScroll where);
static void nc_help_switch_to(NcScreen *screen);
static void nc_help_resize(NcScreen *screen);
static int32 nc_help_window_timeout(NcScreen *screen);
static char *nc_help_title(NcScreen *screen);
static void nc_help_update(NcScreen *screen);
static void nc_help_mouse_button_pressed(NcScreen *screen, MEVENT event);
static bool nc_help_is_lockable(NcScreen *screen);
static bool nc_help_is_mergable(NcScreen *screen);
static void nc_help_destroy_callback(NcScreen *screen);
static void nc_help_display(NcHelpScreen *help);
static void nc_help_mouse_scroll(NcHelpScreen *help, enum NcScroll where);
static bool nc_help_find_match_callback(int32 start, int32 len, void *user);

void
nc_help_screen_init(NcHelpScreen *screen,
                    NcHelpHooks hooks,
                    int64 start_x, int64 width,
                    int64 main_start_y, int64 main_height,
                    NcColor color, NcBorder border,
                    int64 lines_scrolled) {
    screen->hooks = hooks;
    screen->lines_scrolled = lines_scrolled;
    nc_scrollpad_screen_init(&screen->scrollpad_screen,
                             nc_help_callbacks(),
                             hooks.user,
                             NC_SCREEN_TYPE_HELP,
                             0,
                             0,
                             0,
                             0);
    nc_buffer_init(&screen->buffer);
    ncm_buffer_init(&screen->search_constraint);
    nc_help_screen_set_geometry(screen,
                                start_x,
                                width,
                                main_start_y,
                                main_height);
    nc_window_init(&screen->window,
                   nc_help_screen_start_x(screen),
                   nc_help_screen_start_y(screen),
                   nc_help_screen_width(screen),
                   nc_help_screen_height(screen),
                   STRLIT_ARGS(""),
                   color,
                   border);
    nc_scrollpad_init(&screen->scrollpad,
                      nc_window_height(&screen->window));
    return;
}

void
nc_help_screen_destroy(NcHelpScreen *screen) {
    ncm_buffer_destroy(&screen->search_constraint);
    nc_buffer_destroy(&screen->buffer);
    nc_window_destroy(&screen->window);
    nc_scrollpad_init_empty(&screen->scrollpad);
    return;
}

void
nc_help_screen_set_geometry(NcHelpScreen *screen,
                            int64 start_x, int64 width,
                            int64 main_start_y, int64 main_height) {
    nc_scrollpad_screen_set_main_area(&screen->scrollpad_screen,
                                      start_x,
                                      width,
                                      main_start_y,
                                      main_height);
    return;
}

bool
nc_help_screen_reload(NcHelpScreen *screen) {
    NcBuffer next_buffer;

    if (screen->hooks.render == NULL) {
        return false;
    }

    nc_buffer_init(&next_buffer);
    if (!screen->hooks.render(screen->hooks.user, &next_buffer)) {
        nc_buffer_destroy(&next_buffer);
        return false;
    }

    nc_buffer_destroy(&screen->buffer);
    nc_buffer_move(&screen->buffer, &next_buffer);
    nc_scrollpad_flush(&screen->scrollpad,
                       &screen->window,
                       &screen->buffer);
    return true;
}

bool
nc_help_screen_find(NcHelpScreen *screen, char *pattern,
                    int32 pattern_len, NcmError *error) {
    NcmRegex regex;
    char *data;
    bool result;

    if (screen == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing help screen"));
        return false;
    }

    nc_buffer_remove_properties(&screen->buffer, 0);
    if ((pattern == NULL) || (pattern_len <= 0)) {
        ncm_buffer_clear(&screen->search_constraint);
        nc_scrollpad_flush(&screen->scrollpad,
                           &screen->window,
                           &screen->buffer);
        ncm_error_clear(error);
        return true;
    }

    ncm_regex_init(&regex);
    if (!ncm_regex_compile(&regex, pattern, pattern_len, Config.regex_type,
                           error)) {
        ncm_regex_destroy(&regex);
        return false;
    }

    if (!ncm_buffer_set(&screen->search_constraint, pattern, pattern_len)) {
        ncm_regex_destroy(&regex);
        ncm_error_set(error, ENOMEM, STRLIT_ARGS("failed to save search"));
        return false;
    }

    data = nc_buffer_data(&screen->buffer);
    result = ncm_regex_for_each_match(&regex,
                                      data,
                                      screen->buffer.len,
                                      nc_help_find_match_callback,
                                      screen);
    ncm_regex_destroy(&regex);
    nc_scrollpad_flush(&screen->scrollpad,
                       &screen->window,
                       &screen->buffer);
    return result;
}

void
nc_help_screen_clear_search(NcHelpScreen *screen) {
    if (screen == NULL) {
        return;
    }
    ncm_buffer_clear(&screen->search_constraint);
    nc_buffer_remove_properties(&screen->buffer, 0);
    nc_scrollpad_flush(&screen->scrollpad,
                       &screen->window,
                       &screen->buffer);
    return;
}

NcScreen *
nc_help_screen_base(NcHelpScreen *screen) {
    return nc_scrollpad_screen_base(&screen->scrollpad_screen);
}

NcWindow *
nc_help_screen_window(NcHelpScreen *screen) {
    return &screen->window;
}

int64
nc_help_screen_start_x(NcHelpScreen *screen) {
    return nc_scrollpad_screen_start_x(&screen->scrollpad_screen);
}

int64
nc_help_screen_start_y(NcHelpScreen *screen) {
    return nc_scrollpad_screen_start_y(&screen->scrollpad_screen);
}

int64
nc_help_screen_width(NcHelpScreen *screen) {
    return nc_scrollpad_screen_width(&screen->scrollpad_screen);
}

int64
nc_help_screen_height(NcHelpScreen *screen) {
    return nc_scrollpad_screen_height(&screen->scrollpad_screen);
}

static NcHelpScreen *
nc_help_from_screen(NcScreen *screen) {
    return (NcHelpScreen *)screen;
}

static NcScreenCallbacks
nc_help_callbacks(void) {
    NcScreenCallbacks callbacks = {0};

    callbacks.active_window = nc_help_active_window;
    callbacks.refresh = nc_help_refresh;
    callbacks.refresh_window = nc_help_refresh_window;
    callbacks.scroll = nc_help_scroll;
    callbacks.switch_to = nc_help_switch_to;
    callbacks.resize = nc_help_resize;
    callbacks.window_timeout = nc_help_window_timeout;
    callbacks.title = nc_help_title;
    callbacks.update = nc_help_update;
    callbacks.mouse_button_pressed = nc_help_mouse_button_pressed;
    callbacks.is_lockable = nc_help_is_lockable;
    callbacks.is_mergable = nc_help_is_mergable;
    callbacks.destroy = nc_help_destroy_callback;
    return callbacks;
}

static NcWindow *
nc_help_active_window(NcScreen *screen) {
    return &nc_help_from_screen(screen)->window;
}

static void
nc_help_refresh(NcScreen *screen) {
    nc_help_display(nc_help_from_screen(screen));
    return;
}

static void
nc_help_refresh_window(NcScreen *screen) {
    nc_help_display(nc_help_from_screen(screen));
    return;
}

static void
nc_help_scroll(NcScreen *screen, enum NcScroll where) {
    NcHelpScreen *help;

    help = nc_help_from_screen(screen);
    nc_scrollpad_scroll(&help->scrollpad, &help->window, where);
    return;
}

static void
nc_help_switch_to(NcScreen *screen) {
    NcHelpScreen *help;

    help = nc_help_from_screen(screen);
    if (help->hooks.switch_to) {
        help->hooks.switch_to(help->hooks.user);
    }
    return;
}

static void
nc_help_resize(NcScreen *screen) {
    NcHelpScreen *help;

    help = nc_help_from_screen(screen);
    if (help->hooks.resize_layout) {
        help->hooks.resize_layout(help->hooks.user, help);
    }
    nc_scrollpad_resize(&help->scrollpad,
                        &help->window,
                        nc_help_screen_width(help),
                        nc_help_screen_height(help));
    nc_window_move_to(&help->window,
                      nc_help_screen_start_x(help),
                      nc_help_screen_start_y(help));
    nc_scrollpad_flush(&help->scrollpad,
                       &help->window,
                       &help->buffer);
    if (help->hooks.resize_background) {
        help->hooks.resize_background(help->hooks.user);
    }
    return;
}

static int32
nc_help_window_timeout(NcScreen *screen) {
    (void)screen;
    return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
}

static char *
nc_help_title(NcScreen *screen) {
    static char title[] = "Help";

    (void)screen;
    return title;
}

static void
nc_help_update(NcScreen *screen) {
    (void)screen;
    return;
}

static void
nc_help_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    NcHelpScreen *help;

    help = nc_help_from_screen(screen);
    if (event.bstate & BUTTON5_PRESSED) {
        nc_help_mouse_scroll(help, NC_SCROLL_DOWN);
    } else if (event.bstate & BUTTON4_PRESSED) {
        nc_help_mouse_scroll(help, NC_SCROLL_UP);
    }
    return;
}

static bool
nc_help_is_lockable(NcScreen *screen) {
    (void)screen;
    return true;
}

static bool
nc_help_is_mergable(NcScreen *screen) {
    (void)screen;
    return true;
}

static void
nc_help_destroy_callback(NcScreen *screen) {
    NcHelpScreen *help;

    help = nc_help_from_screen(screen);
    if (help->hooks.destroy) {
        help->hooks.destroy(help->hooks.user);
    }
    return;
}

static void
nc_help_display(NcHelpScreen *help) {
    nc_scrollpad_refresh(&help->scrollpad, &help->window);
    return;
}

static void
nc_help_mouse_scroll(NcHelpScreen *help, enum NcScroll where) {
    for (int64 i = 0; i < help->lines_scrolled; i += 1) {
        nc_scrollpad_scroll(&help->scrollpad, &help->window, where);
    }
    return;
}

static bool
nc_help_find_match_callback(int32 start, int32 len, void *user) {
    NcHelpScreen *screen;

    if (len <= 0) {
        return true;
    }

    screen = user;
    nc_buffer_add_format(&screen->buffer, start, NC_FORMAT_REVERSE, 0);
    nc_buffer_add_format(&screen->buffer, start + len,
                         NC_FORMAT_NO_REVERSE, 0);
    return true;
}
