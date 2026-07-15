#include <assert.h>
#include <stdlib.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_string.h"
#include "screens/nc_browser.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)
#define BROWSER_PARITY_TEST_PENDING(NAME) browser_parity_test_pending(#NAME)

static void test_browser_path_navigation(void);
static void test_browser_selected_songs(void);
static void test_browser_filter_and_search(void);
static void test_browser_local_mode(void);
static void test_browser_local_browsing_parity(void);
static void test_browser_directory_expansion_parity(void);
static void test_browser_playlist_expansion_parity(void);
static void test_browser_deletion_parity(void);
static void test_browser_filter_search_formatting_parity(void);
static void test_browser_sort_modes_parity(void);
static void test_browser_jump_to_playing_song_parity(void);
static void browser_parity_test_pending(char *name);

int
main(void) {
    test_browser_path_navigation();
    test_browser_selected_songs();
    test_browser_filter_and_search();
    test_browser_local_mode();
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
