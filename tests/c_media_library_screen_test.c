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
static void test_media_library_layout_and_rendering(void);
static void test_media_library_tag_grouping(void);
static void test_media_library_tag_primitives(void);
static void test_media_library_album_primitives(void);
static void test_media_library_song_ordering(void);
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
static bool test_set_library_song(
    NcmSong *song, char *uri, int32 uri_len,
    char *album, int32 album_len, char *date, int32 date_len,
    char *disc, int32 disc_len, char *track, int32 track_len,
    char *title, int32 title_len, time_t mtime);

int
main(void) {
    test_media_library_state_contract();
    test_media_library_layout_and_rendering();
    test_media_library_tag_grouping();
    test_media_library_tag_primitives();
    test_media_library_album_primitives();
    test_media_library_song_ordering();
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
test_media_library_layout_and_rendering(void) {
    NativeMediaLibraryScreen screen;
    NativeMediaLibraryHooks hooks = {0};
    NcMediaLibraryTagRow tag = {0};
    NcMediaLibraryAlbumRow album = {0};
    NcmInt32Array old_three_ratios;
    NcmInt32Array old_two_ratios;
    NcBuffer old_selected_prefix;
    NcBuffer old_selected_suffix;
    NcBuffer old_current_prefix;
    NcBuffer old_current_suffix;
    NcBuffer old_inactive_prefix;
    NcBuffer old_inactive_suffix;
    NcmFormatAst old_song_format;
    NcmBuffer text;
    NcBuffer song_text;
    NcmSong song;
    NcMenu *tags;
    NcMenu *albums;
    NcMenu *songs;
    int32 *ratio;
    char *old_empty_tag;
    int32 old_empty_tag_len;
    int32 old_empty_tag_cap;
    bool old_titles_visibility;
    bool old_centered_cursor;
    bool old_cyclic_scrolling;
    bool old_sort_by_mtime;
    bool old_hide_album_dates;
    enum mpd_tag_type old_primary_tag;

    old_three_ratios = Config.media_library_column_width_ratio_three;
    old_two_ratios = Config.media_library_column_width_ratio_two;
    old_selected_prefix = Config.selected_item_prefix;
    old_selected_suffix = Config.selected_item_suffix;
    old_current_prefix = Config.current_item_prefix;
    old_current_suffix = Config.current_item_suffix;
    old_inactive_prefix = Config.current_item_inactive_column_prefix;
    old_inactive_suffix = Config.current_item_inactive_column_suffix;
    old_song_format = Config.song_library_format;
    old_empty_tag = Config.empty_tag;
    old_empty_tag_len = Config.empty_tag_len;
    old_empty_tag_cap = Config.empty_tag_cap;
    old_titles_visibility = Config.titles_visibility;
    old_centered_cursor = Config.centered_cursor;
    old_cyclic_scrolling = Config.use_cyclic_scrolling;
    old_sort_by_mtime = Config.media_library_sort_by_mtime;
    old_hide_album_dates = Config.media_lib_hide_album_dates;
    old_primary_tag = Config.media_lib_primary_tag;

    ncm_int32_array_init(
        &Config.media_library_column_width_ratio_three);
    ratio = ncm_int32_array_append(
        &Config.media_library_column_width_ratio_three);
    assert(ratio);
    *ratio = 2;
    ratio = ncm_int32_array_append(
        &Config.media_library_column_width_ratio_three);
    assert(ratio);
    *ratio = 3;
    ratio = ncm_int32_array_append(
        &Config.media_library_column_width_ratio_three);
    assert(ratio);
    *ratio = 5;
    ncm_int32_array_init(&Config.media_library_column_width_ratio_two);
    ratio = ncm_int32_array_append(
        &Config.media_library_column_width_ratio_two);
    assert(ratio);
    *ratio = 3;
    ratio = ncm_int32_array_append(
        &Config.media_library_column_width_ratio_two);
    assert(ratio);
    *ratio = 7;

    nc_buffer_init(&Config.selected_item_prefix);
    nc_buffer_append_data(&Config.selected_item_prefix,
                          LIT_ARGS("<selected>"));
    nc_buffer_init(&Config.selected_item_suffix);
    nc_buffer_append_data(&Config.selected_item_suffix,
                          LIT_ARGS("</selected>"));
    nc_buffer_init(&Config.current_item_prefix);
    nc_buffer_append_data(&Config.current_item_prefix,
                          LIT_ARGS("<active>"));
    nc_buffer_init(&Config.current_item_suffix);
    nc_buffer_append_data(&Config.current_item_suffix,
                          LIT_ARGS("</active>"));
    nc_buffer_init(&Config.current_item_inactive_column_prefix);
    nc_buffer_append_data(
        &Config.current_item_inactive_column_prefix,
        LIT_ARGS("<inactive>"));
    nc_buffer_init(&Config.current_item_inactive_column_suffix);
    nc_buffer_append_data(
        &Config.current_item_inactive_column_suffix,
        LIT_ARGS("</inactive>"));
    ncm_format_ast_init(&Config.song_library_format);
    assert(ncm_format_ast_append_text(&Config.song_library_format,
                                      LIT_ARGS("library row")));

    Config.empty_tag = (char *)"<empty>";
    Config.empty_tag_len = STRLIT_LEN("<empty>");
    Config.empty_tag_cap = 0;
    Config.titles_visibility = true;
    Config.centered_cursor = true;
    Config.use_cyclic_scrolling = true;
    Config.media_library_sort_by_mtime = false;
    Config.media_lib_hide_album_dates = false;
    Config.media_lib_primary_tag = MPD_TAG_ARTIST;

    native_media_library_screen_init(&screen, hooks, 7, 100, 3, 20,
                                     nc_color_default(),
                                     nc_border_none());
    tags = nc_media_library_tag_menu_base(&screen.tags);
    albums = nc_media_library_album_menu_base(&screen.albums);
    songs = nc_media_library_song_menu_base(&screen.songs);

    assert(tags->cyclic_scroll_enabled);
    assert(albums->cyclic_scroll_enabled);
    assert(songs->cyclic_scroll_enabled);
    assert(tags->autocenter_cursor);
    assert(albums->autocenter_cursor);
    assert(songs->autocenter_cursor);
    assert(nc_buffer_equal(&tags->selected_prefix,
                           &Config.selected_item_prefix));
    assert(nc_buffer_equal(&albums->selected_suffix,
                           &Config.selected_item_suffix));
    assert(nc_buffer_equal(&songs->selected_prefix,
                           &Config.selected_item_prefix));
    assert(nc_buffer_equal(&tags->highlight_prefix,
                           &Config.current_item_prefix));
    assert(nc_buffer_equal(
        &albums->highlight_prefix,
        &Config.current_item_inactive_column_prefix));
    assert(nc_buffer_equal(
        &songs->highlight_suffix,
        &Config.current_item_inactive_column_suffix));
    assert(tags->display_callbacks.draw);
    assert(albums->display_callbacks.draw);
    assert(songs->display_callbacks.draw);

    assert(nc_window_start_x(&screen.tags_window) == 7);
    assert(nc_window_width(&screen.tags_window) == 19);
    assert(nc_window_start_x(&screen.albums_window) == 27);
    assert(nc_window_width(&screen.albums_window) == 30);
    assert(nc_window_start_x(&screen.songs_window) == 58);
    assert(nc_window_width(&screen.songs_window) == 49);
    assert(native_media_library_screen_column_visible(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS));
    assert(ncm_string_equal(screen.albums_title.data,
                            screen.albums_title.len,
                            LIT_ARGS("Albums")));

    assert(native_media_library_screen_set_mode(
        &screen, NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS));
    assert(!native_media_library_screen_column_visible(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS));
    assert(nc_window_start_x(&screen.albums_window) == 7);
    assert(nc_window_width(&screen.albums_window) == 30);
    assert(nc_window_start_x(&screen.songs_window) == 38);
    assert(nc_window_width(&screen.songs_window) == 69);
    assert(ncm_string_equal(
        screen.albums_title.data, screen.albums_title.len,
        LIT_ARGS("Albums (sorted by artist)")));
    assert(nc_buffer_equal(&albums->highlight_prefix,
                           &Config.current_item_prefix));
    assert(nc_buffer_equal(
        &tags->highlight_prefix,
        &Config.current_item_inactive_column_prefix));

    assert(native_media_library_screen_toggle_sort_mode(&screen));
    assert(ncm_string_equal(
        screen.albums_title.data, screen.albums_title.len,
        LIT_ARGS("Albums (sorted by artist and mtime)")));
    assert(native_media_library_screen_set_mode(
        &screen, NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY));
    assert(ncm_string_equal(
        screen.albums_title.data, screen.albums_title.len,
        LIT_ARGS("Albums (sorted by mtime)")));
    assert(!native_media_library_screen_toggle_sort_mode(&screen));
    assert(ncm_string_equal(screen.albums_title.data,
                            screen.albums_title.len,
                            LIT_ARGS("Albums")));

    ncm_buffer_init(&text);
    tag.tag = (char *)"Artist";
    tag.tag_len = STRLIT_LEN("Artist");
    native_media_library_screen_format_tag_row(&screen, &tag, &text);
    assert(ncm_string_equal(text.data, text.len, LIT_ARGS("Artist")));
    tag.tag = NULL;
    tag.tag_len = 0;
    native_media_library_screen_format_tag_row(&screen, &tag, &text);
    assert(ncm_string_equal(text.data, text.len, LIT_ARGS("<empty>")));

    album.tag = (char *)"Artist";
    album.tag_len = STRLIT_LEN("Artist");
    album.album = (char *)"Album";
    album.album_len = STRLIT_LEN("Album");
    album.date = (char *)"2026";
    album.date_len = STRLIT_LEN("2026");
    native_media_library_screen_format_album_row(&screen, &album,
                                                  &text);
    assert(ncm_string_equal(text.data, text.len,
                            LIT_ARGS("(2026) Album")));
    assert(native_media_library_screen_set_mode(
        &screen, NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS));
    native_media_library_screen_format_album_row(&screen, &album,
                                                  &text);
    assert(ncm_string_equal(
        text.data, text.len, LIT_ARGS("Artist - (2026) Album")));
    album.tag = NULL;
    album.tag_len = 0;
    album.album = NULL;
    album.album_len = 0;
    native_media_library_screen_format_album_row(&screen, &album,
                                                  &text);
    assert(ncm_string_equal(
        text.data, text.len,
        LIT_ARGS("<empty> - (2026) <no album>")));
    album.all_tracks_entry = true;
    native_media_library_screen_format_album_row(&screen, &album,
                                                  &text);
    assert(ncm_string_equal(text.data, text.len,
                            LIT_ARGS("All tracks")));
    album.all_tracks_entry = false;

    ncm_song_init(&song);
    nc_buffer_init(&song_text);
    assert(ncm_song_set_uri(&song, LIT_ARGS("song.flac")));
    native_media_library_screen_format_song_row(&screen, &song,
                                                 &song_text);
    assert(ncm_string_equal(song_text.data, song_text.len,
                            LIT_ARGS("library row")));

    nc_buffer_destroy(&song_text);
    ncm_song_destroy(&song);
    ncm_buffer_destroy(&text);
    native_media_library_screen_destroy(&screen);

    ncm_int32_array_destroy(
        &Config.media_library_column_width_ratio_three);
    ncm_int32_array_destroy(&Config.media_library_column_width_ratio_two);
    nc_buffer_destroy(&Config.selected_item_prefix);
    nc_buffer_destroy(&Config.selected_item_suffix);
    nc_buffer_destroy(&Config.current_item_prefix);
    nc_buffer_destroy(&Config.current_item_suffix);
    nc_buffer_destroy(&Config.current_item_inactive_column_prefix);
    nc_buffer_destroy(&Config.current_item_inactive_column_suffix);
    ncm_format_ast_destroy(&Config.song_library_format);

    Config.media_library_column_width_ratio_three = old_three_ratios;
    Config.media_library_column_width_ratio_two = old_two_ratios;
    Config.selected_item_prefix = old_selected_prefix;
    Config.selected_item_suffix = old_selected_suffix;
    Config.current_item_prefix = old_current_prefix;
    Config.current_item_suffix = old_current_suffix;
    Config.current_item_inactive_column_prefix = old_inactive_prefix;
    Config.current_item_inactive_column_suffix = old_inactive_suffix;
    Config.song_library_format = old_song_format;
    Config.empty_tag = old_empty_tag;
    Config.empty_tag_len = old_empty_tag_len;
    Config.empty_tag_cap = old_empty_tag_cap;
    Config.titles_visibility = old_titles_visibility;
    Config.centered_cursor = old_centered_cursor;
    Config.use_cyclic_scrolling = old_cyclic_scrolling;
    Config.media_library_sort_by_mtime = old_sort_by_mtime;
    Config.media_lib_hide_album_dates = old_hide_album_dates;
    Config.media_lib_primary_tag = old_primary_tag;
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
test_media_library_tag_primitives(void) {
    NativeMediaLibraryTagArray tags;
    NcmMpdStringList strings;
    NcmMpdSongList songs;
    NcmSong song;
    bool old_ignore_leading_the;
    bool old_sort_by_mtime;

    old_ignore_leading_the = Config.ignore_leading_the;
    old_sort_by_mtime = Config.media_library_sort_by_mtime;
    Config.ignore_leading_the = true;
    Config.media_library_sort_by_mtime = false;

    native_media_library_tag_array_init(&tags);
    ncm_mpd_string_list_init(&strings);
    ncm_mpd_song_list_init(&songs);
    ncm_song_init(&song);

    assert(ncm_mpd_string_list_append(
        &strings, LIT_ARGS("The Zebra")));
    assert(ncm_mpd_string_list_append(&strings, LIT_ARGS("Yellow")));
    assert(ncm_mpd_string_list_append(&strings, LIT_ARGS("")));
    assert(ncm_mpd_string_list_append(&strings, LIT_ARGS("Yellow")));
    assert(native_media_library_tags_from_strings(&tags, &strings));
    assert(tags.len == 3);
    assert(tags.items[0].tag_len == 0);
    assert(ncm_string_equal(tags.items[1].tag, tags.items[1].tag_len,
                            LIT_ARGS("Yellow")));
    assert(ncm_string_equal(tags.items[2].tag, tags.items[2].tag_len,
                            LIT_ARGS("The Zebra")));

    assert(ncm_song_set_uri(&song, LIT_ARGS("first.flac")));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST,
                            LIT_ARGS("First")));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST,
                            LIT_ARGS("Second")));
    ncm_song_set_mtime(&song, 10);
    assert(ncm_mpd_song_list_append_copy(&songs, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);

    assert(ncm_song_set_uri(&song, LIT_ARGS("second.flac")));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST,
                            LIT_ARGS("First")));
    ncm_song_set_mtime(&song, 20);
    assert(ncm_mpd_song_list_append_copy(&songs, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);

    assert(ncm_song_set_uri(&song, LIT_ARGS("empty.flac")));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST, LIT_ARGS("")));
    ncm_song_set_mtime(&song, 15);
    assert(ncm_mpd_song_list_append_copy(&songs, &song));

    Config.media_library_sort_by_mtime = true;
    assert(native_media_library_tags_from_songs(
        &tags, &songs, MPD_TAG_ARTIST));
    assert(tags.len == 3);
    assert(ncm_string_equal(tags.items[0].tag, tags.items[0].tag_len,
                            LIT_ARGS("First")));
    assert(tags.items[0].mtime == 20);
    assert(tags.items[1].tag_len == 0);
    assert(tags.items[1].mtime == 15);
    assert(ncm_string_equal(tags.items[2].tag, tags.items[2].tag_len,
                            LIT_ARGS("Second")));
    assert(tags.items[2].mtime == 10);

    ncm_song_destroy(&song);
    ncm_mpd_song_list_destroy(&songs);
    ncm_mpd_string_list_destroy(&strings);
    native_media_library_tag_array_destroy(&tags);
    Config.ignore_leading_the = old_ignore_leading_the;
    Config.media_library_sort_by_mtime = old_sort_by_mtime;
    return;
}

static void
test_media_library_album_primitives(void) {
    NativeMediaLibraryAlbumArray albums;
    NcmMpdSongList three_column_songs;
    NcmMpdSongList two_column_songs;
    NcmSong song;
    NcMediaLibraryAlbumRow *row;
    bool old_ignore_leading_the;
    bool old_sort_by_mtime;
    bool old_split_by_date;

    old_ignore_leading_the = Config.ignore_leading_the;
    old_sort_by_mtime = Config.media_library_sort_by_mtime;
    old_split_by_date = Config.media_library_albums_split_by_date;
    Config.ignore_leading_the = true;
    Config.media_library_sort_by_mtime = false;
    Config.media_library_albums_split_by_date = false;

    native_media_library_album_array_init(&albums);
    ncm_mpd_song_list_init(&three_column_songs);
    ncm_mpd_song_list_init(&two_column_songs);
    ncm_song_init(&song);

    assert(test_set_library_song(
        &song, LIT_ARGS("one.flac"), LIT_ARGS("The Zebra"),
        LIT_ARGS("2020"), NULL, 0, NULL, 0, NULL, 0, 100));
    assert(ncm_mpd_song_list_append_copy(&three_column_songs, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(test_set_library_song(
        &song, LIT_ARGS("two.flac"), LIT_ARGS("The Zebra"),
        LIT_ARGS("2020"), NULL, 0, NULL, 0, NULL, 0, 200));
    assert(ncm_mpd_song_list_append_copy(&three_column_songs, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(test_set_library_song(
        &song, LIT_ARGS("three.flac"), LIT_ARGS("The Zebra"),
        LIT_ARGS("2021"), NULL, 0, NULL, 0, NULL, 0, 150));
    assert(ncm_mpd_song_list_append_copy(&three_column_songs, &song));

    assert(native_media_library_albums_from_songs(
        &albums, &three_column_songs,
        NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS, MPD_TAG_ARTIST,
        LIT_ARGS("Artist")));
    assert(albums.len == 1);
    row = &albums.items[0].row;
    assert(ncm_string_equal(row->album, row->album_len,
                            LIT_ARGS("The Zebra")));
    assert(row->date_len == 0);
    assert(row->mtime == 200);
    assert(!row->all_tracks_entry);

    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(test_set_library_song(
        &song, LIT_ARGS("yellow.flac"), LIT_ARGS("Yellow"),
        LIT_ARGS("2020"), NULL, 0, NULL, 0, NULL, 0, 50));
    assert(ncm_mpd_song_list_append_copy(&three_column_songs, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(test_set_library_song(
        &song, LIT_ARGS("empty-album.flac"), LIT_ARGS(""),
        LIT_ARGS("2020"), NULL, 0, NULL, 0, NULL, 0, 60));
    assert(ncm_mpd_song_list_append_copy(&three_column_songs, &song));

    Config.media_library_albums_split_by_date = true;
    assert(native_media_library_albums_from_songs(
        &albums, &three_column_songs,
        NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS, MPD_TAG_ARTIST,
        LIT_ARGS("Artist")));
    assert(albums.len == 6);
    assert(albums.items[0].row.album_len == 0);
    assert(ncm_string_equal(albums.items[1].row.album,
                            albums.items[1].row.album_len,
                            LIT_ARGS("Yellow")));
    assert(ncm_string_equal(albums.items[2].row.album,
                            albums.items[2].row.album_len,
                            LIT_ARGS("The Zebra")));
    assert(albums.items[2].row.mtime == 200);
    assert(ncm_string_equal(albums.items[3].row.date,
                            albums.items[3].row.date_len,
                            LIT_ARGS("2021")));
    assert(albums.items[4].menu_flags == NC_MENU_ITEM_SEPARATOR);
    assert(albums.items[5].row.all_tracks_entry);
    assert(ncm_string_equal(albums.items[5].row.tag,
                            albums.items[5].row.tag_len,
                            LIT_ARGS("Artist")));

    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(test_set_library_song(
        &song, LIT_ARGS("recursive-one.flac"), LIT_ARGS("Album"),
        LIT_ARGS("2020"), NULL, 0, NULL, 0, NULL, 0, 300));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST,
                            LIT_ARGS("The Zebra")));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST,
                            LIT_ARGS("Second")));
    assert(ncm_mpd_song_list_append_copy(&two_column_songs, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(test_set_library_song(
        &song, LIT_ARGS("recursive-two.flac"), LIT_ARGS("Album"),
        LIT_ARGS("2020"), NULL, 0, NULL, 0, NULL, 0, 100));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST,
                            LIT_ARGS("The Zebra")));
    assert(ncm_mpd_song_list_append_copy(&two_column_songs, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(test_set_library_song(
        &song, LIT_ARGS("recursive-empty.flac"), LIT_ARGS(""),
        LIT_ARGS("2020"), NULL, 0, NULL, 0, NULL, 0, 50));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST, LIT_ARGS("")));
    assert(ncm_mpd_song_list_append_copy(&two_column_songs, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(test_set_library_song(
        &song, LIT_ARGS("recursive-ignored.flac"),
        LIT_ARGS("Ignored"), LIT_ARGS("2020"),
        NULL, 0, NULL, 0, NULL, 0, 999));
    assert(ncm_mpd_song_list_append_copy(&two_column_songs, &song));

    Config.media_library_sort_by_mtime = false;
    assert(native_media_library_albums_from_songs(
        &albums, &two_column_songs,
        NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS, MPD_TAG_ARTIST,
        NULL, 0));
    assert(albums.len == 3);
    assert(albums.items[0].row.tag_len == 0);
    assert(ncm_string_equal(albums.items[1].row.tag,
                            albums.items[1].row.tag_len,
                            LIT_ARGS("Second")));
    assert(ncm_string_equal(albums.items[2].row.tag,
                            albums.items[2].row.tag_len,
                            LIT_ARGS("The Zebra")));
    assert(albums.items[2].row.mtime == 100);

    Config.media_library_sort_by_mtime = true;
    assert(native_media_library_albums_from_songs(
        &albums, &two_column_songs,
        NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS, MPD_TAG_ARTIST,
        NULL, 0));
    assert(albums.len == 3);
    assert(ncm_string_equal(albums.items[0].row.tag,
                            albums.items[0].row.tag_len,
                            LIT_ARGS("Second")));
    assert(albums.items[0].row.mtime == 300);
    assert(albums.items[1].row.mtime == 100);
    assert(albums.items[2].row.mtime == 50);

    Config.media_library_sort_by_mtime = false;
    assert(native_media_library_albums_from_songs(
        &albums, &two_column_songs,
        NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY, MPD_TAG_ARTIST,
        NULL, 0));
    assert(albums.len == 2);
    assert(albums.items[0].row.tag_len == 0);
    assert(albums.items[0].row.album_len == 0);
    assert(albums.items[1].row.tag_len == 0);
    assert(ncm_string_equal(albums.items[1].row.album,
                            albums.items[1].row.album_len,
                            LIT_ARGS("Album")));
    assert(albums.items[1].row.mtime == 100);

    ncm_song_destroy(&song);
    ncm_mpd_song_list_destroy(&two_column_songs);
    ncm_mpd_song_list_destroy(&three_column_songs);
    native_media_library_album_array_destroy(&albums);
    Config.ignore_leading_the = old_ignore_leading_the;
    Config.media_library_sort_by_mtime = old_sort_by_mtime;
    Config.media_library_albums_split_by_date = old_split_by_date;
    return;
}

static void
test_media_library_song_ordering(void) {
    NcmMpdSongList source;
    NcmSongArray songs;
    NcmSong song;
    NcmFormatAst old_song_format;
    enum NcmSongGetter title_getter;
    char *old_tags_separator;
    int32 old_tags_separator_len;
    int32 old_tags_separator_cap;
    bool old_ignore_leading_the;
    bool old_show_duplicate_tags;

    old_song_format = Config.song_library_format;
    old_tags_separator = Config.tags_separator;
    old_tags_separator_len = Config.tags_separator_len;
    old_tags_separator_cap = Config.tags_separator_cap;
    old_ignore_leading_the = Config.ignore_leading_the;
    old_show_duplicate_tags = Config.show_duplicate_tags;

    ncm_format_ast_init(&Config.song_library_format);
    title_getter = NCM_SONG_GETTER_TITLE;
    assert(ncm_format_ast_append_first_of_getters(
        &Config.song_library_format, &title_getter, 1));
    Config.tags_separator = (char *)" | ";
    Config.tags_separator_len = STRLIT_LEN(" | ");
    Config.tags_separator_cap = 0;
    Config.ignore_leading_the = false;
    Config.show_duplicate_tags = true;

    ncm_mpd_song_list_init(&source);
    ncm_song_array_init(&songs);
    ncm_song_init(&song);

    assert(test_set_library_song(
        &song, LIT_ARGS("six.flac"), LIT_ARGS("B"),
        LIT_ARGS("2020"), LIT_ARGS("1"), LIT_ARGS("1"),
        LIT_ARGS("Alpha"), 0));
    assert(ncm_mpd_song_list_append_copy(&source, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(test_set_library_song(
        &song, LIT_ARGS("five.flac"), LIT_ARGS("A"),
        LIT_ARGS("2020"), LIT_ARGS("2"), LIT_ARGS("1"),
        LIT_ARGS("Alpha"), 0));
    assert(ncm_mpd_song_list_append_copy(&source, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(test_set_library_song(
        &song, LIT_ARGS("four.flac"), LIT_ARGS("A"),
        LIT_ARGS("2020"), LIT_ARGS("1"), LIT_ARGS("2"),
        LIT_ARGS("Alpha"), 0));
    assert(ncm_mpd_song_list_append_copy(&source, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(test_set_library_song(
        &song, LIT_ARGS("three.flac"), LIT_ARGS("A"),
        LIT_ARGS("2020"), LIT_ARGS("1"), LIT_ARGS("1"),
        LIT_ARGS("Beta"), 0));
    assert(ncm_mpd_song_list_append_copy(&source, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(test_set_library_song(
        &song, LIT_ARGS("two.flac"), LIT_ARGS("A"),
        LIT_ARGS("2020"), LIT_ARGS("1"), LIT_ARGS("1"),
        LIT_ARGS("Alpha"), 0));
    assert(ncm_mpd_song_list_append_copy(&source, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(test_set_library_song(
        &song, LIT_ARGS("one.flac"), LIT_ARGS("Z"),
        LIT_ARGS("2019"), LIT_ARGS("1"), LIT_ARGS("1"),
        LIT_ARGS("Zulu"), 0));
    assert(ncm_mpd_song_list_append_copy(&source, &song));

    assert(native_media_library_songs_from_list(&songs, &source));
    assert(songs.len == 6);
    assert(ncm_string_equal(songs.items[0].uri,
                            songs.items[0].uri_len,
                            LIT_ARGS("one.flac")));
    assert(ncm_string_equal(songs.items[1].uri,
                            songs.items[1].uri_len,
                            LIT_ARGS("two.flac")));
    assert(ncm_string_equal(songs.items[2].uri,
                            songs.items[2].uri_len,
                            LIT_ARGS("three.flac")));
    assert(ncm_string_equal(songs.items[3].uri,
                            songs.items[3].uri_len,
                            LIT_ARGS("four.flac")));
    assert(ncm_string_equal(songs.items[4].uri,
                            songs.items[4].uri_len,
                            LIT_ARGS("five.flac")));
    assert(ncm_string_equal(songs.items[5].uri,
                            songs.items[5].uri_len,
                            LIT_ARGS("six.flac")));

    ncm_song_destroy(&song);
    ncm_song_array_destroy(&songs);
    ncm_mpd_song_list_destroy(&source);
    ncm_format_ast_destroy(&Config.song_library_format);
    Config.song_library_format = old_song_format;
    Config.tags_separator = old_tags_separator;
    Config.tags_separator_len = old_tags_separator_len;
    Config.tags_separator_cap = old_tags_separator_cap;
    Config.ignore_leading_the = old_ignore_leading_the;
    Config.show_duplicate_tags = old_show_duplicate_tags;
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
test_set_library_song(
    NcmSong *song, char *uri, int32 uri_len,
    char *album, int32 album_len, char *date, int32 date_len,
    char *disc, int32 disc_len, char *track, int32 track_len,
    char *title, int32 title_len, time_t mtime) {
    if (!ncm_song_set_uri(song, uri, uri_len)) {
        return false;
    }
    if ((album != NULL)
        && !ncm_song_add_tag(song, MPD_TAG_ALBUM, album, album_len)) {
        return false;
    }
    if ((date != NULL)
        && !ncm_song_add_tag(song, MPD_TAG_DATE, date, date_len)) {
        return false;
    }
    if ((disc != NULL)
        && !ncm_song_add_tag(song, MPD_TAG_DISC, disc, disc_len)) {
        return false;
    }
    if ((track != NULL)
        && !ncm_song_add_tag(song, MPD_TAG_TRACK, track, track_len)) {
        return false;
    }
    if ((title != NULL)
        && !ncm_song_add_tag(song, MPD_TAG_TITLE, title, title_len)) {
        return false;
    }
    ncm_song_set_mtime(song, mtime);
    return true;
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
