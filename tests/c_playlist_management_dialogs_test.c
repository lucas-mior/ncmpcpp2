#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include "app_controller.h"
#include "c/ncm_app_arrays.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "screens/nc_browser.h"
#include "screens/nc_playlist.h"
#include "screens/nc_playlist_editor.h"
#include "screens/nc_sel_items_adder.h"
#include "screens/nc_sort_playlist.h"
#include "settings.h"
#include "statusbar.h"
#include "ui_state.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)
#define TEST_EVENT_CAP 8

static struct TestState {
    NcmMpdPlaylistList playlists;
    NcWindow *displayed_windows[TEST_EVENT_CAP];
    NcWindow *refreshed_windows[TEST_EVENT_CAP];
    NcMenu *refreshed_menus[TEST_EVENT_CAP];
    char status_error[256];
    char drawn_label[256];
    int32 fetch_calls;
    int32 status_error_len;
    int32 drawn_label_len;
    int32 status_calls;
    int32 display_calls;
    int32 refresh_calls;
    int32 draw_calls;
    int32 previous_resize_calls;
    int32 previous_refresh_calls;
    bool fetch_success;
} test_state;

static void test_playlist_editor_command_preparation(void);
static void test_sort_playlist_row_movement(void);
static void test_selected_items_transfer_and_actions(void);
static void test_state_init(void);
static void test_state_destroy(void);
static void test_state_reset_events(void);
static void test_state_add_playlist(char *name, int32 name_len);
static NcEditorActionRow *test_adder_row(
    NativeSelectedItemsAdderScreen *screen, int64 pos);
static void test_assert_adder_label(
    NativeSelectedItemsAdderScreen *screen, int64 pos,
    char *label, int32 label_len);
static void test_cancel_adder(NativeSelectedItemsAdderScreen *screen);
static void test_previous_resize(void *user);
static void test_previous_refresh(void *user);

bool
__wrap_ncm_mpd_client_get_playlists(NcmMpdClient *client,
                                    NcmMpdPlaylistList *playlists,
                                    NcmError *error) {
    (void)client;
    test_state.fetch_calls += 1;
    if (!test_state.fetch_success) {
        if (test_state.playlists.count > 0) {
            assert(ncm_mpd_playlist_list_append_copy(
                playlists, &test_state.playlists.items[0]));
        }
        ncm_error_set(error, EIO, STRLIT_ARGS("playlist fetch failure"));
        return false;
    }
    if (!ncm_mpd_playlist_list_copy(playlists, &test_state.playlists)) {
        ncm_error_set(error, ENOMEM, STRLIT_ARGS("playlist copy failure"));
        return false;
    }
    ncm_error_clear(error);
    return true;
}

void
__wrap_ncm_statusbar_format(int32 delay_seconds, char *format,
                            int32 format_len, NcmStringFormatArg *args,
                            int32 args_len) {
    NcmStringView message;

    (void)delay_seconds;
    assert(ncm_string_equal(format, format_len,
                            STRLIT_ARGS("Could not fetch playlists: %1")));
    assert(args != NULL);
    assert(args_len == 1);
    assert(args[0].type == NCM_STRING_FORMAT_ARG_STRING);
    message = args[0].value.string;
    assert(message.len < (int32)SIZEOF(test_state.status_error));
    ncm_memcpy(test_state.status_error, message.data, message.len);
    test_state.status_error[message.len] = '\0';
    test_state.status_error_len = message.len;
    test_state.status_calls += 1;
    return;
}

void
__wrap_nc_window_init(NcWindow *window, int64 start_x, int64 start_y,
                      int64 width, int64 height, char *title,
                      int32 title_len, NcColor color, NcBorder border) {
    (void)title;
    (void)color;
    *window = (NcWindow){0};
    window->start_x = start_x;
    window->start_y = start_y;
    window->width = width;
    window->height = height;
    window->title_len = title_len;
    window->border = border;
    if (window->border.enabled) {
        window->start_x += 1;
        window->start_y += 1;
        window->width -= 2;
        window->height -= 2;
    }
    if (window->title_len > 0) {
        window->start_y += 2;
        window->height -= 2;
    }
    return;
}

void
__wrap_nc_window_destroy(NcWindow *window) {
    *window = (NcWindow){0};
    return;
}

void
__wrap_nc_window_resize(NcWindow *window, int64 width, int64 height) {
    window->width = width;
    window->height = height;
    if (window->border.enabled) {
        if (window->width >= 2) {
            window->width -= 2;
        }
        if (window->height >= 2) {
            window->height -= 2;
        }
    }
    if ((window->title_len > 0) && (window->height >= 2)) {
        window->height -= 2;
    }
    return;
}

void
__wrap_nc_window_move_to(NcWindow *window, int64 start_x,
                         int64 start_y) {
    window->start_x = start_x;
    window->start_y = start_y;
    if (window->border.enabled) {
        window->start_x += 1;
        window->start_y += 1;
    }
    if (window->title_len > 0) {
        window->start_y += 2;
    }
    return;
}

void
__wrap_nc_window_display(NcWindow *window) {
    assert(test_state.display_calls < TEST_EVENT_CAP);
    test_state.displayed_windows[test_state.display_calls++] = window;
    return;
}

void
__wrap_nc_window_print_data(NcWindow *window, char *string,
                            int32 string_len) {
    (void)window;
    assert(string_len >= 0);
    assert(string_len < (int32)SIZEOF(test_state.drawn_label));
    ncm_memcpy(test_state.drawn_label, string, string_len);
    test_state.drawn_label[string_len] = '\0';
    test_state.drawn_label_len = string_len;
    test_state.draw_calls += 1;
    return;
}

void
__wrap_nc_menu_refresh(NcMenu *menu, NcWindow *window,
                       int64 width, int64 height) {
    int32 event;

    (void)width;
    (void)height;
    event = test_state.refresh_calls;
    assert(event < TEST_EVENT_CAP);
    test_state.refreshed_menus[event] = menu;
    test_state.refreshed_windows[event] = window;
    test_state.refresh_calls += 1;
    return;
}

int
main(void) {
    test_state_init();
    test_playlist_editor_command_preparation();
    test_sort_playlist_row_movement();
    test_selected_items_transfer_and_actions();
    test_state_destroy();
    return EXIT_SUCCESS;
}

static void
test_state_init(void) {
    test_state = (struct TestState){0};
    ncm_mpd_playlist_list_init(&test_state.playlists);
    test_state.fetch_success = true;
    return;
}

static void
test_state_destroy(void) {
    ncm_mpd_playlist_list_destroy(&test_state.playlists);
    return;
}

static void
test_state_reset_events(void) {
    for (int32 i = 0; i < TEST_EVENT_CAP; i += 1) {
        test_state.displayed_windows[i] = NULL;
        test_state.refreshed_windows[i] = NULL;
        test_state.refreshed_menus[i] = NULL;
    }
    test_state.status_error[0] = '\0';
    test_state.drawn_label[0] = '\0';
    test_state.status_error_len = 0;
    test_state.drawn_label_len = 0;
    test_state.status_calls = 0;
    test_state.display_calls = 0;
    test_state.refresh_calls = 0;
    test_state.draw_calls = 0;
    test_state.previous_resize_calls = 0;
    test_state.previous_refresh_calls = 0;
    return;
}

static void
test_state_add_playlist(char *name, int32 name_len) {
    NcmPlaylist playlist;

    ncm_playlist_init(&playlist);
    assert(ncm_playlist_set(&playlist, name, name_len, 0));
    assert(ncm_mpd_playlist_list_append_copy(&test_state.playlists,
                                             &playlist));
    ncm_playlist_destroy(&playlist);
    return;
}

static void
test_playlist_editor_command_preparation(void) {
    NativePlaylistEditorScreen screen;
    NativePlaylistEditorCommand command;
    NcmMpdPlaylistList playlists;
    NcmPlaylist playlist;

    native_playlist_editor_screen_init(&screen, 0, 80, 0, 24,
                                       nc_color_default(),
                                       nc_border_none());
    native_playlist_editor_command_init(&command);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_playlist_init(&playlist);

    assert(ncm_playlist_set(&playlist, LIT_ARGS("mix"), 0));
    assert(ncm_mpd_playlist_list_append_copy(&playlists, &playlist));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                        &playlists));
    assert(native_playlist_editor_screen_prepare_playlist_command(
        &screen, NATIVE_PLAYLIST_EDITOR_COMMAND_RENAME,
        LIT_ARGS("renamed"), &command));
    assert(command.type == NATIVE_PLAYLIST_EDITOR_COMMAND_RENAME);
    assert(ncm_string_equal(command.playlist, command.playlist_len,
                            LIT_ARGS("mix")));
    assert(ncm_string_equal(command.target, command.target_len,
                            LIT_ARGS("renamed")));

    ncm_playlist_destroy(&playlist);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_command_destroy(&command);
    native_playlist_editor_screen_destroy(&screen);
    return;
}

static void
test_sort_playlist_row_movement(void) {
    NativeSortPlaylistDialog dialog;
    enum NcmSongGetter getters[16];
    NcMenu *menu;
    int32 count;

    native_sort_playlist_dialog_init(&dialog, 0, 0, 30, 17,
                                     nc_color_default(),
                                     nc_border_none());
    menu = nc_editor_sort_menu_base(
        native_sort_playlist_dialog_menu(&dialog));

    count = native_sort_playlist_dialog_get_order(
        &dialog, getters, (int32)(SIZEOF(getters) / SIZEOF(getters[0])));
    assert(count >= 2);
    assert(getters[0] == NCM_SONG_GETTER_ARTIST);
    assert(getters[1] == NCM_SONG_GETTER_ALBUM_ARTIST);

    nc_menu_highlight_position(menu, 0, nc_menu_item_count(menu));
    assert(native_sort_playlist_dialog_move_current_down(&dialog));
    count = native_sort_playlist_dialog_get_order(
        &dialog, getters, (int32)(SIZEOF(getters) / SIZEOF(getters[0])));
    assert(count >= 2);
    assert(getters[0] == NCM_SONG_GETTER_ALBUM_ARTIST);
    assert(getters[1] == NCM_SONG_GETTER_ARTIST);
    assert(native_sort_playlist_dialog_move_current_up(&dialog));
    count = native_sort_playlist_dialog_get_order(
        &dialog, getters, (int32)(SIZEOF(getters) / SIZEOF(getters[0])));
    assert(getters[0] == NCM_SONG_GETTER_ARTIST);

    native_sort_playlist_dialog_destroy(&dialog);
    return;
}

static NcEditorActionRow *
test_adder_row(NativeSelectedItemsAdderScreen *screen, int64 pos) {
    return nc_editor_action_menu_item_at(
        native_selected_items_adder_screen_playlist_menu(screen),
        NC_MENU_ITEMS_ALL, pos);
}

static void
test_assert_adder_label(NativeSelectedItemsAdderScreen *screen,
                        int64 pos, char *label, int32 label_len) {
    NcEditorActionRow *row;

    row = test_adder_row(screen, pos);
    assert(row != NULL);
    assert(ncm_string_equal(row->label, row->label_len, label, label_len));
    return;
}

static void
test_cancel_adder(NativeSelectedItemsAdderScreen *screen) {
    NcMenu *menu;

    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_playlist_menu(screen));
    nc_menu_highlight_position(menu, nc_menu_item_count(menu) - 1,
                               nc_menu_item_count(menu));
    assert(native_selected_items_adder_screen_run_current(screen));
    return;
}

static void
test_previous_resize(void *user) {
    (void)user;
    test_state.previous_resize_calls += 1;
    return;
}

static void
test_previous_refresh(void *user) {
    (void)user;
    test_state.previous_refresh_calls += 1;
    return;
}

static void
test_selected_items_transfer_and_actions(void) {
    NativeSelectedItemsAdderScreen screen;
    NativeSelectedItemsAdderAction *action;
    NativeBrowserScreen browser;
    NativeBrowserBridge bridge = {0};
    NativePlaylistScreen playlist = {0};
    NcmMpdClient client = {0};
    NcmSongArray copied_songs;
    NcmSongArray songs;
    NcmSong song;
    NcmError error;
    NcMenu *menu;
    NcMenu *position_menu;
    NcEditorActionRow *row;

    app_controller_init();
    nc_buffer_init(&Config.current_item_prefix);
    nc_buffer_init(&Config.current_item_suffix);
    nc_buffer_append_data(&Config.current_item_prefix,
                          STRLIT_ARGS(">"));
    nc_buffer_append_data(&Config.current_item_suffix,
                          STRLIT_ARGS("<"));
    Config.use_cyclic_scrolling = true;
    Config.centered_cursor = true;
    Config.ignore_leading_the = true;
    Config.message_delay_time = 5;
    ui_state_set_screen_size(100, 40);
    ui_state_set_main_geometry(2, 30);

    native_browser_screen_init(&browser, 0, 100, 2, 30,
                               nc_color_default(), nc_border_none());
    bridge.resize = test_previous_resize;
    bridge.refresh = test_previous_refresh;
    native_browser_screen_set_bridge(&browser, bridge);
    native_selected_items_adder_screen_init(&screen, 0, 0, 40, 10,
                                            nc_color_default(),
                                            nc_border_none());
    ncm_song_array_init(&copied_songs);
    ncm_song_array_init(&songs);
    ncm_song_init(&song);

    assert(app_controller_register_screen(
        native_browser_screen_base(&browser)));
    assert(app_controller_register_screen(
        native_selected_items_adder_screen_base(&screen)));
    assert(app_controller_switch_to_screen(
        native_browser_screen_base(&browser)));

    assert(ncm_song_set_uri(&song, LIT_ARGS("first.flac")));
    assert(ncm_song_array_append_copy(&songs, &song));
    test_state_add_playlist(LIT_ARGS("Zulu"));
    test_state_add_playlist(LIT_ARGS("Beatles"));
    test_state_add_playlist(LIT_ARGS("The Aardvarks"));
    test_state_reset_events();

    native_browser_screen_set_local(&browser, false);
    ncm_error_clear(&error);
    assert(native_selected_items_adder_screen_open(
        &screen, &songs, &playlist, &client, &error));
    assert(test_state.fetch_calls == 1);
    assert(!screen.local_browser);
    assert(screen.ready);
    assert(screen.previous_screen == native_browser_screen_base(&browser));
    assert(screen.playlist == &playlist);
    assert(screen.client == &client);
    assert(app_controller_current_screen()
           == native_selected_items_adder_screen_base(&screen));

    ncm_error_clear(&error);
    assert(!native_selected_items_adder_screen_open(
        &screen, &songs, &playlist, &client, &error));
    assert(error.code == EBUSY);
    assert(screen.ready);
    assert(app_controller_current_screen()
           == native_selected_items_adder_screen_base(&screen));

    assert(native_selected_items_adder_screen_selected_songs(
        &screen, &copied_songs));
    assert(copied_songs.len == 1);
    assert(ncm_string_equal(copied_songs.items[0].uri,
                            copied_songs.items[0].uri_len,
                            LIT_ARGS("first.flac")));

    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_playlist_menu(&screen));
    position_menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_position_menu(&screen));
    assert(menu->cyclic_scroll_enabled);
    assert(menu->autocenter_cursor);
    assert(position_menu->cyclic_scroll_enabled);
    assert(position_menu->autocenter_cursor);
    assert(menu->display_callbacks.draw != NULL);
    assert(position_menu->display_callbacks.draw != NULL);
    assert(nc_menu_item_count(menu) == 8);
    test_assert_adder_label(&screen, 0, LIT_ARGS("Current playlist"));
    test_assert_adder_label(&screen, 1, LIT_ARGS("New playlist"));
    assert(nc_menu_position_is_separator(menu, 2));
    test_assert_adder_label(&screen, 3, LIT_ARGS("The Aardvarks"));
    test_assert_adder_label(&screen, 4, LIT_ARGS("Beatles"));
    test_assert_adder_label(&screen, 5, LIT_ARGS("Zulu"));
    assert(nc_menu_position_is_separator(menu, 6));
    test_assert_adder_label(&screen, 7, LIT_ARGS("Cancel"));

    assert(screen.playlist_width == 60);
    assert(screen.playlist_height == 26);
    assert(screen.position_width == 35);
    assert(screen.position_height == 11);
    assert(nc_window_start_x(&screen.playlist_window) == 20);
    assert(nc_window_start_y(&screen.playlist_window) == 4);
    assert(nc_window_width(&screen.playlist_window) == 60);
    assert(nc_window_height(&screen.playlist_window) == 26);
    assert(nc_window_start_x(&screen.position_window) == 32);
    assert(nc_window_start_y(&screen.position_window) == 11);
    assert(nc_window_width(&screen.position_window) == 35);
    assert(nc_window_height(&screen.position_window) == 11);

    row = test_adder_row(&screen, 0);
    menu->display_callbacks.draw(menu, &screen.playlist_window, row, 0,
                                 menu->display_callbacks.user);
    assert(test_state.draw_calls == 1);
    assert(ncm_string_equal(test_state.drawn_label,
                            test_state.drawn_label_len,
                            LIT_ARGS("Current playlist")));

    ncm_error_clear(&error);
    assert(native_selected_items_adder_screen_apply_search(
        &screen, LIT_ARGS("playlist"), NCM_REGEX_LITERAL, &error));
    assert(nc_menu_is_filtered(menu));
    assert(nc_menu_is_filtered(position_menu));
    native_selected_items_adder_screen_clear_search(&screen);
    assert(!nc_menu_is_filtered(menu));
    assert(!nc_menu_is_filtered(position_menu));

    nc_menu_highlight_position(menu, 3, nc_menu_item_count(menu));
    assert(native_selected_items_adder_screen_run_current(&screen));
    action = native_selected_items_adder_screen_last_action(&screen);
    assert(action->target
           == NATIVE_SELECTED_ITEMS_ADDER_TARGET_EXISTING_PLAYLIST);
    assert(ncm_string_equal(action->playlist, action->playlist_len,
                            LIT_ARGS("The Aardvarks")));

    native_selected_items_adder_screen_choose_current_playlist(&screen);
    test_state_reset_events();
    nc_screen_refresh(native_selected_items_adder_screen_base(&screen));
    assert(test_state.display_calls == 2);
    assert(test_state.refresh_calls == 2);
    assert(test_state.displayed_windows[0] == &screen.playlist_window);
    assert(test_state.displayed_windows[1] == &screen.position_window);
    assert(test_state.refreshed_windows[0] == &screen.playlist_window);
    assert(test_state.refreshed_windows[1] == &screen.position_window);
    assert(test_state.refreshed_menus[0] == menu);
    assert(test_state.refreshed_menus[1] == position_menu);

    ui_state_set_screen_size(80, 25);
    ui_state_set_main_geometry(1, 20);
    nc_screen_request_resize(native_browser_screen_base(&browser));
    nc_screen_request_resize(
        native_selected_items_adder_screen_base(&screen));
    nc_screen_resize(native_selected_items_adder_screen_base(&screen));
    assert(test_state.previous_resize_calls == 1);
    assert(test_state.previous_refresh_calls == 1);
    assert(screen.playlist_width == 48);
    assert(screen.playlist_height == 16);
    assert(screen.position_width == 35);
    assert(screen.position_height == 11);
    assert(nc_window_start_x(&screen.playlist_window) == 16);
    assert(nc_window_start_y(&screen.playlist_window) == 3);
    assert(nc_window_start_x(&screen.position_window) == 22);
    assert(nc_window_start_y(&screen.position_window) == 5);

    nc_menu_highlight_position(position_menu,
                               nc_menu_item_count(position_menu) - 1,
                               nc_menu_item_count(position_menu));
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(screen.active_menu
           == NATIVE_SELECTED_ITEMS_ADDER_MENU_PLAYLISTS);
    test_cancel_adder(&screen);
    assert(app_controller_current_screen()
           == native_browser_screen_base(&browser));
    assert(!screen.ready);
    assert(screen.selected_songs.len == 0);
    assert(screen.previous_screen == NULL);
    assert(screen.playlist == NULL);
    assert(screen.client == NULL);

    native_browser_screen_set_local(&browser, true);
    test_state_reset_events();
    ncm_error_clear(&error);
    assert(native_selected_items_adder_screen_open(
        &screen, &songs, &playlist, &client, &error));
    assert(test_state.fetch_calls == 1);
    assert(screen.local_browser);
    action = native_selected_items_adder_screen_last_action(&screen);
    assert(action->target == NATIVE_SELECTED_ITEMS_ADDER_TARGET_NONE);
    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_playlist_menu(&screen));
    assert(nc_menu_item_count(menu) == 3);
    test_assert_adder_label(&screen, 0, LIT_ARGS("Current playlist"));
    assert(nc_menu_position_is_separator(menu, 1));
    test_assert_adder_label(&screen, 2, LIT_ARGS("Cancel"));
    test_cancel_adder(&screen);

    native_browser_screen_set_local(&browser, false);
    test_state.fetch_success = false;
    test_state_reset_events();
    ncm_error_clear(&error);
    assert(native_selected_items_adder_screen_open(
        &screen, &songs, &playlist, &client, &error));
    assert(test_state.fetch_calls == 2);
    assert(test_state.status_calls == 1);
    assert(ncm_string_equal(test_state.status_error,
                            test_state.status_error_len,
                            LIT_ARGS("playlist fetch failure")));
    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_playlist_menu(&screen));
    assert(nc_menu_item_count(menu) == 4);
    test_assert_adder_label(&screen, 0, LIT_ARGS("Current playlist"));
    test_assert_adder_label(&screen, 1, LIT_ARGS("New playlist"));
    assert(nc_menu_position_is_separator(menu, 2));
    test_assert_adder_label(&screen, 3, LIT_ARGS("Cancel"));
    test_cancel_adder(&screen);

    ncm_song_destroy(&song);
    ncm_song_array_destroy(&songs);
    ncm_song_array_destroy(&copied_songs);
    native_selected_items_adder_screen_destroy(&screen);
    assert(app_controller_unregister_screen(
        native_browser_screen_base(&browser)));
    native_browser_screen_destroy(&browser);
    nc_buffer_destroy(&Config.current_item_prefix);
    nc_buffer_destroy(&Config.current_item_suffix);
    return;
}
