#include "screens/nc_playlist.h"

#include <stdio.h>
#include <stddef.h>

#include "app_controller.h"
#include "global.h"
#include "settings.h"
#include "statusbar.h"
#include "c/ncm_base.h"
#include "c/ncm_display.h"
#include "cbase/base_macros.h"

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

static NativePlaylistScreen *native_playlist_from_screen(NcScreen *screen);
static NcScreenCallbacks native_playlist_callbacks(void);
static NcWindow *native_playlist_active_window(NcScreen *screen);
static void native_playlist_refresh(NcScreen *screen);
static void native_playlist_refresh_window(NcScreen *screen);
static void native_playlist_scroll(NcScreen *screen, enum NcScroll where);
static void native_playlist_switch_to(NcScreen *screen);
static void native_playlist_resize(NcScreen *screen);
static int32 native_playlist_window_timeout(NcScreen *screen);
static char *native_playlist_title(NcScreen *screen);
static void native_playlist_update(NcScreen *screen);
static void native_playlist_mouse_button_pressed(NcScreen *screen,
                                                 MEVENT event);
static bool native_playlist_is_lockable(NcScreen *screen);
static bool native_playlist_is_mergable(NcScreen *screen);
static void native_playlist_destroy_callback(NcScreen *screen);
static NcMenuDisplayCallbacks native_playlist_display_callbacks(void);
static NcMenuActionCallbacks native_playlist_action_callbacks(void);
static void native_playlist_draw_song(NcMenu *menu, NcWindow *window,
                                      void *item, int64 pos, void *user);
static void native_playlist_activate_song(NcMenu *menu, void *item,
                                          int64 pos, void *user);
static void native_playlist_print_buffer(NcWindow *window, NcBuffer *buffer);
static void native_playlist_sync_if_needed(NativePlaylistScreen *screen);
static void native_playlist_refresh_stats(NativePlaylistScreen *screen);
static bool native_playlist_copy_one_song(NativePlaylistScreen *screen,
                                          NcmSong *song);
static bool native_playlist_append_selected(NcMenu *menu,
                                            NcmSongArray *songs);
static bool native_playlist_append_position(NcMenu *menu, int64 pos,
                                            NcmSongArray *songs);
static bool native_playlist_set_one_priority(NcmSong *song, int32 idx,
                                             void *user);

typedef struct NativePlaylistPriorityContext {
    NcmMpdClient *client;
    NcmError *error;
    int32 priority;
} NativePlaylistPriorityContext;

void
native_playlist_screen_init(NativePlaylistScreen *screen, int64 start_x,
                            int64 width, int64 main_start_y,
                            int64 main_height, NcColor color,
                            NcBorder border) {
    NcScreenCallbacks callbacks;

    callbacks = native_playlist_callbacks();
    nc_song_menu_init(&screen->songs);
    nc_window_init(&screen->window, start_x, main_start_y, width,
                   main_height, (char *)"", 0, color, border);
    ncm_buffer_init(&screen->title_cache);
    ncm_buffer_init(&screen->search_constraint);
    screen->total_length = 0;
    screen->remaining_time = 0;
    screen->scroll_begin = 0;
    screen->highlight_timer.ns = 0;
    screen->sync = NULL;
    screen->sync_user = NULL;
    screen->bridge.active_window = NULL;
    screen->bridge.refresh = NULL;
    screen->bridge.refresh_window = NULL;
    screen->bridge.resize = NULL;
    screen->bridge.user = NULL;
    screen->reload_total_length = true;
    screen->reload_remaining = true;
    screen->registered = false;
    screen->syncing = false;
    screen->highlighting_requested = false;
    nc_playlist_screen_init(&screen->screen, callbacks, screen,
                            nc_song_menu_base(&screen->songs), start_x,
                            width, main_start_y, main_height);
    nc_menu_set_display_callbacks(native_playlist_screen_menu(screen),
                                  native_playlist_display_callbacks());
    nc_menu_set_action_callbacks(native_playlist_screen_menu(screen),
                                 native_playlist_action_callbacks());
    nc_menu_set_selected_prefix(native_playlist_screen_menu(screen),
                                &Config.selected_item_prefix);
    nc_menu_set_selected_suffix(native_playlist_screen_menu(screen),
                                &Config.selected_item_suffix);
    nc_menu_set_highlight_prefix(native_playlist_screen_menu(screen),
                                 &Config.current_item_prefix);
    nc_menu_set_highlight_suffix(native_playlist_screen_menu(screen),
                                 &Config.current_item_suffix);
    nc_menu_set_cyclic_scrolling(native_playlist_screen_menu(screen),
                                 Config.use_cyclic_scrolling);
    nc_menu_set_centered_cursor(native_playlist_screen_menu(screen),
                                Config.centered_cursor);
    return;
}

void
native_playlist_screen_destroy(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return;
    }
    (void)native_playlist_screen_unregister(screen);
    nc_playlist_screen_set_menu(&screen->screen, NULL);
    nc_window_destroy(&screen->window);
    nc_song_menu_destroy(&screen->songs);
    ncm_buffer_destroy(&screen->title_cache);
    ncm_buffer_destroy(&screen->search_constraint);
    return;
}

bool
native_playlist_screen_register(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (screen->registered) {
        return true;
    }
    if (!app_controller_register_screen(native_playlist_screen_base(screen))) {
        return false;
    }
    screen->registered = true;
    return true;
}

bool
native_playlist_screen_unregister(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (!screen->registered) {
        return true;
    }
    if (!app_controller_unregister_screen(
            native_playlist_screen_base(screen))) {
        return false;
    }
    screen->registered = false;
    return true;
}

NcScreen *
native_playlist_screen_base(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return nc_playlist_screen_base(&screen->screen);
}

NcPlaylistScreen *
native_playlist_screen_playlist(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->screen;
}

NcSongMenu *
native_playlist_screen_song_menu(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->songs;
}

NcMenu *
native_playlist_screen_menu(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return nc_song_menu_base(&screen->songs);
}

NcWindow *
native_playlist_screen_window(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->window;
}

void
native_playlist_screen_set_geometry(NativePlaylistScreen *screen,
                                    int64 start_x, int64 width,
                                    int64 main_start_y, int64 main_height) {
    if (screen == NULL) {
        return;
    }
    nc_playlist_screen_set_geometry(&screen->screen, start_x, width,
                                    main_start_y, main_height);
    nc_window_resize(&screen->window, width, main_height);
    nc_window_move_to(&screen->window, start_x, main_start_y);
    return;
}

void
native_playlist_screen_set_mouse_config(NativePlaylistScreen *screen,
                                        int64 lines_scrolled,
                                        bool scroll_whole_page) {
    if (screen == NULL) {
        return;
    }
    nc_playlist_screen_set_mouse_config(&screen->screen, lines_scrolled,
                                        scroll_whole_page);
    return;
}

void
native_playlist_screen_set_sync_callback(NativePlaylistScreen *screen,
                                         NativePlaylistSync *sync,
                                         void *user) {
    if (screen == NULL) {
        return;
    }
    screen->sync = sync;
    screen->sync_user = user;
    return;
}

void
native_playlist_screen_set_bridge(NativePlaylistScreen *screen,
                                  NativePlaylistBridge bridge) {
    if (screen == NULL) {
        return;
    }
    screen->bridge = bridge;
    return;
}

void
native_playlist_screen_set_display_menu(NativePlaylistScreen *screen,
                                        NcMenu *menu) {
    if (screen == NULL) {
        return;
    }
    if (menu == NULL) {
        menu = nc_song_menu_base(&screen->songs);
    }
    nc_playlist_screen_set_menu(&screen->screen, menu);
    return;
}

void
native_playlist_screen_sync(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return;
    }
    native_playlist_sync_if_needed(screen);
    return;
}

void
native_playlist_screen_set_highlighting(NativePlaylistScreen *screen,
                                        bool enabled) {
    if (screen == NULL) {
        return;
    }
    nc_menu_set_highlighting(native_playlist_screen_menu(screen), enabled);
    if (enabled) {
        screen->highlight_timer.ns = 0;
    }
    return;
}

bool
native_playlist_screen_highlighting(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return nc_menu_highlight_enabled(native_playlist_screen_menu(screen));
}

void
native_playlist_screen_request_highlighting(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->highlighting_requested = true;
    native_playlist_screen_set_highlighting(screen, true);
    return;
}

bool
native_playlist_screen_consume_highlighting_request(
    NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (!screen->highlighting_requested) {
        return false;
    }
    screen->highlighting_requested = false;
    return true;
}

void
native_playlist_screen_clear(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_clear_items(native_playlist_screen_menu(screen));
    native_playlist_screen_reload_total_length(screen);
    native_playlist_screen_reload_remaining(screen);
    return;
}

bool
native_playlist_screen_add_song_copy(NativePlaylistScreen *screen,
                                     NcmSong *song) {
    if (screen == NULL) {
        return false;
    }
    if (song == NULL) {
        return false;
    }
    nc_song_menu_add(native_playlist_screen_song_menu(screen), song);
    native_playlist_screen_reload_total_length(screen);
    native_playlist_screen_reload_remaining(screen);
    return true;
}

void
native_playlist_screen_add_song_move(NativePlaylistScreen *screen,
                                     NcmSong *song) {
    NcmSong copy;

    if (screen == NULL) {
        return;
    }
    if (song == NULL) {
        return;
    }

    ncm_song_init(&copy);
    ncm_song_move(&copy, song);
    native_playlist_screen_add_song_copy(screen, &copy);
    ncm_song_destroy(&copy);
    return;
}

bool
native_playlist_screen_load_song_list(NativePlaylistScreen *screen,
                                      NcmMpdSongList *songs) {
    if (screen == NULL) {
        return false;
    }
    if (songs == NULL) {
        return false;
    }

    native_playlist_screen_clear(screen);
    for (int32 i = 0; i < songs->count; i += 1) {
        if (!native_playlist_copy_one_song(screen, &songs->items[i])) {
            native_playlist_screen_clear(screen);
            return false;
        }
    }
    return true;
}

bool
native_playlist_screen_reload_from_mpd(NativePlaylistScreen *screen,
                                       NcmMpdClient *client,
                                       uint32 version,
                                       NcmError *error) {
    NcmMpdSongList songs;
    bool result;

    if (screen == NULL) {
        ncm_error_set(error, -1, STRLIT_ARGS("playlist screen is NULL"));
        return false;
    }
    if (client == NULL) {
        ncm_error_set(error, -1, STRLIT_ARGS("MPD client is NULL"));
        return false;
    }

    ncm_mpd_song_list_init(&songs);
    result = ncm_mpd_client_get_queue_changes(client, version, &songs, error);
    if (result) {
        result = native_playlist_screen_load_song_list(screen, &songs);
        if (!result) {
            ncm_error_set(error, -1,
                          STRLIT_ARGS("could not copy playlist songs"));
        }
    }
    ncm_mpd_song_list_destroy(&songs);
    return result;
}

int64
native_playlist_screen_song_count(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return 0;
    }
    native_playlist_sync_if_needed(screen);
    return nc_menu_all_item_count(native_playlist_screen_menu(screen));
}

bool
native_playlist_screen_empty(NativePlaylistScreen *screen) {
    return native_playlist_screen_song_count(screen) == 0;
}

bool
native_playlist_screen_current_song(NativePlaylistScreen *screen,
                                    NcmSong *song) {
    NcmSong *current;

    if (screen == NULL) {
        return false;
    }
    if (song == NULL) {
        return false;
    }

    native_playlist_sync_if_needed(screen);
    current = nc_song_menu_current(native_playlist_screen_song_menu(screen));
    if (current == NULL) {
        return false;
    }
    return ncm_song_copy(song, current);
}

bool
native_playlist_screen_now_playing_song(NativePlaylistScreen *screen,
                                        int32 position, NcmSong *song) {
    NcmSong *item;

    if (screen == NULL) {
        return false;
    }
    if (song == NULL) {
        return false;
    }
    if (position < 0) {
        return false;
    }

    native_playlist_sync_if_needed(screen);
    item = nc_song_menu_item_at(native_playlist_screen_song_menu(screen),
                                NC_MENU_ITEMS_ALL, position);
    if (item == NULL) {
        return false;
    }
    return ncm_song_copy(song, item);
}

bool
native_playlist_screen_locate_position(NativePlaylistScreen *screen,
                                       uint32 position) {
    NcMenu *menu;
    NcmSong *song;

    if (screen == NULL) {
        return false;
    }

    native_playlist_sync_if_needed(screen);
    menu = native_playlist_screen_menu(screen);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        song = nc_menu_active_item_at(menu, i);
        if ((song != NULL) && (ncm_song_position(song) == position)) {
            nc_menu_highlight_position(menu, i,
                                       nc_playlist_screen_height(
                                           &screen->screen));
            return true;
        }
    }
    return false;
}

bool
native_playlist_screen_selected_songs(NativePlaylistScreen *screen,
                                      NcmSongArray *songs) {
    NcMenu *menu;

    if (songs == NULL) {
        return false;
    }
    ncm_song_array_clear(songs);
    if (screen == NULL) {
        return false;
    }

    native_playlist_sync_if_needed(screen);
    menu = native_playlist_screen_menu(screen);
    if (native_playlist_append_selected(menu, songs)) {
        return true;
    }
    if (nc_menu_item_count(menu) <= 0) {
        return true;
    }
    return native_playlist_append_position(menu, nc_menu_highlight(menu),
                                           songs);
}

bool
native_playlist_screen_select_current_if_none_selected(
    NativePlaylistScreen *screen) {
    NcMenu *menu;

    if (screen == NULL) {
        return false;
    }
    native_playlist_sync_if_needed(screen);
    menu = native_playlist_screen_menu(screen);
    if (nc_menu_has_selected(menu)) {
        return true;
    }
    return nc_menu_set_current_selected(menu, true);
}

void
native_playlist_screen_reverse_selection(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_reverse_selection(native_playlist_screen_menu(screen));
    return;
}

void
native_playlist_screen_clear_selection(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_clear_selection(native_playlist_screen_menu(screen));
    return;
}

bool
native_playlist_screen_set_selected_priority(NativePlaylistScreen *screen,
                                             NcmMpdClient *client,
                                             int32 priority,
                                             NcmError *error) {
    NativePlaylistPriorityContext context;
    NcmSongArray songs;
    bool result;

    if (screen == NULL) {
        return false;
    }
    if (client == NULL) {
        return false;
    }

    ncm_song_array_init(&songs);
    if (!native_playlist_screen_selected_songs(screen, &songs)) {
        ncm_song_array_destroy(&songs);
        return false;
    }

    context.client = client;
    context.error = error;
    context.priority = priority;
    result = true;
    for (int32 i = 0; i < songs.len; i += 1) {
        if (!native_playlist_set_one_priority(&songs.items[i], i,
                                              &context)) {
            result = false;
            break;
        }
    }

    ncm_song_array_destroy(&songs);
    return result;
}

void
native_playlist_screen_reload_total_length(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->reload_total_length = true;
    return;
}

void
native_playlist_screen_reload_remaining(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->reload_remaining = true;
    return;
}

static NativePlaylistScreen *
native_playlist_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcScreenCallbacks
native_playlist_callbacks(void) {
    NcScreenCallbacks callbacks = {0};

    callbacks.active_window = native_playlist_active_window;
    callbacks.refresh = native_playlist_refresh;
    callbacks.refresh_window = native_playlist_refresh_window;
    callbacks.scroll = native_playlist_scroll;
    callbacks.switch_to = native_playlist_switch_to;
    callbacks.resize = native_playlist_resize;
    callbacks.window_timeout = native_playlist_window_timeout;
    callbacks.title = native_playlist_title;
    callbacks.update = native_playlist_update;
    callbacks.mouse_button_pressed = native_playlist_mouse_button_pressed;
    callbacks.is_lockable = native_playlist_is_lockable;
    callbacks.is_mergable = native_playlist_is_mergable;
    callbacks.destroy = native_playlist_destroy_callback;
    return callbacks;
}

static NcWindow *
native_playlist_active_window(NcScreen *screen) {
    NativePlaylistScreen *playlist;

    playlist = native_playlist_from_screen(screen);
    if (playlist->bridge.active_window != NULL) {
        return playlist->bridge.active_window(playlist->bridge.user);
    }
    return native_playlist_screen_window(playlist);
}

static void
native_playlist_refresh(NcScreen *screen) {
    NativePlaylistScreen *playlist;
    NcMenu *menu;
    NcWindow *window;

    playlist = native_playlist_from_screen(screen);
    native_playlist_sync_if_needed(playlist);
    if (playlist->bridge.refresh != NULL) {
        playlist->bridge.refresh(playlist->bridge.user);
        return;
    }
    menu = nc_playlist_screen_menu(&playlist->screen);
    window = native_playlist_screen_window(playlist);
    nc_window_display(window);
    nc_menu_refresh(menu, window, nc_window_width(window),
                    nc_window_height(window));
    return;
}

static void
native_playlist_refresh_window(NcScreen *screen) {
    NativePlaylistScreen *playlist;

    playlist = native_playlist_from_screen(screen);
    native_playlist_sync_if_needed(playlist);
    if (playlist->bridge.refresh_window != NULL) {
        playlist->bridge.refresh_window(playlist->bridge.user);
        return;
    }
    native_playlist_refresh(screen);
    return;
}

static void
native_playlist_scroll(NcScreen *screen, enum NcScroll where) {
    NativePlaylistScreen *playlist;

    playlist = native_playlist_from_screen(screen);
    native_playlist_sync_if_needed(playlist);
    nc_playlist_screen_scroll(&playlist->screen, where);
    return;
}

static void
native_playlist_switch_to(NcScreen *screen) {
    NativePlaylistScreen *playlist;

    playlist = native_playlist_from_screen(screen);
    playlist->scroll_begin = 0;
    return;
}

static void
native_playlist_resize(NcScreen *screen) {
    NativePlaylistScreen *playlist;
    NcScreenResizeParams params;

    playlist = native_playlist_from_screen(screen);
    params = app_controller_screen_resize_params(screen, true);
    native_playlist_screen_set_geometry(playlist, params.x_offset,
                                        params.width,
                                        playlist->screen.main_start_y,
                                        playlist->screen.main_height);
    if (playlist->bridge.resize != NULL) {
        playlist->bridge.resize(playlist->bridge.user);
    }
    nc_screen_set_has_to_be_resized(screen, false);
    return;
}

static int32
native_playlist_window_timeout(NcScreen *screen) {
    (void)screen;
    return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
}

static char *
native_playlist_title(NcScreen *screen) {
    NativePlaylistScreen *playlist;

    playlist = native_playlist_from_screen(screen);
    native_playlist_sync_if_needed(playlist);
    native_playlist_refresh_stats(playlist);
    return playlist->title_cache.data;
}

static void
native_playlist_update(NcScreen *screen) {
    (void)screen;
    return;
}

static void
native_playlist_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    NativePlaylistScreen *playlist;

    playlist = native_playlist_from_screen(screen);
    native_playlist_sync_if_needed(playlist);
    nc_playlist_screen_mouse_button_pressed(&playlist->screen, event);
    return;
}

static bool
native_playlist_is_lockable(NcScreen *screen) {
    (void)screen;
    return true;
}

static bool
native_playlist_is_mergable(NcScreen *screen) {
    (void)screen;
    return true;
}

static void
native_playlist_destroy_callback(NcScreen *screen) {
    NativePlaylistScreen *playlist;

    playlist = native_playlist_from_screen(screen);
    native_playlist_screen_destroy(playlist);
    return;
}

static NcMenuDisplayCallbacks
native_playlist_display_callbacks(void) {
    NcMenuDisplayCallbacks callbacks = {0};

    callbacks.draw = native_playlist_draw_song;
    return callbacks;
}

static NcMenuActionCallbacks
native_playlist_action_callbacks(void) {
    NcMenuActionCallbacks callbacks = {0};

    callbacks.activate = native_playlist_activate_song;
    return callbacks;
}

static void
native_playlist_draw_song(NcMenu *menu, NcWindow *window, void *item,
                          int64 pos, void *user) {
    NcBuffer buffer;
    NcmFormatAst *format;

    (void)menu;
    (void)pos;
    (void)user;
    if (item == NULL) {
        return;
    }

    nc_buffer_init(&buffer);
    if (Config.playlist_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
        format = &Config.song_columns_mode_format;
    } else {
        format = &Config.song_list_format;
    }
    ncm_display_song_row(&buffer, format, item, 0);
    native_playlist_print_buffer(window, &buffer);
    nc_buffer_destroy(&buffer);
    return;
}

static void
native_playlist_activate_song(NcMenu *menu, void *item, int64 pos,
                              void *user) {
    NcmError error = {0};

    (void)menu;
    (void)pos;
    (void)user;
    if (item == NULL) {
        return;
    }
    if (!ncm_mpd_client_play_id(&global_mpd, (int32)ncm_song_id(item),
                                &error)) {
        ncm_statusbar_print_cstring(1, error.message);
    }
    return;
}

static void
native_playlist_print_buffer(NcWindow *window, NcBuffer *buffer) {
    NcBufferProperty *properties;
    char *data;
    int32 property_count;
    int32 property_index;
    int32 len;

    data = nc_buffer_data(buffer);
    len = nc_buffer_len(buffer);
    properties = nc_buffer_properties(buffer);
    property_count = nc_buffer_property_count(buffer);
    property_index = 0;

    for (int32 i = 0;; i += 1) {
        while ((property_index < property_count)
               && (properties[property_index].position == i)) {
            nc_buffer_apply_property(window, &properties[property_index]);
            property_index += 1;
        }
        if (i >= len) {
            break;
        }
        nc_window_print_char(window, data[i]);
    }
    return;
}

static void
native_playlist_sync_if_needed(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return;
    }
    if (screen->sync == NULL) {
        return;
    }
    if (screen->syncing) {
        return;
    }

    screen->syncing = true;
    screen->sync(screen->sync_user);
    screen->syncing = false;
    return;
}

static void
native_playlist_refresh_stats(NativePlaylistScreen *screen) {
    int64 count;

    ncm_buffer_clear(&screen->title_cache);
    ncm_buffer_append(&screen->title_cache, STRLIT_ARGS("Playlist ("));
    count = native_playlist_screen_song_count(screen);
    {
        char count_buffer[64];
        int32 count_len;

        count_len = snprintf(count_buffer, sizeof(count_buffer),
                             "%lld", (llong)count);
        ncm_buffer_append(&screen->title_cache, count_buffer, count_len);
    }
    if (count == 1) {
        ncm_buffer_append(&screen->title_cache, STRLIT_ARGS(" item)"));
    } else {
        ncm_buffer_append(&screen->title_cache, STRLIT_ARGS(" items)"));
    }
    return;
}

static bool
native_playlist_copy_one_song(NativePlaylistScreen *screen, NcmSong *song) {
    return native_playlist_screen_add_song_copy(screen, song);
}

static bool
native_playlist_append_selected(NcMenu *menu, NcmSongArray *songs) {
    bool found;

    found = false;
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (nc_menu_position_is_selected(menu, i)) {
            if (!native_playlist_append_position(menu, i, songs)) {
                ncm_song_array_clear(songs);
                return false;
            }
            found = true;
        }
    }
    return found;
}

static bool
native_playlist_append_position(NcMenu *menu, int64 pos,
                                NcmSongArray *songs) {
    NcmSong *song;

    song = nc_menu_active_item_at(menu, pos);
    if (song == NULL) {
        return false;
    }
    return ncm_song_array_append_copy(songs, song);
}

static bool
native_playlist_set_one_priority(NcmSong *song, int32 idx, void *user) {
    NativePlaylistPriorityContext *context;

    (void)idx;
    context = user;
    return ncm_mpd_client_set_priority_song(context->client, song,
                                            (uint32)context->priority,
                                            context->error);
}
