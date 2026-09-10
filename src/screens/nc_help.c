#if !defined(NC_HELP_C)
#define NC_HELP_C

#include "cbase.h"

#include "c/ncm_c.h"
#include "screens/nc_screens.h"
#include "settings.h"

static void
nc_help_switch_to(NcScreen *screen) {
    NcHelpScreen *help = (NcHelpScreen *)screen;

    if (help->hooks.switch_to) {
        help->hooks.switch_to(help->hooks.user);
    }
    return;
}

static void
nc_help_resize(NcScreen *screen) {
    NcHelpScreen *help = (NcHelpScreen *)screen;
    int32 start_x;
    int32 start_y;
    int32 width;
    int32 height;

    if (help->hooks.resize_layout) {
        help->hooks.resize_layout(help->hooks.user, help);
    }
    start_x = nc_help_screen_start_x(help);
    start_y = nc_help_screen_start_y(help);
    width = nc_help_screen_width(help);
    height = nc_help_screen_height(help);
    nc_scrollpad_resize(&help->scrollpad, &help->window, width, height);
    nc_window_move_to(&help->window, start_x, start_y);
    nc_scrollpad_flush(&help->scrollpad, &help->window, &help->buffer);
    if (help->hooks.resize_background) {
        help->hooks.resize_background(help->hooks.user);
    }
    return;
}

static void
nc_help_mouse_scroll(NcHelpScreen *help, enum NcScroll where) {
    for (int32 i = 0; i < help->lines_scrolled; i += 1) {
        nc_scrollpad_scroll(&help->scrollpad, &help->window, where);
    }
    return;
}

static void
nc_help_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    NcHelpScreen *help = (NcHelpScreen *)screen;

    if (event.bstate & BUTTON5_PRESSED) {
        nc_help_mouse_scroll(help, NC_SCROLL_DOWN);
    } else if (event.bstate & BUTTON4_PRESSED) {
        nc_help_mouse_scroll(help, NC_SCROLL_UP);
    }
    return;
}

static void
nc_help_destroy_callback(NcScreen *screen) {
    NcHelpScreen *help = (NcHelpScreen *)screen;

    if (help->hooks.destroy) {
        help->hooks.destroy(help->hooks.user);
    }
    return;
}

#define NC_SCREEN_IMPL_TYPE NcHelpScreen
#define NC_SCREEN_IMPL_PREFIX nc_help
#define NC_SCREEN_IMPL_PUBLIC_PREFIX nc_help_screen
#define NC_SCREEN_IMPL_BASE_FIELD scrollpad_screen
#define NC_SCREEN_IMPL_WINDOW_FIELD window
#define NC_SCREEN_IMPL_SCROLLPAD_FIELD scrollpad
#define NC_SCREEN_IMPL_REFRESH_CALLBACK(help) \
    nc_scrollpad_refresh(&(help)->scrollpad, &(help)->window)
#define NC_SCREEN_IMPL_SWITCH_TO_CALLBACK nc_help_switch_to
#define NC_SCREEN_IMPL_RESIZE_CALLBACK nc_help_resize
#define NC_SCREEN_IMPL_SEARCH_CONSTRAINT_FIELD search_constraint
#define NC_SCREEN_IMPL_SEARCH_CLEAR_CALLBACK nc_help_screen_clear_search
#define NC_SCREEN_IMPL_FIND_CALLBACK nc_help_screen_find
#define NC_SCREEN_IMPL_TITLE_LITERAL "Help"
#define NC_SCREEN_IMPL_MOUSE_CALLBACK nc_help_mouse_button_pressed
#define NC_SCREEN_IMPL_DESTROY_CALLBACK nc_help_destroy_callback
#define NC_SCREEN_IMPL_LOCKABLE true
#define NC_SCREEN_IMPL_MERGABLE true
#include "screens/nc_screen_impl_template.h"

void
nc_help_screen_init(NcHelpScreen *screen, NcHelpHooks hooks,
                    int32 start_x, int32 width,
                    int32 main_start_y, int32 main_height,
                    NcColor color, NcBorder border, int32 lines_scrolled) {
    NcScreenOps ops;
    int32 window_start_x;
    int32 window_start_y;
    int32 window_width;
    int32 window_height;

    screen->hooks = hooks;
    screen->lines_scrolled = lines_scrolled;
    ops = nc_help_ops;
    nc_scrollpad_screen_init(&screen->scrollpad_screen, ops, hooks.user,
                             NC_SCREEN_TYPE_HELP, 0, 0, 0, 0);
    screen->buffer = (NcBuffer){0};
    screen->search_constraint = (StrBuilder){0};
    nc_help_screen_set_geometry(screen, start_x, width, main_start_y,
                                main_height);
    window_start_x = nc_help_screen_start_x(screen);
    window_start_y = nc_help_screen_start_y(screen);
    window_width = nc_help_screen_width(screen);
    window_height = nc_help_screen_height(screen);
    nc_window_init(&screen->window, window_start_x, window_start_y,
                   window_width, window_height, STRLIT(""), color, border);
    nc_scrollpad_init(&screen->scrollpad, nc_window_height(&screen->window));
    return;
}

void
nc_help_screen_set_geometry(NcHelpScreen *screen, int32 start_x, int32 width,
                            int32 main_start_y, int32 main_height) {
    nc_scrollpad_screen_set_main_area(&screen->scrollpad_screen, start_x, width,
                                      main_start_y, main_height);
    return;
}

int32
nc_help_screen_reload(NcHelpScreen *screen) {
    NcBuffer next_buffer;

    int32 status;

    ASSERT(screen != NULL);
    if (screen->hooks.render == NULL) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    next_buffer = (NcBuffer){0};
    status = screen->hooks.render(screen->hooks.user, &next_buffer);
    if (status < 0) {
        nc_buffer_destroy(&next_buffer);
        return status;
    }
    if (status == 0) {
        nc_buffer_destroy(&next_buffer);
        return -NCM_ERROR_UNAVAILABLE;
    }

    nc_buffer_destroy(&screen->buffer);
    nc_buffer_move(&screen->buffer, &next_buffer);
    nc_scrollpad_flush(&screen->scrollpad, &screen->window, &screen->buffer);
    return 0;
}

static bool
nc_help_find_match_callback(int32 start, int32 len, void *user) {
    NcHelpScreen *screen = user;

    if (len <= 0) {
        return true;
    }

    nc_buffer_add_format(&screen->buffer, start, NC_FORMAT_REVERSE, 0);
    nc_buffer_add_format(&screen->buffer, start + len, NC_FORMAT_NO_REVERSE, 0);
    return true;
}

int32
nc_help_screen_find(NcHelpScreen *screen, char *pattern, int32 pattern_len,
                    NcmError *ncm_error) {
    NcmRegex regex;
    char *data;
    int32 match_count;
    int32 status;

    ASSERT(screen != NULL);
    nc_buffer_remove_properties(&screen->buffer, 0);
    if ((pattern == NULL) || (pattern_len <= 0)) {
        sb_clear(&screen->search_constraint);
        nc_scrollpad_flush(&screen->scrollpad, &screen->window,
                           &screen->buffer);
        return ncm_error_ok(ncm_error);
    }

    regex = (NcmRegex){0};
    if ((status = ncm_regex_compile(&regex, pattern, pattern_len,
                                    Config.regular_expressions,
                                    ncm_error)) < 0) {
        ncm_regex_destroy(&regex);
        return status;
    }

    sb_set(&screen->search_constraint, pattern, pattern_len);

    data = nc_buffer_data(&screen->buffer);
    match_count = ncm_regex_for_each_match(&regex, data, screen->buffer.len,
                                           nc_help_find_match_callback,
                                           screen);
    ncm_regex_destroy(&regex);
    nc_scrollpad_flush(&screen->scrollpad, &screen->window, &screen->buffer);
    if (match_count > 0) {
        return 1;
    }
    return 0;
}

void
nc_help_screen_clear_search(NcHelpScreen *screen) {
    ASSERT(screen != NULL);
    sb_clear(&screen->search_constraint);
    nc_buffer_remove_properties(&screen->buffer, 0);
    nc_scrollpad_flush(&screen->scrollpad, &screen->window, &screen->buffer);
    return;
}

#endif /* NC_HELP_C */
