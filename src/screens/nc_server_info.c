#if !defined(NCMPCPP_NC_SERVER_INFO_C)
#define NCMPCPP_NC_SERVER_INFO_C

#include "cbase.h"

#include "screens/nc_server_info.h"

static void nc_server_info_switch_to(NcScreen *screen);
static void nc_server_info_resize(NcScreen *screen);
static char *nc_server_info_title(NcScreen *screen);
static void nc_server_info_update(NcScreen *screen);
static void nc_server_info_mouse_button_pressed(NcScreen *screen,
                                                MEVENT event);
static void nc_server_info_destroy_callback(NcScreen *screen);
static void nc_server_info_display(NcServerInfoScreen *server_info);

#define NC_SCREEN_IMPL_TYPE NcServerInfoScreen
#define NC_SCREEN_IMPL_PREFIX nc_server_info
#define NC_SCREEN_IMPL_PUBLIC_PREFIX nc_server_info_screen
#define NC_SCREEN_IMPL_BASE_FIELD scrollpad_screen
#define NC_SCREEN_IMPL_WINDOW_FIELD window
#define NC_SCREEN_IMPL_SCROLLPAD_FIELD scrollpad
#define NC_SCREEN_IMPL_REFRESH_CALLBACK nc_server_info_display
#define NC_SCREEN_IMPL_SWITCH_TO_CALLBACK nc_server_info_switch_to
#define NC_SCREEN_IMPL_RESIZE_CALLBACK nc_server_info_resize
#define NC_SCREEN_IMPL_TITLE_CALLBACK nc_server_info_title
#define NC_SCREEN_IMPL_UPDATE_CALLBACK nc_server_info_update
#define NC_SCREEN_IMPL_MOUSE_CALLBACK nc_server_info_mouse_button_pressed
#define NC_SCREEN_IMPL_DESTROY_CALLBACK nc_server_info_destroy_callback
#include "screens/nc_screen_impl_template.c"

void
nc_server_info_screen_init(NcServerInfoScreen *screen,
                           NcServerInfoHooks hooks,
                           int32 cols, int32 lines,
                           int32 main_start_y,
                           int32 main_height,
                           NcColor color, NcBorder border) {
    screen->hooks = hooks;
    nc_scrollpad_screen_init(&screen->scrollpad_screen,
                             nc_server_info_ops,
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
                   STRLIT("MPD server info"),
                   color,
                   border);
    nc_scrollpad_init(&screen->scrollpad,
                      nc_window_height(&screen->window));
    return;
}

void
nc_server_info_screen_set_dimensions(NcServerInfoScreen *screen,
                                     int32 cols, int32 lines,
                                     int32 main_start_y,
                                     int32 main_height) {
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
