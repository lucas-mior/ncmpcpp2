#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "actions.h"
#include "app_controller.h"
#include "global.h"
#include "c/ncm_playlist_sort.h"
#include "cbase/base_macros.h"
#include "screens/native_c_screens.h"
#include "screens/nc_playlist.h"
#include "screens/nc_sort_playlist.h"
#include "settings.h"
#include "status.h"
#include "ui_state.h"

#define TEST_MESSAGE_CAP 8

static struct TestState {
    char messages[TEST_MESSAGE_CAP][256];
    int32 message_count;
    int32 sort_calls;
    int32 refresh_calls;
    int32 captured_song_count;
    int32 captured_getter_count;
    enum NcmSongGetter captured_getters[16];
    uint32 captured_start_position;
    bool captured_ignore_leading_the;
    bool sort_success;
    bool refresh_success;
    bool mpd_connected;
    char drawn_label[256];
    int32 draw_count;
} test_state;

static void test_state_reset(void);
static void test_add_song(NativePlaylistScreen *playlist, char *uri,
                          int32 uri_len, uint32 position);
static void test_select(NativePlaylistScreen *playlist, int64 position);
static void test_clear_selection(NativePlaylistScreen *playlist);
static void test_dialog_rows_render_labels(
    NativeSortPlaylistDialog *dialog);
static void test_dialog_entry_and_cancel(NativePlaylistScreen *playlist,
                                         NativeSortPlaylistDialog *dialog,
                                         NcmMpdClient *client);
static void test_dialog_sort_success(NativePlaylistScreen *playlist,
                                     NativeSortPlaylistDialog *dialog,
                                     NcmMpdClient *client);
static void test_dialog_sort_errors(NativePlaylistScreen *playlist,
                                    NativeSortPlaylistDialog *dialog,
                                    NcmMpdClient *client);
static void test_dialog_rejects_invalid_entry(
    NativePlaylistScreen *playlist, NativeSortPlaylistDialog *dialog,
    NcmMpdClient *client);
static void test_replaces_legacy_registration(void);
static void test_action_routing(void);

bool
__wrap_ncm_mpd_client_connected(NcmMpdClient *client) {
    (void)client;
    return test_state.mpd_connected;
}

bool
__wrap_ncm_playlist_sort_range(
    NcmSongArray *songs, uint32 start_position,
    enum NcmSongGetter *getters, int32 getters_len,
    bool ignore_leading_the, NcmMpdClient *client,
    NcmError *error) {
    (void)client;
    test_state.sort_calls += 1;
    test_state.captured_song_count = songs->len;
    test_state.captured_start_position = start_position;
    test_state.captured_ignore_leading_the = ignore_leading_the;
    test_state.captured_getter_count = getters_len;
    for (int32 i = 0; i < getters_len; i += 1) {
        test_state.captured_getters[i] = getters[i];
    }

    if (!test_state.sort_success) {
        ncm_error_set(error, EIO, STRLIT_ARGS("sort failure"));
        return false;
    }
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_status_update_full(NcmMpdClient *client,
                              NcmStatusHooks *hooks,
                              NcmError *error) {
    (void)client;
    (void)hooks;
    test_state.refresh_calls += 1;
    if (!test_state.refresh_success) {
        ncm_error_set(error, EIO, STRLIT_ARGS("refresh failure"));
        return false;
    }
    ncm_error_clear(error);
    return true;
}

void
__wrap_nc_window_init(NcWindow *window, int64 start_x, int64 start_y,
                      int64 width, int64 height, char *title,
                      int32 title_len, NcColor color, NcBorder border) {
    (void)title;
    (void)title_len;
    (void)color;
    (void)border;
    *window = (NcWindow){0};
    window->start_x = start_x;
    window->start_y = start_y;
    window->width = width;
    window->height = height;
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
    return;
}

void
__wrap_nc_window_move_to(NcWindow *window, int64 start_x,
                         int64 start_y) {
    window->start_x = start_x;
    window->start_y = start_y;
    return;
}

void
__wrap_nc_window_display(NcWindow *window) {
    (void)window;
    return;
}

void
__wrap_nc_window_print_data(NcWindow *window, char *string,
                            int32 string_len) {
    (void)window;
    assert(string_len >= 0);
    assert(string_len < (int32)sizeof(test_state.drawn_label));
    ncm_memcpy(test_state.drawn_label, string, string_len);
    test_state.drawn_label[string_len] = '\0';
    test_state.draw_count += 1;
    return;
}

bool
__wrap_nc_window_has_coords(NcWindow *window, int32 *x, int32 *y) {
    (void)window;
    (void)x;
    (void)y;
    return false;
}

void
__wrap_nc_menu_refresh(NcMenu *menu, NcWindow *window,
                       int64 width, int64 height) {
    (void)menu;
    (void)window;
    (void)width;
    (void)height;
    return;
}

void
__wrap_ncm_statusbar_print_cstring(int32 delay_seconds, char *message) {
    int32 message_index;

    (void)delay_seconds;
    message_index = test_state.message_count;
    assert(message_index < TEST_MESSAGE_CAP);
    snprintf(test_state.messages[message_index],
             sizeof(test_state.messages[message_index]), "%s", message);
    test_state.message_count += 1;
    return;
}

int
main(void) {
    NativePlaylistScreen playlist;
    NativeSortPlaylistDialog dialog;
    NcmMpdClient client = {0};
    NcScreenCallbacks callbacks = {0};
    NcColor color = {0};
    NcBorder border = {0};

    app_controller_init();
    ui_state_set_screen_size(100, 40);
    ui_state_set_main_geometry(2, 30);
    playlist = (NativePlaylistScreen){0};
    nc_song_menu_init(&playlist.songs);
    nc_playlist_screen_init(&playlist.screen, callbacks, &playlist,
                            nc_song_menu_base(&playlist.songs),
                            0, 100, 2, 30);
    color.is_default = true;
    border.color = color;
    native_sort_playlist_dialog_init(&dialog, 0, 0, 30, 17,
                                     color, border);
    assert(app_controller_register_screen(
        native_playlist_screen_base(&playlist)));
    assert(app_controller_register_screen(
        native_sort_playlist_dialog_base(&dialog)));
    test_dialog_rows_render_labels(&dialog);

    test_add_song(&playlist, STRLIT_ARGS("zero.flac"), 10);
    test_add_song(&playlist, STRLIT_ARGS("one.flac"), 11);
    test_add_song(&playlist, STRLIT_ARGS("two.flac"), 12);
    test_add_song(&playlist, STRLIT_ARGS("three.flac"), 13);

    test_dialog_rejects_invalid_entry(&playlist, &dialog, &client);
    assert(app_controller_switch_to_screen(
        native_playlist_screen_base(&playlist)));
    test_dialog_entry_and_cancel(&playlist, &dialog, &client);
    test_dialog_sort_success(&playlist, &dialog, &client);
    test_dialog_sort_errors(&playlist, &dialog, &client);
    test_dialog_rejects_invalid_entry(&playlist, &dialog, &client);

    native_sort_playlist_dialog_destroy(&dialog);
    assert(app_controller_unregister_screen(
        native_playlist_screen_base(&playlist)));
    nc_song_menu_destroy(&playlist.songs);

    app_controller_init();
    test_replaces_legacy_registration();
    app_controller_init();
    test_action_routing();
    exit(EXIT_SUCCESS);
}

static void
test_state_reset(void) {
    test_state = (typeof(test_state)){0};
    test_state.sort_success = true;
    test_state.refresh_success = true;
    return;
}

static void
test_add_song(NativePlaylistScreen *playlist, char *uri,
              int32 uri_len, uint32 position) {
    NcmSong song;

    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, uri, uri_len));
    ncm_song_set_position(&song, position);
    nc_song_menu_add(&playlist->songs, &song);
    ncm_song_destroy(&song);
    return;
}

static void
test_select(NativePlaylistScreen *playlist, int64 position) {
    assert(nc_menu_set_position_selected(
        native_playlist_screen_menu(playlist), position, true));
    return;
}

static void
test_clear_selection(NativePlaylistScreen *playlist) {
    nc_menu_clear_selection(native_playlist_screen_menu(playlist));
    return;
}

static void
test_dialog_rows_render_labels(NativeSortPlaylistDialog *dialog) {
    NcEditorSortRow *row;
    NcMenu *menu;

    test_state_reset();
    menu = nc_editor_sort_menu_base(
        native_sort_playlist_dialog_menu(dialog));
    assert(menu->display_callbacks.draw != NULL);

    row = nc_editor_sort_menu_item_at(
        native_sort_playlist_dialog_menu(dialog), NC_MENU_ITEMS_ALL, 0);
    assert(row != NULL);
    menu->display_callbacks.draw(
        menu, &dialog->window, row, 0, menu->display_callbacks.user);
    assert(test_state.draw_count == 1);
    assert(strcmp(test_state.drawn_label, "Artist") == 0);

    row = nc_editor_sort_menu_item_at(
        native_sort_playlist_dialog_menu(dialog), NC_MENU_ITEMS_ALL,
        nc_menu_all_item_count(menu) - 2);
    assert(row != NULL);
    menu->display_callbacks.draw(
        menu, &dialog->window, row, nc_menu_all_item_count(menu) - 2,
        menu->display_callbacks.user);
    assert(test_state.draw_count == 2);
    assert(strcmp(test_state.drawn_label, "Sort") == 0);
    return;
}

static void
test_dialog_entry_and_cancel(NativePlaylistScreen *playlist,
                             NativeSortPlaylistDialog *dialog,
                             NcmMpdClient *client) {
    enum NcmSongGetter getters[16];
    NcmError error;
    NcMenu *menu;
    int32 getter_count;

    test_state_reset();
    menu = nc_editor_sort_menu_base(
        native_sort_playlist_dialog_menu(dialog));
    nc_menu_highlight_position(menu, 0, nc_menu_item_count(menu));
    assert(native_sort_playlist_dialog_move_current_down(dialog));

    ncm_error_clear(&error);
    assert(native_sort_playlist_dialog_open(
        dialog, playlist, client, false, &error));
    assert(app_controller_current_screen()
           == native_sort_playlist_dialog_base(dialog));
    assert(dialog->previous_screen == native_playlist_screen_base(playlist));
    assert(dialog->songs.len == 4);
    assert(dialog->start_position == 10);
    assert(dialog->start_x == 35);
    assert(dialog->start_y == 8);
    assert(dialog->width == 30);
    assert(dialog->height == 17);

    getter_count = native_sort_playlist_dialog_get_order(
        dialog, getters, NCM_ARRAY_LEN(getters));
    assert(getter_count == 11);
    assert(getters[0] == NCM_SONG_GETTER_ARTIST);
    assert(getters[1] == NCM_SONG_GETTER_ALBUM_ARTIST);

    menu = nc_editor_sort_menu_base(
        native_sort_playlist_dialog_menu(dialog));
    nc_menu_highlight_position(menu, 0, nc_menu_item_count(menu));
    assert(native_sort_playlist_dialog_run_current(dialog));
    assert(app_controller_current_screen()
           == native_sort_playlist_dialog_base(dialog));
    assert(test_state.sort_calls == 0);
    assert(test_state.refresh_calls == 0);
    assert(test_state.message_count == 1);
    assert(strcmp(test_state.messages[0],
                  "Move tag types up and down to adjust sort order") == 0);

    nc_menu_highlight_position(menu, nc_menu_item_count(menu) - 1,
                               nc_menu_item_count(menu));
    assert(native_sort_playlist_dialog_run_current(dialog));
    assert(app_controller_current_screen()
           == native_playlist_screen_base(playlist));
    assert(test_state.sort_calls == 0);
    assert(test_state.refresh_calls == 0);
    assert(dialog->songs.len == 0);
    assert(!dialog->ready);
    return;
}

static void
test_dialog_sort_success(NativePlaylistScreen *playlist,
                         NativeSortPlaylistDialog *dialog,
                         NcmMpdClient *client) {
    NcmError error;
    NcMenu *menu;

    test_state_reset();
    test_clear_selection(playlist);
    test_select(playlist, 1);
    test_select(playlist, 2);
    ncm_error_clear(&error);
    assert(native_sort_playlist_dialog_open(
        dialog, playlist, client, true, &error));
    assert(dialog->songs.len == 2);
    assert(dialog->start_position == 11);

    menu = nc_editor_sort_menu_base(
        native_sort_playlist_dialog_menu(dialog));
    nc_menu_highlight_position(menu, nc_menu_item_count(menu) - 2,
                               nc_menu_item_count(menu));
    assert(native_sort_playlist_dialog_run_current(dialog));
    assert(test_state.sort_calls == 1);
    assert(test_state.refresh_calls == 1);
    assert(test_state.captured_song_count == 2);
    assert(test_state.captured_start_position == 11);
    assert(test_state.captured_ignore_leading_the);
    assert(test_state.captured_getter_count == 11);
    assert(test_state.captured_getters[0] == NCM_SONG_GETTER_ARTIST);
    assert(test_state.message_count == 2);
    assert(strcmp(test_state.messages[0], "Sorting...") == 0);
    assert(strcmp(test_state.messages[1], "Range sorted") == 0);
    assert(app_controller_current_screen()
           == native_playlist_screen_base(playlist));
    return;
}

static void
test_dialog_sort_errors(NativePlaylistScreen *playlist,
                        NativeSortPlaylistDialog *dialog,
                        NcmMpdClient *client) {
    NcmError error;
    NcMenu *menu;

    test_state_reset();
    test_state.sort_success = false;
    test_clear_selection(playlist);
    ncm_error_clear(&error);
    assert(native_sort_playlist_dialog_open(
        dialog, playlist, client, false, &error));
    menu = nc_editor_sort_menu_base(
        native_sort_playlist_dialog_menu(dialog));
    nc_menu_highlight_position(menu, nc_menu_item_count(menu) - 2,
                               nc_menu_item_count(menu));
    assert(native_sort_playlist_dialog_run_current(dialog));
    assert(test_state.sort_calls == 1);
    assert(test_state.refresh_calls == 0);
    assert(test_state.message_count == 2);
    assert(strcmp(test_state.messages[1], "sort failure") == 0);
    assert(app_controller_current_screen()
           == native_playlist_screen_base(playlist));

    test_state_reset();
    test_state.refresh_success = false;
    ncm_error_clear(&error);
    assert(native_sort_playlist_dialog_open(
        dialog, playlist, client, false, &error));
    menu = nc_editor_sort_menu_base(
        native_sort_playlist_dialog_menu(dialog));
    nc_menu_highlight_position(menu, nc_menu_item_count(menu) - 2,
                               nc_menu_item_count(menu));
    assert(native_sort_playlist_dialog_run_current(dialog));
    assert(test_state.sort_calls == 1);
    assert(test_state.refresh_calls == 1);
    assert(test_state.message_count == 2);
    assert(strcmp(test_state.messages[1], "refresh failure") == 0);
    assert(app_controller_current_screen()
           == native_playlist_screen_base(playlist));
    return;
}

static void
test_dialog_rejects_invalid_entry(
    NativePlaylistScreen *playlist, NativeSortPlaylistDialog *dialog,
    NcmMpdClient *client) {
    NcmError error;

    ncm_error_clear(&error);
    if (app_controller_current_screen()
        != native_playlist_screen_base(playlist)) {
        assert(!native_sort_playlist_dialog_open(
            dialog, playlist, client, false, &error));
        assert(error.code == EINVAL);
        return;
    }

    test_clear_selection(playlist);
    test_select(playlist, 0);
    test_select(playlist, 2);
    ncm_error_clear(&error);
    assert(!native_sort_playlist_dialog_open(
        dialog, playlist, client, false, &error));
    assert(error.code == EINVAL);
    assert(strcmp(error.message,
                  "selected songs are not contiguous") == 0);
    assert(app_controller_current_screen()
           == native_playlist_screen_base(playlist));
    test_clear_selection(playlist);
    return;
}

static void
test_replaces_legacy_registration(void) {
    NcScreen legacy_screen;
    NcScreenCallbacks callbacks = {0};
    NcScreen *native_screen;

    nc_screen_init(&legacy_screen, callbacks, NULL,
                   NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG);
    assert(app_controller_register_screen(&legacy_screen));
    assert(app_controller_find_screen_type(
               NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG)
           == &legacy_screen);

    native_c_screen_sort_playlist_dialog_register();
    native_screen = native_c_screen_sort_playlist_dialog_native();
    assert(app_controller_find_screen_type(
               NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG)
           == native_screen);
    assert(!app_controller_is_screen_registered(&legacy_screen));
    assert(app_controller_is_screen_registered(native_screen));
    return;
}

static void
test_action_routing(void) {
    enum NcmSongGetter getters[16];
    NativePlaylistScreen *playlist;
    NativeSortPlaylistDialog *dialog;
    NcMenu *menu;
    int32 getter_count;

    test_state_reset();
    playlist = native_c_screen_playlist();
    dialog = native_c_screen_sort_playlist_dialog();
    native_c_screen_playlist_register();
    native_c_screen_sort_playlist_dialog_register();

    test_add_song(playlist, STRLIT_ARGS("zero.flac"), 20);
    test_add_song(playlist, STRLIT_ARGS("one.flac"), 21);
    test_add_song(playlist, STRLIT_ARGS("two.flac"), 22);
    assert(app_controller_switch_to_screen(
        native_playlist_screen_base(playlist)));

    assert(native_playlist_screen_has_sortable_range(playlist));
    assert(!ncm_action_runtime_can_run(NULL, NCM_ACTION_SORT_PLAYLIST));
    test_state.mpd_connected = true;
    assert(!ncm_action_runtime_can_run(NULL, NCM_ACTION_RUN_ACTION));
    assert(ncm_action_runtime_can_run(NULL, NCM_ACTION_SORT_PLAYLIST));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_SORT_PLAYLIST));
    assert(app_controller_current_screen()
           == native_sort_playlist_dialog_base(dialog));

    menu = nc_editor_sort_menu_base(
        native_sort_playlist_dialog_menu(dialog));
    nc_menu_highlight_position(menu, 0, nc_menu_item_count(menu));
    assert(ncm_action_runtime_can_run(NULL, NCM_ACTION_RUN_ACTION));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_RUN_ACTION));
    assert(app_controller_current_screen()
           == native_sort_playlist_dialog_base(dialog));
    assert(test_state.message_count == 1);

    nc_menu_highlight_position(menu, 1, nc_menu_item_count(menu));
    assert(ncm_action_runtime_can_run(
        NULL, NCM_ACTION_MOVE_SORT_ORDER_UP));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_MOVE_SORT_ORDER_UP));
    getter_count = native_sort_playlist_dialog_get_order(
        dialog, getters, NCM_ARRAY_LEN(getters));
    assert(getter_count == 11);
    assert(getters[0] == NCM_SONG_GETTER_ALBUM_ARTIST);
    assert(ncm_action_runtime_can_run(
        NULL, NCM_ACTION_MOVE_SORT_ORDER_DOWN));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_MOVE_SORT_ORDER_DOWN));
    getter_count = native_sort_playlist_dialog_get_order(
        dialog, getters, NCM_ARRAY_LEN(getters));
    assert(getter_count == 11);
    assert(getters[0] == NCM_SONG_GETTER_ARTIST);

    nc_menu_highlight_position(menu, nc_menu_item_count(menu) - 1,
                               nc_menu_item_count(menu));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_RUN_ACTION));
    assert(app_controller_current_screen()
           == native_playlist_screen_base(playlist));
    assert(test_state.sort_calls == 0);
    assert(test_state.refresh_calls == 0);

    assert(ncm_action_runtime_run(NULL, NCM_ACTION_SORT_PLAYLIST));
    menu = nc_editor_sort_menu_base(
        native_sort_playlist_dialog_menu(dialog));
    nc_menu_highlight_position(menu, nc_menu_item_count(menu) - 2,
                               nc_menu_item_count(menu));
    assert(ncm_action_runtime_can_run(NULL, NCM_ACTION_RUN_ACTION));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_RUN_ACTION));
    assert(app_controller_current_screen()
           == native_playlist_screen_base(playlist));
    assert(test_state.sort_calls == 1);
    assert(test_state.refresh_calls == 1);

    test_select(playlist, 0);
    test_select(playlist, 2);
    assert(!native_playlist_screen_has_sortable_range(playlist));
    assert(!ncm_action_runtime_can_run(NULL, NCM_ACTION_SORT_PLAYLIST));
    assert(!ncm_action_runtime_run(NULL, NCM_ACTION_SORT_PLAYLIST));
    assert(app_controller_current_screen()
           == native_playlist_screen_base(playlist));

    native_sort_playlist_dialog_destroy(dialog);
    native_playlist_screen_destroy(playlist);
    return;
}
