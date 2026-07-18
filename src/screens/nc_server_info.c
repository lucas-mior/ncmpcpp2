#if !defined(NCMPCPP_NC_SERVER_INFO_C)
#define NCMPCPP_NC_SERVER_INFO_C

#include "screens/nc_server_info.h"

#include "cbase/base_macros.h"

static NcScreenCallbacks nc_server_info_callbacks(void);
static NcWindow *nc_server_info_active_window(NcScreen *screen);
static void nc_server_info_refresh(NcScreen *screen);
static void nc_server_info_refresh_window(NcScreen *screen);
static void nc_server_info_scroll(NcScreen *screen, enum NcScroll where);
static void nc_server_info_switch_to(NcScreen *screen);
static void nc_server_info_resize(NcScreen *screen);
static int32 nc_server_info_window_timeout(NcScreen *screen);
static char *nc_server_info_title(NcScreen *screen);
static void nc_server_info_update(NcScreen *screen);
static void nc_server_info_mouse_button_pressed(NcScreen *screen,
                                                MEVENT event);
static bool nc_server_info_is_lockable(NcScreen *screen);
static bool nc_server_info_is_mergable(NcScreen *screen);
static void nc_server_info_destroy_callback(NcScreen *screen);
static void nc_server_info_display(NcServerInfoScreen *server_info);

void
nc_server_info_screen_init(NcServerInfoScreen *screen,
                           NcServerInfoHooks hooks,
                           int64 cols, int64 lines,
                           int64 main_start_y,
                           int64 main_height,
                           NcColor color, NcBorder border) {
    screen->hooks = hooks;
    nc_scrollpad_screen_init(&screen->scrollpad_screen,
                             nc_server_info_callbacks(),
                             hooks.user,
                             NC_SCREEN_TYPE_SERVER_INFO,
                             0,
                             0,
                             0,
                             0);
    nc_buffer_init(&screen->buffer);
    nc_server_info_screen_set_dimensions(screen,
                                         cols,
                                         lines,
                                         main_start_y,
                                         main_height);
    nc_window_init(&screen->window,
                   nc_server_info_screen_start_x(screen),
                   nc_server_info_screen_start_y(screen),
                   nc_server_info_screen_width(screen),
                   nc_server_info_screen_height(screen),
                   STRLIT_ARGS("MPD server info"),
                   color,
                   border);
    nc_scrollpad_init(&screen->scrollpad,
                      nc_window_height(&screen->window));
    return;
}

void
nc_server_info_screen_set_dimensions(NcServerInfoScreen *screen,
                                     int64 cols, int64 lines,
                                     int64 main_start_y,
                                     int64 main_height) {
    nc_scrollpad_screen_set_centered_box(&screen->scrollpad_screen,
                                         cols,
                                         lines,
                                         main_start_y,
                                         main_height,
                                         6,
                                         10,
                                         7,
                                         10);
    return;
}

NcScreen *
nc_server_info_screen_base(NcServerInfoScreen *screen) {
    return nc_scrollpad_screen_base(&screen->scrollpad_screen);
}

int64
nc_server_info_screen_width(NcServerInfoScreen *screen) {
    return nc_scrollpad_screen_width(&screen->scrollpad_screen);
}

int64
nc_server_info_screen_height(NcServerInfoScreen *screen) {
    return nc_scrollpad_screen_height(&screen->scrollpad_screen);
}

int64
nc_server_info_screen_start_x(NcServerInfoScreen *screen) {
    return nc_scrollpad_screen_start_x(&screen->scrollpad_screen);
}

int64
nc_server_info_screen_start_y(NcServerInfoScreen *screen) {
    return nc_scrollpad_screen_start_y(&screen->scrollpad_screen);
}

static NcServerInfoScreen *
nc_server_info_from_screen(NcScreen *screen) {
    return (NcServerInfoScreen *)screen;
}

static NcScreenCallbacks
nc_server_info_callbacks(void) {
    NcScreenCallbacks callbacks = {0};

    callbacks.active_window = nc_server_info_active_window;
    callbacks.refresh = nc_server_info_refresh;
    callbacks.refresh_window = nc_server_info_refresh_window;
    callbacks.scroll = nc_server_info_scroll;
    callbacks.switch_to = nc_server_info_switch_to;
    callbacks.resize = nc_server_info_resize;
    callbacks.window_timeout = nc_server_info_window_timeout;
    callbacks.title = nc_server_info_title;
    callbacks.update = nc_server_info_update;
    callbacks.mouse_button_pressed = nc_server_info_mouse_button_pressed;
    callbacks.is_lockable = nc_server_info_is_lockable;
    callbacks.is_mergable = nc_server_info_is_mergable;
    callbacks.destroy = nc_server_info_destroy_callback;
    return callbacks;
}

static NcWindow *
nc_server_info_active_window(NcScreen *screen) {
    NcServerInfoScreen *server_info;

    server_info = nc_server_info_from_screen(screen);
    return &server_info->window;
}

static void
nc_server_info_refresh(NcScreen *screen) {
    nc_server_info_display(nc_server_info_from_screen(screen));
    return;
}

static void
nc_server_info_refresh_window(NcScreen *screen) {
    nc_server_info_display(nc_server_info_from_screen(screen));
    return;
}

static void
nc_server_info_scroll(NcScreen *screen, enum NcScroll where) {
    NcServerInfoScreen *server_info;

    server_info = nc_server_info_from_screen(screen);
    nc_scrollpad_scroll(&server_info->scrollpad, &server_info->window, where);
    return;
}

static void
nc_server_info_switch_to(NcScreen *screen) {
    NcServerInfoScreen *server_info;

    server_info = nc_server_info_from_screen(screen);
    if (server_info->hooks.switch_to) {
        server_info->hooks.switch_to(server_info->hooks.user);
    }
    if (server_info->hooks.load_lists) {
        server_info->hooks.load_lists(server_info->hooks.user);
    }
    return;
}

static void
nc_server_info_resize(NcScreen *screen) {
    NcServerInfoScreen *server_info;

    server_info = nc_server_info_from_screen(screen);
    if (server_info->hooks.resize_layout) {
        server_info->hooks.resize_layout(server_info->hooks.user);
    }
    nc_scrollpad_resize(&server_info->scrollpad,
                        &server_info->window,
                        nc_server_info_screen_width(server_info),
                        nc_server_info_screen_height(server_info));
    nc_window_move_to(&server_info->window,
                      nc_server_info_screen_start_x(server_info),
                      nc_server_info_screen_start_y(server_info));
    nc_scrollpad_flush(&server_info->scrollpad,
                       &server_info->window,
                       &server_info->buffer);
    if (server_info->hooks.resize_background) {
        server_info->hooks.resize_background(server_info->hooks.user);
    }
    return;
}

static int32
nc_server_info_window_timeout(NcScreen *screen) {
    (void)screen;
    return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
}

static char *
nc_server_info_title(NcScreen *screen) {
    NcServerInfoScreen *server_info;

    server_info = nc_server_info_from_screen(screen);
    if (server_info->hooks.title == NULL) {
        return NULL;
    }
    return server_info->hooks.title(server_info->hooks.user);
}

static void
nc_server_info_update(NcScreen *screen) {
    NcServerInfoScreen *server_info;
    NcBuffer next_buffer;

    server_info = nc_server_info_from_screen(screen);
    if (server_info->hooks.render == NULL) {
        return;
    }
    nc_buffer_init(&next_buffer);
    if (!server_info->hooks.render(server_info->hooks.user,
                                   &next_buffer)) {
        nc_buffer_destroy(&next_buffer);
        return;
    }
    nc_buffer_destroy(&server_info->buffer);
    nc_buffer_move(&server_info->buffer, &next_buffer);
    if (!nc_buffer_empty(&server_info->buffer)) {
        nc_scrollpad_flush(&server_info->scrollpad,
                           &server_info->window,
                           &server_info->buffer);
        nc_scrollpad_refresh(&server_info->scrollpad,
                             &server_info->window);
    }
    return;
}

static void
nc_server_info_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    NcServerInfoScreen *server_info;

    server_info = nc_server_info_from_screen(screen);
    if (event.bstate & BUTTON5_PRESSED) {
        nc_scrollpad_scroll(&server_info->scrollpad,
                            &server_info->window,
                            NC_SCROLL_DOWN);
    } else if (event.bstate & BUTTON4_PRESSED) {
        nc_scrollpad_scroll(&server_info->scrollpad,
                            &server_info->window,
                            NC_SCROLL_UP);
    }
    return;
}

static bool
nc_server_info_is_lockable(NcScreen *screen) {
    (void)screen;
    return false;
}

static bool
nc_server_info_is_mergable(NcScreen *screen) {
    (void)screen;
    return false;
}

static void
nc_server_info_destroy_callback(NcScreen *screen) {
    NcServerInfoScreen *server_info;

    server_info = nc_server_info_from_screen(screen);
    if (server_info->hooks.destroy) {
        server_info->hooks.destroy(server_info->hooks.user);
    }
    return;
}

static void
nc_server_info_display(NcServerInfoScreen *server_info) {
    nc_window_refresh_border(&server_info->window);
    nc_scrollpad_refresh(&server_info->scrollpad, &server_info->window);
    return;
}

#endif /* NCMPCPP_NC_SERVER_INFO_C */
