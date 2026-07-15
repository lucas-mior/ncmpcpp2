#include <assert.h>
#include <stdlib.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_base.h"
#include "c/ncm_string.h"
#include "screens/nc_browser.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)
#define BROWSER_PARITY_TEST_PENDING(NAME) browser_parity_test_pending(#NAME)

static void test_browser_path_navigation(void);
static void test_browser_selected_songs(void);
static void test_browser_filter_and_search(void);
static void test_browser_local_mode(void);
static void test_browser_owned_state(void);
static void test_browser_native_callbacks_ignore_bridge(void);
static void test_browser_local_browsing_parity(void);
static void test_browser_directory_expansion_parity(void);
static void test_browser_playlist_expansion_parity(void);
static void test_browser_deletion_parity(void);
static void test_browser_filter_search_formatting_parity(void);
static void test_browser_sort_modes_parity(void);
static void test_browser_jump_to_playing_song_parity(void);
static void browser_parity_test_pending(char *name);
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

static int32 bridge_callback_calls;

int
main(void) {
    test_browser_path_navigation();
    test_browser_selected_songs();
    test_browser_filter_and_search();
    test_browser_local_mode();
    test_browser_owned_state();
    test_browser_native_callbacks_ignore_bridge();
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
    NcmMpdItem item;
    NcmSong song;
    NcmError error = {0};

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
    assert(nc_menu_highlight(native_browser_screen_menu(&screen)) == 2);

    ncm_song_destroy(&song);
    ncm_mpd_item_destroy(&item);
    native_browser_screen_destroy(&screen);
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
    BROWSER_PARITY_TEST_PENDING(browser_filter_search_formatting_parity);
    return;
}

static void
test_browser_sort_modes_parity(void) {
    BROWSER_PARITY_TEST_PENDING(browser_sort_modes_parity);
    return;
}

static void
test_browser_jump_to_playing_song_parity(void) {
    BROWSER_PARITY_TEST_PENDING(browser_jump_to_playing_song_parity);
    return;
}
