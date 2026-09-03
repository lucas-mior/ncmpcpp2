#if !defined(NCMPCPP_NC_SEL_ITEMS_ADDER_C)
#define NCMPCPP_NC_SEL_ITEMS_ADDER_C

#include "cbase.h"

#include "app_controller.h"
#include "c/ncm_c.h"
#include "helpers.h"
#include "screens/nc_screens.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "ui_state.h"

static void adder_display(SelectedItemsAdderScreen *screen);
static void adder_draw_row(NcMenu *menu, NcWindow *window, void *item,
                           int32 pos, void *user);
static bool adder_can_run_current_callback(NcScreen *screen);
static int32 adder_run_current_callback(NcScreen *screen);
static void adder_resize_callback(NcScreen *screen);
static char *adder_title_callback(NcScreen *screen);
static void adder_update_callback(NcScreen *screen);
static void adder_mouse_callback(NcScreen *screen, MEVENT event);
static bool adder_filter_callback(NcMenu *menu, void *item, void *user);
static bool adder_row_matches(NcEditorActionRow *row, NcmRegex *regex);
static bool adder_position_matches(NcMenu *menu, int32 pos, void *user);
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
static void adder_add_action_row(NcEditorActionMenu *menu, char *label,
                                  int32 label_len, void (*run)(void *),
                                  void *user);
static void adder_clear_playlist_selector(
    SelectedItemsAdderScreen *screen);
static bool adder_previous_is_local_browser(NcScreen *previous);
static void adder_sort_playlist_rows(SelectedItemsAdderScreen *screen,
                                     int32 begin, int32 end);
static void adder_apply_geometry(SelectedItemsAdderScreen *screen);
static void adder_finish(SelectedItemsAdderScreen *screen);

typedef struct ExistingPlaylistAction {
    SelectedItemsAdderScreen *screen;
    char *playlist;
    int32 playlist_len;
    int32 playlist_cap;
} ExistingPlaylistAction;

static void existing_playlist_action_destroy(void *user);
static ExistingPlaylistAction *existing_playlist_action_create(
    SelectedItemsAdderScreen *screen, char *playlist,
    int32 playlist_len);

#define NC_SCREEN_IMPL_TYPE SelectedItemsAdderScreen
#define NC_SCREEN_IMPL_PREFIX adder
#define NC_SCREEN_IMPL_PUBLIC_PREFIX selected_items_adder_screen
#define NC_SCREEN_IMPL_BASE_FIELD screen
#define NC_SCREEN_IMPL_WINDOW(screen) \
    selected_items_adder_screen_active_window(screen)
#define NC_SCREEN_IMPL_MENU(screen) \
    selected_items_adder_screen_active_menu(screen)
#define NC_SCREEN_IMPL_REFRESH_CALLBACK adder_display
#define NC_SCREEN_IMPL_CAN_RUN_CURRENT_CALLBACK \
    adder_can_run_current_callback
#define NC_SCREEN_IMPL_RUN_CURRENT_CALLBACK adder_run_current_callback
#define NC_SCREEN_IMPL_RESIZE_CALLBACK adder_resize_callback
#define NC_SCREEN_IMPL_TITLE_CALLBACK adder_title_callback
#define NC_SCREEN_IMPL_UPDATE_CALLBACK adder_update_callback
#define NC_SCREEN_IMPL_MOUSE_CALLBACK adder_mouse_callback
#define NC_SCREEN_IMPL_DESTROY_TYPED_CALLBACK \
    selected_items_adder_screen_destroy
#include "screens/nc_screen_impl_template.h"

void
selected_items_adder_screen_init(
    SelectedItemsAdderScreen *screen, int32 start_x, int32 start_y,
    int32 width, int32 height, NcColor color, NcBorder border
) {
    NcMenuDisplayCallbacks display_callbacks = {0};
    NcMenu *playlist_menu;
    NcMenu *position_menu;

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
                   height, STRLIT("Add selected item(s) to..."),
                   color, border);
    nc_window_init(&screen->position_window, start_x, start_y, width,
                   height, STRLIT("Where?"), color, border);
    screen->selected_songs = (NcmSongArray){0};
    screen->search_regex = (NcmRegex){0};

    screen->search_constraint = (StrBuilder){0};

    screen->playlist = NULL;
    screen->previous_screen = NULL;
    screen->client = NULL;
    screen->playlist_width = width;
    screen->playlist_height = height;
    screen->position_width = width;
    screen->position_height = height;
    screen->active_menu = SELECTED_ITEMS_ADDER_MENU_PLAYLISTS;
    screen->local_browser = false;
    screen->search_enabled = false;
    screen->registered = false;
    screen->ready = false;
    nc_screen_init_ops(&screen->screen, adder_ops, screen,
                       NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER);
    display_callbacks.draw = adder_draw_row;
    display_callbacks.matches_filter = adder_filter_callback;
    display_callbacks.user = screen;
    nc_menu_set_display_callbacks(nc_editor_action_menu_base(
        &screen->playlist_selector),
                                  display_callbacks);
    nc_menu_set_display_callbacks(nc_editor_action_menu_base(
        &screen->position_selector),
                                  display_callbacks);
    selected_items_adder_screen_populate_position_selector(screen);
    return;
}

void
selected_items_adder_screen_destroy(
    SelectedItemsAdderScreen *screen
) {
    if (screen == NULL) {
        return;
    }
    (void)app_controller_unregister_screen(
        selected_items_adder_screen_base(screen));
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
    sb_free(&screen->search_constraint);
    screen->playlist = NULL;
    screen->previous_screen = NULL;
    screen->client = NULL;
    screen->registered = false;
    screen->ready = false;
    return;
}

NcMenu *
selected_items_adder_screen_active_menu(
    SelectedItemsAdderScreen *screen
) {
    if (screen == NULL) {
        return NULL;
    }
    if (screen->active_menu == SELECTED_ITEMS_ADDER_MENU_POSITIONS) {
        return nc_editor_action_menu_base(&screen->position_selector);
    }
    return nc_editor_action_menu_base(&screen->playlist_selector);
}

NcWindow *
selected_items_adder_screen_active_window(
    SelectedItemsAdderScreen *screen
) {
    if (screen == NULL) {
        return NULL;
    }
    if (screen->active_menu == SELECTED_ITEMS_ADDER_MENU_POSITIONS) {
        return &screen->position_window;
    }
    return &screen->playlist_window;
}

int32
selected_items_adder_screen_open(
    SelectedItemsAdderScreen *screen, NcmSongArray *songs,
    PlaylistScreen *playlist, NcmMpdClient *client, NcmError *ncm_error
) {
    NcmMpdPlaylistList playlists;
    NcmSongArray selected_songs;
    NcmError playlist_error;
    NcScreen *current;
    int32 status;
    bool local_browser;

    if (screen == NULL) {
        return ncm_error_set_status(
            ncm_error, -EINVAL, STRLIT("missing selected items dialog"));
    }
    if ((songs == NULL) || (songs->len <= 0)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("no selected songs"));
    }
    if (playlist == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing playlist screen"));
    }
    if (client == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing MPD client"));
    }
    if (screen->ready) {
        return ncm_error_set_status(
            ncm_error, -EBUSY,
            STRLIT("selected items dialog is already open"));
    }

    if (((current = nc_screen_switcher_current()) == NULL)
        || (current == selected_items_adder_screen_base(screen))) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing previous screen"));
    }

    selected_songs = (NcmSongArray){0};
    if ((status = ncm_song_array_copy(&selected_songs, songs)) < 0) {
        ncm_song_array_destroy(&selected_songs);
        ncm_error_set_status(ncm_error, status,
                             STRLIT("failed to copy selected songs"));
        return status;
    }

    nc_menu_reset(nc_editor_action_menu_base(&screen->playlist_selector));
    nc_menu_reset(nc_editor_action_menu_base(&screen->position_selector));
    screen->active_menu = SELECTED_ITEMS_ADDER_MENU_PLAYLISTS;
    screen->search_enabled = false;
    sb_clear(&screen->search_constraint);
    nc_menu_show_all_items(
        nc_editor_action_menu_base(&screen->playlist_selector));
    nc_menu_show_all_items(
        nc_editor_action_menu_base(&screen->position_selector));

    local_browser = adder_previous_is_local_browser(current);
    playlists = (NcmMpdPlaylistList){0};
    if (!local_browser) {
        ncm_error_clear(&playlist_error);
        if (ncm_mpd_client_get_playlists(client, &playlists,
                                         &playlist_error) < 0) {
            NcmStringFormatArg arg;

            ncm_mpd_playlist_list_clear(&playlists);
            arg = ncm_string_format_arg_cstring(playlist_error.message);
            ncm_statusbar_format(
                Config.message_delay_time,
                STRLIT("Could not fetch playlists: %1"), &arg, 1);
        }
    }
    selected_items_adder_screen_populate_playlist_selector(
        screen, &playlists, local_browser);
    ncm_mpd_playlist_list_destroy(&playlists);
    adder_apply_geometry(screen);

    ncm_song_array_move(&screen->selected_songs, &selected_songs);
    screen->playlist = playlist;
    screen->previous_screen = current;
    screen->client = client;
    screen->ready = true;

    if ((status = nc_screen_switcher_switch_to(
         selected_items_adder_screen_base(screen), false)) < 0) {
        ncm_song_array_clear(&screen->selected_songs);
        screen->playlist = NULL;
        screen->previous_screen = NULL;
        screen->client = NULL;
        screen->ready = false;
        ncm_error_set_status(
            ncm_error, status,
            STRLIT("selected items dialog is not registered"));
        return status;
    }

    ncm_error_clear(ncm_error);
    return 0;
}

void
selected_items_adder_screen_populate_playlist_selector(
    SelectedItemsAdderScreen *screen, NcmMpdPlaylistList *playlists,
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
    adder_add_action_row(menu, STRLIT("Current playlist"),
                               adder_action_current_playlist, screen);
    if (!local_browser) {
        adder_add_action_row(menu, STRLIT("New playlist"),
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
            adder_add_action_row(menu, playlist->path, playlist->path_len,
                                 adder_action_existing_playlist, action);
        }
    }
    stored_end = nc_menu_all_item_count(base);
    adder_sort_playlist_rows(screen, stored_begin, stored_end);
    if (stored_end > stored_begin) {
        nc_editor_action_menu_add_separator(menu);
    }
    adder_add_action_row(menu, STRLIT("Cancel"),
                               adder_action_cancel_target, screen);
    nc_menu_reset(base);
    screen->active_menu = SELECTED_ITEMS_ADDER_MENU_PLAYLISTS;
    return;
}

void
selected_items_adder_screen_populate_position_selector(
    SelectedItemsAdderScreen *screen
) {
    NcEditorActionMenu *menu;

    if (screen == NULL) {
        return;
    }
    menu = &screen->position_selector;
    nc_menu_clear_items(nc_editor_action_menu_base(menu));
    adder_add_action_row(menu, STRLIT("At the end of playlist"),
                               adder_action_position_end, screen);
    adder_add_action_row(menu,
                               STRLIT("At the beginning of playlist"),
                               adder_action_position_beginning, screen);
    adder_add_action_row(menu, STRLIT("After current song"),
                               adder_action_position_current_song, screen);
    adder_add_action_row(menu, STRLIT("After current album"),
                               adder_action_position_current_album, screen);
    adder_add_action_row(menu, STRLIT("After highlighted item"),
                               adder_action_position_highlighted, screen);
    nc_editor_action_menu_add_separator(menu);
    adder_add_action_row(menu, STRLIT("Cancel"),
                               adder_action_position_cancel, screen);
    return;
}

int32
selected_items_adder_screen_run_current(
    SelectedItemsAdderScreen *screen
) {
    NcEditorActionRow *row;

    if (screen == NULL) {
        return -EINVAL;
    }
    row = nc_menu_current_item(
        selected_items_adder_screen_active_menu(screen));
    if ((row == NULL) || (row->run == NULL)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    row->run(row->user);
    return 0;
}

int32
selected_items_adder_screen_return_to_previous(
    SelectedItemsAdderScreen *screen
) {
    if (screen == NULL) {
        return -EINVAL;
    }
    if (!screen->ready || (screen->previous_screen == NULL)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    adder_finish(screen);
    return 0;
}

void
selected_items_adder_screen_choose_current_playlist(
    SelectedItemsAdderScreen *screen
) {
    if (screen == NULL) {
        return;
    }
    screen->active_menu = SELECTED_ITEMS_ADDER_MENU_POSITIONS;
    nc_menu_reset(nc_editor_action_menu_base(&screen->position_selector));
    return;
}

int32
selected_items_adder_screen_add_to_existing_playlist(
    SelectedItemsAdderScreen *screen, NcmMpdClient *client,
    char *playlist, NcmError *ncm_error
) {
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(
            ncm_error, -EINVAL, STRLIT("missing selected items dialog"));
    }
    if (client == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing MPD client"));
    }
    if (playlist == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing stored playlist"));
    }
    if (screen->selected_songs.len <= 0) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("no selected songs"));
    }

    status = ncm_mpd_client_start_command_list(client, ncm_error);
    for (int32 i = 0; (status == 0) && (i < screen->selected_songs.len);
         i += 1) {
        status = ncm_mpd_client_add_song_to_playlist(
            client, playlist, &screen->selected_songs.items[i], ncm_error);
    }
    if (status == 0) {
        status = ncm_mpd_client_commit_command_list(client, ncm_error);
    }
    if ((status < 0) && client->command_list_active) {
        client->command_list_active = false;
    }
    return status;
}

int32
selected_items_adder_screen_search(
    SelectedItemsAdderScreen *screen, char *pattern,
    int32 pattern_len, uint32 regex_flags, bool forward, bool wrap,
    bool skip_current, NcmError *ncm_error
) {
    NcmRegex regex;
    NcMenu *menu;
    NcWindow *window;
    bool found;
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing selected-items adder"));
    }
    if ((pattern == NULL) || (pattern_len <= 0)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing search pattern"));
    }

    regex = (NcmRegex){0};
    if ((status = ncm_regex_compile(&regex, pattern, pattern_len,
                                    regex_flags, ncm_error)) < 0) {
        ncm_regex_destroy(&regex);
        return status;
    }

    menu = selected_items_adder_screen_active_menu(screen);
    window = selected_items_adder_screen_active_window(screen);
    found = nc_menu_search_selectable(menu, nc_window_height(window),
                                      forward, wrap, skip_current,
                                      adder_position_matches, &regex,
                                      NULL) == 0;

    ncm_regex_destroy(&regex);
    if (found) {
        return 1;
    }
    return 0;
}

static bool
adder_position_matches(NcMenu *menu, int32 pos, void *user) {
    return adder_row_matches(nc_menu_active_item_at(menu, pos), user);
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
adder_display(SelectedItemsAdderScreen *adder) {
    NcMenu *menu;
    NcWindow *window;

    if (adder->active_menu
        == SELECTED_ITEMS_ADDER_MENU_POSITIONS) {
        menu = nc_editor_action_menu_base(&adder->playlist_selector);
        nc_menu_prepare_refresh(menu,
                                nc_window_height(&adder->playlist_window),
                                NULL, NULL);
        nc_window_display(&adder->playlist_window);
        nc_menu_refresh(menu, &adder->playlist_window,
                        nc_window_width(&adder->playlist_window),
                        nc_window_height(&adder->playlist_window));
    }

    menu = selected_items_adder_screen_active_menu(adder);
    window = selected_items_adder_screen_active_window(adder);
    nc_menu_prepare_refresh(menu, nc_window_height(window), NULL, NULL);
    nc_window_display(window);
    nc_menu_refresh(menu, window, nc_window_width(window),
                    nc_window_height(window));
    return;
}

static bool
adder_can_run_current_callback(NcScreen *screen) {
    SelectedItemsAdderScreen *adder;
    NcEditorActionRow *row;

    if (((adder = adder_from_screen(screen)) == NULL) || !adder->ready) {
        return false;
    }
    row = nc_menu_current_item(
        selected_items_adder_screen_active_menu(adder));
    return row && row->run;
}

static int32
adder_run_current_callback(NcScreen *screen) {
    return selected_items_adder_screen_run_current(
        adder_from_screen(screen));
}

static void
adder_resize_callback(NcScreen *screen) {
    SelectedItemsAdderScreen *adder;
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


static char *
adder_title_callback(NcScreen *screen) {
    SelectedItemsAdderScreen *adder;

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
    SelectedItemsAdderScreen *adder;
    NcWindow *window;
    enum NcScroll where;
    int32 count;
    int32 x;
    int32 y;

    adder = adder_from_screen(screen);
    window = selected_items_adder_screen_active_window(adder);
    x = event.x;
    y = event.y;
    if (!nc_window_has_coords(window, &x, &y)) {
        return;
    }
    if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
        (void)nc_menu_goto_selectable(
            selected_items_adder_screen_active_menu(adder), y);
        if (event.bstate & BUTTON3_PRESSED) {
            (void)selected_items_adder_screen_run_current(adder);
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
        adder_scroll(screen, where);
    }
    return;
}



static bool
adder_filter_callback(NcMenu *menu, void *item, void *user) {
    SelectedItemsAdderScreen *screen;

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
    return ncm_regex_matches(regex, row->label, row->label_len);
}

static void
adder_action_row_set(NcEditorActionRow *row, char *label,
                     int32 label_len, void (*run)(void *), void *user) {
    ASSERT(row != NULL);
    if (label && (label_len > 0)) {
        row->label_cap = label_len + 1;
        row->label = malloc2(row->label_cap);
        memcpy64(row->label, label, label_len);
        row->label[label_len] = '\0';
        row->label_len = label_len;
    }
    row->run = run;
    row->user = user;
    return;
}

static void
adder_action_set_playlist(char **dest, int32 *dest_len, int32 *dest_cap,
                          char *source, int32 source_len) {
    *dest = NULL;
    *dest_len = 0;
    *dest_cap = 0;
    if ((source == NULL) || (source_len <= 0)) {
        return;
    }
    *dest_cap = source_len + 1;
    *dest = malloc2(*dest_cap);
    memcpy64(*dest, source, source_len);
    (*dest)[source_len] = '\0';
    *dest_len = source_len;
    return;
}

static bool
adder_statusbar_prompt_can_continue(char *text, void *user) {
    (void)user;
    return ncm_statusbar_prompt_should_continue(text, optional_strlen32(text));
}

static int32
adder_add_to_stored_playlist(
    SelectedItemsAdderScreen *screen, char *playlist,
    int32 playlist_len
) {
    NcmStringFormatArg arg;
    NcmError ncm_error;
    int32 status;

    if ((screen == NULL) || !screen->ready || (screen->client == NULL)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    ncm_error_clear(&ncm_error);
    status = selected_items_adder_screen_add_to_existing_playlist(
        screen, screen->client, playlist, &ncm_error);
    if (status < 0) {
        if (ncm_error.message[0] != '\0') {
            ncm_statusbar_print_cstring(
                Config.message_delay_time, ncm_error.message);
        } else {
            ncm_statusbar_print_cstring(
                Config.message_delay_time,
                "Could not add selected items");
        }
        return status;
    }

    arg = ncm_string_format_arg_string(playlist, playlist_len);
    ncm_statusbar_format(
        Config.message_delay_time,
        STRLIT("Selected item(s) added to playlist \"%1\""),
        &arg, 1);
    adder_finish(screen);
    return 0;
}

static int32
adder_try_add_current_song(
    SelectedItemsAdderScreen *screen, NcmSong *song,
    int32 position, bool *added, bool *success
) {
    NcmError ncm_error;
    enum mpd_server_error server_error;
    int32 status;

    *added = false;
    ncm_error_clear(&ncm_error);
    status = ncm_mpd_client_add_song_value(screen->client, song, position,
                                           NULL, &ncm_error);
    if (status == 0) {
        *added = true;
        return 0;
    }

    if (ncm_error.code == MPD_ERROR_SERVER) {
        server_error = ncm_mpd_client_server_error_code(screen->client);
        ncm_status_handle_server_error_value(
            screen->client, (int32)server_error,
            ncm_error.message, optional_strlen32(ncm_error.message));
        *success = false;
        return 0;
    }

    if (ncm_error.message[0] != '\0') {
        ncm_statusbar_print_cstring(
            Config.message_delay_time, ncm_error.message);
    } else {
        ncm_statusbar_print_cstring(
            Config.message_delay_time,
            "Could not add selected item");
    }
    return status;
}

static int32
adder_add_to_current_playlist(
    SelectedItemsAdderScreen *screen, int32 position
) {
    StrBuilder message = {0};
    char *suffix;
    bool added;
    bool success;
    int32 first;
    int32 insert_position;
    int32 status;

    if ((screen == NULL) || !screen->ready || (screen->client == NULL)
        || (screen->selected_songs.len <= 0) || (position < -1)) {
        return -EINVAL;
    }
    if (position == INT32_MAX) {
        ncm_statusbar_print_cstring(
            Config.message_delay_time,
            "Playlist position is too large");
        return -EOVERFLOW;
    }

    success = true;
    first = 0;
    while (first < screen->selected_songs.len) {
        status = adder_try_add_current_song(
            screen, &screen->selected_songs.items[first],
            position, &added, &success);
        if (status < 0) {
            return status;
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
                status = adder_try_add_current_song(
                    screen, &screen->selected_songs.items[i], -1,
                    &added, &success);
                if (status < 0) {
                    return status;
                }
            }
        } else {
            insert_position = position + 1;
            for (int32 i = screen->selected_songs.len - 1;
                 i > first; i -= 1) {
                status = adder_try_add_current_song(
                    screen, &screen->selected_songs.items[i],
                    insert_position, &added, &success);
                if (status < 0) {
                    return status;
                }
            }
        }
    }

    SB_APPEND(&message, "Selected items added");
    suffix = ncm_helpers_with_errors(success);
    SB_APPEND(&message, suffix, optional_strlen32(suffix));
    ncm_statusbar_print(Config.message_delay_time,
                        message.data, message.len);
    sb_free(&message);
    adder_finish(screen);
    return 0;
}

static void
adder_song_album_view(NcmSong *song, NcmStringView *album) {
    if (!ncm_song_has_tag_view(song, MPD_TAG_ALBUM, 0, album)) {
        ncm_string_view_set(album, "", 0);
    }
    return;
}

static void
adder_action_current_playlist(void *user) {
    SelectedItemsAdderScreen *screen;

    screen = user;
    selected_items_adder_screen_choose_current_playlist(screen);
    return;
}

static void
adder_action_new_playlist(void *user) {
    SelectedItemsAdderScreen *screen;
    NcmStatusbarScopedLock scoped_lock;
    enum NcPromptStatus prompt_status;
    NcPrompt prompt;
    NcWindow *window;
    char *input;
    char *playlist;
    int32 playlist_len;

    screen = user;
    input = NULL;
    prompt_status = NC_PROMPT_ABORTED;
    ncm_statusbar_scoped_lock_init(&scoped_lock);
    if ((window = ncm_statusbar_put())) {
        nc_window_print_data(window,
                             STRLIT("Save playlist as: "));
        prompt = (NcPrompt){0};
        prompt.initial_text = "";
        prompt.width = -1;
        prompt.should_continue = adder_statusbar_prompt_can_continue;
        prompt.should_continue_user_data = NULL;
        prompt.encrypted = false;
        prompt.remember = true;
        prompt_status = nc_window_prompt(window, &prompt, &input);
    }
    ncm_statusbar_scoped_lock_destroy(&scoped_lock);

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
    SelectedItemsAdderScreen *screen;

    screen = user;
    adder_finish(screen);
    return;
}

static void
adder_action_position_end(void *user) {
    SelectedItemsAdderScreen *screen;

    screen = user;
    (void)adder_add_to_current_playlist(screen, -1);
    return;
}

static void
adder_action_position_beginning(void *user) {
    SelectedItemsAdderScreen *screen;

    screen = user;
    (void)adder_add_to_current_playlist(screen, 0);
    return;
}

static void
adder_action_position_current_song(void *user) {
    SelectedItemsAdderScreen *screen;
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
    SelectedItemsAdderScreen *screen;
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

    current = (NcmSong){0};
    if (playlist_screen_now_playing_song(
        screen->playlist, position, &current) < 0) {
        ncm_song_destroy(&current);
        return;
    }
    adder_song_album_view(&current, &album);
    position += 1;

    while (true) {
        next = (NcmSong){0};
        if (playlist_screen_now_playing_song(
            screen->playlist, position, &next) < 0) {
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
    SelectedItemsAdderScreen *screen = user;
    NcmSong song;
    int32 song_position;

    song = (NcmSong){0};
    if (playlist_screen_current_song(screen->playlist, &song) < 0) {
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
    SelectedItemsAdderScreen *screen;

    screen = user;
    screen->active_menu = SELECTED_ITEMS_ADDER_MENU_PLAYLISTS;
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

static void
adder_add_action_row(NcEditorActionMenu *menu, char *label,
                     int32 label_len, void (*run)(void *), void *user) {
    NcEditorActionRow row;

    row = (NcEditorActionRow){0};
    adder_action_row_set(&row, label, label_len, run, user);
    nc_editor_action_menu_add(menu, &row);
    nc_editor_action_row_destroy(&row);
    return;
}

static void
existing_playlist_action_destroy(void *user) {
    ExistingPlaylistAction *action;

    action = user;
    if (action == NULL) {
        return;
    }
    free2(action->playlist, action->playlist_cap);
    free2(action, SIZEOF(*action));
    return;
}

static ExistingPlaylistAction *
existing_playlist_action_create(SelectedItemsAdderScreen *screen,
                                char *playlist, int32 playlist_len) {
    ExistingPlaylistAction *action;

    action = malloc2(SIZEOF(*action));
    *action = (ExistingPlaylistAction){0};
    action->screen = screen;
    adder_action_set_playlist(&action->playlist, &action->playlist_len,
                              &action->playlist_cap, playlist,
                              playlist_len);
    return action;
}

static void
adder_clear_playlist_selector(SelectedItemsAdderScreen *screen) {
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
    BrowserScreen *browser;

    if ((previous == NULL)
        || (nc_screen_type(previous) != NC_SCREEN_TYPE_BROWSER)) {
        return false;
    }
    if (nc_screen_user(previous) != (void *)previous) {
        return false;
    }

    browser = nc_screen_user(previous);
    return browser_screen_is_local(browser);
}

static void
adder_sort_playlist_rows(SelectedItemsAdderScreen *screen,
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
adder_apply_geometry(SelectedItemsAdderScreen *screen) {
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
adder_finish(SelectedItemsAdderScreen *screen) {
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
    screen->active_menu = SELECTED_ITEMS_ADDER_MENU_PLAYLISTS;
    screen->search_enabled = false;
    sb_clear(&screen->search_constraint);
    nc_menu_show_all_items(
        nc_editor_action_menu_base(&screen->playlist_selector));
    nc_menu_show_all_items(
        nc_editor_action_menu_base(&screen->position_selector));
    return;
}

#endif /* NCMPCPP_NC_SEL_ITEMS_ADDER_C */
