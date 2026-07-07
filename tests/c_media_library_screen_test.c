#include <assert.h>
#include <stdlib.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_string.h"
#include "screens/nc_media_library.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

static void test_media_library_tag_grouping(void);
static void test_media_library_column_transitions(void);
static void test_media_library_selected_songs(void);

int
main(void) {
    test_media_library_tag_grouping();
    test_media_library_column_transitions();
    test_media_library_selected_songs();
    return EXIT_SUCCESS;
}

static void
test_media_library_tag_grouping(void) {
    NativeMediaLibraryScreen screen;
    NcmSongArray songs;
    NcmSong song;
    NcMediaLibraryTagRow *row;

    native_media_library_screen_init(&screen, 0, 90, 0, 24,
                                     nc_color_default(),
                                     nc_border_none());
    ncm_song_array_init(&songs);
    ncm_song_init(&song);

    assert(ncm_song_set_uri(&song, LIT_ARGS("one.flac")));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST, LIT_ARGS("Artist A")));
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("two.flac")));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST, LIT_ARGS("Artist A")));
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("three.flac")));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST, LIT_ARGS("Artist B")));
    assert(ncm_song_array_append_copy(&songs, &song));

    assert(native_media_library_screen_group_tags_from_songs(
        &screen, &songs, MPD_TAG_ARTIST));
    assert(nc_menu_item_count(nc_media_library_tag_menu_base(
                                  native_media_library_screen_tags(&screen)))
           == 2);
    row = nc_media_library_tag_menu_item_at(
        native_media_library_screen_tags(&screen), NC_MENU_ITEMS_ALL, 0);
    assert(row != NULL);
    assert(ncm_string_equal(row->tag, row->tag_len, LIT_ARGS("Artist A")));

    ncm_song_destroy(&song);
    ncm_song_array_destroy(&songs);
    native_media_library_screen_destroy(&screen);
    return;
}

static void
test_media_library_column_transitions(void) {
    NativeMediaLibraryScreen screen;

    native_media_library_screen_init(&screen, 0, 90, 0, 24,
                                     nc_color_default(),
                                     nc_border_none());
    assert(native_media_library_screen_columns(&screen) == 3);
    assert(!native_media_library_screen_previous_column_available(&screen));
    assert(native_media_library_screen_next_column_available(&screen));
    native_media_library_screen_next_column(&screen);
    assert(native_media_library_screen_previous_column_available(&screen));
    native_media_library_screen_set_columns(&screen, 2);
    assert(native_media_library_screen_columns(&screen) == 2);

    native_media_library_screen_destroy(&screen);
    return;
}

static void
test_media_library_selected_songs(void) {
    NativeMediaLibraryScreen screen;
    NcmSong song;
    NcmSongArray selected;

    native_media_library_screen_init(&screen, 0, 90, 0, 24,
                                     nc_color_default(),
                                     nc_border_none());
    ncm_song_init(&song);
    ncm_song_array_init(&selected);

    assert(ncm_song_set_uri(&song, LIT_ARGS("first.flac")));
    assert(native_media_library_screen_add_song_copy(&screen, &song));
    assert(ncm_song_set_uri(&song, LIT_ARGS("second.flac")));
    assert(native_media_library_screen_add_song_copy(&screen, &song));
    assert(nc_menu_set_position_selected(
        nc_media_library_song_menu_base(
            native_media_library_screen_songs(&screen)),
        1, true));
    assert(native_media_library_screen_selected_songs(&screen, &selected));
    assert(selected.len == 1);

    ncm_song_array_destroy(&selected);
    ncm_song_destroy(&song);
    native_media_library_screen_destroy(&screen);
    return;
}
