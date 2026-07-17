#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <mpd/client.h>

#include "actions.h"
#include "c/ncm_app_arrays.h"
#include "c/ncm_format.h"
#include "c/ncm_base.h"
#include "c/ncm_fs.h"
#include "c/ncm_string.h"
#include "screens/nc_browser.h"
#include "screens/native_c_screens.h"
#include "settings.h"
#include "statusbar.h"
#include "ui_state.h"
#include "cbase/util.c"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)
#define BROWSER_PARITY_TEST_PENDING(NAME) browser_parity_test_pending(#NAME)

static void test_browser_path_navigation(void);
static void test_browser_parent_directory_compat(void);
static void test_browser_mpd_reload(void);
static void test_browser_native_update_reloads_mpd(void);
static void test_browser_navigation_requests_reload(void);
static void test_browser_native_update_retries_mpd(void);
static void test_browser_native_switch_requests_reload(void);
static void test_browser_selected_songs(void);
static void test_browser_selected_mpd_directory_recursion(void);
static void test_browser_selected_local_directory_recursion(void);
static void test_browser_selected_filtered_current(void);
static void test_browser_delete_rejects_parent(void);
static void test_browser_delete_rejects_disabled_config(void);
static void test_browser_delete_playlist_uses_mpd(void);
static void test_browser_delete_local_directory_requests_reload(void);
static void test_browser_rename_local_directory(void);
static void test_browser_rename_mpd_directory_updates(void);
static void test_browser_rename_playlist_uses_mpd(void);
static void test_browser_action_rename_prompts(void);
static void test_browser_locate_database_song(void);
static void test_browser_locate_local_song(void);
static void test_browser_locate_clears_filter(void);
static void test_browser_locate_missing_directory(void);
static void test_browser_action_jump_to_browser(void);
static void test_browser_filter_and_search(void);
static void test_browser_local_mode(void);
static void test_browser_change_browse_mode(void);
static void test_browser_supported_extensions_fetch(void);
static void test_browser_owned_state(void);
static void test_browser_menu_callbacks(void);
static void test_browser_item_rendering(void);
static void test_browser_item_to_string(void);
static void test_browser_header_title(void);
static void test_browser_column_title(void);
static void test_browser_local_browsing_parity(void);
static void test_browser_directory_expansion_parity(void);
static void test_browser_playlist_expansion_parity(void);
static void test_browser_deletion_parity(void);
static void test_browser_filter_search_formatting_parity(void);
static void test_browser_sort_modes_parity(void);
static void test_browser_jump_to_playing_song_parity(void);
static void browser_parity_test_pending(char *name);

typedef struct BrowserFormatFixture {
    NcmFormatAst old_song_list_format;
    NcmFormatAst old_song_columns_mode_format;
    NcmFormatAst old_browser_sort_format;
    ColumnArray old_columns;
    NcBuffer old_browser_playlist_prefix;
    enum DisplayMode old_browser_display_mode;
    enum SortMode old_browser_sort_mode;
    bool old_discard_colors_if_item_is_selected;
    bool old_ignore_leading_the;
} BrowserFormatFixture;

static void browser_format_fixture_begin(BrowserFormatFixture *fixture);
static void browser_format_fixture_end(BrowserFormatFixture *fixture);
static void browser_test_add_directory(NativeBrowserScreen *screen,
                                       char *path, int32 path_len,
                                       time_t mtime);
static void browser_test_add_playlist(NativeBrowserScreen *screen,
                                      char *path, int32 path_len,
                                      time_t mtime);
static void browser_test_add_song(NativeBrowserScreen *screen,
                                  char *uri, int32 uri_len,
                                  char *name, int32 name_len,
                                  char *artist, int32 artist_len,
                                  time_t mtime);
static void browser_test_assert_item_path(NativeBrowserScreen *screen,
                                          int64 pos,
                                          enum NcmMpdItemKind kind,
                                          char *path, int32 path_len);
static bool browser_test_has_item_path(NativeBrowserScreen *screen,
                                       enum NcmMpdItemKind kind,
                                       char *path, int32 path_len);
static void browser_test_make_path(NcmBuffer *path, char *root,
                                   int32 root_len, char *name,
                                   int32 name_len);
static void browser_test_write_file(char *path);
static void browser_test_remove_path(char *path);
static void browser_action_set_prompt(char *text);
static int32 browser_test_cstrlen32(char *text);


typedef enum BrowserMpdTraceMode {
    BROWSER_MPD_TRACE_ROOT,
    BROWSER_MPD_TRACE_ARTIST,
    BROWSER_MPD_TRACE_GONE,
    BROWSER_MPD_TRACE_FAIL_ONCE,
    BROWSER_MPD_TRACE_LOCATE,
    BROWSER_MPD_TRACE_NO_EXIST,
    BROWSER_MPD_TRACE_RECURSIVE,
} BrowserMpdTraceMode;

typedef enum BrowserMpdDeleteMode {
    BROWSER_MPD_DELETE_SUCCESS,
    BROWSER_MPD_DELETE_NO_EXIST,
    BROWSER_MPD_DELETE_FAIL,
} BrowserMpdDeleteMode;

typedef struct BrowserMpdTrace {
    BrowserMpdTraceMode mode;
    char paths[8][64];
    int32 path_lens[8];
    int32 calls;
} BrowserMpdTrace;

static void browser_mpd_trace_reset(BrowserMpdTraceMode mode);
static void browser_mpd_trace_record_path(char *path);
static void browser_mpd_trace_add_directory(NcmMpdItemArray *items,
                                            char *path, int32 path_len);
static void browser_mpd_trace_add_song(NcmMpdItemArray *items, char *path,
                                       int32 path_len);
static void browser_mpd_trace_append_song(NcmMpdSongList *songs,
                                          char *path, int32 path_len);
static void browser_test_assert_song_uri(NcmSongArray *songs, int32 pos,
                                         char *uri, int32 uri_len);
static bool browser_test_song_array_has_uri(NcmSongArray *songs,
                                            char *uri, int32 uri_len);
bool __wrap_nc_window_has_coords(NcWindow *window, int32 *x, int32 *y);
bool __wrap_ncm_mpd_client_get_directory_entries(
    NcmMpdClient *client, char *path, NcmMpdItemArray *items,
    NcmError *error);
bool __wrap_ncm_mpd_client_get_directory_recursive(
    NcmMpdClient *client, char *path, NcmMpdSongList *songs,
    NcmError *error);
bool __wrap_ncm_mpd_client_get_supported_extensions(
    NcmMpdClient *client, NcmMpdStringList *strings, NcmError *error);
bool __wrap_ncm_mpd_client_delete_playlist(NcmMpdClient *client,
                                           char *name, NcmError *error);
bool __wrap_ncm_mpd_client_rename_playlist(NcmMpdClient *client,
                                           char *from, char *to,
                                           NcmError *error);
bool __wrap_ncm_mpd_client_update_directory(NcmMpdClient *client,
                                            char *path, uint32 *id,
                                            NcmError *error);
NcScreen *__wrap_app_controller_current_screen(void);
enum ScreenType __wrap_native_c_screens_current_type(void);
NativeBrowserScreen *__wrap_native_c_screen_browser(void);
bool __wrap_ncm_mpd_client_connected(NcmMpdClient *client);
void __wrap_ncm_statusbar_scoped_lock_init(NcmStatusbarScopedLock *lock);
void __wrap_ncm_statusbar_scoped_lock_destroy(NcmStatusbarScopedLock *lock);
NcWindow *__wrap_ncm_statusbar_put(void);
void __wrap_ncm_statusbar_print(int32 delay, char *message,
                                int32 message_len);
void __wrap_ncm_statusbar_print_cstring(int32 delay, char *message);
void __wrap_nc_window_print_data(NcWindow *window, char *data,
                                 int32 data_len);
enum NcPromptStatus __wrap_nc_window_prompt(NcWindow *window,
                                            NcPrompt *prompt,
                                            char **result);
void __wrap_nc_window_prompt_result_destroy(char *result);

static BrowserMpdTrace mpd_trace;
static int32 supported_extensions_calls;
static BrowserMpdDeleteMode delete_playlist_mode;
static char delete_playlist_path[128];
static char rename_playlist_from[128];
static char rename_playlist_to[128];
static char update_directory_path[128];
static char browser_action_prompt_text[128];
static char browser_action_status[256];
static int32 delete_playlist_calls;
static int32 rename_playlist_calls;
static int32 update_directory_calls;
static int32 browser_action_status_calls;
static NativeBrowserScreen *browser_action_screen;
static NcWindow browser_action_prompt_window;
static bool browser_action_connected;

int
main(void) {
    test_browser_path_navigation();
    test_browser_parent_directory_compat();
    test_browser_mpd_reload();
    test_browser_native_update_reloads_mpd();
    test_browser_navigation_requests_reload();
    test_browser_native_update_retries_mpd();
    test_browser_native_switch_requests_reload();
    test_browser_selected_songs();
    test_browser_selected_mpd_directory_recursion();
    test_browser_selected_local_directory_recursion();
    test_browser_selected_filtered_current();
    test_browser_delete_rejects_parent();
    test_browser_delete_rejects_disabled_config();
    test_browser_delete_playlist_uses_mpd();
    test_browser_delete_local_directory_requests_reload();
    test_browser_rename_local_directory();
    test_browser_rename_mpd_directory_updates();
    test_browser_rename_playlist_uses_mpd();
    test_browser_action_rename_prompts();
    test_browser_locate_database_song();
    test_browser_locate_local_song();
    test_browser_locate_clears_filter();
    test_browser_locate_missing_directory();
    test_browser_action_jump_to_browser();
    test_browser_filter_and_search();
    test_browser_local_mode();
    test_browser_change_browse_mode();
    test_browser_supported_extensions_fetch();
    test_browser_owned_state();
    test_browser_menu_callbacks();
    test_browser_item_rendering();
    test_browser_item_to_string();
    test_browser_header_title();
    test_browser_column_title();
    test_browser_local_browsing_parity();
    test_browser_directory_expansion_parity();
    test_browser_playlist_expansion_parity();
    test_browser_deletion_parity();
    test_browser_filter_search_formatting_parity();
    test_browser_sort_modes_parity();
    test_browser_jump_to_playing_song_parity();
    return EXIT_SUCCESS;
}

static void
test_browser_path_navigation(void) {
    NativeBrowserScreen screen;
    NcmMpdItem item;
    NcmDirectory directory;
    NcmStringView view;

    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());
    ncm_mpd_item_init(&item);
    ncm_directory_init(&directory);

    assert(native_browser_screen_in_root_directory(&screen));
    assert(ncm_directory_set(&directory, LIT_ARGS("artist/album"), 0));
    assert(ncm_mpd_item_set_directory(&item, &directory));
    assert(native_browser_screen_add_item_copy(&screen, &item));
    assert(native_browser_screen_enter_directory(&screen));
    view = native_browser_screen_current_directory(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("artist/album")));

    assert(native_browser_screen_go_to_parent(&screen));
    view = native_browser_screen_current_directory(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("artist")));

    ncm_directory_destroy(&directory);
    ncm_mpd_item_destroy(&item);
    native_browser_screen_destroy(&screen);
    return;
}

static void
test_browser_parent_directory_compat(void) {
    NativeBrowserScreen screen;
    NcmMpdItem item;
    NcmDirectory directory;
    NcmStringView view;

    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());
    ncm_mpd_item_init(&item);
    ncm_directory_init(&directory);

    assert(native_browser_screen_in_root_directory(&screen));
    assert(native_browser_screen_set_current_directory(&screen,
                                                       LIT_ARGS("/")));
    assert(native_browser_screen_in_root_directory(&screen));
    assert(!native_browser_screen_go_to_parent(&screen));

    assert(ncm_directory_set(&directory, LIT_ARGS("artist/album/.."),
                             0));
    assert(ncm_mpd_item_set_directory(&item, &directory));
    assert(native_browser_screen_item_is_parent(&item));
    assert(native_browser_screen_add_item_copy(&screen, &item));
    assert(native_browser_screen_enter_directory(&screen));
    view = native_browser_screen_current_directory(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("artist")));

    native_browser_screen_clear(&screen);
    assert(ncm_directory_set(&directory, LIT_ARGS(".."), 0));
    assert(ncm_mpd_item_set_directory(&item, &directory));
    assert(native_browser_screen_item_is_parent(&item));
    assert(native_browser_screen_add_item_copy(&screen, &item));
    assert(native_browser_screen_enter_directory(&screen));
    view = native_browser_screen_current_directory(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("/")));
    assert(native_browser_screen_in_root_directory(&screen));

    native_browser_screen_clear(&screen);
    assert(ncm_directory_set(&directory, LIT_ARGS("artist/..hidden"),
                             0));
    assert(ncm_mpd_item_set_directory(&item, &directory));
    assert(!native_browser_screen_item_is_parent(&item));

    ncm_directory_destroy(&directory);
    ncm_mpd_item_destroy(&item);
    native_browser_screen_destroy(&screen);
    return;
}

static void
test_browser_mpd_reload(void) {
    NativeBrowserScreen screen;
    BrowserFormatFixture fixture;
    NcmMpdClient client = {0};
    NcmStringView view;
    NcmError error = {0};
    NcMenu *menu;

    browser_format_fixture_begin(&fixture);
    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());
    menu = native_browser_screen_menu(&screen);

    browser_mpd_trace_reset(BROWSER_MPD_TRACE_ROOT);
    assert(native_browser_screen_reload_from_mpd(&screen, &client,
                                                 &error));
    assert(mpd_trace.calls == 1);
    assert(STREQUAL(mpd_trace.paths[0], mpd_trace.path_lens[0],
                            LIT_ARGS("/")));
    view = native_browser_screen_current_directory(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("/")));
    assert(nc_menu_all_item_count(menu) == 1);
    assert(nc_menu_item_count(menu) == 1);
    assert(!native_browser_screen_item_is_parent(
               nc_menu_active_item_at(menu, 0)));

    browser_mpd_trace_reset(BROWSER_MPD_TRACE_ARTIST);
    assert(native_browser_screen_set_current_directory(
        &screen, LIT_ARGS("artist")));
    assert(native_browser_screen_set_last_highlighted_directory(
        &screen, LIT_ARGS("artist/album")));
    assert(native_browser_screen_reload_from_mpd(&screen, &client,
                                                 &error));
    assert(nc_menu_all_item_count(menu) == 4);
    assert(nc_menu_item_count(menu) == 4);
    assert(native_browser_screen_item_is_parent(
               nc_menu_active_item_at(menu, 0)));
    assert(ncm_directory_path_view(ncm_mpd_item_directory(
               nc_menu_active_item_at(menu, 0)), &view));
    assert(STREQUAL(view.data, view.len, LIT_ARGS("artist/..")));
    assert(nc_menu_highlight(menu) == 1);

    browser_mpd_trace_reset(BROWSER_MPD_TRACE_ARTIST);
    assert(native_browser_screen_apply_filter(&screen, LIT_ARGS("keep"),
                                              &error));
    assert(native_browser_screen_reload_from_mpd(&screen, &client,
                                                 &error));
    assert(nc_menu_is_filtered(menu));
    assert(nc_menu_all_item_count(menu) == 4);
    assert(nc_menu_item_count(menu) == 2);
    assert(native_browser_screen_item_is_parent(
               nc_menu_active_item_at(menu, 0)));
    native_browser_screen_clear_filter(&screen);

    browser_mpd_trace_reset(BROWSER_MPD_TRACE_GONE);
    assert(native_browser_screen_set_current_directory(
        &screen, LIT_ARGS("gone/child")));
    assert(native_browser_screen_reload_from_mpd(&screen, &client,
                                                 &error));
    assert(mpd_trace.calls == 2);
    assert(STREQUAL(mpd_trace.paths[0], mpd_trace.path_lens[0],
                            LIT_ARGS("gone/child")));
    assert(STREQUAL(mpd_trace.paths[1], mpd_trace.path_lens[1],
                            LIT_ARGS("gone")));
    view = native_browser_screen_current_directory(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("gone")));
    assert(nc_menu_all_item_count(menu) == 2);
    assert(nc_menu_highlight(menu) == 1);

    native_browser_screen_destroy(&screen);
    browser_format_fixture_end(&fixture);
    return;
}

static void
test_browser_native_update_reloads_mpd(void) {
    NativeBrowserScreen screen;
    BrowserFormatFixture fixture;
    NcMenu *menu;

    browser_format_fixture_begin(&fixture);
    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    menu = native_browser_screen_menu(&screen);

    browser_mpd_trace_reset(BROWSER_MPD_TRACE_ROOT);
    assert(native_browser_screen_update_requested(&screen));
    nc_screen_update(native_browser_screen_base(&screen));
    assert(mpd_trace.calls == 1);
    assert(nc_menu_all_item_count(menu) == 1);
    assert(!native_browser_screen_update_requested(&screen));

    nc_screen_update(native_browser_screen_base(&screen));
    assert(mpd_trace.calls == 1);

    native_browser_screen_destroy(&screen);
    browser_format_fixture_end(&fixture);
    return;
}

static void
test_browser_navigation_requests_reload(void) {
    NativeBrowserScreen screen;
    BrowserFormatFixture fixture;
    NcmStringView view;
    NcMenu *menu;

    browser_format_fixture_begin(&fixture);
    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    menu = native_browser_screen_menu(&screen);

    browser_mpd_trace_reset(BROWSER_MPD_TRACE_ROOT);
    nc_screen_update(native_browser_screen_base(&screen));
    assert(nc_menu_all_item_count(menu) == 1);
    assert(!native_browser_screen_update_requested(&screen));

    browser_mpd_trace_reset(BROWSER_MPD_TRACE_ARTIST);
    assert(native_browser_screen_enter_directory(&screen));
    assert(native_browser_screen_update_requested(&screen));
    view = native_browser_screen_current_directory(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("artist")));

    nc_screen_update(native_browser_screen_base(&screen));
    assert(mpd_trace.calls == 1);
    assert(STREQUAL(mpd_trace.paths[0], mpd_trace.path_lens[0],
                            LIT_ARGS("artist")));
    assert(nc_menu_all_item_count(menu) == 4);
    assert(!native_browser_screen_update_requested(&screen));

    browser_mpd_trace_reset(BROWSER_MPD_TRACE_ROOT);
    assert(native_browser_screen_go_to_parent(&screen));
    assert(native_browser_screen_update_requested(&screen));
    view = native_browser_screen_current_directory(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("/")));

    nc_screen_update(native_browser_screen_base(&screen));
    assert(mpd_trace.calls == 1);
    assert(STREQUAL(mpd_trace.paths[0], mpd_trace.path_lens[0],
                            LIT_ARGS("/")));
    assert(nc_menu_all_item_count(menu) == 1);
    assert(!native_browser_screen_update_requested(&screen));

    native_browser_screen_destroy(&screen);
    browser_format_fixture_end(&fixture);
    return;
}

static void
test_browser_native_update_retries_mpd(void) {
    NativeBrowserScreen screen;
    BrowserFormatFixture fixture;
    NcMenu *menu;

    browser_format_fixture_begin(&fixture);
    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    menu = native_browser_screen_menu(&screen);

    browser_mpd_trace_reset(BROWSER_MPD_TRACE_FAIL_ONCE);
    nc_screen_update(native_browser_screen_base(&screen));
    assert(mpd_trace.calls == 1);
    assert(nc_menu_all_item_count(menu) == 0);
    assert(native_browser_screen_update_requested(&screen));

    nc_screen_update(native_browser_screen_base(&screen));
    assert(mpd_trace.calls == 2);
    assert(nc_menu_all_item_count(menu) == 1);
    assert(!native_browser_screen_update_requested(&screen));

    native_browser_screen_destroy(&screen);
    browser_format_fixture_end(&fixture);
    return;
}

static void
test_browser_native_switch_requests_reload(void) {
    NativeBrowserScreen screen;

    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    native_browser_screen_clear_update_request(&screen);
    assert(!native_browser_screen_update_requested(&screen));

    nc_screen_switch_to(native_browser_screen_base(&screen));
    assert(native_browser_screen_update_requested(&screen));

    native_browser_screen_destroy(&screen);
    return;
}

static void
test_browser_selected_songs(void) {
    NativeBrowserScreen screen;
    NcmMpdItem item;
    NcmPlaylist playlist;
    NcmSong song;
    NcmSongArray songs;
    NcMenu *menu;

    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());
    ncm_mpd_item_init(&item);
    ncm_playlist_init(&playlist);
    ncm_song_init(&song);
    ncm_song_array_init(&songs);
    menu = native_browser_screen_menu(&screen);

    assert(ncm_song_set_uri(&song, LIT_ARGS("one.flac")));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_add_item_copy(&screen, &item));
    assert(ncm_playlist_set(&playlist, LIT_ARGS("lists/favorites"), 0));
    assert(ncm_mpd_item_set_playlist(&item, &playlist));
    assert(native_browser_screen_add_item_copy(&screen, &item));
    assert(ncm_song_set_uri(&song, LIT_ARGS("two.flac")));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_add_item_copy(&screen, &item));

    nc_menu_highlight_position(menu, 0, 24);
    assert(native_browser_screen_selected_songs(&screen, &songs));
    assert(songs.len == 1);
    browser_test_assert_song_uri(&songs, 0, LIT_ARGS("one.flac"));

    ncm_song_array_clear(&songs);
    nc_menu_highlight_position(menu, 1, 24);
    assert(native_browser_screen_selected_songs(&screen, &songs));
    assert(songs.len == 0);

    assert(nc_menu_set_position_selected(menu, 1, true));
    assert(nc_menu_set_position_selected(menu, 2, true));
    assert(native_browser_screen_selected_songs(&screen, &songs));
    assert(songs.len == 1);
    browser_test_assert_song_uri(&songs, 0, LIT_ARGS("two.flac"));

    ncm_song_array_destroy(&songs);
    ncm_song_destroy(&song);
    ncm_playlist_destroy(&playlist);
    ncm_mpd_item_destroy(&item);
    native_browser_screen_destroy(&screen);
    return;
}

static void
test_browser_selected_mpd_directory_recursion(void) {
    NativeBrowserScreen screen;
    NcmSongArray songs;
    NcMenu *menu;

    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());
    ncm_song_array_init(&songs);
    menu = native_browser_screen_menu(&screen);

    browser_test_add_directory(&screen, LIT_ARGS("artist"), 0);
    assert(nc_menu_set_position_selected(menu, 0, true));

    browser_mpd_trace_reset(BROWSER_MPD_TRACE_RECURSIVE);
    assert(native_browser_screen_selected_songs(&screen, &songs));
    assert(mpd_trace.calls == 1);
    assert(STREQUAL(mpd_trace.paths[0], mpd_trace.path_lens[0],
                            LIT_ARGS("artist")));
    assert(songs.len == 2);
    browser_test_assert_song_uri(&songs, 0, LIT_ARGS("artist/a.flac"));
    browser_test_assert_song_uri(&songs, 1,
                                 LIT_ARGS("artist/album/b.flac"));

    ncm_song_array_destroy(&songs);
    native_browser_screen_destroy(&screen);
    return;
}

static void
test_browser_selected_local_directory_recursion(void) {
    NativeBrowserScreen screen;
    BrowserFormatFixture fixture;
    NcmBuffer path;
    NcmSongArray songs;
    char root[128];
    int32 root_len;
    bool old_show_hidden;

    browser_format_fixture_begin(&fixture);
    old_show_hidden = Config.local_browser_show_hidden_files;
    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());
    ncm_buffer_init(&path);
    ncm_song_array_init(&songs);

    root_len = snprintf(root, SIZEOF(root),
                        "/tmp/ncmpcpp-browser-selected-%ld",
                        (long)getpid());
    assert(ncm_fs_mkdir_all(root, root_len, NULL));
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Album"));
    assert(ncm_fs_mkdir_all(path.data, path.len, NULL));
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Album/a.flac"));
    browser_test_write_file(path.data);
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Album/skip.txt"));
    browser_test_write_file(path.data);
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Album/Sub"));
    assert(ncm_fs_mkdir_all(path.data, path.len, NULL));
    browser_test_make_path(&path, root, root_len,
                           LIT_ARGS("Album/Sub/b.mp3"));
    browser_test_write_file(path.data);
    browser_test_make_path(&path, root, root_len,
                           LIT_ARGS("Album/Sub/.hidden.flac"));
    browser_test_write_file(path.data);

    native_browser_screen_set_local(&screen, true);
    Config.local_browser_show_hidden_files = false;
    assert(native_browser_screen_add_supported_extension(&screen,
                                                         LIT_ARGS(".flac")));
    assert(native_browser_screen_add_supported_extension(&screen,
                                                         LIT_ARGS(".mp3")));
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Album"));
    browser_test_add_directory(&screen, path.data, path.len, 0);
    assert(nc_menu_set_position_selected(native_browser_screen_menu(&screen),
                                         0, true));

    assert(native_browser_screen_selected_songs(&screen, &songs));
    assert(songs.len == 2);
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Album/a.flac"));
    assert(browser_test_song_array_has_uri(&songs, path.data, path.len));
    browser_test_make_path(&path, root, root_len,
                           LIT_ARGS("Album/Sub/b.mp3"));
    assert(browser_test_song_array_has_uri(&songs, path.data, path.len));

    browser_test_make_path(&path, root, root_len,
                           LIT_ARGS("Album/Sub/.hidden.flac"));
    browser_test_remove_path(path.data);
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Album/Sub/b.mp3"));
    browser_test_remove_path(path.data);
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Album/Sub"));
    browser_test_remove_path(path.data);
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Album/skip.txt"));
    browser_test_remove_path(path.data);
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Album/a.flac"));
    browser_test_remove_path(path.data);
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Album"));
    browser_test_remove_path(path.data);
    browser_test_remove_path(root);

    Config.local_browser_show_hidden_files = old_show_hidden;
    ncm_song_array_destroy(&songs);
    ncm_buffer_destroy(&path);
    native_browser_screen_destroy(&screen);
    browser_format_fixture_end(&fixture);
    return;
}

static void
test_browser_selected_filtered_current(void) {
    NativeBrowserScreen screen;
    BrowserFormatFixture fixture;
    NcmSongArray songs;
    NcmError error = {0};

    browser_format_fixture_begin(&fixture);
    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());
    ncm_song_array_init(&songs);

    browser_test_add_song(&screen, LIT_ARGS("drop.flac"),
                          LIT_ARGS("Drop"), LIT_ARGS("Artist"), 0);
    browser_test_add_song(&screen, LIT_ARGS("keep.flac"),
                          LIT_ARGS("Keep"), LIT_ARGS("Artist"), 0);
    assert(nc_menu_set_position_selected(native_browser_screen_menu(&screen),
                                         0, true));
    assert(native_browser_screen_apply_filter(&screen, LIT_ARGS("keep"),
                                              &error));
    assert(nc_menu_item_count(native_browser_screen_menu(&screen)) == 1);

    assert(native_browser_screen_selected_songs(&screen, &songs));
    assert(songs.len == 1);
    browser_test_assert_song_uri(&songs, 0, LIT_ARGS("keep.flac"));

    ncm_song_array_destroy(&songs);
    native_browser_screen_destroy(&screen);
    browser_format_fixture_end(&fixture);
    return;
}

static void
test_browser_delete_rejects_parent(void) {
    NativeBrowserScreen screen;
    NcmMpdClient client = {0};
    NcmError error = {0};
    bool old_allow;

    old_allow = Config.allow_for_physical_item_deletion;
    Config.allow_for_physical_item_deletion = true;
    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    native_browser_screen_clear_update_request(&screen);
    browser_test_add_directory(&screen, LIT_ARGS("artist/.."), 0);

    delete_playlist_calls = 0;
    update_directory_calls = 0;
    assert(!native_browser_screen_delete_items(&screen, &client, &error));
    assert(ncm_error_is_set(&error));
    assert(!native_browser_screen_update_requested(&screen));
    assert(delete_playlist_calls == 0);
    assert(update_directory_calls == 0);

    Config.allow_for_physical_item_deletion = old_allow;
    native_browser_screen_destroy(&screen);
    return;
}

static void
test_browser_delete_rejects_disabled_config(void) {
    NativeBrowserScreen screen;
    NcmMpdClient client = {0};
    NcmError error = {0};
    bool old_allow;

    old_allow = Config.allow_for_physical_item_deletion;
    Config.allow_for_physical_item_deletion = false;
    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    native_browser_screen_set_local(&screen, true);
    native_browser_screen_clear_update_request(&screen);
    browser_test_add_song(&screen, LIT_ARGS("song.flac"),
                          LIT_ARGS("Song"), LIT_ARGS("Artist"), 0);

    assert(!native_browser_screen_delete_items(&screen, &client, &error));
    assert(ncm_error_is_set(&error));
    assert(!native_browser_screen_update_requested(&screen));

    Config.allow_for_physical_item_deletion = old_allow;
    native_browser_screen_destroy(&screen);
    return;
}

static void
test_browser_delete_playlist_uses_mpd(void) {
    NativeBrowserScreen screen;
    NcmMpdClient client = {0};
    NcmBuffer path;
    NcmError error = {0};
    char root[128];
    char *old_dir;
    int32 root_len;
    int32 old_dir_len;
    int32 old_dir_cap;
    bool old_allow;

    old_allow = Config.allow_for_physical_item_deletion;
    old_dir = Config.mpd_music_dir;
    old_dir_len = Config.mpd_music_dir_len;
    old_dir_cap = Config.mpd_music_dir_cap;
    Config.allow_for_physical_item_deletion = true;
    ncm_buffer_init(&path);

    root_len = snprintf(root, SIZEOF(root),
                        "/tmp/ncmpcpp-browser-playlist-%lld",
                        (llong)getpid());
    assert(root_len > 0);
    assert(root_len < (int32)SIZEOF(root));
    assert(ncm_fs_mkdir_all(root, root_len, NULL));
    Config.mpd_music_dir = root;
    Config.mpd_music_dir_len = root_len;
    Config.mpd_music_dir_cap = root_len + 1;

    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    native_browser_screen_clear_update_request(&screen);
    browser_test_add_playlist(&screen, LIT_ARGS("stored"), 0);

    delete_playlist_mode = BROWSER_MPD_DELETE_SUCCESS;
    delete_playlist_calls = 0;
    update_directory_calls = 0;
    assert(native_browser_screen_delete_items(&screen, &client, &error));
    assert(delete_playlist_calls == 1);
    assert(STREQUAL(delete_playlist_path,
                            (int32)strlen(delete_playlist_path),
                            LIT_ARGS("stored")));
    assert(update_directory_calls == 1);
    assert(STREQUAL(update_directory_path,
                            (int32)strlen(update_directory_path),
                            LIT_ARGS("/")));
    assert(native_browser_screen_update_requested(&screen));
    native_browser_screen_destroy(&screen);

    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    native_browser_screen_clear_update_request(&screen);
    browser_test_add_playlist(&screen, LIT_ARGS("stored-fallback"), 0);
    browser_test_make_path(&path, root, root_len, LIT_ARGS("stored-fallback"));
    browser_test_write_file(path.data);

    delete_playlist_mode = BROWSER_MPD_DELETE_NO_EXIST;
    delete_playlist_calls = 0;
    update_directory_calls = 0;
    assert(native_browser_screen_delete_items(&screen, &client, &error));
    assert(delete_playlist_calls == 1);
    assert(update_directory_calls == 1);
    assert(!ncm_fs_exists(path.data, path.len));
    assert(native_browser_screen_update_requested(&screen));
    native_browser_screen_destroy(&screen);

    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    native_browser_screen_clear_update_request(&screen);
    browser_test_add_playlist(&screen, LIT_ARGS("stored-fail"), 0);
    browser_test_make_path(&path, root, root_len, LIT_ARGS("stored-fail"));
    browser_test_write_file(path.data);

    delete_playlist_mode = BROWSER_MPD_DELETE_FAIL;
    delete_playlist_calls = 0;
    update_directory_calls = 0;
    assert(!native_browser_screen_delete_items(&screen, &client, &error));
    assert(delete_playlist_calls == 1);
    assert(update_directory_calls == 0);
    assert(ncm_fs_exists(path.data, path.len));
    assert(!native_browser_screen_update_requested(&screen));
    browser_test_remove_path(path.data);
    native_browser_screen_destroy(&screen);

    browser_test_remove_path(root);
    ncm_buffer_destroy(&path);
    Config.allow_for_physical_item_deletion = old_allow;
    Config.mpd_music_dir = old_dir;
    Config.mpd_music_dir_len = old_dir_len;
    Config.mpd_music_dir_cap = old_dir_cap;
    return;
}

static void
test_browser_delete_local_directory_requests_reload(void) {
    NativeBrowserScreen screen;
    NcmMpdClient client = {0};
    NcmBuffer path;
    NcmError error = {0};
    char root[128];
    int32 root_len;
    bool old_allow;

    old_allow = Config.allow_for_physical_item_deletion;
    Config.allow_for_physical_item_deletion = true;
    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    native_browser_screen_set_local(&screen, true);
    native_browser_screen_clear_update_request(&screen);
    ncm_buffer_init(&path);

    root_len = snprintf(root, SIZEOF(root),
                        "/tmp/ncmpcpp-browser-delete-%lld",
                        (llong)getpid());
    assert(root_len > 0);
    assert(root_len < (int32)SIZEOF(root));
    assert(ncm_fs_mkdir_all(root, root_len, NULL));
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Album/Sub"));
    assert(ncm_fs_mkdir_all(path.data, path.len, NULL));
    browser_test_make_path(&path, root, root_len,
                           LIT_ARGS("Album/Sub/song.flac"));
    browser_test_write_file(path.data);
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Album"));
    browser_test_add_directory(&screen, path.data, path.len, 0);

    delete_playlist_calls = 0;
    update_directory_calls = 0;
    assert(native_browser_screen_delete_items(&screen, &client, &error));
    assert(native_browser_screen_update_requested(&screen));
    assert(delete_playlist_calls == 0);
    assert(update_directory_calls == 0);
    assert(!ncm_fs_exists(path.data, path.len));
    browser_test_remove_path(root);

    ncm_buffer_destroy(&path);
    Config.allow_for_physical_item_deletion = old_allow;
    native_browser_screen_destroy(&screen);
    return;
}

static void
test_browser_rename_local_directory(void) {
    NativeBrowserScreen screen;
    NcmMpdClient client = {0};
    NcmBuffer path;
    NcmBuffer new_path;
    NcmError error = {0};
    char root[128];
    int32 root_len;

    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    native_browser_screen_set_local(&screen, true);
    native_browser_screen_clear_update_request(&screen);
    ncm_buffer_init(&path);
    ncm_buffer_init(&new_path);

    root_len = snprintf(root, SIZEOF(root),
                        "/tmp/ncmpcpp-browser-rename-local-%lld",
                        (llong)getpid());
    assert(root_len > 0);
    assert(root_len < (int32)SIZEOF(root));
    assert(ncm_fs_mkdir_all(root, root_len, NULL));
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Old"));
    assert(ncm_fs_mkdir_all(path.data, path.len, NULL));
    browser_test_add_directory(&screen, path.data, path.len, 0);
    browser_test_make_path(&new_path, root, root_len, LIT_ARGS("New"));

    update_directory_calls = 0;
    assert(native_browser_screen_rename_directory_available(&screen));
    assert(native_browser_screen_rename_current_directory(
        &screen, new_path.data, new_path.len, &client, &error));
    assert(update_directory_calls == 0);
    assert(native_browser_screen_update_requested(&screen));
    assert(!ncm_fs_exists(path.data, path.len));
    assert(ncm_fs_is_directory(new_path.data, new_path.len));

    browser_test_remove_path(new_path.data);
    browser_test_remove_path(root);
    ncm_buffer_destroy(&new_path);
    ncm_buffer_destroy(&path);
    native_browser_screen_destroy(&screen);
    return;
}

static void
test_browser_rename_mpd_directory_updates(void) {
    NativeBrowserScreen screen;
    NcmMpdClient client = {0};
    NcmBuffer old_path;
    NcmBuffer new_path;
    NcmError error = {0};
    char root[128];
    int32 root_len;
    char *old_dir;
    int32 old_dir_len;
    int32 old_dir_cap;

    old_dir = Config.mpd_music_dir;
    old_dir_len = Config.mpd_music_dir_len;
    old_dir_cap = Config.mpd_music_dir_cap;
    ncm_buffer_init(&old_path);
    ncm_buffer_init(&new_path);

    root_len = snprintf(root, SIZEOF(root),
                        "/tmp/ncmpcpp-browser-rename-mpd-%lld",
                        (llong)getpid());
    assert(root_len > 0);
    assert(root_len < (int32)SIZEOF(root));
    assert(ncm_fs_mkdir_all(root, root_len, NULL));
    Config.mpd_music_dir = root;
    Config.mpd_music_dir_len = root_len;
    Config.mpd_music_dir_cap = root_len + 1;

    browser_test_make_path(&old_path, root, root_len,
                           LIT_ARGS("Artist/Old"));
    assert(ncm_fs_mkdir_all(old_path.data, old_path.len, NULL));
    browser_test_make_path(&new_path, root, root_len,
                           LIT_ARGS("Artist/New"));

    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    native_browser_screen_clear_update_request(&screen);
    browser_test_add_directory(&screen, LIT_ARGS("Artist/Old"), 0);

    update_directory_calls = 0;
    update_directory_path[0] = '\0';
    assert(native_browser_screen_rename_directory_available(&screen));
    assert(native_browser_screen_rename_current_directory(
        &screen, LIT_ARGS("Artist/New"), &client, &error));
    assert(update_directory_calls == 1);
    assert(STREQUAL(update_directory_path,
                            (int32)strlen(update_directory_path),
                            LIT_ARGS("Artist")));
    assert(native_browser_screen_update_requested(&screen));
    assert(!ncm_fs_exists(old_path.data, old_path.len));
    assert(ncm_fs_is_directory(new_path.data, new_path.len));

    browser_test_remove_path(new_path.data);
    browser_test_make_path(&old_path, root, root_len, LIT_ARGS("Artist"));
    browser_test_remove_path(old_path.data);
    browser_test_remove_path(root);
    native_browser_screen_destroy(&screen);
    ncm_buffer_destroy(&new_path);
    ncm_buffer_destroy(&old_path);
    Config.mpd_music_dir = old_dir;
    Config.mpd_music_dir_len = old_dir_len;
    Config.mpd_music_dir_cap = old_dir_cap;
    return;
}

static void
test_browser_rename_playlist_uses_mpd(void) {
    NativeBrowserScreen screen;
    NcmMpdClient client = {0};
    NcmError error = {0};

    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    native_browser_screen_clear_update_request(&screen);
    browser_test_add_playlist(&screen, LIT_ARGS("old-list"), 0);

    rename_playlist_calls = 0;
    rename_playlist_from[0] = '\0';
    rename_playlist_to[0] = '\0';
    assert(native_browser_screen_rename_playlist_available(&screen));
    assert(native_browser_screen_rename_current_playlist(
        &screen, LIT_ARGS("new-list"), &client, &error));
    assert(rename_playlist_calls == 1);
    assert(STREQUAL(rename_playlist_from,
                            (int32)strlen(rename_playlist_from),
                            LIT_ARGS("old-list")));
    assert(STREQUAL(rename_playlist_to,
                            (int32)strlen(rename_playlist_to),
                            LIT_ARGS("new-list")));
    assert(native_browser_screen_update_requested(&screen));

    native_browser_screen_destroy(&screen);
    return;
}

static void
test_browser_action_rename_prompts(void) {
    NativeBrowserScreen screen;
    NcmBuffer old_path;
    NcmBuffer new_path;
    char root[128];
    int32 root_len;

    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    browser_action_screen = &screen;
    browser_action_connected = true;
    browser_action_status_calls = 0;
    browser_action_status[0] = '\0';
    ncm_buffer_init(&old_path);
    ncm_buffer_init(&new_path);

    root_len = snprintf(root, SIZEOF(root),
                        "/tmp/ncmpcpp-browser-action-rename-%lld",
                        (llong)getpid());
    assert(root_len > 0);
    assert(root_len < (int32)SIZEOF(root));
    assert(ncm_fs_mkdir_all(root, root_len, NULL));
    browser_test_make_path(&old_path, root, root_len, LIT_ARGS("Old"));
    assert(ncm_fs_mkdir_all(old_path.data, old_path.len, NULL));
    browser_test_make_path(&new_path, root, root_len, LIT_ARGS("New"));

    native_browser_screen_set_local(&screen, true);
    native_browser_screen_clear_update_request(&screen);
    browser_test_add_directory(&screen, old_path.data, old_path.len, 0);
    browser_action_set_prompt(new_path.data);

    assert(ncm_action_runtime_can_run(NULL, NCM_ACTION_EDIT_DIRECTORY_NAME));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_EDIT_DIRECTORY_NAME));
    assert(native_browser_screen_update_requested(&screen));
    assert(!ncm_fs_exists(old_path.data, old_path.len));
    assert(ncm_fs_is_directory(new_path.data, new_path.len));
    assert(strstr(browser_action_status,
                  "Directory renamed to") != NULL);

    native_browser_screen_clear(&screen);
    native_browser_screen_clear_update_request(&screen);
    rename_playlist_calls = 0;
    browser_action_status_calls = 0;
    browser_action_status[0] = '\0';
    browser_test_add_playlist(&screen, LIT_ARGS("old-playlist"), 0);
    browser_action_set_prompt("new-playlist");

    assert(ncm_action_runtime_can_run(NULL, NCM_ACTION_EDIT_PLAYLIST_NAME));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_EDIT_PLAYLIST_NAME));
    assert(rename_playlist_calls == 1);
    assert(STREQUAL(rename_playlist_from,
                            (int32)strlen(rename_playlist_from),
                            LIT_ARGS("old-playlist")));
    assert(STREQUAL(rename_playlist_to,
                            (int32)strlen(rename_playlist_to),
                            LIT_ARGS("new-playlist")));
    assert(native_browser_screen_update_requested(&screen));
    assert(strstr(browser_action_status,
                  "Playlist renamed to") != NULL);

    browser_test_remove_path(new_path.data);
    browser_test_remove_path(root);
    ncm_buffer_destroy(&new_path);
    ncm_buffer_destroy(&old_path);
    browser_action_screen = NULL;
    native_browser_screen_destroy(&screen);
    return;
}

static void
test_browser_locate_database_song(void) {
    NativeBrowserScreen screen;
    BrowserFormatFixture fixture;
    NcmSong song;
    NcmMpdClient client = {0};
    NcmStringView view;
    NcmError error = {0};
    enum SortMode old_sort;

    browser_format_fixture_begin(&fixture);
    old_sort = Config.browser_sort_mode;
    Config.browser_sort_mode = NCM_SORT_MODE_NONE;
    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    native_browser_screen_clear_update_request(&screen);
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("artist/album/target.flac")));

    browser_mpd_trace_reset(BROWSER_MPD_TRACE_LOCATE);
    assert(native_browser_screen_locate_song(&screen, &song, &client,
                                             &error));
    assert(mpd_trace.calls == 1);
    assert(STREQUAL(mpd_trace.paths[0], mpd_trace.path_lens[0],
                            LIT_ARGS("artist/album")));
    assert(!native_browser_screen_is_local(&screen));
    assert(!native_browser_screen_update_requested(&screen));
    view = native_browser_screen_current_directory(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("artist/album")));
    assert(nc_menu_highlight(native_browser_screen_menu(&screen)) == 2);
    browser_test_assert_item_path(&screen, 2, NCM_MPD_ITEM_SONG,
                                  LIT_ARGS("artist/album/target.flac"));

    ncm_song_destroy(&song);
    native_browser_screen_destroy(&screen);
    Config.browser_sort_mode = old_sort;
    browser_format_fixture_end(&fixture);
    return;
}

static void
test_browser_locate_local_song(void) {
    NativeBrowserScreen screen;
    NcmBuffer album;
    NcmBuffer song_path;
    NcmSong song;
    NcmMpdClient client = {0};
    NcmStringView view;
    NcmError error = {0};
    char root[128];
    int32 root_len;
    enum SortMode old_sort;

    old_sort = Config.browser_sort_mode;
    Config.browser_sort_mode = NCM_SORT_MODE_NONE;
    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    native_browser_screen_clear_update_request(&screen);
    ncm_buffer_init(&album);
    ncm_buffer_init(&song_path);
    ncm_song_init(&song);

    root_len = snprintf(root, SIZEOF(root),
                        "/tmp/ncmpcpp-browser-locate-%lld",
                        (llong)getpid());
    assert(root_len > 0);
    assert(root_len < (int32)SIZEOF(root));
    assert(ncm_fs_mkdir_all(root, root_len, NULL));
    browser_test_make_path(&album, root, root_len, LIT_ARGS("Album"));
    assert(ncm_fs_mkdir_all(album.data, album.len, NULL));
    browser_test_make_path(&song_path, root, root_len,
                           LIT_ARGS("Album/target.flac"));
    browser_test_write_file(song_path.data);

    assert(native_browser_screen_add_supported_extension(&screen,
                                                         LIT_ARGS(".flac")));
    assert(ncm_song_set_uri(&song, song_path.data, song_path.len));
    assert(native_browser_screen_locate_song(&screen, &song, &client,
                                             &error));
    assert(native_browser_screen_is_local(&screen));
    assert(!native_browser_screen_update_requested(&screen));
    view = native_browser_screen_current_directory(&screen);
    assert(STREQUAL(view.data, view.len, album.data, album.len));
    assert(nc_menu_item_count(native_browser_screen_menu(&screen)) == 2);
    assert(nc_menu_highlight(native_browser_screen_menu(&screen)) == 1);
    browser_test_assert_item_path(&screen, 1, NCM_MPD_ITEM_SONG,
                                  song_path.data, song_path.len);

    browser_test_remove_path(song_path.data);
    browser_test_remove_path(album.data);
    browser_test_remove_path(root);
    ncm_song_destroy(&song);
    ncm_buffer_destroy(&song_path);
    ncm_buffer_destroy(&album);
    native_browser_screen_destroy(&screen);
    Config.browser_sort_mode = old_sort;
    return;
}

static void
test_browser_locate_clears_filter(void) {
    NativeBrowserScreen screen;
    BrowserFormatFixture fixture;
    NcmSong song;
    NcmMpdClient client = {0};
    NcmError error = {0};
    enum SortMode old_sort;

    browser_format_fixture_begin(&fixture);
    old_sort = Config.browser_sort_mode;
    Config.browser_sort_mode = NCM_SORT_MODE_NONE;
    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    ncm_song_init(&song);
    browser_test_add_song(&screen, LIT_ARGS("visible.flac"),
                          LIT_ARGS("Visible"), LIT_ARGS("Artist"), 0);
    assert(native_browser_screen_apply_filter(&screen, LIT_ARGS("visible"),
                                              &error));
    assert(screen.filter_enabled);
    assert(nc_menu_item_count(native_browser_screen_menu(&screen)) == 1);
    assert(ncm_song_set_uri(&song, LIT_ARGS("artist/album/target.flac")));

    browser_mpd_trace_reset(BROWSER_MPD_TRACE_LOCATE);
    assert(native_browser_screen_locate_song(&screen, &song, &client,
                                             &error));
    assert(!screen.filter_enabled);
    assert(nc_menu_item_count(native_browser_screen_menu(&screen)) == 3);
    assert(nc_menu_highlight(native_browser_screen_menu(&screen)) == 2);

    ncm_song_destroy(&song);
    native_browser_screen_destroy(&screen);
    Config.browser_sort_mode = old_sort;
    browser_format_fixture_end(&fixture);
    return;
}

static void
test_browser_locate_missing_directory(void) {
    NativeBrowserScreen screen;
    NcmSong song;
    NcmMpdClient client = {0};
    NcmStringView view;
    NcmError error = {0};

    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    assert(native_browser_screen_set_current_directory(&screen,
                                                       LIT_ARGS("/")));
    native_browser_screen_clear_update_request(&screen);
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("missing/song.flac")));

    browser_mpd_trace_reset(BROWSER_MPD_TRACE_NO_EXIST);
    assert(native_browser_screen_locate_song(&screen, &song, &client,
                                             &error));
    assert(mpd_trace.calls == 1);
    assert(native_browser_screen_update_requested(&screen));
    view = native_browser_screen_current_directory(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("/")));

    ncm_song_destroy(&song);
    native_browser_screen_destroy(&screen);
    return;
}

static void
test_browser_action_jump_to_browser(void) {
    NativeBrowserScreen screen;
    BrowserFormatFixture fixture;
    enum SortMode old_sort;

    browser_format_fixture_begin(&fixture);
    old_sort = Config.browser_sort_mode;
    Config.browser_sort_mode = NCM_SORT_MODE_NONE;
    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    browser_action_screen = &screen;
    browser_action_connected = true;
    browser_test_add_song(&screen, LIT_ARGS("artist/album/target.flac"),
                          LIT_ARGS("Target"), LIT_ARGS("Artist"), 0);

    browser_mpd_trace_reset(BROWSER_MPD_TRACE_LOCATE);
    assert(ncm_action_runtime_can_run(NULL, NCM_ACTION_JUMP_TO_BROWSER));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_JUMP_TO_BROWSER));
    assert(mpd_trace.calls == 1);
    assert(nc_menu_highlight(native_browser_screen_menu(&screen)) == 2);
    browser_test_assert_item_path(&screen, 2, NCM_MPD_ITEM_SONG,
                                  LIT_ARGS("artist/album/target.flac"));

    browser_action_screen = NULL;
    native_browser_screen_destroy(&screen);
    Config.browser_sort_mode = old_sort;
    browser_format_fixture_end(&fixture);
    return;
}

static void
test_browser_filter_and_search(void) {
    NativeBrowserScreen screen;
    BrowserFormatFixture fixture;
    NcmMpdItem item;
    NcmSong song;
    NcmError error = {0};

    browser_format_fixture_begin(&fixture);
    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());
    ncm_mpd_item_init(&item);
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("keep-one.flac")));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_add_item_copy(&screen, &item));
    assert(ncm_song_set_uri(&song, LIT_ARGS("drop.flac")));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_add_item_copy(&screen, &item));
    assert(ncm_song_set_uri(&song, LIT_ARGS("keep-two.flac")));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_add_item_copy(&screen, &item));

    assert(native_browser_screen_apply_filter(&screen, LIT_ARGS("keep"),
                                              &error));
    assert(nc_menu_item_count(native_browser_screen_menu(&screen)) == 2);
    native_browser_screen_clear_filter(&screen);
    assert(nc_menu_item_count(native_browser_screen_menu(&screen)) == 3);
    assert(native_browser_screen_search(&screen, LIT_ARGS("two"), true,
                                        true, false, &error));
    assert(STREQUAL(screen.search_constraint.data,
                            screen.search_constraint.len,
                            LIT_ARGS("two")));
    assert(nc_menu_highlight(native_browser_screen_menu(&screen)) == 2);

    ncm_song_destroy(&song);
    ncm_mpd_item_destroy(&item);
    native_browser_screen_destroy(&screen);
    browser_format_fixture_end(&fixture);
    return;
}

static void
test_browser_local_mode(void) {
    NativeBrowserScreen screen;

    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());
    assert(!native_browser_screen_is_local(&screen));
    native_browser_screen_set_local(&screen, true);
    assert(native_browser_screen_is_local(&screen));
    native_browser_screen_destroy(&screen);
    return;
}

static void
test_browser_change_browse_mode(void) {
    NativeBrowserScreen screen;
    NcmMpdClient client;
    NcmBuffer old_home_buffer;
    NcmStringView view;
    NcmError error = {0};
    char *old_home;
    bool had_home;

    ncm_mpd_client_init(&client);
    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());
    native_browser_screen_clear_update_request(&screen);

    ncm_error_clear(&error);
    assert(!native_browser_screen_change_browse_mode(&screen, &client,
                                                     &error));
    assert(ncm_error_is_set(&error));
    assert(!native_browser_screen_is_local(&screen));
    assert(!native_browser_screen_update_requested(&screen));

    ncm_error_clear(&error);
    assert(ncm_mpd_client_set_hostname(&client, LIT_ARGS("/tmp/mpd.sock"),
                                       &error));

    ncm_buffer_init(&old_home_buffer);
    old_home = getenv("HOME");
    had_home = old_home != NULL;
    if (had_home) {
        assert(ncm_buffer_set(&old_home_buffer, old_home,
                              (int32)strlen(old_home)));
    }
    assert(setenv("HOME", "/tmp/ncmpcpp-browser-home", 1) == 0);

    assert(native_browser_screen_change_browse_mode(&screen, &client,
                                                    &error));
    assert(native_browser_screen_is_local(&screen));
    assert(native_browser_screen_update_requested(&screen));
    assert(native_browser_screen_supported_extensions(&screen)->len == 3);
    view = native_browser_screen_current_directory(&screen);
    assert(STREQUAL(view.data, view.len,
                            LIT_ARGS("/tmp/ncmpcpp-browser-home")));

    native_browser_screen_clear_update_request(&screen);
    assert(native_browser_screen_change_browse_mode(&screen, &client,
                                                    &error));
    assert(!native_browser_screen_is_local(&screen));
    assert(native_browser_screen_update_requested(&screen));
    view = native_browser_screen_current_directory(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("/")));

    if (had_home) {
        assert(setenv("HOME", old_home_buffer.data, 1) == 0);
    } else {
        assert(unsetenv("HOME") == 0);
    }

    ncm_buffer_destroy(&old_home_buffer);
    native_browser_screen_destroy(&screen);
    ncm_mpd_client_destroy(&client);
    return;
}

static void
test_browser_supported_extensions_fetch(void) {
    NativeBrowserScreen screen;
    NcmMpdClient client = {0};
    NcmError error = {0};

    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    assert(native_browser_screen_add_supported_extension(&screen,
                                                         LIT_ARGS(".old")));

    supported_extensions_calls = 0;
    ncm_error_clear(&error);
    assert(native_browser_screen_fetch_supported_extensions(&screen,
                                                            &client,
                                                            &error));
    assert(supported_extensions_calls == 1);
    assert(native_browser_screen_supported_extensions(&screen)->len == 3);
    assert(!native_browser_screen_has_supported_extension(&screen,
                                                          LIT_ARGS(".old")));
    assert(native_browser_screen_has_supported_extension(&screen,
                                                         LIT_ARGS(".flac")));
    assert(native_browser_screen_has_supported_extension(&screen,
                                                         LIT_ARGS(".mp3")));
    assert(native_browser_screen_has_supported_extension(&screen,
                                                         LIT_ARGS(".")));

    native_browser_screen_destroy(&screen);
    return;
}

static void
test_browser_owned_state(void) {
    NativeBrowserScreen screen;
    NcmStringView view;
    char *title;

    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());

    assert(native_browser_screen_update_requested(&screen));
    native_browser_screen_clear_update_request(&screen);
    assert(!native_browser_screen_update_requested(&screen));
    native_browser_screen_request_update(&screen);
    assert(native_browser_screen_update_requested(&screen));
    assert(nc_screen_has_to_be_updated(native_browser_screen_base(&screen)));

    assert(native_browser_screen_set_title_text(&screen,
                                                LIT_ARGS("Browse: /")));
    view = native_browser_screen_title_text(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("Browse: /")));
    title = nc_screen_title(native_browser_screen_base(&screen));
    assert(STREQUAL(title, 9, LIT_ARGS("Browse: /")));

    assert(native_browser_screen_set_column_title_text(
        &screen, LIT_ARGS("Artist / Title")));
    view = native_browser_screen_column_title_text(&screen);
    assert(STREQUAL(view.data, view.len,
                            LIT_ARGS("Artist / Title")));

    native_browser_screen_set_display_mode(&screen, NCM_DISPLAY_MODE_COLUMNS);
    assert(native_browser_screen_display_mode(&screen)
           == NCM_DISPLAY_MODE_COLUMNS);
    native_browser_screen_set_display_mode(&screen, NCM_DISPLAY_MODE_LAST);
    assert(native_browser_screen_display_mode(&screen)
           == NCM_DISPLAY_MODE_COLUMNS);

    assert(native_browser_screen_add_supported_extension(&screen,
                                                         LIT_ARGS("flac")));
    assert(native_browser_screen_add_supported_extension(&screen,
                                                         LIT_ARGS(".mp3")));
    assert(native_browser_screen_add_supported_extension(&screen,
                                                         LIT_ARGS(".mp3")));
    assert(native_browser_screen_has_supported_extension(&screen,
                                                         LIT_ARGS(".flac")));
    assert(!native_browser_screen_has_supported_extension(&screen,
                                                          LIT_ARGS(".ogg")));
    assert(native_browser_screen_supported_extensions(&screen)->len == 2);
    native_browser_screen_clear_supported_extensions(&screen);
    assert(native_browser_screen_supported_extensions(&screen)->len == 0);

    assert(native_browser_screen_set_current_directory(&screen,
                                                       LIT_ARGS("artist")));
    assert(native_browser_screen_set_current_directory(
        &screen, LIT_ARGS("artist/album")));
    view = native_browser_screen_last_highlighted_directory(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("artist")));
    assert(native_browser_screen_go_to_parent(&screen));
    view = native_browser_screen_last_highlighted_directory(&screen);
    assert(STREQUAL(view.data, view.len,
                            LIT_ARGS("artist/album")));
    view = native_browser_screen_current_directory(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("artist")));

    ncm_buffer_append(&screen.item_text_buffer, LIT_ARGS("item"));
    ncm_buffer_append(&screen.path_buffer, LIT_ARGS("path"));
    ncm_buffer_append(&screen.scratch_buffer, LIT_ARGS("scratch"));
    native_browser_screen_clear_temp_buffers(&screen);
    assert(screen.item_text_buffer.len == 0);
    assert(screen.path_buffer.len == 0);
    assert(screen.scratch_buffer.len == 0);

    native_browser_screen_destroy(&screen);
    return;
}


static void
test_browser_menu_callbacks(void) {
    NativeBrowserScreen screen;
    NcmMpdItem item;
    NcmDirectory directory;
    NcmSong song;
    NcmStringView view;
    NcMenu *menu;
    MEVENT event = {0};

    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());
    ncm_mpd_item_init(&item);
    ncm_directory_init(&directory);
    ncm_song_init(&song);
    menu = native_browser_screen_menu(&screen);

    assert(menu->display_callbacks.draw != NULL);
    assert(menu->display_callbacks.filter != NULL);
    assert(menu->display_callbacks.user == &screen);
    assert(menu->action_callbacks.activate != NULL);
    assert(menu->action_callbacks.set_selected != NULL);
    assert(menu->action_callbacks.user == &screen);

    assert(ncm_directory_set(&directory, LIT_ARGS("artist"), 0));
    assert(ncm_mpd_item_set_directory(&item, &directory));
    assert(native_browser_screen_add_item_copy(&screen, &item));
    assert(ncm_song_set_uri(&song, LIT_ARGS("song.flac")));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_add_item_copy(&screen, &item));

    nc_menu_apply_filter(menu);
    assert(nc_menu_item_count(menu) == 2);
    nc_menu_show_all_items(menu);

    native_browser_screen_clear_update_request(&screen);
    assert(native_browser_screen_activate_current(&screen));
    assert(native_browser_screen_update_requested(&screen));
    view = native_browser_screen_current_directory(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("artist")));

    assert(native_browser_screen_set_current_directory(&screen,
                                                       LIT_ARGS("")));
    native_browser_screen_clear_update_request(&screen);
    event.x = 0;
    event.y = 0;
    event.bstate = BUTTON1_PRESSED;
    nc_screen_mouse_button_pressed(native_browser_screen_base(&screen),
                                   event);
    assert(native_browser_screen_update_requested(&screen));
    view = native_browser_screen_current_directory(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("artist")));

    assert(native_browser_screen_set_current_directory(&screen,
                                                       LIT_ARGS("")));
    native_browser_screen_clear_update_request(&screen);
    nc_menu_highlight_position(menu, 1, 24);
    assert(native_browser_screen_activate_current(&screen));
    assert(!native_browser_screen_update_requested(&screen));
    view = native_browser_screen_current_directory(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("")));

    ncm_song_destroy(&song);
    ncm_directory_destroy(&directory);
    ncm_mpd_item_destroy(&item);
    native_browser_screen_destroy(&screen);
    return;
}

static void
test_browser_item_rendering(void) {
    NativeBrowserScreen screen;
    BrowserFormatFixture fixture;
    NcmMpdItem item;
    NcmDirectory directory;
    NcmPlaylist playlist;
    NcmSong song;
    NcBuffer buffer;
    Column columns[2];

    browser_format_fixture_begin(&fixture);
    columns[0] = (Column){0};
    columns[0].name = "Artist";
    columns[0].name_len = STRLIT_LEN("Artist");
    columns[0].type = "a";
    columns[0].type_len = STRLIT_LEN("a");
    columns[0].width = 10;
    columns[0].stretch_limit = -1;
    columns[0].fixed = true;
    columns[1] = (Column){0};
    columns[1].name = "Title";
    columns[1].name_len = STRLIT_LEN("Title");
    columns[1].type = "t";
    columns[1].type_len = STRLIT_LEN("t");
    columns[1].width = 10;
    columns[1].stretch_limit = -1;
    columns[1].fixed = true;
    Config.columns.items = columns;
    Config.columns.len = 2;
    Config.columns.cap = 2;

    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());
    ncm_mpd_item_init(&item);
    ncm_directory_init(&directory);
    ncm_playlist_init(&playlist);
    ncm_song_init(&song);
    nc_buffer_init(&buffer);

    assert(ncm_directory_set(&directory, LIT_ARGS("artists/Alpha"), 0));
    assert(ncm_mpd_item_set_directory(&item, &directory));
    assert(native_browser_screen_render_item(&screen, &buffer, &item,
                                             80, false, false));
    assert(STREQUAL(buffer.data, buffer.len, LIT_ARGS("[Alpha]")));

    assert(ncm_playlist_set(&playlist, LIT_ARGS("lists/Favorites"), 0));
    assert(ncm_mpd_item_set_playlist(&item, &playlist));
    assert(native_browser_screen_render_item(&screen, &buffer, &item,
                                             80, false, false));
    assert(STREQUAL(buffer.data, buffer.len,
                            LIT_ARGS("PL:Favorites")));

    Config.browser_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    assert(ncm_song_set_uri(&song, LIT_ARGS("songs/file.flac")));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_render_item(&screen, &buffer, &item,
                                             80, false, false));
    assert(STREQUAL(buffer.data, buffer.len,
                            LIT_ARGS("classic:songs/file.flac")));

    Config.browser_display_mode = NCM_DISPLAY_MODE_COLUMNS;
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST, LIT_ARGS("Artist")));
    assert(ncm_song_add_tag(&song, MPD_TAG_TITLE, LIT_ARGS("Title")));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_render_item(&screen, &buffer, &item,
                                             20, false, false));
    assert(STREQUAL(buffer.data, buffer.len,
                            LIT_ARGS("Artist    Title     ")));

    nc_buffer_destroy(&buffer);
    ncm_song_destroy(&song);
    ncm_playlist_destroy(&playlist);
    ncm_directory_destroy(&directory);
    ncm_mpd_item_destroy(&item);
    native_browser_screen_destroy(&screen);
    browser_format_fixture_end(&fixture);
    return;
}

static void
test_browser_item_to_string(void) {
    NativeBrowserScreen screen;
    BrowserFormatFixture fixture;
    NcmMpdItem item;
    NcmDirectory directory;
    NcmPlaylist playlist;
    NcmSong song;
    NcmBuffer text;

    browser_format_fixture_begin(&fixture);
    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());
    ncm_mpd_item_init(&item);
    ncm_directory_init(&directory);
    ncm_playlist_init(&playlist);
    ncm_song_init(&song);
    ncm_buffer_init(&text);

    assert(ncm_directory_set(&directory, LIT_ARGS("artists/Alpha"), 0));
    assert(ncm_mpd_item_set_directory(&item, &directory));
    assert(native_browser_screen_item_to_string(&screen, &item, &text));
    assert(STREQUAL(text.data, text.len, LIT_ARGS("[Alpha]")));

    assert(ncm_playlist_set(&playlist, LIT_ARGS("lists/Favorites"), 0));
    assert(ncm_mpd_item_set_playlist(&item, &playlist));
    assert(native_browser_screen_item_to_string(&screen, &item, &text));
    assert(STREQUAL(text.data, text.len,
                            LIT_ARGS("PL:Favorites")));

    Config.browser_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    assert(ncm_song_set_uri(&song, LIT_ARGS("songs/file.flac")));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_item_to_string(&screen, &item, &text));
    assert(STREQUAL(text.data, text.len,
                            LIT_ARGS("classic:songs/file.flac")));

    Config.browser_display_mode = NCM_DISPLAY_MODE_COLUMNS;
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST, LIT_ARGS("Artist")));
    assert(ncm_song_add_tag(&song, MPD_TAG_TITLE, LIT_ARGS("Title")));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_item_to_string(&screen, &item, &text));
    assert(STREQUAL(text.data, text.len,
                            LIT_ARGS("columns:Artist-Title")));

    ncm_buffer_destroy(&text);
    ncm_song_destroy(&song);
    ncm_playlist_destroy(&playlist);
    ncm_directory_destroy(&directory);
    ncm_mpd_item_destroy(&item);
    native_browser_screen_destroy(&screen);
    browser_format_fixture_end(&fixture);
    return;
}

static void
test_browser_header_title(void) {
    NativeBrowserScreen screen;
    NcmStringView view;
    char first_title[64];
    int32 first_len;
    bool old_scrolling;
    enum Design old_design;
    enum DisplayMode old_mode;
    int64 old_width;
    int64 old_height;

    old_scrolling = Config.header_text_scrolling;
    old_design = Config.design;
    old_mode = Config.browser_display_mode;
    old_width = ui_state_screen_width();
    old_height = ui_state_screen_height();

    Config.header_text_scrolling = false;
    Config.design = NCM_DESIGN_CLASSIC;
    Config.browser_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    ui_state_set_screen_size(80, 24);

    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());
    view = native_browser_screen_title_text(&screen);
    assert(STREQUAL(view.data, view.len, LIT_ARGS("Browse: /")));

    assert(native_browser_screen_set_current_directory(
        &screen, LIT_ARGS("artists/alpha")));
    native_browser_screen_update_title_text(&screen);
    view = native_browser_screen_title_text(&screen);
    assert(STREQUAL(view.data, view.len,
                            LIT_ARGS("Browse: artists/alpha")));
    assert(STREQUAL(nc_screen_title(native_browser_screen_base(
                                &screen)), view.len,
                            LIT_ARGS("Browse: artists/alpha")));

    assert(screen.redraw_header);
    native_browser_screen_draw_header(&screen);
    assert(!screen.redraw_header);

    Config.header_text_scrolling = true;
    ui_state_set_screen_size(14, 24);
    assert(native_browser_screen_set_current_directory(
        &screen, LIT_ARGS("abcdefghijklmnopqrstuvwxyz")));
    native_browser_screen_update_title_text(&screen);
    view = native_browser_screen_title_text(&screen);
    assert(view.len < STRLIT_LEN("Browse: abcdefghijklmnopqrstuvwxyz"));
    assert(STREQUAL(view.data, STRLIT_LEN("Browse: "),
                            LIT_ARGS("Browse: ")));

    first_len = view.len;
    assert(first_len < (int32)sizeof(first_title));
    for (int32 i = 0; i < first_len; i += 1) {
        first_title[i] = view.data[i];
    }
    first_title[first_len] = '\0';

    native_browser_screen_update_title_text(&screen);
    view = native_browser_screen_title_text(&screen);
    assert((view.len != first_len)
           || !STREQUAL(view.data, view.len,
                                first_title, first_len));

    native_browser_screen_destroy(&screen);
    Config.header_text_scrolling = old_scrolling;
    Config.design = old_design;
    Config.browser_display_mode = old_mode;
    ui_state_set_screen_size(old_width, old_height);
    return;
}

static void
test_browser_column_title(void) {
    NativeBrowserScreen screen;
    ColumnArray old_columns;
    Column columns[2];
    NcmStringView view;
    enum DisplayMode old_mode;
    bool old_titles_visibility;

    old_columns = Config.columns;
    old_mode = Config.browser_display_mode;
    old_titles_visibility = Config.titles_visibility;

    columns[0] = (Column){0};
    columns[0].name = "Artist";
    columns[0].name_len = STRLIT_LEN("Artist");
    columns[0].type = "a";
    columns[0].type_len = STRLIT_LEN("a");
    columns[0].width = 10;
    columns[0].stretch_limit = -1;
    columns[0].fixed = true;
    columns[1] = (Column){0};
    columns[1].name = "Title";
    columns[1].name_len = STRLIT_LEN("Title");
    columns[1].type = "t";
    columns[1].type_len = STRLIT_LEN("t");
    columns[1].width = 10;
    columns[1].stretch_limit = -1;
    columns[1].fixed = true;
    Config.columns.items = columns;
    Config.columns.len = 2;
    Config.columns.cap = 2;
    Config.titles_visibility = true;
    Config.browser_display_mode = NCM_DISPLAY_MODE_COLUMNS;

    native_browser_screen_init(&screen, 0, 20, 0, 24,
                               nc_color_default(), nc_border_none());
    view = native_browser_screen_column_title_text(&screen);
    assert(STREQUAL(view.data, view.len,
                            LIT_ARGS("Artist    Title     ")));
    assert(STREQUAL(nc_window_title(&screen.window),
                            nc_window_title_len(&screen.window),
                            LIT_ARGS("Artist    Title     ")));

    native_browser_screen_set_display_mode(&screen,
                                           NCM_DISPLAY_MODE_CLASSIC);
    view = native_browser_screen_column_title_text(&screen);
    assert(view.len == 0);
    assert(nc_window_title_len(&screen.window) == 0);

    native_browser_screen_set_display_mode(&screen,
                                           NCM_DISPLAY_MODE_COLUMNS);
    assert(native_browser_screen_column_title_text(&screen).len > 0);

    Config.titles_visibility = false;
    native_browser_screen_update_column_title(&screen);
    assert(native_browser_screen_column_title_text(&screen).len == 0);
    assert(nc_window_title_len(&screen.window) == 0);

    Config.titles_visibility = true;
    native_browser_screen_set_geometry(&screen, 0, 20, 0, 2);
    assert(native_browser_screen_column_title_text(&screen).len == 0);
    assert(nc_window_title_len(&screen.window) == 0);

    native_browser_screen_destroy(&screen);
    Config.columns = old_columns;
    Config.browser_display_mode = old_mode;
    Config.titles_visibility = old_titles_visibility;
    return;
}

static void
browser_mpd_trace_reset(BrowserMpdTraceMode mode) {
    mpd_trace = (BrowserMpdTrace){0};
    mpd_trace.mode = mode;
    return;
}

static void
browser_mpd_trace_record_path(char *path) {
    int32 len;

    assert(mpd_trace.calls < (int32)NCM_ARRAY_LEN(mpd_trace.paths));
    len = 0;
    if (path != NULL) {
        len = (int32)strlen(path);
        assert(len < (int32)SIZEOF(mpd_trace.paths[0]));
        memcpy64(mpd_trace.paths[mpd_trace.calls], path, len);
    }
    mpd_trace.paths[mpd_trace.calls][len] = '\0';
    mpd_trace.path_lens[mpd_trace.calls] = len;
    mpd_trace.calls += 1;
    return;
}

static void
browser_mpd_trace_add_directory(NcmMpdItemArray *items,
                                char *path, int32 path_len) {
    NcmDirectory directory;
    NcmMpdItem item;

    ncm_directory_init(&directory);
    ncm_mpd_item_init(&item);
    assert(ncm_directory_set(&directory, path, path_len, 0));
    assert(ncm_mpd_item_set_directory(&item, &directory));
    assert(ncm_mpd_item_array_append_copy(items, &item));
    ncm_mpd_item_destroy(&item);
    ncm_directory_destroy(&directory);
    return;
}

static void
browser_mpd_trace_add_song(NcmMpdItemArray *items, char *path,
                           int32 path_len) {
    NcmSong song;
    NcmMpdItem item;

    ncm_song_init(&song);
    ncm_mpd_item_init(&item);
    assert(ncm_song_set_uri(&song, path, path_len));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(ncm_mpd_item_array_append_copy(items, &item));
    ncm_mpd_item_destroy(&item);
    ncm_song_destroy(&song);
    return;
}

static void
browser_mpd_trace_append_song(NcmMpdSongList *songs, char *path,
                              int32 path_len) {
    NcmSong song;

    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, path, path_len));
    assert(ncm_mpd_song_list_append_copy(songs, &song));
    ncm_song_destroy(&song);
    return;
}

bool
__wrap_nc_window_has_coords(NcWindow *window, int32 *x, int32 *y) {
    if ((window == NULL) || (x == NULL) || (y == NULL)) {
        return false;
    }
    if ((*x < window->start_x) || (*x >= window->start_x + window->width)) {
        return false;
    }
    if ((*y < window->start_y) || (*y >= window->start_y + window->height)) {
        return false;
    }

    *x -= (int32)window->start_x;
    *y -= (int32)window->start_y;
    return true;
}

bool
__wrap_ncm_mpd_client_get_directory_entries(
    NcmMpdClient *client, char *path, NcmMpdItemArray *items,
    NcmError *error) {
    int32 path_len;

    browser_mpd_trace_record_path(path);
    path_len = 0;
    if (path != NULL) {
        path_len = (int32)strlen(path);
    }
    if ((mpd_trace.mode == BROWSER_MPD_TRACE_GONE)
        && (mpd_trace.calls == 1)) {
        client->connection.server_error_code = MPD_SERVER_ERROR_NO_EXIST;
        ncm_error_set(error, EIO, STRLIT_ARGS("directory missing"));
        return false;
    }
    if ((mpd_trace.mode == BROWSER_MPD_TRACE_FAIL_ONCE)
        && (mpd_trace.calls == 1)) {
        client->connection.server_error_code = (enum mpd_server_error)0;
        ncm_error_set(error, EIO, STRLIT_ARGS("temporary MPD failure"));
        return false;
    }
    if ((mpd_trace.mode == BROWSER_MPD_TRACE_NO_EXIST)
        && STREQUAL(path, path_len, LIT_ARGS("missing"))) {
        client->connection.server_error_code = MPD_SERVER_ERROR_NO_EXIST;
        ncm_error_set(error, ENOENT, STRLIT_ARGS("directory missing"));
        return false;
    }

    client->connection.server_error_code = (enum mpd_server_error)0;
    ncm_error_clear(error);
    if (STREQUAL(path, path_len, LIT_ARGS("/"))) {
        browser_mpd_trace_add_directory(items, LIT_ARGS("artist"));
    } else if (STREQUAL(path, path_len, LIT_ARGS("artist"))) {
        browser_mpd_trace_add_directory(items, LIT_ARGS("artist/album"));
        browser_mpd_trace_add_song(items, LIT_ARGS("artist/keep.flac"));
        browser_mpd_trace_add_song(items, LIT_ARGS("artist/drop.flac"));
    } else if ((mpd_trace.mode == BROWSER_MPD_TRACE_LOCATE)
               && STREQUAL(path, path_len,
                                   LIT_ARGS("artist/album"))) {
        browser_mpd_trace_add_song(items, LIT_ARGS("artist/album/other.flac"));
        browser_mpd_trace_add_song(items,
                                   LIT_ARGS("artist/album/target.flac"));
    } else if (STREQUAL(path, path_len, LIT_ARGS("gone"))) {
        browser_mpd_trace_add_directory(items, LIT_ARGS("gone/child"));
    }
    return true;
}

bool
__wrap_ncm_mpd_client_get_directory_recursive(
    NcmMpdClient *client, char *path, NcmMpdSongList *songs,
    NcmError *error) {
    int32 path_len;

    (void)client;
    browser_mpd_trace_record_path(path);
    path_len = 0;
    if (path != NULL) {
        path_len = (int32)strlen(path);
    }

    if ((mpd_trace.mode == BROWSER_MPD_TRACE_RECURSIVE)
        && STREQUAL(path, path_len, LIT_ARGS("artist"))) {
        browser_mpd_trace_append_song(songs, LIT_ARGS("artist/a.flac"));
        browser_mpd_trace_append_song(songs,
                                      LIT_ARGS("artist/album/b.flac"));
    }
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_get_supported_extensions(
    NcmMpdClient *client, NcmMpdStringList *strings, NcmError *error) {
    (void)client;

    supported_extensions_calls += 1;
    assert(ncm_mpd_string_list_append(strings, LIT_ARGS("flac")));
    assert(ncm_mpd_string_list_append(strings, LIT_ARGS(".mp3")));
    assert(ncm_mpd_string_list_append(strings, LIT_ARGS("flac")));
    assert(ncm_mpd_string_list_append(strings, NULL, 0));
    ncm_error_clear(error);
    return true;
}


bool
__wrap_ncm_mpd_client_delete_playlist(NcmMpdClient *client, char *name,
                                      NcmError *error) {
    delete_playlist_calls += 1;
    delete_playlist_path[0] = '\0';
    if (name != NULL) {
        int32 len;

        len = (int32)strlen(name);
        assert(len < (int32)SIZEOF(delete_playlist_path));
        memcpy64(delete_playlist_path, name, len + 1);
    }

    switch (delete_playlist_mode) {
    case BROWSER_MPD_DELETE_SUCCESS:
        client->connection.server_error_code = (enum mpd_server_error)0;
        ncm_error_clear(error);
        return true;
    case BROWSER_MPD_DELETE_NO_EXIST:
        client->connection.server_error_code = MPD_SERVER_ERROR_NO_EXIST;
        ncm_error_set(error, ENOENT, STRLIT_ARGS("playlist missing"));
        return false;
    case BROWSER_MPD_DELETE_FAIL:
        client->connection.server_error_code =
            (enum mpd_server_error)1234;
        ncm_error_set(error, EIO, STRLIT_ARGS("playlist delete failed"));
        return false;
    }
    return false;
}

bool
__wrap_ncm_mpd_client_rename_playlist(NcmMpdClient *client,
                                      char *from, char *to,
                                      NcmError *error) {
    int32 len;

    (void)client;
    rename_playlist_calls += 1;
    rename_playlist_from[0] = '\0';
    rename_playlist_to[0] = '\0';
    if (from != NULL) {
        len = (int32)strlen(from);
        assert(len < (int32)SIZEOF(rename_playlist_from));
        memcpy64(rename_playlist_from, from, len + 1);
    }
    if (to != NULL) {
        len = (int32)strlen(to);
        assert(len < (int32)SIZEOF(rename_playlist_to));
        memcpy64(rename_playlist_to, to, len + 1);
    }
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_update_directory(NcmMpdClient *client, char *path,
                                       uint32 *id, NcmError *error) {
    int32 len;

    (void)client;
    (void)id;
    update_directory_calls += 1;
    update_directory_path[0] = '\0';
    len = 0;
    if (path != NULL) {
        len = (int32)strlen(path);
        assert(len < (int32)SIZEOF(update_directory_path));
        memcpy64(update_directory_path, path, len + 1);
    }
    ncm_error_clear(error);
    return true;
}

NcScreen *
__wrap_app_controller_current_screen(void) {
    if (browser_action_screen == NULL) {
        return NULL;
    }
    return native_browser_screen_base(browser_action_screen);
}

enum ScreenType
__wrap_native_c_screens_current_type(void) {
    if (browser_action_screen == NULL) {
        return NCM_SCREEN_TYPE_UNKNOWN;
    }
    return NCM_SCREEN_TYPE_BROWSER;
}

NativeBrowserScreen *
__wrap_native_c_screen_browser(void) {
    return browser_action_screen;
}

bool
__wrap_ncm_mpd_client_connected(NcmMpdClient *client) {
    (void)client;
    return browser_action_connected;
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
    return &browser_action_prompt_window;
}

void
__wrap_ncm_statusbar_print(int32 delay, char *message,
                           int32 message_len) {
    (void)delay;
    if (message_len >= (int32)SIZEOF(browser_action_status)) {
        message_len = (int32)SIZEOF(browser_action_status) - 1;
    }
    memcpy64(browser_action_status, message, message_len);
    browser_action_status[message_len] = '\0';
    browser_action_status_calls += 1;
    return;
}

void
__wrap_ncm_statusbar_print_cstring(int32 delay, char *message) {
    __wrap_ncm_statusbar_print(delay, message,
                               browser_test_cstrlen32(message));
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

enum NcPromptStatus
__wrap_nc_window_prompt(NcWindow *window, NcPrompt *prompt,
                        char **result) {
    int32 len;

    (void)window;
    (void)prompt;
    len = browser_test_cstrlen32(browser_action_prompt_text);
    *result = malloc2(len + 1);
    memcpy64(*result, browser_action_prompt_text, len + 1);
    return NC_PROMPT_ACCEPTED;
}

void
__wrap_nc_window_prompt_result_destroy(char *result) {
    int32 len;

    if (result == NULL) {
        return;
    }
    len = browser_test_cstrlen32(result);
    free2(result, len + 1);
    return;
}

static void
browser_format_fixture_begin(BrowserFormatFixture *fixture) {
    NcmError error;

    fixture->old_song_list_format = Config.song_list_format;
    fixture->old_song_columns_mode_format = Config.song_columns_mode_format;
    fixture->old_browser_sort_format = Config.browser_sort_format;
    fixture->old_columns = Config.columns;
    fixture->old_browser_playlist_prefix = Config.browser_playlist_prefix;
    fixture->old_browser_display_mode = Config.browser_display_mode;
    fixture->old_browser_sort_mode = Config.browser_sort_mode;
    fixture->old_discard_colors_if_item_is_selected =
        Config.discard_colors_if_item_is_selected;
    fixture->old_ignore_leading_the = Config.ignore_leading_the;

    ncm_format_ast_init(&Config.song_list_format);
    ncm_format_ast_init(&Config.song_columns_mode_format);
    ncm_format_ast_init(&Config.browser_sort_format);
    nc_buffer_init(&Config.browser_playlist_prefix);
    nc_buffer_append_data(&Config.browser_playlist_prefix,
                          LIT_ARGS("PL:"));
    ncm_error_clear(&error);
    assert(ncm_format_parse(&Config.song_list_format,
                            LIT_ARGS("classic:%F"),
                            NCM_FORMAT_FLAG_ALL, &error));
    assert(ncm_format_parse(&Config.song_columns_mode_format,
                            LIT_ARGS("columns:%a-%t"),
                            NCM_FORMAT_FLAG_ALL, &error));
    assert(ncm_format_parse(&Config.browser_sort_format,
                            LIT_ARGS("custom:%a"),
                            NCM_FORMAT_FLAG_ALL, &error));
    Config.browser_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    Config.browser_sort_mode = NCM_SORT_MODE_TYPE;
    Config.discard_colors_if_item_is_selected = true;
    Config.ignore_leading_the = false;
    return;
}

static void
browser_format_fixture_end(BrowserFormatFixture *fixture) {
    ncm_format_ast_destroy(&Config.song_list_format);
    ncm_format_ast_destroy(&Config.song_columns_mode_format);
    ncm_format_ast_destroy(&Config.browser_sort_format);
    nc_buffer_destroy(&Config.browser_playlist_prefix);

    Config.song_list_format = fixture->old_song_list_format;
    Config.song_columns_mode_format =
        fixture->old_song_columns_mode_format;
    Config.browser_sort_format = fixture->old_browser_sort_format;
    Config.columns = fixture->old_columns;
    Config.browser_playlist_prefix = fixture->old_browser_playlist_prefix;
    Config.browser_display_mode = fixture->old_browser_display_mode;
    Config.browser_sort_mode = fixture->old_browser_sort_mode;
    Config.discard_colors_if_item_is_selected =
        fixture->old_discard_colors_if_item_is_selected;
    Config.ignore_leading_the = fixture->old_ignore_leading_the;
    return;
}

static void
browser_test_add_directory(NativeBrowserScreen *screen, char *path,
                           int32 path_len, time_t mtime) {
    NcmDirectory directory;
    NcmMpdItem item;

    ncm_directory_init(&directory);
    ncm_mpd_item_init(&item);
    assert(ncm_directory_set(&directory, path, path_len, mtime));
    assert(ncm_mpd_item_set_directory(&item, &directory));
    assert(native_browser_screen_add_item_copy(screen, &item));
    ncm_mpd_item_destroy(&item);
    ncm_directory_destroy(&directory);
    return;
}

static void
browser_test_add_playlist(NativeBrowserScreen *screen, char *path,
                          int32 path_len, time_t mtime) {
    NcmPlaylist playlist;
    NcmMpdItem item;

    ncm_playlist_init(&playlist);
    ncm_mpd_item_init(&item);
    assert(ncm_playlist_set(&playlist, path, path_len, mtime));
    assert(ncm_mpd_item_set_playlist(&item, &playlist));
    assert(native_browser_screen_add_item_copy(screen, &item));
    ncm_mpd_item_destroy(&item);
    ncm_playlist_destroy(&playlist);
    return;
}

static void
browser_test_add_song(NativeBrowserScreen *screen, char *uri,
                      int32 uri_len, char *name, int32 name_len,
                      char *artist, int32 artist_len, time_t mtime) {
    NcmSong song;
    NcmMpdItem item;

    ncm_song_init(&song);
    ncm_mpd_item_init(&item);
    assert(ncm_song_set_uri(&song, uri, uri_len));
    assert(ncm_song_add_tag(&song, MPD_TAG_NAME, name, name_len));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST, artist, artist_len));
    ncm_song_set_mtime(&song, mtime);
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_add_item_copy(screen, &item));
    ncm_mpd_item_destroy(&item);
    ncm_song_destroy(&song);
    return;
}

static void
browser_test_assert_item_path(NativeBrowserScreen *screen, int64 pos,
                              enum NcmMpdItemKind kind, char *path,
                              int32 path_len) {
    NcmStringView view;
    NcmMpdItem *item;

    item = nc_menu_item_at(native_browser_screen_menu(screen),
                           NC_MENU_ITEMS_ALL, pos);
    assert(ncm_mpd_item_kind(item) == kind);
    ncm_string_view_clear(&view);
    switch (kind) {
    case NCM_MPD_ITEM_DIRECTORY:
        assert(ncm_directory_path_view(ncm_mpd_item_directory(item),
                                       &view));
        break;
    case NCM_MPD_ITEM_SONG:
        assert(ncm_song_uri_view(ncm_mpd_item_song(item), 0, &view));
        break;
    case NCM_MPD_ITEM_PLAYLIST:
        assert(ncm_playlist_path_view(ncm_mpd_item_playlist(item),
                                      &view));
        break;
    case NCM_MPD_ITEM_UNKNOWN:
        break;
    }
    assert(STREQUAL(view.data, view.len, path, path_len));
    return;
}

static void
browser_test_assert_song_uri(NcmSongArray *songs, int32 pos, char *uri,
                             int32 uri_len) {
    NcmStringView view;

    assert(songs != NULL);
    assert(pos >= 0);
    assert(pos < songs->len);
    assert(ncm_song_uri_view(&songs->items[pos], 0, &view));
    assert(STREQUAL(view.data, view.len, uri, uri_len));
    return;
}

static bool
browser_test_song_array_has_uri(NcmSongArray *songs, char *uri,
                                int32 uri_len) {
    NcmStringView view;

    if (songs == NULL) {
        return false;
    }
    for (int32 i = 0; i < songs->len; i += 1) {
        if (!ncm_song_uri_view(&songs->items[i], 0, &view)) {
            continue;
        }
        if (STREQUAL(view.data, view.len, uri, uri_len)) {
            return true;
        }
    }
    return false;
}

static bool
browser_test_has_item_path(NativeBrowserScreen *screen,
                           enum NcmMpdItemKind kind, char *path,
                           int32 path_len) {
    NcmStringView view;
    NcmMpdItem *item;
    NcMenu *menu;

    menu = native_browser_screen_menu(screen);
    for (int64 i = 0; i < nc_menu_all_item_count(menu); i += 1) {
        item = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, i);
        if (ncm_mpd_item_kind(item) != kind) {
            continue;
        }
        ncm_string_view_clear(&view);
        switch (kind) {
        case NCM_MPD_ITEM_DIRECTORY:
            if (!ncm_directory_path_view(ncm_mpd_item_directory(item),
                                         &view)) {
                continue;
            }
            break;
        case NCM_MPD_ITEM_SONG:
            if (!ncm_song_uri_view(ncm_mpd_item_song(item), 0, &view)) {
                continue;
            }
            break;
        case NCM_MPD_ITEM_PLAYLIST:
            if (!ncm_playlist_path_view(ncm_mpd_item_playlist(item),
                                        &view)) {
                continue;
            }
            break;
        case NCM_MPD_ITEM_UNKNOWN:
            break;
        }
        if (STREQUAL(view.data, view.len, path, path_len)) {
            return true;
        }
    }
    return false;
}

static void
browser_test_make_path(NcmBuffer *path, char *root, int32 root_len,
                       char *name, int32 name_len) {
    ncm_buffer_clear(path);
    assert(ncm_fs_join(path, root, root_len, name, name_len));
    return;
}

static void
browser_test_write_file(char *path) {
    FILE *file;

    file = fopen(path, "wb");
    assert(file != NULL);
    assert(fputs("test", file) >= 0);
    assert(fclose(file) == 0);
    return;
}

static void
browser_test_remove_path(char *path) {
    if (remove(path) == 0) {
        return;
    }
    assert(rmdir(path) == 0);
    return;
}

static int32
browser_test_cstrlen32(char *text) {
    int32 len;

    if (text == NULL) {
        return 0;
    }

    len = 0;
    while (text[len] != '\0') {
        len += 1;
    }
    return len;
}

static void
browser_action_set_prompt(char *text) {
    int32 len;

    len = browser_test_cstrlen32(text);
    assert(len < (int32)SIZEOF(browser_action_prompt_text));
    memcpy64(browser_action_prompt_text, text, len + 1);
    return;
}

static void
browser_parity_test_pending(char *name) {
    (void)name;
    return;
}

static void
test_browser_local_browsing_parity(void) {
    NativeBrowserScreen screen;
    BrowserFormatFixture fixture;
    NcmBuffer path;
    char root[128];
    int32 root_len;
    bool old_show_hidden;

    browser_format_fixture_begin(&fixture);
    old_show_hidden = Config.local_browser_show_hidden_files;
    native_browser_screen_init(&screen, 0, 80, 0, 24,
                               nc_color_default(), nc_border_none());
    ncm_buffer_init(&path);

    root_len = snprintf(root, SIZEOF(root),
                         "/tmp/ncmpcpp2-browser-local-%lld",
                         (llong)getpid());
    assert(root_len > 0);
    assert(root_len < (int32)SIZEOF(root));
    assert(ncm_fs_mkdir_all(root, root_len, NULL));

    browser_test_make_path(&path, root, root_len, LIT_ARGS("Album"));
    assert(ncm_fs_mkdir_all(path.data, path.len, NULL));
    browser_test_make_path(&path, root, root_len, LIT_ARGS(".hidden-dir"));
    assert(ncm_fs_mkdir_all(path.data, path.len, NULL));
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Track.flac"));
    browser_test_write_file(path.data);
    browser_test_make_path(&path, root, root_len, LIT_ARGS(".hidden.mp3"));
    browser_test_write_file(path.data);
    browser_test_make_path(&path, root, root_len, LIT_ARGS("notes.txt"));
    browser_test_write_file(path.data);

    native_browser_screen_set_local(&screen, true);
    assert(native_browser_screen_set_current_directory(&screen,
                                                       root, root_len));
    assert(native_browser_screen_add_supported_extension(&screen,
                                                         LIT_ARGS(".flac")));
    assert(native_browser_screen_add_supported_extension(&screen,
                                                         LIT_ARGS(".mp3")));

    Config.local_browser_show_hidden_files = false;
    Config.browser_sort_mode = NCM_SORT_MODE_TYPE;
    nc_screen_update(native_browser_screen_base(&screen));
    assert(!native_browser_screen_update_requested(&screen));
    assert(Config.browser_sort_mode == NCM_SORT_MODE_NAME);
    assert(nc_menu_all_item_count(native_browser_screen_menu(&screen)) == 3);
    browser_test_make_path(&path, root, root_len, LIT_ARGS(".."));
    browser_test_assert_item_path(&screen, 0, NCM_MPD_ITEM_DIRECTORY,
                                  path.data, path.len);
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Album"));
    browser_test_assert_item_path(&screen, 1, NCM_MPD_ITEM_DIRECTORY,
                                  path.data, path.len);
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Track.flac"));
    browser_test_assert_item_path(&screen, 2, NCM_MPD_ITEM_SONG,
                                  path.data, path.len);

    Config.local_browser_show_hidden_files = true;
    native_browser_screen_request_update(&screen);
    nc_screen_update(native_browser_screen_base(&screen));
    assert(nc_menu_all_item_count(native_browser_screen_menu(&screen)) == 5);
    browser_test_make_path(&path, root, root_len, LIT_ARGS(".hidden-dir"));
    assert(browser_test_has_item_path(&screen, NCM_MPD_ITEM_DIRECTORY,
                                      path.data, path.len));
    browser_test_make_path(&path, root, root_len, LIT_ARGS(".hidden.mp3"));
    assert(browser_test_has_item_path(&screen, NCM_MPD_ITEM_SONG,
                                      path.data, path.len));
    browser_test_make_path(&path, root, root_len, LIT_ARGS("notes.txt"));
    assert(!browser_test_has_item_path(&screen, NCM_MPD_ITEM_SONG,
                                       path.data, path.len));

    browser_test_make_path(&path, root, root_len, LIT_ARGS("notes.txt"));
    browser_test_remove_path(path.data);
    browser_test_make_path(&path, root, root_len, LIT_ARGS(".hidden.mp3"));
    browser_test_remove_path(path.data);
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Track.flac"));
    browser_test_remove_path(path.data);
    browser_test_make_path(&path, root, root_len, LIT_ARGS(".hidden-dir"));
    browser_test_remove_path(path.data);
    browser_test_make_path(&path, root, root_len, LIT_ARGS("Album"));
    browser_test_remove_path(path.data);
    browser_test_remove_path(root);

    Config.local_browser_show_hidden_files = old_show_hidden;
    ncm_buffer_destroy(&path);
    native_browser_screen_destroy(&screen);
    browser_format_fixture_end(&fixture);
    return;
}

static void
test_browser_directory_expansion_parity(void) {
    BROWSER_PARITY_TEST_PENDING(browser_directory_expansion_parity);
    return;
}

static void
test_browser_playlist_expansion_parity(void) {
    BROWSER_PARITY_TEST_PENDING(browser_playlist_expansion_parity);
    return;
}

static void
test_browser_deletion_parity(void) {
    BROWSER_PARITY_TEST_PENDING(browser_deletion_parity);
    return;
}

static void
test_browser_filter_search_formatting_parity(void) {
    NativeBrowserScreen screen;
    BrowserFormatFixture fixture;
    NcmMpdItem item;
    NcmDirectory directory;
    NcmSong song;
    NcmError error = {0};

    browser_format_fixture_begin(&fixture);
    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());
    ncm_mpd_item_init(&item);
    ncm_directory_init(&directory);
    ncm_song_init(&song);

    ncm_error_clear(&error);
    assert(ncm_format_parse(&Config.song_list_format,
                            LIT_ARGS("classic:%t"),
                            NCM_FORMAT_FLAG_ALL, &error));

    assert(ncm_directory_set(&directory, LIT_ARGS(".."), 0));
    assert(ncm_mpd_item_set_directory(&item, &directory));
    assert(native_browser_screen_add_item_copy(&screen, &item));
    assert(ncm_song_set_uri(&song, LIT_ARGS("hidden-uri.flac")));
    assert(ncm_song_add_tag(&song, MPD_TAG_TITLE, LIT_ARGS("Visible")));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_add_item_copy(&screen, &item));

    assert(native_browser_screen_apply_filter(&screen,
                                              LIT_ARGS("Visible"),
                                              &error));
    assert(nc_menu_item_count(native_browser_screen_menu(&screen)) == 2);
    native_browser_screen_clear_filter(&screen);

    assert(!native_browser_screen_search(&screen, LIT_ARGS("hidden-uri"),
                                         true, true, false, &error));
    assert(native_browser_screen_search(&screen, LIT_ARGS("Visible"),
                                        true, true, false, &error));
    assert(nc_menu_highlight(native_browser_screen_menu(&screen)) == 1);

    ncm_song_destroy(&song);
    ncm_directory_destroy(&directory);
    ncm_mpd_item_destroy(&item);
    native_browser_screen_destroy(&screen);
    browser_format_fixture_end(&fixture);
    return;
}

static void
test_browser_sort_modes_parity(void) {
    NativeBrowserScreen screen;
    BrowserFormatFixture fixture;

    browser_format_fixture_begin(&fixture);
    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());

    assert(native_browser_screen_set_current_directory(
        &screen, LIT_ARGS("artists")));
    browser_test_add_directory(&screen, LIT_ARGS("artists/.."), 0);
    browser_test_add_playlist(&screen, LIT_ARGS("lists/Zed"), 0);
    browser_test_add_song(&screen, LIT_ARGS("songs/B.flac"),
                          LIT_ARGS("Beta"), LIT_ARGS("Alpha"), 2);
    browser_test_add_directory(&screen, LIT_ARGS("artists/Alpha"), 5);
    browser_test_add_song(&screen, LIT_ARGS("songs/A.flac"),
                          LIT_ARGS("Alpha"), LIT_ARGS("Zulu"), 10);
    browser_test_add_playlist(&screen, LIT_ARGS("lists/Alpha"), 9);

    Config.browser_sort_mode = NCM_SORT_MODE_TYPE;
    assert(native_browser_screen_sort(&screen));
    browser_test_assert_item_path(&screen, 0, NCM_MPD_ITEM_DIRECTORY,
                                  LIT_ARGS("artists/.."));
    browser_test_assert_item_path(&screen, 1, NCM_MPD_ITEM_DIRECTORY,
                                  LIT_ARGS("artists/Alpha"));
    browser_test_assert_item_path(&screen, 2, NCM_MPD_ITEM_SONG,
                                  LIT_ARGS("songs/B.flac"));
    browser_test_assert_item_path(&screen, 3, NCM_MPD_ITEM_SONG,
                                  LIT_ARGS("songs/A.flac"));
    browser_test_assert_item_path(&screen, 4, NCM_MPD_ITEM_PLAYLIST,
                                  LIT_ARGS("lists/Zed"));
    browser_test_assert_item_path(&screen, 5, NCM_MPD_ITEM_PLAYLIST,
                                  LIT_ARGS("lists/Alpha"));

    Config.browser_sort_mode = NCM_SORT_MODE_NAME;
    assert(native_browser_screen_sort(&screen));
    browser_test_assert_item_path(&screen, 0, NCM_MPD_ITEM_DIRECTORY,
                                  LIT_ARGS("artists/.."));
    browser_test_assert_item_path(&screen, 2, NCM_MPD_ITEM_SONG,
                                  LIT_ARGS("songs/A.flac"));
    browser_test_assert_item_path(&screen, 3, NCM_MPD_ITEM_SONG,
                                  LIT_ARGS("songs/B.flac"));
    browser_test_assert_item_path(&screen, 4, NCM_MPD_ITEM_PLAYLIST,
                                  LIT_ARGS("lists/Alpha"));
    browser_test_assert_item_path(&screen, 5, NCM_MPD_ITEM_PLAYLIST,
                                  LIT_ARGS("lists/Zed"));

    Config.browser_sort_mode = NCM_SORT_MODE_MODIFICATION_TIME;
    assert(native_browser_screen_sort(&screen));
    browser_test_assert_item_path(&screen, 2, NCM_MPD_ITEM_SONG,
                                  LIT_ARGS("songs/A.flac"));
    browser_test_assert_item_path(&screen, 3, NCM_MPD_ITEM_SONG,
                                  LIT_ARGS("songs/B.flac"));
    browser_test_assert_item_path(&screen, 4, NCM_MPD_ITEM_PLAYLIST,
                                  LIT_ARGS("lists/Alpha"));
    browser_test_assert_item_path(&screen, 5, NCM_MPD_ITEM_PLAYLIST,
                                  LIT_ARGS("lists/Zed"));

    Config.browser_sort_mode = NCM_SORT_MODE_CUSTOM_FORMAT;
    assert(native_browser_screen_sort(&screen));
    browser_test_assert_item_path(&screen, 2, NCM_MPD_ITEM_SONG,
                                  LIT_ARGS("songs/B.flac"));
    browser_test_assert_item_path(&screen, 3, NCM_MPD_ITEM_SONG,
                                  LIT_ARGS("songs/A.flac"));

    Config.browser_sort_mode = NCM_SORT_MODE_NONE;
    assert(native_browser_screen_sort(&screen));
    browser_test_assert_item_path(&screen, 2, NCM_MPD_ITEM_SONG,
                                  LIT_ARGS("songs/B.flac"));
    browser_test_assert_item_path(&screen, 3, NCM_MPD_ITEM_SONG,
                                  LIT_ARGS("songs/A.flac"));

    native_browser_screen_destroy(&screen);
    browser_format_fixture_end(&fixture);
    return;
}

static void
test_browser_jump_to_playing_song_parity(void) {
    BROWSER_PARITY_TEST_PENDING(browser_jump_to_playing_song_parity);
    return;
}
