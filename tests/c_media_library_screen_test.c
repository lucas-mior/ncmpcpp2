#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_string.h"
#include "screens/nc_media_library.h"
#include "settings.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

typedef struct MediaLibraryHookFixture {
    int32 list_tags_calls;
    int32 list_all_songs_calls;
    int32 search_songs_calls;
    int32 add_songs_calls;
    int32 destroy_calls;
    int32 added_song_count;
    bool added_with_play;
    bool fail;
} MediaLibraryHookFixture;

static void test_media_library_state_contract(void);
static void test_media_library_tag_grouping(void);
static void test_media_library_mode_transitions(void);
static void test_media_library_selected_songs(void);
static void test_media_library_hooks(void);
static NativeMediaLibraryHooks test_hooks(MediaLibraryHookFixture *fixture);
static bool test_list_tags(void *user, enum mpd_tag_type tag_type,
                           NcmMpdStringList *tags, NcmError *error);
static bool test_list_all_songs(void *user, NcmMpdSongList *songs,
                                NcmError *error);
static bool test_search_songs(void *user,
                              NativeMediaLibrarySongQuery *query,
                              NcmMpdSongList *songs, NcmError *error);
static bool test_add_songs(void *user, NcmSongArray *songs, bool play,
                           NcmError *error);
static void test_destroy_hooks(void *user);
static bool test_append_song(NcmMpdSongList *songs, char *uri,
                             int32 uri_len);

int
main(void) {
    test_media_library_state_contract();
    test_media_library_tag_grouping();
    test_media_library_mode_transitions();
    test_media_library_selected_songs();
    test_media_library_hooks();
    exit(EXIT_SUCCESS);
}

static void
test_media_library_state_contract(void) {
    NativeMediaLibraryScreen screen;
    NativeMediaLibraryHooks hooks = {0};
    NativeMediaLibraryColumnState *tags_state;
    NativeMediaLibraryColumnState *albums_state;
    NcmTimePoint timer;
    NcmError error;
    bool old_data_fetching_delay;
    bool old_sort_by_mtime;
    bool old_titles_visibility;
    enum mpd_tag_type old_primary_tag;
    uint32 old_regex_type;

    old_data_fetching_delay = Config.data_fetching_delay;
    old_sort_by_mtime = Config.media_library_sort_by_mtime;
    old_titles_visibility = Config.titles_visibility;
    old_primary_tag = Config.media_lib_primary_tag;
    old_regex_type = Config.regex_type;
    Config.data_fetching_delay = true;
    Config.media_library_sort_by_mtime = false;
    Config.titles_visibility = true;
    Config.media_lib_primary_tag = MPD_TAG_ARTIST;
    Config.regex_type = NCM_REGEX_LITERAL_CASE_INSENSITIVE;

    native_media_library_screen_init(&screen, hooks, 0, 90, 0, 24,
                                     nc_color_default(),
                                     nc_border_none());
    assert(native_media_library_screen_mode(&screen)
           == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS);
    assert(native_media_library_screen_active_column(&screen)
           == NATIVE_MEDIA_LIBRARY_COLUMN_TAGS);
    assert(native_media_library_screen_fetching_delay_ms(&screen)
           == NATIVE_MEDIA_LIBRARY_FETCH_DELAY_MS);
    assert(native_media_library_screen_window_timeout_ms(&screen)
           == NATIVE_MEDIA_LIBRARY_FETCH_DELAY_MS);
    assert(ncm_string_equal(screen.tags_title.data,
                            screen.tags_title.len,
                            LIT_ARGS("Artists")));

    tags_state = native_media_library_screen_column_state(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS);
    albums_state = native_media_library_screen_column_state(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS);
    assert(tags_state);
    assert(albums_state);
    assert(tags_state != albums_state);
    assert(tags_state->filter_constraint.len == 0);
    assert(tags_state->search_constraint.len == 0);

    ncm_error_clear(&error);
    assert(native_media_library_screen_add_tag(
        &screen, LIT_ARGS("Artist A"), 0));
    assert(native_media_library_screen_add_tag(
        &screen, LIT_ARGS("Artist B"), 0));
    assert(native_media_library_screen_current_tag(&screen));
    assert(native_media_library_screen_add_album(
        &screen, LIT_ARGS("Artist A"), LIT_ARGS("Album A"),
        LIT_ARGS("2026"), 0, false));
    assert(native_media_library_screen_current_album(&screen));
    assert(native_media_library_screen_apply_filter(
        &screen, LIT_ARGS("Artist A"), &error));
    assert(tags_state->filter_enabled);
    assert(ncm_string_equal(tags_state->filter_constraint.data,
                            tags_state->filter_constraint.len,
                            LIT_ARGS("Artist A")));
    assert(native_media_library_screen_set_active_column(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS));
    assert(!albums_state->filter_enabled);
    assert(native_media_library_screen_search(
        &screen, LIT_ARGS("Album"), true, true, false, &error));
    assert(albums_state->search_enabled);
    assert(ncm_string_equal(albums_state->search_constraint.data,
                            albums_state->search_constraint.len,
                            LIT_ARGS("Album")));
    assert(!tags_state->search_enabled);

    timer.ns = 123456;
    native_media_library_screen_set_update_timer(&screen, timer);
    timer = native_media_library_screen_update_timer(&screen);
    assert(timer.ns == 123456);
    assert(!native_media_library_screen_sort_by_mtime(&screen));
    assert(native_media_library_screen_toggle_sort_mode(&screen));
    assert(native_media_library_screen_sort_by_mtime(&screen));

    native_media_library_screen_destroy(&screen);
    Config.data_fetching_delay = old_data_fetching_delay;
    Config.media_library_sort_by_mtime = old_sort_by_mtime;
    Config.titles_visibility = old_titles_visibility;
    Config.media_lib_primary_tag = old_primary_tag;
    Config.regex_type = old_regex_type;
    return;
}

static void
test_media_library_tag_grouping(void) {
    NativeMediaLibraryScreen screen;
    NativeMediaLibraryHooks hooks = {0};
    NcmSongArray songs;
    NcmSong song;
    NcMediaLibraryTagRow *row;

    native_media_library_screen_init(&screen, hooks, 0, 90, 0, 24,
                                     nc_color_default(),
                                     nc_border_none());
    ncm_song_array_init(&songs);
    ncm_song_init(&song);

    assert(ncm_song_set_uri(&song, LIT_ARGS("one.flac")));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST,
                            LIT_ARGS("Artist A")));
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("two.flac")));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST,
                            LIT_ARGS("Artist A")));
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("three.flac")));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST,
                            LIT_ARGS("Artist B")));
    assert(ncm_song_array_append_copy(&songs, &song));

    assert(native_media_library_screen_group_tags_from_songs(
        &screen, &songs, MPD_TAG_ARTIST));
    assert(nc_menu_item_count(nc_media_library_tag_menu_base(
               native_media_library_screen_tags(&screen))) == 2);
    row = nc_media_library_tag_menu_item_at(
        native_media_library_screen_tags(&screen), NC_MENU_ITEMS_ALL, 0);
    assert(row);
    assert(ncm_string_equal(row->tag, row->tag_len,
                            LIT_ARGS("Artist A")));

    ncm_song_destroy(&song);
    ncm_song_array_destroy(&songs);
    native_media_library_screen_destroy(&screen);
    return;
}

static void
test_media_library_mode_transitions(void) {
    NativeMediaLibraryScreen screen;
    NativeMediaLibraryHooks hooks = {0};

    native_media_library_screen_init(&screen, hooks, 0, 90, 0, 24,
                                     nc_color_default(),
                                     nc_border_none());
    assert(!native_media_library_screen_previous_column_available(&screen));
    assert(native_media_library_screen_next_column_available(&screen));
    native_media_library_screen_next_column(&screen);
    assert(native_media_library_screen_active_column(&screen)
           == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS);
    assert(native_media_library_screen_previous_column_available(&screen));

    assert(native_media_library_screen_set_mode(
        &screen, NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS));
    assert(native_media_library_screen_mode(&screen)
           == NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS);
    assert(!native_media_library_screen_set_active_column(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS));
    assert(native_media_library_screen_toggle_mode(&screen)
           == NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY);
    assert(native_media_library_screen_toggle_mode(&screen)
           == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS);
    assert(native_media_library_screen_set_active_column(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS));

    native_media_library_screen_destroy(&screen);
    return;
}

static void
test_media_library_selected_songs(void) {
    NativeMediaLibraryScreen screen;
    NativeMediaLibraryHooks hooks = {0};
    NcmSong song;
    NcmSongArray selected;
    NcmError error;

    native_media_library_screen_init(&screen, hooks, 0, 90, 0, 24,
                                     nc_color_default(),
                                     nc_border_none());
    ncm_song_init(&song);
    ncm_song_array_init(&selected);

    assert(ncm_song_set_uri(&song, LIT_ARGS("first.flac")));
    assert(native_media_library_screen_add_song_copy(&screen, &song));
    assert(ncm_song_set_uri(&song, LIT_ARGS("second.flac")));
    assert(native_media_library_screen_add_song_copy(&screen, &song));
    ncm_error_clear(&error);
    assert(native_media_library_screen_locate_song(
        &screen, &song, &error));
    assert(native_media_library_screen_visible_song_count(&screen) == 2);
    assert(native_media_library_screen_visible_song_at(&screen, 1));
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

static void
test_media_library_hooks(void) {
    NativeMediaLibraryScreen screen;
    MediaLibraryHookFixture fixture = {0};
    NativeMediaLibrarySongQuery query = {0};
    NativeMediaLibraryHooks hooks;
    NcmMpdStringList tags;
    NcmMpdSongList songs;
    NcmSongArray additions;
    NcmSong song;
    NcmError error;

    hooks = test_hooks(&fixture);
    native_media_library_screen_init(&screen, hooks, 0, 90, 0, 24,
                                     nc_color_default(),
                                     nc_border_none());
    ncm_mpd_string_list_init(&tags);
    ncm_mpd_song_list_init(&songs);
    ncm_song_array_init(&additions);
    ncm_song_init(&song);
    ncm_error_clear(&error);

    assert(native_media_library_screen_list_tags(
        &screen, MPD_TAG_ARTIST, &tags, &error));
    assert(fixture.list_tags_calls == 1);
    assert(ncm_mpd_string_list_count(&tags) == 2);
    assert(native_media_library_screen_list_all_songs(
        &screen, &songs, &error));
    assert(fixture.list_all_songs_calls == 1);
    assert(ncm_mpd_song_list_count(&songs) == 1);

    query.primary_tag = MPD_TAG_ARTIST;
    query.primary_value = (char *)"Artist A";
    query.primary_value_len = STRLIT_LEN("Artist A");
    query.match_primary_tag = true;
    assert(native_media_library_screen_search_songs(
        &screen, &query, &songs, &error));
    assert(fixture.search_songs_calls == 1);

    assert(ncm_song_set_uri(&song, LIT_ARGS("added.flac")));
    assert(ncm_song_array_append_copy(&additions, &song));
    assert(native_media_library_screen_add_songs(
        &screen, &additions, true, &error));
    assert(fixture.add_songs_calls == 1);
    assert(fixture.added_song_count == 1);
    assert(fixture.added_with_play);

    fixture.fail = true;
    ncm_error_clear(&error);
    assert(!native_media_library_screen_list_tags(
        &screen, MPD_TAG_ARTIST, &tags, &error));
    assert(error.code == EIO);

    ncm_song_destroy(&song);
    ncm_song_array_destroy(&additions);
    ncm_mpd_song_list_destroy(&songs);
    ncm_mpd_string_list_destroy(&tags);
    native_media_library_screen_destroy(&screen);
    assert(fixture.destroy_calls == 1);
    return;
}

static NativeMediaLibraryHooks
test_hooks(MediaLibraryHookFixture *fixture) {
    NativeMediaLibraryHooks hooks = {0};

    hooks.list_tags = test_list_tags;
    hooks.list_all_songs = test_list_all_songs;
    hooks.search_songs = test_search_songs;
    hooks.add_songs = test_add_songs;
    hooks.destroy = test_destroy_hooks;
    hooks.user = fixture;
    return hooks;
}

static bool
test_list_tags(void *user, enum mpd_tag_type tag_type,
               NcmMpdStringList *tags, NcmError *error) {
    MediaLibraryHookFixture *fixture;

    fixture = user;
    fixture->list_tags_calls += 1;
    if (fixture->fail) {
        ncm_error_set(error, EIO, STRLIT_ARGS("test tag error"));
        return false;
    }
    assert(tag_type == MPD_TAG_ARTIST);
    assert(ncm_mpd_string_list_append(tags, LIT_ARGS("Artist A")));
    assert(ncm_mpd_string_list_append(tags, LIT_ARGS("Artist B")));
    return true;
}

static bool
test_list_all_songs(void *user, NcmMpdSongList *songs,
                    NcmError *error) {
    MediaLibraryHookFixture *fixture;

    fixture = user;
    fixture->list_all_songs_calls += 1;
    if (fixture->fail) {
        ncm_error_set(error, EIO, STRLIT_ARGS("test song-list error"));
        return false;
    }
    return test_append_song(songs, LIT_ARGS("all.flac"));
}

static bool
test_search_songs(void *user, NativeMediaLibrarySongQuery *query,
                  NcmMpdSongList *songs, NcmError *error) {
    MediaLibraryHookFixture *fixture;

    fixture = user;
    fixture->search_songs_calls += 1;
    if (fixture->fail) {
        ncm_error_set(error, EIO, STRLIT_ARGS("test search error"));
        return false;
    }
    assert(query->match_primary_tag);
    assert(query->primary_tag == MPD_TAG_ARTIST);
    assert(ncm_string_equal(query->primary_value,
                            query->primary_value_len,
                            LIT_ARGS("Artist A")));
    return test_append_song(songs, LIT_ARGS("search.flac"));
}

static bool
test_add_songs(void *user, NcmSongArray *songs, bool play,
               NcmError *error) {
    MediaLibraryHookFixture *fixture;

    fixture = user;
    fixture->add_songs_calls += 1;
    if (fixture->fail) {
        ncm_error_set(error, EIO, STRLIT_ARGS("test add error"));
        return false;
    }
    fixture->added_song_count = songs->len;
    fixture->added_with_play = play;
    return true;
}

static void
test_destroy_hooks(void *user) {
    MediaLibraryHookFixture *fixture;

    fixture = user;
    fixture->destroy_calls += 1;
    return;
}

static bool
test_append_song(NcmMpdSongList *songs, char *uri, int32 uri_len) {
    NcmSong song;
    bool result;

    ncm_song_init(&song);
    result = ncm_song_set_uri(&song, uri, uri_len);
    if (result) {
        result = ncm_mpd_song_list_append_copy(songs, &song);
    }
    ncm_song_destroy(&song);
    return result;
}
