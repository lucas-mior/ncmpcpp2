#if !defined(NCMPCPP_NC_SONG_INFO_C)
#define NCMPCPP_NC_SONG_INFO_C

#include "screens/nc_song_info.h"

#include "cbase/base_macros.h"

static NcScreenCallbacks nc_song_info_callbacks(void);
static NcWindow *nc_song_info_active_window(NcScreen *screen);
static void nc_song_info_refresh(NcScreen *screen);
static void nc_song_info_refresh_window(NcScreen *screen);
static void nc_song_info_scroll(NcScreen *screen, enum NcScroll where);
static void nc_song_info_switch_to(NcScreen *screen);
static void nc_song_info_resize(NcScreen *screen);
static int32 nc_song_info_window_timeout(NcScreen *screen);
static char *nc_song_info_title(NcScreen *screen);
static void nc_song_info_update(NcScreen *screen);
static void nc_song_info_mouse_button_pressed(NcScreen *screen,
                                              MEVENT event);
static bool nc_song_info_is_lockable(NcScreen *screen);
static bool nc_song_info_is_mergable(NcScreen *screen);
static void nc_song_info_destroy_callback(NcScreen *screen);
static void nc_song_info_display(NcSongInfoScreen *song_info);

void
nc_song_info_screen_init(NcSongInfoScreen *screen,
                         NcSongInfoHooks hooks,
                         int64 start_x, int64 width,
                         int64 main_start_y, int64 main_height,
                         NcColor color, NcBorder border,
                         int64 lines_scrolled) {
    screen->hooks = hooks;
    screen->lines_scrolled = lines_scrolled;
    nc_scrollpad_screen_init(&screen->scrollpad_screen,
                             nc_song_info_callbacks(),
                             hooks.user,
                             NC_SCREEN_TYPE_SONG_INFO,
                             0,
                             0,
                             0,
                             0);
    nc_buffer_init(&screen->buffer);
    nc_song_info_screen_set_geometry(screen,
                                     start_x,
                                     width,
                                     main_start_y,
                                     main_height);
    nc_window_init(&screen->window,
                   nc_song_info_screen_start_x(screen),
                   nc_song_info_screen_start_y(screen),
                   nc_song_info_screen_width(screen),
                   nc_song_info_screen_height(screen),
                   STRLIT_ARGS(""),
                   color,
                   border);
    nc_scrollpad_init(&screen->scrollpad,
                      nc_window_height(&screen->window));
    return;
}

void
nc_song_info_screen_set_geometry(NcSongInfoScreen *screen,
                                 int64 start_x, int64 width,
                                 int64 main_start_y,
                                 int64 main_height) {
    nc_scrollpad_screen_set_main_area(&screen->scrollpad_screen,
                                      start_x,
                                      width,
                                      main_start_y,
                                      main_height);
    return;
}

bool
nc_song_info_screen_prepare_current(NcSongInfoScreen *screen) {
    NcBuffer next_buffer;

    if (screen->hooks.render == NULL) {
        return false;
    }

    nc_buffer_init(&next_buffer);
    if (!screen->hooks.render(screen->hooks.user, screen, &next_buffer)) {
        nc_buffer_destroy(&next_buffer);
        return false;
    }

    nc_buffer_destroy(&screen->buffer);
    nc_buffer_move(&screen->buffer, &next_buffer);
    nc_scrollpad_reset(&screen->scrollpad);
    nc_window_clear(&screen->window);
    nc_scrollpad_flush(&screen->scrollpad,
                       &screen->window,
                       &screen->buffer);
    nc_scrollpad_refresh(&screen->scrollpad, &screen->window);
    return true;
}

NcScreen *
nc_song_info_screen_base(NcSongInfoScreen *screen) {
    return nc_scrollpad_screen_base(&screen->scrollpad_screen);
}

int64
nc_song_info_screen_start_x(NcSongInfoScreen *screen) {
    return nc_scrollpad_screen_start_x(&screen->scrollpad_screen);
}

int64
nc_song_info_screen_start_y(NcSongInfoScreen *screen) {
    return nc_scrollpad_screen_start_y(&screen->scrollpad_screen);
}

int64
nc_song_info_screen_width(NcSongInfoScreen *screen) {
    return nc_scrollpad_screen_width(&screen->scrollpad_screen);
}

int64
nc_song_info_screen_height(NcSongInfoScreen *screen) {
    return nc_scrollpad_screen_height(&screen->scrollpad_screen);
}

static NcSongInfoScreen *
nc_song_info_from_screen(NcScreen *screen) {
    return (NcSongInfoScreen *)screen;
}

static NcScreenCallbacks
nc_song_info_callbacks(void) {
    NcScreenCallbacks callbacks = {0};

    callbacks.active_window = nc_song_info_active_window;
    callbacks.refresh = nc_song_info_refresh;
    callbacks.refresh_window = nc_song_info_refresh_window;
    callbacks.scroll = nc_song_info_scroll;
    callbacks.switch_to = nc_song_info_switch_to;
    callbacks.resize = nc_song_info_resize;
    callbacks.window_timeout = nc_song_info_window_timeout;
    callbacks.title = nc_song_info_title;
    callbacks.update = nc_song_info_update;
    callbacks.mouse_button_pressed = nc_song_info_mouse_button_pressed;
    callbacks.is_lockable = nc_song_info_is_lockable;
    callbacks.is_mergable = nc_song_info_is_mergable;
    callbacks.destroy = nc_song_info_destroy_callback;
    return callbacks;
}

static NcWindow *
nc_song_info_active_window(NcScreen *screen) {
    NcSongInfoScreen *song_info;

    song_info = nc_song_info_from_screen(screen);
    return &song_info->window;
}

static void
nc_song_info_refresh(NcScreen *screen) {
    nc_song_info_display(nc_song_info_from_screen(screen));
    return;
}

static void
nc_song_info_refresh_window(NcScreen *screen) {
    nc_song_info_display(nc_song_info_from_screen(screen));
    return;
}

static void
nc_song_info_scroll(NcScreen *screen, enum NcScroll where) {
    NcSongInfoScreen *song_info;

    song_info = nc_song_info_from_screen(screen);
    nc_scrollpad_scroll(&song_info->scrollpad, &song_info->window, where);
    return;
}

static void
nc_song_info_switch_to(NcScreen *screen) {
    NcSongInfoScreen *song_info;

    song_info = nc_song_info_from_screen(screen);
    if (song_info->hooks.switch_to) {
        song_info->hooks.switch_to(song_info->hooks.user, song_info);
    }
    return;
}

static void
nc_song_info_resize(NcScreen *screen) {
    NcSongInfoScreen *song_info;

    song_info = nc_song_info_from_screen(screen);
    if (song_info->hooks.resize_layout) {
        song_info->hooks.resize_layout(song_info->hooks.user, song_info);
    }
    nc_scrollpad_resize(&song_info->scrollpad,
                        &song_info->window,
                        nc_song_info_screen_width(song_info),
                        nc_song_info_screen_height(song_info));
    nc_window_move_to(&song_info->window,
                      nc_song_info_screen_start_x(song_info),
                      nc_song_info_screen_start_y(song_info));
    nc_scrollpad_flush(&song_info->scrollpad,
                       &song_info->window,
                       &song_info->buffer);
    return;
}

static int32
nc_song_info_window_timeout(NcScreen *screen) {
    (void)screen;
    return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
}

static char *
nc_song_info_title(NcScreen *screen) {
    static char title[] = "Song info";

    (void)screen;
    return title;
}

static void
nc_song_info_update(NcScreen *screen) {
    (void)screen;
    return;
}

static void
nc_song_info_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    NcSongInfoScreen *song_info;
    enum NcScroll where = NC_SCROLL_HOME;
    bool do_scroll;

    song_info = nc_song_info_from_screen(screen);
    do_scroll = true;
    if (event.bstate & BUTTON5_PRESSED) {
        where = NC_SCROLL_DOWN;
    } else if (event.bstate & BUTTON4_PRESSED) {
        where = NC_SCROLL_UP;
    } else {
        do_scroll = false;
    }

    if (do_scroll) {
        for (int64 i = 0; i < song_info->lines_scrolled; i += 1) {
            nc_scrollpad_scroll(&song_info->scrollpad,
                                &song_info->window,
                                where);
        }
    }
    return;
}

static bool
nc_song_info_is_lockable(NcScreen *screen) {
    (void)screen;
    return false;
}

static bool
nc_song_info_is_mergable(NcScreen *screen) {
    (void)screen;
    return true;
}

static void
nc_song_info_destroy_callback(NcScreen *screen) {
    NcSongInfoScreen *song_info;

    song_info = nc_song_info_from_screen(screen);
    if (song_info->hooks.destroy) {
        song_info->hooks.destroy(song_info->hooks.user);
    }
    return;
}

static void
nc_song_info_display(NcSongInfoScreen *song_info) {
    nc_scrollpad_refresh(&song_info->scrollpad, &song_info->window);
    return;
}

#endif /* NCMPCPP_NC_SONG_INFO_C */
