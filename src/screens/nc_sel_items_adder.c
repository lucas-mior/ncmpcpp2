#include "screens/nc_sel_items_adder.h"

#include "app_controller.h"
#include "c/ncm_base.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "screens/screen_switcher.h"

static NativeSelectedItemsAdderScreen *adder_from_screen(NcScreen *screen);
static NcScreenCallbacks adder_callbacks(void);
static NcWindow *adder_active_window_callback(NcScreen *screen);
static void adder_refresh_callback(NcScreen *screen);
static void adder_scroll_callback(NcScreen *screen, enum NcScroll where);
static void adder_switch_to_callback(NcScreen *screen);
static void adder_resize_callback(NcScreen *screen);
static int32 adder_timeout_callback(NcScreen *screen);
static char *adder_title_callback(NcScreen *screen);
static void adder_update_callback(NcScreen *screen);
static void adder_mouse_callback(NcScreen *screen, MEVENT event);
static bool adder_lockable_callback(NcScreen *screen);
static bool adder_mergable_callback(NcScreen *screen);
static void adder_destroy_callback(NcScreen *screen);
static bool adder_filter_callback(NcMenu *menu, void *item, void *user);
static bool adder_row_matches(NcEditorActionRow *row, NcmRegex *regex);
static bool adder_action_row_set(NcEditorActionRow *row, char *label,
                                 int32 label_len, void (*run)(void *),
                                 void *user);
static bool adder_action_set_playlist(char **dest, int32 *dest_len,
                                      int32 *dest_cap, char *source,
                                      int32 source_len);
static void adder_action_current_playlist(void *user);
static void adder_action_new_playlist(void *user);
static void adder_action_cancel_target(void *user);
static void adder_action_position_end(void *user);
static void adder_action_position_beginning(void *user);
static void adder_action_position_current_song(void *user);
static void adder_action_position_current_album(void *user);
static void adder_action_position_highlighted(void *user);
static void adder_action_position_cancel(void *user);
static void adder_action_existing_playlist(void *user);
static bool adder_add_action_row(NcEditorActionMenu *menu, char *label,
                                 int32 label_len, void (*run)(void *),
                                 void *user);
static void adder_clear_playlist_selector(
    NativeSelectedItemsAdderScreen *screen);

typedef struct ExistingPlaylistAction {
    NativeSelectedItemsAdderScreen *screen;
    char *playlist;
    int32 playlist_len;
    int32 playlist_cap;
} ExistingPlaylistAction;

static void existing_playlist_action_init(ExistingPlaylistAction *action);
static void existing_playlist_action_destroy(void *user);
static ExistingPlaylistAction *existing_playlist_action_create(
    NativeSelectedItemsAdderScreen *screen, char *playlist,
    int32 playlist_len);

void
native_selected_items_adder_action_init(
    NativeSelectedItemsAdderAction *action) {
    action->target = NATIVE_SELECTED_ITEMS_ADDER_TARGET_NONE;
    action->position = NATIVE_SELECTED_ITEMS_ADDER_POSITION_NONE;
    action->playlist = NULL;
    action->playlist_len = 0;
    action->playlist_cap = 0;
    return;
}

void
native_selected_items_adder_action_destroy(
    NativeSelectedItemsAdderAction *action) {
    if (action == NULL) {
        return;
    }
    if (action->playlist != NULL) {
        ncm_free(action->playlist, action->playlist_cap);
    }
    native_selected_items_adder_action_init(action);
    return;
}

bool
native_selected_items_adder_action_set(
    NativeSelectedItemsAdderAction *action,
    enum NativeSelectedItemsAdderTarget target,
    enum NativeSelectedItemsAdderPosition position, char *playlist,
    int32 playlist_len) {
    char *source;
    int32 source_len;
    int32 source_cap;

    if (action == NULL) {
        return false;
    }

    source = playlist;
    source_len = playlist_len;
    source_cap = 0;
    if (playlist == action->playlist && playlist != NULL) {
        if (!adder_action_set_playlist(&source, &source_len, &source_cap,
                                       playlist, playlist_len)) {
            return false;
        }
    }

    native_selected_items_adder_action_destroy(action);
    action->target = target;
    action->position = position;
    if (!adder_action_set_playlist(&action->playlist,
                                   &action->playlist_len,
                                   &action->playlist_cap,
                                   source, source_len)) {
        if (source_cap > 0) {
            ncm_free(source, source_cap);
        }
        native_selected_items_adder_action_destroy(action);
        return false;
    }
    if (source_cap > 0) {
        ncm_free(source, source_cap);
    }
    return true;
}

void
native_selected_items_adder_screen_init(
    NativeSelectedItemsAdderScreen *screen, int64 start_x, int64 start_y,
    int64 width, int64 height, NcColor color, NcBorder border) {
    NcScreenCallbacks callbacks;
    NcMenuDisplayCallbacks display_callbacks = {0};

    callbacks = adder_callbacks();
    nc_editor_action_menu_init(&screen->playlist_selector);
    nc_editor_action_menu_init(&screen->position_selector);
    nc_window_init(&screen->playlist_window, start_x, start_y, width,
                   height, STRLIT_ARGS("Add selected item(s) to..."),
                   color, border);
    nc_window_init(&screen->position_window, start_x, start_y, width,
                   height, STRLIT_ARGS("Where?"), color, border);
    ncm_song_array_init(&screen->selected_songs);
    ncm_regex_init(&screen->search_regex);
    ncm_buffer_init(&screen->search_constraint);
    native_selected_items_adder_action_init(&screen->last_action);
    screen->playlist_width = width;
    screen->playlist_height = height;
    screen->position_width = width;
    screen->position_height = height;
    screen->active_menu = NATIVE_SELECTED_ITEMS_ADDER_MENU_PLAYLISTS;
    screen->local_browser = false;
    screen->search_enabled = false;
    screen->registered = false;
    nc_screen_init(&screen->screen, callbacks, screen,
                   NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER);
    display_callbacks.filter = adder_filter_callback;
    display_callbacks.user = screen;
    nc_menu_set_display_callbacks(nc_editor_action_menu_base(
                                      &screen->playlist_selector),
                                  display_callbacks);
    nc_menu_set_display_callbacks(nc_editor_action_menu_base(
                                      &screen->position_selector),
                                  display_callbacks);
    native_selected_items_adder_screen_populate_position_selector(screen);
    return;
}

void
native_selected_items_adder_screen_destroy(
    NativeSelectedItemsAdderScreen *screen) {
    if (screen == NULL) {
        return;
    }
    (void)app_controller_unregister_screen(
        native_selected_items_adder_screen_base(screen));
    for (int64 i = 0; i < nc_menu_all_item_count(
             nc_editor_action_menu_base(&screen->playlist_selector));
         i += 1) {
        NcEditorActionRow *row;

        row = nc_editor_action_menu_item_at(&screen->playlist_selector,
                                            NC_MENU_ITEMS_ALL, i);
        if (row != NULL && row->run == adder_action_existing_playlist) {
            existing_playlist_action_destroy(row->user);
            row->user = NULL;
        }
    }
    nc_editor_action_menu_destroy(&screen->playlist_selector);
    nc_editor_action_menu_destroy(&screen->position_selector);
    nc_window_destroy(&screen->playlist_window);
    nc_window_destroy(&screen->position_window);
    ncm_song_array_destroy(&screen->selected_songs);
    ncm_regex_destroy(&screen->search_regex);
    ncm_buffer_destroy(&screen->search_constraint);
    native_selected_items_adder_action_destroy(&screen->last_action);
    screen->registered = false;
    return;
}

NcScreen *
native_selected_items_adder_screen_base(
    NativeSelectedItemsAdderScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->screen;
}

NcEditorActionMenu *
native_selected_items_adder_screen_playlist_menu(
    NativeSelectedItemsAdderScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->playlist_selector;
}

NcEditorActionMenu *
native_selected_items_adder_screen_position_menu(
    NativeSelectedItemsAdderScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->position_selector;
}

NcMenu *
native_selected_items_adder_screen_active_menu(
    NativeSelectedItemsAdderScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    if (screen->active_menu == NATIVE_SELECTED_ITEMS_ADDER_MENU_POSITIONS) {
        return nc_editor_action_menu_base(&screen->position_selector);
    }
    return nc_editor_action_menu_base(&screen->playlist_selector);
}

NcWindow *
native_selected_items_adder_screen_active_window(
    NativeSelectedItemsAdderScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    if (screen->active_menu == NATIVE_SELECTED_ITEMS_ADDER_MENU_POSITIONS) {
        return &screen->position_window;
    }
    return &screen->playlist_window;
}

bool
native_selected_items_adder_screen_set_selected_songs(
    NativeSelectedItemsAdderScreen *screen, NcmSongArray *songs) {
    if (screen == NULL) {
        return false;
    }
    return ncm_song_array_copy(&screen->selected_songs, songs);
}

bool
native_selected_items_adder_screen_selected_songs(
    NativeSelectedItemsAdderScreen *screen, NcmSongArray *songs) {
    if (screen == NULL || songs == NULL) {
        return false;
    }
    return ncm_song_array_copy(songs, &screen->selected_songs);
}

void
native_selected_items_adder_screen_populate_playlist_selector(
    NativeSelectedItemsAdderScreen *screen, NcmMpdPlaylistList *playlists,
    bool local_browser) {
    NcEditorActionMenu *menu;

    if (screen == NULL) {
        return;
    }
    menu = &screen->playlist_selector;
    adder_clear_playlist_selector(screen);
    screen->local_browser = local_browser;
    (void)adder_add_action_row(menu, STRLIT_ARGS("Current playlist"),
                               adder_action_current_playlist, screen);
    if (!local_browser) {
        (void)adder_add_action_row(menu, STRLIT_ARGS("New playlist"),
                                   adder_action_new_playlist, screen);
    }
    nc_editor_action_menu_add_separator(menu);
    if (!local_browser && playlists != NULL) {
        for (int32 i = 0; i < playlists->count; i += 1) {
            ExistingPlaylistAction *action;
            NcmPlaylist *playlist;

            playlist = &playlists->items[i];
            action = existing_playlist_action_create(
                screen, playlist->path, playlist->path_len);
            if (action == NULL) {
                continue;
            }
            if (!adder_add_action_row(menu, playlist->path,
                                      playlist->path_len,
                                      adder_action_existing_playlist,
                                      action)) {
                existing_playlist_action_destroy(action);
            }
        }
        nc_editor_action_menu_add_separator(menu);
    }
    (void)adder_add_action_row(menu, STRLIT_ARGS("Cancel"),
                               adder_action_cancel_target, screen);
    screen->active_menu = NATIVE_SELECTED_ITEMS_ADDER_MENU_PLAYLISTS;
    return;
}

void
native_selected_items_adder_screen_populate_position_selector(
    NativeSelectedItemsAdderScreen *screen) {
    NcEditorActionMenu *menu;

    if (screen == NULL) {
        return;
    }
    menu = &screen->position_selector;
    nc_menu_clear_items(nc_editor_action_menu_base(menu));
    (void)adder_add_action_row(menu, STRLIT_ARGS("At the end of playlist"),
                               adder_action_position_end, screen);
    (void)adder_add_action_row(menu,
                               STRLIT_ARGS("At the beginning of playlist"),
                               adder_action_position_beginning, screen);
    (void)adder_add_action_row(menu, STRLIT_ARGS("After current song"),
                               adder_action_position_current_song, screen);
    (void)adder_add_action_row(menu, STRLIT_ARGS("After current album"),
                               adder_action_position_current_album, screen);
    (void)adder_add_action_row(menu, STRLIT_ARGS("After highlighted item"),
                               adder_action_position_highlighted, screen);
    nc_editor_action_menu_add_separator(menu);
    (void)adder_add_action_row(menu, STRLIT_ARGS("Cancel"),
                               adder_action_position_cancel, screen);
    return;
}

bool
native_selected_items_adder_screen_run_current(
    NativeSelectedItemsAdderScreen *screen) {
    NcEditorActionRow *row;

    if (screen == NULL) {
        return false;
    }
    row = nc_menu_current_item(
        native_selected_items_adder_screen_active_menu(screen));
    if (row == NULL || row->run == NULL) {
        return false;
    }
    row->run(row->user);
    return true;
}

void
native_selected_items_adder_screen_choose_current_playlist(
    NativeSelectedItemsAdderScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->active_menu = NATIVE_SELECTED_ITEMS_ADDER_MENU_POSITIONS;
    nc_menu_reset(nc_editor_action_menu_base(&screen->position_selector));
    return;
}

bool
native_selected_items_adder_screen_add_to_existing_playlist(
    NativeSelectedItemsAdderScreen *screen, NcmMpdClient *client,
    char *playlist, NcmError *error) {
    bool ok;

    if (screen == NULL) {
        return false;
    }
    ok = ncm_mpd_client_start_command_list(client, error);
    for (int32 i = 0; ok && i < screen->selected_songs.len; i += 1) {
        ok = ncm_mpd_client_add_song_to_playlist(
            client, playlist, &screen->selected_songs.items[i], error);
    }
    if (ok) {
        ok = ncm_mpd_client_commit_command_list(client, error);
    }
    return ok;
}

bool
native_selected_items_adder_screen_apply_search(
    NativeSelectedItemsAdderScreen *screen, char *pattern,
    int32 pattern_len, uint32 regex_flags, NcmError *error) {
    if (screen == NULL) {
        return false;
    }
    if (pattern == NULL || pattern_len <= 0) {
        native_selected_items_adder_screen_clear_search(screen);
        return true;
    }
    if (!ncm_regex_compile(&screen->search_regex, pattern, pattern_len,
                           regex_flags, error)) {
        return false;
    }
    ncm_buffer_set(&screen->search_constraint, pattern, pattern_len);
    screen->search_enabled = true;
    nc_menu_apply_filter(native_selected_items_adder_screen_active_menu(
                             screen));
    return true;
}

void
native_selected_items_adder_screen_clear_search(
    NativeSelectedItemsAdderScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->search_enabled = false;
    ncm_buffer_clear(&screen->search_constraint);
    nc_menu_show_all_items(native_selected_items_adder_screen_active_menu(
                               screen));
    return;
}

bool
native_selected_items_adder_screen_search(
    NativeSelectedItemsAdderScreen *screen, char *pattern,
    int32 pattern_len, uint32 regex_flags, bool forward, bool wrap,
    bool skip_current, NcmError *error) {
    NcmRegex regex;
    NcMenu *menu;
    int64 count;
    int64 start;

    if (screen == NULL || pattern == NULL || pattern_len <= 0) {
        return false;
    }

    ncm_regex_init(&regex);
    if (!ncm_regex_compile(&regex, pattern, pattern_len, regex_flags,
                           error)) {
        ncm_regex_destroy(&regex);
        return false;
    }

    menu = native_selected_items_adder_screen_active_menu(screen);
    count = nc_menu_item_count(menu);
    start = nc_menu_highlight(menu);
    if (skip_current) {
        if (forward) {
            start += 1;
        } else {
            start -= 1;
        }
    }

    for (int64 i = 0; i < count; i += 1) {
        int64 pos;

        if (forward) {
            pos = start + i;
        } else {
            pos = start - i;
        }
        if (wrap) {
            while (pos < 0) {
                pos += count;
            }
            while (pos >= count) {
                pos -= count;
            }
        }
        if (pos < 0 || pos >= count) {
            continue;
        }
        if (adder_row_matches(nc_menu_active_item_at(menu, pos),
                              &regex)) {
            ncm_regex_destroy(&regex);
            return nc_menu_goto_selectable(menu, pos);
        }
    }

    ncm_regex_destroy(&regex);
    return false;
}

NativeSelectedItemsAdderAction *
native_selected_items_adder_screen_last_action(
    NativeSelectedItemsAdderScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->last_action;
}

static NativeSelectedItemsAdderScreen *
adder_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcScreenCallbacks
adder_callbacks(void) {
    NcScreenCallbacks callbacks = {0};

    callbacks.active_window = adder_active_window_callback;
    callbacks.refresh = adder_refresh_callback;
    callbacks.refresh_window = adder_refresh_callback;
    callbacks.scroll = adder_scroll_callback;
    callbacks.switch_to = adder_switch_to_callback;
    callbacks.resize = adder_resize_callback;
    callbacks.window_timeout = adder_timeout_callback;
    callbacks.title = adder_title_callback;
    callbacks.update = adder_update_callback;
    callbacks.mouse_button_pressed = adder_mouse_callback;
    callbacks.is_lockable = adder_lockable_callback;
    callbacks.is_mergable = adder_mergable_callback;
    callbacks.destroy = adder_destroy_callback;
    return callbacks;
}

static NcWindow *
adder_active_window_callback(NcScreen *screen) {
    return native_selected_items_adder_screen_active_window(
        adder_from_screen(screen));
}

static void
adder_refresh_callback(NcScreen *screen) {
    NativeSelectedItemsAdderScreen *adder;
    NcWindow *window;
    NcMenu *menu;

    adder = adder_from_screen(screen);
    window = native_selected_items_adder_screen_active_window(adder);
    menu = native_selected_items_adder_screen_active_menu(adder);
    nc_menu_prepare_refresh(menu, nc_window_height(window), NULL, NULL);
    nc_window_display(window);
    nc_menu_refresh(menu, window, nc_window_width(window),
                    nc_window_height(window));
    return;
}

static void
adder_scroll_callback(NcScreen *screen, enum NcScroll where) {
    NativeSelectedItemsAdderScreen *adder;
    NcWindow *window;

    adder = adder_from_screen(screen);
    window = native_selected_items_adder_screen_active_window(adder);
    nc_menu_scroll_selectable(native_selected_items_adder_screen_active_menu(
                                  adder), nc_window_height(window), where);
    return;
}

static void
adder_switch_to_callback(NcScreen *screen) {
    (void)screen;
    return;
}

static void
adder_resize_callback(NcScreen *screen) {
    nc_screen_clear_resize_request(screen);
    return;
}

static int32
adder_timeout_callback(NcScreen *screen) {
    (void)screen;
    return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
}

static char *
adder_title_callback(NcScreen *screen) {
    NcScreen *previous;

    (void)screen;
    previous = nc_screen_switcher_previous();
    if (previous != NULL) {
        return nc_screen_title(previous);
    }
    return (char *)"Add selected items";
}

static void
adder_update_callback(NcScreen *screen) {
    nc_screen_clear_update_request(screen);
    return;
}

static void
adder_mouse_callback(NcScreen *screen, MEVENT event) {
    NativeSelectedItemsAdderScreen *adder;
    NcWindow *window;
    int32 x;
    int32 y;

    adder = adder_from_screen(screen);
    window = native_selected_items_adder_screen_active_window(adder);
    x = event.x;
    y = event.y;
    if (!nc_window_has_coords(window, &x, &y)) {
        return;
    }
    if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
        (void)nc_menu_goto_selectable(
            native_selected_items_adder_screen_active_menu(adder), y);
        if (event.bstate & BUTTON3_PRESSED) {
            (void)native_selected_items_adder_screen_run_current(adder);
        }
    }
    return;
}

static bool
adder_lockable_callback(NcScreen *screen) {
    (void)screen;
    return false;
}

static bool
adder_mergable_callback(NcScreen *screen) {
    (void)screen;
    return false;
}

static void
adder_destroy_callback(NcScreen *screen) {
    native_selected_items_adder_screen_destroy(adder_from_screen(screen));
    return;
}

static bool
adder_filter_callback(NcMenu *menu, void *item, void *user) {
    NativeSelectedItemsAdderScreen *screen;

    (void)menu;
    screen = user;
    if (!screen->search_enabled) {
        return true;
    }
    return adder_row_matches(item, &screen->search_regex);
}

static bool
adder_row_matches(NcEditorActionRow *row, NcmRegex *regex) {
    if (row == NULL || row->label == NULL) {
        return false;
    }
    return ncm_regex_search(regex, row->label, row->label_len);
}

static bool
adder_action_row_set(NcEditorActionRow *row, char *label,
                     int32 label_len, void (*run)(void *), void *user) {
    if (row == NULL) {
        return false;
    }
    if (label != NULL && label_len > 0) {
        row->label_cap = label_len + 1;
        row->label = ncm_malloc(row->label_cap);
        ncm_memcpy(row->label, label, label_len);
        row->label[label_len] = '\0';
        row->label_len = label_len;
    }
    row->run = run;
    row->user = user;
    return true;
}

static bool
adder_action_set_playlist(char **dest, int32 *dest_len, int32 *dest_cap,
                          char *source, int32 source_len) {
    *dest = NULL;
    *dest_len = 0;
    *dest_cap = 0;
    if (source == NULL || source_len <= 0) {
        return true;
    }
    *dest_cap = source_len + 1;
    *dest = ncm_malloc(*dest_cap);
    ncm_memcpy(*dest, source, source_len);
    (*dest)[source_len] = '\0';
    *dest_len = source_len;
    return true;
}

static void
adder_action_current_playlist(void *user) {
    NativeSelectedItemsAdderScreen *screen;

    screen = user;
    (void)native_selected_items_adder_action_set(
        &screen->last_action,
        NATIVE_SELECTED_ITEMS_ADDER_TARGET_CURRENT_PLAYLIST,
        NATIVE_SELECTED_ITEMS_ADDER_POSITION_NONE, NULL, 0);
    native_selected_items_adder_screen_choose_current_playlist(screen);
    return;
}

static void
adder_action_new_playlist(void *user) {
    NativeSelectedItemsAdderScreen *screen;

    screen = user;
    (void)native_selected_items_adder_action_set(
        &screen->last_action,
        NATIVE_SELECTED_ITEMS_ADDER_TARGET_NEW_PLAYLIST,
        NATIVE_SELECTED_ITEMS_ADDER_POSITION_NONE, NULL, 0);
    return;
}

static void
adder_action_cancel_target(void *user) {
    NativeSelectedItemsAdderScreen *screen;

    screen = user;
    (void)native_selected_items_adder_action_set(
        &screen->last_action,
        NATIVE_SELECTED_ITEMS_ADDER_TARGET_CANCEL,
        NATIVE_SELECTED_ITEMS_ADDER_POSITION_CANCEL, NULL, 0);
    return;
}

static void
adder_action_position_end(void *user) {
    NativeSelectedItemsAdderScreen *screen;

    screen = user;
    (void)native_selected_items_adder_action_set(
        &screen->last_action, screen->last_action.target,
        NATIVE_SELECTED_ITEMS_ADDER_POSITION_END,
        screen->last_action.playlist, screen->last_action.playlist_len);
    return;
}

static void
adder_action_position_beginning(void *user) {
    NativeSelectedItemsAdderScreen *screen;

    screen = user;
    (void)native_selected_items_adder_action_set(
        &screen->last_action, screen->last_action.target,
        NATIVE_SELECTED_ITEMS_ADDER_POSITION_BEGINNING,
        screen->last_action.playlist, screen->last_action.playlist_len);
    return;
}

static void
adder_action_position_current_song(void *user) {
    NativeSelectedItemsAdderScreen *screen;

    screen = user;
    (void)native_selected_items_adder_action_set(
        &screen->last_action, screen->last_action.target,
        NATIVE_SELECTED_ITEMS_ADDER_POSITION_AFTER_CURRENT_SONG,
        screen->last_action.playlist, screen->last_action.playlist_len);
    return;
}

static void
adder_action_position_current_album(void *user) {
    NativeSelectedItemsAdderScreen *screen;

    screen = user;
    (void)native_selected_items_adder_action_set(
        &screen->last_action, screen->last_action.target,
        NATIVE_SELECTED_ITEMS_ADDER_POSITION_AFTER_CURRENT_ALBUM,
        screen->last_action.playlist, screen->last_action.playlist_len);
    return;
}

static void
adder_action_position_highlighted(void *user) {
    NativeSelectedItemsAdderScreen *screen;

    screen = user;
    (void)native_selected_items_adder_action_set(
        &screen->last_action, screen->last_action.target,
        NATIVE_SELECTED_ITEMS_ADDER_POSITION_AFTER_HIGHLIGHTED,
        screen->last_action.playlist, screen->last_action.playlist_len);
    return;
}

static void
adder_action_position_cancel(void *user) {
    NativeSelectedItemsAdderScreen *screen;

    screen = user;
    screen->active_menu = NATIVE_SELECTED_ITEMS_ADDER_MENU_PLAYLISTS;
    (void)native_selected_items_adder_action_set(
        &screen->last_action, screen->last_action.target,
        NATIVE_SELECTED_ITEMS_ADDER_POSITION_CANCEL,
        screen->last_action.playlist, screen->last_action.playlist_len);
    return;
}

static void
adder_action_existing_playlist(void *user) {
    ExistingPlaylistAction *action;

    action = user;
    (void)native_selected_items_adder_action_set(
        &action->screen->last_action,
        NATIVE_SELECTED_ITEMS_ADDER_TARGET_EXISTING_PLAYLIST,
        NATIVE_SELECTED_ITEMS_ADDER_POSITION_NONE,
        action->playlist, action->playlist_len);
    return;
}

static bool
adder_add_action_row(NcEditorActionMenu *menu, char *label,
                     int32 label_len, void (*run)(void *), void *user) {
    NcEditorActionRow row;
    bool ok;

    nc_editor_action_row_init(&row);
    ok = adder_action_row_set(&row, label, label_len, run, user);
    if (ok) {
        nc_editor_action_menu_add(menu, &row);
    }
    nc_editor_action_row_destroy(&row);
    return ok;
}

static void
existing_playlist_action_init(ExistingPlaylistAction *action) {
    action->screen = NULL;
    action->playlist = NULL;
    action->playlist_len = 0;
    action->playlist_cap = 0;
    return;
}

static void
existing_playlist_action_destroy(void *user) {
    ExistingPlaylistAction *action;

    action = user;
    if (action == NULL) {
        return;
    }
    if (action->playlist != NULL) {
        ncm_free(action->playlist, action->playlist_cap);
    }
    ncm_free(action, SIZEOF(*action));
    return;
}

static ExistingPlaylistAction *
existing_playlist_action_create(NativeSelectedItemsAdderScreen *screen,
                                char *playlist, int32 playlist_len) {
    ExistingPlaylistAction *action;

    action = ncm_malloc(SIZEOF(*action));
    existing_playlist_action_init(action);
    action->screen = screen;
    if (!adder_action_set_playlist(&action->playlist,
                                   &action->playlist_len,
                                   &action->playlist_cap, playlist,
                                   playlist_len)) {
        existing_playlist_action_destroy(action);
        return NULL;
    }
    return action;
}

static void
adder_clear_playlist_selector(NativeSelectedItemsAdderScreen *screen) {
    NcMenu *menu;

    if (screen == NULL) {
        return;
    }
    menu = nc_editor_action_menu_base(&screen->playlist_selector);
    for (int64 i = 0; i < nc_menu_all_item_count(menu); i += 1) {
        NcEditorActionRow *row;

        row = nc_editor_action_menu_item_at(&screen->playlist_selector,
                                            NC_MENU_ITEMS_ALL, i);
        if (row != NULL && row->run == adder_action_existing_playlist) {
            existing_playlist_action_destroy(row->user);
            row->user = NULL;
        }
    }
    nc_menu_clear_items(menu);
    return;
}
