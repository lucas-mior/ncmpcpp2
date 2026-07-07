#include <assert.h>
#include <stdlib.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_string.h"
#include "screens/nc_search_engine.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

static void test_search_query_construction(void);
static void test_search_result_row_ownership(void);
static void test_search_selected_songs(void);

int
main(void) {
    test_search_query_construction();
    test_search_result_row_ownership();
    test_search_selected_songs();
    return EXIT_SUCCESS;
}

static void
test_search_query_construction(void) {
    NativeSearchEngineScreen screen;
    NcmBuffer query;

    native_search_engine_screen_init(&screen, 0, 80, 0, 24,
                                     nc_color_default(),
                                     nc_border_none());
    ncm_buffer_init(&query);

    assert(native_search_engine_screen_set_constraint(&screen, 0,
                                                      LIT_ARGS("artist A")));
    assert(native_search_engine_screen_set_constraint(&screen, 2,
                                                      LIT_ARGS("album B")));
    assert(native_search_engine_screen_build_query(&screen, &query));
    assert(ncm_string_equal(query.data, query.len,
                            LIT_ARGS("artist A album B")));

    ncm_buffer_destroy(&query);
    native_search_engine_screen_destroy(&screen);
    return;
}

static void
test_search_result_row_ownership(void) {
    NativeSearchEngineScreen screen;
    NcmSong song;
    NcSearchRow *row;
    NcmStringView view;

    native_search_engine_screen_init(&screen, 0, 80, 0, 24,
                                     nc_color_default(),
                                     nc_border_none());
    ncm_song_init(&song);

    assert(ncm_song_set_uri(&song, LIT_ARGS("owned.flac")));
    assert(native_search_engine_screen_add_song_copy(&screen, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);

    row = nc_search_row_menu_item_at(native_search_engine_screen_rows(&screen),
                                     NC_MENU_ITEMS_ALL, 0);
    assert(row != NULL);
    assert(row->is_song);
    assert(ncm_song_uri_view(&row->song, 0, &view));
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("owned.flac")));

    ncm_song_destroy(&song);
    native_search_engine_screen_destroy(&screen);
    return;
}

static void
test_search_selected_songs(void) {
    NativeSearchEngineScreen screen;
    NcmSong song;
    NcmSongArray songs;

    native_search_engine_screen_init(&screen, 0, 80, 0, 24,
                                     nc_color_default(),
                                     nc_border_none());
    ncm_song_init(&song);
    ncm_song_array_init(&songs);

    assert(ncm_song_set_uri(&song, LIT_ARGS("first.flac")));
    assert(native_search_engine_screen_add_song_copy(&screen, &song));
    assert(ncm_song_set_uri(&song, LIT_ARGS("second.flac")));
    assert(native_search_engine_screen_add_song_copy(&screen, &song));
    assert(nc_menu_set_position_selected(
        native_search_engine_screen_menu(&screen), 1, true));
    assert(native_search_engine_screen_selected_songs(&screen, &songs));
    assert(songs.len == 1);

    ncm_song_array_destroy(&songs);
    ncm_song_destroy(&song);
    native_search_engine_screen_destroy(&screen);
    return;
}
