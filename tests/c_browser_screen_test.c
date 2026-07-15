#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_format.h"
#include "c/ncm_base.h"
#include "c/ncm_string.h"
#include "screens/nc_browser.h"
#include "settings.h"
#include "ui_state.h"

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
static void test_browser_filter_and_search(void);
static void test_browser_local_mode(void);
static void test_browser_owned_state(void);
static void test_browser_native_callbacks_ignore_bridge(void);
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
static NcWindow *test_bridge_active_window(void *user);
static void test_bridge_refresh(void *user);
static void test_bridge_refresh_window(void *user);
static void test_bridge_scroll(void *user, enum NcScroll where);
static void test_bridge_switch_to(void *user);
static void test_bridge_resize(void *user);
static char *test_bridge_title(void *user);
static void test_bridge_update(void *user);
static void test_bridge_request_update(void *user);
static void test_bridge_mouse(void *user, MEVENT event);


typedef enum BrowserMpdTraceMode {
    BROWSER_MPD_TRACE_ROOT,
    BROWSER_MPD_TRACE_ARTIST,
    BROWSER_MPD_TRACE_GONE,
    BROWSER_MPD_TRACE_FAIL_ONCE,
} BrowserMpdTraceMode;

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
bool __wrap_nc_window_has_coords(NcWindow *window, int32 *x, int32 *y);
bool __wrap_ncm_mpd_client_get_directory_entries(
    NcmMpdClient *client, char *path, NcmMpdItemArray *items,
    NcmError *error);

static BrowserMpdTrace mpd_trace;
static int32 bridge_callback_calls;

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
    test_browser_filter_and_search();
    test_browser_local_mode();
    test_browser_owned_state();
    test_browser_native_callbacks_ignore_bridge();
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
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("artist/album")));

    assert(native_browser_screen_go_to_parent(&screen));
    view = native_browser_screen_current_directory(&screen);
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("artist")));

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
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("artist")));

    native_browser_screen_clear(&screen);
    assert(ncm_directory_set(&directory, LIT_ARGS(".."), 0));
    assert(ncm_mpd_item_set_directory(&item, &directory));
    assert(native_browser_screen_item_is_parent(&item));
    assert(native_browser_screen_add_item_copy(&screen, &item));
    assert(native_browser_screen_enter_directory(&screen));
    view = native_browser_screen_current_directory(&screen);
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("/")));
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
    assert(ncm_string_equal(mpd_trace.paths[0], mpd_trace.path_lens[0],
                            LIT_ARGS("/")));
    view = native_browser_screen_current_directory(&screen);
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("/")));
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
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("artist/..")));
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
    assert(ncm_string_equal(mpd_trace.paths[0], mpd_trace.path_lens[0],
                            LIT_ARGS("gone/child")));
    assert(ncm_string_equal(mpd_trace.paths[1], mpd_trace.path_lens[1],
                            LIT_ARGS("gone")));
    view = native_browser_screen_current_directory(&screen);
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("gone")));
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
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("artist")));

    nc_screen_update(native_browser_screen_base(&screen));
    assert(mpd_trace.calls == 1);
    assert(ncm_string_equal(mpd_trace.paths[0], mpd_trace.path_lens[0],
                            LIT_ARGS("artist")));
    assert(nc_menu_all_item_count(menu) == 4);
    assert(!native_browser_screen_update_requested(&screen));

    browser_mpd_trace_reset(BROWSER_MPD_TRACE_ROOT);
    assert(native_browser_screen_go_to_parent(&screen));
    assert(native_browser_screen_update_requested(&screen));
    view = native_browser_screen_current_directory(&screen);
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("/")));

    nc_screen_update(native_browser_screen_base(&screen));
    assert(mpd_trace.calls == 1);
    assert(ncm_string_equal(mpd_trace.paths[0], mpd_trace.path_lens[0],
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
    NcmSong song;
    NcmSongArray songs;
    NcmStringView view;

    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());
    ncm_mpd_item_init(&item);
    ncm_song_init(&song);
    ncm_song_array_init(&songs);

    assert(ncm_song_set_uri(&song, LIT_ARGS("one.flac")));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_add_item_copy(&screen, &item));
    assert(ncm_song_set_uri(&song, LIT_ARGS("two.flac")));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_add_item_copy(&screen, &item));
    assert(nc_menu_set_position_selected(native_browser_screen_menu(&screen),
                                         1, true));

    assert(native_browser_screen_selected_songs(&screen, &songs));
    assert(songs.len == 1);
    assert(ncm_song_uri_view(&songs.items[0], 0, &view));
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("two.flac")));

    ncm_song_array_destroy(&songs);
    ncm_song_destroy(&song);
    ncm_mpd_item_destroy(&item);
    native_browser_screen_destroy(&screen);
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
    assert(ncm_string_equal(screen.search_constraint.data,
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
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("Browse: /")));
    title = nc_screen_title(native_browser_screen_base(&screen));
    assert(ncm_string_equal(title, 9, LIT_ARGS("Browse: /")));

    assert(native_browser_screen_set_column_title_text(
        &screen, LIT_ARGS("Artist / Title")));
    view = native_browser_screen_column_title_text(&screen);
    assert(ncm_string_equal(view.data, view.len,
                            LIT_ARGS("Artist / Title")));

    native_browser_screen_set_display_mode(&screen, NCM_DISPLAY_MODE_COLUMNS);
    assert(native_browser_screen_display_mode(&screen)
           == NCM_DISPLAY_MODE_COLUMNS);
    native_browser_screen_set_display_mode(&screen, NCM_DISPLAY_MODE_LAST);
    assert(native_browser_screen_display_mode(&screen)
           == NCM_DISPLAY_MODE_COLUMNS);

    assert(native_browser_screen_add_supported_extension(&screen,
                                                         LIT_ARGS(".flac")));
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
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("artist")));
    assert(native_browser_screen_go_to_parent(&screen));
    view = native_browser_screen_last_highlighted_directory(&screen);
    assert(ncm_string_equal(view.data, view.len,
                            LIT_ARGS("artist/album")));
    view = native_browser_screen_current_directory(&screen);
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("artist")));

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
test_browser_native_callbacks_ignore_bridge(void) {
    NativeBrowserScreen screen;
    NativeBrowserBridge bridge = {0};
    NcWindow *active;
    char *title;
    MEVENT event = {0};

    native_browser_screen_init(&screen, 0, 80, 0, 24, nc_color_default(),
                               nc_border_none());
    assert(native_browser_screen_set_current_directory(&screen,
                                                       LIT_ARGS("music")));

    bridge.active_window = test_bridge_active_window;
    bridge.refresh = test_bridge_refresh;
    bridge.refresh_window = test_bridge_refresh_window;
    bridge.scroll = test_bridge_scroll;
    bridge.switch_to = test_bridge_switch_to;
    bridge.resize = test_bridge_resize;
    bridge.title = test_bridge_title;
    bridge.update = test_bridge_update;
    bridge.request_update = test_bridge_request_update;
    bridge.mouse_button_pressed = test_bridge_mouse;
    native_browser_screen_set_bridge(&screen, bridge);

    bridge_callback_calls = 0;
    active = nc_screen_active_window(native_browser_screen_base(&screen));
    assert(active == native_browser_screen_window(&screen));
    nc_screen_scroll(native_browser_screen_base(&screen), NC_SCROLL_DOWN);
    nc_screen_switch_to(native_browser_screen_base(&screen));
    native_browser_screen_request_update(&screen);
    nc_screen_update(native_browser_screen_base(&screen));
    nc_screen_mouse_button_pressed(native_browser_screen_base(&screen),
                                   event);
    title = nc_screen_title(native_browser_screen_base(&screen));
    assert(ncm_string_equal(title, 13, LIT_ARGS("Browse: music")));
    assert(bridge_callback_calls == 0);

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
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("artist")));

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
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("artist")));

    assert(native_browser_screen_set_current_directory(&screen,
                                                       LIT_ARGS("")));
    native_browser_screen_clear_update_request(&screen);
    nc_menu_highlight_position(menu, 1, 24);
    assert(native_browser_screen_activate_current(&screen));
    assert(!native_browser_screen_update_requested(&screen));
    view = native_browser_screen_current_directory(&screen);
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("")));

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
    columns[0].name = (char *)"Artist";
    columns[0].name_len = STRLIT_LEN("Artist");
    columns[0].type = (char *)"a";
    columns[0].type_len = STRLIT_LEN("a");
    columns[0].width = 10;
    columns[0].stretch_limit = -1;
    columns[0].fixed = true;
    columns[1] = (Column){0};
    columns[1].name = (char *)"Title";
    columns[1].name_len = STRLIT_LEN("Title");
    columns[1].type = (char *)"t";
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
    assert(ncm_string_equal(buffer.data, buffer.len, LIT_ARGS("[Alpha]")));

    assert(ncm_playlist_set(&playlist, LIT_ARGS("lists/Favorites"), 0));
    assert(ncm_mpd_item_set_playlist(&item, &playlist));
    assert(native_browser_screen_render_item(&screen, &buffer, &item,
                                             80, false, false));
    assert(ncm_string_equal(buffer.data, buffer.len,
                            LIT_ARGS("PL:Favorites")));

    Config.browser_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    assert(ncm_song_set_uri(&song, LIT_ARGS("songs/file.flac")));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_render_item(&screen, &buffer, &item,
                                             80, false, false));
    assert(ncm_string_equal(buffer.data, buffer.len,
                            LIT_ARGS("classic:songs/file.flac")));

    Config.browser_display_mode = NCM_DISPLAY_MODE_COLUMNS;
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST, LIT_ARGS("Artist")));
    assert(ncm_song_add_tag(&song, MPD_TAG_TITLE, LIT_ARGS("Title")));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_render_item(&screen, &buffer, &item,
                                             20, false, false));
    assert(ncm_string_equal(buffer.data, buffer.len,
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
    assert(ncm_string_equal(text.data, text.len, LIT_ARGS("[Alpha]")));

    assert(ncm_playlist_set(&playlist, LIT_ARGS("lists/Favorites"), 0));
    assert(ncm_mpd_item_set_playlist(&item, &playlist));
    assert(native_browser_screen_item_to_string(&screen, &item, &text));
    assert(ncm_string_equal(text.data, text.len,
                            LIT_ARGS("PL:Favorites")));

    Config.browser_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    assert(ncm_song_set_uri(&song, LIT_ARGS("songs/file.flac")));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_item_to_string(&screen, &item, &text));
    assert(ncm_string_equal(text.data, text.len,
                            LIT_ARGS("classic:songs/file.flac")));

    Config.browser_display_mode = NCM_DISPLAY_MODE_COLUMNS;
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST, LIT_ARGS("Artist")));
    assert(ncm_song_add_tag(&song, MPD_TAG_TITLE, LIT_ARGS("Title")));
    assert(ncm_mpd_item_set_song(&item, &song));
    assert(native_browser_screen_item_to_string(&screen, &item, &text));
    assert(ncm_string_equal(text.data, text.len,
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
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("Browse: /")));

    assert(native_browser_screen_set_current_directory(
        &screen, LIT_ARGS("artists/alpha")));
    native_browser_screen_update_title_text(&screen);
    view = native_browser_screen_title_text(&screen);
    assert(ncm_string_equal(view.data, view.len,
                            LIT_ARGS("Browse: artists/alpha")));
    assert(ncm_string_equal(nc_screen_title(native_browser_screen_base(
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
    assert(ncm_string_equal(view.data, STRLIT_LEN("Browse: "),
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
           || !ncm_string_equal(view.data, view.len,
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
    columns[0].name = (char *)"Artist";
    columns[0].name_len = STRLIT_LEN("Artist");
    columns[0].type = (char *)"a";
    columns[0].type_len = STRLIT_LEN("a");
    columns[0].width = 10;
    columns[0].stretch_limit = -1;
    columns[0].fixed = true;
    columns[1] = (Column){0};
    columns[1].name = (char *)"Title";
    columns[1].name_len = STRLIT_LEN("Title");
    columns[1].type = (char *)"t";
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
    assert(ncm_string_equal(view.data, view.len,
                            LIT_ARGS("Artist    Title     ")));
    assert(ncm_string_equal(nc_window_title(&screen.window),
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
        ncm_memcpy(mpd_trace.paths[mpd_trace.calls], path, len);
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

    client->connection.server_error_code = (enum mpd_server_error)0;
    ncm_error_clear(error);
    if (ncm_string_equal(path, path_len, LIT_ARGS("/"))) {
        browser_mpd_trace_add_directory(items, LIT_ARGS("artist"));
    } else if (ncm_string_equal(path, path_len, LIT_ARGS("artist"))) {
        browser_mpd_trace_add_directory(items, LIT_ARGS("artist/album"));
        browser_mpd_trace_add_song(items, LIT_ARGS("artist/keep.flac"));
        browser_mpd_trace_add_song(items, LIT_ARGS("artist/drop.flac"));
    } else if (ncm_string_equal(path, path_len, LIT_ARGS("gone"))) {
        browser_mpd_trace_add_directory(items, LIT_ARGS("gone/child"));
    }
    return true;
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
    assert(ncm_string_equal(view.data, view.len, path, path_len));
    return;
}

static NcWindow *
test_bridge_active_window(void *user) {
    (void)user;
    bridge_callback_calls += 1;
    return NULL;
}

static void
test_bridge_refresh(void *user) {
    (void)user;
    bridge_callback_calls += 1;
    return;
}

static void
test_bridge_refresh_window(void *user) {
    (void)user;
    bridge_callback_calls += 1;
    return;
}

static void
test_bridge_scroll(void *user, enum NcScroll where) {
    (void)user;
    (void)where;
    bridge_callback_calls += 1;
    return;
}

static void
test_bridge_switch_to(void *user) {
    (void)user;
    bridge_callback_calls += 1;
    return;
}

static void
test_bridge_resize(void *user) {
    (void)user;
    bridge_callback_calls += 1;
    return;
}

static char *
test_bridge_title(void *user) {
    (void)user;
    bridge_callback_calls += 1;
    return (char *)"Bridge";
}

static void
test_bridge_update(void *user) {
    (void)user;
    bridge_callback_calls += 1;
    return;
}

static void
test_bridge_request_update(void *user) {
    (void)user;
    bridge_callback_calls += 1;
    return;
}

static void
test_bridge_mouse(void *user, MEVENT event) {
    (void)user;
    (void)event;
    bridge_callback_calls += 1;
    return;
}

static void
browser_parity_test_pending(char *name) {
    (void)name;
    return;
}

static void
test_browser_local_browsing_parity(void) {
    BROWSER_PARITY_TEST_PENDING(browser_local_browsing_parity);
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
