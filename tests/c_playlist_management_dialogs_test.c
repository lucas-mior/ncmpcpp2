#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include "app_controller.h"
#include "c/ncm_app_arrays.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "screens/nc_playlist.h"
#include "screens/nc_playlist_editor.h"
#include "screens/nc_sel_items_adder.h"
#include "screens/nc_sort_playlist.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

static void test_playlist_editor_command_preparation(void);
static void test_sort_playlist_row_movement(void);
static void test_selected_items_transfer_and_actions(void);

int
main(void) {
    test_playlist_editor_command_preparation();
    test_sort_playlist_row_movement();
    test_selected_items_transfer_and_actions();
    return EXIT_SUCCESS;
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

static void
test_selected_items_transfer_and_actions(void) {
    NativeSelectedItemsAdderScreen screen;
    NativeSelectedItemsAdderAction *action;
    NativePlaylistScreen playlist = {0};
    NcmMpdClient client = {0};
    NcmMpdPlaylistList playlists;
    NcmPlaylist stored_playlist;
    NcmSongArray songs;
    NcmSong song;
    NcmError error;
    NcScreen previous;
    NcScreenCallbacks callbacks = {0};
    NcMenu *menu;

    app_controller_init();
    nc_screen_init(&previous, callbacks, NULL, NC_SCREEN_TYPE_BROWSER);
    native_selected_items_adder_screen_init(&screen, 0, 0, 40, 10,
                                            nc_color_default(),
                                            nc_border_none());
    ncm_mpd_playlist_list_init(&playlists);
    ncm_playlist_init(&stored_playlist);
    ncm_song_array_init(&songs);
    ncm_song_init(&song);

    assert(app_controller_register_screen(&previous));
    assert(app_controller_register_screen(
        native_selected_items_adder_screen_base(&screen)));
    assert(app_controller_switch_to_screen(&previous));

    assert(ncm_song_set_uri(&song, LIT_ARGS("first.flac")));
    assert(ncm_song_array_append_copy(&songs, &song));
    assert(ncm_playlist_set(&stored_playlist, LIT_ARGS("favorites"), 0));
    assert(ncm_mpd_playlist_list_append_copy(&playlists, &stored_playlist));
    native_selected_items_adder_screen_populate_playlist_selector(
        &screen, &playlists, false);

    ncm_error_clear(&error);
    assert(native_selected_items_adder_screen_open(
        &screen, &songs, &playlist, &client, &error));
    assert(app_controller_current_screen()
           == native_selected_items_adder_screen_base(&screen));
    assert(screen.ready);
    assert(screen.previous_screen == &previous);
    assert(screen.playlist == &playlist);
    assert(screen.client == &client);

    ncm_error_clear(&error);
    assert(!native_selected_items_adder_screen_open(
        &screen, &songs, &playlist, &client, &error));
    assert(error.code == EBUSY);
    assert(app_controller_current_screen()
           == native_selected_items_adder_screen_base(&screen));
    assert(screen.ready);

    ncm_song_array_clear(&songs);
    assert(native_selected_items_adder_screen_selected_songs(&screen,
                                                             &songs));
    assert(songs.len == 1);

    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_playlist_menu(&screen));
    assert(nc_menu_item_count(menu) >= 4);
    nc_menu_highlight_position(menu, 3, nc_menu_item_count(menu));
    assert(native_selected_items_adder_screen_run_current(&screen));
    action = native_selected_items_adder_screen_last_action(&screen);
    assert(action->target
           == NATIVE_SELECTED_ITEMS_ADDER_TARGET_EXISTING_PLAYLIST);
    assert(ncm_string_equal(action->playlist, action->playlist_len,
                            LIT_ARGS("favorites")));

    nc_menu_highlight_position(menu, nc_menu_item_count(menu) - 1,
                               nc_menu_item_count(menu));
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(app_controller_current_screen() == &previous);
    assert(!screen.ready);
    assert(screen.selected_songs.len == 0);
    assert(screen.previous_screen == NULL);
    assert(screen.playlist == NULL);
    assert(screen.client == NULL);

    ncm_error_clear(&error);
    assert(native_selected_items_adder_screen_open(
        &screen, &songs, &playlist, &client, &error));
    action = native_selected_items_adder_screen_last_action(&screen);
    assert(action->target == NATIVE_SELECTED_ITEMS_ADDER_TARGET_NONE);
    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_playlist_menu(&screen));
    nc_menu_highlight_position(menu, nc_menu_item_count(menu) - 1,
                               nc_menu_item_count(menu));
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(app_controller_current_screen() == &previous);
    assert(!screen.ready);
    assert(screen.selected_songs.len == 0);

    ncm_song_destroy(&song);
    ncm_song_array_destroy(&songs);
    ncm_playlist_destroy(&stored_playlist);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_selected_items_adder_screen_destroy(&screen);
    assert(app_controller_unregister_screen(&previous));
    return;
}
