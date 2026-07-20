#if !defined(NCMPCPP_NC_PLAYLIST_C)
#define NCMPCPP_NC_PLAYLIST_C

#include "screens/nc_playlist.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#include "app_controller.h"
#include "global.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "c/ncm_base.h"
#include "c/ncm_display.h"
#include "c/ncm_type_conversions.h"
#include "cbase/utf8.c"
#include "cbase/base_macros.h"
#include "cbase/util.c"

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
static bool native_playlist_song_is_now_playing(NcmSong *song);
static NcMenu *native_playlist_storage_menu(NativePlaylistScreen *screen);
static bool native_playlist_build_mutable_song(
    NcmSong *replacement, NcmSong *current, NcmMutableSong *edited);
static bool native_playlist_set_mutable_uri(
    NcmSong *song, NcmMutableSong *edited);
static void native_playlist_refresh_stats(NativePlaylistScreen *screen);
static bool native_playlist_truncate_storage(NativePlaylistScreen *screen,
                                             uint32 playlist_length);
static bool native_playlist_apply_changed_song_to_storage(
    NativePlaylistScreen *screen, NcmSong *song);
static bool native_playlist_apply_changed_songs(
    NativePlaylistScreen *screen, NcmMpdSongList *songs,
    uint32 playlist_length);
static bool native_playlist_should_reload_full(
    NativePlaylistScreen *screen, uint32 version,
    uint32 playlist_length, NcmMpdSongList *changes);
static bool native_playlist_append_selected(NcMenu *menu,
                                            NcmSongArray *songs);
static bool native_playlist_append_position(NcMenu *menu, int64 pos,
                                            NcmSongArray *songs);
static bool native_playlist_set_one_priority(NcmSong *song, int32 idx,
                                             void *user);
static bool native_playlist_filter_song(NcMenu *menu, void *item,
                                        void *user);
static bool native_playlist_song_matches(NativePlaylistScreen *screen,
                                         NcmSong *song, NcmRegex *regex);
static bool native_playlist_search_menu(NativePlaylistScreen *screen,
                                        NcMenu *menu, NcmRegex *regex,
                                        bool forward, bool wrap,
                                        bool skip_current);
static bool native_playlist_search_position(NcMenu *menu, int64 pos,
                                            void *user);

typedef struct NativePlaylistSearchContext {
    NativePlaylistScreen *screen;
    NcmRegex *regex;
} NativePlaylistSearchContext;

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
                   main_height, "", 0, color, border);
    ncm_buffer_init(&screen->title_cache);
    ncm_buffer_init(&screen->column_title);
    ncm_buffer_init(&screen->filter_constraint);
    ncm_buffer_init(&screen->search_constraint);
    ncm_regex_init(&screen->filter_regex);
    screen->total_length = 0;
    screen->remaining_time = 0;
    screen->scroll_begin = 0;
    screen->highlight_timer.ns = 0;
    screen->reload_total_length = true;
    screen->reload_remaining = true;
    screen->registered = false;
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
    native_playlist_screen_set_mouse_config(
        screen, Config.lines_scrolled,
        Config.mouse_list_scroll_whole_page);
    native_playlist_screen_update_column_title(screen);
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
    ncm_buffer_destroy(&screen->column_title);
    ncm_buffer_destroy(&screen->filter_constraint);
    ncm_buffer_destroy(&screen->search_constraint);
    ncm_regex_destroy(&screen->filter_regex);
    return;
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
native_playlist_screen_update_column_title(NativePlaylistScreen *screen) {
    int32 list_width;

    if (screen == NULL) {
        return;
    }

    ncm_buffer_clear(&screen->column_title);
    if ((Config.playlist_display_mode != NCM_DISPLAY_MODE_COLUMNS)
        || !Config.titles_visibility || (Config.columns.items == NULL)
        || (Config.columns.len <= 0) || (screen->screen.main_height <= 2)) {
        nc_window_set_title(&screen->window, NULL, 0);
        return;
    }

    if (screen->screen.width > INT32_MAX) {
        list_width = INT32_MAX;
    } else {
        list_width = (int32)screen->screen.width;
    }
    if (list_width <= 0) {
        nc_window_set_title(&screen->window, NULL, 0);
        return;
    }

    ncm_display_column_title(&screen->column_title, Config.columns.items,
                             Config.columns.len, list_width);
    nc_window_set_title(&screen->window, screen->column_title.data,
                        screen->column_title.len);
    return;
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
    native_playlist_screen_update_column_title(screen);
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
native_playlist_screen_set_highlighting(NativePlaylistScreen *screen,
                                        bool enabled) {
    bool was_enabled;

    if (screen == NULL) {
        return;
    }

    was_enabled = native_playlist_screen_highlighting(screen);
    nc_menu_set_highlighting(native_playlist_screen_menu(screen), enabled);
    if (enabled && !was_enabled) {
        screen->highlight_timer = global_timer;
    } else if (!enabled) {
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
    screen->highlight_timer = global_timer;
    return;
}

void
native_playlist_screen_clear(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_clear_items(native_playlist_storage_menu(screen));
    native_playlist_screen_reload_total_length(screen);
    native_playlist_screen_reload_remaining(screen);
    return;
}

bool
native_playlist_screen_reload_from_mpd(NativePlaylistScreen *screen,
                                       NcmMpdClient *client,
                                       uint32 version,
                                       uint32 playlist_length,
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
    if (native_playlist_should_reload_full(screen, version,
                                           playlist_length, NULL)) {
        result = ncm_mpd_client_get_queue(client, &songs, error);
    } else {
        result = ncm_mpd_client_get_queue_changes(client, version, &songs,
                                                  error);
        if (result
            && native_playlist_should_reload_full(screen, version,
                                                  playlist_length,
                                                  &songs)) {
            result = ncm_mpd_client_get_queue(client, &songs, error);
        }
    }

    if (result) {
        result = native_playlist_apply_changed_songs(
            screen, &songs, playlist_length);
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
    return nc_menu_all_item_count(native_playlist_storage_menu(screen));
}

bool
native_playlist_screen_empty(NativePlaylistScreen *screen) {
    return native_playlist_screen_song_count(screen) <= 0;
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

    current = nc_song_menu_current(native_playlist_screen_song_menu(screen));
    if (current == NULL) {
        return false;
    }
    return ncm_song_copy(song, current);
}

bool
native_playlist_screen_update_current_mutable_song(
    NativePlaylistScreen *screen, NcmMutableSong *song) {
    NcmSong replacement;
    NcmSong *current;
    NcMenu *menu;
    bool was_filtered;

    if ((screen == NULL) || (song == NULL)) {
        return false;
    }

    menu = native_playlist_storage_menu(screen);
    current = nc_menu_current_item(menu);
    if (current == NULL) {
        return false;
    }

    ncm_song_init(&replacement);
    if (!native_playlist_build_mutable_song(&replacement, current, song)) {
        ncm_song_destroy(&replacement);
        return false;
    }

    was_filtered = nc_menu_is_filtered(menu);
    ncm_song_move(current, &replacement);
    if (was_filtered) {
        nc_menu_apply_filter(menu);
    }
    native_playlist_screen_reload_total_length(screen);
    native_playlist_screen_reload_remaining(screen);
    return true;
}

bool
native_playlist_screen_now_playing_song(NativePlaylistScreen *screen,
                                        int32 position, NcmSong *song) {
    NcMenu *base;
    NcSongMenu *menu;
    NcmSong *item;
    int64 count;
    int32 queue_position;

    if (screen == NULL) {
        return false;
    }
    if (song == NULL) {
        return false;
    }
    if (position < 0) {
        return false;
    }

    menu = native_playlist_screen_song_menu(screen);
    base = nc_song_menu_base(menu);
    count = nc_menu_all_item_count(base);
    queue_position = position;

    if ((int64)position < count) {
        item = nc_song_menu_item_at(menu, NC_MENU_ITEMS_ALL, position);
        if (item
            && (ncm_song_position(item) == queue_position)) {
            return ncm_song_copy(song, item);
        }
    }

    for (int64 i = 0; i < count; i += 1) {
        if (i == (int64)position) {
            continue;
        }
        item = nc_song_menu_item_at(menu, NC_MENU_ITEMS_ALL, i);
        if (item
            && (ncm_song_position(item) == queue_position)) {
            return ncm_song_copy(song, item);
        }
    }
    return false;
}

bool
native_playlist_screen_locate_position(NativePlaylistScreen *screen,
                                       int32 position) {
    NcMenu *menu;
    NcmSong *song;
    int64 height;

    if (screen == NULL) {
        return false;
    }

    menu = native_playlist_storage_menu(screen);
    height = nc_playlist_screen_height(&screen->screen);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        song = nc_menu_active_item_at(menu, i);
        if (song && (ncm_song_position(song) == position)) {
            nc_menu_highlight_position(menu, i, height);
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

    menu = native_playlist_storage_menu(screen);
    if (native_playlist_append_selected(menu, songs)) {
        return true;
    }
    if (nc_menu_item_count(menu) <= 0) {
        return true;
    }
    return native_playlist_append_position(menu, nc_menu_highlight(menu),
                                           songs);
}

static bool
native_playlist_screen_find_sort_range(
    NativePlaylistScreen *screen, int64 *first_position,
    int64 *last_position, int32 *start_position, NcmError *error) {
    NcMenu *menu;
    NcmSong *song;
    int64 first;
    int64 last;
    int32 range_start;

    if (screen == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing playlist screen"));
        return false;
    }

    menu = native_playlist_storage_menu(screen);
    last = nc_menu_all_item_count(menu);
    if (last <= 0) {
        ncm_error_set(error, ENOENT, STRLIT_ARGS("playlist is empty"));
        return false;
    }

    first = last;
    for (int64 i = 0; i < last; i += 1) {
        uint32 flags;

        flags = nc_menu_item_flags_at(menu, NC_MENU_ITEMS_ALL, i);
        if (flags & NC_MENU_ITEM_SELECTED) {
            first = i;
            break;
        }
    }
    if (first == last) {
        first = 0;
    } else {
        int64 selected_last;

        selected_last = first + 1;
        for (int64 i = first + 1; i < last; i += 1) {
            uint32 flags;

            flags = nc_menu_item_flags_at(menu, NC_MENU_ITEMS_ALL, i);
            if (flags & NC_MENU_ITEM_SELECTED) {
                selected_last = i + 1;
            }
        }
        for (int64 i = first; i < selected_last; i += 1) {
            uint32 flags;

            flags = nc_menu_item_flags_at(menu, NC_MENU_ITEMS_ALL, i);
            if (!(flags & NC_MENU_ITEM_SELECTED)) {
                ncm_error_set(
                    error, EINVAL,
                    STRLIT_ARGS("selected songs are not contiguous"));
                return false;
            }
        }
        last = selected_last;
    }

    song = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, first);
    if (song == NULL) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing playlist range song"));
        return false;
    }
    range_start = ncm_song_position(song);

    for (int64 i = first; i < last; i += 1) {
        int64 expected_position;

        song = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, i);
        if (song == NULL) {
            ncm_error_set(error, EINVAL,
                          STRLIT_ARGS("missing playlist range song"));
            return false;
        }
        expected_position = range_start + i - first;
        if ((expected_position > UINT32_MAX)
            || (ncm_song_position(song) != expected_position)) {
            ncm_error_set(
                error, EINVAL,
                STRLIT_ARGS("playlist range positions are not contiguous"));
            return false;
        }
    }

    if (first_position) {
        *first_position = first;
    }
    if (last_position) {
        *last_position = last;
    }
    if (start_position) {
        *start_position = range_start;
    }
    ncm_error_clear(error);
    return true;
}

bool
native_playlist_screen_has_sortable_range(
    NativePlaylistScreen *screen) {
    return native_playlist_screen_find_sort_range(
        screen, NULL, NULL, NULL, NULL);
}

bool
native_playlist_screen_copy_sort_range(
    NativePlaylistScreen *screen, NcmSongArray *songs,
    int32 *start_position, NcmError *error) {
    NcmSongArray replacement;
    NcMenu *menu;
    NcmSong *song;
    int64 first;
    int64 last;
    int32 range_start;

    if (songs == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing song array"));
        return false;
    }
    if (start_position == NULL) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing playlist range position"));
        return false;
    }
    if (!native_playlist_screen_find_sort_range(
            screen, &first, &last, &range_start, error)) {
        return false;
    }

    menu = native_playlist_storage_menu(screen);
    ncm_song_array_init(&replacement);
    for (int64 i = first; i < last; i += 1) {
        song = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, i);
        if (song == NULL) {
            ncm_error_set(error, EINVAL,
                          STRLIT_ARGS("missing playlist range song"));
            ncm_song_array_destroy(&replacement);
            return false;
        }
        if (!ncm_song_array_append_copy(&replacement, song)) {
            ncm_error_set(error, ENOMEM,
                          STRLIT_ARGS("could not copy playlist range"));
            ncm_song_array_destroy(&replacement);
            return false;
        }
    }

    ncm_song_array_destroy(songs);
    *songs = replacement;
    *start_position = range_start;
    ncm_error_clear(error);
    return true;
}

bool
native_playlist_screen_apply_filter(NativePlaylistScreen *screen,
                                    char *pattern, int32 pattern_len,
                                    NcmError *error) {
    NcMenuDisplayCallbacks callbacks;

    if (screen == NULL) {
        return false;
    }
    if ((pattern == NULL) || (pattern_len <= 0)) {
        native_playlist_screen_clear_filter(screen);
        return true;
    }
    if (!ncm_regex_compile(&screen->filter_regex, pattern, pattern_len,
                           Config.regex_flags, error)) {
        return false;
    }
    if (!ncm_buffer_set(&screen->filter_constraint, pattern, pattern_len)) {
        return false;
    }
    callbacks = native_playlist_display_callbacks();
    callbacks.filter = native_playlist_filter_song;
    callbacks.user = screen;
    nc_menu_set_display_callbacks(native_playlist_storage_menu(screen),
                                  callbacks);
    nc_menu_apply_filter(native_playlist_storage_menu(screen));
    return true;
}

void
native_playlist_screen_clear_filter(NativePlaylistScreen *screen) {
    if (screen == NULL) {
        return;
    }
    ncm_regex_destroy(&screen->filter_regex);
    ncm_regex_init(&screen->filter_regex);
    ncm_buffer_clear(&screen->filter_constraint);
    nc_menu_set_display_callbacks(native_playlist_storage_menu(screen),
                                  native_playlist_display_callbacks());
    nc_menu_show_all_items(native_playlist_storage_menu(screen));
    return;
}

bool
native_playlist_screen_search(NativePlaylistScreen *screen,
                              char *pattern, int32 pattern_len,
                              bool forward, bool wrap,
                              bool skip_current, NcmError *error) {
    NcmRegex regex;
    bool result;

    if ((screen == NULL) || (pattern == NULL) || (pattern_len <= 0)) {
        return false;
    }

    ncm_regex_init(&regex);
    if (!ncm_regex_compile(&regex, pattern, pattern_len,
                           Config.regex_flags, error)) {
        ncm_regex_destroy(&regex);
        return false;
    }
    if (!ncm_buffer_set(&screen->search_constraint, pattern, pattern_len)) {
        ncm_regex_destroy(&regex);
        return false;
    }

    result = native_playlist_search_menu(
        screen, native_playlist_storage_menu(screen), &regex, forward,
        wrap, skip_current);
    ncm_regex_destroy(&regex);
    return result;
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
    return native_playlist_screen_window(playlist);
}

static void
native_playlist_refresh(NcScreen *screen) {
    NativePlaylistScreen *playlist;
    NcMenu *menu;
    NcWindow *window;

    playlist = native_playlist_from_screen(screen);
    menu = nc_playlist_screen_menu(&playlist->screen);
    window = native_playlist_screen_window(playlist);
    nc_window_display(window);
    nc_menu_refresh(menu, window, nc_window_width(window),
                    nc_window_height(window));
    return;
}

static void
native_playlist_refresh_window(NcScreen *screen) {
    native_playlist_refresh(screen);
    return;
}

static void
native_playlist_scroll(NcScreen *screen, enum NcScroll where) {
    NativePlaylistScreen *playlist;

    playlist = native_playlist_from_screen(screen);
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
    native_playlist_refresh_stats(playlist);
    return playlist->title_cache.data;
}

static void
native_playlist_update(NcScreen *screen) {
    NativePlaylistScreen *playlist;
    uint32 delay;

    playlist = native_playlist_from_screen(screen);
    if (!native_playlist_screen_highlighting(playlist)) {
        return;
    }

    delay = Config.playlist_disable_highlight_delay_seconds;
    if ((delay == 0)
        || (global_timer_elapsed_seconds(playlist->highlight_timer)
            <= (int64)delay)) {
        return;
    }

    native_playlist_screen_set_highlighting(playlist, false);
    nc_screen_refresh(screen);
    return;
}

static void
native_playlist_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    NativePlaylistScreen *playlist;

    playlist = native_playlist_from_screen(screen);
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

static bool
native_playlist_filter_song(NcMenu *menu, void *item, void *user) {
    NativePlaylistScreen *screen;

    (void)menu;
    screen = user;
    return native_playlist_song_matches(screen, item,
                                        &screen->filter_regex);
}

static bool
native_playlist_song_matches(NativePlaylistScreen *screen,
                             NcmSong *song, NcmRegex *regex) {
    NcmBuffer buffer;
    bool result;

    (void)screen;
    if (song == NULL) {
        return false;
    }

    if (Config.playlist_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
        buffer = ncm_format_render_string(&Config.song_columns_mode_format,
                                          song);
    } else {
        buffer = ncm_format_render_string(&Config.song_list_format, song);
    }
    result = ncm_regex_search(regex, buffer.data, buffer.len);
    ncm_buffer_destroy(&buffer);
    return result;
}

static bool
native_playlist_search_menu(NativePlaylistScreen *screen,
                            NcMenu *menu, NcmRegex *regex,
                            bool forward, bool wrap,
                            bool skip_current) {
    NativePlaylistSearchContext context;

    context.screen = screen;
    context.regex = regex;
    return nc_menu_search_selectable(menu, screen->screen.main_height,
                                     forward, wrap, skip_current,
                                     native_playlist_search_position,
                                     &context, NULL);
}

static bool
native_playlist_search_position(NcMenu *menu, int64 pos,
                                void *user) {
    NativePlaylistSearchContext *context;

    context = user;
    return native_playlist_song_matches(
        context->screen, nc_menu_active_item_at(menu, pos),
        context->regex);
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
    bool is_now_playing;

    (void)user;
    if ((menu == NULL) || (window == NULL) || (item == NULL)) {
        return;
    }

    is_now_playing = native_playlist_song_is_now_playing(item);
    if (is_now_playing) {
        native_playlist_print_buffer(window, &Config.now_playing_prefix);
    }

    nc_buffer_init(&buffer);
    if (Config.playlist_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
        int64 available_width;
        int32 list_width;
        bool use_colors;

        available_width = nc_window_width(window) - nc_window_get_x(window);
        if (is_now_playing) {
            available_width -= utf8_width(
                Config.now_playing_suffix.data,
                Config.now_playing_suffix.len);
        }
        if (nc_menu_position_is_selected(menu, pos)) {
            available_width -= utf8_width(
                menu->selected_suffix.data, menu->selected_suffix.len);
        }
        if (menu->highlight_enabled && (pos == menu->highlight)) {
            available_width -= utf8_width(
                menu->highlight_suffix.data, menu->highlight_suffix.len);
        }
        if (available_width < 0) {
            available_width = 0;
        }
        if (available_width > INT32_MAX) {
            list_width = INT32_MAX;
        } else {
            list_width = (int32)available_width;
        }
        use_colors = !Config.discard_colors_if_item_is_selected
                     || !nc_menu_position_is_selected(menu, pos);
        ncm_display_song_columns(&buffer, item, Config.columns.items,
                                 Config.columns.len, list_width, use_colors);
    } else {
        ncm_display_song_row(&buffer, &Config.song_list_format, item,
                             NCM_FORMAT_FLAG_ALL);
    }
    native_playlist_print_buffer(window, &buffer);
    nc_buffer_destroy(&buffer);

    if (is_now_playing) {
        native_playlist_print_buffer(window, &Config.now_playing_suffix);
    }
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

static bool
native_playlist_song_is_now_playing(NcmSong *song) {
    int32 current_position;

    if ((song == NULL)
        || (ncm_status_state_player() == NCM_STATUS_PLAYER_STOP)) {
        return false;
    }

    current_position = ncm_status_state_current_song_position();
    if (current_position < 0) {
        return false;
    }
    return ncm_song_position(song) == current_position;
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

static NcMenu *
native_playlist_storage_menu(NativePlaylistScreen *screen) {
    return nc_song_menu_base(native_playlist_screen_song_menu(screen));
}

static bool
native_playlist_build_mutable_song(
    NcmSong *replacement, NcmSong *current, NcmMutableSong *edited) {
    NcmStringView value;
    enum NcmTagsField field;
    enum mpd_tag_type type;
    int64 mtime;
    uint32 duration;

    if ((replacement == NULL) || (current == NULL) || (edited == NULL)) {
        return false;
    }
    if (!native_playlist_set_mutable_uri(replacement, edited)) {
        return false;
    }

    for (int32 i = 0; i < current->tags_len; i += 1) {
        if (current->tags[i].type == MPD_TAG_UNKNOWN) {
            continue;
        }
        field = ncm_tags_field_from_tag_type(current->tags[i].type);
        if (field != NCM_TAGS_FIELD_LAST) {
            continue;
        }
        if (!ncm_song_add_tag(replacement, current->tags[i].type,
                              current->tags[i].value,
                              current->tags[i].value_len)) {
            return false;
        }
    }

    for (uint32 i = 0; i < NCM_TAGS_FIELD_LAST; i += 1) {
        type = ncm_tags_field_to_tag_type((enum NcmTagsField)i);
        for (int32 j = 0; ; j += 1) {
            if (!ncm_mutable_song_get_tag(edited,
                                          (enum NcmTagsField)i,
                                          j, &value)) {
                break;
            }
            if (value.len <= 0) {
                continue;
            }
            if (!ncm_song_add_tag(replacement, type,
                                  value.data, value.len)) {
                return false;
            }
        }
    }

    duration = ncm_mutable_song_duration(edited);
    if (duration == 0) {
        duration = ncm_song_duration(current);
    }
    mtime = ncm_mutable_song_mtime(edited);
    if (mtime <= 0) {
        mtime = (int64)ncm_song_mtime(current);
    }
    ncm_song_set_duration(replacement, duration);
    ncm_song_set_position(replacement, ncm_song_position(current));
    ncm_song_set_id(replacement, ncm_song_id(current));
    ncm_song_set_priority(replacement, ncm_song_priority(current));
    ncm_song_set_mtime(replacement, (time_t)mtime);
    return true;
}

static bool
native_playlist_set_mutable_uri(NcmSong *song, NcmMutableSong *edited) {
    NcmStringView new_name;
    NcmBuffer uri;
    bool result;

    if (!ncm_mutable_song_get_new_name(edited, &new_name)) {
        return ncm_song_set_uri(song, edited->uri, edited->uri_len);
    }

    ncm_buffer_init(&uri);
    if (edited->directory_len > 0) {
        ncm_buffer_append(&uri, edited->directory, edited->directory_len);
        if (edited->directory[edited->directory_len - 1] != '/') {
            ncm_buffer_append(&uri, STRLIT_ARGS("/"));
        }
    }
    ncm_buffer_append(&uri, new_name.data, new_name.len);
    result = ncm_song_set_uri(song, uri.data, uri.len);
    ncm_buffer_destroy(&uri);
    return result;
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

        count_len = SNPRINTF(count_buffer,
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
native_playlist_truncate_storage(NativePlaylistScreen *screen,
                                 uint32 playlist_length) {
    NcMenu *menu;
    int64 new_count;
    int64 old_count;
    if (screen == NULL) {
        return false;
    }

    menu = native_playlist_storage_menu(screen);
    new_count = (int64)playlist_length;
    old_count = nc_menu_all_item_count(menu);
    while (old_count > new_count) {
        old_count -= 1;
        if (!nc_menu_remove_item(menu, NC_MENU_ITEMS_ALL, old_count)) {
            return false;
        }
    }
    return true;
}

static bool
native_playlist_apply_changed_song_to_storage(NativePlaylistScreen *screen,
                                              NcmSong *song) {
    NcMenu *menu;
    uint32 position;

    if (screen == NULL) {
        return false;
    }
    if (song == NULL) {
        return false;
    }

    menu = native_playlist_storage_menu(screen);
    position = ncm_song_position(song);
    if (position < (uint32)nc_menu_all_item_count(menu)) {
        return nc_menu_replace_item(menu, NC_MENU_ITEMS_ALL,
                                    (int64)position, song);
    }

    nc_menu_add_item(menu, song);
    return true;
}

static bool
native_playlist_should_reload_full(NativePlaylistScreen *screen,
                                   uint32 version,
                                   uint32 playlist_length,
                                   NcmMpdSongList *changes) {
    int64 count;
    uint32 next_append_position;

    if (playlist_length == 0) {
        return false;
    }
    if (version == 0) {
        return true;
    }

    count = nc_menu_all_item_count(native_playlist_storage_menu(screen));
    if (count <= 0) {
        return true;
    }
    if (changes == NULL) {
        return false;
    }

    next_append_position = (uint32)count;
    for (int32 i = 0; i < changes->count; i += 1) {
        uint32 position;

        position = ncm_song_position(&changes->items[i]);
        if (position > next_append_position) {
            return true;
        }
        if (position == next_append_position) {
            next_append_position += 1;
        }
    }
    return false;
}

static bool
native_playlist_apply_changed_songs(NativePlaylistScreen *screen,
                                    NcmMpdSongList *songs,
                                    uint32 playlist_length) {
    NcMenu *menu;
    bool result;
    bool was_filtered;

    if (screen == NULL) {
        return false;
    }
    if (songs == NULL) {
        return false;
    }

    menu = native_playlist_storage_menu(screen);
    result = true;
    was_filtered = nc_menu_is_filtered(menu);

    if (result && !native_playlist_truncate_storage(screen,
                                                    playlist_length)) {
        result = false;
    }

    for (int32 i = 0; result && i < songs->count; i += 1) {
        if (!native_playlist_apply_changed_song_to_storage(
                screen, &songs->items[i])) {
            result = false;
            break;
        }
    }

    if (was_filtered) {
        nc_menu_apply_filter(menu);
    }
    if (!result) {
        return false;
    }

    native_playlist_screen_reload_total_length(screen);
    native_playlist_screen_reload_remaining(screen);
    return true;
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

#endif /* NCMPCPP_NC_PLAYLIST_C */
