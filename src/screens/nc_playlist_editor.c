#include "screens/nc_playlist_editor.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "app_controller.h"
#include "global.h"
#include "settings.h"
#include "c/ncm_base.h"
#include "c/ncm_comparators.h"
#include "c/ncm_charset.h"
#include "c/ncm_display.h"
#include "c/ncm_utf8.h"
#include "c/ncm_string.h"
#include "statusbar.h"
#include "cbase/base_macros.h"

static NativePlaylistEditorScreen *playlist_editor_from_screen(
    NcScreen *screen);
static NcScreenCallbacks playlist_editor_callbacks(void);
static NcWindow *playlist_editor_active_window_callback(NcScreen *screen);
static void playlist_editor_refresh_callback(NcScreen *screen);
static void playlist_editor_refresh_window_callback(NcScreen *screen);
static void playlist_editor_scroll_callback(NcScreen *screen,
                                            enum NcScroll where);
static void playlist_editor_switch_to_callback(NcScreen *screen);
static void playlist_editor_resize_callback(NcScreen *screen);
static int32 playlist_editor_timeout_callback(NcScreen *screen);
static char *playlist_editor_title_callback(NcScreen *screen);
static void playlist_editor_update_callback(NcScreen *screen);
static void playlist_editor_mouse_callback(NcScreen *screen, MEVENT event);
static bool playlist_editor_lockable_callback(NcScreen *screen);
static bool playlist_editor_mergable_callback(NcScreen *screen);
static void playlist_editor_destroy_callback(NcScreen *screen);
static bool playlist_filter_callback(NcMenu *menu, void *item, void *user);
static bool content_filter_callback(NcMenu *menu, void *item, void *user);
static void playlist_draw_callback(NcMenu *menu, NcWindow *window,
                                   void *item, int64 pos, void *user);
static void content_draw_callback(NcMenu *menu, NcWindow *window,
                                  void *item, int64 pos, void *user);
static void playlist_editor_print_buffer(NcWindow *window,
                                         NcBuffer *buffer);
static int32 playlist_editor_content_list_width(NcMenu *menu,
                                                NcWindow *window,
                                                int64 pos);
static bool song_matches_regex(NcmRegex *regex, NcmSong *song);
static void playlist_editor_initialize_buffers(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_destroy_buffers(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_initialize_regexes(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_apply_geometry(
    NativePlaylistEditorScreen *screen);
static int64 playlist_editor_separator_width(int64 width);
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
static void playlist_editor_append_int64(NcmBuffer *buffer, int64 value);
static void playlist_editor_reset_content_timer(
    NativePlaylistEditorScreen *screen);
static bool playlist_editor_current_playlist_path(
    NativePlaylistEditorScreen *screen, char **path, int32 *path_len);
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
static bool playlist_editor_content_fetch_due(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_report_error(char *context, int32 context_len,
                                         NcmError *error);
static void playlist_editor_update_from_mpd(
    NativePlaylistEditorScreen *screen, NcmMpdClient *client);
static void playlist_editor_observe_current_playlist(
    NativePlaylistEditorScreen *screen);
static bool playlist_editor_displayed_playlist_is_current(
    NativePlaylistEditorScreen *screen);
static bool playlist_editor_playlist_row_changed(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_clear_stale_content(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_finish_playlist_change(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_set_displayed_playlist(
    NativePlaylistEditorScreen *screen);
static void playlist_editor_refresh_window(NativePlaylistEditorScreen *screen,
                                           NcWindow *window, NcMenu *menu);
static bool append_current_content(NativePlaylistEditorScreen *screen,
                                   NcmSongArray *songs);
static bool append_selected_content(NativePlaylistEditorScreen *screen,
                                    NcmSongArray *songs);
static bool append_content_item(NativePlaylistEditorScreen *screen,
                                int64 pos, NcmSongArray *songs);
static bool playlist_editor_search_menu(NcMenu *menu, NcmRegex *regex,
                                        bool forward, bool wrap,
                                        bool skip_current);
static bool playlist_editor_search_item(NcMenu *menu, NcmRegex *regex,
                                        int64 pos);
static void playlist_editor_set_display_callbacks(
    NativePlaylistEditorScreen *screen);
static bool command_set_string(char **dest, int32 *dest_len,
                               int32 *dest_cap, char *source,
                               int32 source_len);

void
native_playlist_editor_command_init(
    NativePlaylistEditorCommand *command) {
    command->type = NATIVE_PLAYLIST_EDITOR_COMMAND_NONE;
    command->playlist = NULL;
    command->target = NULL;
    command->playlist_len = 0;
    command->playlist_cap = 0;
    command->target_len = 0;
    command->target_cap = 0;
    return;
}

void
native_playlist_editor_command_destroy(
    NativePlaylistEditorCommand *command) {
    if (command == NULL) {
        return;
    }
    if (command->playlist != NULL) {
        ncm_free(command->playlist, command->playlist_cap);
    }
    if (command->target != NULL) {
        ncm_free(command->target, command->target_cap);
    }
    native_playlist_editor_command_init(command);
    return;
}

bool
native_playlist_editor_command_set(
    NativePlaylistEditorCommand *command,
    enum NativePlaylistEditorCommandType type, char *playlist,
    int32 playlist_len, char *target, int32 target_len) {
    if (command == NULL) {
        return false;
    }

    native_playlist_editor_command_destroy(command);
    command->type = type;
    if (!command_set_string(&command->playlist, &command->playlist_len,
                            &command->playlist_cap, playlist,
                            playlist_len)) {
        native_playlist_editor_command_destroy(command);
        return false;
    }
    if (!command_set_string(&command->target, &command->target_len,
                            &command->target_cap, target, target_len)) {
        native_playlist_editor_command_destroy(command);
        return false;
    }
    return true;
}

void
native_playlist_editor_screen_init(NativePlaylistEditorScreen *screen,
                                   int64 start_x, int64 width,
                                   int64 main_start_y, int64 main_height,
                                   NcColor color, NcBorder border) {
    NcScreenCallbacks callbacks;
    int64 initial_left_width;
    int64 initial_right_width;

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
    screen->bridge = (NativePlaylistEditorBridge){0};
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
    NativePlaylistEditorScreen *screen) {
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
    NativePlaylistEditorScreen *screen) {
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
    NativePlaylistEditorScreen *screen, int64 start_x, int64 width,
    int64 main_start_y, int64 main_height) {
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
    NativePlaylistEditorScreen *screen, int64 left, int64 right) {
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
    NativePlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT;
}

bool
native_playlist_editor_screen_next_column_available(
    NativePlaylistEditorScreen *screen) {
    NcMenu *content;

    if (screen == NULL) {
        return false;
    }
    content = nc_song_menu_base(&screen->content);
    return (screen->active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS)
           && !nc_menu_empty(content);
}

void
native_playlist_editor_screen_previous_column(
    NativePlaylistEditorScreen *screen) {
    if (native_playlist_editor_screen_previous_column_available(screen)) {
        screen->active_column = NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS;
        playlist_editor_update_menu_highlights(screen);
    }
    return;
}

void
native_playlist_editor_screen_next_column(
    NativePlaylistEditorScreen *screen) {
    if (native_playlist_editor_screen_next_column_available(screen)) {
        screen->active_column = NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT;
        playlist_editor_update_menu_highlights(screen);
    }
    return;
}

void
native_playlist_editor_screen_clear(NativePlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_clear_items(nc_playlist_entry_menu_base(&screen->playlists));
    nc_menu_clear_items(nc_song_menu_base(&screen->content));
    ncm_buffer_clear(&screen->displayed_playlist_path);
    ncm_buffer_clear(&screen->observed_playlist_path);
    screen->displayed_playlist_valid = false;
    screen->observed_playlist_valid = false;
    screen->last_playlist_highlight = -1;
    screen->last_known_content_count = -1;
    screen->playlists_update_requested = true;
    screen->content_update_requested = true;
    playlist_editor_reset_content_timer(screen);
    playlist_editor_update_titles(screen, true);
    return;
}

bool
native_playlist_editor_screen_load_playlists(
    NativePlaylistEditorScreen *screen, NcmMpdPlaylistList *playlists) {
    NcmBuffer preserved;
    NcMenu *menu;
    bool had_preserved;

    if (screen == NULL || playlists == NULL) {
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
    NcmError *error) {
    NcmMpdPlaylistList playlists;
    bool ok;

    if (screen == NULL) {
        return false;
    }
    ncm_mpd_playlist_list_init(&playlists);
    ok = ncm_mpd_client_get_playlists(client, &playlists, error);
    if (ok) {
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
    NativePlaylistEditorScreen *screen, NcmMpdSongList *songs) {
    NcMenu *menu;
    NcmSong preserved_song;
    bool had_preserved_song;

    if (screen == NULL || songs == NULL) {
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
    NcmError *error) {
    NcmMpdSongList songs;
    NcmPlaylist *playlist;
    bool ok;

    if (screen == NULL) {
        return false;
    }
    playlist = nc_playlist_entry_menu_current(&screen->playlists);
    if (playlist == NULL || playlist->path == NULL) {
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
native_playlist_editor_screen_current_playlist(
    NativePlaylistEditorScreen *screen, NcmPlaylist *playlist) {
    NcmPlaylist *current;

    if (screen == NULL || playlist == NULL) {
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
    NativePlaylistEditorScreen *screen, NcmSong *song) {
    NcmSong *current;

    if (screen == NULL || song == NULL) {
        return false;
    }
    current = nc_song_menu_current(&screen->content);
    if (current == NULL) {
        return false;
    }
    return ncm_song_copy(song, current);
}

bool
native_playlist_editor_screen_selected_songs(
    NativePlaylistEditorScreen *screen, NcmSongArray *songs) {
    if (screen == NULL || songs == NULL) {
        return false;
    }
    if (screen->active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT) {
        return append_selected_content(screen, songs);
    }
    return append_current_content(screen, songs);
}

bool
native_playlist_editor_screen_apply_active_filter(
    NativePlaylistEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, NcmError *error) {
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

    if (pattern == NULL || pattern_len <= 0) {
        *enabled = false;
        ncm_buffer_clear(constraint);
        nc_menu_show_all_items(menu);
        playlist_editor_update_titles(screen, true);
        return true;
    }
    if (!ncm_regex_compile(regex, pattern, pattern_len, regex_flags,
                           error)) {
        return false;
    }
    ncm_buffer_set(constraint, pattern, pattern_len);
    *enabled = true;
    nc_menu_apply_filter(menu);
    playlist_editor_update_titles(screen, true);
    return true;
}

void
native_playlist_editor_screen_clear_active_filter(
    NativePlaylistEditorScreen *screen) {
    NcMenu *menu;

    if (screen == NULL) {
        return;
    }
    menu = native_playlist_editor_screen_active_menu(screen);
    if (screen->active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT) {
        screen->content_filter_enabled = false;
        ncm_buffer_clear(&screen->content_filter_constraint);
    } else {
        screen->playlist_filter_enabled = false;
        ncm_buffer_clear(&screen->playlist_filter_constraint);
    }
    nc_menu_show_all_items(menu);
    playlist_editor_update_titles(screen, true);
    return;
}

bool
native_playlist_editor_screen_search_active(
    NativePlaylistEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, bool forward, bool wrap, bool skip_current,
    NcmError *error) {
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
    if (pattern == NULL || pattern_len <= 0) {
        *enabled = false;
        ncm_buffer_clear(constraint);
        return false;
    }
    if (!ncm_regex_compile(regex, pattern, pattern_len, regex_flags,
                           error)) {
        return false;
    }
    ncm_buffer_set(constraint, pattern, pattern_len);
    *enabled = true;
    menu = native_playlist_editor_screen_active_menu(screen);
    return playlist_editor_search_menu(menu, regex, forward, wrap,
                                       skip_current);
}

void
native_playlist_editor_screen_request_playlists_update(
    NativePlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->playlists_update_requested = true;
    nc_screen_request_update(&screen->screen);
    return;
}

void
native_playlist_editor_screen_request_content_update(
    NativePlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->content_update_requested = true;
    playlist_editor_reset_content_timer(screen);
    nc_screen_request_update(&screen->screen);
    return;
}

bool
native_playlist_editor_screen_prepare_playlist_command(
    NativePlaylistEditorScreen *screen,
    enum NativePlaylistEditorCommandType type, char *target,
    int32 target_len, NativePlaylistEditorCommand *command) {
    NcmPlaylist *playlist;

    if (screen == NULL || command == NULL) {
        return false;
    }
    playlist = nc_playlist_entry_menu_current(&screen->playlists);
    if (playlist == NULL || playlist->path == NULL) {
        return false;
    }
    return native_playlist_editor_command_set(command, type,
                                              playlist->path,
                                              playlist->path_len,
                                              target, target_len);
}


bool
native_playlist_editor_command_execute(
    NativePlaylistEditorCommand *command, NcmMpdClient *client,
    NcmError *error) {
    if (command == NULL) {
        return false;
    }
    switch (command->type) {
    case NATIVE_PLAYLIST_EDITOR_COMMAND_SAVE:
        return ncm_mpd_client_save_playlist(client, command->playlist,
                                            error);
    case NATIVE_PLAYLIST_EDITOR_COMMAND_RENAME:
        return ncm_mpd_client_rename_playlist(client, command->playlist,
                                              command->target, error);
    case NATIVE_PLAYLIST_EDITOR_COMMAND_DELETE:
        return ncm_mpd_client_delete_playlist(client, command->playlist,
                                              error);
    case NATIVE_PLAYLIST_EDITOR_COMMAND_LOAD: {
        bool loaded;

        loaded = false;
        return ncm_mpd_client_load_playlist(client, command->playlist,
                                            &loaded, error);
    }
    default:
        return false;
    }
}

bool
native_playlist_editor_screen_load_current_playlist(
    NativePlaylistEditorScreen *screen, NcmMpdClient *client,
    bool *loaded, NcmError *error) {
    NcmPlaylist *playlist;

    if (screen == NULL) {
        return false;
    }
    playlist = nc_playlist_entry_menu_current(&screen->playlists);
    if (playlist == NULL || playlist->path == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing playlist"));
        return false;
    }
    return ncm_mpd_client_load_playlist(client, playlist->path, loaded,
                                        error);
}

void
native_playlist_editor_screen_set_bridge(
    NativePlaylistEditorScreen *screen, NativePlaylistEditorBridge bridge) {
    if (screen == NULL) {
        return;
    }
    screen->bridge = bridge;
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
    if (editor->bridge.resize != NULL) {
        editor->bridge.resize(editor->bridge.user);
    }
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
    NativePlaylistEditorScreen *editor;

    editor = playlist_editor_from_screen(screen);
    if (editor->bridge.title != NULL) {
        return editor->bridge.title(editor->bridge.user);
    }
    return (char *)"Playlist editor";
}

static void
playlist_editor_update_callback(NcScreen *screen) {
    NativePlaylistEditorScreen *editor;

    editor = playlist_editor_from_screen(screen);
    playlist_editor_finish_playlist_change(editor);
    playlist_editor_update_from_mpd(editor, &global_mpd);
    nc_screen_clear_update_request(screen);
    return;
}

static void
playlist_editor_mouse_callback(NcScreen *screen, MEVENT event) {
    NativePlaylistEditorScreen *editor;
    NcWindow *window;
    int32 x;
    int32 y;

    editor = playlist_editor_from_screen(screen);
    window = &editor->playlists_window;
    x = event.x;
    y = event.y;
    if (nc_window_has_coords(window, &x, &y)) {
        editor->active_column = NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS;
        playlist_editor_update_menu_highlights(editor);
        (void)nc_menu_goto_selectable(
            nc_playlist_entry_menu_base(&editor->playlists), y);
        playlist_editor_finish_playlist_change(editor);
        return;
    }
    window = &editor->content_window;
    x = event.x;
    y = event.y;
    if (nc_window_has_coords(window, &x, &y)) {
        editor->active_column = NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT;
        playlist_editor_update_menu_highlights(editor);
        (void)nc_menu_goto_selectable(nc_song_menu_base(&editor->content),
                                      y);
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
    if (playlist == NULL || playlist->path == NULL) {
        return false;
    }
    return ncm_regex_search(&editor->playlist_filter_regex,
                            playlist->path, playlist->path_len);
}

static bool
content_filter_callback(NcMenu *menu, void *item, void *user) {
    NativePlaylistEditorScreen *editor;

    (void)menu;
    editor = user;
    if (!editor->content_filter_enabled) {
        return true;
    }
    return song_matches_regex(&editor->content_filter_regex, item);
}

static void
playlist_draw_callback(NcMenu *menu, NcWindow *window, void *item,
                       int64 pos, void *user) {
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
                      int64 pos, void *user) {
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
                                   int64 pos) {
    int64 available_width;

    available_width = nc_window_width(window) - nc_window_get_x(window);
    if (nc_menu_position_is_selected(menu, pos)) {
        available_width -= ncm_utf8_width(
            menu->selected_suffix.data, menu->selected_suffix.len);
    }
    if (menu->highlight_enabled && (pos == menu->highlight)) {
        available_width -= ncm_utf8_width(
            menu->highlight_suffix.data, menu->highlight_suffix.len);
    }
    if (available_width < 0) {
        available_width = 0;
    }
    if (available_width > INT32_MAX) {
        return INT32_MAX;
    }
    return (int32)available_width;
}

static bool
song_matches_regex(NcmRegex *regex, NcmSong *song) {
    NcmStringView view;

    if (song == NULL) {
        return false;
    }
    if (ncm_song_uri_view(song, 0, &view)
        && ncm_regex_search(regex, view.data, view.len)) {
        return true;
    }
    for (int32 i = 0; i < song->tags_len; i += 1) {
        if (ncm_regex_search(regex, song->tags[i].value,
                             song->tags[i].value_len)) {
            return true;
        }
    }
    return false;
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
    int64 total;
    int64 separator_width;
    int64 left_width;

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

static int64
playlist_editor_separator_width(int64 width) {
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
    NativePlaylistEditorScreen *screen) {
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
playlist_editor_append_int64(NcmBuffer *buffer, int64 value) {
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
    if (playlist == NULL || playlist->path == NULL) {
        return false;
    }
    *path = playlist->path;
    *path_len = playlist->path_len;
    return true;
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
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmPlaylist *playlist;

        playlist = nc_menu_active_item_at(menu, i);
        if ((playlist != NULL)
            && ncm_string_equal(playlist->path, playlist->path_len,
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
    current = nc_song_menu_current(&screen->content);
    if (current == NULL) {
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
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmSong *item;

        item = nc_menu_active_item_at(menu, i);
        if ((item != NULL) && ncm_song_equal(item, song)) {
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
    if ((error != NULL) && (error->message[0] != 0)) {
        ncm_buffer_append(&message, STRLIT_ARGS(": "));
        ncm_buffer_append(&message, error->message,
                          (int32)strlen(error->message));
    }
    ncm_buffer_append_byte(&message, '\0');
    ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                message.data);
    ncm_buffer_destroy(&message);
    return;
}

static void
playlist_editor_update_from_mpd(NativePlaylistEditorScreen *screen,
                                NcmMpdClient *client) {
    NcmError error;
    bool ok;

    if (screen == NULL) {
        return;
    }

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
            return;
        }
    }

    playlist_editor_finish_playlist_change(screen);
    if (!playlist_editor_content_fetch_due(screen)) {
        playlist_editor_update_titles(screen, true);
        return;
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
        return;
    }
    playlist_editor_update_titles(screen, true);
    return;
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
    NativePlaylistEditorScreen *screen) {
    char *path;
    int32 path_len;

    if ((screen == NULL) || !screen->displayed_playlist_valid) {
        return false;
    }
    if (!playlist_editor_current_playlist_path(screen, &path, &path_len)) {
        return false;
    }
    return ncm_string_equal(screen->displayed_playlist_path.data,
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
              || !ncm_string_equal(screen->observed_playlist_path.data,
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
    if (screen == NULL || window == NULL || menu == NULL) {
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
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (!append_content_item(screen, i, songs)) {
            return false;
        }
    }
    return true;
}

static bool
append_selected_content(NativePlaylistEditorScreen *screen,
                        NcmSongArray *songs) {
    NcMenu *menu;
    bool any_selected;

    menu = nc_song_menu_base(&screen->content);
    any_selected = nc_menu_has_selected(menu);
    if (!any_selected) {
        return append_content_item(screen, nc_menu_highlight(menu), songs);
    }
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
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
append_content_item(NativePlaylistEditorScreen *screen, int64 pos,
                    NcmSongArray *songs) {
    NcmSong *song;

    song = nc_menu_active_item_at(nc_song_menu_base(&screen->content),
                                  pos);
    if (song == NULL) {
        return false;
    }
    return ncm_song_array_append_copy(songs, song);
}

static bool
playlist_editor_search_menu(NcMenu *menu, NcmRegex *regex, bool forward,
                            bool wrap, bool skip_current) {
    int64 count;
    int64 start;

    if (menu == NULL) {
        return false;
    }
    count = nc_menu_item_count(menu);
    if (count <= 0) {
        return false;
    }
    start = nc_menu_highlight(menu);
    if (skip_current) {
        if (forward) {
            start += 1;
        } else {
            start -= 1;
        }
    }

    for (int64 checked = 0; checked < count; checked += 1) {
        int64 pos;

        if (forward) {
            pos = start + checked;
        } else {
            pos = start - checked;
        }
        if (wrap) {
            while (pos < 0) {
                pos += count;
            }
            pos %= count;
        } else if (pos < 0 || pos >= count) {
            break;
        }
        if (playlist_editor_search_item(menu, regex, pos)) {
            nc_menu_highlight_position(menu, pos, count);
            return true;
        }
    }
    return false;
}

static bool
playlist_editor_search_item(NcMenu *menu, NcmRegex *regex, int64 pos) {
    void *item;

    item = nc_menu_active_item_at(menu, pos);
    if (item == NULL) {
        return false;
    }
    if (menu->item_callbacks.item_size == SIZEOF(NcmPlaylist)) {
        NcmPlaylist *playlist;

        playlist = item;
        if (playlist->path == NULL) {
            return false;
        }
        return ncm_regex_search(regex, playlist->path,
                                playlist->path_len);
    }
    return song_matches_regex(regex, item);
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

static bool
command_set_string(char **dest, int32 *dest_len, int32 *dest_cap,
                   char *source, int32 source_len) {
    int32 cap;

    *dest = NULL;
    *dest_len = 0;
    *dest_cap = 0;
    if (source == NULL || source_len <= 0) {
        return true;
    }
    cap = source_len + 1;
    *dest = ncm_malloc(cap);
    ncm_memcpy(*dest, source, source_len);
    (*dest)[source_len] = '\0';
    *dest_len = source_len;
    *dest_cap = cap;
    return true;
}
