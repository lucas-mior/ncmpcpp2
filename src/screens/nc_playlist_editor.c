#if !defined(NCMPCPP_NC_PLAYLIST_EDITOR_C)
#define NCMPCPP_NC_PLAYLIST_EDITOR_C

#include "screens/nc_playlist_editor.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "actions.h"
#include "app_controller.h"
#include "global.h"
#include "settings.h"
#include "c/ncm_base.h"
#include "c/ncm_comparators.h"
#include "c/ncm_charset.h"
#include "c/ncm_display.h"
#include "cbase/utf8.c"
#include "c/ncm_string.h"
#include "status.h"
#include "statusbar.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"

static NcScreenCallbacks playlist_editor_callbacks(void);
static NcWindow *playlist_editor_active_window_callback(NcScreen *screen);
static void playlist_editor_refresh_callback(NcScreen *screen);
static void playlist_editor_refresh_window_callback(NcScreen *screen);
static void playlist_editor_scroll_callback(NcScreen *screen,
                                            enum NcScroll where);
static void playlist_editor_finish_list_change_callback(NcScreen *screen);
static void playlist_editor_switch_to_callback(NcScreen *screen);
static void playlist_editor_resize_callback(NcScreen *screen);
static int32 playlist_editor_timeout_callback(NcScreen *screen);
static char *playlist_editor_title_callback(NcScreen *screen);
static void playlist_editor_update_callback(NcScreen *screen);
static void playlist_editor_mouse_callback(NcScreen *screen, MEVENT event);
static bool playlist_editor_lockable_callback(NcScreen *screen);
static bool playlist_editor_mergable_callback(NcScreen *screen);
static void playlist_editor_destroy_callback(NcScreen *screen);
static void playlist_editor_print_buffer(NcWindow *window,
                                         NcBuffer *buffer);
static int32 playlist_editor_content_list_width(NcMenu *menu,
                                                NcWindow *window,
                                                int32 pos);
static bool playlist_editor_playlist_matches_regex(NcmRegex *regex,
                                                   NcmPlaylist *playlist);
static bool playlist_editor_content_matches_regex(
    NativePlaylistEditorScreen *screen, NcmRegex *regex, NcmSong *song);
static bool playlist_editor_format_content_search_text(
    NativePlaylistEditorScreen *screen, NcmSong *song, NcBuffer *buffer);
static bool playlist_editor_search_text_matches(NcmRegex *regex,
                                                char *data, int32 len);
static void playlist_editor_initialize_buffers(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_destroy_buffers(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_initialize_regexes(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_apply_geometry(
    NativePlaylistEditorScreen *screen);
static int32 playlist_editor_separator_width(int32 width);
static void playlist_editor_configure_menus(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_update_menu_highlights(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_draw_separator(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_destroy_regexes(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_update_titles(
    NativePlaylistEditorScreen *screen, bool update_windows);
static void playlist_editor_append_int64(NcmBuffer *buffer, int32 value);
static void playlist_editor_reset_content_timer(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_clear_playlist_filter(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_clear_content_filter(
    NativePlaylistEditorScreen *screen);
static bool playlist_editor_find_playlist_position(
    NativePlaylistEditorScreen *screen, char *path, int32 path_len,
    int32 *pos);
static bool playlist_editor_highlight_content_position(
    NativePlaylistEditorScreen *screen, int32 pos);
static int32 playlist_editor_find_song_in_content_range(
    NativePlaylistEditorScreen *screen, NcmSong *song,
    int32 first, int32 last);
static bool playlist_editor_locate_song_in_playlist_range(
    NativePlaylistEditorScreen *screen, NcmMpdClient *client,
    NcmSong *song, int32 first, int32 last, NcmError *error);
static bool playlist_editor_show_screen(NativePlaylistEditorScreen *screen);
static bool playlist_editor_store_current_playlist_path(
    NativePlaylistEditorScreen *screen, NcmBuffer *buffer);
static bool playlist_editor_restore_playlist_path(
    NativePlaylistEditorScreen *screen, NcmBuffer *buffer);
static bool playlist_editor_store_current_song(
    NativePlaylistEditorScreen *screen, NcmSong *song);
static bool playlist_editor_restore_content_song(
    NativePlaylistEditorScreen *screen, NcmSong *song);
static void playlist_editor_sort_playlists(NcmMpdPlaylistList *playlists);
static int32 playlist_editor_compare_playlists(NcmPlaylist *left,
                                               NcmPlaylist *right);
static bool playlist_editor_update_from_mpd(
    NativePlaylistEditorScreen *screen, NcmMpdClient *client);
static void playlist_editor_observe_current_playlist(
    NativePlaylistEditorScreen *screen);
static bool playlist_editor_displayed_playlist_is_current(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_clear_stale_content(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_finish_playlist_change(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_mouse_scroll(
    NativePlaylistEditorScreen *screen, enum NcScroll where);
static bool playlist_editor_mouse_select_playlist(
    NativePlaylistEditorScreen *screen, int32 y, bool load);
static bool playlist_editor_mouse_select_content(
    NativePlaylistEditorScreen *screen, int32 y, bool play);
static bool playlist_editor_mouse_load_current_playlist(
    NativePlaylistEditorScreen *screen);
static bool playlist_editor_mouse_add_current_song(
    NativePlaylistEditorScreen *screen, bool play);
static void playlist_editor_print_playlist_loaded(NcmPlaylist *playlist);
static void playlist_editor_set_displayed_playlist(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_refresh_window(NativePlaylistEditorScreen *screen,
                                           NcWindow *window, NcMenu *menu);
static bool append_current_content(NativePlaylistEditorScreen *screen,
                                   NcmSongArray *songs);
static bool append_selected_content(NativePlaylistEditorScreen *screen,
                                    NcmSongArray *songs);
static bool append_selected_playlist_content(
    NativePlaylistEditorScreen *screen, NcmSongArray *songs);
static bool append_playlist_content_from_mpd(
    NcmPlaylist *playlist, NcmSongArray *songs);
static bool append_song_list_content(NcmMpdSongList *list,
                                     NcmSongArray *songs);
static bool append_content_item(NativePlaylistEditorScreen *screen,
                                int32 pos, NcmSongArray *songs);
static bool append_content_item_from_source(
    NativePlaylistEditorScreen *screen, enum NcMenuItemSource source,
    int32 pos, NcmSongArray *songs);
static bool playlist_editor_search_menu(
    NativePlaylistEditorScreen *screen, NcMenu *menu, NcmRegex *regex,
    bool forward, bool wrap, bool skip_current);
static bool playlist_editor_search_item(
    NativePlaylistEditorScreen *screen, NcMenu *menu, NcmRegex *regex,
    int32 pos);
static bool playlist_editor_search_position(NcMenu *menu, int32 pos,
                                            void *user);
static void playlist_editor_set_display_callbacks(
    NativePlaylistEditorScreen *screen);

typedef struct PlaylistEditorSearchContext {
    NativePlaylistEditorScreen *screen;
    NcmRegex *regex;
} PlaylistEditorSearchContext;

void
native_playlist_editor_screen_init(NativePlaylistEditorScreen *screen,
                                   int32 start_x, int32 width,
                                   int32 main_start_y, int32 main_height,
                                   NcColor color, NcBorder border) {
    NcScreenCallbacks callbacks;
    int32 initial_left_width;
    int32 initial_right_width;

    if (width < 1) {
        width = 1;
    }
    if (main_height < 1) {
        main_height = 1;
    }
    initial_left_width = width / 2;
    if (initial_left_width < 1) {
        initial_left_width = 1;
    }
    initial_right_width = width - initial_left_width;
    if (initial_right_width < 1) {
        initial_right_width = 1;
    }

    callbacks = playlist_editor_callbacks();
    nc_playlist_entry_menu_init(&screen->playlists);
    nc_song_menu_init(&screen->content);
    playlist_editor_initialize_buffers(screen);
    playlist_editor_initialize_regexes(screen);
    playlist_editor_reset_content_timer(screen);
    screen->active_column = NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS;
    screen->column_ratio_left = 1;
    screen->column_ratio_right = 1;
    screen->playlists_update_requested = true;
    screen->content_update_requested = true;
    screen->playlist_filter_enabled = false;
    screen->content_filter_enabled = false;
    screen->playlist_search_enabled = false;
    screen->content_search_enabled = false;
    screen->displayed_playlist_valid = false;
    screen->observed_playlist_valid = false;
    screen->last_playlist_highlight = -1;
    screen->last_known_content_count = -1;
    if (Config.data_fetching_delay) {
        screen->fetching_delay_ms = NATIVE_PLAYLIST_EDITOR_FETCH_DELAY_MS;
        screen->window_timeout_ms = NATIVE_PLAYLIST_EDITOR_FETCH_DELAY_MS;
    } else {
        screen->fetching_delay_ms = -1;
        screen->window_timeout_ms = NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
    }
    screen->registered = false;
    playlist_editor_update_titles(screen, false);
    nc_window_init(&screen->playlists_window, start_x, main_start_y,
                   initial_left_width, main_height,
                   screen->playlists_title.data,
                   screen->playlists_title.len, color, border);
    nc_window_init(&screen->content_window,
                   start_x + initial_left_width, main_start_y,
                   initial_right_width, main_height,
                   screen->content_title.data, screen->content_title.len,
                   color, border);
    native_playlist_editor_screen_set_geometry(screen, start_x, width,
                                               main_start_y, main_height);
    nc_screen_init(&screen->screen, callbacks, screen,
                   NC_SCREEN_TYPE_PLAYLIST_EDITOR);
    playlist_editor_configure_menus(screen);
    playlist_editor_set_display_callbacks(screen);
    return;
}

void
native_playlist_editor_screen_destroy(NativePlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    (void)app_controller_unregister_screen(
        native_playlist_editor_screen_base(screen));
    nc_window_destroy(&screen->content_window);
    nc_window_destroy(&screen->playlists_window);
    nc_song_menu_destroy(&screen->content);
    nc_playlist_entry_menu_destroy(&screen->playlists);
    playlist_editor_destroy_regexes(screen);
    playlist_editor_destroy_buffers(screen);
    screen->registered = false;
    return;
}

NcScreen *
native_playlist_editor_screen_base(NativePlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->screen;
}

NcPlaylistEntryMenu *
native_playlist_editor_screen_playlists(NativePlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->playlists;
}

NcSongMenu *
native_playlist_editor_screen_content(NativePlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->content;
}

NcMenu *
native_playlist_editor_screen_active_menu(
    NativePlaylistEditorScreen *screen
) {
    if (screen == NULL) {
        return NULL;
    }
    if (screen->active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT) {
        return nc_song_menu_base(&screen->content);
    }
    return nc_playlist_entry_menu_base(&screen->playlists);
}

NcWindow *
native_playlist_editor_screen_active_window(
    NativePlaylistEditorScreen *screen
) {
    if (screen == NULL) {
        return NULL;
    }
    if (screen->active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT) {
        return &screen->content_window;
    }
    return &screen->playlists_window;
}

void
native_playlist_editor_screen_set_geometry(
    NativePlaylistEditorScreen *screen, int32 start_x, int32 width,
    int32 main_start_y, int32 main_height
) {
    if (screen == NULL) {
        return;
    }
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    playlist_editor_apply_geometry(screen);
    return;
}

void
native_playlist_editor_screen_set_column_ratio(
    NativePlaylistEditorScreen *screen, int32 left, int32 right
) {
    if (screen == NULL) {
        return;
    }
    if (left < 1) {
        left = 1;
    }
    if (right < 1) {
        right = 1;
    }
    screen->column_ratio_left = left;
    screen->column_ratio_right = right;
    playlist_editor_apply_geometry(screen);
    return;
}

bool
native_playlist_editor_screen_previous_column_available(
    NativePlaylistEditorScreen *screen
) {
    NcMenu *playlists;

    if (screen == NULL) {
        return false;
    }
    playlists = nc_playlist_entry_menu_base(&screen->playlists);
    return (screen->active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT)
           && (nc_menu_all_item_count(playlists) > 0);
}

bool
native_playlist_editor_screen_next_column_available(
    NativePlaylistEditorScreen *screen
) {
    NcMenu *content;

    if (screen == NULL) {
        return false;
    }
    content = nc_song_menu_base(&screen->content);
    return (screen->active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS)
           && (nc_menu_all_item_count(content) > 0);
}

void
native_playlist_editor_screen_previous_column(
    NativePlaylistEditorScreen *screen
) {
    if (native_playlist_editor_screen_previous_column_available(screen)) {
        screen->active_column = NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS;
        playlist_editor_update_menu_highlights(screen);
    }
    return;
}

void
native_playlist_editor_screen_next_column(
    NativePlaylistEditorScreen *screen
) {
    if (native_playlist_editor_screen_next_column_available(screen)) {
        screen->active_column = NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT;
        playlist_editor_update_menu_highlights(screen);
    }
    return;
}

bool
native_playlist_editor_screen_load_playlists(
    NativePlaylistEditorScreen *screen, NcmMpdPlaylistList *playlists
) {
    NcmBuffer preserved;
    NcMenu *menu;
    bool had_preserved;

    if ((screen == NULL) || (playlists == NULL)) {
        return false;
    }
    ncm_buffer_init(&preserved);
    had_preserved = playlist_editor_store_current_playlist_path(
        screen, &preserved);
    menu = nc_playlist_entry_menu_base(&screen->playlists);
    nc_menu_show_all_items(menu);
    nc_menu_clear_items(menu);
    for (int32 i = 0; i < playlists->count; i += 1) {
        nc_playlist_entry_menu_add(&screen->playlists,
                                   &playlists->items[i]);
    }
    if (had_preserved) {
        (void)playlist_editor_restore_playlist_path(screen, &preserved);
    }
    if (screen->playlist_filter_enabled) {
        nc_menu_apply_filter(menu);
        if (had_preserved) {
            (void)playlist_editor_restore_playlist_path(screen,
                                                        &preserved);
        }
    }
    if (screen->displayed_playlist_valid
        && !playlist_editor_displayed_playlist_is_current(screen)) {
        playlist_editor_clear_stale_content(screen);
    }
    playlist_editor_observe_current_playlist(screen);
    screen->playlists_update_requested = false;
    ncm_buffer_destroy(&preserved);
    return true;
}

bool
native_playlist_editor_screen_reload_playlists_from_mpd(
    NativePlaylistEditorScreen *screen, NcmMpdClient *client,
    NcmError *error
) {
    NcmMpdPlaylistList playlists;
    bool ok;

    if (screen == NULL) {
        return false;
    }
    ncm_mpd_playlist_list_init(&playlists);
    if ((ok = ncm_mpd_client_get_playlists(client, &playlists, error))) {
        playlist_editor_sort_playlists(&playlists);
        ok = native_playlist_editor_screen_load_playlists(screen,
                                                          &playlists);
        if (!ok) {
            ncm_error_set(error, ENOMEM,
                          STRLIT_ARGS("could not copy playlists"));
        }
    }
    ncm_mpd_playlist_list_destroy(&playlists);
    return ok;
}

bool
native_playlist_editor_screen_load_content(
    NativePlaylistEditorScreen *screen, NcmMpdSongList *songs
) {
    NcMenu *menu;
    NcmSong preserved_song;
    bool had_preserved_song;

    if ((screen == NULL) || (songs == NULL)) {
        return false;
    }
    ncm_song_init(&preserved_song);
    had_preserved_song = playlist_editor_store_current_song(
        screen, &preserved_song);

    menu = nc_song_menu_base(&screen->content);
    nc_menu_show_all_items(menu);
    nc_menu_clear_items(menu);
    for (int32 i = 0; i < songs->count; i += 1) {
        nc_song_menu_add(&screen->content, &songs->items[i]);
    }
    if (screen->content_filter_enabled) {
        nc_menu_apply_filter(menu);
    }
    if (had_preserved_song) {
        (void)playlist_editor_restore_content_song(screen,
                                                   &preserved_song);
    }
    playlist_editor_set_displayed_playlist(screen);
    playlist_editor_observe_current_playlist(screen);
    screen->last_known_content_count = nc_menu_all_item_count(menu);
    screen->content_update_requested = false;
    playlist_editor_update_titles(screen, true);
    ncm_song_destroy(&preserved_song);
    return true;
}

bool
native_playlist_editor_screen_reload_content_from_mpd(
    NativePlaylistEditorScreen *screen, NcmMpdClient *client,
    NcmError *error
) {
    NcmMpdSongList songs;
    NcmPlaylist *playlist;
    bool ok;

    if (screen == NULL) {
        return false;
    }
    playlist = nc_playlist_entry_menu_current(&screen->playlists);
    if ((playlist == NULL) || (playlist->path == NULL)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing playlist"));
        return false;
    }

    ncm_mpd_song_list_init(&songs);
    ok = ncm_mpd_client_get_playlist_content(client, playlist->path,
                                             &songs, error)
         && native_playlist_editor_screen_load_content(screen, &songs);
    ncm_mpd_song_list_destroy(&songs);
    return ok;
}

bool
native_playlist_editor_screen_locate_playlist(
    NativePlaylistEditorScreen *screen, NcmMpdClient *client,
    char *path, int32 path_len, NcmError *error
) {
    NcMenu *menu;
    int32 pos;

    if ((screen == NULL) || (path == NULL) || (path_len <= 0)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing playlist"));
        return false;
    }
    if (!native_playlist_editor_screen_reload_playlists_from_mpd(
            screen, client, error)) {
        return false;
    }

    playlist_editor_clear_playlist_filter(screen);
    if (!playlist_editor_find_playlist_position(screen, path, path_len,
                                                &pos)) {
        ncm_error_set(error, ENOENT, STRLIT_ARGS("playlist not found"));
        return false;
    }

    menu = nc_playlist_entry_menu_base(&screen->playlists);
    nc_menu_highlight_position(menu, pos,
                               nc_window_height(
                                   &screen->playlists_window));
    screen->active_column = NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS;
    playlist_editor_update_menu_highlights(screen);
    playlist_editor_clear_content_filter(screen);
    playlist_editor_clear_stale_content(screen);
    if (!native_playlist_editor_screen_reload_content_from_mpd(
            screen, client, error)) {
        return false;
    }
    return playlist_editor_show_screen(screen);
}

bool
native_playlist_editor_screen_locate_song(
    NativePlaylistEditorScreen *screen, NcmMpdClient *client,
    NcmSong *song, NcmError *error
) {
    NcMenu *playlists;
    NcMenu *content;
    NcmSong current_song;
    int32 playlist_pos;
    int32 song_pos;
    int32 found_pos;
    bool success;

    if ((screen == NULL) || (song == NULL)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing song"));
        return false;
    }
    playlists = nc_playlist_entry_menu_base(&screen->playlists);
    if ((nc_menu_all_item_count(playlists) <= 0)
        || screen->playlists_update_requested) {
        if (!native_playlist_editor_screen_reload_playlists_from_mpd(
                screen, client, error)) {
            return false;
        }
    }
    if (nc_menu_all_item_count(playlists) <= 0) {
        return false;
    }

    playlist_editor_clear_content_filter(screen);
    playlist_editor_clear_playlist_filter(screen);
    playlists = nc_playlist_entry_menu_base(&screen->playlists);
    content = nc_song_menu_base(&screen->content);
    playlist_pos = nc_menu_highlight(playlists);
    song_pos = nc_menu_highlight(content);
    if (song_pos < 0) {
        song_pos = 0;
    }

    found_pos = playlist_editor_find_song_in_content_range(
        screen, song, song_pos + 1, nc_menu_all_item_count(content));
    if (found_pos >= 0) {
        if (!playlist_editor_highlight_content_position(screen,
                                                        found_pos)) {
            return false;
        }
        return playlist_editor_show_screen(screen);
    }

    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Jumping to song...");
    success = playlist_editor_locate_song_in_playlist_range(
        screen, client, song, playlist_pos + 1,
        nc_menu_all_item_count(playlists), error);
    if (success) {
        return playlist_editor_show_screen(screen);
    }
    if (ncm_error_is_set(error)) {
        return false;
    }

    success = playlist_editor_locate_song_in_playlist_range(
        screen, client, song, 0, playlist_pos, error);
    if (success) {
        return playlist_editor_show_screen(screen);
    }
    if (ncm_error_is_set(error)) {
        return false;
    }

    found_pos = playlist_editor_find_song_in_content_range(
        screen, song, 0, song_pos);
    if (found_pos >= 0) {
        if (!playlist_editor_highlight_content_position(screen,
                                                        found_pos)) {
            return false;
        }
        return playlist_editor_show_screen(screen);
    }

    ncm_song_init(&current_song);
    success = native_playlist_editor_screen_current_content_song(
        screen, &current_song) && ncm_song_equal(&current_song, song);
    ncm_song_destroy(&current_song);
    if (success) {
        screen->active_column = NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT;
        playlist_editor_update_menu_highlights(screen);
        return playlist_editor_show_screen(screen);
    }

    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Song was not found in playlists");
    return false;
}

bool
native_playlist_editor_screen_current_playlist(
    NativePlaylistEditorScreen *screen, NcmPlaylist *playlist
) {
    NcmPlaylist *current;

    if ((screen == NULL) || (playlist == NULL)) {
        return false;
    }
    current = nc_playlist_entry_menu_current(&screen->playlists);
    if (current == NULL) {
        return false;
    }
    return ncm_playlist_copy(playlist, current);
}

bool
native_playlist_editor_screen_current_song(
    NativePlaylistEditorScreen *screen, NcmSong *song
) {
    return native_playlist_editor_screen_current_content_song(screen, song);
}

bool
native_playlist_editor_screen_current_content_song(
    NativePlaylistEditorScreen *screen, NcmSong *song
) {
    NcmSong *current;

    if ((screen == NULL) || (song == NULL)) {
        return false;
    }
    if ((current = nc_song_menu_current(&screen->content)) == NULL) {
        return false;
    }
    return ncm_song_copy(song, current);
}

int32
native_playlist_editor_screen_selected_playlist_count(
    NativePlaylistEditorScreen *screen
) {
    if (screen == NULL) {
        return 0;
    }
    return nc_menu_selected_count(
        nc_playlist_entry_menu_base(&screen->playlists));
}

bool
native_playlist_editor_screen_selected_songs(
    NativePlaylistEditorScreen *screen, NcmSongArray *songs
) {
    if (songs) {
        ncm_song_array_clear(songs);
    }
    if ((screen == NULL) || (songs == NULL)) {
        return false;
    }
    if (screen->active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT) {
        return append_selected_content(screen, songs);
    }
    if (native_playlist_editor_screen_selected_playlist_count(screen) > 0) {
        return append_selected_playlist_content(screen, songs);
    }
    return append_current_content(screen, songs);
}

bool
native_playlist_editor_screen_apply_active_filter(
    NativePlaylistEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, NcmError *error
) {
    NcMenu *menu;
    NcmRegex *regex;
    NcmBuffer *constraint;
    bool *enabled;

    if (screen == NULL) {
        return false;
    }
    menu = native_playlist_editor_screen_active_menu(screen);
    if (screen->active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT) {
        regex = &screen->content_filter_regex;
        constraint = &screen->content_filter_constraint;
        enabled = &screen->content_filter_enabled;
    } else {
        regex = &screen->playlist_filter_regex;
        constraint = &screen->playlist_filter_constraint;
        enabled = &screen->playlist_filter_enabled;
    }

    if ((pattern == NULL) || (pattern_len <= 0)) {
        *enabled = false;
        ncm_buffer_clear(constraint);
        nc_menu_show_all_items(menu);
        playlist_editor_update_titles(screen, true);
        return true;
    }
    if (!ncm_regex_compile(regex, pattern, pattern_len, regex_flags, error)) {
        return false;
    }
    ncm_buffer_set(constraint, pattern, pattern_len);
    *enabled = true;
    nc_menu_apply_filter(menu);
    playlist_editor_update_titles(screen, true);
    return true;
}

bool
native_playlist_editor_screen_search_active(
    NativePlaylistEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, bool forward, bool wrap, bool skip_current,
    NcmError *error
) {
    NcmBuffer *constraint;
    NcmRegex *regex;
    NcMenu *menu;
    bool *enabled;

    if (screen == NULL) {
        return false;
    }
    if (screen->active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT) {
        regex = &screen->content_search_regex;
        constraint = &screen->content_search_constraint;
        enabled = &screen->content_search_enabled;
    } else {
        regex = &screen->playlist_search_regex;
        constraint = &screen->playlist_search_constraint;
        enabled = &screen->playlist_search_enabled;
    }
    if ((pattern == NULL) || (pattern_len <= 0)) {
        *enabled = false;
        ncm_buffer_clear(constraint);
        return false;
    }
    if (!ncm_regex_compile(regex, pattern, pattern_len, regex_flags, error)) {
        return false;
    }
    ncm_buffer_set(constraint, pattern, pattern_len);
    *enabled = true;
    menu = native_playlist_editor_screen_active_menu(screen);
    if (!playlist_editor_search_menu(screen, menu, regex, forward, wrap,
                                     skip_current)) {
        return false;
    }
    playlist_editor_finish_playlist_change(screen);
    return true;
}

void
native_playlist_editor_screen_request_playlists_update(
    NativePlaylistEditorScreen *screen
) {
    if (screen == NULL) {
        return;
    }
    screen->playlists_update_requested = true;
    nc_screen_request_update(&screen->screen);
    return;
}

void
native_playlist_editor_screen_request_content_update(
    NativePlaylistEditorScreen *screen
) {
    if (screen == NULL) {
        return;
    }
    screen->content_update_requested = true;
    playlist_editor_reset_content_timer(screen);
    nc_screen_request_update(&screen->screen);
    return;
}

static NativePlaylistEditorScreen *
playlist_editor_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcScreenCallbacks
playlist_editor_callbacks(void) {
    NcScreenCallbacks callbacks = {0};

    callbacks.active_window = playlist_editor_active_window_callback;
    callbacks.refresh = playlist_editor_refresh_callback;
    callbacks.refresh_window = playlist_editor_refresh_window_callback;
    callbacks.scroll = playlist_editor_scroll_callback;
    callbacks.list_change_finished =
        playlist_editor_finish_list_change_callback;
    callbacks.switch_to = playlist_editor_switch_to_callback;
    callbacks.resize = playlist_editor_resize_callback;
    callbacks.window_timeout = playlist_editor_timeout_callback;
    callbacks.title = playlist_editor_title_callback;
    callbacks.update = playlist_editor_update_callback;
    callbacks.mouse_button_pressed = playlist_editor_mouse_callback;
    callbacks.is_lockable = playlist_editor_lockable_callback;
    callbacks.is_mergable = playlist_editor_mergable_callback;
    callbacks.destroy = playlist_editor_destroy_callback;
    return callbacks;
}

static NcWindow *
playlist_editor_active_window_callback(NcScreen *screen) {
    NativePlaylistEditorScreen *editor;

    editor = playlist_editor_from_screen(screen);
    return native_playlist_editor_screen_active_window(editor);
}

static void
playlist_editor_refresh_callback(NcScreen *screen) {
    NativePlaylistEditorScreen *editor;

    editor = playlist_editor_from_screen(screen);
    playlist_editor_update_titles(editor, true);
    playlist_editor_update_menu_highlights(editor);
    playlist_editor_refresh_window(editor, &editor->playlists_window,
                                   nc_playlist_entry_menu_base(
                                       &editor->playlists));
    playlist_editor_draw_separator(editor);
    playlist_editor_refresh_window(editor, &editor->content_window,
                                   nc_song_menu_base(&editor->content));
    return;
}

static void
playlist_editor_refresh_window_callback(NcScreen *screen) {
    NativePlaylistEditorScreen *editor;
    NcWindow *window;
    NcMenu *menu;

    editor = playlist_editor_from_screen(screen);
    playlist_editor_update_titles(editor, true);
    playlist_editor_update_menu_highlights(editor);
    window = native_playlist_editor_screen_active_window(editor);
    menu = native_playlist_editor_screen_active_menu(editor);
    playlist_editor_refresh_window(editor, window, menu);
    return;
}

static void
playlist_editor_scroll_callback(NcScreen *screen, enum NcScroll where) {
    NativePlaylistEditorScreen *editor;
    NcMenu *menu;

    editor = playlist_editor_from_screen(screen);
    menu = native_playlist_editor_screen_active_menu(editor);
    nc_menu_scroll_selectable(menu, editor->main_height, where);
    return;
}

static void
playlist_editor_finish_list_change_callback(NcScreen *screen) {
    NativePlaylistEditorScreen *editor;

    editor = playlist_editor_from_screen(screen);
    playlist_editor_finish_playlist_change(editor);
    return;
}

static void
playlist_editor_switch_to_callback(NcScreen *screen) {
    (void)screen;
    return;
}

static void
playlist_editor_resize_callback(NcScreen *screen) {
    NativePlaylistEditorScreen *editor;
    NcScreenResizeParams params;

    editor = playlist_editor_from_screen(screen);
    params = nc_screen_resize_params(screen);
    native_playlist_editor_screen_set_geometry(editor, params.x_offset,
                                               params.width,
                                               editor->main_start_y,
                                               editor->main_height);
    nc_screen_clear_resize_request(screen);
    return;
}

static int32
playlist_editor_timeout_callback(NcScreen *screen) {
    NativePlaylistEditorScreen *editor;

    editor = playlist_editor_from_screen(screen);
    if ((editor->fetching_delay_ms >= 0)
        && nc_menu_empty(nc_song_menu_base(&editor->content))
        && (nc_menu_empty(nc_playlist_entry_menu_base(&editor->playlists))
            || !playlist_editor_displayed_playlist_is_current(editor))) {
        return editor->window_timeout_ms;
    }
    return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
}

static char *
playlist_editor_title_callback(NcScreen *screen) {
    (void)screen;
    return "Playlist editor";
}

static void
playlist_editor_update_callback(NcScreen *screen) {
    NativePlaylistEditorScreen *editor;
    bool changed;

    editor = playlist_editor_from_screen(screen);
    playlist_editor_finish_playlist_change(editor);
    changed = playlist_editor_update_from_mpd(editor, &global_mpd);
    nc_screen_clear_update_request(screen);
    if (changed && app_controller_is_screen_visible(screen)) {
        nc_screen_refresh(screen);
    }
    return;
}

static void
playlist_editor_mouse_callback(NcScreen *screen, MEVENT event) {
    NativePlaylistEditorScreen *editor;
    int32 x;
    int32 y;

    if ((editor = playlist_editor_from_screen(screen)) == NULL) {
        return;
    }

    x = event.x;
    y = event.y;
    if (nc_window_has_coords(&editor->playlists_window, &x, &y)) {
        if (editor->active_column
            != NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS) {
            if (!native_playlist_editor_screen_previous_column_available(
                    editor)) {
                return;
            }
            native_playlist_editor_screen_previous_column(editor);
        }
        if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
            (void)playlist_editor_mouse_select_playlist(
                editor, y, (event.bstate & BUTTON3_PRESSED) != 0);
        } else if (event.bstate & BUTTON5_PRESSED) {
            playlist_editor_mouse_scroll(editor, NC_SCROLL_DOWN);
        } else if (event.bstate & BUTTON4_PRESSED) {
            playlist_editor_mouse_scroll(editor, NC_SCROLL_UP);
        }
        playlist_editor_finish_playlist_change(editor);
        return;
    }

    x = event.x;
    y = event.y;
    if (nc_window_has_coords(&editor->content_window, &x, &y)) {
        if (editor->active_column
            != NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT) {
            if (!native_playlist_editor_screen_next_column_available(
                    editor)) {
                return;
            }
            native_playlist_editor_screen_next_column(editor);
        }
        if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
            (void)playlist_editor_mouse_select_content(
                editor, y, (event.bstate & BUTTON3_PRESSED) != 0);
        } else if (event.bstate & BUTTON5_PRESSED) {
            playlist_editor_mouse_scroll(editor, NC_SCROLL_DOWN);
        } else if (event.bstate & BUTTON4_PRESSED) {
            playlist_editor_mouse_scroll(editor, NC_SCROLL_UP);
        }
    }
    return;
}

static bool
playlist_editor_lockable_callback(NcScreen *screen) {
    (void)screen;
    return true;
}

static bool
playlist_editor_mergable_callback(NcScreen *screen) {
    (void)screen;
    return true;
}

static void
playlist_editor_destroy_callback(NcScreen *screen) {
    native_playlist_editor_screen_destroy(playlist_editor_from_screen(screen));
    return;
}

static bool
playlist_filter_callback(NcMenu *menu, void *item, void *user) {
    NativePlaylistEditorScreen *editor;
    NcmPlaylist *playlist;

    (void)menu;
    editor = user;
    playlist = item;
    if (!editor->playlist_filter_enabled) {
        return true;
    }
    if ((playlist == NULL) || (playlist->path == NULL)) {
        return false;
    }
    return playlist_editor_playlist_matches_regex(
        &editor->playlist_filter_regex, playlist);
}

static bool
content_filter_callback(NcMenu *menu, void *item, void *user) {
    NativePlaylistEditorScreen *editor;

    (void)menu;
    editor = user;
    if (!editor->content_filter_enabled) {
        return true;
    }
    return playlist_editor_content_matches_regex(
        editor, &editor->content_filter_regex, item);
}

static void
playlist_draw_callback(NcMenu *menu, NcWindow *window, void *item,
                       int32 pos, void *user) {
    NcmPlaylist *playlist;
    NcmBuffer converted;

    (void)menu;
    (void)pos;
    (void)user;
    playlist = item;
    if ((window == NULL) || (playlist == NULL)
        || (playlist->path == NULL) || (playlist->path_len <= 0)) {
        return;
    }

    converted = ncm_charset_utf8_to_locale(playlist->path,
                                           playlist->path_len);
    nc_window_print_data(window, converted.data, converted.len);
    ncm_buffer_destroy(&converted);
    return;
}

static void
content_draw_callback(NcMenu *menu, NcWindow *window, void *item,
                      int32 pos, void *user) {
    NcBuffer buffer;
    int32 list_width;
    bool use_colors;

    (void)user;
    if ((menu == NULL) || (window == NULL) || (item == NULL)) {
        return;
    }

    nc_buffer_init(&buffer);
    if (Config.playlist_editor_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
        list_width = playlist_editor_content_list_width(menu, window,
                                                        pos);
        use_colors = !Config.discard_colors_if_item_is_selected
                     || !nc_menu_position_is_selected(menu, pos);
        ncm_display_song_columns(&buffer, item, Config.columns.items,
                                 Config.columns.len, list_width,
                                 use_colors);
    } else {
        ncm_display_song_row(&buffer, &Config.song_list_format, item,
                             NCM_FORMAT_FLAG_ALL);
    }
    playlist_editor_print_buffer(window, &buffer);
    nc_buffer_destroy(&buffer);
    return;
}

static void
playlist_editor_print_buffer(NcWindow *window, NcBuffer *buffer) {
    NcBufferProperty *properties;
    char *data;
    int32 property_count;
    int32 property_index;
    int32 len;

    if ((window == NULL) || (buffer == NULL)) {
        return;
    }

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

static int32
playlist_editor_content_list_width(NcMenu *menu, NcWindow *window,
                                   int32 pos) {
    int32 available_width;

    available_width = nc_window_width(window) - nc_window_get_x(window);
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
        return INT32_MAX;
    }
    return available_width;
}

static bool
playlist_editor_playlist_matches_regex(NcmRegex *regex,
                                       NcmPlaylist *playlist) {
    if ((playlist == NULL) || (playlist->path == NULL)) {
        return false;
    }
    return playlist_editor_search_text_matches(regex, playlist->path,
                                               playlist->path_len);
}

static bool
playlist_editor_content_matches_regex(NativePlaylistEditorScreen *screen,
                                      NcmRegex *regex, NcmSong *song) {
    NcBuffer buffer;
    bool result;

    if (song == NULL) {
        return false;
    }

    nc_buffer_init(&buffer);
    result = playlist_editor_format_content_search_text(screen, song,
                                                        &buffer);
    if (result) {
        result = playlist_editor_search_text_matches(regex, buffer.data,
                                                     buffer.len);
    }
    nc_buffer_destroy(&buffer);
    return result;
}

static bool
playlist_editor_format_content_search_text(
    NativePlaylistEditorScreen *screen, NcmSong *song, NcBuffer *buffer
) {
    (void)screen;
    if ((song == NULL) || (buffer == NULL)) {
        return false;
    }

    if (Config.playlist_editor_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
        ncm_display_song_row(buffer, &Config.song_columns_mode_format,
                             song, NCM_FORMAT_FLAG_ALL);
    } else {
        ncm_display_song_row(buffer, &Config.song_list_format, song,
                             NCM_FORMAT_FLAG_ALL);
    }
    return true;
}

static bool
playlist_editor_search_text_matches(NcmRegex *regex, char *data,
                                    int32 len) {
    if (data == NULL) {
        data = "";
        len = 0;
    }
    return ncm_regex_search(regex, data, len);
}

static void
playlist_editor_initialize_buffers(NativePlaylistEditorScreen *screen) {
    ncm_buffer_init(&screen->playlist_filter_constraint);
    ncm_buffer_init(&screen->content_filter_constraint);
    ncm_buffer_init(&screen->playlist_search_constraint);
    ncm_buffer_init(&screen->content_search_constraint);
    ncm_buffer_init(&screen->playlists_title);
    ncm_buffer_init(&screen->content_title);
    ncm_buffer_init(&screen->displayed_playlist_path);
    ncm_buffer_init(&screen->observed_playlist_path);
    return;
}

static void
playlist_editor_destroy_buffers(NativePlaylistEditorScreen *screen) {
    ncm_buffer_destroy(&screen->observed_playlist_path);
    ncm_buffer_destroy(&screen->displayed_playlist_path);
    ncm_buffer_destroy(&screen->content_title);
    ncm_buffer_destroy(&screen->playlists_title);
    ncm_buffer_destroy(&screen->content_search_constraint);
    ncm_buffer_destroy(&screen->playlist_search_constraint);
    ncm_buffer_destroy(&screen->content_filter_constraint);
    ncm_buffer_destroy(&screen->playlist_filter_constraint);
    return;
}

static void
playlist_editor_initialize_regexes(NativePlaylistEditorScreen *screen) {
    ncm_regex_init(&screen->playlist_filter_regex);
    ncm_regex_init(&screen->content_filter_regex);
    ncm_regex_init(&screen->playlist_search_regex);
    ncm_regex_init(&screen->content_search_regex);
    return;
}

static void
playlist_editor_destroy_regexes(NativePlaylistEditorScreen *screen) {
    ncm_regex_destroy(&screen->content_search_regex);
    ncm_regex_destroy(&screen->playlist_search_regex);
    ncm_regex_destroy(&screen->content_filter_regex);
    ncm_regex_destroy(&screen->playlist_filter_regex);
    return;
}

static void
playlist_editor_apply_geometry(NativePlaylistEditorScreen *screen) {
    int32 total;
    int32 separator_width;
    int32 left_width;

    if (screen == NULL) {
        return;
    }
    if (screen->width < 1) {
        screen->width = 1;
    }
    if (screen->main_height < 1) {
        screen->main_height = 1;
    }
    if (screen->column_ratio_left < 1) {
        screen->column_ratio_left = 1;
    }
    if (screen->column_ratio_right < 1) {
        screen->column_ratio_right = 1;
    }

    total = screen->column_ratio_left + screen->column_ratio_right;
    separator_width = playlist_editor_separator_width(screen->width);
    left_width = screen->width*screen->column_ratio_left / total
                 - separator_width;
    if (left_width < 1) {
        left_width = 1;
    }
    if ((left_width + separator_width + 1) > screen->width) {
        left_width = screen->width - separator_width - 1;
    }
    if (left_width < 1) {
        left_width = 1;
    }

    screen->left_width = left_width;
    screen->right_start_x = screen->start_x + screen->left_width
                            + separator_width;
    screen->right_width = screen->width - screen->left_width
                          - separator_width;
    if (screen->right_width < 1) {
        screen->right_width = 1;
    }

    nc_window_resize(&screen->playlists_window, screen->left_width,
                     screen->main_height);
    nc_window_move_to(&screen->playlists_window, screen->start_x,
                      screen->main_start_y);
    nc_window_resize(&screen->content_window, screen->right_width,
                     screen->main_height);
    nc_window_move_to(&screen->content_window, screen->right_start_x,
                      screen->main_start_y);
    return;
}

static int32
playlist_editor_separator_width(int32 width) {
    if (width >= 3) {
        return 1;
    }
    return 0;
}

static void
playlist_editor_configure_menus(NativePlaylistEditorScreen *screen) {
    NcMenu *playlists;
    NcMenu *content;

    if (screen == NULL) {
        return;
    }
    playlists = nc_playlist_entry_menu_base(&screen->playlists);
    content = nc_song_menu_base(&screen->content);

    nc_menu_set_selected_prefix(playlists, &Config.selected_item_prefix);
    nc_menu_set_selected_suffix(playlists, &Config.selected_item_suffix);
    nc_menu_set_selected_prefix(content, &Config.selected_item_prefix);
    nc_menu_set_selected_suffix(content, &Config.selected_item_suffix);

    nc_menu_set_cyclic_scrolling(playlists, Config.use_cyclic_scrolling);
    nc_menu_set_cyclic_scrolling(content, Config.use_cyclic_scrolling);
    nc_menu_set_centered_cursor(playlists, Config.centered_cursor);
    nc_menu_set_centered_cursor(content, Config.centered_cursor);

    playlist_editor_update_menu_highlights(screen);
    return;
}

static void
playlist_editor_update_menu_highlights(
    NativePlaylistEditorScreen *screen
) {
    NcMenu *playlists;
    NcMenu *content;
    NcMenu *active;

    if (screen == NULL) {
        return;
    }
    playlists = nc_playlist_entry_menu_base(&screen->playlists);
    content = nc_song_menu_base(&screen->content);

    nc_menu_set_highlight_prefix(
        playlists, &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(
        playlists, &Config.current_item_inactive_column_suffix);
    nc_menu_set_highlight_prefix(
        content, &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(
        content, &Config.current_item_inactive_column_suffix);

    active = native_playlist_editor_screen_active_menu(screen);
    nc_menu_set_highlight_prefix(active, &Config.current_item_prefix);
    nc_menu_set_highlight_suffix(active, &Config.current_item_suffix);
    return;
}

static void
playlist_editor_draw_separator(NativePlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    if (playlist_editor_separator_width(screen->width) <= 0) {
        return;
    }
    nc_screen_draw_vertical_separator(screen->right_start_x - 1);
    return;
}

static void
playlist_editor_update_titles(NativePlaylistEditorScreen *screen,
                              bool update_windows) {
    if (screen == NULL) {
        return;
    }

    if (screen->last_known_content_count >= 0) {
        screen->last_known_content_count = nc_menu_item_count(
            nc_song_menu_base(&screen->content));
    }

    ncm_buffer_clear(&screen->playlists_title);
    ncm_buffer_clear(&screen->content_title);
    if (Config.titles_visibility) {
        ncm_buffer_append(&screen->playlists_title,
                          STRLIT_ARGS("Playlists"));
        ncm_buffer_append(&screen->content_title, STRLIT_ARGS("Content"));
        if (screen->last_known_content_count >= 0) {
            ncm_buffer_append(&screen->content_title, STRLIT_ARGS(" ("));
            playlist_editor_append_int64(&screen->content_title,
                                         screen->last_known_content_count);
            if (screen->last_known_content_count == 1) {
                ncm_buffer_append(&screen->content_title,
                                  STRLIT_ARGS(" item)"));
            } else {
                ncm_buffer_append(&screen->content_title,
                                  STRLIT_ARGS(" items)"));
            }
        }
    }

    if (update_windows) {
        nc_window_set_title(&screen->playlists_window,
                            screen->playlists_title.data,
                            screen->playlists_title.len);
        nc_window_set_title(&screen->content_window,
                            screen->content_title.data,
                            screen->content_title.len);
    }
    return;
}

static void
playlist_editor_append_int64(NcmBuffer *buffer, int32 value) {
    char digits[32];
    int32 len;

    len = 0;
    if (value == 0) {
        ncm_buffer_append_byte(buffer, '0');
        return;
    }
    while (value > 0) {
        digits[len] = (char)('0' + (value % 10));
        value /= 10;
        len += 1;
    }
    for (int32 i = len - 1; i >= 0; i -= 1) {
        ncm_buffer_append_byte(buffer, digits[i]);
    }
    return;
}

static void
playlist_editor_reset_content_timer(NativePlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->timer = global_timer;
    return;
}

static bool
playlist_editor_current_playlist_path(NativePlaylistEditorScreen *screen,
                                      char **path, int32 *path_len) {
    NcmPlaylist *playlist;

    if ((screen == NULL) || (path == NULL) || (path_len == NULL)) {
        return false;
    }
    playlist = nc_playlist_entry_menu_current(&screen->playlists);
    if ((playlist == NULL) || (playlist->path == NULL)) {
        return false;
    }
    *path = playlist->path;
    *path_len = playlist->path_len;
    return true;
}

static void
playlist_editor_clear_playlist_filter(
    NativePlaylistEditorScreen *screen
) {
    NcmBuffer path;
    bool has_path;

    if (screen == NULL) {
        return;
    }
    ncm_buffer_init(&path);
    has_path = playlist_editor_store_current_playlist_path(screen, &path);
    screen->playlist_filter_enabled = false;
    ncm_buffer_clear(&screen->playlist_filter_constraint);
    nc_menu_show_all_items(nc_playlist_entry_menu_base(
                               &screen->playlists));
    if (has_path) {
        (void)playlist_editor_restore_playlist_path(screen, &path);
    }
    ncm_buffer_destroy(&path);
    playlist_editor_update_titles(screen, true);
    return;
}

static void
playlist_editor_clear_content_filter(
    NativePlaylistEditorScreen *screen
) {
    NcmSong song;
    bool has_song;

    if (screen == NULL) {
        return;
    }
    ncm_song_init(&song);
    has_song = playlist_editor_store_current_song(screen, &song);
    screen->content_filter_enabled = false;
    ncm_buffer_clear(&screen->content_filter_constraint);
    nc_menu_show_all_items(nc_song_menu_base(&screen->content));
    if (has_song) {
        (void)playlist_editor_restore_content_song(screen, &song);
    }
    ncm_song_destroy(&song);
    playlist_editor_update_titles(screen, true);
    return;
}

static bool
playlist_editor_find_playlist_position(
    NativePlaylistEditorScreen *screen, char *path, int32 path_len,
    int32 *pos
) {
    NcMenu *menu;

    if ((screen == NULL) || (path == NULL) || (pos == NULL)) {
        return false;
    }
    menu = nc_playlist_entry_menu_base(&screen->playlists);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmPlaylist *playlist;

        playlist = nc_menu_active_item_at(menu, i);
        if (playlist
            && STREQUAL(playlist->path, playlist->path_len,
                                path, path_len)) {
            *pos = i;
            return true;
        }
    }
    return false;
}

static bool
playlist_editor_highlight_content_position(
    NativePlaylistEditorScreen *screen, int32 pos
) {
    NcMenu *menu;

    if (screen == NULL) {
        return false;
    }
    menu = nc_song_menu_base(&screen->content);
    if ((pos < 0) || (pos >= nc_menu_item_count(menu))) {
        return false;
    }
    nc_menu_highlight_position(menu, pos,
                               nc_window_height(&screen->content_window));
    screen->active_column = NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT;
    playlist_editor_update_menu_highlights(screen);
    return true;
}

static int32
playlist_editor_find_song_in_content_range(
    NativePlaylistEditorScreen *screen, NcmSong *song,
    int32 first, int32 last
) {
    NcMenu *menu;

    if ((screen == NULL) || (song == NULL)) {
        return -1;
    }
    menu = nc_song_menu_base(&screen->content);
    if (first < 0) {
        first = 0;
    }
    if (last > nc_menu_item_count(menu)) {
        last = nc_menu_item_count(menu);
    }
    for (int32 i = first; i < last; i += 1) {
        NcmSong *candidate;

        if ((candidate = nc_menu_active_item_at(menu, i))
            && ncm_song_equal(candidate, song)) {
            return i;
        }
    }
    return -1;
}

static bool
playlist_editor_find_song_in_mpd_playlist(
    NcmMpdClient *client, NcmPlaylist *playlist, NcmSong *song,
    int32 *song_index, NcmError *error
) {
    NcmMpdSongList songs;
    bool success;

    if ((playlist == NULL) || (playlist->path == NULL)
        || (song == NULL) || (song_index == NULL)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing playlist"));
        return false;
    }

    ncm_mpd_song_list_init(&songs);
    success = ncm_mpd_client_get_playlist_content_no_info(
        client, playlist->path, &songs, error);
    if (!success) {
        ncm_mpd_song_list_destroy(&songs);
        return false;
    }

    for (int32 i = 0; i < songs.count; i += 1) {
        if (ncm_song_equal(&songs.items[i], song)) {
            *song_index = i;
            ncm_mpd_song_list_destroy(&songs);
            return true;
        }
    }

    ncm_error_clear(error);
    ncm_mpd_song_list_destroy(&songs);
    return false;
}

static bool
playlist_editor_locate_song_in_playlist_range(
    NativePlaylistEditorScreen *screen, NcmMpdClient *client,
    NcmSong *song, int32 first, int32 last, NcmError *error
) {
    NcMenu *menu;

    if (screen == NULL) {
        return false;
    }
    menu = nc_playlist_entry_menu_base(&screen->playlists);
    if (first < 0) {
        first = 0;
    }
    if (last > nc_menu_item_count(menu)) {
        last = nc_menu_item_count(menu);
    }
    for (int32 i = first; i < last; i += 1) {
        NcmPlaylist *playlist;
        int32 song_index;

        playlist = nc_menu_active_item_at(menu, i);
        song_index = -1;
        if (!playlist_editor_find_song_in_mpd_playlist(
                client, playlist, song, &song_index, error)) {
            if (ncm_error_is_set(error)) {
                return false;
            }
            continue;
        }
        nc_menu_highlight_position(menu, i,
                                   nc_window_height(
                                       &screen->playlists_window));
        screen->active_column = NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS;
        playlist_editor_update_menu_highlights(screen);
        playlist_editor_clear_stale_content(screen);
        if (!native_playlist_editor_screen_reload_content_from_mpd(
                screen, client, error)) {
            return false;
        }
        return playlist_editor_highlight_content_position(screen,
                                                          song_index);
    }
    ncm_error_clear(error);
    return false;
}

static bool
playlist_editor_show_screen(NativePlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (!app_controller_is_screen_registered(&screen->screen)) {
        if (!app_controller_register_screen(&screen->screen)) {
            return false;
        }
        screen->registered = true;
    }
    return app_controller_switch_to_screen(&screen->screen);
}

static bool
playlist_editor_store_current_playlist_path(NativePlaylistEditorScreen *screen,
                                            NcmBuffer *buffer) {
    char *path;
    int32 path_len;

    if (buffer == NULL) {
        return false;
    }
    ncm_buffer_clear(buffer);
    if (!playlist_editor_current_playlist_path(screen, &path, &path_len)) {
        return false;
    }
    return ncm_buffer_set(buffer, path, path_len);
}

static bool
playlist_editor_restore_playlist_path(NativePlaylistEditorScreen *screen,
                                      NcmBuffer *buffer) {
    NcMenu *menu;

    if ((screen == NULL) || (buffer == NULL) || (buffer->len <= 0)) {
        return false;
    }
    menu = nc_playlist_entry_menu_base(&screen->playlists);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmPlaylist *playlist;

        playlist = nc_menu_active_item_at(menu, i);
        if (playlist
            && STREQUAL(playlist->path, playlist->path_len,
                                buffer->data, buffer->len)) {
            nc_menu_highlight_position(menu, i, screen->main_height);
            return true;
        }
    }
    return false;
}

static bool
playlist_editor_store_current_song(NativePlaylistEditorScreen *screen,
                                   NcmSong *song) {
    NcmSong *current;

    if ((screen == NULL) || (song == NULL)) {
        return false;
    }
    if ((current = nc_song_menu_current(&screen->content)) == NULL) {
        return false;
    }
    return ncm_song_copy(song, current);
}

static bool
playlist_editor_restore_content_song(NativePlaylistEditorScreen *screen,
                                     NcmSong *song) {
    NcMenu *menu;

    if ((screen == NULL) || (song == NULL)) {
        return false;
    }
    menu = nc_song_menu_base(&screen->content);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmSong *item;

        if ((item = nc_menu_active_item_at(menu, i))
            && ncm_song_equal(item, song)) {
            nc_menu_highlight_position(menu, i, screen->main_height);
            return true;
        }
    }
    return false;
}

static void
playlist_editor_sort_playlists(NcmMpdPlaylistList *playlists) {
    if (playlists == NULL) {
        return;
    }

    for (int32 i = 1; i < playlists->count; i += 1) {
        NcmPlaylist current;
        int32 j;

        ncm_playlist_init(&current);
        ncm_playlist_move(&current, &playlists->items[i]);
        j = i;
        while ((j > 0)
               && (playlist_editor_compare_playlists(
                       &playlists->items[j - 1], &current) > 0)) {
            ncm_playlist_move(&playlists->items[j],
                              &playlists->items[j - 1]);
            j -= 1;
        }
        ncm_playlist_move(&playlists->items[j], &current);
        ncm_playlist_destroy(&current);
    }
    return;
}

static int32
playlist_editor_compare_playlists(NcmPlaylist *left, NcmPlaylist *right) {
    if ((left == NULL) || (left->path == NULL)) {
        if ((right == NULL) || (right->path == NULL)) {
            return 0;
        }
        return -1;
    }
    if ((right == NULL) || (right->path == NULL)) {
        return 1;
    }
    return ncm_compare_locale_strings(left->path, left->path_len,
                                      right->path, right->path_len,
                                      Config.ignore_leading_the);
}

static bool
playlist_editor_content_fetch_due(NativePlaylistEditorScreen *screen) {
    NcMenu *playlists;
    NcMenu *content;

    if (screen == NULL) {
        return false;
    }
    playlists = nc_playlist_entry_menu_base(&screen->playlists);
    if (nc_menu_empty(playlists)) {
        return false;
    }

    content = nc_song_menu_base(&screen->content);
    if (screen->content_update_requested
        && playlist_editor_displayed_playlist_is_current(screen)) {
        return true;
    }
    if (playlist_editor_displayed_playlist_is_current(screen)) {
        return false;
    }
    if ((screen->last_known_content_count == 0)
        && screen->displayed_playlist_valid) {
        return false;
    }
    if (!nc_menu_empty(content)) {
        return false;
    }
    if (screen->fetching_delay_ms < 0) {
        return true;
    }
    return global_timer_elapsed_ms(screen->timer)
           > screen->fetching_delay_ms;
}

static void
playlist_editor_report_error(char *context, int32 context_len,
                             NcmError *error) {
    NcmBuffer message;

    ncm_buffer_init(&message);
    ncm_buffer_append(&message, context, context_len);
    if (error && (error->message[0] != 0)) {
        ncm_buffer_append(&message, STRLIT_ARGS(": "));
        ncm_buffer_append(&message, error->message,
                          strlen32(error->message));
    }
    ncm_buffer_append_byte(&message, '\0');
    ncm_statusbar_print_cstring(Config.message_delay_time,
                                message.data);
    ncm_buffer_destroy(&message);
    return;
}

static bool
playlist_editor_update_from_mpd(NativePlaylistEditorScreen *screen,
                                NcmMpdClient *client) {
    NcmError error;
    bool changed;
    bool ok;

    if (screen == NULL) {
        return false;
    }

    changed = false;

    ncm_error_clear(&error);
    if (screen->playlists_update_requested
        || nc_menu_empty(nc_playlist_entry_menu_base(&screen->playlists))) {
        ok = native_playlist_editor_screen_reload_playlists_from_mpd(
            screen, client, &error);
        if (!ok) {
            screen->playlists_update_requested = false;
            playlist_editor_report_error(
                STRLIT_ARGS("Could not fetch playlists"), &error);
            ncm_error_clear(&error);
            playlist_editor_update_titles(screen, true);
            return changed;
        }
        changed = true;
    }

    playlist_editor_finish_playlist_change(screen);
    if (!playlist_editor_content_fetch_due(screen)) {
        playlist_editor_update_titles(screen, true);
        return changed;
    }

    ncm_error_clear(&error);
    ok = native_playlist_editor_screen_reload_content_from_mpd(
        screen, client, &error);
    if (!ok) {
        screen->content_update_requested = false;
        playlist_editor_report_error(
            STRLIT_ARGS("Could not fetch playlist content"), &error);
        ncm_error_clear(&error);
        playlist_editor_update_titles(screen, true);
        return changed;
    }
    changed = true;
    playlist_editor_update_titles(screen, true);
    return changed;
}

static void
playlist_editor_observe_current_playlist(NativePlaylistEditorScreen *screen) {
    char *path;
    int32 path_len;
    NcMenu *menu;

    if (screen == NULL) {
        return;
    }
    menu = nc_playlist_entry_menu_base(&screen->playlists);
    screen->last_playlist_highlight = nc_menu_highlight(menu);
    if (!playlist_editor_current_playlist_path(screen, &path, &path_len)) {
        ncm_buffer_clear(&screen->observed_playlist_path);
        screen->observed_playlist_valid = false;
        return;
    }
    ncm_buffer_set(&screen->observed_playlist_path, path, path_len);
    screen->observed_playlist_valid = true;
    return;
}

static bool
playlist_editor_displayed_playlist_is_current(
    NativePlaylistEditorScreen *screen
) {
    char *path;
    int32 path_len;

    if ((screen == NULL) || !screen->displayed_playlist_valid) {
        return false;
    }
    if (!playlist_editor_current_playlist_path(screen, &path, &path_len)) {
        return false;
    }
    return STREQUAL(screen->displayed_playlist_path.data,
                            screen->displayed_playlist_path.len,
                            path, path_len);
}

static bool
playlist_editor_playlist_row_changed(NativePlaylistEditorScreen *screen) {
    char *path;
    int32 path_len;
    NcMenu *menu;
    bool changed;

    if (screen == NULL) {
        return false;
    }
    menu = nc_playlist_entry_menu_base(&screen->playlists);
    if (!playlist_editor_current_playlist_path(screen, &path, &path_len)) {
        changed = screen->observed_playlist_valid;
        playlist_editor_observe_current_playlist(screen);
        return changed;
    }

    changed = !screen->observed_playlist_valid
              || !STREQUAL(screen->observed_playlist_path.data,
                                   screen->observed_playlist_path.len,
                                   path, path_len)
              || (screen->last_playlist_highlight
                  != nc_menu_highlight(menu));
    if (changed) {
        playlist_editor_observe_current_playlist(screen);
    }
    return changed;
}

static void
playlist_editor_clear_stale_content(NativePlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_clear_items(nc_song_menu_base(&screen->content));
    ncm_buffer_clear(&screen->displayed_playlist_path);
    screen->displayed_playlist_valid = false;
    screen->content_update_requested = true;
    screen->last_known_content_count = -1;
    playlist_editor_reset_content_timer(screen);
    playlist_editor_update_titles(screen, true);
    return;
}

static void
playlist_editor_finish_playlist_change(NativePlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    if (screen->active_column != NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS) {
        return;
    }
    if (playlist_editor_playlist_row_changed(screen)) {
        playlist_editor_clear_stale_content(screen);
    }
    return;
}

static void
playlist_editor_mouse_scroll(NativePlaylistEditorScreen *screen,
                             enum NcScroll where) {
    enum NcScroll effective;
    NcMenu *menu;
    int32 count;

    if (screen == NULL) {
        return;
    }
    if ((menu = native_playlist_editor_screen_active_menu(screen)) == NULL) {
        return;
    }

    effective = where;
    count = Config.lines_scrolled;
    if (Config.mouse_list_scroll_whole_page) {
        count = 1;
        if (where == NC_SCROLL_DOWN) {
            effective = NC_SCROLL_PAGE_DOWN;
        } else if (where == NC_SCROLL_UP) {
            effective = NC_SCROLL_PAGE_UP;
        }
    }
    if (count < 1) {
        count = 1;
    }
    for (int32 i = 0; i < count; i += 1) {
        nc_menu_scroll_selectable(menu, screen->main_height, effective);
    }
    playlist_editor_finish_playlist_change(screen);
    return;
}

static bool
playlist_editor_mouse_select_playlist(NativePlaylistEditorScreen *screen,
                                      int32 y, bool load) {
    NcMenu *menu;

    if (screen == NULL) {
        return false;
    }
    menu = nc_playlist_entry_menu_base(&screen->playlists);
    if ((y < 0) || (y >= nc_menu_item_count(menu))) {
        return false;
    }
    if (!nc_menu_goto_selectable(menu, y)) {
        return false;
    }
    playlist_editor_finish_playlist_change(screen);
    if (load) {
        return playlist_editor_mouse_load_current_playlist(screen);
    }
    return true;
}

static bool
playlist_editor_mouse_select_content(NativePlaylistEditorScreen *screen,
                                     int32 y, bool play) {
    NcMenu *menu;

    if (screen == NULL) {
        return false;
    }
    menu = nc_song_menu_base(&screen->content);
    if ((y < 0) || (y >= nc_menu_item_count(menu))) {
        return false;
    }
    if (!nc_menu_goto_selectable(menu, y)) {
        return false;
    }
    if (play) {
        return playlist_editor_mouse_add_current_song(screen, true);
    }
    return true;
}

static bool
playlist_editor_mouse_load_current_playlist(
    NativePlaylistEditorScreen *screen
) {
    NcmPlaylist *playlist;
    NcmError error;
    bool loaded;

    if (screen == NULL) {
        return false;
    }
    playlist = nc_playlist_entry_menu_current(&screen->playlists);
    if ((playlist == NULL) || (playlist->path == NULL)) {
        return false;
    }

    loaded = false;
    ncm_error_clear(&error);
    if (!ncm_mpd_client_load_playlist(&global_mpd, playlist->path, &loaded,
                                      &error)) {
        playlist_editor_report_error(STRLIT_ARGS("Could not load playlist"),
                                     &error);
        return false;
    }
    if (loaded) {
        playlist_editor_print_playlist_loaded(playlist);
        (void)ncm_status_update_full(&global_mpd, NULL, &error);
    }
    return loaded;
}

static bool
playlist_editor_mouse_add_current_song(NativePlaylistEditorScreen *screen,
                                       bool play) {
    NcmSong *song;

    if (screen == NULL) {
        return false;
    }
    if ((song = nc_song_menu_current(&screen->content)) == NULL) {
        return false;
    }
    return ncm_action_add_song_to_playlist(song, play, -1);
}

static void
playlist_editor_print_playlist_loaded(NcmPlaylist *playlist) {
    NcmBuffer message;

    if ((playlist == NULL) || (playlist->path == NULL)) {
        return;
    }
    ncm_buffer_init(&message);
    ncm_buffer_append(&message, STRLIT_ARGS("Playlist \""));
    ncm_buffer_append(&message, playlist->path, playlist->path_len);
    ncm_buffer_append(&message, STRLIT_ARGS("\" loaded"));
    ncm_statusbar_print(Config.message_delay_time,
                        message.data, message.len);
    ncm_buffer_destroy(&message);
    return;
}

static void
playlist_editor_set_displayed_playlist(NativePlaylistEditorScreen *screen) {
    char *path;
    int32 path_len;

    if (screen == NULL) {
        return;
    }
    if (!playlist_editor_current_playlist_path(screen, &path, &path_len)) {
        ncm_buffer_clear(&screen->displayed_playlist_path);
        screen->displayed_playlist_valid = false;
        return;
    }
    ncm_buffer_set(&screen->displayed_playlist_path, path, path_len);
    screen->displayed_playlist_valid = true;
    return;
}

static void
playlist_editor_refresh_window(NativePlaylistEditorScreen *screen,
                               NcWindow *window, NcMenu *menu) {
    if ((screen == NULL) || (window == NULL) || (menu == NULL)) {
        return;
    }
    nc_menu_prepare_refresh(menu, nc_window_height(window), NULL, NULL);
    nc_window_display(window);
    nc_menu_refresh(menu, window, nc_window_width(window),
                    nc_window_height(window));
    return;
}

static bool
append_current_content(NativePlaylistEditorScreen *screen,
                       NcmSongArray *songs) {
    NcMenu *menu;

    menu = nc_song_menu_base(&screen->content);
    for (int32 i = 0; i < nc_menu_all_item_count(menu); i += 1) {
        if (!append_content_item_from_source(
                screen, NC_MENU_ITEMS_ALL, i, songs)) {
            return false;
        }
    }
    return true;
}

static bool
append_selected_content(NativePlaylistEditorScreen *screen,
                        NcmSongArray *songs) {
    NcMenu *menu;

    menu = nc_song_menu_base(&screen->content);
    if (!nc_menu_has_selected(menu)) {
        return append_content_item(screen, nc_menu_highlight(menu), songs);
    }
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (!nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!append_content_item(screen, i, songs)) {
            return false;
        }
    }
    return true;
}

static bool
append_selected_playlist_content(NativePlaylistEditorScreen *screen,
                                 NcmSongArray *songs) {
    NcMenu *menu;

    if ((screen == NULL) || (songs == NULL)) {
        return false;
    }

    menu = nc_playlist_entry_menu_base(&screen->playlists);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmPlaylist *playlist;

        if (!nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        playlist = nc_menu_active_item_at(menu, i);
        if (!append_playlist_content_from_mpd(playlist, songs)) {
            ncm_song_array_clear(songs);
            return false;
        }
    }
    return true;
}

static bool
append_playlist_content_from_mpd(NcmPlaylist *playlist,
                                 NcmSongArray *songs) {
    NcmMpdSongList list;
    NcmError error;
    bool ok;

    if ((playlist == NULL) || (playlist->path == NULL)
        || (songs == NULL)) {
        return false;
    }

    ncm_mpd_song_list_init(&list);
    ncm_error_clear(&error);
    ok = ncm_mpd_client_get_playlist_content(&global_mpd, playlist->path,
                                             &list, &error);
    if (!ok) {
        playlist_editor_report_error(
            STRLIT_ARGS("Could not fetch playlist content"), &error);
        ncm_error_clear(&error);
        ncm_mpd_song_list_destroy(&list);
        return false;
    }

    ok = append_song_list_content(&list, songs);
    ncm_mpd_song_list_destroy(&list);
    return ok;
}

static bool
append_song_list_content(NcmMpdSongList *list, NcmSongArray *songs) {
    if ((list == NULL) || (songs == NULL)) {
        return false;
    }
    for (int32 i = 0; i < list->count; i += 1) {
        if (!ncm_song_array_append_copy(songs, &list->items[i])) {
            return false;
        }
    }
    return true;
}

static bool
append_content_item(NativePlaylistEditorScreen *screen, int32 pos,
                    NcmSongArray *songs) {
    enum NcMenuItemSource source;
    NcMenu *menu;

    menu = nc_song_menu_base(&screen->content);
    source = NC_MENU_ITEMS_ALL;
    if (nc_menu_is_filtered(menu)) {
        source = NC_MENU_ITEMS_FILTERED;
    }
    return append_content_item_from_source(screen, source, pos, songs);
}

static bool
append_content_item_from_source(NativePlaylistEditorScreen *screen,
                                enum NcMenuItemSource source, int32 pos,
                                NcmSongArray *songs) {
    NcmSong *song;

    song = nc_menu_item_at(nc_song_menu_base(&screen->content), source,
                           pos);
    if (song == NULL) {
        return false;
    }
    return ncm_song_array_append_copy(songs, song);
}

static bool
playlist_editor_search_menu(NativePlaylistEditorScreen *screen,
                            NcMenu *menu, NcmRegex *regex, bool forward,
                            bool wrap, bool skip_current) {
    PlaylistEditorSearchContext context;

    context.screen = screen;
    context.regex = regex;
    return nc_menu_search_selectable(menu, screen->main_height, forward,
                                     wrap, skip_current,
                                     playlist_editor_search_position,
                                     &context, NULL);
}

static bool
playlist_editor_search_position(NcMenu *menu, int32 pos,
                                void *user) {
    PlaylistEditorSearchContext *context;

    context = user;
    return playlist_editor_search_item(context->screen, menu,
                                       context->regex, pos);
}

static bool
playlist_editor_search_item(NativePlaylistEditorScreen *screen,
                            NcMenu *menu, NcmRegex *regex, int32 pos) {
    void *item;

    if ((item = nc_menu_active_item_at(menu, pos)) == NULL) {
        return false;
    }
    if (menu->item_callbacks.item_size == SIZEOF(NcmPlaylist)) {
        NcmPlaylist *playlist;

        playlist = item;
        return playlist_editor_playlist_matches_regex(regex, playlist);
    }
    return playlist_editor_content_matches_regex(screen, regex, item);
}

static void
playlist_editor_set_display_callbacks(NativePlaylistEditorScreen *screen) {
    NcMenuDisplayCallbacks callbacks = {0};

    callbacks.draw = playlist_draw_callback;
    callbacks.filter = playlist_filter_callback;
    callbacks.user = screen;
    nc_menu_set_display_callbacks(nc_playlist_entry_menu_base(
                                      &screen->playlists), callbacks);

    callbacks = (NcMenuDisplayCallbacks){0};
    callbacks.draw = content_draw_callback;
    callbacks.filter = content_filter_callback;
    callbacks.user = screen;
    nc_menu_set_display_callbacks(nc_song_menu_base(&screen->content),
                                  callbacks);
    return;
}

#endif /* NCMPCPP_NC_PLAYLIST_EDITOR_C */
