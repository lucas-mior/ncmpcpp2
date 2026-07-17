#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "actions.h"
#include "global.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "c/ncm_base.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"
#include "screens/native_c_screens.h"
#include "screens/nc_playlist.h"
#include "screen_actions.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

typedef struct PlaylistActionMove {
    uint32 from;
    uint32 to;
} PlaylistActionMove;

typedef struct PlaylistActionTestState {
    NativePlaylistScreen screen;
    NcWindow prompt_window;

    PlaylistActionMove moves[16];
    PlaylistActionMove swaps[16];
    uint32 added_positions[16];
    int64 search_results[16];
    char prompt_text[128];
    char search_constraint[128];
    char status[256];

    enum ScreenType current_type;
    enum NcmStatusPlayerState player;
    enum NcPromptStatus prompt_status;
    enum mpd_server_error server_error;

    uint32 version;
    uint32 total_time;
    uint32 seek_position;
    uint32 seek_seconds;
    uint32 shuffle_first;
    uint32 shuffle_last;
    uint32 priority;
    uint32 crossfade_seconds;
    uint32 volume_value;
    enum mpd_tag_type random_tag;
    int32 random_count;
    int32 current_song_position;
    int32 move_calls;
    int32 swap_calls;
    int32 command_start_calls;
    int32 command_commit_calls;
    int32 status_update_calls;
    int32 shuffle_calls;
    int32 clear_calls;
    int32 add_calls;
    int32 play_id_calls;
    int32 played_id;
    int32 priority_calls;
    int32 crossfade_calls;
    int32 volume_calls;
    int32 random_tag_calls;
    int32 random_song_calls;
    int32 seek_calls;
    int32 locate_calls;
    int32 located_position;
    int32 save_calls;
    int32 delete_playlist_calls;
    int32 search_result_count;
    int32 search_calls;
    int32 status_calls;
    int32 volume_state;
    char one_of_result;

    bool connected;
    bool command_success;
    bool confirm_result;
    bool locate_result;
    bool save_first_fails_exists;
} PlaylistActionTestState;

static PlaylistActionTestState test_state;

static void test_state_init(void);
static void test_state_destroy(void);
static void test_state_reset(void);
static int32 test_cstring_len(char *text);
static void set_prompt(char *text);
static void add_song(uint32 position);
static NcMenu *playlist_menu(void);
static void select_position(int64 position);
static void highlight_position(int64 position);
static void test_move_selected_items_up_down(void);
static void test_play_item_does_not_append_playlist_song(void);
static void test_move_selected_items_to(void);
static void test_filtered_move_is_rejected(void);
static void test_shuffle_confirmation(void);
static void test_reverse_selected_range(void);
static void test_crop_confirmation_and_range(void);
static void test_clear_confirmation_and_immediate_state(void);
static void test_priority_prompt(void);
static void test_jump_to_position_formats(void);
static void test_autocenter_locates_playing_song(void);
static void test_save_playlist_overwrite(void);
static void test_select_album(void);
static void test_select_found_items(void);
static void test_set_crossfade(void);
static void test_set_volume(void);
static void test_add_random_items(void);

int
main(void) {
    test_state_init();
    test_move_selected_items_up_down();
    test_play_item_does_not_append_playlist_song();
    test_move_selected_items_to();
    test_filtered_move_is_rejected();
    test_shuffle_confirmation();
    test_reverse_selected_range();
    test_crop_confirmation_and_range();
    test_clear_confirmation_and_immediate_state();
    test_priority_prompt();
    test_jump_to_position_formats();
    test_autocenter_locates_playing_song();
    test_save_playlist_overwrite();
    test_select_album();
    test_select_found_items();
    test_set_crossfade();
    test_set_volume();
    test_add_random_items();
    test_state_destroy();
    exit(EXIT_SUCCESS);
}

static void
test_state_init(void) {
    test_state = (PlaylistActionTestState){0};
    native_playlist_screen_init(&test_state.screen, 0, 80, 0, 24,
                                nc_color_default(), nc_border_none());
    test_state.current_type = NCM_SCREEN_TYPE_PLAYLIST;
    test_state.player = NCM_STATUS_PLAYER_PLAY;
    test_state.prompt_status = NC_PROMPT_ACCEPTED;
    test_state.version = 17;
    test_state.total_time = 400;
    test_state.current_song_position = 2;
    test_state.connected = true;
    test_state.command_success = true;
    test_state.confirm_result = true;
    test_state.locate_result = true;
    test_state.volume_state = 50;
    test_state.one_of_result = 's';
    Config.ask_before_clearing_playlists = false;
    Config.ask_before_shuffling_playlists = false;
    Config.autocenter_mode = false;
    return;
}

static void
test_state_destroy(void) {
    native_playlist_screen_destroy(&test_state.screen);
    return;
}

static void
test_state_reset(void) {
    native_playlist_screen_clear(&test_state.screen);
    nc_menu_show_all_items(playlist_menu());
    test_state.move_calls = 0;
    test_state.swap_calls = 0;
    test_state.command_start_calls = 0;
    test_state.command_commit_calls = 0;
    test_state.status_update_calls = 0;
    test_state.shuffle_calls = 0;
    test_state.clear_calls = 0;
    test_state.add_calls = 0;
    test_state.play_id_calls = 0;
    test_state.played_id = -1;
    test_state.priority_calls = 0;
    test_state.crossfade_calls = 0;
    test_state.volume_calls = 0;
    test_state.random_tag_calls = 0;
    test_state.random_song_calls = 0;
    test_state.seek_calls = 0;
    test_state.locate_calls = 0;
    test_state.save_calls = 0;
    test_state.delete_playlist_calls = 0;
    test_state.search_result_count = 0;
    test_state.search_calls = 0;
    test_state.status_calls = 0;
    test_state.search_constraint[0] = '\0';
    test_state.status[0] = '\0';
    test_state.command_success = true;
    test_state.confirm_result = true;
    test_state.locate_result = true;
    test_state.save_first_fails_exists = false;
    test_state.volume_state = 50;
    test_state.one_of_result = 's';
    test_state.server_error = (enum mpd_server_error)0;
    test_state.version = 17;
    test_state.connected = true;
    test_state.player = NCM_STATUS_PLAYER_PLAY;
    test_state.total_time = 400;
    test_state.current_song_position = 2;
    test_state.prompt_status = NC_PROMPT_ACCEPTED;
    Config.ask_before_clearing_playlists = false;
    Config.ask_before_shuffling_playlists = false;
    Config.autocenter_mode = false;
    Config.space_add_mode = NCM_SPACE_ADD_MODE_ADD_REMOVE;
    Config.crossfade_time = 5;
    Config.random_exclude_pattern = "";
    Config.random_exclude_pattern_len = 0;
    for (uint32 i = 0; i < 6; i += 1) {
        add_song(i);
    }
    return;
}

static void
test_play_item_does_not_append_playlist_song(void) {
    test_state_reset();
    Config.space_add_mode = NCM_SPACE_ADD_MODE_ALWAYS_ADD;
    highlight_position(3);

    assert(ncm_action_runtime_can_run(NULL, NCM_ACTION_PLAY_ITEM));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_PLAY_ITEM));
    assert(test_state.add_calls == 0);
    assert(test_state.play_id_calls == 1);
    assert(test_state.played_id == 13);
    return;
}

static int32
test_cstring_len(char *text) {
    int32 len;

    len = 0;
    while (text[len] != '\0') {
        len += 1;
    }
    return len;
}

static void
set_prompt(char *text) {
    int32 len;

    len = test_cstring_len(text);
    assert(len < (int32)sizeof(test_state.prompt_text));
    memcpy64(test_state.prompt_text, text, len + 1);
    return;
}

static void
add_song(uint32 position) {
    NcmSong song;
    char *album;
    int32 album_len;
    char uri[32];
    int32 uri_len;

    uri_len = snprintf(uri, sizeof(uri), "song-%u.flac", position);
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, uri, uri_len));
    ncm_song_set_position(&song, position);
    ncm_song_set_id(&song, position + 10);
    if (position < 2) {
        album = "album-a";
        album_len = 7;
    } else if (position < 5) {
        album = "album-b";
        album_len = 7;
    } else {
        album = "album-c";
        album_len = 7;
    }
    assert(ncm_song_add_tag(&song, MPD_TAG_ALBUM,
                            album, album_len));
    assert(native_playlist_screen_add_song_copy(&test_state.screen,
                                                &song));
    ncm_song_destroy(&song);
    return;
}

static NcMenu *
playlist_menu(void) {
    return native_playlist_screen_menu(&test_state.screen);
}

static void
select_position(int64 position) {
    assert(nc_menu_set_position_selected(playlist_menu(), position, true));
    return;
}

static void
highlight_position(int64 position) {
    nc_menu_highlight_position(playlist_menu(), position, 24);
    return;
}

static void
test_move_selected_items_up_down(void) {
    test_state_reset();
    select_position(1);
    select_position(2);

    assert(ncm_action_runtime_can_run(
        NULL, NCM_ACTION_MOVE_SELECTED_ITEMS_UP));
    assert(ncm_action_runtime_run(NULL,
                                  NCM_ACTION_MOVE_SELECTED_ITEMS_UP));
    assert(test_state.command_start_calls == 1);
    assert(test_state.command_commit_calls == 1);
    assert(test_state.swap_calls == 2);
    assert(test_state.swaps[0].from == 1);
    assert(test_state.swaps[0].to == 0);
    assert(test_state.swaps[1].from == 2);
    assert(test_state.swaps[1].to == 1);

    test_state_reset();
    select_position(3);
    select_position(4);
    assert(ncm_action_runtime_run(NULL,
                                  NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN));
    assert(test_state.command_start_calls == 1);
    assert(test_state.command_commit_calls == 1);
    assert(test_state.swap_calls == 2);
    assert(test_state.swaps[0].from == 4);
    assert(test_state.swaps[0].to == 5);
    assert(test_state.swaps[1].from == 3);
    assert(test_state.swaps[1].to == 4);
    return;
}

static void
test_move_selected_items_to(void) {
    test_state_reset();
    select_position(1);
    select_position(2);
    highlight_position(4);

    assert(ncm_action_runtime_can_run(
        NULL, NCM_ACTION_MOVE_SELECTED_ITEMS_TO));
    assert(ncm_action_runtime_run(NULL,
                                  NCM_ACTION_MOVE_SELECTED_ITEMS_TO));
    assert(test_state.command_start_calls == 1);
    assert(test_state.command_commit_calls == 1);
    assert(test_state.move_calls == 2);
    assert(test_state.moves[0].from == 2);
    assert(test_state.moves[0].to == 3);
    assert(test_state.moves[1].from == 1);
    assert(test_state.moves[1].to == 2);

    test_state_reset();
    select_position(3);
    select_position(4);
    highlight_position(0);
    assert(ncm_action_runtime_run(NULL,
                                  NCM_ACTION_MOVE_SELECTED_ITEMS_TO));
    assert(test_state.move_calls == 2);
    assert(test_state.moves[0].from == 3);
    assert(test_state.moves[0].to == 0);
    assert(test_state.moves[1].from == 4);
    assert(test_state.moves[1].to == 1);

    test_state_reset();
    select_position(1);
    select_position(3);
    highlight_position(2);
    assert(ncm_action_runtime_run(NULL,
                                  NCM_ACTION_MOVE_SELECTED_ITEMS_TO));
    assert(test_state.move_calls == 0);
    assert(test_state.command_start_calls == 0);
    return;
}

static void
test_filtered_move_is_rejected(void) {
    NcMenu *menu;

    test_state_reset();
    menu = playlist_menu();
    for (int64 i = 0; i < nc_menu_all_item_count(menu); i += 1) {
        nc_menu_add_filtered_item_ref(
            menu, nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, i));
    }
    nc_menu_show_filtered_items(menu);
    select_position(2);

    assert(ncm_action_runtime_run(NULL,
                                  NCM_ACTION_MOVE_SELECTED_ITEMS_UP));
    assert(test_state.move_calls == 0);
    assert(strstr(test_state.status, "disabled in filtered") != NULL);
    return;
}

static void
test_shuffle_confirmation(void) {
    test_state_reset();
    select_position(1);
    select_position(2);
    Config.ask_before_shuffling_playlists = true;
    test_state.confirm_result = false;

    assert(ncm_action_runtime_run(NULL, NCM_ACTION_SHUFFLE));
    assert(test_state.shuffle_calls == 0);

    test_state.confirm_result = true;
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_SHUFFLE));
    assert(test_state.shuffle_calls == 1);
    assert(test_state.shuffle_first == 1);
    assert(test_state.shuffle_last == 3);
    return;
}


static void
test_reverse_selected_range(void) {
    NcMenu *menu;

    test_state_reset();
    select_position(1);
    select_position(2);
    select_position(3);

    assert(ncm_action_runtime_can_run(NULL,
                                      NCM_ACTION_REVERSE_PLAYLIST));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_REVERSE_PLAYLIST));
    assert(test_state.command_start_calls == 1);
    assert(test_state.command_commit_calls == 1);
    assert(test_state.swap_calls == 1);
    assert(test_state.swaps[0].from == 1);
    assert(test_state.swaps[0].to == 3);
    assert(strstr(test_state.status, "Range reversed") != NULL);

    test_state_reset();
    menu = playlist_menu();
    for (int64 i = 0; i < nc_menu_all_item_count(menu); i += 2) {
        nc_menu_add_filtered_item_ref(
            menu, nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, i));
    }
    nc_menu_show_filtered_items(menu);
    assert(ncm_action_runtime_can_run(NULL,
                                      NCM_ACTION_REVERSE_PLAYLIST));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_REVERSE_PLAYLIST));
    assert(test_state.swap_calls == 1);
    assert(test_state.swaps[0].from == 0);
    assert(test_state.swaps[0].to == 4);
    return;
}

static void
test_crop_confirmation_and_range(void) {
    test_state_reset();
    select_position(1);
    select_position(2);
    Config.ask_before_clearing_playlists = true;
    test_state.confirm_result = false;

    assert(ncm_action_runtime_can_run(NULL,
                                      NCM_ACTION_CROP_MAIN_PLAYLIST));
    assert(ncm_action_runtime_run(NULL,
                                  NCM_ACTION_CROP_MAIN_PLAYLIST));
    assert(test_state.clear_calls == 0);
    assert(test_state.add_calls == 0);

    test_state.confirm_result = true;
    assert(ncm_action_runtime_run(NULL,
                                  NCM_ACTION_CROP_MAIN_PLAYLIST));
    assert(test_state.clear_calls == 1);
    assert(test_state.add_calls == 2);
    assert(test_state.added_positions[0] == 1);
    assert(test_state.added_positions[1] == 2);
    assert(strstr(test_state.status, "Playlist cropped") != NULL);
    return;
}

static void
test_clear_confirmation_and_immediate_state(void) {
    test_state_reset();
    Config.ask_before_clearing_playlists = true;
    test_state.confirm_result = false;

    assert(ncm_action_runtime_run(NULL, NCM_ACTION_CLEAR_MAIN_PLAYLIST));
    assert(test_state.clear_calls == 0);
    assert(native_playlist_screen_song_count(&test_state.screen) == 6);

    test_state.confirm_result = true;
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_CLEAR_MAIN_PLAYLIST));
    assert(test_state.clear_calls == 1);
    assert(native_playlist_screen_empty(&test_state.screen));
    return;
}

static void
test_priority_prompt(void) {
    test_state_reset();
    select_position(1);
    set_prompt("200");

    assert(ncm_action_runtime_can_run(
        NULL, NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY));
    assert(ncm_action_runtime_run(
        NULL, NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY));
    assert(test_state.priority_calls == 1);
    assert(test_state.priority == 200);

    set_prompt("256");
    assert(ncm_action_runtime_run(
        NULL, NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY));
    assert(test_state.priority_calls == 1);
    assert(strstr(test_state.status, "between 0 and 255") != NULL);

    test_state.version = 16;
    assert(!ncm_action_runtime_can_run(
        NULL, NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY));
    assert(strstr(test_state.status, "MPD >= 0.17.0") != NULL);
    return;
}

static void
test_jump_to_position_formats(void) {
    test_state_reset();
    set_prompt("1:02");
    assert(ncm_action_runtime_run(NULL,
                                  NCM_ACTION_JUMP_TO_POSITION_IN_SONG));
    assert(test_state.seek_calls == 1);
    assert(test_state.seek_position == 2);
    assert(test_state.seek_seconds == 62);

    set_prompt("25");
    assert(ncm_action_runtime_run(NULL,
                                  NCM_ACTION_JUMP_TO_POSITION_IN_SONG));
    assert(test_state.seek_calls == 2);
    assert(test_state.seek_seconds == 100);

    set_prompt("1:60");
    assert(ncm_action_runtime_run(NULL,
                                  NCM_ACTION_JUMP_TO_POSITION_IN_SONG));
    assert(test_state.seek_calls == 3);
    assert(test_state.seek_seconds == 120);

    set_prompt("1:61");
    assert(ncm_action_runtime_run(NULL,
                                  NCM_ACTION_JUMP_TO_POSITION_IN_SONG));
    assert(test_state.seek_calls == 3);
    assert(strstr(test_state.status, "Invalid format") != NULL);
    return;
}

static void
test_autocenter_locates_playing_song(void) {
    test_state_reset();
    test_state.current_song_position = 4;

    assert(ncm_action_runtime_run(
        NULL, NCM_ACTION_TOGGLE_PLAYING_SONG_CENTERING));
    assert(Config.autocenter_mode);
    assert(test_state.locate_calls == 1);
    assert(test_state.located_position == 4);

    test_state_reset();
    test_state.current_song_position = 3;
    test_state.locate_result = false;
    assert(ncm_action_runtime_can_run(
        NULL, NCM_ACTION_JUMP_TO_PLAYING_SONG));
    assert(ncm_action_runtime_run(NULL,
                                  NCM_ACTION_JUMP_TO_PLAYING_SONG));
    assert(strstr(test_state.status, "Song is filtered out") != NULL);
    return;
}

static void
test_save_playlist_overwrite(void) {
    test_state_reset();
    set_prompt("mix");
    test_state.save_first_fails_exists = true;
    test_state.confirm_result = true;

    assert(ncm_action_runtime_run(NULL, NCM_ACTION_SAVE_PLAYLIST));
    assert(test_state.save_calls == 2);
    assert(test_state.delete_playlist_calls == 1);
    assert(strstr(test_state.status, "overwritten") != NULL);
    return;
}

static void
test_select_album(void) {
    NcMenu *menu;

    test_state_reset();
    menu = playlist_menu();
    highlight_position(3);

    assert(ncm_action_runtime_can_run(NULL, NCM_ACTION_SELECT_ALBUM));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_SELECT_ALBUM));
    assert(!nc_menu_position_is_selected(menu, 1));
    assert(nc_menu_position_is_selected(menu, 2));
    assert(nc_menu_position_is_selected(menu, 3));
    assert(nc_menu_position_is_selected(menu, 4));
    assert(!nc_menu_position_is_selected(menu, 5));
    assert(strstr(test_state.status,
                  "Album around cursor position selected") != NULL);
    return;
}

static void
test_select_found_items(void) {
    NcMenu *menu;

    test_state_reset();
    menu = playlist_menu();
    highlight_position(3);
    memcpy64(test_state.search_constraint, "song", 5);
    test_state.search_results[0] = 1;
    test_state.search_results[1] = 4;
    test_state.search_result_count = 2;

    assert(ncm_action_runtime_can_run(NULL,
                                      NCM_ACTION_SELECT_FOUND_ITEMS));
    assert(ncm_action_runtime_run(NULL,
                                  NCM_ACTION_SELECT_FOUND_ITEMS));
    assert(test_state.search_calls == 3);
    assert(nc_menu_position_is_selected(menu, 1));
    assert(nc_menu_position_is_selected(menu, 4));
    assert(!nc_menu_position_is_selected(menu, 0));
    assert(nc_menu_highlight(menu) == 3);
    assert(strstr(test_state.status, "Found items selected") != NULL);
    return;
}

static void
test_set_crossfade(void) {
    test_state_reset();
    set_prompt("12");

    assert(ncm_action_runtime_can_run(NULL, NCM_ACTION_SET_CROSSFADE));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_SET_CROSSFADE));
    assert(test_state.crossfade_calls == 1);
    assert(test_state.crossfade_seconds == 12);
    assert(Config.crossfade_time == 12);

    set_prompt("invalid");
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_SET_CROSSFADE));
    assert(test_state.crossfade_calls == 1);
    assert(strstr(test_state.status, "non-negative") != NULL);
    return;
}

static void
test_set_volume(void) {
    test_state_reset();
    set_prompt("75");

    assert(ncm_action_runtime_can_run(NULL, NCM_ACTION_SET_VOLUME));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_SET_VOLUME));
    assert(test_state.volume_calls == 1);
    assert(test_state.volume_value == 75);

    set_prompt("101");
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_SET_VOLUME));
    assert(test_state.volume_calls == 1);
    assert(strstr(test_state.status, "between 0 and 100") != NULL);

    test_state.volume_state = -1;
    assert(!ncm_action_runtime_can_run(NULL, NCM_ACTION_SET_VOLUME));
    return;
}

static void
test_add_random_items(void) {
    test_state_reset();
    test_state.one_of_result = 's';
    set_prompt("3");

    assert(ncm_action_runtime_can_run(NULL, NCM_ACTION_ADD_RANDOM_ITEMS));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_ADD_RANDOM_ITEMS));
    assert(test_state.random_song_calls == 1);
    assert(test_state.random_tag_calls == 0);
    assert(test_state.random_count == 3);

    test_state.one_of_result = 'A';
    set_prompt("2");
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_ADD_RANDOM_ITEMS));
    assert(test_state.random_tag_calls == 1);
    assert(test_state.random_tag == MPD_TAG_ALBUM_ARTIST);
    assert(test_state.random_count == 2);

    test_state.one_of_result = 'b';
    set_prompt("0");
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_ADD_RANDOM_ITEMS));
    assert(test_state.random_tag_calls == 1);
    return;
}

NcScreen *
__wrap_app_controller_current_screen(void) {
    return native_playlist_screen_base(&test_state.screen);
}

enum ScreenType
__wrap_native_c_screens_current_type(void) {
    return test_state.current_type;
}

NativePlaylistScreen *
__wrap_native_c_screen_playlist(void) {
    return &test_state.screen;
}

NcmStringView
__wrap_current_screen_current_search_constraint(void) {
    int32 len;

    len = test_cstring_len(test_state.search_constraint);
    return ncm_string_view_make(test_state.search_constraint, len);
}

bool
__wrap_current_screen_search(enum SearchDirection direction,
                             char *pattern, int32 pattern_len,
                             bool wrap, bool skip_current,
                             NcmError *error) {
    int64 result;

    (void)pattern;
    (void)pattern_len;
    (void)skip_current;
    assert(direction == NCM_SEARCH_DIRECTION_FORWARD);
    assert(!wrap);
    ncm_error_clear(error);
    test_state.search_calls += 1;
    if (test_state.search_calls > test_state.search_result_count) {
        return false;
    }
    result = test_state.search_results[test_state.search_calls - 1];
    nc_menu_highlight_position(playlist_menu(), result, 24);
    return true;
}

bool
__wrap_ncm_mpd_client_connected(NcmMpdClient *client) {
    (void)client;
    return test_state.connected;
}

uint32
__wrap_ncm_mpd_client_version(NcmMpdClient *client) {
    (void)client;
    return test_state.version;
}

bool
__wrap_ncm_mpd_client_start_command_list(NcmMpdClient *client,
                                         NcmError *error) {
    test_state.command_start_calls += 1;
    ncm_error_clear(error);
    client->command_list_active = test_state.command_success;
    return test_state.command_success;
}

bool
__wrap_ncm_mpd_client_commit_command_list(NcmMpdClient *client,
                                          NcmError *error) {
    test_state.command_commit_calls += 1;
    ncm_error_clear(error);
    client->command_list_active = false;
    return test_state.command_success;
}

bool
__wrap_ncm_mpd_client_move(NcmMpdClient *client, uint32 from,
                           uint32 to, NcmError *error) {
    (void)client;
    assert(test_state.move_calls < LENGTH(test_state.moves));
    test_state.moves[test_state.move_calls].from = from;
    test_state.moves[test_state.move_calls].to = to;
    test_state.move_calls += 1;
    ncm_error_clear(error);
    return test_state.command_success;
}


bool
__wrap_ncm_mpd_client_swap(NcmMpdClient *client, uint32 from,
                           uint32 to, NcmError *error) {
    (void)client;
    assert(test_state.swap_calls < LENGTH(test_state.swaps));
    test_state.swaps[test_state.swap_calls].from = from;
    test_state.swaps[test_state.swap_calls].to = to;
    test_state.swap_calls += 1;
    ncm_error_clear(error);
    return test_state.command_success;
}

bool
__wrap_ncm_mpd_client_shuffle_range(NcmMpdClient *client,
                                    uint32 first, uint32 last,
                                    NcmError *error) {
    (void)client;
    test_state.shuffle_calls += 1;
    test_state.shuffle_first = first;
    test_state.shuffle_last = last;
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_clear_queue(NcmMpdClient *client,
                                  NcmError *error) {
    (void)client;
    test_state.clear_calls += 1;
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_add_song_value(NcmMpdClient *client,
                                     NcmSong *song, int32 position,
                                     int32 *id, NcmError *error) {
    (void)client;
    (void)position;
    (void)id;
    assert(test_state.add_calls < LENGTH(test_state.added_positions));
    test_state.added_positions[test_state.add_calls] =
        ncm_song_position(song);
    test_state.add_calls += 1;
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_play_id(NcmMpdClient *client, int32 id,
                              NcmError *error) {
    (void)client;
    test_state.play_id_calls += 1;
    test_state.played_id = id;
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_set_priority_song(NcmMpdClient *client,
                                        NcmSong *song,
                                        int32 priority,
                                        NcmError *error) {
    (void)client;
    (void)song;
    test_state.priority_calls += 1;
    test_state.priority = (uint32)priority;
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_set_crossfade(NcmMpdClient *client,
                                     uint32 seconds,
                                     NcmError *error) {
    (void)client;
    test_state.crossfade_calls += 1;
    test_state.crossfade_seconds = seconds;
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_set_volume(NcmMpdClient *client, uint32 volume,
                                  NcmError *error) {
    (void)client;
    test_state.volume_calls += 1;
    test_state.volume_value = volume;
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_add_random_tag(NcmMpdClient *client,
                                     enum mpd_tag_type tag,
                                     int32 number, NcmRandom *random,
                                     NcmError *error) {
    (void)client;
    (void)random;
    test_state.random_tag_calls += 1;
    test_state.random_tag = tag;
    test_state.random_count = number;
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_add_random_songs(NcmMpdClient *client,
                                       int32 number,
                                       char *exclude_pattern,
                                       int32 exclude_pattern_len,
                                       NcmRandom *random,
                                       NcmError *error) {
    (void)client;
    (void)exclude_pattern;
    (void)exclude_pattern_len;
    (void)random;
    test_state.random_song_calls += 1;
    test_state.random_count = number;
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_seek_pos(NcmMpdClient *client, uint32 position,
                               uint32 seconds, NcmError *error) {
    (void)client;
    test_state.seek_calls += 1;
    test_state.seek_position = position;
    test_state.seek_seconds = seconds;
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_save_playlist(NcmMpdClient *client, char *name,
                                    NcmError *error) {
    (void)client;
    (void)name;
    test_state.save_calls += 1;
    ncm_error_clear(error);
    if (test_state.save_first_fails_exists
        && (test_state.save_calls == 1)) {
        test_state.server_error = MPD_SERVER_ERROR_EXIST;
        ncm_error_set(error, MPD_ERROR_SERVER,
                      LIT_ARGS("playlist exists"));
        return false;
    }
    test_state.server_error = (enum mpd_server_error)0;
    return true;
}

bool
__wrap_ncm_mpd_client_delete_playlist(NcmMpdClient *client,
                                      char *name, NcmError *error) {
    (void)client;
    (void)name;
    test_state.delete_playlist_calls += 1;
    ncm_error_clear(error);
    return true;
}

enum mpd_server_error
__wrap_ncm_mpd_client_server_error_code(NcmMpdClient *client) {
    (void)client;
    return test_state.server_error;
}

bool
__wrap_ncm_status_update_full(NcmMpdClient *client,
                              NcmStatusHooks *hooks,
                              NcmError *error) {
    (void)client;
    (void)hooks;
    test_state.status_update_calls += 1;
    ncm_error_clear(error);
    return true;
}

int32
__wrap_ncm_status_state_current_song_position(void) {
    return test_state.current_song_position;
}

uint32
__wrap_ncm_status_state_playlist_length(void) {
    return (uint32)native_playlist_screen_song_count(&test_state.screen);
}

int32
__wrap_ncm_status_state_volume(void) {
    return test_state.volume_state;
}

enum NcmStatusPlayerState
__wrap_ncm_status_state_player(void) {
    return test_state.player;
}

uint32
__wrap_ncm_status_state_total_time(void) {
    return test_state.total_time;
}

bool
__wrap_native_playlist_screen_locate_position(
    NativePlaylistScreen *screen, uint32 position) {
    (void)screen;
    test_state.locate_calls += 1;
    test_state.located_position = (int32)position;
    return test_state.locate_result;
}

void
__wrap_ncm_statusbar_scoped_lock_init(NcmStatusbarScopedLock *lock) {
    if (lock != NULL) {
        *lock = (NcmStatusbarScopedLock){0};
    }
    return;
}

void
__wrap_ncm_statusbar_scoped_lock_destroy(NcmStatusbarScopedLock *lock) {
    (void)lock;
    return;
}

NcWindow *
__wrap_ncm_statusbar_put(void) {
    return &test_state.prompt_window;
}

void
__wrap_ncm_statusbar_print(int32 delay, char *message,
                           int32 message_len) {
    (void)delay;
    if (message_len >= (int32)sizeof(test_state.status)) {
        message_len = (int32)sizeof(test_state.status) - 1;
    }
    memcpy64(test_state.status, message, message_len);
    test_state.status[message_len] = '\0';
    test_state.status_calls += 1;
    return;
}

void
__wrap_ncm_statusbar_print_cstring(int32 delay, char *message) {
    __wrap_ncm_statusbar_print(delay, message,
                               test_cstring_len(message));
    return;
}

void
__wrap_ncm_statusbar_format(int32 delay, char *format,
                            int32 format_len,
                            NcmStringFormatArg *args,
                            int32 args_len) {
    (void)args;
    (void)args_len;
    __wrap_ncm_statusbar_print(delay, format, format_len);
    return;
}

bool
__wrap_ncm_statusbar_prompt_return_one_of(NcWindow *window,
                                          char *values,
                                          int32 values_len,
                                          char *result) {
    (void)window;
    if ((values_len == 4) && (values[0] == 's')) {
        if (test_state.one_of_result == 0) {
            return false;
        }
        *result = test_state.one_of_result;
        return true;
    }
    if (!test_state.confirm_result) {
        *result = 'n';
        return true;
    }
    *result = 'y';
    return true;
}

enum NcPromptStatus
__wrap_nc_window_prompt(NcWindow *window, NcPrompt *prompt,
                        char **result) {
    int32 len;

    (void)window;
    (void)prompt;
    *result = NULL;
    if (test_state.prompt_status != NC_PROMPT_ACCEPTED) {
        return test_state.prompt_status;
    }
    len = test_cstring_len(test_state.prompt_text);
    *result = malloc2(len + 1);
    memcpy64(*result, test_state.prompt_text, len + 1);
    return NC_PROMPT_ACCEPTED;
}

void
__wrap_nc_window_prompt_result_destroy(char *result) {
    int32 len;

    if (result == NULL) {
        return;
    }
    len = test_cstring_len(result);
    free2(result, len + 1);
    return;
}

void
__wrap_nc_window_print_data(NcWindow *window, char *data,
                            int32 data_len) {
    (void)window;
    (void)data;
    (void)data_len;
    return;
}

void
__wrap_nc_window_init(NcWindow *window, int64 start_x, int64 start_y,
                      int64 width, int64 height, char *title,
                      int32 title_len, NcColor color, NcBorder border) {
    (void)color;
    *window = (NcWindow){0};
    window->start_x = start_x;
    window->start_y = start_y;
    window->width = width;
    window->height = height;
    window->title = title;
    window->title_len = title_len;
    window->border = border;
    return;
}

void
__wrap_nc_window_destroy(NcWindow *window) {
    *window = (NcWindow){0};
    return;
}

void
__wrap_nc_window_set_title(NcWindow *window, char *title,
                           int32 title_len) {
    window->title = title;
    window->title_len = title_len;
    return;
}
