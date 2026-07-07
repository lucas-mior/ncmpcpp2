#include "screens/nc_playlist_editor.h"

#include <errno.h>

#include "app_controller.h"
#include "c/ncm_base.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "screens/screen_switcher.h"

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
static bool song_matches_regex(NcmRegex *regex, NcmSong *song);
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
static void playlist_editor_set_filter_callbacks(
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

    callbacks = playlist_editor_callbacks();
    nc_playlist_entry_menu_init(&screen->playlists);
    nc_song_menu_init(&screen->content);
    nc_window_init(&screen->playlists_window, start_x, main_start_y,
                   width / 2, main_height, STRLIT_ARGS("Playlists"),
                   color, border);
    nc_window_init(&screen->content_window, start_x + width / 2,
                   main_start_y, width - width / 2, main_height,
                   STRLIT_ARGS("Content"), color, border);
    ncm_buffer_init(&screen->playlist_search_constraint);
    ncm_buffer_init(&screen->content_search_constraint);
    ncm_regex_init(&screen->playlist_filter_regex);
    ncm_regex_init(&screen->content_filter_regex);
    screen->timer.ns = 0;
    screen->active_column = NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS;
    screen->playlists_update_requested = true;
    screen->content_update_requested = true;
    screen->playlist_filter_enabled = false;
    screen->content_filter_enabled = false;
    screen->registered = false;
    native_playlist_editor_screen_set_geometry(screen, start_x, width,
                                               main_start_y, main_height);
    nc_screen_init(&screen->screen, callbacks, screen,
                   NC_SCREEN_TYPE_PLAYLIST_EDITOR);
    playlist_editor_set_filter_callbacks(screen);
    return;
}

void
native_playlist_editor_screen_destroy(NativePlaylistEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    (void)app_controller_unregister_screen(
        native_playlist_editor_screen_base(screen));
    nc_playlist_entry_menu_destroy(&screen->playlists);
    nc_song_menu_destroy(&screen->content);
    nc_window_destroy(&screen->playlists_window);
    nc_window_destroy(&screen->content_window);
    ncm_buffer_destroy(&screen->playlist_search_constraint);
    ncm_buffer_destroy(&screen->content_search_constraint);
    ncm_regex_destroy(&screen->playlist_filter_regex);
    ncm_regex_destroy(&screen->content_filter_regex);
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
    native_playlist_editor_screen_set_column_ratio(screen, 1, 1);
    return;
}

void
native_playlist_editor_screen_set_column_ratio(
    NativePlaylistEditorScreen *screen, int64 left, int64 right) {
    int64 total;

    if (screen == NULL) {
        return;
    }
    if (left < 1) {
        left = 1;
    }
    if (right < 1) {
        right = 1;
    }
    total = left + right;
    screen->left_width = screen->width*left / total;
    if (screen->left_width >= screen->width) {
        screen->left_width = screen->width - 1;
    }
    if (screen->left_width < 1) {
        screen->left_width = 1;
    }
    screen->right_start_x = screen->start_x + screen->left_width;
    screen->right_width = screen->width - screen->left_width;
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
    }
    return;
}

void
native_playlist_editor_screen_next_column(
    NativePlaylistEditorScreen *screen) {
    if (native_playlist_editor_screen_next_column_available(screen)) {
        screen->active_column = NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT;
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
    screen->playlists_update_requested = true;
    screen->content_update_requested = true;
    return;
}

bool
native_playlist_editor_screen_load_playlists(
    NativePlaylistEditorScreen *screen, NcmMpdPlaylistList *playlists) {
    NcMenu *menu;

    if (screen == NULL || playlists == NULL) {
        return false;
    }
    menu = nc_playlist_entry_menu_base(&screen->playlists);
    nc_menu_clear_items(menu);
    for (int32 i = 0; i < playlists->count; i += 1) {
        nc_playlist_entry_menu_add(&screen->playlists,
                                   &playlists->items[i]);
    }
    if (screen->playlist_filter_enabled) {
        nc_menu_apply_filter(menu);
    }
    screen->playlists_update_requested = false;
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
    ok = ncm_mpd_client_get_playlists(client, &playlists, error)
         && native_playlist_editor_screen_load_playlists(screen,
                                                         &playlists);
    ncm_mpd_playlist_list_destroy(&playlists);
    return ok;
}

bool
native_playlist_editor_screen_load_content(
    NativePlaylistEditorScreen *screen, NcmMpdSongList *songs) {
    NcMenu *menu;

    if (screen == NULL || songs == NULL) {
        return false;
    }
    menu = nc_song_menu_base(&screen->content);
    nc_menu_clear_items(menu);
    for (int32 i = 0; i < songs->count; i += 1) {
        nc_song_menu_add(&screen->content, &songs->items[i]);
    }
    if (screen->content_filter_enabled) {
        nc_menu_apply_filter(menu);
    }
    screen->content_update_requested = false;
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
        constraint = &screen->content_search_constraint;
        enabled = &screen->content_filter_enabled;
    } else {
        regex = &screen->playlist_filter_regex;
        constraint = &screen->playlist_search_constraint;
        enabled = &screen->playlist_filter_enabled;
    }

    if (pattern == NULL || pattern_len <= 0) {
        *enabled = false;
        ncm_buffer_clear(constraint);
        nc_menu_show_all_items(menu);
        return true;
    }
    if (!ncm_regex_compile(regex, pattern, pattern_len, regex_flags,
                           error)) {
        return false;
    }
    ncm_buffer_set(constraint, pattern, pattern_len);
    *enabled = true;
    nc_menu_apply_filter(menu);
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
        ncm_buffer_clear(&screen->content_search_constraint);
    } else {
        screen->playlist_filter_enabled = false;
        ncm_buffer_clear(&screen->playlist_search_constraint);
    }
    nc_menu_show_all_items(menu);
    return;
}

bool
native_playlist_editor_screen_search_active(
    NativePlaylistEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, bool forward, bool wrap, bool skip_current,
    NcmError *error) {
    NcmRegex regex;
    NcMenu *menu;
    bool found;

    if (screen == NULL) {
        return false;
    }
    ncm_regex_init(&regex);
    if (!ncm_regex_compile(&regex, pattern, pattern_len, regex_flags,
                           error)) {
        ncm_regex_destroy(&regex);
        return false;
    }
    menu = native_playlist_editor_screen_active_menu(screen);
    found = playlist_editor_search_menu(menu, &regex, forward, wrap,
                                        skip_current);
    ncm_regex_destroy(&regex);
    return found;
}

void
native_playlist_editor_screen_request_playlists_update(
    NativePlaylistEditorScreen *screen) {
    if (screen != NULL) {
        screen->playlists_update_requested = true;
    }
    return;
}

void
native_playlist_editor_screen_request_content_update(
    NativePlaylistEditorScreen *screen) {
    if (screen != NULL) {
        screen->content_update_requested = true;
    }
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
    return native_playlist_editor_screen_active_window(
        playlist_editor_from_screen(screen));
}

static void
playlist_editor_refresh_callback(NcScreen *screen) {
    NativePlaylistEditorScreen *editor;

    editor = playlist_editor_from_screen(screen);
    playlist_editor_refresh_window(editor, &editor->playlists_window,
                                   nc_playlist_entry_menu_base(
                                       &editor->playlists));
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
playlist_editor_switch_to_callback(NcScreen *screen) {
    (void)nc_screen_switcher_switch_to(screen,
                                       nc_screen_has_to_be_resized(screen));
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
    (void)screen;
    return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
}

static char *
playlist_editor_title_callback(NcScreen *screen) {
    (void)screen;
    return (char *)"Playlist editor";
}

static void
playlist_editor_update_callback(NcScreen *screen) {
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
        (void)nc_menu_goto_selectable(
            nc_playlist_entry_menu_base(&editor->playlists), y);
        return;
    }
    window = &editor->content_window;
    x = event.x;
    y = event.y;
    if (nc_window_has_coords(window, &x, &y)) {
        editor->active_column = NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT;
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

    song = nc_song_menu_item_at(&screen->content, NC_MENU_ITEMS_FILTERED,
                                pos);
    if (song == NULL) {
        song = nc_song_menu_item_at(&screen->content, NC_MENU_ITEMS_ALL,
                                    pos);
    }
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
playlist_editor_set_filter_callbacks(NativePlaylistEditorScreen *screen) {
    NcMenuDisplayCallbacks callbacks = {0};

    callbacks.filter = playlist_filter_callback;
    callbacks.user = screen;
    nc_menu_set_display_callbacks(nc_playlist_entry_menu_base(
                                      &screen->playlists), callbacks);

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
