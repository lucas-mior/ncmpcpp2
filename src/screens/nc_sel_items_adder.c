#if !defined(NCMPCPP_NC_SEL_ITEMS_ADDER_C)
#define NCMPCPP_NC_SEL_ITEMS_ADDER_C

#include "screens/nc_sel_items_adder.h"

#include <errno.h>
#include <stdint.h>

#include "app_controller.h"
#include "c/ncm_base.h"
#include "c/ncm_charset.h"
#include "c/ncm_comparators.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"
#include "helpers.h"
#include "screens/nc_browser.h"
#include "screens/nc_playlist.h"
#include "screens/screen_switcher.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "ui_state.h"

static NcScreenCallbacks adder_callbacks(void);
static NcWindow *adder_active_window_callback(NcScreen *screen);
static void adder_refresh_callback(NcScreen *screen);
static void adder_draw_row(NcMenu *menu, NcWindow *window, void *item,
                           int32 pos, void *user);
static void adder_scroll_callback(NcScreen *screen, enum NcScroll where);
static bool adder_can_run_current_callback(NcScreen *screen);
static bool adder_run_current_callback(NcScreen *screen);
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
static bool adder_search_position(NcMenu *menu, int32 pos, void *user);
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
static bool adder_previous_is_local_browser(NcScreen *previous);
static void adder_sort_playlist_rows(NativeSelectedItemsAdderScreen *screen,
                                     int32 begin, int32 end);
static void adder_apply_geometry(NativeSelectedItemsAdderScreen *screen);
static void adder_finish(NativeSelectedItemsAdderScreen *screen);

typedef struct ExistingPlaylistAction {
    NativeSelectedItemsAdderScreen *screen;
    char *playlist;
    int32 playlist_len;
    int32 playlist_cap;
} ExistingPlaylistAction;

static void existing_playlist_action_destroy(void *user);
static ExistingPlaylistAction *existing_playlist_action_create(
    NativeSelectedItemsAdderScreen *screen, char *playlist,
    int32 playlist_len);

void
native_selected_items_adder_screen_init(
    NativeSelectedItemsAdderScreen *screen, int32 start_x, int32 start_y,
    int32 width, int32 height, NcColor color, NcBorder border
) {
    NcScreenCallbacks callbacks;
    NcMenuDisplayCallbacks display_callbacks = {0};
    NcMenu *playlist_menu;
    NcMenu *position_menu;

    callbacks = adder_callbacks();
    nc_editor_action_menu_init(&screen->playlist_selector);
    nc_editor_action_menu_init(&screen->position_selector);
    playlist_menu = nc_editor_action_menu_base(&screen->playlist_selector);
    position_menu = nc_editor_action_menu_base(&screen->position_selector);
    nc_menu_set_highlight_prefix(playlist_menu, &Config.current_item_prefix);
    nc_menu_set_highlight_suffix(playlist_menu, &Config.current_item_suffix);
    nc_menu_set_cyclic_scrolling(playlist_menu,
                                 Config.use_cyclic_scrolling);
    nc_menu_set_centered_cursor(playlist_menu, Config.centered_cursor);
    nc_menu_set_highlight_prefix(position_menu, &Config.current_item_prefix);
    nc_menu_set_highlight_suffix(position_menu, &Config.current_item_suffix);
    nc_menu_set_cyclic_scrolling(position_menu,
                                 Config.use_cyclic_scrolling);
    nc_menu_set_centered_cursor(position_menu, Config.centered_cursor);
    nc_window_init(&screen->playlist_window, start_x, start_y, width,
                   height, STRLIT_ARGS("Add selected item(s) to..."),
                   color, border);
    nc_window_init(&screen->position_window, start_x, start_y, width,
                   height, STRLIT_ARGS("Where?"), color, border);
    ncm_song_array_init(&screen->selected_songs);
    ncm_regex_init(&screen->search_regex);
    ncm_buffer_init(&screen->search_constraint);
    screen->playlist = NULL;
    screen->previous_screen = NULL;
    screen->client = NULL;
    screen->playlist_width = width;
    screen->playlist_height = height;
    screen->position_width = width;
    screen->position_height = height;
    screen->active_menu = NATIVE_SELECTED_ITEMS_ADDER_MENU_PLAYLISTS;
    screen->local_browser = false;
    screen->search_enabled = false;
    screen->registered = false;
    screen->ready = false;
    nc_screen_init(&screen->screen, callbacks, screen,
                   NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER);
    display_callbacks.draw = adder_draw_row;
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
    NativeSelectedItemsAdderScreen *screen
) {
    if (screen == NULL) {
        return;
    }
    (void)app_controller_unregister_screen(
        native_selected_items_adder_screen_base(screen));
    for (int32 i = 0; i < nc_menu_all_item_count(
             nc_editor_action_menu_base(&screen->playlist_selector));
         i += 1) {
        NcEditorActionRow *row;

        row = nc_editor_action_menu_item_at(&screen->playlist_selector,
                                            NC_MENU_ITEMS_ALL, i);
        if (row && (row->run == adder_action_existing_playlist)) {
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
    screen->playlist = NULL;
    screen->previous_screen = NULL;
    screen->client = NULL;
    screen->registered = false;
    screen->ready = false;
    return;
}

NcScreen *
native_selected_items_adder_screen_base(
    NativeSelectedItemsAdderScreen *screen
) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->screen;
}

NcMenu *
native_selected_items_adder_screen_active_menu(
    NativeSelectedItemsAdderScreen *screen
) {
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
    NativeSelectedItemsAdderScreen *screen
) {
    if (screen == NULL) {
        return NULL;
    }
    if (screen->active_menu == NATIVE_SELECTED_ITEMS_ADDER_MENU_POSITIONS) {
        return &screen->position_window;
    }
    return &screen->playlist_window;
}

bool
native_selected_items_adder_screen_open(
    NativeSelectedItemsAdderScreen *screen, NcmSongArray *songs,
    NativePlaylistScreen *playlist, NcmMpdClient *client, NcmError *error
) {
    NcmMpdPlaylistList playlists;
    NcmSongArray selected_songs;
    NcmError playlist_error;
    NcScreen *current;
    bool local_browser;

    if (screen == NULL) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing selected items dialog"));
        return false;
    }
    if ((songs == NULL) || (songs->len <= 0)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("no selected songs"));
        return false;
    }
    if (playlist == NULL) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing native playlist"));
        return false;
    }
    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }
    if (screen->ready) {
        ncm_error_set(error, EBUSY,
                      STRLIT_ARGS("selected items dialog is already open"));
        return false;
    }

    if (((current = nc_screen_switcher_current()) == NULL)
        || (current == native_selected_items_adder_screen_base(screen))) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing previous screen"));
        return false;
    }

    ncm_song_array_init(&selected_songs);
    if (!ncm_song_array_copy(&selected_songs, songs)) {
        ncm_song_array_destroy(&selected_songs);
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("failed to copy selected songs"));
        return false;
    }

    nc_menu_reset(nc_editor_action_menu_base(&screen->playlist_selector));
    nc_menu_reset(nc_editor_action_menu_base(&screen->position_selector));
    screen->active_menu = NATIVE_SELECTED_ITEMS_ADDER_MENU_PLAYLISTS;
    screen->search_enabled = false;
    ncm_buffer_clear(&screen->search_constraint);
    nc_menu_show_all_items(
        nc_editor_action_menu_base(&screen->playlist_selector));
    nc_menu_show_all_items(
        nc_editor_action_menu_base(&screen->position_selector));

    local_browser = adder_previous_is_local_browser(current);
    ncm_mpd_playlist_list_init(&playlists);
    if (!local_browser) {
        ncm_error_clear(&playlist_error);
        if (!ncm_mpd_client_get_playlists(client, &playlists,
                                          &playlist_error)) {
            NcmStringFormatArg arg;

            ncm_mpd_playlist_list_clear(&playlists);
            arg = ncm_string_format_arg_cstring(playlist_error.message);
            ncm_statusbar_format(
                Config.message_delay_time,
                STRLIT_ARGS("Could not fetch playlists: %1"), &arg, 1);
        }
    }
    native_selected_items_adder_screen_populate_playlist_selector(
        screen, &playlists, local_browser);
    ncm_mpd_playlist_list_destroy(&playlists);
    adder_apply_geometry(screen);

    ncm_song_array_move(&screen->selected_songs, &selected_songs);
    screen->playlist = playlist;
    screen->previous_screen = current;
    screen->client = client;
    screen->ready = true;

    if (!nc_screen_switcher_switch_to(
            native_selected_items_adder_screen_base(screen), false)) {
        ncm_song_array_clear(&screen->selected_songs);
        screen->playlist = NULL;
        screen->previous_screen = NULL;
        screen->client = NULL;
        screen->ready = false;
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("selected items dialog is not registered"));
        return false;
    }

    ncm_error_clear(error);
    return true;
}

void
native_selected_items_adder_screen_populate_playlist_selector(
    NativeSelectedItemsAdderScreen *screen, NcmMpdPlaylistList *playlists,
    bool local_browser
) {
    NcEditorActionMenu *menu;
    NcMenu *base;
    int32 stored_begin;
    int32 stored_end;

    if (screen == NULL) {
        return;
    }
    menu = &screen->playlist_selector;
    base = nc_editor_action_menu_base(menu);
    adder_clear_playlist_selector(screen);
    screen->local_browser = local_browser;
    (void)adder_add_action_row(menu, STRLIT_ARGS("Current playlist"),
                               adder_action_current_playlist, screen);
    if (!local_browser) {
        (void)adder_add_action_row(menu, STRLIT_ARGS("New playlist"),
                                   adder_action_new_playlist, screen);
    }
    nc_editor_action_menu_add_separator(menu);
    stored_begin = nc_menu_all_item_count(base);
    if (!local_browser && playlists) {
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
    }
    stored_end = nc_menu_all_item_count(base);
    adder_sort_playlist_rows(screen, stored_begin, stored_end);
    if (stored_end > stored_begin) {
        nc_editor_action_menu_add_separator(menu);
    }
    (void)adder_add_action_row(menu, STRLIT_ARGS("Cancel"),
                               adder_action_cancel_target, screen);
    nc_menu_reset(base);
    screen->active_menu = NATIVE_SELECTED_ITEMS_ADDER_MENU_PLAYLISTS;
    return;
}

void
native_selected_items_adder_screen_populate_position_selector(
    NativeSelectedItemsAdderScreen *screen
) {
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
    NativeSelectedItemsAdderScreen *screen
) {
    NcEditorActionRow *row;

    if (screen == NULL) {
        return false;
    }
    row = nc_menu_current_item(
        native_selected_items_adder_screen_active_menu(screen));
    if ((row == NULL) || (row->run == NULL)) {
        return false;
    }
    row->run(row->user);
    return true;
}

bool
native_selected_items_adder_screen_return_to_previous(
    NativeSelectedItemsAdderScreen *screen
) {
    if ((screen == NULL) || !screen->ready
        || (screen->previous_screen == NULL)) {
        return false;
    }

    adder_finish(screen);
    return true;
}

void
native_selected_items_adder_screen_choose_current_playlist(
    NativeSelectedItemsAdderScreen *screen
) {
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
    char *playlist, NcmError *error
) {
    bool ok;

    if (screen == NULL) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing selected items dialog"));
        return false;
    }
    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }
    if (playlist == NULL) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing stored playlist"));
        return false;
    }
    if (screen->selected_songs.len <= 0) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("no selected songs"));
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
    if (!ok && client->command_list_active) {
        client->command_list_active = false;
    }
    return ok;
}

bool
native_selected_items_adder_screen_search(
    NativeSelectedItemsAdderScreen *screen, char *pattern,
    int32 pattern_len, uint32 regex_flags, bool forward, bool wrap,
    bool skip_current, NcmError *error
) {
    NcmRegex regex;
    NcMenu *menu;
    NcWindow *window;
    bool result;

    if ((screen == NULL) || (pattern == NULL) || (pattern_len <= 0)) {
        return false;
    }

    ncm_regex_init(&regex);
    if (!ncm_regex_compile(&regex, pattern, pattern_len, regex_flags,
                           error)) {
        ncm_regex_destroy(&regex);
        return false;
    }

    menu = native_selected_items_adder_screen_active_menu(screen);
    window = native_selected_items_adder_screen_active_window(screen);
    result = nc_menu_search_selectable(menu, nc_window_height(window),
                                       forward, wrap, skip_current,
                                       adder_search_position, &regex,
                                       NULL);

    ncm_regex_destroy(&regex);
    return result;
}

static bool
adder_search_position(NcMenu *menu, int32 pos, void *user) {
    return adder_row_matches(nc_menu_active_item_at(menu, pos), user);
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
    callbacks.can_run_current = adder_can_run_current_callback;
    callbacks.run_current = adder_run_current_callback;
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
adder_draw_row(NcMenu *menu, NcWindow *window, void *item,
               int32 pos, void *user) {
    NcEditorActionRow *row;
    StrBuilder converted;

    (void)menu;
    (void)pos;
    (void)user;
    row = item;
    if ((row == NULL) || (row->label == NULL)
        || (row->label_len <= 0)) {
        return;
    }

    converted = ncm_charset_utf8_to_locale(row->label, row->label_len);
    nc_window_print_data(window, converted.data, converted.len);
    sb_free(&converted);
    return;
}

static void
adder_refresh_callback(NcScreen *screen) {
    NativeSelectedItemsAdderScreen *adder;
    NcMenu *menu;
    NcWindow *window;

    adder = adder_from_screen(screen);
    if (adder->active_menu
        == NATIVE_SELECTED_ITEMS_ADDER_MENU_POSITIONS) {
        menu = nc_editor_action_menu_base(&adder->playlist_selector);
        nc_menu_prepare_refresh(menu,
                                nc_window_height(&adder->playlist_window),
                                NULL, NULL);
        nc_window_display(&adder->playlist_window);
        nc_menu_refresh(menu, &adder->playlist_window,
                        nc_window_width(&adder->playlist_window),
                        nc_window_height(&adder->playlist_window));
    }

    menu = native_selected_items_adder_screen_active_menu(adder);
    window = native_selected_items_adder_screen_active_window(adder);
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

static bool
adder_can_run_current_callback(NcScreen *screen) {
    NativeSelectedItemsAdderScreen *adder;
    NcEditorActionRow *row;

    if (((adder = adder_from_screen(screen)) == NULL) || !adder->ready) {
        return false;
    }
    row = nc_menu_current_item(
        native_selected_items_adder_screen_active_menu(adder));
    return row && row->run;
}

static bool
adder_run_current_callback(NcScreen *screen) {
    return native_selected_items_adder_screen_run_current(
        adder_from_screen(screen));
}

static void
adder_switch_to_callback(NcScreen *screen) {
    (void)screen;
    return;
}

static void
adder_resize_callback(NcScreen *screen) {
    NativeSelectedItemsAdderScreen *adder;
    NcScreen *previous;

    adder = adder_from_screen(screen);
    adder_apply_geometry(adder);
    previous = adder->previous_screen;
    if (previous
        && nc_screen_has_to_be_resized(previous)) {
        nc_screen_resize(previous);
        nc_screen_refresh(previous);
    }
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
    NativeSelectedItemsAdderScreen *adder;

    if ((adder = adder_from_screen(screen)) && adder->previous_screen) {
        return nc_screen_title(adder->previous_screen);
    }
    return "Add selected items";
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
    enum NcScroll where;
    int32 count;
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
        return;
    }

    count = Config.lines_scrolled;
    if (event.bstate & BUTTON5_PRESSED) {
        where = NC_SCROLL_DOWN;
    } else if (event.bstate & BUTTON4_PRESSED) {
        where = NC_SCROLL_UP;
    } else {
        return;
    }
    if (Config.mouse_list_scroll_whole_page) {
        count = 1;
        if (where == NC_SCROLL_DOWN) {
            where = NC_SCROLL_PAGE_DOWN;
        } else {
            where = NC_SCROLL_PAGE_UP;
        }
    }
    for (int32 i = 0; i < count; i += 1) {
        adder_scroll_callback(screen, where);
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
    if ((row == NULL) || (row->label == NULL)) {
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
    if (label && (label_len > 0)) {
        row->label_cap = label_len + 1;
        row->label = malloc2(row->label_cap);
        memcpy64(row->label, label, label_len);
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
    if ((source == NULL) || (source_len <= 0)) {
        return true;
    }
    *dest_cap = source_len + 1;
    *dest = malloc2(*dest_cap);
    memcpy64(*dest, source, source_len);
    (*dest)[source_len] = '\0';
    *dest_len = source_len;
    return true;
}

static bool
adder_statusbar_prompt_hook(char *text, void *user) {
    (void)user;
    return ncm_statusbar_main_hook(text, optional_strlen32(text));
}

static bool
adder_add_to_stored_playlist(
    NativeSelectedItemsAdderScreen *screen, char *playlist,
    int32 playlist_len
) {
    NcmStringFormatArg arg;
    NcmError error;

    if ((screen == NULL) || !screen->ready || (screen->client == NULL)) {
        return false;
    }

    ncm_error_clear(&error);
    if (!native_selected_items_adder_screen_add_to_existing_playlist(
            screen, screen->client, playlist, &error)) {
        if (error.message[0] != '\0') {
            ncm_statusbar_print_cstring(
                Config.message_delay_time, error.message);
        } else {
            ncm_statusbar_print_cstring(
                Config.message_delay_time,
                "Could not add selected items");
        }
        return false;
    }

    arg = ncm_string_format_arg_string(playlist, playlist_len);
    ncm_statusbar_format(
        Config.message_delay_time,
        STRLIT_ARGS("Selected item(s) added to playlist \"%1\""),
        &arg, 1);
    adder_finish(screen);
    return true;
}

static bool
adder_try_add_current_song(
    NativeSelectedItemsAdderScreen *screen, NcmSong *song,
    int32 position, bool *added, bool *success
) {
    NcmError error;

    *added = false;
    ncm_error_clear(&error);
    if (ncm_mpd_client_add_song_value(screen->client, song, position,
                                      NULL, &error)) {
        *added = true;
        return true;
    }

    if (error.code == MPD_ERROR_SERVER) {
        ncm_status_handle_server_error_value(
            screen->client,
            (int32)ncm_mpd_client_server_error_code(screen->client),
            error.message, optional_strlen32(error.message));
        *success = false;
        return true;
    }

    if (error.message[0] != '\0') {
        ncm_statusbar_print_cstring(
            Config.message_delay_time, error.message);
    } else {
        ncm_statusbar_print_cstring(
            Config.message_delay_time,
            "Could not add selected item");
    }
    return false;
}

static bool
adder_add_to_current_playlist(
    NativeSelectedItemsAdderScreen *screen, int32 position
) {
    StrBuilder message;
    char *suffix;
    bool added;
    bool success;
    int32 first;
    int32 insert_position;

    if ((screen == NULL) || !screen->ready || (screen->client == NULL)
        || (screen->selected_songs.len <= 0) || (position < -1)) {
        return false;
    }
    if (position == INT32_MAX) {
        ncm_statusbar_print_cstring(
            Config.message_delay_time,
            "Playlist position is too large");
        return false;
    }

    success = true;
    first = 0;
    while (first < screen->selected_songs.len) {
        if (!adder_try_add_current_song(
                screen, &screen->selected_songs.items[first],
                position, &added, &success)) {
            return false;
        }
        if (added) {
            break;
        }
        first += 1;
    }

    if (first < screen->selected_songs.len) {
        if (position == -1) {
            for (int32 i = first + 1;
                 i < screen->selected_songs.len; i += 1) {
                if (!adder_try_add_current_song(
                        screen, &screen->selected_songs.items[i], -1,
                        &added, &success)) {
                    return false;
                }
            }
        } else {
            insert_position = position + 1;
            for (int32 i = screen->selected_songs.len - 1;
                 i > first; i -= 1) {
                if (!adder_try_add_current_song(
                        screen, &screen->selected_songs.items[i],
                        insert_position, &added, &success)) {
                    return false;
                }
            }
        }
    }

    sb_init(&message);
    sb_append(&message, STRLIT_ARGS("Selected items added"));
    suffix = ncm_helpers_with_errors(success);
    sb_append(&message, suffix, optional_strlen32(suffix));
    ncm_statusbar_print(Config.message_delay_time,
                        message.data, message.len);
    sb_free(&message);
    adder_finish(screen);
    return true;
}

static void
adder_song_album_view(NcmSong *song, NcmStringView *album) {
    if (!ncm_song_tag_view(song, MPD_TAG_ALBUM, 0, album)) {
        ncm_string_view_set(album, "", 0);
    }
    return;
}

static void
adder_action_current_playlist(void *user) {
    NativeSelectedItemsAdderScreen *screen;

    screen = user;
    native_selected_items_adder_screen_choose_current_playlist(screen);
    return;
}

static void
adder_action_new_playlist(void *user) {
    NativeSelectedItemsAdderScreen *screen;
    NcmStatusbarScopedLock lock;
    enum NcPromptStatus prompt_status;
    NcPrompt prompt;
    NcWindow *window;
    char *input;
    char *playlist;
    int32 playlist_len;

    screen = user;
    input = NULL;
    prompt_status = NC_PROMPT_ABORTED;
    ncm_statusbar_scoped_lock_init(&lock);
    if ((window = ncm_statusbar_put())) {
        nc_window_print_data(window,
                             STRLIT_ARGS("Save playlist as: "));
        prompt = (NcPrompt){0};
        prompt.initial_text = "";
        prompt.width = -1;
        prompt.hook = adder_statusbar_prompt_hook;
        prompt.hook_user_data = NULL;
        prompt.encrypted = false;
        prompt.remember = true;
        prompt_status = nc_window_prompt(window, &prompt, &input);
    }
    ncm_statusbar_scoped_lock_destroy(&lock);

    if (prompt_status != NC_PROMPT_ACCEPTED) {
        nc_window_prompt_result_destroy(input);
        ncm_statusbar_print_cstring(
            Config.message_delay_time,
            "Action aborted");
        return;
    }

    playlist = input;
    if (playlist == NULL) {
        playlist = "";
    }
    playlist_len = optional_strlen32(playlist);
    (void)adder_add_to_stored_playlist(screen, playlist, playlist_len);
    nc_window_prompt_result_destroy(input);
    return;
}

static void
adder_action_cancel_target(void *user) {
    NativeSelectedItemsAdderScreen *screen;

    screen = user;
    adder_finish(screen);
    return;
}

static void
adder_action_position_end(void *user) {
    NativeSelectedItemsAdderScreen *screen;

    screen = user;
    (void)adder_add_to_current_playlist(screen, -1);
    return;
}

static void
adder_action_position_beginning(void *user) {
    NativeSelectedItemsAdderScreen *screen;

    screen = user;
    (void)adder_add_to_current_playlist(screen, 0);
    return;
}

static void
adder_action_position_current_song(void *user) {
    NativeSelectedItemsAdderScreen *screen;
    int32 position;

    screen = user;
    if (ncm_status_state_player() == NCM_STATUS_PLAYER_STOP) {
        return;
    }

    position = ncm_status_state_current_song_position();
    if ((position < 0) || (position == INT32_MAX)) {
        return;
    }
    (void)adder_add_to_current_playlist(screen, position + 1);
    return;
}

static void
adder_action_position_current_album(void *user) {
    NativeSelectedItemsAdderScreen *screen;
    NcmSong current;
    NcmSong next;
    NcmStringView album;
    NcmStringView next_album;
    int32 position;

    screen = user;
    if (ncm_status_state_player() == NCM_STATUS_PLAYER_STOP) {
        return;
    }

    position = ncm_status_state_current_song_position();
    if ((position < 0) || (position == INT32_MAX)) {
        return;
    }

    ncm_song_init(&current);
    if (!native_playlist_screen_now_playing_song(
            screen->playlist, position, &current)) {
        ncm_song_destroy(&current);
        return;
    }
    adder_song_album_view(&current, &album);
    position += 1;

    while (true) {
        ncm_song_init(&next);
        if (!native_playlist_screen_now_playing_song(
                screen->playlist, position, &next)) {
            ncm_song_destroy(&next);
            break;
        }
        adder_song_album_view(&next, &next_album);
        if (!STREQUAL(album.data, album.len,
                              next_album.data, next_album.len)) {
            ncm_song_destroy(&next);
            break;
        }
        ncm_song_destroy(&next);
        if (position == INT32_MAX) {
            ncm_song_destroy(&current);
            return;
        }
        position += 1;
    }

    ncm_song_destroy(&current);
    (void)adder_add_to_current_playlist(screen, position);
    return;
}

static void
adder_action_position_highlighted(void *user) {
    NativeSelectedItemsAdderScreen *screen = user;
    NcmSong song;
    int32 song_position;

    ncm_song_init(&song);
    if (!native_playlist_screen_current_song(screen->playlist, &song)) {
        ncm_song_destroy(&song);
        return;
    }
    song_position = ncm_song_position(&song);
    ncm_song_destroy(&song);

    (void)adder_add_to_current_playlist(screen, song_position + 1);
    return;
}

static void
adder_action_position_cancel(void *user) {
    NativeSelectedItemsAdderScreen *screen;

    screen = user;
    screen->active_menu = NATIVE_SELECTED_ITEMS_ADDER_MENU_PLAYLISTS;
    return;
}

static void
adder_action_existing_playlist(void *user) {
    ExistingPlaylistAction *action;

    action = user;
    (void)adder_add_to_stored_playlist(
        action->screen, action->playlist, action->playlist_len);
    return;
}

static bool
adder_add_action_row(NcEditorActionMenu *menu, char *label,
                     int32 label_len, void (*run)(void *), void *user) {
    NcEditorActionRow row;
    bool ok;

    nc_editor_action_row_init(&row);
    if ((ok = adder_action_row_set(&row, label, label_len, run, user))) {
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
    if (action->playlist) {
        free2(action->playlist, action->playlist_cap);
    }
    free2(action, SIZEOF(*action));
    return;
}

static ExistingPlaylistAction *
existing_playlist_action_create(NativeSelectedItemsAdderScreen *screen,
                                char *playlist, int32 playlist_len) {
    ExistingPlaylistAction *action;

    action = malloc2(SIZEOF(*action));
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
    for (int32 i = 0; i < nc_menu_all_item_count(menu); i += 1) {
        NcEditorActionRow *row;

        row = nc_editor_action_menu_item_at(&screen->playlist_selector,
                                            NC_MENU_ITEMS_ALL, i);
        if (row && (row->run == adder_action_existing_playlist)) {
            existing_playlist_action_destroy(row->user);
            row->user = NULL;
        }
    }
    nc_menu_clear_items(menu);
    return;
}

static bool
adder_previous_is_local_browser(NcScreen *previous) {
    NativeBrowserScreen *browser;

    if ((previous == NULL)
        || (nc_screen_type(previous) != NC_SCREEN_TYPE_BROWSER)) {
        return false;
    }
    if (nc_screen_user(previous) != (void *)previous) {
        return false;
    }

    browser = nc_screen_user(previous);
    return native_browser_screen_is_local(browser);
}

static void
adder_sort_playlist_rows(NativeSelectedItemsAdderScreen *screen,
                         int32 begin, int32 end) {
    NcMenu *menu;

    menu = nc_editor_action_menu_base(&screen->playlist_selector);
    for (int32 i = begin; i < end; i += 1) {
        int32 smallest;

        smallest = i;
        for (int32 j = i + 1; j < end; j += 1) {
            NcEditorActionRow *left;
            NcEditorActionRow *right;

            left = nc_editor_action_menu_item_at(
                &screen->playlist_selector, NC_MENU_ITEMS_ALL, smallest);
            right = nc_editor_action_menu_item_at(
                &screen->playlist_selector, NC_MENU_ITEMS_ALL, j);
            if (left && right
                && (ncm_compare_locale_strings(
                        right->label, right->label_len,
                        left->label, left->label_len,
                        Config.ignore_leading_the) < 0)) {
                smallest = j;
            }
        }
        if (smallest != i) {
            nc_menu_swap_item_slots(menu, NC_MENU_ITEMS_ALL, i, smallest);
        }
    }
    return;
}

static void
adder_apply_geometry(NativeSelectedItemsAdderScreen *screen) {
    int32 main_height;
    int32 main_start_y;
    int32 screen_height;
    int32 screen_width;
    int32 playlist_start_x;
    int32 playlist_start_y;
    int32 position_start_x;
    int32 position_start_y;

    screen_width = ui_state_screen_width();
    screen_height = ui_state_screen_height();
    main_start_y = ui_state_main_start_y();
    main_height = ui_state_main_height();
    if (screen_width < 0) {
        screen_width = 0;
    }
    if (screen_height < 0) {
        screen_height = 0;
    }
    if (main_height < 0) {
        main_height = 0;
    }

    screen->playlist_width = screen_width*6/10;
    screen->playlist_height = screen_height*66/100;
    if (screen->playlist_height > main_height) {
        screen->playlist_height = main_height;
    }
    screen->position_width = screen_width;
    if (screen->position_width > 35) {
        screen->position_width = 35;
    }
    screen->position_height = main_height;
    if (screen->position_height > 11) {
        screen->position_height = 11;
    }

    playlist_start_x = (screen_width - screen->playlist_width)/2;
    playlist_start_y = main_start_y
                       + (main_height - screen->playlist_height)/2;
    position_start_x = (screen_width - screen->position_width)/2;
    position_start_y = main_start_y
                       + (main_height - screen->position_height)/2;
    nc_window_resize(&screen->playlist_window, screen->playlist_width,
                     screen->playlist_height);
    nc_window_move_to(&screen->playlist_window, playlist_start_x,
                      playlist_start_y);
    nc_window_resize(&screen->position_window, screen->position_width,
                     screen->position_height);
    nc_window_move_to(&screen->position_window, position_start_x,
                      position_start_y);
    return;
}

static void
adder_finish(NativeSelectedItemsAdderScreen *screen) {
    NcScreen *previous;

    if (screen == NULL) {
        return;
    }

    previous = screen->previous_screen;
    screen->ready = false;
    if (previous) {
        (void)nc_screen_switcher_switch_to(
            previous, nc_screen_has_to_be_resized(previous));
    }

    ncm_song_array_clear(&screen->selected_songs);
    screen->playlist = NULL;
    screen->previous_screen = NULL;
    screen->client = NULL;
    screen->active_menu = NATIVE_SELECTED_ITEMS_ADDER_MENU_PLAYLISTS;
    screen->search_enabled = false;
    ncm_buffer_clear(&screen->search_constraint);
    nc_menu_show_all_items(
        nc_editor_action_menu_base(&screen->playlist_selector));
    nc_menu_show_all_items(
        nc_editor_action_menu_base(&screen->position_selector));
    return;
}

#endif /* NCMPCPP_NC_SEL_ITEMS_ADDER_C */
