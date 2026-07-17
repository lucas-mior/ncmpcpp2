#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include "app_controller.h"
#include "c/ncm_app_arrays.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"
#include "screens/nc_browser.h"
#include "screens/nc_playlist.h"
#include "screens/nc_playlist_editor.h"
#include "screens/nc_sel_items_adder.h"
#include "screens/nc_sort_playlist.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "ui_state.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)
#define TEST_EVENT_CAP 8
#define TEST_TEXT_CAP 256
#define TEST_STORED_SONG_CAP 4
#define TEST_QUEUE_SONG_CAP 8

static struct TestState {
    NcmMpdPlaylistList playlists;
    NcmSongArray queue_songs;
    NcmSong highlighted_song;
    NcWindow status_window;
    NcWindow *displayed_windows[TEST_EVENT_CAP];
    NcWindow *refreshed_windows[TEST_EVENT_CAP];
    NcMenu *refreshed_menus[TEST_EVENT_CAP];
    char added_playlists[TEST_STORED_SONG_CAP][TEST_TEXT_CAP];
    char added_uris[TEST_STORED_SONG_CAP][TEST_TEXT_CAP];
    char queue_added_uris[TEST_QUEUE_SONG_CAP][TEST_TEXT_CAP];
    int32 queue_added_positions[TEST_QUEUE_SONG_CAP];
    char status_error[TEST_TEXT_CAP];
    char status_playlist[TEST_TEXT_CAP];
    char printed_status[TEST_TEXT_CAP];
    char queue_status[TEST_TEXT_CAP];
    char server_error[TEST_TEXT_CAP];
    char prompt_prefix[TEST_TEXT_CAP];
    char prompt_input[TEST_TEXT_CAP];
    char drawn_label[TEST_TEXT_CAP];
    int32 fetch_calls;
    int32 status_error_len;
    int32 status_playlist_len;
    int32 printed_status_len;
    int32 queue_status_len;
    int32 server_error_len;
    int32 prompt_prefix_len;
    int32 prompt_input_len;
    int32 drawn_label_len;
    int32 status_calls;
    int32 status_success_calls;
    int32 status_print_calls;
    int32 display_calls;
    int32 refresh_calls;
    int32 draw_calls;
    int32 command_start_calls;
    int32 command_add_calls;
    int32 command_commit_calls;
    int32 command_add_failure_at;
    int32 queue_add_calls;
    int32 queue_add_client_failure_at;
    int32 server_error_calls;
    int32 current_song_position;
    uint32 queue_add_failure_mask;
    enum NcmStatusPlayerState player_state;
    int32 status_lock_calls;
    int32 status_unlock_calls;
    int32 status_put_calls;
    int32 prompt_calls;
    int32 prompt_destroy_calls;
    enum NcPromptStatus prompt_status;
    bool fetch_success;
    bool command_start_success;
    bool command_commit_success;
    bool highlighted_available;
} test_state;

static void test_playlist_editor_command_preparation(void);
static void test_sort_playlist_row_movement(void);
static void test_selected_items_lifecycle_and_stored_operations(void);
static void test_selected_items_current_playlist_operations(void);
static void test_state_init(void);
static void test_state_destroy(void);
static void test_state_reset_events(void);
static void test_state_reset_stored_playlist_events(void);
static void test_state_reset_current_playlist_events(void);
static void test_state_set_prompt(enum NcPromptStatus status,
                                  char *input, int32 input_len);
static void test_state_add_playlist(char *name, int32 name_len);
static void test_state_add_queue_song(char *uri, int32 uri_len,
                                      uint32 position, char *album,
                                      int32 album_len);
static int32 test_cstring_len(char *string);
static void test_copy_text(char *dest, int32 *dest_len,
                           char *source, int32 source_len);
static NcEditorActionRow *test_adder_row(
    NativeSelectedItemsAdderScreen *screen, int64 pos);
static void test_assert_adder_label(
    NativeSelectedItemsAdderScreen *screen, int64 pos,
    char *label, int32 label_len);
static void test_assert_adder_open(
    NativeSelectedItemsAdderScreen *screen, NcScreen *previous);
static void test_assert_adder_closed(
    NativeSelectedItemsAdderScreen *screen, NcScreen *previous);
static void test_cancel_adder(
    NativeSelectedItemsAdderScreen *screen, NcScreen *previous);
static void test_open_current_playlist(
    NativeSelectedItemsAdderScreen *screen, NativePlaylistScreen *playlist,
    NcmMpdClient *client, NcmSongArray *songs);
static void test_assert_queue_add(int32 call, char *uri, int32 uri_len,
                                  int32 position);

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

bool
__wrap_ncm_mpd_client_start_command_list(NcmMpdClient *client,
                                         NcmError *error) {
    test_state.command_start_calls += 1;
    if (!test_state.command_start_success) {
        client->command_list_active = true;
        ncm_error_set(error, EIO, STRLIT_ARGS("command start failure"));
        return false;
    }

    client->command_list_active = true;
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_add_song_to_playlist(NcmMpdClient *client,
                                           char *playlist,
                                           NcmSong *song,
                                           NcmError *error) {
    int32 call;
    int32 playlist_len;

    assert(client->command_list_active);
    assert(song != NULL);
    assert(song->uri != NULL);
    call = test_state.command_add_calls;
    assert(call < TEST_STORED_SONG_CAP);
    playlist_len = test_cstring_len(playlist);
    test_copy_text(test_state.added_playlists[call], NULL,
                   playlist, playlist_len);
    test_copy_text(test_state.added_uris[call], NULL,
                   song->uri, song->uri_len);
    test_state.command_add_calls += 1;
    if (call == test_state.command_add_failure_at) {
        ncm_error_set(error, EIO, STRLIT_ARGS("command add failure"));
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_add_song_value(NcmMpdClient *client,
                                     NcmSong *song, int32 position,
                                     int32 *id, NcmError *error) {
    int32 call;

    (void)client;
    (void)id;
    assert(song);
    assert(song->uri);
    call = test_state.queue_add_calls;
    assert(call < TEST_QUEUE_SONG_CAP);
    test_copy_text(test_state.queue_added_uris[call], NULL,
                   song->uri, song->uri_len);
    test_state.queue_added_positions[call] = position;
    test_state.queue_add_calls += 1;

    if (call == test_state.queue_add_client_failure_at) {
        ncm_error_set(error, EIO, STRLIT_ARGS("queue client failure"));
        return false;
    }
    if (test_state.queue_add_failure_mask & (1u << call)) {
        ncm_error_set(error, MPD_ERROR_SERVER,
                      STRLIT_ARGS("queue add failure"));
        return false;
    }

    ncm_error_clear(error);
    return true;
}

enum NcmStatusPlayerState
__wrap_ncm_status_state_player(void) {
    return test_state.player_state;
}

int32
__wrap_ncm_status_state_current_song_position(void) {
    return test_state.current_song_position;
}

bool
__wrap_native_playlist_screen_current_song(NativePlaylistScreen *screen,
                                            NcmSong *song) {
    (void)screen;
    if (!test_state.highlighted_available) {
        return false;
    }
    return ncm_song_copy(song, &test_state.highlighted_song);
}

bool
__wrap_native_playlist_screen_now_playing_song(
    NativePlaylistScreen *screen, int32 position, NcmSong *song) {
    (void)screen;
    for (int32 i = 0; i < test_state.queue_songs.len; i += 1) {
        if (ncm_song_position(&test_state.queue_songs.items[i])
            == (uint32)position) {
            return ncm_song_copy(
                song, &test_state.queue_songs.items[i]);
        }
    }
    return false;
}

void
__wrap_ncm_status_handle_server_error_value(
    NcmMpdClient *client, int32 code, char *message,
    int32 message_len) {
    (void)client;
    (void)code;
    test_copy_text(test_state.server_error,
                   &test_state.server_error_len, message, message_len);
    test_state.server_error_calls += 1;
    return;
}

bool
__wrap_ncm_mpd_client_commit_command_list(NcmMpdClient *client,
                                          NcmError *error) {
    assert(client->command_list_active);
    test_state.command_commit_calls += 1;
    if (!test_state.command_commit_success) {
        ncm_error_set(error, EIO, STRLIT_ARGS("command commit failure"));
        return false;
    }

    client->command_list_active = false;
    ncm_error_clear(error);
    return true;
}

void
__wrap_ncm_statusbar_scoped_lock_init(NcmStatusbarScopedLock *lock) {
    if (lock) {
        *lock = (NcmStatusbarScopedLock){0};
    }
    test_state.status_lock_calls += 1;
    return;
}

void
__wrap_ncm_statusbar_scoped_lock_destroy(NcmStatusbarScopedLock *lock) {
    (void)lock;
    test_state.status_unlock_calls += 1;
    return;
}

NcWindow *
__wrap_ncm_statusbar_put(void) {
    test_state.status_put_calls += 1;
    return &test_state.status_window;
}

void
__wrap_ncm_statusbar_print(int32 delay_seconds, char *message,
                           int32 message_len) {
    assert(delay_seconds == (int32)Config.message_delay_time);
    test_copy_text(test_state.queue_status,
                   &test_state.queue_status_len, message, message_len);
    return;
}

void
__wrap_ncm_statusbar_print_cstring(int32 delay_seconds, char *message) {
    int32 message_len;

    assert(delay_seconds == (int32)Config.message_delay_time);
    message_len = test_cstring_len(message);
    test_copy_text(test_state.printed_status,
                   &test_state.printed_status_len,
                   message, message_len);
    test_state.status_print_calls += 1;
    return;
}

void
__wrap_ncm_statusbar_format(int32 delay_seconds, char *format,
                            int32 format_len, NcmStringFormatArg *args,
                            int32 args_len) {
    NcmStringView message;

    assert(delay_seconds == (int32)Config.message_delay_time);
    assert(args != NULL);
    assert(args_len == 1);
    assert(args[0].type == NCM_STRING_FORMAT_ARG_STRING);
    message = args[0].value.string;
    if (ncm_string_equal(
            format, format_len,
            STRLIT_ARGS("Could not fetch playlists: %1"))) {
        test_copy_text(test_state.status_error,
                       &test_state.status_error_len,
                       message.data, message.len);
        test_state.status_calls += 1;
    } else {
        assert(ncm_string_equal(
            format, format_len,
            STRLIT_ARGS(
                "Selected item(s) added to playlist \"%1\"")));
        test_copy_text(test_state.status_playlist,
                       &test_state.status_playlist_len,
                       message.data, message.len);
        test_state.status_success_calls += 1;
    }
    return;
}

enum NcPromptStatus
__wrap_nc_window_prompt(NcWindow *window, NcPrompt *prompt,
                        char **result) {
    assert(window == &test_state.status_window);
    assert(prompt != NULL);
    assert(result != NULL);
    assert(ncm_string_equal(prompt->initial_text,
                            test_cstring_len(prompt->initial_text),
                            STRLIT_ARGS("")));
    assert(prompt->width == -1);
    assert(prompt->hook != NULL);
    assert(prompt->hook_user_data == NULL);
    assert(!prompt->encrypted);
    assert(prompt->remember);

    test_state.prompt_calls += 1;
    *result = NULL;
    if (test_state.prompt_status == NC_PROMPT_ACCEPTED) {
        *result = malloc2(test_state.prompt_input_len + 1);
        if (test_state.prompt_input_len > 0) {
            memcpy64(*result, test_state.prompt_input,
                   test_state.prompt_input_len);
        }
        (*result)[test_state.prompt_input_len] = '\0';
    }
    return test_state.prompt_status;
}

void
__wrap_nc_window_prompt_result_destroy(char *result) {
    int32 result_len;

    test_state.prompt_destroy_calls += 1;
    if (result == NULL) {
        return;
    }
    result_len = test_cstring_len(result);
    free2(result, result_len + 1);
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
    assert(string_len >= 0);
    if (window == &test_state.status_window) {
        test_copy_text(test_state.prompt_prefix,
                       &test_state.prompt_prefix_len,
                       string, string_len);
        return;
    }
    assert(string_len < (int32)SIZEOF(test_state.drawn_label));
    memcpy64(test_state.drawn_label, string, string_len);
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
    test_selected_items_lifecycle_and_stored_operations();
    test_selected_items_current_playlist_operations();
    test_state_destroy();
    return EXIT_SUCCESS;
}

static void
test_state_init(void) {
    test_state = (struct TestState){0};
    ncm_mpd_playlist_list_init(&test_state.playlists);
    ncm_song_array_init(&test_state.queue_songs);
    ncm_song_init(&test_state.highlighted_song);
    test_state.fetch_success = true;
    test_state.command_start_success = true;
    test_state.command_commit_success = true;
    test_state.command_add_failure_at = -1;
    test_state.prompt_status = NC_PROMPT_ABORTED;
    test_state.player_state = NCM_STATUS_PLAYER_PLAY;
    test_state.queue_add_client_failure_at = -1;
    return;
}

static void
test_state_destroy(void) {
    ncm_song_destroy(&test_state.highlighted_song);
    ncm_song_array_destroy(&test_state.queue_songs);
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
    test_state.status_playlist[0] = '\0';
    test_state.printed_status[0] = '\0';
    test_state.prompt_prefix[0] = '\0';
    test_state.drawn_label[0] = '\0';
    test_state.status_error_len = 0;
    test_state.status_playlist_len = 0;
    test_state.printed_status_len = 0;
    test_state.prompt_prefix_len = 0;
    test_state.drawn_label_len = 0;
    test_state.status_calls = 0;
    test_state.status_success_calls = 0;
    test_state.status_print_calls = 0;
    test_state.display_calls = 0;
    test_state.refresh_calls = 0;
    test_state.draw_calls = 0;
    return;
}

static void
test_state_reset_stored_playlist_events(void) {
    for (int32 i = 0; i < TEST_STORED_SONG_CAP; i += 1) {
        test_state.added_playlists[i][0] = '\0';
        test_state.added_uris[i][0] = '\0';
    }
    test_state.status_playlist[0] = '\0';
    test_state.printed_status[0] = '\0';
    test_state.prompt_prefix[0] = '\0';
    test_state.status_playlist_len = 0;
    test_state.printed_status_len = 0;
    test_state.prompt_prefix_len = 0;
    test_state.status_success_calls = 0;
    test_state.status_print_calls = 0;
    test_state.command_start_calls = 0;
    test_state.command_add_calls = 0;
    test_state.command_commit_calls = 0;
    test_state.command_add_failure_at = -1;
    test_state.status_lock_calls = 0;
    test_state.status_unlock_calls = 0;
    test_state.status_put_calls = 0;
    test_state.prompt_calls = 0;
    test_state.prompt_destroy_calls = 0;
    test_state.command_start_success = true;
    test_state.command_commit_success = true;
    test_state.prompt_status = NC_PROMPT_ABORTED;
    return;
}

static void
test_state_reset_current_playlist_events(void) {
    for (int32 i = 0; i < TEST_QUEUE_SONG_CAP; i += 1) {
        test_state.queue_added_uris[i][0] = '\0';
        test_state.queue_added_positions[i] = 0;
    }
    test_state.queue_status[0] = '\0';
    test_state.server_error[0] = '\0';
    test_state.printed_status[0] = '\0';
    test_state.queue_status_len = 0;
    test_state.server_error_len = 0;
    test_state.printed_status_len = 0;
    test_state.queue_add_calls = 0;
    test_state.queue_add_client_failure_at = -1;
    test_state.server_error_calls = 0;
    test_state.status_print_calls = 0;
    test_state.queue_add_failure_mask = 0;
    test_state.current_song_position = 0;
    test_state.player_state = NCM_STATUS_PLAYER_PLAY;
    test_state.highlighted_available = false;
    return;
}

static void
test_state_set_prompt(enum NcPromptStatus status, char *input,
                      int32 input_len) {
    assert(input_len >= 0);
    assert(input_len < (int32)SIZEOF(test_state.prompt_input));
    test_state.prompt_status = status;
    test_state.prompt_input_len = input_len;
    if (input_len > 0) {
        assert(input != NULL);
        memcpy64(test_state.prompt_input, input, input_len);
    }
    test_state.prompt_input[input_len] = '\0';
    return;
}

static int32
test_cstring_len(char *string) {
    int32 len;

    if (string == NULL) {
        return 0;
    }
    len = 0;
    while (string[len] != '\0') {
        len += 1;
    }
    return len;
}

static void
test_copy_text(char *dest, int32 *dest_len, char *source,
               int32 source_len) {
    assert(dest != NULL);
    assert(source_len >= 0);
    assert(source_len < TEST_TEXT_CAP);
    if (source_len > 0) {
        assert(source != NULL);
        memcpy64(dest, source, source_len);
    }
    dest[source_len] = '\0';
    if (dest_len) {
        *dest_len = source_len;
    }
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
test_state_add_queue_song(char *uri, int32 uri_len, uint32 position,
                          char *album, int32 album_len) {
    NcmSong song;

    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, uri, uri_len));
    ncm_song_set_position(&song, position);
    if (album) {
        assert(ncm_song_add_tag(&song, MPD_TAG_ALBUM, album, album_len));
    }
    assert(ncm_song_array_append_copy(&test_state.queue_songs, &song));
    ncm_song_destroy(&song);
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
test_assert_adder_open(NativeSelectedItemsAdderScreen *screen,
                       NcScreen *previous) {
    assert(screen->ready);
    assert(screen->previous_screen == previous);
    assert(screen->playlist != NULL);
    assert(screen->client != NULL);
    assert(screen->selected_songs.len > 0);
    assert(app_controller_current_screen()
           == native_selected_items_adder_screen_base(screen));
    return;
}

static void
test_assert_adder_closed(NativeSelectedItemsAdderScreen *screen,
                         NcScreen *previous) {
    assert(!screen->ready);
    assert(screen->selected_songs.len == 0);
    assert(screen->previous_screen == NULL);
    assert(screen->playlist == NULL);
    assert(screen->client == NULL);
    assert(screen->active_menu
           == NATIVE_SELECTED_ITEMS_ADDER_MENU_PLAYLISTS);
    assert(app_controller_current_screen() == previous);
    return;
}

static void
test_cancel_adder(NativeSelectedItemsAdderScreen *screen,
                  NcScreen *previous) {
    NcMenu *menu;

    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_playlist_menu(screen));
    nc_menu_highlight_position(menu, nc_menu_item_count(menu) - 1,
                               nc_menu_item_count(menu));
    assert(native_selected_items_adder_screen_run_current(screen));
    test_assert_adder_closed(screen, previous);
    return;
}

static void
test_open_current_playlist(
    NativeSelectedItemsAdderScreen *screen, NativePlaylistScreen *playlist,
    NcmMpdClient *client, NcmSongArray *songs) {
    NcmError error;
    NcMenu *menu;

    ncm_error_clear(&error);
    assert(native_selected_items_adder_screen_open(
        screen, songs, playlist, client, &error));
    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_playlist_menu(screen));
    nc_menu_highlight_position(menu, 0, nc_menu_item_count(menu));
    assert(native_selected_items_adder_screen_run_current(screen));
    assert(screen->active_menu
           == NATIVE_SELECTED_ITEMS_ADDER_MENU_POSITIONS);
    return;
}

static void
test_assert_queue_add(int32 call, char *uri, int32 uri_len,
                      int32 position) {
    assert(call >= 0);
    assert(call < test_state.queue_add_calls);
    assert(ncm_string_equal(test_state.queue_added_uris[call],
                            test_cstring_len(
                                test_state.queue_added_uris[call]),
                            uri, uri_len));
    assert(test_state.queue_added_positions[call] == position);
    return;
}

static void
test_selected_items_current_playlist_operations(void) {
    NativeSelectedItemsAdderScreen screen;
    NativeBrowserScreen browser;
    NativePlaylistScreen playlist = {0};
    NcmMpdClient client = {0};
    NcmSongArray songs;
    NcmSong song;
    NcMenu *menu;

    app_controller_init();
    nc_buffer_init(&Config.current_item_prefix);
    nc_buffer_init(&Config.current_item_suffix);
    Config.use_cyclic_scrolling = true;
    Config.centered_cursor = true;
    Config.ignore_leading_the = false;
    Config.message_delay_time = 5;
    ui_state_set_screen_size(100, 40);
    ui_state_set_main_geometry(2, 30);

    native_browser_screen_init(&browser, 0, 100, 2, 30,
                               nc_color_default(), nc_border_none());
    native_browser_screen_set_local(&browser, false);
    native_selected_items_adder_screen_init(&screen, 0, 0, 40, 10,
                                            nc_color_default(),
                                            nc_border_none());
    ncm_song_array_init(&songs);
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("first.flac")));
    assert(ncm_song_array_append_copy(&songs, &song));
    assert(ncm_song_set_uri(&song, LIT_ARGS("second.flac")));
    assert(ncm_song_array_append_copy(&songs, &song));
    assert(ncm_song_set_uri(&song, LIT_ARGS("third.flac")));
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);

    assert(app_controller_register_screen(
        native_browser_screen_base(&browser)));
    assert(app_controller_register_screen(
        native_selected_items_adder_screen_base(&screen)));
    assert(app_controller_switch_to_screen(
        native_browser_screen_base(&browser)));

    test_open_current_playlist(&screen, &playlist, &client, &songs);
    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_position_menu(&screen));
    nc_menu_highlight_position(menu, 0, nc_menu_item_count(menu));
    test_state_reset_current_playlist_events();
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.queue_add_calls == 3);
    test_assert_queue_add(0, LIT_ARGS("first.flac"), -1);
    test_assert_queue_add(1, LIT_ARGS("second.flac"), -1);
    test_assert_queue_add(2, LIT_ARGS("third.flac"), -1);
    assert(ncm_string_equal(test_state.queue_status,
                            test_state.queue_status_len,
                            LIT_ARGS("Selected items added")));
    test_assert_adder_closed(&screen, native_browser_screen_base(&browser));

    test_open_current_playlist(&screen, &playlist, &client, &songs);
    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_position_menu(&screen));
    nc_menu_highlight_position(menu, 1, nc_menu_item_count(menu));
    test_state_reset_current_playlist_events();
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.queue_add_calls == 3);
    test_assert_queue_add(0, LIT_ARGS("first.flac"), 0);
    test_assert_queue_add(1, LIT_ARGS("third.flac"), 1);
    test_assert_queue_add(2, LIT_ARGS("second.flac"), 1);
    test_assert_adder_closed(&screen, native_browser_screen_base(&browser));

    test_open_current_playlist(&screen, &playlist, &client, &songs);
    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_position_menu(&screen));
    nc_menu_highlight_position(menu, 0, nc_menu_item_count(menu));
    test_state_reset_current_playlist_events();
    test_state.queue_add_failure_mask = (1u << 0)|(1u << 2);
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.queue_add_calls == 3);
    assert(test_state.server_error_calls == 2);
    assert(ncm_string_equal(test_state.server_error,
                            test_state.server_error_len,
                            LIT_ARGS("queue add failure")));
    assert(ncm_string_equal(
        test_state.queue_status, test_state.queue_status_len,
        LIT_ARGS("Selected items added (with errors)")));
    test_assert_adder_closed(&screen, native_browser_screen_base(&browser));

    test_open_current_playlist(&screen, &playlist, &client, &songs);
    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_position_menu(&screen));
    nc_menu_highlight_position(menu, 0, nc_menu_item_count(menu));
    test_state_reset_current_playlist_events();
    test_state.queue_add_client_failure_at = 1;
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.queue_add_calls == 2);
    assert(test_state.server_error_calls == 0);
    assert(test_state.status_print_calls == 1);
    assert(ncm_string_equal(test_state.printed_status,
                            test_state.printed_status_len,
                            LIT_ARGS("queue client failure")));
    test_assert_adder_open(&screen, native_browser_screen_base(&browser));
    assert(screen.active_menu
           == NATIVE_SELECTED_ITEMS_ADDER_MENU_POSITIONS);
    assert(native_selected_items_adder_screen_return_to_previous(&screen));
    test_assert_adder_closed(&screen, native_browser_screen_base(&browser));

    test_open_current_playlist(&screen, &playlist, &client, &songs);
    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_position_menu(&screen));
    nc_menu_highlight_position(menu, 2, nc_menu_item_count(menu));
    test_state_reset_current_playlist_events();
    test_state.player_state = NCM_STATUS_PLAYER_STOP;
    test_state.current_song_position = 4;
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.queue_add_calls == 0);
    test_assert_adder_open(&screen, native_browser_screen_base(&browser));
    assert(screen.active_menu
           == NATIVE_SELECTED_ITEMS_ADDER_MENU_POSITIONS);

    test_state_reset_current_playlist_events();
    test_state.current_song_position = 4;
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.queue_add_calls == 3);
    test_assert_queue_add(0, LIT_ARGS("first.flac"), 5);
    test_assert_queue_add(1, LIT_ARGS("third.flac"), 6);
    test_assert_queue_add(2, LIT_ARGS("second.flac"), 6);
    test_assert_adder_closed(&screen, native_browser_screen_base(&browser));

    ncm_song_array_clear(&test_state.queue_songs);
    test_state_add_queue_song(LIT_ARGS("playing.flac"), 3,
                              LIT_ARGS("Alpha"));
    test_state_add_queue_song(LIT_ARGS("next.flac"), 4,
                              LIT_ARGS("Alpha"));
    test_state_add_queue_song(LIT_ARGS("other.flac"), 5,
                              LIT_ARGS("Beta"));
    test_open_current_playlist(&screen, &playlist, &client, &songs);
    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_position_menu(&screen));
    nc_menu_highlight_position(menu, 3, nc_menu_item_count(menu));
    test_state_reset_current_playlist_events();
    test_state.player_state = NCM_STATUS_PLAYER_STOP;
    test_state.current_song_position = 3;
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.queue_add_calls == 0);
    test_assert_adder_open(&screen, native_browser_screen_base(&browser));
    assert(screen.active_menu
           == NATIVE_SELECTED_ITEMS_ADDER_MENU_POSITIONS);

    test_state_reset_current_playlist_events();
    test_state.current_song_position = 3;
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.queue_add_calls == 3);
    test_assert_queue_add(0, LIT_ARGS("first.flac"), 5);
    test_assert_queue_add(1, LIT_ARGS("third.flac"), 6);
    test_assert_queue_add(2, LIT_ARGS("second.flac"), 6);
    test_assert_adder_closed(&screen, native_browser_screen_base(&browser));

    ncm_song_array_clear(&test_state.queue_songs);
    test_state_add_queue_song(LIT_ARGS("untagged.flac"), 7, NULL, 0);
    test_state_add_queue_song(LIT_ARGS("also-untagged.flac"), 8,
                              NULL, 0);
    test_state_add_queue_song(LIT_ARGS("tagged.flac"), 9,
                              LIT_ARGS("Beta"));
    test_open_current_playlist(&screen, &playlist, &client, &songs);
    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_position_menu(&screen));
    nc_menu_highlight_position(menu, 3, nc_menu_item_count(menu));
    test_state_reset_current_playlist_events();
    test_state.current_song_position = 7;
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.queue_add_calls == 3);
    test_assert_queue_add(0, LIT_ARGS("first.flac"), 9);
    test_assert_queue_add(1, LIT_ARGS("third.flac"), 10);
    test_assert_queue_add(2, LIT_ARGS("second.flac"), 10);
    test_assert_adder_closed(&screen, native_browser_screen_base(&browser));

    test_open_current_playlist(&screen, &playlist, &client, &songs);
    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_position_menu(&screen));
    nc_menu_highlight_position(menu, 4, nc_menu_item_count(menu));
    test_state_reset_current_playlist_events();
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.queue_add_calls == 0);
    test_assert_adder_open(&screen, native_browser_screen_base(&browser));
    assert(screen.active_menu
           == NATIVE_SELECTED_ITEMS_ADDER_MENU_POSITIONS);

    test_state_reset_current_playlist_events();
    test_state.highlighted_available = true;
    ncm_song_set_position(&test_state.highlighted_song, 8);
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.queue_add_calls == 3);
    test_assert_queue_add(0, LIT_ARGS("first.flac"), 9);
    test_assert_queue_add(1, LIT_ARGS("third.flac"), 10);
    test_assert_queue_add(2, LIT_ARGS("second.flac"), 10);
    test_assert_adder_closed(&screen, native_browser_screen_base(&browser));

    ncm_song_array_destroy(&songs);
    native_selected_items_adder_screen_destroy(&screen);
    assert(app_controller_unregister_screen(
        native_browser_screen_base(&browser)));
    native_browser_screen_destroy(&browser);
    nc_buffer_destroy(&Config.current_item_prefix);
    nc_buffer_destroy(&Config.current_item_suffix);
    return;
}

static void
test_selected_items_lifecycle_and_stored_operations(void) {
    NativeSelectedItemsAdderScreen screen;
    NativeBrowserScreen browser;
    NativePlaylistScreen playlist = {0};
    NcmMpdClient client = {0};
    NcmSongArray copied_songs;
    NcmSongArray songs;
    NcmSong song;
    NcmSong second_song;
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
    native_selected_items_adder_screen_init(&screen, 0, 0, 40, 10,
                                            nc_color_default(),
                                            nc_border_none());
    ncm_song_array_init(&copied_songs);
    ncm_song_array_init(&songs);
    ncm_song_init(&song);
    ncm_song_init(&second_song);

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
    test_assert_adder_open(&screen, native_browser_screen_base(&browser));
    assert(screen.playlist == &playlist);
    assert(screen.client == &client);

    ncm_error_clear(&error);
    assert(!native_selected_items_adder_screen_open(
        &screen, &songs, &playlist, &client, &error));
    assert(error.code == EBUSY);
    test_assert_adder_open(&screen, native_browser_screen_base(&browser));

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

    nc_menu_highlight_position(menu, 0, nc_menu_item_count(menu));
    assert(nc_screen_can_run_current(
        native_selected_items_adder_screen_base(&screen)));
    assert(nc_screen_run_current(
        native_selected_items_adder_screen_base(&screen)));
    assert(screen.active_menu
           == NATIVE_SELECTED_ITEMS_ADDER_MENU_POSITIONS);

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

    test_state_reset_events();
    ui_state_set_screen_size(80, 25);
    ui_state_set_main_geometry(1, 20);
    nc_screen_request_resize(native_browser_screen_base(&browser));
    nc_screen_request_resize(
        native_selected_items_adder_screen_base(&screen));
    nc_screen_resize(native_selected_items_adder_screen_base(&screen));
    assert(!nc_screen_has_to_be_resized(native_browser_screen_base(&browser)));
    assert(nc_window_start_x(native_browser_screen_window(&browser)) == 0);
    assert(nc_window_start_y(native_browser_screen_window(&browser)) == 1);
    assert(nc_window_width(native_browser_screen_window(&browser)) == 80);
    assert(nc_window_height(native_browser_screen_window(&browser)) == 20);
    assert(test_state.refreshed_windows[0]
           == native_browser_screen_window(&browser));
    assert(test_state.refreshed_menus[0]
           == native_browser_screen_menu(&browser));
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
    test_state_reset_current_playlist_events();
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.queue_add_calls == 0);
    assert(screen.active_menu
           == NATIVE_SELECTED_ITEMS_ADDER_MENU_PLAYLISTS);
    test_assert_adder_open(&screen, native_browser_screen_base(&browser));
    assert(native_selected_items_adder_screen_return_to_previous(
        &screen));
    test_assert_adder_closed(&screen, native_browser_screen_base(&browser));
    assert(!native_selected_items_adder_screen_return_to_previous(
        &screen));

    native_browser_screen_set_local(&browser, true);
    test_state_reset_events();
    ncm_error_clear(&error);
    assert(native_selected_items_adder_screen_open(
        &screen, &songs, &playlist, &client, &error));
    assert(test_state.fetch_calls == 1);
    assert(screen.local_browser);
    test_assert_adder_open(&screen, native_browser_screen_base(&browser));
    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_playlist_menu(&screen));
    assert(nc_menu_item_count(menu) == 3);
    test_assert_adder_label(&screen, 0, LIT_ARGS("Current playlist"));
    assert(nc_menu_position_is_separator(menu, 1));
    test_assert_adder_label(&screen, 2, LIT_ARGS("Cancel"));
    test_state_reset_stored_playlist_events();
    test_cancel_adder(&screen, native_browser_screen_base(&browser));
    assert(test_state.command_start_calls == 0);
    assert(test_state.command_add_calls == 0);
    assert(test_state.command_commit_calls == 0);
    assert(test_state.prompt_calls == 0);

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
    test_assert_adder_open(&screen, native_browser_screen_base(&browser));
    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_playlist_menu(&screen));
    assert(nc_menu_item_count(menu) == 4);
    test_assert_adder_label(&screen, 0, LIT_ARGS("Current playlist"));
    test_assert_adder_label(&screen, 1, LIT_ARGS("New playlist"));
    assert(nc_menu_position_is_separator(menu, 2));
    test_assert_adder_label(&screen, 3, LIT_ARGS("Cancel"));
    test_cancel_adder(&screen, native_browser_screen_base(&browser));

    test_state.fetch_success = true;
    assert(ncm_song_set_uri(&second_song, LIT_ARGS("second.flac")));
    assert(ncm_song_array_append_copy(&songs, &second_song));
    ncm_error_clear(&error);
    assert(native_selected_items_adder_screen_open(
        &screen, &songs, &playlist, &client, &error));
    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_playlist_menu(&screen));
    nc_menu_highlight_position(menu, 3, nc_menu_item_count(menu));

    test_state_reset_stored_playlist_events();
    test_state.command_start_success = false;
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.command_start_calls == 1);
    assert(test_state.command_add_calls == 0);
    assert(test_state.command_commit_calls == 0);
    assert(!client.command_list_active);
    assert(test_state.status_print_calls == 1);
    assert(ncm_string_equal(test_state.printed_status,
                            test_state.printed_status_len,
                            LIT_ARGS("command start failure")));
    test_assert_adder_open(&screen, native_browser_screen_base(&browser));

    test_state_reset_stored_playlist_events();
    test_state.command_add_failure_at = 1;
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.command_start_calls == 1);
    assert(test_state.command_add_calls == 2);
    assert(test_state.command_commit_calls == 0);
    assert(!client.command_list_active);
    assert(ncm_string_equal(test_state.added_playlists[0],
                            test_cstring_len(
                                test_state.added_playlists[0]),
                            LIT_ARGS("The Aardvarks")));
    assert(ncm_string_equal(test_state.added_uris[0],
                            test_cstring_len(test_state.added_uris[0]),
                            LIT_ARGS("first.flac")));
    assert(ncm_string_equal(test_state.added_uris[1],
                            test_cstring_len(test_state.added_uris[1]),
                            LIT_ARGS("second.flac")));
    assert(test_state.status_print_calls == 1);
    assert(ncm_string_equal(test_state.printed_status,
                            test_state.printed_status_len,
                            LIT_ARGS("command add failure")));
    test_assert_adder_open(&screen, native_browser_screen_base(&browser));

    test_state_reset_stored_playlist_events();
    test_state.command_commit_success = false;
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.command_start_calls == 1);
    assert(test_state.command_add_calls == 2);
    assert(test_state.command_commit_calls == 1);
    assert(!client.command_list_active);
    assert(test_state.status_print_calls == 1);
    assert(ncm_string_equal(test_state.printed_status,
                            test_state.printed_status_len,
                            LIT_ARGS("command commit failure")));
    test_assert_adder_open(&screen, native_browser_screen_base(&browser));

    test_state_reset_stored_playlist_events();
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.command_start_calls == 1);
    assert(test_state.command_add_calls == 2);
    assert(test_state.command_commit_calls == 1);
    assert(!client.command_list_active);
    assert(test_state.status_success_calls == 1);
    assert(ncm_string_equal(test_state.status_playlist,
                            test_state.status_playlist_len,
                            LIT_ARGS("The Aardvarks")));
    assert(ncm_string_equal(test_state.added_playlists[0],
                            test_cstring_len(
                                test_state.added_playlists[0]),
                            LIT_ARGS("The Aardvarks")));
    assert(ncm_string_equal(test_state.added_uris[0],
                            test_cstring_len(test_state.added_uris[0]),
                            LIT_ARGS("first.flac")));
    assert(ncm_string_equal(test_state.added_uris[1],
                            test_cstring_len(test_state.added_uris[1]),
                            LIT_ARGS("second.flac")));
    test_assert_adder_closed(&screen, native_browser_screen_base(&browser));

    ncm_error_clear(&error);
    assert(native_selected_items_adder_screen_open(
        &screen, &songs, &playlist, &client, &error));
    menu = nc_editor_action_menu_base(
        native_selected_items_adder_screen_playlist_menu(&screen));
    nc_menu_highlight_position(menu, 1, nc_menu_item_count(menu));

    test_state_reset_stored_playlist_events();
    test_state_set_prompt(NC_PROMPT_ABORTED, NULL, 0);
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.status_lock_calls == 1);
    assert(test_state.status_unlock_calls == 1);
    assert(test_state.status_put_calls == 1);
    assert(test_state.prompt_calls == 1);
    assert(test_state.prompt_destroy_calls == 1);
    assert(ncm_string_equal(test_state.prompt_prefix,
                            test_state.prompt_prefix_len,
                            LIT_ARGS("Save playlist as: ")));
    assert(test_state.command_start_calls == 0);
    assert(test_state.status_print_calls == 1);
    assert(ncm_string_equal(test_state.printed_status,
                            test_state.printed_status_len,
                            LIT_ARGS("Action aborted")));
    test_assert_adder_open(&screen, native_browser_screen_base(&browser));
    assert(screen.selected_songs.len == 2);

    test_state_reset_stored_playlist_events();
    test_state_set_prompt(NC_PROMPT_ACCEPTED,
                          LIT_ARGS("Road Trip"));
    test_state.command_add_failure_at = 1;
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.status_lock_calls == 1);
    assert(test_state.status_unlock_calls == 1);
    assert(test_state.prompt_calls == 1);
    assert(test_state.prompt_destroy_calls == 1);
    assert(test_state.command_start_calls == 1);
    assert(test_state.command_add_calls == 2);
    assert(test_state.command_commit_calls == 0);
    assert(!client.command_list_active);
    assert(test_state.status_print_calls == 1);
    assert(ncm_string_equal(test_state.printed_status,
                            test_state.printed_status_len,
                            LIT_ARGS("command add failure")));
    assert(ncm_string_equal(test_state.added_playlists[0],
                            test_cstring_len(
                                test_state.added_playlists[0]),
                            LIT_ARGS("Road Trip")));
    assert(ncm_string_equal(test_state.added_uris[0],
                            test_cstring_len(test_state.added_uris[0]),
                            LIT_ARGS("first.flac")));
    assert(ncm_string_equal(test_state.added_uris[1],
                            test_cstring_len(test_state.added_uris[1]),
                            LIT_ARGS("second.flac")));
    test_assert_adder_open(&screen, native_browser_screen_base(&browser));

    test_state_reset_stored_playlist_events();
    test_state_set_prompt(NC_PROMPT_ACCEPTED,
                          LIT_ARGS("Road Trip"));
    assert(native_selected_items_adder_screen_run_current(&screen));
    assert(test_state.status_lock_calls == 1);
    assert(test_state.status_unlock_calls == 1);
    assert(test_state.prompt_calls == 1);
    assert(test_state.prompt_destroy_calls == 1);
    assert(test_state.command_start_calls == 1);
    assert(test_state.command_add_calls == 2);
    assert(test_state.command_commit_calls == 1);
    assert(!client.command_list_active);
    assert(ncm_string_equal(test_state.added_playlists[0],
                            test_cstring_len(
                                test_state.added_playlists[0]),
                            LIT_ARGS("Road Trip")));
    assert(ncm_string_equal(test_state.added_playlists[1],
                            test_cstring_len(
                                test_state.added_playlists[1]),
                            LIT_ARGS("Road Trip")));
    assert(ncm_string_equal(test_state.added_uris[0],
                            test_cstring_len(test_state.added_uris[0]),
                            LIT_ARGS("first.flac")));
    assert(ncm_string_equal(test_state.added_uris[1],
                            test_cstring_len(test_state.added_uris[1]),
                            LIT_ARGS("second.flac")));
    assert(test_state.status_success_calls == 1);
    assert(ncm_string_equal(test_state.status_playlist,
                            test_state.status_playlist_len,
                            LIT_ARGS("Road Trip")));
    test_assert_adder_closed(&screen, native_browser_screen_base(&browser));

    ncm_song_destroy(&second_song);
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
