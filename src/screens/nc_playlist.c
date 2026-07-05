#include "screens/nc_playlist.h"

#include <stddef.h>

static void playlist_scroll_lines(NcPlaylistScreen *screen,
                                  enum NcScroll where);

void
nc_playlist_screen_init(NcPlaylistScreen *screen,
                        NcScreenCallbacks callbacks, void *user,
                        NcMenu *menu, int64 start_x, int64 width,
                        int64 main_start_y, int64 main_height) {
    nc_screen_init(&screen->screen, callbacks, user, NC_SCREEN_TYPE_PLAYLIST);
    nc_playlist_screen_set_menu(screen, menu);
    nc_playlist_screen_set_geometry(screen, start_x, width, main_start_y,
                                    main_height);
    nc_playlist_screen_set_mouse_config(screen, 1, false);
    return;
}

void
nc_playlist_screen_set_geometry(NcPlaylistScreen *screen,
                                int64 start_x, int64 width,
                                int64 main_start_y, int64 main_height) {
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    return;
}

void
nc_playlist_screen_set_menu(NcPlaylistScreen *screen, NcMenu *menu) {
    screen->menu = menu;
    return;
}

void
nc_playlist_screen_set_mouse_config(NcPlaylistScreen *screen,
                                    int64 lines_scrolled,
                                    bool scroll_whole_page) {
    if (lines_scrolled < 1) {
        lines_scrolled = 1;
    }
    screen->lines_scrolled = lines_scrolled;
    screen->mouse_list_scroll_whole_page = scroll_whole_page;
    return;
}

NcScreen *
nc_playlist_screen_base(NcPlaylistScreen *screen) {
    return &screen->screen;
}

NcMenu *
nc_playlist_screen_menu(NcPlaylistScreen *screen) {
    return screen->menu;
}

int64
nc_playlist_screen_start_x(NcPlaylistScreen *screen) {
    return screen->start_x;
}

int64
nc_playlist_screen_start_y(NcPlaylistScreen *screen) {
    return screen->main_start_y;
}

int64
nc_playlist_screen_width(NcPlaylistScreen *screen) {
    return screen->width;
}

int64
nc_playlist_screen_height(NcPlaylistScreen *screen) {
    return screen->main_height;
}

void
nc_playlist_screen_scroll(NcPlaylistScreen *screen, enum NcScroll where) {
    if (screen->menu == NULL) {
        return;
    }
    nc_menu_scroll_selectable(screen->menu, screen->main_height, where);
    return;
}

bool
nc_playlist_screen_goto_y(NcPlaylistScreen *screen, int64 y) {
    if (screen->menu == NULL) {
        return false;
    }
    return nc_menu_goto_selectable(screen->menu, y);
}

bool
nc_playlist_screen_activate_current(NcPlaylistScreen *screen) {
    if (screen->menu == NULL) {
        return false;
    }
    return nc_menu_activate_current(screen->menu);
}

bool
nc_playlist_screen_activate_position(NcPlaylistScreen *screen, int64 pos) {
    if (screen->menu == NULL) {
        return false;
    }
    return nc_menu_activate_position(screen->menu, pos);
}

bool
nc_playlist_screen_set_current_selected(NcPlaylistScreen *screen,
                                        bool selected) {
    if (screen->menu == NULL) {
        return false;
    }
    return nc_menu_set_current_selected(screen->menu, selected);
}

bool
nc_playlist_screen_toggle_current_selected(NcPlaylistScreen *screen) {
    if (screen->menu == NULL) {
        return false;
    }
    return nc_menu_toggle_current_selected(screen->menu);
}

bool
nc_playlist_screen_set_position_selected(NcPlaylistScreen *screen,
                                         int64 pos, bool selected) {
    if (screen->menu == NULL) {
        return false;
    }
    return nc_menu_set_position_selected(screen->menu, pos, selected);
}

bool
nc_playlist_screen_toggle_position_selected(NcPlaylistScreen *screen,
                                            int64 pos) {
    if (screen->menu == NULL) {
        return false;
    }
    return nc_menu_toggle_position_selected(screen->menu, pos);
}

void
nc_playlist_screen_mouse_button_pressed(NcPlaylistScreen *screen,
                                        MEVENT event) {
    NcWindow *window;
    int32 x;
    int32 y;

    if (screen->menu == NULL) {
        return;
    }
    if (nc_menu_empty(screen->menu)) {
        return;
    }

    window = nc_screen_active_window(&screen->screen);
    if (window == NULL) {
        return;
    }

    x = event.x;
    y = event.y;
    if (!nc_window_has_coords(window, &x, &y)) {
        return;
    }

    if ((y >= 0)
        && (y < nc_menu_item_count(screen->menu))
        && (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))) {
        if (nc_playlist_screen_goto_y(screen, y)
            && (event.bstate & BUTTON3_PRESSED)) {
            nc_playlist_screen_activate_current(screen);
        }
        return;
    }

    if (event.bstate & BUTTON5_PRESSED) {
        if (screen->mouse_list_scroll_whole_page) {
            nc_playlist_screen_scroll(screen, NC_SCROLL_PAGE_DOWN);
        } else {
            playlist_scroll_lines(screen, NC_SCROLL_DOWN);
        }
    } else if (event.bstate & BUTTON4_PRESSED) {
        if (screen->mouse_list_scroll_whole_page) {
            nc_playlist_screen_scroll(screen, NC_SCROLL_PAGE_UP);
        } else {
            playlist_scroll_lines(screen, NC_SCROLL_UP);
        }
    }
    return;
}

static void
playlist_scroll_lines(NcPlaylistScreen *screen, enum NcScroll where) {
    for (int64 i = 0; i < screen->lines_scrolled; i += 1) {
        nc_playlist_screen_scroll(screen, where);
    }
    return;
}
