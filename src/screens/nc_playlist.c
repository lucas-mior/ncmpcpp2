#if !defined(NCMPCPP_NC_PLAYLIST_C)
#define NCMPCPP_NC_PLAYLIST_C

#include "cbase.h"

#include "app_controller.h"
#include "c/ncm_c.h"
#include "global.h"
#include "screens/nc_screens.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "ui_state.h"

static void playlist_scroll_lines(NcPlaylistScreen *screen,
                                  enum NcScroll where);

void
nc_playlist_screen_init(NcPlaylistScreen *screen,
                        NcScreenOps callbacks, void *user,
                        NcMenu *menu, int32 start_x, int32 width,
                        int32 main_start_y, int32 main_height) {
    nc_screen_init_ops(&screen->screen, callbacks, user,
                       NC_SCREEN_TYPE_PLAYLIST);
    nc_playlist_screen_set_menu(screen, menu);
    nc_playlist_screen_set_geometry(screen, start_x, width, main_start_y,
                                    main_height);
    nc_playlist_screen_set_mouse_config(screen, 1, false);
    return;
}

void
nc_playlist_screen_set_geometry(NcPlaylistScreen *screen,
                                int32 start_x, int32 width,
                                int32 main_start_y, int32 main_height) {
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
                                    int32 lines_scrolled,
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

int32
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
nc_playlist_screen_goto_y(NcPlaylistScreen *screen, int32 y) {
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

    if ((window = nc_screen_active_window(&screen->screen)) == NULL) {
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
    for (int32 i = 0; i < screen->lines_scrolled; i += 1) {
        nc_playlist_screen_scroll(screen, where);
    }
    return;
}

static void playlist_display(PlaylistScreen *screen);
static void playlist_switch_to(NcScreen *screen);
static void playlist_resize(NcScreen *screen);
static char *playlist_title(NcScreen *screen);
static void playlist_update(NcScreen *screen);
static void playlist_mouse_button_pressed(NcScreen *screen,
                                          MEVENT event);
static NcMenuDisplayCallbacks playlist_display_callbacks(void);
static NcMenuActionCallbacks playlist_action_callbacks(void);
static void playlist_draw_song(NcMenu *menu, NcWindow *window,
                               void *item, int32 pos, void *user);
static void playlist_activate_song(NcMenu *menu, void *item,
                                   int32 pos, void *user);
static void playlist_print_buffer(NcWindow *window, NcBuffer *buffer);
static bool playlist_song_is_now_playing(NcmSong *song);
static NcMenu *playlist_storage_menu(PlaylistScreen *screen);
static bool playlist_build_mutable_song(
    NcmSong *replacement, NcmSong *current, NcmMutableSong *edited);
static bool playlist_set_mutable_uri(
    NcmSong *song, NcmMutableSong *edited);
static void playlist_refresh_stats(PlaylistScreen *screen);
static bool playlist_truncate_storage(PlaylistScreen *screen,
                                      int32 playlist_length);
static bool playlist_apply_changed_song_to_storage(
    PlaylistScreen *screen, NcmSong *song);
static bool playlist_apply_changed_songs(
    PlaylistScreen *screen, NcmMpdSongList *songs,
    int32 playlist_length);
static bool playlist_should_reload_full(PlaylistScreen *screen,
                                        int32 version,
                                        int32 playlist_length,
                                        NcmMpdSongList *changes);
static bool playlist_append_selected(NcMenu *menu,
                                     NcmSongArray *songs);
static bool playlist_append_position(NcMenu *menu, int32 pos,
                                     NcmSongArray *songs);
static bool playlist_set_one_priority(NcmSong *song, int32 idx,
                                      void *user);
static bool playlist_filter_song(NcMenu *menu, void *item,
                                 void *user);
static bool playlist_song_matches(PlaylistScreen *screen,
                                  NcmSong *song, NcmRegex *regex);
static bool playlist_search_menu(PlaylistScreen *screen,
                                 NcMenu *menu, NcmRegex *regex,
                                 bool forward, bool wrap,
                                 bool skip_current);
static bool playlist_search_position(NcMenu *menu, int32 pos,
                                     void *user);

typedef struct PlaylistSearchContext {
    PlaylistScreen *screen;
    NcmRegex *regex;
} PlaylistSearchContext;

typedef struct PlaylistPriorityContext {
    NcmMpdClient *client;
    NcmError *ncm_error;
    int32 priority;
} PlaylistPriorityContext;

#define NC_SCREEN_IMPL_TYPE PlaylistScreen
#define NC_SCREEN_IMPL_PREFIX playlist
#define NC_SCREEN_IMPL_PUBLIC_PREFIX playlist_screen
#define NC_SCREEN_IMPL_BASE_FIELD screen
#define NC_SCREEN_IMPL_BASE_EXPR(playlist) \
    nc_playlist_screen_base(&(playlist)->screen)
#define NC_SCREEN_IMPL_WINDOW(playlist) playlist_screen_window(playlist)
#define NC_SCREEN_IMPL_MENU(playlist) \
    nc_playlist_screen_menu(&(playlist)->screen)
#define NC_SCREEN_IMPL_SCROLL_HEIGHT(playlist) ((playlist)->screen.main_height)
#define NC_SCREEN_IMPL_REFRESH_CALLBACK playlist_display
#define NC_SCREEN_IMPL_SWITCH_TO_CALLBACK playlist_switch_to
#define NC_SCREEN_IMPL_RESIZE_CALLBACK playlist_resize
#define NC_SCREEN_IMPL_TITLE_CALLBACK playlist_title
#define NC_SCREEN_IMPL_UPDATE_CALLBACK playlist_update
#define NC_SCREEN_IMPL_MOUSE_CALLBACK playlist_mouse_button_pressed
#define NC_SCREEN_IMPL_DESTROY_TYPED_CALLBACK playlist_screen_destroy
#define NC_SCREEN_IMPL_LOCKABLE true
#define NC_SCREEN_IMPL_MERGABLE true
#include "screens/nc_screen_impl_template.h"

void
playlist_screen_init(PlaylistScreen *screen, int32 start_x,
                     int32 width, int32 main_start_y,
                     int32 main_height, NcColor color,
                     NcBorder border) {
    nc_song_menu_init(&screen->songs);
    nc_window_init(&screen->window, start_x, main_start_y, width,
                   main_height, "", 0, color, border);

    screen->title_cache = (StrBuilder){0};
    screen->column_title = (StrBuilder){0};
    screen->filter_constraint = (StrBuilder){0};
    screen->search_constraint = (StrBuilder){0};

    screen->filter_regex = (NcmRegex){0};
    screen->total_length = 0;
    screen->remaining_time = 0;
    screen->scroll_begin = 0;
    screen->highlight_timer.ns = 0;
    screen->reload_total_length = true;
    screen->reload_remaining = true;
    screen->registered = false;
    screen->highlighting_requested = false;
    nc_playlist_screen_init(&screen->screen, playlist_ops, screen,
                            nc_song_menu_base(&screen->songs), start_x,
                            width, main_start_y, main_height);
    nc_menu_set_display_callbacks(playlist_screen_menu(screen),
                                  playlist_display_callbacks());
    nc_menu_set_action_callbacks(playlist_screen_menu(screen),
                                 playlist_action_callbacks());
    nc_menu_set_selected_prefix(playlist_screen_menu(screen),
                                &Config.selected_item_prefix);
    nc_menu_set_selected_suffix(playlist_screen_menu(screen),
                                &Config.selected_item_suffix);
    nc_menu_set_highlight_prefix(playlist_screen_menu(screen),
                                 &Config.current_item_prefix);
    nc_menu_set_highlight_suffix(playlist_screen_menu(screen),
                                 &Config.current_item_suffix);
    nc_menu_set_cyclic_scrolling(playlist_screen_menu(screen),
                                 Config.use_cyclic_scrolling);
    nc_menu_set_centered_cursor(playlist_screen_menu(screen),
                                Config.centered_cursor);
    playlist_screen_set_mouse_config(
        screen, Config.lines_scrolled,
        Config.mouse_list_scroll_whole_page);
    playlist_screen_update_column_title(screen);
    return;
}

void
playlist_screen_destroy(PlaylistScreen *screen) {
    if (screen == NULL) {
        return;
    }
    (void)playlist_screen_unregister(screen);
    nc_playlist_screen_set_menu(&screen->screen, NULL);
    nc_window_destroy(&screen->window);
    nc_song_menu_destroy(&screen->songs);
    sb_free(&screen->title_cache);
    sb_free(&screen->column_title);
    sb_free(&screen->filter_constraint);
    sb_free(&screen->search_constraint);
    ncm_regex_destroy(&screen->filter_regex);
    return;
}

bool
playlist_screen_unregister(PlaylistScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (!screen->registered) {
        return true;
    }
    if (!app_controller_unregister_screen(
        playlist_screen_base(screen))) {
        return false;
    }
    screen->registered = false;
    return true;
}

NcPlaylistScreen *
playlist_screen_playlist(PlaylistScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->screen;
}

NcSongMenu *
playlist_screen_song_menu(PlaylistScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->songs;
}

NcMenu *
playlist_screen_menu(PlaylistScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return nc_song_menu_base(&screen->songs);
}

NcWindow *
playlist_screen_window(PlaylistScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->window;
}

void
playlist_screen_update_column_title(PlaylistScreen *screen) {
    int32 list_width;

    if (screen == NULL) {
        return;
    }

    sb_clear(&screen->column_title);
    if ((Config.playlist_display_mode != NCM_DISPLAY_MODE_COLUMNS)
        || !Config.titles_visibility || (Config.columns.items == NULL)
        || (Config.columns.len <= 0) || (screen->screen.main_height <= 2)) {
        nc_window_set_title(&screen->window, NULL, 0);
        return;
    }

    if (screen->screen.width > INT32_MAX) {
        list_width = INT32_MAX;
    } else {
        list_width = screen->screen.width;
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
playlist_screen_set_geometry(PlaylistScreen *screen,
                             int32 start_x, int32 width,
                             int32 main_start_y, int32 main_height) {
    if (screen == NULL) {
        return;
    }
    nc_playlist_screen_set_geometry(&screen->screen, start_x, width,
                                    main_start_y, main_height);
    nc_window_resize(&screen->window, width, main_height);
    nc_window_move_to(&screen->window, start_x, main_start_y);
    playlist_screen_update_column_title(screen);
    return;
}

void
playlist_screen_set_mouse_config(PlaylistScreen *screen,
                                 int32 lines_scrolled,
                                 bool scroll_whole_page) {
    if (screen == NULL) {
        return;
    }
    nc_playlist_screen_set_mouse_config(&screen->screen, lines_scrolled,
                                        scroll_whole_page);
    return;
}

void
playlist_screen_set_highlighting(PlaylistScreen *screen,
                                 bool enabled) {
    bool was_enabled;

    if (screen == NULL) {
        return;
    }

    was_enabled = playlist_screen_highlighting(screen);
    nc_menu_set_highlighting(playlist_screen_menu(screen), enabled);
    if (enabled && !was_enabled) {
        screen->highlight_timer = global_timer;
    } else if (!enabled) {
        screen->highlight_timer.ns = 0;
    }
    return;
}

bool
playlist_screen_highlighting(PlaylistScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return nc_menu_highlight_enabled(playlist_screen_menu(screen));
}

void
playlist_screen_request_highlighting(PlaylistScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->highlighting_requested = true;
    playlist_screen_set_highlighting(screen, true);
    screen->highlight_timer = global_timer;
    return;
}

void
playlist_screen_clear(PlaylistScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_clear_items(playlist_storage_menu(screen));
    playlist_screen_reload_total_length(screen);
    playlist_screen_reload_remaining(screen);
    return;
}

bool
playlist_screen_reload_from_mpd(PlaylistScreen *screen,
                                NcmMpdClient *client,
                                int32 version,
                                int32 playlist_length,
                                NcmError *ncm_error) {
    NcmMpdSongList songs;
    bool result;

    if (screen == NULL) {
        ncm_error_set(ncm_error, -1, STRLIT("playlist screen is NULL"));
        return false;
    }
    if (client == NULL) {
        ncm_error_set(ncm_error, -1, STRLIT("MPD client is NULL"));
        return false;
    }

    songs = (NcmMpdSongList){0};
    if (playlist_should_reload_full(screen, version,
                                    playlist_length, NULL)) {
        result = ncm_mpd_client_get_queue(client, &songs, ncm_error);
    } else {
        result = ncm_mpd_client_get_queue_changes(client, version, &songs,
                                                  ncm_error);
        if (result
            && playlist_should_reload_full(screen, version,
                                           playlist_length,
                                           &songs)) {
            result = ncm_mpd_client_get_queue(client, &songs, ncm_error);
        }
    }

    if (result) {
        result = playlist_apply_changed_songs(screen,
                                              &songs, playlist_length);
        if (!result) {
            ncm_error_set(ncm_error, -1,
                          STRLIT("could not copy playlist songs"));
        }
    }
    ncm_mpd_song_list_destroy(&songs);
    return result;
}

int32
playlist_screen_song_count(PlaylistScreen *screen) {
    if (screen == NULL) {
        return 0;
    }
    return nc_menu_all_item_count(playlist_storage_menu(screen));
}

bool
playlist_screen_empty(PlaylistScreen *screen) {
    return playlist_screen_song_count(screen) <= 0;
}

bool
playlist_screen_current_song(PlaylistScreen *screen,
                             NcmSong *song) {
    NcmSong *current;

    if (screen == NULL) {
        return false;
    }
    if (song == NULL) {
        return false;
    }

    current = nc_song_menu_current(playlist_screen_song_menu(screen));
    if (current == NULL) {
        return false;
    }
    return ncm_song_copy(song, current);
}

bool
playlist_screen_update_current_mutable_song(
    PlaylistScreen *screen, NcmMutableSong *song
) {
    NcmSong replacement;
    NcmSong *current;
    NcMenu *menu;
    bool was_filtered;

    if ((screen == NULL) || (song == NULL)) {
        return false;
    }

    menu = playlist_storage_menu(screen);
    if ((current = nc_menu_current_item(menu)) == NULL) {
        return false;
    }

    replacement = (NcmSong){0};
    if (!playlist_build_mutable_song(&replacement, current, song)) {
        ncm_song_destroy(&replacement);
        return false;
    }

    was_filtered = nc_menu_is_filtered(menu);
    ncm_song_move(current, &replacement);
    if (was_filtered) {
        nc_menu_apply_filter(menu);
    }
    playlist_screen_reload_total_length(screen);
    playlist_screen_reload_remaining(screen);
    return true;
}

bool
playlist_screen_now_playing_song(PlaylistScreen *screen,
                                 int32 position, NcmSong *song) {
    NcMenu *base;
    NcSongMenu *menu;
    NcmSong *item;
    int32 count;
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

    menu = playlist_screen_song_menu(screen);
    base = nc_song_menu_base(menu);
    count = nc_menu_all_item_count(base);
    queue_position = position;

    if (position < count) {
        item = nc_song_menu_item_at(menu, NC_MENU_ITEMS_ALL, position);
        if (item
            && (ncm_song_position(item) == queue_position)) {
            return ncm_song_copy(song, item);
        }
    }

    for (int32 i = 0; i < count; i += 1) {
        if (i == position) {
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
playlist_screen_locate_position(PlaylistScreen *screen,
                                int32 position) {
    NcMenu *menu;
    NcmSong *song;
    int32 height;

    if (screen == NULL) {
        return false;
    }

    menu = playlist_storage_menu(screen);
    height = nc_playlist_screen_height(&screen->screen);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if ((song = nc_menu_active_item_at(menu, i))
            && (ncm_song_position(song) == position)) {
            nc_menu_highlight_position(menu, i, height);
            return true;
        }
    }
    return false;
}

bool
playlist_screen_selected_songs(PlaylistScreen *screen,
                               NcmSongArray *songs) {
    NcMenu *menu;

    if (songs == NULL) {
        return false;
    }
    ncm_song_array_clear(songs);
    if (screen == NULL) {
        return false;
    }

    menu = playlist_storage_menu(screen);
    if (playlist_append_selected(menu, songs)) {
        return true;
    }
    if (nc_menu_item_count(menu) <= 0) {
        return true;
    }
    return playlist_append_position(menu, nc_menu_highlight(menu),
                                    songs);
}

static bool
playlist_screen_find_sort_range(
    PlaylistScreen *screen, int32 *first_position,
    int32 *last_position, int32 *start_position, NcmError *ncm_error
) {
    NcMenu *menu;
    NcmSong *song;
    int32 first;
    int32 last;
    int32 range_start;

    if (screen == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing playlist screen"));
        return false;
    }

    menu = playlist_storage_menu(screen);
    last = nc_menu_all_item_count(menu);
    if (last <= 0) {
        ncm_error_set(ncm_error, ENOENT, STRLIT("playlist is empty"));
        return false;
    }

    first = last;
    for (int32 i = 0; i < last; i += 1) {
        uint32 flags = nc_menu_item_flags_at(menu, NC_MENU_ITEMS_ALL, i);

        if (flags & NC_MENU_ITEM_SELECTED) {
            first = i;
            break;
        }
    }
    if (first == last) {
        first = 0;
    } else {
        int32 selected_last;

        selected_last = first + 1;
        for (int32 i = first + 1; i < last; i += 1) {
            uint32 flags;

            flags = nc_menu_item_flags_at(menu, NC_MENU_ITEMS_ALL, i);
            if (flags & NC_MENU_ITEM_SELECTED) {
                selected_last = i + 1;
            }
        }
        for (int32 i = first; i < selected_last; i += 1) {
            uint32 flags;

            flags = nc_menu_item_flags_at(menu, NC_MENU_ITEMS_ALL, i);
            if (!(flags & NC_MENU_ITEM_SELECTED)) {
                ncm_error_set(
                    ncm_error, EINVAL,
                    STRLIT("selected songs are not contiguous"));
                return false;
            }
        }
        last = selected_last;
    }

    if ((song = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, first)) == NULL) {
        ncm_error_set(ncm_error, EINVAL,
                      STRLIT("missing playlist range song"));
        return false;
    }
    range_start = ncm_song_position(song);

    for (int32 i = first; i < last; i += 1) {
        int32 expected_position;

        if ((song = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, i)) == NULL) {
            ncm_error_set(ncm_error, EINVAL,
                          STRLIT("missing playlist range song"));
            return false;
        }
        expected_position = range_start + i - first;
        if ((expected_position > INT32_MAX)
            || (ncm_song_position(song) != expected_position)) {
            ncm_error_set(
                ncm_error, EINVAL,
                STRLIT("playlist range positions are not contiguous"));
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
    ncm_error_clear(ncm_error);
    return true;
}

bool
playlist_screen_has_sortable_range(
    PlaylistScreen *screen
) {
    return playlist_screen_find_sort_range(
        screen, NULL, NULL, NULL, NULL);
}

bool
playlist_screen_copy_sort_range(
    PlaylistScreen *screen, NcmSongArray *songs,
    int32 *start_position, NcmError *ncm_error
) {
    NcmSongArray replacement;
    NcMenu *menu;
    NcmSong *song;
    int32 first;
    int32 last;
    int32 range_start;

    if (songs == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing song array"));
        return false;
    }
    if (start_position == NULL) {
        ncm_error_set(ncm_error, EINVAL,
                      STRLIT("missing playlist range position"));
        return false;
    }
    if (!playlist_screen_find_sort_range(
        screen, &first, &last, &range_start, ncm_error)) {
        return false;
    }

    menu = playlist_storage_menu(screen);
    replacement = (NcmSongArray){0};
    for (int32 i = first; i < last; i += 1) {
        if ((song = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, i)) == NULL) {
            ncm_error_set(ncm_error, EINVAL,
                          STRLIT("missing playlist range song"));
            ncm_song_array_destroy(&replacement);
            return false;
        }
        if (!ncm_song_array_append_copy(&replacement, song)) {
            ncm_error_set(ncm_error, ENOMEM,
                          STRLIT("could not copy playlist range"));
            ncm_song_array_destroy(&replacement);
            return false;
        }
    }

    ncm_song_array_destroy(songs);
    *songs = replacement;
    *start_position = range_start;
    ncm_error_clear(ncm_error);
    return true;
}

bool
playlist_screen_apply_filter(PlaylistScreen *screen,
                             char *pattern, int32 pattern_len,
                             NcmError *ncm_error) {
    NcMenuDisplayCallbacks callbacks;

    if (screen == NULL) {
        return false;
    }
    if ((pattern == NULL) || (pattern_len <= 0)) {
        playlist_screen_clear_filter(screen);
        return true;
    }
    if (!ncm_regex_compile(&screen->filter_regex, pattern, pattern_len,
                           Config.regex_flags, ncm_error)) {
        return false;
    }
    if (sb_set(&screen->filter_constraint, pattern, pattern_len) < 0) {
        return false;
    }
    callbacks = playlist_display_callbacks();
    callbacks.filter = playlist_filter_song;
    callbacks.user = screen;
    nc_menu_set_display_callbacks(playlist_storage_menu(screen),
                                  callbacks);
    nc_menu_apply_filter(playlist_storage_menu(screen));
    return true;
}

void
playlist_screen_clear_filter(PlaylistScreen *screen) {
    if (screen == NULL) {
        return;
    }
    ncm_regex_destroy(&screen->filter_regex);
    screen->filter_regex = (NcmRegex){0};
    sb_clear(&screen->filter_constraint);
    nc_menu_set_display_callbacks(playlist_storage_menu(screen),
                                  playlist_display_callbacks());
    nc_menu_show_all_items(playlist_storage_menu(screen));
    return;
}

bool
playlist_screen_search(PlaylistScreen *screen,
                       char *pattern, int32 pattern_len,
                       bool forward, bool wrap,
                       bool skip_current, NcmError *ncm_error) {
    NcmRegex regex;
    bool result;

    if ((screen == NULL) || (pattern == NULL) || (pattern_len <= 0)) {
        return false;
    }

    regex = (NcmRegex){0};
    if (!ncm_regex_compile(&regex, pattern, pattern_len,
                           Config.regex_flags, ncm_error)) {
        ncm_regex_destroy(&regex);
        return false;
    }
    if (sb_set(&screen->search_constraint, pattern, pattern_len) < 0) {
        ncm_regex_destroy(&regex);
        return false;
    }

    result = playlist_search_menu(
        screen, playlist_storage_menu(screen), &regex, forward,
        wrap, skip_current);
    ncm_regex_destroy(&regex);
    return result;
}

bool
playlist_screen_set_selected_priority(PlaylistScreen *screen,
                                      NcmMpdClient *client,
                                      int32 priority,
                                      NcmError *ncm_error) {
    PlaylistPriorityContext context;
    NcmSongArray songs;
    bool result;

    if (screen == NULL) {
        return false;
    }
    if (client == NULL) {
        return false;
    }

    songs = (NcmSongArray){0};
    if (!playlist_screen_selected_songs(screen, &songs)) {
        ncm_song_array_destroy(&songs);
        return false;
    }

    context.client = client;
    context.ncm_error = ncm_error;
    context.priority = priority;
    result = true;
    for (int32 i = 0; i < songs.len; i += 1) {
        if (!playlist_set_one_priority(&songs.items[i], i,
                                       &context)) {
            result = false;
            break;
        }
    }

    ncm_song_array_destroy(&songs);
    return result;
}

void
playlist_screen_reload_total_length(PlaylistScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->reload_total_length = true;
    return;
}

void
playlist_screen_reload_remaining(PlaylistScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->reload_remaining = true;
    return;
}

static void
playlist_display(PlaylistScreen *playlist) {
    NcMenu *menu;
    NcWindow *window;

    menu = nc_playlist_screen_menu(&playlist->screen);
    window = playlist_screen_window(playlist);
    nc_window_display(window);
    nc_menu_refresh(menu, window, nc_window_width(window),
                    nc_window_height(window));
    return;
}

static void
playlist_switch_to(NcScreen *screen) {
    PlaylistScreen *playlist;

    playlist = playlist_from_screen(screen);
    playlist->scroll_begin = 0;
    return;
}

static void
playlist_resize(NcScreen *screen) {
    PlaylistScreen *playlist;
    NcScreenResizeParams params;

    playlist = playlist_from_screen(screen);
    params = app_controller_screen_resize_params(screen, true);
    playlist_screen_set_geometry(playlist, params.x_offset,
                                 params.width,
                                 ui_state_main_start_y(),
                                 ui_state_main_height());
    nc_screen_clear_resize_request(screen);
    return;
}


static char *
playlist_title(NcScreen *screen) {
    PlaylistScreen *playlist;

    playlist = playlist_from_screen(screen);
    playlist_refresh_stats(playlist);
    return playlist->title_cache.data;
}

static void
playlist_update(NcScreen *screen) {
    PlaylistScreen *playlist;
    int32 delay;

    playlist = playlist_from_screen(screen);
    if (!playlist_screen_highlighting(playlist)) {
        return;
    }

    delay = Config.playlist_disable_highlight_delay_seconds;
    if ((delay == 0)
        || (global_timer_elapsed_seconds(playlist->highlight_timer)
            <= delay)) {
        return;
    }

    playlist_screen_set_highlighting(playlist, false);
    nc_screen_refresh(screen);
    return;
}

static void
playlist_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    PlaylistScreen *playlist;

    playlist = playlist_from_screen(screen);
    nc_playlist_screen_mouse_button_pressed(&playlist->screen, event);
    return;
}



static bool
playlist_filter_song(NcMenu *menu, void *item, void *user) {
    PlaylistScreen *screen;

    (void)menu;
    screen = user;
    return playlist_song_matches(screen, item,
                                 &screen->filter_regex);
}

static bool
playlist_song_matches(PlaylistScreen *screen,
                      NcmSong *song, NcmRegex *regex) {
    StrBuilder buffer;
    bool result;

    (void)screen;
    ASSERT(song != NULL);

    if (Config.playlist_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
        buffer = ncm_format_render_string(&Config.song_columns_mode_format,
                                          song);
    } else {
        buffer = ncm_format_render_string(&Config.song_list_format, song);
    }
    result = ncm_regex_search(regex, buffer.data, buffer.len);
    sb_free(&buffer);
    return result;
}

static bool
playlist_search_menu(PlaylistScreen *screen,
                     NcMenu *menu, NcmRegex *regex,
                     bool forward, bool wrap,
                     bool skip_current) {
    PlaylistSearchContext context;

    context.screen = screen;
    context.regex = regex;
    return nc_menu_search_selectable(menu, screen->screen.main_height,
                                     forward, wrap, skip_current,
                                     playlist_search_position,
                                     &context, NULL);
}

static bool
playlist_search_position(NcMenu *menu, int32 pos,
                         void *user) {
    PlaylistSearchContext *context;

    context = user;
    return playlist_song_matches(
        context->screen, nc_menu_active_item_at(menu, pos),
        context->regex);
}

static NcMenuDisplayCallbacks
playlist_display_callbacks(void) {
    NcMenuDisplayCallbacks callbacks = {0};

    callbacks.draw = playlist_draw_song;
    return callbacks;
}

static NcMenuActionCallbacks
playlist_action_callbacks(void) {
    NcMenuActionCallbacks callbacks = {0};

    callbacks.activate = playlist_activate_song;
    return callbacks;
}

static void
playlist_draw_song(NcMenu *menu, NcWindow *window, void *item,
                   int32 pos, void *user) {
    NcBuffer buffer;
    bool is_now_playing;

    (void)user;
    ASSERT(menu != NULL);
    ASSERT(window != NULL);
    ASSERT(item != NULL);

    if ((is_now_playing = playlist_song_is_now_playing(item))) {
        playlist_print_buffer(window, &Config.now_playing_prefix);
    }

    buffer = (NcBuffer){0};
    if (Config.playlist_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
        int32 available_width;
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
        if (!menu->highlight_disabled && (pos == menu->highlight)) {
            available_width -= utf8_width(
                menu->highlight_suffix.data, menu->highlight_suffix.len);
        }
        if (available_width < 0) {
            available_width = 0;
        }
        if (available_width > INT32_MAX) {
            list_width = INT32_MAX;
        } else {
            list_width = available_width;
        }
        use_colors = !Config.discard_colors_if_item_is_selected
                     || !nc_menu_position_is_selected(menu, pos);
        ncm_display_song_columns(&buffer, item, Config.columns.items,
                                 Config.columns.len, list_width, use_colors);
    } else {
        ncm_display_song_row(&buffer, &Config.song_list_format, item,
                             NCM_FORMAT_FLAG_ALL);
    }
    playlist_print_buffer(window, &buffer);
    nc_buffer_destroy(&buffer);

    if (is_now_playing) {
        playlist_print_buffer(window, &Config.now_playing_suffix);
    }
    return;
}

static void
playlist_activate_song(NcMenu *menu, void *item, int32 pos,
                       void *user) {
    NcmError ncm_error = {0};

    (void)menu;
    (void)pos;
    (void)user;
    ASSERT(item != NULL);
    if (!ncm_mpd_client_play_id(&global_mpd, ncm_song_id(item),
                                &ncm_error)) {
        ncm_statusbar_print_cstring(1, ncm_error.message);
    }
    return;
}

static bool
playlist_song_is_now_playing(NcmSong *song) {
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
playlist_print_buffer(NcWindow *window, NcBuffer *buffer) {
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
playlist_storage_menu(PlaylistScreen *screen) {
    return nc_song_menu_base(playlist_screen_song_menu(screen));
}

static bool
playlist_build_mutable_song(
    NcmSong *replacement, NcmSong *current, NcmMutableSong *edited
) {
    NcmStringView value;
    enum NcmTagsField field;
    enum mpd_tag_type type;
    int32 mtime;
    int32 duration;

    if ((replacement == NULL) || (current == NULL) || (edited == NULL)) {
        return false;
    }
    if (!playlist_set_mutable_uri(replacement, edited)) {
        return false;
    }

    for (int32 i = 0; i < current->tags_len; i += 1) {
        if (current->tags[i].type == MPD_TAG_UNKNOWN) {
            continue;
        }
        field = ncm_tags_field_from_tag_type(current->tags[i].type);
        if (field != NCM_TAGS_FIELD_COUNT) {
            continue;
        }
        if (!ncm_song_add_tag(replacement, current->tags[i].type,
                              current->tags[i].value,
                              current->tags[i].value_len)) {
            return false;
        }
    }

    for (uint32 i = 0; i < NCM_TAGS_FIELD_COUNT; i += 1) {
        type = ncm_tags_field_to_tag_type(i);
        for (int32 j = 0; ; j += 1) {
            if (!ncm_mutable_song_get_tag(edited, i, j, &value)) {
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
        mtime = (int32)ncm_song_mtime(current);
    }
    ncm_song_set_duration(replacement, duration);
    ncm_song_set_position(replacement, ncm_song_position(current));
    ncm_song_set_id(replacement, ncm_song_id(current));
    ncm_song_set_priority(replacement, ncm_song_priority(current));
    ncm_song_set_mtime(replacement, (time_t)mtime);
    return true;
}

static bool
playlist_set_mutable_uri(NcmSong *song, NcmMutableSong *edited) {
    NcmStringView new_name;
    StrBuilder uri = {0};
    bool result;

    if (!ncm_mutable_song_get_new_name(edited, &new_name)) {
        return ncm_song_set_uri(song, edited->uri, edited->uri_len);
    }

    if (edited->directory_len > 0) {
        SB_APPEND(&uri, edited->directory, edited->directory_len);
        if (edited->directory[edited->directory_len - 1] != '/') {
            SB_APPEND(&uri, STRLIT("/"));
        }
    }
    SB_APPEND(&uri, new_name.data, new_name.len);
    result = ncm_song_set_uri(song, uri.data, uri.len);
    sb_free(&uri);
    return result;
}

static void
playlist_refresh_stats(PlaylistScreen *screen) {
    int32 count;

    sb_clear(&screen->title_cache);
    SB_APPEND(&screen->title_cache, STRLIT("Playlist ("));
    count = playlist_screen_song_count(screen);
    sb_printf(&screen->title_cache, "%d", count);
    if (count == 1) {
        SB_APPEND(&screen->title_cache, STRLIT(" item)"));
    } else {
        SB_APPEND(&screen->title_cache, STRLIT(" items)"));
    }
    return;
}

static bool
playlist_truncate_storage(PlaylistScreen *screen,
                          int32 playlist_length) {
    NcMenu *menu;
    int32 new_count;
    int32 old_count;

    ASSERT(screen != NULL);
    menu = playlist_storage_menu(screen);
    new_count = playlist_length;
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
playlist_apply_changed_song_to_storage(PlaylistScreen *screen,
                                       NcmSong *song) {
    NcMenu *menu;
    int32 position;

    ASSERT(screen != NULL);
    ASSERT(song != NULL);

    menu = playlist_storage_menu(screen);
    position = ncm_song_position(song);
    if (position < nc_menu_all_item_count(menu)) {
        return nc_menu_replace_item(menu, NC_MENU_ITEMS_ALL,
                                    position, song);
    }

    nc_menu_add_item(menu, song);
    return true;
}

static bool
playlist_should_reload_full(PlaylistScreen *screen,
                            int32 version,
                            int32 playlist_length,
                            NcmMpdSongList *changes) {
    int32 count;
    int32 next_append_position;

    if (playlist_length == 0) {
        return false;
    }
    if (version == 0) {
        return true;
    }

    count = nc_menu_all_item_count(playlist_storage_menu(screen));
    if (count <= 0) {
        return true;
    }
    if (changes == NULL) {
        return false;
    }

    next_append_position = count;
    for (int32 i = 0; i < changes->count; i += 1) {
        int32 position;

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
playlist_apply_changed_songs(PlaylistScreen *screen,
                             NcmMpdSongList *songs,
                             int32 playlist_length) {
    NcMenu *menu;
    bool result;
    bool was_filtered;

    ASSERT(screen != NULL);
    ASSERT(songs != NULL);

    menu = playlist_storage_menu(screen);
    result = true;
    was_filtered = nc_menu_is_filtered(menu);

    if (result && !playlist_truncate_storage(screen,
                                             playlist_length)) {
        result = false;
    }

    for (int32 i = 0; result && i < songs->count; i += 1) {
        if (!playlist_apply_changed_song_to_storage(
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

    playlist_screen_reload_total_length(screen);
    playlist_screen_reload_remaining(screen);
    return true;
}

static bool
playlist_append_selected(NcMenu *menu, NcmSongArray *songs) {
    bool found;

    found = false;
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (nc_menu_position_is_selected(menu, i)) {
            if (!playlist_append_position(menu, i, songs)) {
                ncm_song_array_clear(songs);
                return false;
            }
            found = true;
        }
    }
    return found;
}

static bool
playlist_append_position(NcMenu *menu, int32 pos,
                         NcmSongArray *songs) {
    NcmSong *song;

    if ((song = nc_menu_active_item_at(menu, pos)) == NULL) {
        return false;
    }
    return ncm_song_array_append_copy(songs, song);
}

static bool
playlist_set_one_priority(NcmSong *song, int32 idx, void *user) {
    PlaylistPriorityContext *context;

    (void)idx;
    context = user;
    return ncm_mpd_client_set_priority_song(context->client, song,
                                            context->priority,
                                            context->ncm_error);
}

#endif /* NCMPCPP_NC_PLAYLIST_C */
