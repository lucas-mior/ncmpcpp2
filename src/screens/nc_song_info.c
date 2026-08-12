#if !defined(NCMPCPP_NC_SONG_INFO_C)
#define NCMPCPP_NC_SONG_INFO_C

#include "cbase.h"

#include "screens/nc_screens.h"

static void nc_song_info_switch_to(NcScreen *screen);
static void nc_song_info_resize(NcScreen *screen);
static void nc_song_info_mouse_button_pressed(NcScreen *screen,
                                              MEVENT event);
static void nc_song_info_destroy_callback(NcScreen *screen);
static void nc_song_info_display(NcSongInfoScreen *song_info);

#define NC_SCREEN_IMPL_TYPE NcSongInfoScreen
#define NC_SCREEN_IMPL_PREFIX nc_song_info
#define NC_SCREEN_IMPL_PUBLIC_PREFIX nc_song_info_screen
#define NC_SCREEN_IMPL_BASE_FIELD scrollpad_screen
#define NC_SCREEN_IMPL_WINDOW_FIELD window
#define NC_SCREEN_IMPL_SCROLLPAD_FIELD scrollpad
#define NC_SCREEN_IMPL_REFRESH_CALLBACK nc_song_info_display
#define NC_SCREEN_IMPL_SWITCH_TO_CALLBACK nc_song_info_switch_to
#define NC_SCREEN_IMPL_RESIZE_CALLBACK nc_song_info_resize
#define NC_SCREEN_IMPL_TITLE_LITERAL "Song info"
#define NC_SCREEN_IMPL_MOUSE_CALLBACK nc_song_info_mouse_button_pressed
#define NC_SCREEN_IMPL_DESTROY_CALLBACK nc_song_info_destroy_callback
#define NC_SCREEN_IMPL_MERGABLE true
#include "screens/nc_screen_impl_template.h"

void
nc_song_info_screen_init(NcSongInfoScreen *screen,
                         NcSongInfoHooks hooks,
                         int32 start_x, int32 width,
                         int32 main_start_y, int32 main_height,
                         NcColor color, NcBorder border,
                         int32 lines_scrolled) {
    screen->hooks = hooks;
    screen->lines_scrolled = lines_scrolled;
    nc_scrollpad_screen_init(&screen->scrollpad_screen,
                             nc_song_info_ops,
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
                   STRLIT(""),
                   color,
                   border);
    nc_scrollpad_init(&screen->scrollpad,
                      nc_window_height(&screen->window));
    return;
}

void
nc_song_info_screen_set_geometry(NcSongInfoScreen *screen,
                                 int32 start_x, int32 width,
                                 int32 main_start_y,
                                 int32 main_height) {
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
        for (int32 i = 0; i < song_info->lines_scrolled; i += 1) {
            nc_scrollpad_scroll(&song_info->scrollpad,
                                &song_info->window,
                                where);
        }
    }
    return;
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
