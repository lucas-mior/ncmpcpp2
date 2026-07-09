#include <assert.h>
#include <stdlib.h>

#include "c/ncm_string.h"
#include "curses/nc_app_menus.h"
#include "screens/nc_playlist.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

static bool uri_contains_filter(NcMenu *menu, void *item, void *user);
static void test_playlist_row_ownership(void);
static void test_playlist_selection_range(void);
static void test_playlist_current_song_lookup(void);
static void test_playlist_sparse_now_playing_lookup(void);
static void test_playlist_now_playing_lookup_does_not_sync(void);
static void test_playlist_filter_reapplication(void);

int
main(void) {
    test_playlist_row_ownership();
    test_playlist_selection_range();
    test_playlist_current_song_lookup();
    test_playlist_sparse_now_playing_lookup();
    test_playlist_now_playing_lookup_does_not_sync();
    test_playlist_filter_reapplication();
    return EXIT_SUCCESS;
}

static bool
uri_contains_filter(NcMenu *menu, void *item, void *user) {
    NcmSong *song;
    NcmStringView uri;
    NcmStringView needle;

    (void)menu;
    song = item;
    needle = *(NcmStringView *)user;
    if (!ncm_song_uri_view(song, 0, &uri)) {
        return false;
    }
    if (uri.len < needle.len) {
        return false;
    }
    return ncm_string_equal(uri.data, needle.len, needle.data,
                            needle.len);
}

static void
test_playlist_row_ownership(void) {
    NcSongMenu songs;
    NcmSong song;
    NcmSong *stored;
    NcmStringView view;

    nc_song_menu_init(&songs);
    ncm_song_init(&song);

    assert(ncm_song_set_uri(&song, LIT_ARGS("owned-one.flac")));
    nc_song_menu_add(&songs, &song);
    ncm_song_destroy(&song);
    ncm_song_init(&song);

    stored = nc_song_menu_item_at(&songs, NC_MENU_ITEMS_ALL, 0);
    assert(stored != NULL);
    assert(ncm_song_uri_view(stored, 0, &view));
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("owned-one.flac")));

    ncm_song_destroy(&song);
    nc_song_menu_destroy(&songs);
    return;
}

static void
test_playlist_selection_range(void) {
    NcPlaylistScreen screen;
    NcScreenCallbacks callbacks = {0};
    NcSongMenu songs;
    NcmSong song;

    nc_song_menu_init(&songs);
    ncm_song_init(&song);
    nc_playlist_screen_init(&screen, callbacks, NULL,
                            nc_song_menu_base(&songs), 0, 80, 0, 10);

    for (int32 i = 0; i < 4; i += 1) {
        assert(ncm_song_set_uri(&song, LIT_ARGS("range.flac")));
        ncm_song_set_position(&song, (uint32)i);
        nc_song_menu_add(&songs, &song);
    }

    assert(nc_menu_select_range(nc_song_menu_base(&songs), 1, 3, true));
    assert(!nc_menu_position_is_selected(nc_song_menu_base(&songs), 0));
    assert(nc_menu_position_is_selected(nc_song_menu_base(&songs), 1));
    assert(nc_menu_position_is_selected(nc_song_menu_base(&songs), 2));
    assert(nc_menu_position_is_selected(nc_song_menu_base(&songs), 3));

    ncm_song_destroy(&song);
    nc_song_menu_destroy(&songs);
    return;
}

static void
test_playlist_current_song_lookup(void) {
    NcSongMenu songs;
    NcmSong song;
    NcmSong *current;
    NcmStringView view;

    nc_song_menu_init(&songs);
    ncm_song_init(&song);

    assert(ncm_song_set_uri(&song, LIT_ARGS("first.flac")));
    nc_song_menu_add(&songs, &song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("second.flac")));
    nc_song_menu_add(&songs, &song);
    assert(nc_menu_goto_selectable(nc_song_menu_base(&songs), 1));

    current = nc_song_menu_current(&songs);
    assert(current != NULL);
    assert(ncm_song_uri_view(current, 0, &view));
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("second.flac")));

    ncm_song_destroy(&song);
    nc_song_menu_destroy(&songs);
    return;
}

static void
test_playlist_sparse_now_playing_lookup(void) {
    NativePlaylistScreen screen = {0};
    NcmSong found;
    NcmSong song;
    NcmStringView view;
    uint32 sparse_position;

    sparse_position = 8314;
    nc_song_menu_init(&screen.songs);
    ncm_song_init(&found);
    ncm_song_init(&song);

    assert(ncm_song_set_uri(&song, LIT_ARGS("sparse.flac")));
    ncm_song_set_position(&song, sparse_position);
    nc_song_menu_add(&screen.songs, &song);

    assert(native_playlist_screen_now_playing_song(
        &screen, (int32)sparse_position, &found));
    assert(ncm_song_position(&found) == sparse_position);
    assert(ncm_song_uri_view(&found, 0, &view));
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("sparse.flac")));
    assert(!native_playlist_screen_now_playing_song(&screen, 1, &found));

    ncm_song_destroy(&song);
    ncm_song_destroy(&found);
    nc_song_menu_destroy(&screen.songs);
    return;
}

static void
test_playlist_now_playing_sync_callback(void *user) {
    bool *called;

    called = user;
    *called = true;
    return;
}

static void
test_playlist_now_playing_lookup_does_not_sync(void) {
    NativePlaylistScreen screen = {0};
    NcmSong found;
    NcmSong song;
    bool sync_called;

    sync_called = false;
    nc_song_menu_init(&screen.songs);
    ncm_song_init(&found);
    ncm_song_init(&song);

    assert(ncm_song_set_uri(&song, LIT_ARGS("current.flac")));
    ncm_song_set_position(&song, 42);
    nc_song_menu_add(&screen.songs, &song);
    native_playlist_screen_set_sync_callback(
        &screen, test_playlist_now_playing_sync_callback, &sync_called);

    assert(native_playlist_screen_now_playing_song(&screen, 42, &found));
    assert(!sync_called);

    ncm_song_destroy(&song);
    ncm_song_destroy(&found);
    nc_song_menu_destroy(&screen.songs);
    return;
}

static void
test_playlist_filter_reapplication(void) {
    NcMenuDisplayCallbacks display_callbacks = {0};
    NcSongMenu songs;
    NcmSong song;
    NcmStringView filter;

    nc_song_menu_init(&songs);
    ncm_song_init(&song);

    assert(ncm_song_set_uri(&song, LIT_ARGS("keep-one.flac")));
    nc_song_menu_add(&songs, &song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("drop.flac")));
    nc_song_menu_add(&songs, &song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("keep-two.flac")));
    nc_song_menu_add(&songs, &song);

    filter = ncm_string_view_make(LIT_ARGS("keep"));
    display_callbacks.filter = uri_contains_filter;
    display_callbacks.user = &filter;
    nc_menu_set_display_callbacks(nc_song_menu_base(&songs),
                                  display_callbacks);
    nc_menu_apply_filter(nc_song_menu_base(&songs));
    assert(nc_menu_item_count(nc_song_menu_base(&songs)) == 2);
    nc_menu_show_all_items(nc_song_menu_base(&songs));
    assert(nc_menu_item_count(nc_song_menu_base(&songs)) == 3);
    nc_menu_apply_filter(nc_song_menu_base(&songs));
    assert(nc_menu_item_count(nc_song_menu_base(&songs)) == 2);

    ncm_song_destroy(&song);
    nc_song_menu_destroy(&songs);
    return;
}
