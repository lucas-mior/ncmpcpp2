#if !defined(NCMPCPP_NC_PLAYLIST_EDITOR_C)
#define NCMPCPP_NC_PLAYLIST_EDITOR_C

#include "cbase.h"

#include "actions.h"
#include "app_controller.h"
#include "c/ncm_c.h"
#include "global.h"
#include "screens/nc_screens.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"

static NcWindow *playlist_editor_active_window_callback(NcScreen *screen);
static void playlist_editor_refresh_callback(NcScreen *screen);
static void playlist_editor_refresh_window_callback(NcScreen *screen);
static void playlist_editor_scroll_callback(NcScreen *screen, enum NcScroll where);
static void playlist_editor_finish_list_change_callback(NcScreen *screen);
static void playlist_editor_switch_to_callback(NcScreen *screen);
static void playlist_editor_resize_callback(NcScreen *screen);
static int32 playlist_editor_timeout_callback(NcScreen *screen);
static char *playlist_editor_title_callback(NcScreen *screen);
static void playlist_editor_update_callback(NcScreen *screen);
static void playlist_editor_mouse_callback(NcScreen *screen, MEVENT event);
static void playlist_editor_destroy_callback(NcScreen *screen);
static bool playlist_filter_callback(NcMenu *menu, void *item, void *user);
static bool content_filter_callback(NcMenu *menu, void *item, void *user);
static void playlist_draw_callback(NcMenu *menu, NcWindow *window, void *item, int32 pos, void *user);
static void content_draw_callback(NcMenu *menu, NcWindow *window, void *item, int32 pos, void *user);

// declarations to delete
static int32 playlist_editor_locate_song_in_playlist_range(PlaylistEditorScreen *, NcmMpdClient *, NcmSong *, int32 , int32 , NcmError *);
static int32 append_content_item_from_source(PlaylistEditorScreen *, enum NcMenuItemSource , int32 , NcmSongArray *);
static int32 playlist_editor_find_song_in_content_range(PlaylistEditorScreen *, NcmSong *, int32 , int32);
static bool playlist_editor_has_current_playlist_path(PlaylistEditorScreen *, char **, int32 *);
static bool playlist_editor_content_matches_regex(PlaylistEditorScreen *, NcmRegex *, NcmSong *);
static int32 playlist_editor_store_current_playlist_path(PlaylistEditorScreen *, StrBuilder *);
static void playlist_editor_refresh_window(PlaylistEditorScreen *, NcWindow *, NcMenu *);
static int32 playlist_editor_restore_playlist_path(PlaylistEditorScreen *, StrBuilder *);
static int32 playlist_editor_highlight_content_position(PlaylistEditorScreen *, int32);
static void playlist_editor_report_error(char *, int32 , NcmError *);
static int32 playlist_editor_restore_content_song(PlaylistEditorScreen *, NcmSong *);
static int32 append_content_item(PlaylistEditorScreen *, int32 , NcmSongArray *);
static void playlist_editor_update_titles(PlaylistEditorScreen *, bool);
static int32 playlist_editor_store_current_song(PlaylistEditorScreen *, NcmSong *);
static void playlist_editor_mouse_scroll(PlaylistEditorScreen *, enum NcScroll);
static bool playlist_editor_playlist_matches_regex(NcmRegex *, NcmPlaylist *);
static bool playlist_editor_search_text_matches(NcmRegex *, char *, int32);
static bool playlist_editor_displayed_playlist_is_current(PlaylistEditorScreen *);
static void playlist_editor_observe_current_playlist(PlaylistEditorScreen *);
static void playlist_editor_finish_playlist_change( PlaylistEditorScreen *);
static void playlist_editor_update_menu_highlights(PlaylistEditorScreen *);
static bool playlist_editor_search_position(NcMenu *, int32 , void *);
static void playlist_editor_clear_playlist_filter(PlaylistEditorScreen *);
static void playlist_editor_clear_stale_content( PlaylistEditorScreen *);
static void playlist_editor_clear_content_filter(PlaylistEditorScreen *);
static void playlist_editor_reset_content_timer(PlaylistEditorScreen *);
static void playlist_editor_apply_geometry(PlaylistEditorScreen *);
static int32 playlist_editor_show_screen(PlaylistEditorScreen *);
static int32 playlist_editor_separator_width(int32);

typedef struct PlaylistEditorSearchContext {
    PlaylistEditorScreen *screen;
    NcmRegex *regex;
} PlaylistEditorSearchContext;

void
playlist_editor_screen_init(PlaylistEditorScreen *screen,
                            int32 start_x, int32 width,
                            int32 main_start_y, int32 main_height,
                            NcColor color, NcBorder border) {
    NcScreenOps callbacks = {0};
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

    callbacks.active_window = playlist_editor_active_window_callback;
    callbacks.refresh = playlist_editor_refresh_callback;
    callbacks.refresh_window = playlist_editor_refresh_window_callback;
    callbacks.scroll = playlist_editor_scroll_callback;
    callbacks.list_change_finished =
        playlist_editor_finish_list_change_callback;
    callbacks.switch_to = playlist_editor_switch_to_callback;
    callbacks.resize = playlist_editor_resize_callback;
    callbacks.window_timeout_callback = playlist_editor_timeout_callback;
    callbacks.title = playlist_editor_title_callback;
    callbacks.update = playlist_editor_update_callback;
    callbacks.mouse_button_pressed = playlist_editor_mouse_callback;
    callbacks.lockable = true;
    callbacks.mergable = true;
    callbacks.destroy = playlist_editor_destroy_callback;

    nc_playlist_entry_menu_init(&screen->playlists);
    nc_song_menu_init(&screen->content);

    screen->playlist_filter_constraint = (StrBuilder){0};
    screen->content_filter_constraint = (StrBuilder){0};
    screen->playlist_search_constraint = (StrBuilder){0};
    screen->content_search_constraint = (StrBuilder){0};
    screen->playlists_title = (StrBuilder){0};
    screen->content_title = (StrBuilder){0};
    screen->displayed_playlist_path = (StrBuilder){0};
    screen->observed_playlist_path = (StrBuilder){0};

    screen->playlist_filter_regex = (NcmRegex){0};
    screen->content_filter_regex = (NcmRegex){0};
    screen->playlist_search_regex = (NcmRegex){0};
    screen->content_search_regex = (NcmRegex){0};
    playlist_editor_reset_content_timer(screen);

    screen->active_column = PLAYLIST_EDITOR_COLUMN_PLAYLISTS;
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
        screen->fetching_delay_ms = PLAYLIST_EDITOR_FETCH_DELAY_MS;
        screen->window_timeout_ms = PLAYLIST_EDITOR_FETCH_DELAY_MS;
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
    playlist_editor_screen_set_geometry(screen, start_x, width,
                                        main_start_y, main_height);
    nc_screen_init_ops(&screen->screen, callbacks, screen,
                       NC_SCREEN_TYPE_PLAYLIST_EDITOR);
    {
        NcMenu *playlists;
        NcMenu *content;

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
    }
    {
        NcMenuDisplayCallbacks display_callbacks = {0};

        display_callbacks.draw = playlist_draw_callback;
        display_callbacks.matches_filter = playlist_filter_callback;
        display_callbacks.user = screen;
        nc_menu_set_display_callbacks(nc_playlist_entry_menu_base(
            &screen->playlists), display_callbacks);

        display_callbacks = (NcMenuDisplayCallbacks){0};
        display_callbacks.draw = content_draw_callback;
        display_callbacks.matches_filter = content_filter_callback;
        display_callbacks.user = screen;
        nc_menu_set_display_callbacks(nc_song_menu_base(&screen->content),
                                      display_callbacks);
    }

    return;
}

void
playlist_editor_screen_destroy(PlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    (void)app_controller_unregister_screen(playlist_editor_screen_base(screen));
    nc_window_destroy(&screen->content_window);
    nc_window_destroy(&screen->playlists_window);
    nc_song_menu_destroy(&screen->content);
    nc_playlist_entry_menu_destroy(&screen->playlists);
    ncm_regex_destroy(&screen->content_search_regex);
    ncm_regex_destroy(&screen->playlist_search_regex);
    ncm_regex_destroy(&screen->content_filter_regex);
    ncm_regex_destroy(&screen->playlist_filter_regex);
    sb_free(&screen->observed_playlist_path);
    sb_free(&screen->displayed_playlist_path);
    sb_free(&screen->content_title);
    sb_free(&screen->playlists_title);
    sb_free(&screen->content_search_constraint);
    sb_free(&screen->playlist_search_constraint);
    sb_free(&screen->content_filter_constraint);
    sb_free(&screen->playlist_filter_constraint);
    screen->registered = false;
    return;
}

NcScreen *
playlist_editor_screen_base(PlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->screen;
}

NcPlaylistEntryMenu *
playlist_editor_screen_playlists(PlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->playlists;
}

NcSongMenu *
playlist_editor_screen_content(PlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->content;
}

NcMenu *
playlist_editor_screen_active_menu(PlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    if (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT) {
        return nc_song_menu_base(&screen->content);
    }
    return nc_playlist_entry_menu_base(&screen->playlists);
}

NcWindow *
playlist_editor_screen_active_window(PlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    if (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT) {
        return &screen->content_window;
    }
    return &screen->playlists_window;
}

void
playlist_editor_screen_set_geometry(PlaylistEditorScreen *screen,
                                    int32 start_x, int32 width,
                                    int32 main_start_y, int32 main_height) {
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
playlist_editor_screen_set_column_ratio(PlaylistEditorScreen *screen,
                                        int32 left, int32 right) {
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
playlist_editor_screen_can_move_to_previous_column(
    PlaylistEditorScreen *screen
) {
    NcMenu *playlists;

    if (screen == NULL) {
        return false;
    }
    playlists = nc_playlist_entry_menu_base(&screen->playlists);
    return (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT)
           && (nc_menu_all_item_count(playlists) > 0);
}

bool
playlist_editor_screen_can_move_to_next_column(PlaylistEditorScreen *screen) {
    NcMenu *content;

    if (screen == NULL) {
        return false;
    }
    content = nc_song_menu_base(&screen->content);
    return (screen->active_column == PLAYLIST_EDITOR_COLUMN_PLAYLISTS)
           && (nc_menu_all_item_count(content) > 0);
}

void
playlist_editor_screen_previous_column(
    PlaylistEditorScreen *screen
) {
    if (playlist_editor_screen_can_move_to_previous_column(screen)) {
        screen->active_column = PLAYLIST_EDITOR_COLUMN_PLAYLISTS;
        playlist_editor_update_menu_highlights(screen);
    }
    return;
}

void
playlist_editor_screen_next_column(PlaylistEditorScreen *screen) {
    if (playlist_editor_screen_can_move_to_next_column(screen)) {
        screen->active_column = PLAYLIST_EDITOR_COLUMN_CONTENT;
        playlist_editor_update_menu_highlights(screen);
    }
    return;
}

int32
playlist_editor_screen_load_playlists(PlaylistEditorScreen *screen,
                                      NcmMpdPlaylistList *playlists) {
    StrBuilder preserved = {0};
    NcMenu *menu;
    bool had_preserved;
    int32 status;

    if ((screen == NULL) || (playlists == NULL)) {
        return -EINVAL;
    }
    status = playlist_editor_store_current_playlist_path(screen,
                                                         &preserved);
    if (status < 0) {
        sb_free(&preserved);
        return status;
    }
    had_preserved = status > 0;
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
    sb_free(&preserved);
    return 0;
}

int32
playlist_editor_screen_reload_playlists_from_mpd(PlaylistEditorScreen *screen,
                                                 NcmMpdClient *client,
                                                 NcmError *ncm_error) {
    NcmMpdPlaylistList playlists;
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing playlist editor"));
    }
    playlists = (NcmMpdPlaylistList){0};
    status = ncm_mpd_client_get_playlists(client, &playlists, ncm_error);
    if (status == 0) {
        for (int32 i = 1; i < playlists.count; i += 1) {
            NcmPlaylist current = {0};
            int32 j = i;

            ncm_playlist_move(&current, &playlists.items[i]);
            while (j > 0) {
                NcmPlaylist *left;
                NcmPlaylist *right;
                int32 comparison;

                left = &playlists.items[j - 1];
                right = &current;
                if ((left == NULL) || (left->path == NULL)) {
                    if ((right == NULL) || (right->path == NULL)) {
                        comparison = 0;
                    } else {
                        comparison = -1;
                    }
                } else if ((right == NULL) || (right->path == NULL)) {
                    comparison = 1;
                } else {
                    comparison = ncm_compare_locale_strings(
                        left->path, left->path_len,
                        right->path, right->path_len,
                        Config.ignore_leading_the);
                }
                if (comparison <= 0) {
                    break;
                }
                ncm_playlist_move(&playlists.items[j],
                                  &playlists.items[j - 1]);
                j -= 1;
            }
            ncm_playlist_move(&playlists.items[j], &current);
            ncm_playlist_destroy(&current);
        }
        status = playlist_editor_screen_load_playlists(screen, &playlists);
        if (status < 0) {
            status = ncm_error_set_status(
                ncm_error, status, STRLIT("could not copy playlists"));
        }
    }
    ncm_mpd_playlist_list_destroy(&playlists);
    return status;
}

int32
playlist_editor_screen_load_content(PlaylistEditorScreen *screen,
                                    NcmMpdSongList *songs) {
    NcMenu *menu;
    NcmSong preserved_song;
    bool had_preserved_song;
    int32 status;

    if ((screen == NULL) || (songs == NULL)) {
        return -EINVAL;
    }
    preserved_song = (NcmSong){0};
    status = playlist_editor_store_current_song(screen, &preserved_song);
    if (status < 0) {
        ncm_song_destroy(&preserved_song);
        return status;
    }
    had_preserved_song = status > 0;

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
    {
        char *path;
        int32 path_len;

        if (!playlist_editor_has_current_playlist_path(screen,
                                                       &path, &path_len)) {
            sb_clear(&screen->displayed_playlist_path);
            screen->displayed_playlist_valid = false;
        } else {
            sb_set(&screen->displayed_playlist_path, path, path_len);
            screen->displayed_playlist_valid = true;
        }
    }
    playlist_editor_observe_current_playlist(screen);
    screen->last_known_content_count = nc_menu_all_item_count(menu);
    screen->content_update_requested = false;
    playlist_editor_update_titles(screen, true);
    ncm_song_destroy(&preserved_song);
    return 0;
}

int32
playlist_editor_screen_reload_content_from_mpd(PlaylistEditorScreen *screen,
                                               NcmMpdClient *client,
                                               NcmError *ncm_error) {
    NcmMpdSongList songs;
    NcmPlaylist *playlist;
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing playlist editor"));
    }
    if ((playlist = nc_playlist_entry_menu_current(&screen->playlists)) == NULL
        || (playlist->path == NULL)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing playlist"));
    }

    songs = (NcmMpdSongList){0};
    status = ncm_mpd_client_get_playlist_content(
        client, playlist->path, &songs, ncm_error);
    if (status == 0) {
        status = playlist_editor_screen_load_content(screen, &songs);
        if (status < 0) {
            status = ncm_error_set_status(
                ncm_error, status, STRLIT("could not copy playlist content"));
        }
    }
    ncm_mpd_song_list_destroy(&songs);
    return status;
}

int32
playlist_editor_screen_locate_playlist(
    PlaylistEditorScreen *screen, NcmMpdClient *client,
    char *path, int32 path_len, NcmError *ncm_error
) {
    NcMenu *menu;
    int32 pos;
    int32 status;

    if ((screen == NULL) || (path == NULL) || (path_len <= 0)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing playlist"));
    }
    status = playlist_editor_screen_reload_playlists_from_mpd(
        screen, client, ncm_error);
    if (status < 0) {
        return status;
    }

    playlist_editor_clear_playlist_filter(screen);
    pos = -ENOENT;
    menu = nc_playlist_entry_menu_base(&screen->playlists);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmPlaylist *playlist;

        playlist = nc_menu_active_item_at(menu, i);
        if (playlist
            && STREQUAL(playlist->path, playlist->path_len,
                        path, path_len)) {
            pos = i;
            break;
        }
    }
    if (pos < 0) {
        return ncm_error_set_status(ncm_error, -ENOENT,
                                    STRLIT("playlist not found"));
    }

    menu = nc_playlist_entry_menu_base(&screen->playlists);
    nc_menu_highlight_position(menu, pos,
                               nc_window_height(
                                   &screen->playlists_window));
    screen->active_column = PLAYLIST_EDITOR_COLUMN_PLAYLISTS;
    playlist_editor_update_menu_highlights(screen);
    playlist_editor_clear_content_filter(screen);
    playlist_editor_clear_stale_content(screen);
    status = playlist_editor_screen_reload_content_from_mpd(
        screen, client, ncm_error);
    if (status < 0) {
        return status;
    }
    return playlist_editor_show_screen(screen);
}

int32
playlist_editor_screen_locate_song(PlaylistEditorScreen *screen,
                                   NcmMpdClient *client,
                                   NcmSong *song,
                                   NcmError *ncm_error) {
    NcMenu *playlists;
    NcMenu *content;
    NcmSong current_song;
    int32 playlist_pos;
    int32 song_pos;
    int32 found_pos;
    int32 status;

    if ((screen == NULL) || (song == NULL)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing song"));
    }
    playlists = nc_playlist_entry_menu_base(&screen->playlists);
    if ((nc_menu_all_item_count(playlists) <= 0)
        || screen->playlists_update_requested) {
        status = playlist_editor_screen_reload_playlists_from_mpd(
            screen, client, ncm_error);
        if (status < 0) {
            return status;
        }
    }
    if (nc_menu_all_item_count(playlists) <= 0) {
        return ncm_error_set_status(ncm_error, -ENOENT,
                                    STRLIT("playlist list is empty"));
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
        status = playlist_editor_highlight_content_position(screen,
                                                           found_pos);
        if (status < 0) {
            return ncm_error_set_status(
                ncm_error, status, STRLIT("song is not in playlist view"));
        }
        return playlist_editor_show_screen(screen);
    }

    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Jumping to song...");
    status = playlist_editor_locate_song_in_playlist_range(
        screen, client, song, playlist_pos + 1,
        nc_menu_all_item_count(playlists), ncm_error);
    if (status > 0) {
        return playlist_editor_show_screen(screen);
    }
    if (status < 0) {
        return status;
    }

    status = playlist_editor_locate_song_in_playlist_range(
        screen, client, song, 0, playlist_pos, ncm_error);
    if (status > 0) {
        return playlist_editor_show_screen(screen);
    }
    if (status < 0) {
        return status;
    }

    found_pos = playlist_editor_find_song_in_content_range(
        screen, song, 0, song_pos);
    if (found_pos >= 0) {
        status = playlist_editor_highlight_content_position(screen,
                                                           found_pos);
        if (status < 0) {
            return ncm_error_set_status(
                ncm_error, status, STRLIT("song is not in playlist view"));
        }
        return playlist_editor_show_screen(screen);
    }

    current_song = (NcmSong){0};
    status = playlist_editor_screen_current_content_song(
        screen, &current_song);
    if (status > 0) {
        if (ncm_song_is_equal(&current_song, song)) {
            ncm_song_destroy(&current_song);
            screen->active_column = PLAYLIST_EDITOR_COLUMN_CONTENT;
            playlist_editor_update_menu_highlights(screen);
            return playlist_editor_show_screen(screen);
        }
    }
    ncm_song_destroy(&current_song);
    if (status < 0) {
        return ncm_error_set_status(
            ncm_error, status, STRLIT("could not copy current song"));
    }

    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Song was not found in playlists");
    return ncm_error_set_status(ncm_error, -ENOENT,
                                STRLIT("song was not found in playlists"));
}

int32
playlist_editor_screen_current_playlist(
    PlaylistEditorScreen *screen, NcmPlaylist *playlist
) {
    NcmPlaylist *current;
    int32 status;

    if ((screen == NULL) || (playlist == NULL)) {
        return -EINVAL;
    }
    if ((current = nc_playlist_entry_menu_current(&screen->playlists))
        == NULL) {
        return 0;
    }
    if ((status = ncm_playlist_copy(playlist, current)) < 0) {
        return status;
    }
    return 1;
}

int32
playlist_editor_screen_current_song(
    PlaylistEditorScreen *screen, NcmSong *song
) {
    return playlist_editor_screen_current_content_song(screen, song);
}

int32
playlist_editor_screen_current_content_song(
    PlaylistEditorScreen *screen, NcmSong *song
) {
    NcmSong *current;
    int32 status;

    if ((screen == NULL) || (song == NULL)) {
        return -EINVAL;
    }
    if ((current = nc_song_menu_current(&screen->content)) == NULL) {
        return 0;
    }
    if ((status = ncm_song_copy(song, current)) < 0) {
        return status;
    }
    return 1;
}

int32
playlist_editor_screen_selected_playlist_count(
    PlaylistEditorScreen *screen
) {
    if (screen == NULL) {
        return 0;
    }
    return nc_menu_selected_count(
        nc_playlist_entry_menu_base(&screen->playlists));
}

int32
playlist_editor_screen_selected_songs(
    PlaylistEditorScreen *screen, NcmSongArray *songs
) {
    NcMenu *menu;
    int32 status;

    if (songs) {
        ncm_song_array_clear(songs);
    }
    if ((screen == NULL) || (songs == NULL)) {
        return -EINVAL;
    }
    if (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT) {
        menu = nc_song_menu_base(&screen->content);
        if (!nc_menu_has_selected(menu)) {
            return append_content_item(screen, nc_menu_highlight(menu), songs);
        }
        for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
            if (!nc_menu_position_is_selected(menu, i)) {
                continue;
            }
            status = append_content_item(screen, i, songs);
            if (status < 0) {
                return status;
            }
        }
        return 0;
    }
    if (playlist_editor_screen_selected_playlist_count(screen) > 0) {
        menu = nc_playlist_entry_menu_base(&screen->playlists);
        for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
            NcmMpdSongList list = {0};
            NcmError ncm_error = {0};
            NcmPlaylist *playlist;

            if (!nc_menu_position_is_selected(menu, i)) {
                continue;
            }
            playlist = nc_menu_active_item_at(menu, i);
            if ((playlist == NULL) || (playlist->path == NULL)) {
                ncm_song_array_clear(songs);
                return -EINVAL;
            }

            ncm_error_clear(&ncm_error);
            status = ncm_mpd_client_get_playlist_content(
                &global_mpd, playlist->path, &list, &ncm_error);
            if (status < 0) {
                playlist_editor_report_error(
                    STRLIT("Could not fetch playlist content"), &ncm_error);
                ncm_error_clear(&ncm_error);
                ncm_mpd_song_list_destroy(&list);
                ncm_song_array_clear(songs);
                return status;
            }

            for (int32 j = 0; j < list.count; j += 1) {
                status = ncm_song_array_append_copy(songs, &list.items[j]);
                if (status < 0) {
                    ncm_mpd_song_list_destroy(&list);
                    ncm_song_array_clear(songs);
                    return status;
                }
            }
            ncm_mpd_song_list_destroy(&list);
        }
        return 0;
    }

    menu = nc_song_menu_base(&screen->content);
    for (int32 i = 0; i < nc_menu_all_item_count(menu); i += 1) {
        status = append_content_item_from_source(
            screen, NC_MENU_ITEMS_ALL, i, songs);
        if (status < 0) {
            return status;
        }
    }
    return 0;
}

int32
playlist_editor_screen_apply_active_filter(
    PlaylistEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, NcmError *ncm_error
) {
    NcMenu *menu;
    NcmRegex *regex;
    StrBuilder *constraint;
    bool *enabled;
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing playlist editor"));
    }
    menu = playlist_editor_screen_active_menu(screen);
    if (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT) {
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
        sb_clear(constraint);
        nc_menu_show_all_items(menu);
        playlist_editor_update_titles(screen, true);
        return ncm_error_ok(ncm_error);
    }
    if ((status = ncm_regex_compile(regex, pattern, pattern_len,
                                    regex_flags, ncm_error)) < 0) {
        return status;
    }
    if ((status = sb_set(constraint, pattern, pattern_len)) < 0) {
        return ncm_error_set_status(ncm_error, status,
                                    STRLIT("failed to save filter"));
    }
    *enabled = true;
    nc_menu_apply_filter(menu);
    playlist_editor_update_titles(screen, true);
    return ncm_error_ok(ncm_error);
}

int32
playlist_editor_screen_search_active(
    PlaylistEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, bool forward, bool wrap, bool skip_current,
    NcmError *ncm_error
) {
    PlaylistEditorSearchContext context;
    StrBuilder *constraint;
    NcmRegex *regex;
    NcMenu *menu;
    bool *enabled;
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing playlist editor"));
    }
    if (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT) {
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
        sb_clear(constraint);
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing search pattern"));
    }
    if ((status = ncm_regex_compile(regex, pattern, pattern_len,
                                    regex_flags, ncm_error)) < 0) {
        return status;
    }
    if ((status = sb_set(constraint, pattern, pattern_len)) < 0) {
        return ncm_error_set_status(ncm_error, status,
                                    STRLIT("failed to save search"));
    }
    *enabled = true;
    menu = playlist_editor_screen_active_menu(screen);
    context.screen = screen;
    context.regex = regex;
    if (nc_menu_search_selectable(menu, screen->main_height, forward,
                                  wrap, skip_current,
                                  playlist_editor_search_position,
                                  &context, NULL) == 0) {
        playlist_editor_finish_playlist_change(screen);
        return 1;
    }
    return 0;
}

void
playlist_editor_screen_request_playlists_update(
    PlaylistEditorScreen *screen
) {
    if (screen == NULL) {
        return;
    }
    screen->playlists_update_requested = true;
    nc_screen_request_update(&screen->screen);
    return;
}

void
playlist_editor_screen_request_content_update(
    PlaylistEditorScreen *screen
) {
    if (screen == NULL) {
        return;
    }
    screen->content_update_requested = true;
    playlist_editor_reset_content_timer(screen);
    nc_screen_request_update(&screen->screen);
    return;
}

static PlaylistEditorScreen *
playlist_editor_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}


static NcWindow *
playlist_editor_active_window_callback(NcScreen *screen) {
    PlaylistEditorScreen *editor;

    editor = playlist_editor_from_screen(screen);
    return playlist_editor_screen_active_window(editor);
}

static void
playlist_editor_refresh_callback(NcScreen *screen) {
    PlaylistEditorScreen *editor;

    editor = playlist_editor_from_screen(screen);
    playlist_editor_update_titles(editor, true);
    playlist_editor_update_menu_highlights(editor);
    playlist_editor_refresh_window(editor, &editor->playlists_window,
                                   nc_playlist_entry_menu_base(
                                       &editor->playlists));
    if (playlist_editor_separator_width(editor->width) > 0) {
        nc_screen_draw_vertical_separator(editor->right_start_x - 1);
    }
    playlist_editor_refresh_window(editor, &editor->content_window,
                                   nc_song_menu_base(&editor->content));
    return;
}

static void
playlist_editor_refresh_window_callback(NcScreen *screen) {
    PlaylistEditorScreen *editor;
    NcWindow *window;
    NcMenu *menu;

    editor = playlist_editor_from_screen(screen);
    playlist_editor_update_titles(editor, true);
    playlist_editor_update_menu_highlights(editor);
    window = playlist_editor_screen_active_window(editor);
    menu = playlist_editor_screen_active_menu(editor);
    playlist_editor_refresh_window(editor, window, menu);
    return;
}

static void
playlist_editor_scroll_callback(NcScreen *screen, enum NcScroll where) {
    PlaylistEditorScreen *editor;
    NcMenu *menu;

    editor = playlist_editor_from_screen(screen);
    menu = playlist_editor_screen_active_menu(editor);
    nc_menu_scroll_selectable(menu, editor->main_height, where);
    return;
}

static void
playlist_editor_finish_list_change_callback(NcScreen *screen) {
    PlaylistEditorScreen *editor;

    editor = playlist_editor_from_screen(screen);
    playlist_editor_finish_playlist_change(editor);
    return;
}

static void
playlist_editor_switch_to_callback(NcScreen *screen) {
    playlist_editor_refresh_callback(screen);
    return;
}

static void
playlist_editor_resize_callback(NcScreen *screen) {
    PlaylistEditorScreen *editor;
    NcScreenResizeParams params;

    editor = playlist_editor_from_screen(screen);
    params = nc_screen_resize_params(screen);
    playlist_editor_screen_set_geometry(editor, params.x_offset,
                                        params.width,
                                        editor->main_start_y,
                                        editor->main_height);
    nc_screen_clear_resize_request(screen);
    return;
}

static int32
playlist_editor_timeout_callback(NcScreen *screen) {
    PlaylistEditorScreen *editor;

    editor = playlist_editor_from_screen(screen);
    if ((editor->fetching_delay_ms >= 0)
        && nc_menu_is_empty(nc_song_menu_base(&editor->content))
        && (nc_menu_is_empty(nc_playlist_entry_menu_base(&editor->playlists))
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
    PlaylistEditorScreen *editor;
    NcmError ncm_error = {0};
    NcMenu *playlists;
    NcMenu *content;
    bool content_fetch_due;
    bool displayed_playlist_is_current;
    int32 changed;
    int32 status;

    editor = playlist_editor_from_screen(screen);
    playlist_editor_finish_playlist_change(editor);

    changed = 0;
    ncm_error_clear(&ncm_error);
    if (editor->playlists_update_requested
        || nc_menu_is_empty(nc_playlist_entry_menu_base(
            &editor->playlists))) {
        status = playlist_editor_screen_reload_playlists_from_mpd(
            editor, &global_mpd, &ncm_error);
        if (status < 0) {
            editor->playlists_update_requested = false;
            playlist_editor_report_error(
                STRLIT("Could not fetch playlists"), &ncm_error);
            ncm_error_clear(&ncm_error);
            playlist_editor_update_titles(editor, true);
            goto update_finished;
        }
        changed = 1;
    }

    playlist_editor_finish_playlist_change(editor);
    content_fetch_due = false;
    playlists = nc_playlist_entry_menu_base(&editor->playlists);
    if (!nc_menu_is_empty(playlists)) {
        content = nc_song_menu_base(&editor->content);
        displayed_playlist_is_current =
            playlist_editor_displayed_playlist_is_current(editor);
        if (editor->content_update_requested
            && displayed_playlist_is_current) {
            content_fetch_due = true;
        } else if (!displayed_playlist_is_current
                   && !((editor->last_known_content_count == 0)
                        && editor->displayed_playlist_valid)
                   && nc_menu_is_empty(content)) {
            if (editor->fetching_delay_ms < 0) {
                content_fetch_due = true;
            } else {
                content_fetch_due = global_timer_elapsed_ms(editor->timer)
                                    > editor->fetching_delay_ms;
            }
        }
    }
    if (!content_fetch_due) {
        playlist_editor_update_titles(editor, true);
        goto update_finished;
    }

    ncm_error_clear(&ncm_error);
    status = playlist_editor_screen_reload_content_from_mpd(
        editor, &global_mpd, &ncm_error);
    if (status < 0) {
        editor->content_update_requested = false;
        playlist_editor_report_error(
            STRLIT("Could not fetch playlist content"), &ncm_error);
        ncm_error_clear(&ncm_error);
        playlist_editor_update_titles(editor, true);
        goto update_finished;
    }
    changed = 1;
    playlist_editor_update_titles(editor, true);

update_finished:
    nc_screen_clear_update_request(screen);
    if ((changed > 0) && app_controller_is_screen_visible(screen)) {
        nc_screen_refresh(screen);
    }
    return;
}

static void
playlist_editor_mouse_callback(NcScreen *screen, MEVENT event) {
    PlaylistEditorScreen *editor;
    int32 x;
    int32 y;

    if ((editor = playlist_editor_from_screen(screen)) == NULL) {
        return;
    }

    x = event.x;
    y = event.y;
    if (nc_window_has_coords(&editor->playlists_window, &x, &y)) {
        if (editor->active_column
            != PLAYLIST_EDITOR_COLUMN_PLAYLISTS) {
            if (!playlist_editor_screen_can_move_to_previous_column(
                editor)) {
                return;
            }
            playlist_editor_screen_previous_column(editor);
        }
        if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
            NcMenu *menu;

            menu = nc_playlist_entry_menu_base(&editor->playlists);
            if ((y >= 0) && (y < nc_menu_item_count(menu))
                && (nc_menu_goto_selectable(menu, y) >= 0)) {
                playlist_editor_finish_playlist_change(editor);
                if (event.bstate & BUTTON3_PRESSED) {
                    NcmPlaylist *playlist;

                    playlist = nc_playlist_entry_menu_current(
                        &editor->playlists);
                    if ((playlist != NULL) && (playlist->path != NULL)) {
                        NcmError ncm_error = {0};
                        bool loaded;
                        int32 status;

                        loaded = false;
                        ncm_error_clear(&ncm_error);
                        status = ncm_mpd_client_load_playlist(
                            &global_mpd, playlist->path,
                            &loaded, &ncm_error);
                        if (status < 0) {
                            playlist_editor_report_error(
                                STRLIT("Could not load playlist"),
                                &ncm_error);
                        } else if (loaded) {
                            StrBuilder message = {0};

                            SB_APPEND(&message, "Playlist \"");
                            SB_APPEND(&message, playlist->path,
                                      playlist->path_len);
                            SB_APPEND(&message, "\" loaded");
                            ncm_statusbar_print(
                                Config.message_delay_time,
                                message.data, message.len);
                            sb_free(&message);
                            (void)ncm_status_update_full(
                                &global_mpd, NULL, &ncm_error);
                        }
                    }
                }
            }
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
            != PLAYLIST_EDITOR_COLUMN_CONTENT) {
            if (!playlist_editor_screen_can_move_to_next_column(
                editor)) {
                return;
            }
            playlist_editor_screen_next_column(editor);
        }
        if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
            NcMenu *menu;

            menu = nc_song_menu_base(&editor->content);
            if ((y >= 0) && (y < nc_menu_item_count(menu))
                && (nc_menu_goto_selectable(menu, y) >= 0)
                && (event.bstate & BUTTON3_PRESSED)) {
                NcmSong *song;

                song = nc_song_menu_current(&editor->content);
                if (song != NULL) {
                    (void)ncm_action_add_song_to_playlist(song, true, -1);
                }
            }
        } else if (event.bstate & BUTTON5_PRESSED) {
            playlist_editor_mouse_scroll(editor, NC_SCROLL_DOWN);
        } else if (event.bstate & BUTTON4_PRESSED) {
            playlist_editor_mouse_scroll(editor, NC_SCROLL_UP);
        }
    }
    return;
}


static void
playlist_editor_destroy_callback(NcScreen *screen) {
    playlist_editor_screen_destroy(playlist_editor_from_screen(screen));
    return;
}

static bool
playlist_filter_callback(NcMenu *menu, void *item, void *user) {
    PlaylistEditorScreen *editor;
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
    PlaylistEditorScreen *editor;

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

    (void)menu;
    (void)pos;
    (void)user;
    if ((window == NULL) || ((playlist = item) == NULL)
        || (playlist->path == NULL) || (playlist->path_len <= 0)) {
        return;
    }

    nc_window_print_data(window, playlist->path, playlist->path_len);
    return;
}

static void
content_draw_callback(NcMenu *menu, NcWindow *window, void *item,
                      int32 pos, void *user) {
    NcBuffer buffer;
    int32 list_width;
    bool use_colors;

    (void)user;
    ASSERT(menu != NULL);
    ASSERT(window != NULL);
    ASSERT(item != NULL);

    buffer = (NcBuffer){0};
    if (Config.playlist_editor_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
        list_width = nc_window_width(window) - nc_window_get_x(window);
        if (nc_menu_position_is_selected(menu, pos)) {
            list_width -= utf8_width(menu->selected_suffix.data,
                                     menu->selected_suffix.len);
        }
        if (!menu->highlight_disabled && (pos == menu->highlight)) {
            list_width -= utf8_width(menu->highlight_suffix.data,
                                     menu->highlight_suffix.len);
        }
        if (list_width < 0) {
            list_width = 0;
        }
        use_colors = !Config.discard_colors_if_item_is_selected
                     || !nc_menu_position_is_selected(menu, pos);
        ncm_display_song_columns(&buffer, item, Config.columns.items,
                                 Config.columns.len, list_width,
                                 use_colors);
    } else {
        ncm_display_song_row(&buffer, &Config.song_list_format, item,
                             NCM_FORMAT_FLAG_ALL);
    }
    {
        NcBufferProperty *properties;
        char *data;
        int32 property_count;
        int32 property_index;
        int32 len;

        data = nc_buffer_data(&buffer);
        len = nc_buffer_len(&buffer);
        properties = nc_buffer_properties(&buffer);
        property_count = nc_buffer_property_count(&buffer);
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
    }
    nc_buffer_destroy(&buffer);
    return;
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
playlist_editor_content_matches_regex(PlaylistEditorScreen *screen,
                                      NcmRegex *regex, NcmSong *song) {
    NcBuffer buffer;
    bool result;

    (void)screen;
    if (song == NULL) {
        return false;
    }

    buffer = (NcBuffer){0};
    if (Config.playlist_editor_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
        ncm_display_song_row(&buffer, &Config.song_columns_mode_format,
                             song, NCM_FORMAT_FLAG_ALL);
    } else {
        ncm_display_song_row(&buffer, &Config.song_list_format, song,
                             NCM_FORMAT_FLAG_ALL);
    }
    result = playlist_editor_search_text_matches(regex, buffer.data,
                                                 buffer.len);
    nc_buffer_destroy(&buffer);
    return result;
}


static bool
playlist_editor_search_text_matches(NcmRegex *regex, char *data,
                                    int32 len) {
    if (data == NULL) {
        data = "";
        len = 0;
    }
    return ncm_regex_matches(regex, data, len);
}


static void
playlist_editor_apply_geometry(PlaylistEditorScreen *screen) {
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
playlist_editor_update_menu_highlights(
    PlaylistEditorScreen *screen
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

    active = playlist_editor_screen_active_menu(screen);
    nc_menu_set_highlight_prefix(active, &Config.current_item_prefix);
    nc_menu_set_highlight_suffix(active, &Config.current_item_suffix);
    return;
}


static void
playlist_editor_update_titles(PlaylistEditorScreen *screen,
                              bool update_windows) {
    ASSERT(screen != NULL);

    if (screen->last_known_content_count >= 0) {
        screen->last_known_content_count = nc_menu_item_count(
            nc_song_menu_base(&screen->content));
    }

    sb_clear(&screen->playlists_title);
    sb_clear(&screen->content_title);
    if (Config.titles_visibility) {
        SB_APPEND(&screen->playlists_title,
                  "Playlists");
        SB_APPEND(&screen->content_title, "Content");
        if (screen->last_known_content_count >= 0) {
            SB_APPEND(&screen->content_title, " (");
            {
                char digits[32];
                int32 len;
                int32 value;

                value = screen->last_known_content_count;
                len = 0;
                if (value == 0) {
                    sb_append_byte(&screen->content_title, '0');
                } else {
                    while (value > 0) {
                        digits[len] = (char)('0' + (value % 10));
                        value /= 10;
                        len += 1;
                    }
                    for (int32 i = len - 1; i >= 0; i -= 1) {
                        sb_append_byte(&screen->content_title, digits[i]);
                    }
                }
            }
            if (screen->last_known_content_count == 1) {
                SB_APPEND(&screen->content_title,
                          " item)");
            } else {
                SB_APPEND(&screen->content_title,
                          " items)");
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
playlist_editor_reset_content_timer(PlaylistEditorScreen *screen) {
    ASSERT(screen != NULL);
    screen->timer = global_timer;
    return;
}

static bool
playlist_editor_has_current_playlist_path(PlaylistEditorScreen *screen,
                                      char **path, int32 *path_len) {
    NcmPlaylist *playlist;

    ASSERT(path != NULL);
    ASSERT(path_len != NULL);

    if (screen == NULL) {
        return false;
    }
    if ((playlist = nc_playlist_entry_menu_current(&screen->playlists)) == NULL
        || (playlist->path == NULL)) {
        return false;
    }
    *path = playlist->path;
    *path_len = playlist->path_len;
    return true;
}

static void
playlist_editor_clear_playlist_filter(
    PlaylistEditorScreen *screen
) {
    StrBuilder path = {0};
    int32 has_path;

    if (screen == NULL) {
        return;
    }
    has_path = playlist_editor_store_current_playlist_path(screen, &path);
    screen->playlist_filter_enabled = false;
    sb_clear(&screen->playlist_filter_constraint);
    nc_menu_show_all_items(nc_playlist_entry_menu_base(
        &screen->playlists));
    if (has_path > 0) {
        (void)playlist_editor_restore_playlist_path(screen, &path);
    }
    sb_free(&path);
    playlist_editor_update_titles(screen, true);
    return;
}

static void
playlist_editor_clear_content_filter(
    PlaylistEditorScreen *screen
) {
    NcmSong song;
    int32 has_song;

    if (screen == NULL) {
        return;
    }
    song = (NcmSong){0};
    has_song = playlist_editor_store_current_song(screen, &song);
    screen->content_filter_enabled = false;
    sb_clear(&screen->content_filter_constraint);
    nc_menu_show_all_items(nc_song_menu_base(&screen->content));
    if (has_song > 0) {
        (void)playlist_editor_restore_content_song(screen, &song);
    }
    ncm_song_destroy(&song);
    playlist_editor_update_titles(screen, true);
    return;
}


static int32
playlist_editor_highlight_content_position(
    PlaylistEditorScreen *screen, int32 pos
) {
    NcMenu *menu;

    if (screen == NULL) {
        return -EINVAL;
    }
    menu = nc_song_menu_base(&screen->content);
    if ((pos < 0) || (pos >= nc_menu_item_count(menu))) {
        return -EINVAL;
    }
    nc_menu_highlight_position(menu, pos,
                               nc_window_height(&screen->content_window));
    screen->active_column = PLAYLIST_EDITOR_COLUMN_CONTENT;
    playlist_editor_update_menu_highlights(screen);
    return 0;
}

static int32
playlist_editor_find_song_in_content_range(
    PlaylistEditorScreen *screen, NcmSong *song,
    int32 first, int32 last
) {
    NcMenu *menu;

    if ((screen == NULL) || (song == NULL)) {
        return -EINVAL;
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
            && ncm_song_is_equal(candidate, song)) {
            return i;
        }
    }
    return -ENOENT;
}


static int32
playlist_editor_locate_song_in_playlist_range(
    PlaylistEditorScreen *screen, NcmMpdClient *client,
    NcmSong *song, int32 first, int32 last, NcmError *ncm_error
) {
    NcMenu *menu;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing playlist editor"));
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
        int32 status;

        playlist = nc_menu_active_item_at(menu, i);
        if ((playlist == NULL) || (playlist->path == NULL)
            || (song == NULL)) {
            song_index = ncm_error_set_status(
                ncm_error, -EINVAL, STRLIT("missing playlist"));
        } else {
            NcmMpdSongList songs = {0};

            song_index = ncm_mpd_client_get_playlist_content_no_info(
                client, playlist->path, &songs, ncm_error);
            if (song_index >= 0) {
                song_index = -ENOENT;
                for (int32 j = 0; j < songs.count; j += 1) {
                    if (ncm_song_is_equal(&songs.items[j], song)) {
                        song_index = j;
                        break;
                    }
                }
                if (song_index == -ENOENT) {
                    ncm_error_clear(ncm_error);
                }
            }
            ncm_mpd_song_list_destroy(&songs);
        }
        if (song_index < 0) {
            if (song_index == -ENOENT) {
                continue;
            }
            return song_index;
        }
        nc_menu_highlight_position(menu, i,
                                   nc_window_height(
                                       &screen->playlists_window));
        screen->active_column = PLAYLIST_EDITOR_COLUMN_PLAYLISTS;
        playlist_editor_update_menu_highlights(screen);
        playlist_editor_clear_stale_content(screen);
        status = playlist_editor_screen_reload_content_from_mpd(
            screen, client, ncm_error);
        if (status < 0) {
            return status;
        }
        status = playlist_editor_highlight_content_position(screen,
                                                            song_index);
        if (status < 0) {
            return ncm_error_set_status(
                ncm_error, status, STRLIT("song is not in playlist view"));
        }
        return 1;
    }
    ncm_error_clear(ncm_error);
    return 0;
}

static int32
playlist_editor_show_screen(PlaylistEditorScreen *screen) {
    int32 status;

    ASSERT(screen != NULL);
    if (!app_controller_is_screen_registered(&screen->screen)) {
        if ((status = app_controller_register_screen(&screen->screen)) < 0) {
            return status;
        }
        screen->registered = true;
    }
    return nc_screen_switcher_switch_to(
        &screen->screen, nc_screen_has_to_be_resized(&screen->screen));
}

static int32
playlist_editor_store_current_playlist_path(PlaylistEditorScreen *screen,
                                            StrBuilder *buffer) {
    char *path;
    int32 path_len;
    int32 status;

    ASSERT(buffer != NULL);

    sb_clear(buffer);
    if (!playlist_editor_has_current_playlist_path(screen, &path, &path_len)) {
        return 0;
    }
    if ((status = sb_set(buffer, path, path_len)) < 0) {
        return status;
    }
    return 1;
}

static int32
playlist_editor_restore_playlist_path(PlaylistEditorScreen *screen,
                                      StrBuilder *buffer) {
    NcMenu *menu;

    ASSERT(buffer != NULL);

    if ((screen == NULL) || (buffer->len <= 0)) {
        return 0;
    }
    menu = nc_playlist_entry_menu_base(&screen->playlists);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmPlaylist *playlist;

        playlist = nc_menu_active_item_at(menu, i);
        if (playlist
            && STREQUAL(playlist->path, playlist->path_len,
                        buffer->data, buffer->len)) {
            nc_menu_highlight_position(menu, i, screen->main_height);
            return 1;
        }
    }
    return 0;
}

static int32
playlist_editor_store_current_song(PlaylistEditorScreen *screen,
                                   NcmSong *song) {
    NcmSong *current;
    int32 status;

    ASSERT(song != NULL);

    if (screen == NULL) {
        return 0;
    }
    if ((current = nc_song_menu_current(&screen->content)) == NULL) {
        return 0;
    }
    if ((status = ncm_song_copy(song, current)) < 0) {
        return status;
    }
    return 1;
}

static int32
playlist_editor_restore_content_song(PlaylistEditorScreen *screen,
                                     NcmSong *song) {
    NcMenu *menu;

    ASSERT(song != NULL);

    if (screen == NULL) {
        return 0;
    }
    menu = nc_song_menu_base(&screen->content);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmSong *item;

        if ((item = nc_menu_active_item_at(menu, i))
            && ncm_song_is_equal(item, song)) {
            nc_menu_highlight_position(menu, i, screen->main_height);
            return 1;
        }
    }
    return 0;
}


static void
playlist_editor_report_error(char *context, int32 context_len,
                             NcmError *ncm_error) {
    StrBuilder message = {0};

    ASSERT(ncm_error != NULL);

    SB_APPEND(&message, context, context_len);
    if (ncm_error->message[0] != 0) {
        SB_APPEND(&message, ": ");
        SB_APPEND(&message, ncm_error->message, strlen32(ncm_error->message));
    }
    ncm_statusbar_print_cstring(Config.message_delay_time, message.data);
    sb_free(&message);
    return;
}


static void
playlist_editor_observe_current_playlist(PlaylistEditorScreen *screen) {
    char *path;
    int32 path_len;
    NcMenu *menu;

    if (screen == NULL) {
        return;
    }

    menu = nc_playlist_entry_menu_base(&screen->playlists);
    screen->last_playlist_highlight = nc_menu_highlight(menu);

    if (!playlist_editor_has_current_playlist_path(screen, &path, &path_len)) {
        sb_clear(&screen->observed_playlist_path);
        screen->observed_playlist_valid = false;
        return;
    }

    sb_set(&screen->observed_playlist_path, path, path_len);
    screen->observed_playlist_valid = true;
    return;
}

static bool
playlist_editor_displayed_playlist_is_current(
    PlaylistEditorScreen *screen
) {
    char *path;
    int32 path_len;

    if ((screen == NULL) || !screen->displayed_playlist_valid) {
        return false;
    }
    if (!playlist_editor_has_current_playlist_path(screen, &path, &path_len)) {
        return false;
    }
    return STREQUAL(screen->displayed_playlist_path.data,
                    screen->displayed_playlist_path.len,
                    path, path_len);
}


static void
playlist_editor_clear_stale_content(PlaylistEditorScreen *screen) {
    ASSERT(screen != NULL);
    nc_menu_clear_items(nc_song_menu_base(&screen->content));
    sb_clear(&screen->displayed_playlist_path);
    screen->displayed_playlist_valid = false;
    screen->content_update_requested = true;
    screen->last_known_content_count = -1;
    playlist_editor_reset_content_timer(screen);
    playlist_editor_update_titles(screen, true);
    return;
}

static void
playlist_editor_finish_playlist_change(PlaylistEditorScreen *screen) {
    char *path;
    int32 path_len;
    NcMenu *menu;
    bool changed;

    ASSERT(screen != NULL);
    if (screen->active_column != PLAYLIST_EDITOR_COLUMN_PLAYLISTS) {
        return;
    }

    menu = nc_playlist_entry_menu_base(&screen->playlists);
    if (!playlist_editor_has_current_playlist_path(screen, &path, &path_len)) {
        changed = screen->observed_playlist_valid;
        playlist_editor_observe_current_playlist(screen);
    } else {
        changed = !screen->observed_playlist_valid
                  || !STREQUAL(screen->observed_playlist_path.data,
                               screen->observed_playlist_path.len,
                               path, path_len)
                  || (screen->last_playlist_highlight
                      != nc_menu_highlight(menu));
        if (changed) {
            playlist_editor_observe_current_playlist(screen);
        }
    }
    if (changed) {
        playlist_editor_clear_stale_content(screen);
    }
    return;
}

static void
playlist_editor_mouse_scroll(PlaylistEditorScreen *screen,
                             enum NcScroll where) {
    enum NcScroll effective;
    NcMenu *menu;
    int32 count;

    if (screen == NULL) {
        return;
    }
    if ((menu = playlist_editor_screen_active_menu(screen)) == NULL) {
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


static void
playlist_editor_refresh_window(PlaylistEditorScreen *screen,
                               NcWindow *window, NcMenu *menu) {
    ASSERT(screen != NULL);
    ASSERT(window != NULL);
    ASSERT(menu != NULL);
    nc_menu_prepare_refresh(menu, nc_window_height(window), NULL, NULL);
    nc_window_display(window);
    nc_menu_refresh(menu, window, nc_window_width(window),
                    nc_window_height(window));
    return;
}


static int32
append_content_item(PlaylistEditorScreen *screen, int32 pos,
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

static int32
append_content_item_from_source(PlaylistEditorScreen *screen,
                                enum NcMenuItemSource source, int32 pos,
                                NcmSongArray *songs) {
    NcmSong *song;
    int32 status;

    if ((screen == NULL) || (songs == NULL)) {
        return -EINVAL;
    }
    song = nc_menu_item_at(nc_song_menu_base(&screen->content),
                           source, pos);
    if (song == NULL) {
        return -ENOENT;
    }
    status = ncm_song_array_append_copy(songs, song);
    if (status < 0) {
        return status;
    }
    return 0;
}


static bool
playlist_editor_search_position(NcMenu *menu, int32 pos,
                                void *user) {
    PlaylistEditorSearchContext *context;
    void *item;

    context = user;
    if ((item = nc_menu_active_item_at(menu, pos)) == NULL) {
        return false;
    }
    if (menu->item_callbacks.item_size == SIZEOF(NcmPlaylist)) {
        NcmPlaylist *playlist;

        playlist = item;
        return playlist_editor_playlist_matches_regex(context->regex,
                                                      playlist);
    }
    return playlist_editor_content_matches_regex(
        context->screen, context->regex, item);
}


#endif /* NCMPCPP_NC_PLAYLIST_EDITOR_C */
