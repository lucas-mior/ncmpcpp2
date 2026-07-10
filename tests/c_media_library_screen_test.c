#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_string.h"
#include "global.h"
#include "screens/nc_media_library.h"
#include "settings.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

typedef struct MediaLibraryHookFixture {
    NcmMpdStringList *tag_results;
    NcmMpdSongList *all_song_results;
    NcmMpdSongList *search_results;

    char last_primary_value[128];
    char last_album[128];
    char last_date[128];
    char added_uri[128];
    int32 last_primary_value_len;
    int32 last_album_len;
    int32 last_date_len;
    int32 added_uri_len;
    enum mpd_tag_type last_primary_tag;

    int32 list_tags_calls;
    int32 list_all_songs_calls;
    int32 search_songs_calls;
    int32 add_songs_calls;
    int32 destroy_calls;
    int32 added_song_count;

    bool last_match_primary_tag;
    bool last_match_album;
    bool last_match_date;
    bool added_with_play;
    bool append_before_failure;
    bool fail_list_tags;
    bool fail_list_all_songs;
    bool fail_search_songs;
    bool fail;
} MediaLibraryHookFixture;

static void test_media_library_state_contract(void);
static void test_media_library_layout_and_rendering(void);
static void test_media_library_parity_surface(void);
static void test_media_library_external_interface(void);
static void test_media_library_tag_grouping(void);
static void test_media_library_tag_primitives(void);
static void test_media_library_album_primitives(void);
static void test_media_library_song_ordering(void);
static void test_media_library_query_pipeline(void);
static void test_media_library_query_failures(void);
static void test_media_library_delayed_update_state_machine(void);
static void test_media_library_native_list_change_callback(void);
static void test_media_library_identity_and_filter_replacement(void);
static void test_media_library_mode_transitions(void);
static void test_media_library_filter_search_actions(void);
static void test_media_library_selected_songs(void);
static void test_media_library_locate_song(void);
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
static bool test_append_pipeline_song(
    NcmMpdSongList *songs, char *uri, int32 uri_len,
    char *artist_one, int32 artist_one_len,
    char *artist_two, int32 artist_two_len,
    char *album, int32 album_len, char *date, int32 date_len,
    char *title, int32 title_len, time_t mtime);
static void test_record_query_value(char *dest, int32 *dest_len,
                                    char *source, int32 source_len);
static bool test_set_library_song(
    NcmSong *song, char *uri, int32 uri_len,
    char *album, int32 album_len, char *date, int32 date_len,
    char *disc, int32 disc_len, char *track, int32 track_len,
    char *title, int32 title_len, time_t mtime);

int
main(void) {
    test_media_library_state_contract();
    test_media_library_layout_and_rendering();
    test_media_library_parity_surface();
    test_media_library_external_interface();
    test_media_library_tag_grouping();
    test_media_library_tag_primitives();
    test_media_library_album_primitives();
    test_media_library_song_ordering();
    test_media_library_query_pipeline();
    test_media_library_query_failures();
    test_media_library_delayed_update_state_machine();
    test_media_library_native_list_change_callback();
    test_media_library_identity_and_filter_replacement();
    test_media_library_mode_transitions();
    test_media_library_filter_search_actions();
    test_media_library_selected_songs();
    test_media_library_locate_song();
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
    char *value;
    int32 value_len;
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
    assert(native_media_library_screen_current_primary_tag_value(
        &screen, &value, &value_len));
    assert(ncm_string_equal(value, value_len, LIT_ARGS("Artist A")));
    assert(native_media_library_screen_add_album(
        &screen, LIT_ARGS("Artist A"), LIT_ARGS("Album A"),
        LIT_ARGS("2026"), 0, false));
    assert(native_media_library_screen_current_album(&screen));
    assert(native_media_library_screen_current_album_value(
        &screen, &value, &value_len));
    assert(ncm_string_equal(value, value_len, LIT_ARGS("Album A")));
    assert(native_media_library_screen_current_album_date(
        &screen, &value, &value_len));
    assert(ncm_string_equal(value, value_len, LIT_ARGS("2026")));
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

    native_media_library_screen_clear_update_requests(&screen);
    assert(native_media_library_screen_set_primary_tag_type(
        &screen, MPD_TAG_GENRE));
    assert(Config.media_lib_primary_tag == MPD_TAG_GENRE);
    assert(nc_menu_all_item_count(
        nc_media_library_tag_menu_base(&screen.tags)) == 0);
    assert(nc_menu_all_item_count(
        nc_media_library_album_menu_base(&screen.albums)) == 0);
    assert(screen.tags_update_request);
    assert(!screen.albums_update_request);
    assert(!screen.songs_update_request);

    native_media_library_screen_clear_update_requests(&screen);
    native_media_library_screen_request_database_update(&screen);
    assert(screen.tags_update_request);
    assert(screen.albums_update_request);
    assert(screen.songs_update_request);

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
    enum NcmSongGetter title_getter;
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
    title_getter = NCM_SONG_GETTER_TITLE;
    assert(ncm_format_ast_append_first_of_getters(
        &Config.song_library_format, &title_getter, 1));

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
    assert(ncm_song_add_tag(&song, MPD_TAG_TITLE,
                            LIT_ARGS("library row")));
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
test_media_library_parity_surface(void) {
    NativeMediaLibraryScreen screen;
    MediaLibraryHookFixture fixture = {0};
    NcmError error;
    char *title;
    int32 title_len;
    bool old_titles_visibility;
    bool old_data_fetching_delay;
    bool old_sort_by_mtime;
    bool old_split_by_date;
    enum mpd_tag_type old_primary_tag;

    old_titles_visibility = Config.titles_visibility;
    old_data_fetching_delay = Config.data_fetching_delay;
    old_sort_by_mtime = Config.media_library_sort_by_mtime;
    old_split_by_date = Config.media_library_albums_split_by_date;
    old_primary_tag = Config.media_lib_primary_tag;

    Config.titles_visibility = true;
    Config.data_fetching_delay = false;
    Config.media_library_sort_by_mtime = false;
    Config.media_library_albums_split_by_date = true;
    Config.media_lib_primary_tag = MPD_TAG_ARTIST;

    native_media_library_screen_init(
        &screen, test_hooks(&fixture), 0, 90, 0, 24,
        nc_color_default(), nc_border_none());
    assert(ncm_string_equal(
        nc_screen_title(native_media_library_screen_base(&screen)),
        STRLIT_LEN("Media library"), LIT_ARGS("Media library")));
    title = nc_window_title(&screen.tags_window);
    title_len = nc_window_title_len(&screen.tags_window);
    assert(ncm_string_equal(title, title_len, LIT_ARGS("Artists")));
    title = nc_window_title(&screen.albums_window);
    title_len = nc_window_title_len(&screen.albums_window);
    assert(ncm_string_equal(title, title_len, LIT_ARGS("Albums")));
    title = nc_window_title(&screen.songs_window);
    title_len = nc_window_title_len(&screen.songs_window);
    assert(ncm_string_equal(title, title_len, LIT_ARGS("Songs")));

    assert(native_media_library_screen_set_primary_tag_type(
        &screen, MPD_TAG_GENRE));
    title = nc_window_title(&screen.tags_window);
    title_len = nc_window_title_len(&screen.tags_window);
    assert(ncm_string_equal(title, title_len, LIT_ARGS("Genres")));
    assert(native_media_library_screen_set_mode(
        &screen, NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS));
    title = nc_window_title(&screen.albums_window);
    title_len = nc_window_title_len(&screen.albums_window);
    assert(ncm_string_equal(
        title, title_len, LIT_ARGS("Albums (sorted by genre)")));
    assert(native_media_library_screen_toggle_sort_mode(&screen));
    title = nc_window_title(&screen.albums_window);
    title_len = nc_window_title_len(&screen.albums_window);
    assert(ncm_string_equal(
        title, title_len,
        LIT_ARGS("Albums (sorted by genre and mtime)")));
    assert(native_media_library_screen_set_mode(
        &screen, NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY));
    title = nc_window_title(&screen.albums_window);
    title_len = nc_window_title_len(&screen.albums_window);
    assert(ncm_string_equal(
        title, title_len, LIT_ARGS("Albums (sorted by mtime)")));

    native_media_library_screen_set_geometry(&screen, 5, 101, 2, 30);
    assert(nc_window_start_x(&screen.albums_window) == 5);
    assert(nc_window_start_y(&screen.albums_window) == 2);
    assert(nc_window_height(&screen.albums_window) == 30);
    assert(nc_window_start_y(&screen.songs_window) == 2);
    assert(nc_window_height(&screen.songs_window) == 30);
    native_media_library_screen_destroy(&screen);

    Config.media_lib_primary_tag = MPD_TAG_ARTIST;
    Config.media_library_sort_by_mtime = false;
    fixture = (MediaLibraryHookFixture){0};
    fixture.fail_list_all_songs = true;
    native_media_library_screen_init(
        &screen, test_hooks(&fixture), 0, 90, 0, 24,
        nc_color_default(), nc_border_none());
    assert(native_media_library_screen_set_mode(
        &screen, NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS));
    native_media_library_screen_request_albums_update(&screen);
    ncm_error_clear(&error);
    assert(!native_media_library_screen_update(&screen, &error));
    assert(error.code == EIO);
    assert(native_media_library_screen_mode(&screen)
           == NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY);
    assert(nc_screen_has_to_be_updated(
        native_media_library_screen_base(&screen)));
    ncm_error_clear(&error);
    assert(!native_media_library_screen_update(&screen, &error));
    assert(error.code == EIO);
    assert(native_media_library_screen_mode(&screen)
           == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS);
    native_media_library_screen_destroy(&screen);

    Config.titles_visibility = old_titles_visibility;
    Config.data_fetching_delay = old_data_fetching_delay;
    Config.media_library_sort_by_mtime = old_sort_by_mtime;
    Config.media_library_albums_split_by_date = old_split_by_date;
    Config.media_lib_primary_tag = old_primary_tag;
    return;
}

static void
test_media_library_external_interface(void) {
    NativeMediaLibraryScreen screen;
    MediaLibraryHookFixture fixture = {0};
    NcmSong song;
    NcmSongArray songs;
    NcMenu *tags_menu;
    NcMenu *songs_menu;
    NcmError error;
    char *value;
    int32 value_len;
    bool old_split_by_date;
    enum mpd_tag_type old_primary_tag;

    old_split_by_date = Config.media_library_albums_split_by_date;
    old_primary_tag = Config.media_lib_primary_tag;
    Config.media_library_albums_split_by_date = true;
    Config.media_lib_primary_tag = MPD_TAG_ARTIST;

    native_media_library_screen_init(
        &screen, test_hooks(&fixture), 0, 90, 0, 24,
        nc_color_default(), nc_border_none());
    ncm_song_init(&song);
    ncm_song_array_init(&songs);
    ncm_error_clear(&error);

    assert(native_media_library_screen_column_count(NULL) == 0);
    assert(native_media_library_screen_column_count(&screen) == 3);
    assert(!native_media_library_screen_item_available(&screen));

    assert(native_media_library_screen_add_tag(
        &screen, LIT_ARGS("Artist A"), 0));
    assert(native_media_library_screen_add_tag(
        &screen, LIT_ARGS("Artist B"), 0));
    assert(native_media_library_screen_item_available(&screen));
    tags_menu = nc_media_library_tag_menu_base(
        native_media_library_screen_tags(&screen));
    assert(nc_menu_set_position_selected(tags_menu, 1, true));
    assert(native_media_library_screen_current_tag_songs(
        &screen, &songs, &error));
    assert(songs.len == 1);
    assert(fixture.last_match_primary_tag);
    assert(!fixture.last_match_album);
    assert(!fixture.last_match_date);
    assert(ncm_string_equal(
        fixture.last_primary_value,
        fixture.last_primary_value_len,
        LIT_ARGS("Artist A")));
    ncm_song_array_clear(&songs);

    assert(native_media_library_screen_set_mode(
        &screen, NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS));
    assert(native_media_library_screen_column_count(&screen) == 2);
    assert(native_media_library_screen_active_column(&screen)
           == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS);
    assert(!native_media_library_screen_item_available(&screen));
    assert(native_media_library_screen_add_album(
        &screen, LIT_ARGS("Artist A"), NULL, 0, NULL, 0, 0, true));
    assert(native_media_library_screen_item_available(&screen));
    assert(native_media_library_screen_current_album_is_all_tracks(
        &screen));
    assert(!native_media_library_screen_current_album_value(
        &screen, &value, &value_len));
    assert(native_media_library_screen_current_album_songs(
        &screen, &songs, &error));
    assert(fixture.last_match_primary_tag);
    assert(!fixture.last_match_album);
    assert(!fixture.last_match_date);
    ncm_song_array_clear(&songs);

    assert(native_media_library_screen_set_mode(
        &screen, NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY));
    assert(native_media_library_screen_column_count(&screen) == 2);
    assert(!native_media_library_screen_current_album_is_all_tracks(
        &screen));
    ncm_error_clear(&error);
    assert(!native_media_library_screen_current_tag_songs(
        &screen, &songs, &error));
    assert(error.code == ENOENT);

    assert(native_media_library_screen_set_active_column(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_SONGS));
    assert(ncm_song_set_uri(&song, LIT_ARGS("first.flac")));
    assert(native_media_library_screen_add_song_copy(&screen, &song));
    assert(ncm_song_set_uri(&song, LIT_ARGS("second.flac")));
    assert(native_media_library_screen_add_song_copy(&screen, &song));
    assert(native_media_library_screen_item_available(&screen));
    songs_menu = nc_media_library_song_menu_base(
        native_media_library_screen_songs(&screen));
    assert(nc_menu_set_position_selected(songs_menu, 0, true));
    ncm_error_clear(&error);
    assert(native_media_library_screen_copy_visible_songs(
        &screen, &songs, &error));
    assert(songs.len == 2);
    assert(ncm_string_equal(songs.items[0].uri,
                            songs.items[0].uri_len,
                            LIT_ARGS("first.flac")));
    assert(ncm_string_equal(songs.items[1].uri,
                            songs.items[1].uri_len,
                            LIT_ARGS("second.flac")));

    ncm_error_clear(&error);
    assert(!native_media_library_screen_copy_visible_songs(
        &screen, NULL, &error));
    assert(error.code == EINVAL);

    ncm_song_array_destroy(&songs);
    ncm_song_destroy(&song);
    native_media_library_screen_destroy(&screen);
    Config.media_library_albums_split_by_date = old_split_by_date;
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
test_media_library_query_pipeline(void) {
    NcmTimePoint old_global_timer;
    enum mpd_tag_type old_primary_tag;
    bool old_data_fetching_delay;
    bool old_sort_by_mtime;
    bool old_split_by_date;

    old_global_timer = global_timer;
    old_primary_tag = Config.media_lib_primary_tag;
    old_data_fetching_delay = Config.data_fetching_delay;
    old_sort_by_mtime = Config.media_library_sort_by_mtime;
    old_split_by_date = Config.media_library_albums_split_by_date;

    global_timer.ns = 0;
    Config.media_lib_primary_tag = MPD_TAG_ARTIST;
    Config.data_fetching_delay = true;
    Config.media_library_albums_split_by_date = true;

    {
        NativeMediaLibraryScreen screen;
        MediaLibraryHookFixture fixture = {0};
        NcmMpdStringList tags;
        NcmError error;
        NcMediaLibraryTagRow *first;
        NcMediaLibraryTagRow *second;

        Config.media_library_sort_by_mtime = false;
        ncm_mpd_string_list_init(&tags);
        assert(ncm_mpd_string_list_append(
            &tags, LIT_ARGS("Artist B")));
        assert(ncm_mpd_string_list_append(
            &tags, LIT_ARGS("Artist A")));
        fixture.tag_results = &tags;

        native_media_library_screen_init(
            &screen, test_hooks(&fixture), 0, 90, 0, 24,
            nc_color_default(), nc_border_none());
        ncm_error_clear(&error);
        assert(native_media_library_screen_update(&screen, &error));
        assert(fixture.list_tags_calls == 1);
        assert(fixture.list_all_songs_calls == 0);
        assert(fixture.search_songs_calls == 0);
        assert(nc_menu_all_item_count(
                   nc_media_library_tag_menu_base(&screen.tags))
               == 2);
        first = nc_media_library_tag_menu_item_at(
            &screen.tags, NC_MENU_ITEMS_ALL, 0);
        second = nc_media_library_tag_menu_item_at(
            &screen.tags, NC_MENU_ITEMS_ALL, 1);
        assert(first);
        assert(second);
        assert(ncm_string_equal(first->tag, first->tag_len,
                                LIT_ARGS("Artist A")));
        assert(ncm_string_equal(second->tag, second->tag_len,
                                LIT_ARGS("Artist B")));

        native_media_library_screen_destroy(&screen);
        ncm_mpd_string_list_destroy(&tags);
    }

    {
        NativeMediaLibraryScreen screen;
        MediaLibraryHookFixture fixture = {0};
        NcmMpdSongList songs;
        NcmError error;

        Config.media_library_sort_by_mtime = true;
        ncm_mpd_song_list_init(&songs);
        assert(test_append_pipeline_song(
            &songs, LIT_ARGS("one.flac"), LIT_ARGS("Artist A"),
            LIT_ARGS("Alias A"), LIT_ARGS("Album A"),
            LIT_ARGS("2020"), LIT_ARGS("One"), 10));
        assert(test_append_pipeline_song(
            &songs, LIT_ARGS("two.flac"), LIT_ARGS("Artist A"),
            LIT_ARGS("Artist B"), LIT_ARGS("Album B"),
            LIT_ARGS("2021"), LIT_ARGS("Two"), 20));
        fixture.all_song_results = &songs;

        native_media_library_screen_init(
            &screen, test_hooks(&fixture), 0, 90, 0, 24,
            nc_color_default(), nc_border_none());
        ncm_error_clear(&error);
        assert(native_media_library_screen_update(&screen, &error));
        assert(fixture.list_tags_calls == 0);
        assert(fixture.list_all_songs_calls == 1);
        assert(fixture.search_songs_calls == 0);
        assert(nc_menu_all_item_count(
                   nc_media_library_tag_menu_base(&screen.tags))
               == 3);
        assert(native_media_library_screen_current_tag(&screen)->mtime
               == 20);

        native_media_library_screen_destroy(&screen);
        ncm_mpd_song_list_destroy(&songs);
    }

    {
        NativeMediaLibraryScreen screen;
        MediaLibraryHookFixture fixture = {0};
        NcmMpdSongList songs;
        NcmError error;
        NcMenu *albums;

        Config.media_library_sort_by_mtime = false;
        ncm_mpd_song_list_init(&songs);
        assert(test_append_pipeline_song(
            &songs, LIT_ARGS("album-a.flac"), NULL, 0, NULL, 0,
            LIT_ARGS("Album A"), LIT_ARGS("2020"),
            LIT_ARGS("A"), 10));
        assert(test_append_pipeline_song(
            &songs, LIT_ARGS("album-b.flac"), NULL, 0, NULL, 0,
            LIT_ARGS("Album B"), LIT_ARGS("2021"),
            LIT_ARGS("B"), 20));
        fixture.search_results = &songs;

        native_media_library_screen_init(
            &screen, test_hooks(&fixture), 0, 90, 0, 24,
            nc_color_default(), nc_border_none());
        assert(native_media_library_screen_add_tag(
            &screen, LIT_ARGS("Artist A"), 0));
        native_media_library_screen_request_albums_update(&screen);
        ncm_error_clear(&error);
        assert(native_media_library_screen_update(&screen, &error));
        assert(fixture.search_songs_calls == 1);
        assert(fixture.last_match_primary_tag);
        assert(!fixture.last_match_album);
        assert(!fixture.last_match_date);
        assert(fixture.last_primary_tag == MPD_TAG_ARTIST);
        assert(ncm_string_equal(
            fixture.last_primary_value,
            fixture.last_primary_value_len,
            LIT_ARGS("Artist A")));
        albums = nc_media_library_album_menu_base(&screen.albums);
        assert(nc_menu_all_item_count(albums) == 4);
        assert(nc_menu_position_is_separator(albums, 2));
        assert(nc_media_library_album_menu_item_at(
                   &screen.albums, NC_MENU_ITEMS_ALL, 3)
                   ->all_tracks_entry);

        native_media_library_screen_destroy(&screen);
        ncm_mpd_song_list_destroy(&songs);
    }

    {
        NativeMediaLibraryScreen screen;
        MediaLibraryHookFixture fixture = {0};
        NcmMpdSongList songs;
        NcmError error;

        Config.media_library_sort_by_mtime = false;
        ncm_mpd_song_list_init(&songs);
        assert(test_append_pipeline_song(
            &songs, LIT_ARGS("shared.flac"), LIT_ARGS("Artist A"),
            LIT_ARGS("Artist B"), LIT_ARGS("Shared"),
            LIT_ARGS("2020"), LIT_ARGS("Shared"), 10));
        assert(test_append_pipeline_song(
            &songs, LIT_ARGS("other.flac"), LIT_ARGS("Artist A"),
            NULL, 0, LIT_ARGS("Other"), LIT_ARGS("2021"),
            LIT_ARGS("Other"), 20));
        assert(test_append_pipeline_song(
            &songs, LIT_ARGS("hidden.flac"), NULL, 0, NULL, 0,
            LIT_ARGS("Hidden"), LIT_ARGS("2022"),
            LIT_ARGS("Hidden"), 30));
        fixture.all_song_results = &songs;

        native_media_library_screen_init(
            &screen, test_hooks(&fixture), 0, 90, 0, 24,
            nc_color_default(), nc_border_none());
        assert(native_media_library_screen_set_mode(
            &screen, NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS));
        ncm_error_clear(&error);
        assert(native_media_library_screen_update(&screen, &error));
        assert(fixture.list_all_songs_calls == 1);
        assert(fixture.search_songs_calls == 0);
        assert(nc_menu_all_item_count(
                   nc_media_library_album_menu_base(&screen.albums))
               == 3);
        native_media_library_screen_destroy(&screen);

        fixture = (MediaLibraryHookFixture){0};
        fixture.all_song_results = &songs;
        native_media_library_screen_init(
            &screen, test_hooks(&fixture), 0, 90, 0, 24,
            nc_color_default(), nc_border_none());
        assert(native_media_library_screen_set_mode(
            &screen, NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY));
        ncm_error_clear(&error);
        assert(native_media_library_screen_update(&screen, &error));
        assert(fixture.list_all_songs_calls == 1);
        assert(nc_menu_all_item_count(
                   nc_media_library_album_menu_base(&screen.albums))
               == 2);
        native_media_library_screen_destroy(&screen);
        ncm_mpd_song_list_destroy(&songs);
    }

    {
        NativeMediaLibraryScreen screen;
        MediaLibraryHookFixture fixture = {0};
        NcmMpdSongList songs;
        NcmError error;

        Config.media_library_sort_by_mtime = false;
        ncm_mpd_song_list_init(&songs);
        assert(test_append_pipeline_song(
            &songs, LIT_ARGS("two.flac"), NULL, 0, NULL, 0,
            LIT_ARGS("Album A"), LIT_ARGS("2020"),
            LIT_ARGS("Two"), 0));
        assert(test_append_pipeline_song(
            &songs, LIT_ARGS("one.flac"), NULL, 0, NULL, 0,
            LIT_ARGS("Album A"), LIT_ARGS("2020"),
            LIT_ARGS("One"), 0));
        fixture.search_results = &songs;

        native_media_library_screen_init(
            &screen, test_hooks(&fixture), 0, 90, 0, 24,
            nc_color_default(), nc_border_none());
        assert(native_media_library_screen_add_tag(
            &screen, LIT_ARGS("Artist A"), 0));
        assert(native_media_library_screen_add_album(
            &screen, LIT_ARGS("Artist A"), LIT_ARGS("Album A"),
            LIT_ARGS("2020"), 0, false));
        native_media_library_screen_request_songs_update(&screen);
        ncm_error_clear(&error);
        assert(native_media_library_screen_update(&screen, &error));
        assert(fixture.last_match_primary_tag);
        assert(fixture.last_match_album);
        assert(fixture.last_match_date);
        assert(ncm_string_equal(fixture.last_album,
                                fixture.last_album_len,
                                LIT_ARGS("Album A")));
        assert(ncm_string_equal(fixture.last_date,
                                fixture.last_date_len,
                                LIT_ARGS("2020")));
        assert(native_media_library_screen_visible_song_count(&screen)
               == 2);
        native_media_library_screen_destroy(&screen);

        fixture = (MediaLibraryHookFixture){0};
        fixture.search_results = &songs;
        native_media_library_screen_init(
            &screen, test_hooks(&fixture), 0, 90, 0, 24,
            nc_color_default(), nc_border_none());
        assert(native_media_library_screen_add_tag(
            &screen, LIT_ARGS("Artist A"), 0));
        assert(native_media_library_screen_add_album(
            &screen, LIT_ARGS("Artist A"), LIT_ARGS(""),
            LIT_ARGS(""), 0, true));
        native_media_library_screen_request_songs_update(&screen);
        ncm_error_clear(&error);
        assert(native_media_library_screen_update(&screen, &error));
        assert(fixture.last_match_primary_tag);
        assert(!fixture.last_match_album);
        assert(!fixture.last_match_date);
        assert(ncm_string_equal(
            fixture.last_primary_value,
            fixture.last_primary_value_len,
            LIT_ARGS("Artist A")));
        native_media_library_screen_destroy(&screen);

        Config.media_library_albums_split_by_date = false;
        fixture = (MediaLibraryHookFixture){0};
        fixture.search_results = &songs;
        native_media_library_screen_init(
            &screen, test_hooks(&fixture), 0, 90, 0, 24,
            nc_color_default(), nc_border_none());
        assert(native_media_library_screen_set_mode(
            &screen, NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS));
        assert(native_media_library_screen_add_album(
            &screen, LIT_ARGS("Artist B"), LIT_ARGS("Album B"),
            LIT_ARGS("2022"), 0, false));
        native_media_library_screen_request_songs_update(&screen);
        ncm_error_clear(&error);
        assert(native_media_library_screen_update(&screen, &error));
        assert(fixture.last_match_primary_tag);
        assert(fixture.last_match_album);
        assert(!fixture.last_match_date);
        assert(ncm_string_equal(
            fixture.last_primary_value,
            fixture.last_primary_value_len,
            LIT_ARGS("Artist B")));
        assert(ncm_string_equal(fixture.last_album,
                                fixture.last_album_len,
                                LIT_ARGS("Album B")));
        native_media_library_screen_destroy(&screen);

        Config.media_library_albums_split_by_date = true;
        fixture = (MediaLibraryHookFixture){0};
        fixture.search_results = &songs;
        native_media_library_screen_init(
            &screen, test_hooks(&fixture), 0, 90, 0, 24,
            nc_color_default(), nc_border_none());
        assert(native_media_library_screen_set_mode(
            &screen, NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY));
        assert(native_media_library_screen_add_album(
            &screen, LIT_ARGS(""), LIT_ARGS("Album A"),
            LIT_ARGS("2020"), 0, false));
        native_media_library_screen_request_songs_update(&screen);
        ncm_error_clear(&error);
        assert(native_media_library_screen_update(&screen, &error));
        assert(!fixture.last_match_primary_tag);
        assert(fixture.last_match_album);
        assert(fixture.last_match_date);
        native_media_library_screen_destroy(&screen);
        ncm_mpd_song_list_destroy(&songs);
    }

    global_timer = old_global_timer;
    Config.media_lib_primary_tag = old_primary_tag;
    Config.data_fetching_delay = old_data_fetching_delay;
    Config.media_library_sort_by_mtime = old_sort_by_mtime;
    Config.media_library_albums_split_by_date = old_split_by_date;
    return;
}

static void
test_media_library_query_failures(void) {
    NcmTimePoint old_global_timer;
    enum mpd_tag_type old_primary_tag;
    bool old_data_fetching_delay;
    bool old_sort_by_mtime;

    old_global_timer = global_timer;
    old_primary_tag = Config.media_lib_primary_tag;
    old_data_fetching_delay = Config.data_fetching_delay;
    old_sort_by_mtime = Config.media_library_sort_by_mtime;

    global_timer.ns = 0;
    Config.media_lib_primary_tag = MPD_TAG_ARTIST;
    Config.data_fetching_delay = true;

    {
        NativeMediaLibraryScreen screen;
        MediaLibraryHookFixture fixture = {0};
        NcmError error;
        NcMediaLibraryTagRow *tag;

        Config.media_library_sort_by_mtime = false;
        fixture.append_before_failure = true;
        fixture.fail_list_tags = true;
        native_media_library_screen_init(
            &screen, test_hooks(&fixture), 0, 90, 0, 24,
            nc_color_default(), nc_border_none());
        assert(native_media_library_screen_add_tag(
            &screen, LIT_ARGS("Existing"), 1));
        native_media_library_screen_request_tags_update(&screen);
        ncm_error_clear(&error);
        assert(!native_media_library_screen_update(&screen, &error));
        assert(error.code == EIO);
        assert(ncm_string_equal(error.message,
                                STRLIT_LEN("test tag error"),
                                LIT_ARGS("test tag error")));
        assert(!native_media_library_screen_sort_by_mtime(&screen));
        assert(nc_menu_all_item_count(
                   nc_media_library_tag_menu_base(&screen.tags))
               == 1);
        tag = native_media_library_screen_current_tag(&screen);
        assert(tag);
        assert(ncm_string_equal(tag->tag, tag->tag_len,
                                LIT_ARGS("Existing")));
        assert(screen.tags_update_request);
        assert(!screen.albums_update_request);
        assert(!screen.songs_update_request);
        native_media_library_screen_destroy(&screen);
    }

    {
        NativeMediaLibraryScreen screen;
        MediaLibraryHookFixture fixture = {0};
        NcmError error;

        Config.media_library_sort_by_mtime = true;
        fixture.append_before_failure = true;
        fixture.fail_list_all_songs = true;
        native_media_library_screen_init(
            &screen, test_hooks(&fixture), 0, 90, 0, 24,
            nc_color_default(), nc_border_none());
        assert(native_media_library_screen_add_tag(
            &screen, LIT_ARGS("Existing"), 1));
        native_media_library_screen_request_tags_update(&screen);
        ncm_error_clear(&error);
        assert(!native_media_library_screen_update(&screen, &error));
        assert(error.code == EIO);
        assert(ncm_string_equal(error.message,
                                STRLIT_LEN("test song-list error"),
                                LIT_ARGS("test song-list error")));
        assert(!native_media_library_screen_sort_by_mtime(&screen));
        assert(!Config.media_library_sort_by_mtime);
        assert(nc_menu_all_item_count(
                   nc_media_library_tag_menu_base(&screen.tags))
               == 1);
        assert(screen.tags_update_request);
        assert(screen.albums_update_request);
        assert(screen.songs_update_request);
        native_media_library_screen_destroy(&screen);
    }

    {
        NativeMediaLibraryScreen screen;
        MediaLibraryHookFixture fixture = {0};
        NcmError error;

        Config.media_library_sort_by_mtime = false;
        fixture.append_before_failure = true;
        fixture.fail_list_all_songs = true;
        native_media_library_screen_init(
            &screen, test_hooks(&fixture), 0, 90, 0, 24,
            nc_color_default(), nc_border_none());
        assert(native_media_library_screen_set_mode(
            &screen, NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS));
        assert(native_media_library_screen_add_album(
            &screen, LIT_ARGS("Artist A"), LIT_ARGS("Existing"),
            LIT_ARGS("2020"), 1, false));
        native_media_library_screen_request_albums_update(&screen);
        ncm_error_clear(&error);
        assert(!native_media_library_screen_update(&screen, &error));
        assert(error.code == EIO);
        assert(native_media_library_screen_mode(&screen)
               == NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY);
        assert(nc_menu_all_item_count(
                   nc_media_library_album_menu_base(&screen.albums))
               == 0);
        assert(screen.tags_update_request);
        assert(screen.albums_update_request);
        assert(screen.songs_update_request);
        native_media_library_screen_destroy(&screen);

        fixture = (MediaLibraryHookFixture){0};
        fixture.append_before_failure = true;
        fixture.fail_list_all_songs = true;
        native_media_library_screen_init(
            &screen, test_hooks(&fixture), 0, 90, 0, 24,
            nc_color_default(), nc_border_none());
        assert(native_media_library_screen_set_mode(
            &screen, NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY));
        native_media_library_screen_request_albums_update(&screen);
        ncm_error_clear(&error);
        assert(!native_media_library_screen_update(&screen, &error));
        assert(native_media_library_screen_mode(&screen)
               == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS);
        assert(error.code == EIO);
        native_media_library_screen_destroy(&screen);
    }

    {
        NativeMediaLibraryScreen screen;
        MediaLibraryHookFixture fixture = {0};
        NcmError error;
        NcMediaLibraryAlbumRow *album;

        Config.media_library_sort_by_mtime = false;
        fixture.append_before_failure = true;
        fixture.fail_search_songs = true;
        native_media_library_screen_init(
            &screen, test_hooks(&fixture), 0, 90, 0, 24,
            nc_color_default(), nc_border_none());
        assert(native_media_library_screen_add_tag(
            &screen, LIT_ARGS("Artist A"), 0));
        assert(native_media_library_screen_add_album(
            &screen, LIT_ARGS("Artist A"), LIT_ARGS("Existing"),
            LIT_ARGS("2020"), 1, false));
        native_media_library_screen_request_albums_update(&screen);
        ncm_error_clear(&error);
        assert(!native_media_library_screen_update(&screen, &error));
        assert(error.code == EIO);
        assert(screen.albums_update_request);
        assert(nc_menu_all_item_count(
                   nc_media_library_album_menu_base(&screen.albums))
               == 1);
        album = native_media_library_screen_current_album(&screen);
        assert(album);
        assert(ncm_string_equal(album->album, album->album_len,
                                LIT_ARGS("Existing")));
        native_media_library_screen_destroy(&screen);
    }

    {
        NativeMediaLibraryScreen screen;
        MediaLibraryHookFixture fixture = {0};
        NcmError error;
        NcmSong song;
        NcmSong *current;

        Config.media_library_sort_by_mtime = false;
        fixture.append_before_failure = true;
        fixture.fail_search_songs = true;
        native_media_library_screen_init(
            &screen, test_hooks(&fixture), 0, 90, 0, 24,
            nc_color_default(), nc_border_none());
        assert(native_media_library_screen_add_tag(
            &screen, LIT_ARGS("Artist A"), 0));
        assert(native_media_library_screen_add_album(
            &screen, LIT_ARGS("Artist A"), LIT_ARGS("Album A"),
            LIT_ARGS("2020"), 1, false));
        ncm_song_init(&song);
        assert(ncm_song_set_uri(&song, LIT_ARGS("existing.flac")));
        assert(native_media_library_screen_add_song_copy(&screen, &song));
        native_media_library_screen_request_songs_update(&screen);
        ncm_error_clear(&error);
        assert(!native_media_library_screen_update(&screen, &error));
        assert(error.code == EIO);
        assert(screen.songs_update_request);
        assert(native_media_library_screen_visible_song_count(&screen)
               == 1);
        current = native_media_library_screen_visible_song_at(&screen, 0);
        assert(current);
        assert(ncm_string_equal(current->uri, current->uri_len,
                                LIT_ARGS("existing.flac")));
        ncm_song_destroy(&song);
        native_media_library_screen_destroy(&screen);
    }

    global_timer = old_global_timer;
    Config.media_lib_primary_tag = old_primary_tag;
    Config.data_fetching_delay = old_data_fetching_delay;
    Config.media_library_sort_by_mtime = old_sort_by_mtime;
    return;
}

static void
test_media_library_delayed_update_state_machine(void) {
    NativeMediaLibraryScreen screen;
    MediaLibraryHookFixture fixture = {0};
    NcmMpdStringList tags;
    NcmMpdSongList songs;
    NcmTimePoint old_global_timer;
    NcmError error;
    enum mpd_tag_type old_primary_tag;
    bool old_data_fetching_delay;
    bool old_sort_by_mtime;
    bool old_split_by_date;

    old_global_timer = global_timer;
    old_primary_tag = Config.media_lib_primary_tag;
    old_data_fetching_delay = Config.data_fetching_delay;
    old_sort_by_mtime = Config.media_library_sort_by_mtime;
    old_split_by_date = Config.media_library_albums_split_by_date;

    Config.media_lib_primary_tag = MPD_TAG_ARTIST;
    Config.data_fetching_delay = true;
    Config.media_library_sort_by_mtime = false;
    Config.media_library_albums_split_by_date = true;
    global_timer.ns = 1000000000ll;

    ncm_mpd_string_list_init(&tags);
    ncm_mpd_song_list_init(&songs);
    assert(ncm_mpd_string_list_append(&tags, LIT_ARGS("Artist A")));
    assert(test_append_pipeline_song(
        &songs, LIT_ARGS("song-a.flac"), LIT_ARGS("Artist A"),
        NULL, 0, LIT_ARGS("Album A"), LIT_ARGS("2020"),
        LIT_ARGS("Song A"), 10));
    fixture.tag_results = &tags;
    fixture.search_results = &songs;

    native_media_library_screen_init(
        &screen, test_hooks(&fixture), 0, 90, 0, 24,
        nc_color_default(), nc_border_none());
    ncm_error_clear(&error);

    assert(native_media_library_screen_update(&screen, &error));
    assert(fixture.list_tags_calls == 1);
    assert(fixture.search_songs_calls == 0);
    assert(screen.update_timer.ns == global_timer.ns);
    assert(nc_screen_window_timeout(
               native_media_library_screen_base(&screen))
           == NATIVE_MEDIA_LIBRARY_FETCH_DELAY_MS);

    assert(native_media_library_screen_update(&screen, &error));
    assert(fixture.search_songs_calls == 0);
    global_timer.ns += 249000000ll;
    assert(native_media_library_screen_update(&screen, &error));
    assert(fixture.search_songs_calls == 0);

    global_timer.ns += 2000000ll;
    assert(native_media_library_screen_update(&screen, &error));
    assert(fixture.search_songs_calls == 1);
    assert(nc_menu_all_item_count(
               nc_media_library_album_menu_base(&screen.albums))
           == 1);
    assert(screen.update_timer.ns == global_timer.ns);
    assert(nc_screen_window_timeout(
               native_media_library_screen_base(&screen))
           == NATIVE_MEDIA_LIBRARY_FETCH_DELAY_MS);

    assert(native_media_library_screen_update(&screen, &error));
    assert(fixture.search_songs_calls == 1);
    global_timer.ns += 251000000ll;
    assert(native_media_library_screen_update(&screen, &error));
    assert(fixture.search_songs_calls == 2);
    assert(native_media_library_screen_visible_song_count(&screen) == 1);
    assert(native_media_library_screen_next_column_available(&screen));
    native_media_library_screen_next_column(&screen);
    assert(native_media_library_screen_active_column(&screen)
           == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS);
    assert(native_media_library_screen_next_column_available(&screen));
    native_media_library_screen_next_column(&screen);
    assert(native_media_library_screen_active_column(&screen)
           == NATIVE_MEDIA_LIBRARY_COLUMN_SONGS);
    assert(native_media_library_screen_previous_column_available(&screen));
    native_media_library_screen_previous_column(&screen);
    native_media_library_screen_previous_column(&screen);
    assert(native_media_library_screen_active_column(&screen)
           == NATIVE_MEDIA_LIBRARY_COLUMN_TAGS);
    assert(nc_screen_window_timeout(
               native_media_library_screen_base(&screen))
           == NC_SCREEN_DEFAULT_WINDOW_TIMEOUT);

    native_media_library_screen_request_tags_update(&screen);
    native_media_library_screen_request_albums_update(&screen);
    native_media_library_screen_request_songs_update(&screen);
    assert(native_media_library_screen_update(&screen, &error));
    assert(fixture.list_tags_calls == 2);
    assert(fixture.search_songs_calls == 2);
    assert(!screen.tags_update_request);
    assert(screen.albums_update_request);
    assert(screen.songs_update_request);

    assert(native_media_library_screen_update(&screen, &error));
    assert(fixture.search_songs_calls == 3);
    assert(!screen.albums_update_request);
    assert(screen.songs_update_request);

    assert(native_media_library_screen_update(&screen, &error));
    assert(fixture.search_songs_calls == 4);
    assert(!screen.songs_update_request);

    native_media_library_screen_destroy(&screen);
    ncm_mpd_song_list_destroy(&songs);
    ncm_mpd_string_list_destroy(&tags);

    {
        NativeMediaLibraryScreen immediate_screen;
        MediaLibraryHookFixture immediate_fixture = {0};
        NcmMpdSongList immediate_songs;

        Config.data_fetching_delay = false;
        global_timer.ns = 2000000000ll;
        ncm_mpd_song_list_init(&immediate_songs);
        assert(test_append_pipeline_song(
            &immediate_songs, LIT_ARGS("immediate.flac"),
            LIT_ARGS("Artist A"), NULL, 0, LIT_ARGS("Album A"),
            LIT_ARGS("2020"), LIT_ARGS("Immediate"), 10));
        immediate_fixture.all_song_results = &immediate_songs;
        immediate_fixture.search_results = &immediate_songs;

        native_media_library_screen_init(
            &immediate_screen, test_hooks(&immediate_fixture),
            0, 90, 0, 24, nc_color_default(), nc_border_none());
        assert(native_media_library_screen_set_mode(
            &immediate_screen, NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS));
        assert(native_media_library_screen_update(
            &immediate_screen, &error));
        assert(immediate_fixture.list_all_songs_calls == 1);
        assert(immediate_fixture.search_songs_calls == 0);
        assert(native_media_library_screen_update(
            &immediate_screen, &error));
        assert(immediate_fixture.search_songs_calls == 1);

        native_media_library_screen_destroy(&immediate_screen);
        ncm_mpd_song_list_destroy(&immediate_songs);
    }

    global_timer = old_global_timer;
    Config.media_lib_primary_tag = old_primary_tag;
    Config.data_fetching_delay = old_data_fetching_delay;
    Config.media_library_sort_by_mtime = old_sort_by_mtime;
    Config.media_library_albums_split_by_date = old_split_by_date;
    return;
}

static void
test_media_library_native_list_change_callback(void) {
    NativeMediaLibraryScreen screen;
    NativeMediaLibraryHooks hooks = {0};
    NcmSong song;
    NcMediaLibraryTagRow *tag;
    NcMediaLibraryAlbumRow *album;
    NcMenu *albums;
    NcMenu *songs;
    NcmTimePoint old_global_timer;
    bool old_data_fetching_delay;

    old_global_timer = global_timer;
    old_data_fetching_delay = Config.data_fetching_delay;
    Config.data_fetching_delay = true;
    global_timer.ns = 10000000000ll;

    native_media_library_screen_init(
        &screen, hooks, 0, 90, 0, 24, nc_color_default(),
        nc_border_none());
    albums = nc_media_library_album_menu_base(&screen.albums);
    songs = nc_media_library_song_menu_base(&screen.songs);

    assert(native_media_library_screen_add_tag(
        &screen, LIT_ARGS("Artist A"), 0));
    assert(native_media_library_screen_add_tag(
        &screen, LIT_ARGS("Artist B"), 0));
    assert(native_media_library_screen_add_album(
        &screen, LIT_ARGS("Artist A"), LIT_ARGS("Album A"),
        LIT_ARGS("2020"), 0, false));
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("song-a.flac")));
    assert(native_media_library_screen_add_song_copy(&screen, &song));

    nc_screen_scroll(native_media_library_screen_base(&screen),
                     NC_SCROLL_DOWN);
    tag = native_media_library_screen_current_tag(&screen);
    assert(tag);
    assert(ncm_string_equal(tag->tag, tag->tag_len,
                            LIT_ARGS("Artist B")));
    assert(nc_menu_all_item_count(albums) == 0);
    assert(nc_menu_all_item_count(songs) == 0);
    assert(screen.update_timer.ns == global_timer.ns);
    assert(screen.screen.has_to_be_updated);

    assert(native_media_library_screen_add_album(
        &screen, LIT_ARGS("Artist B"), LIT_ARGS("Album B"),
        LIT_ARGS("2020"), 0, false));
    assert(native_media_library_screen_add_album(
        &screen, LIT_ARGS("Artist B"), LIT_ARGS("Album C"),
        LIT_ARGS("2021"), 0, false));
    assert(native_media_library_screen_add_song_copy(&screen, &song));
    assert(native_media_library_screen_set_active_column(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS));
    global_timer.ns = 11000000000ll;

    nc_screen_scroll(native_media_library_screen_base(&screen),
                     NC_SCROLL_DOWN);
    album = native_media_library_screen_current_album(&screen);
    assert(album);
    assert(ncm_string_equal(album->album, album->album_len,
                            LIT_ARGS("Album C")));
    assert(nc_menu_all_item_count(songs) == 0);
    assert(screen.update_timer.ns == global_timer.ns);

    ncm_song_destroy(&song);
    native_media_library_screen_destroy(&screen);
    global_timer = old_global_timer;
    Config.data_fetching_delay = old_data_fetching_delay;
    return;
}

static void
test_media_library_identity_and_filter_replacement(void) {
    NativeMediaLibraryScreen screen;
    MediaLibraryHookFixture fixture = {0};
    NcmMpdStringList replacement_tags;
    NcmMpdSongList replacement_albums;
    NcmMpdSongList replacement_songs;
    NcmSong song;
    NcmSong *current_song;
    NcMediaLibraryTagRow *current_tag;
    NcMediaLibraryAlbumRow *current_album;
    NcMenu *tags_menu;
    NcMenu *albums_menu;
    NcMenu *songs_menu;
    NcmTimePoint old_global_timer;
    NcmError error;
    NcmFormatAst old_song_format;
    enum NcmSongGetter title_getter;
    enum mpd_tag_type old_primary_tag;
    uint32 old_regex_type;
    bool old_data_fetching_delay;
    bool old_sort_by_mtime;
    bool old_split_by_date;

    old_global_timer = global_timer;
    old_song_format = Config.song_library_format;
    old_primary_tag = Config.media_lib_primary_tag;
    old_regex_type = Config.regex_type;
    old_data_fetching_delay = Config.data_fetching_delay;
    old_sort_by_mtime = Config.media_library_sort_by_mtime;
    old_split_by_date = Config.media_library_albums_split_by_date;

    global_timer.ns = 5000000000ll;
    Config.media_lib_primary_tag = MPD_TAG_ARTIST;
    Config.regex_type = NCM_REGEX_LITERAL_CASE_INSENSITIVE;
    Config.data_fetching_delay = false;
    Config.media_library_sort_by_mtime = false;
    Config.media_library_albums_split_by_date = true;
    ncm_format_ast_init(&Config.song_library_format);
    title_getter = NCM_SONG_GETTER_TITLE;
    assert(ncm_format_ast_append_first_of_getters(
        &Config.song_library_format, &title_getter, 1));

    ncm_mpd_string_list_init(&replacement_tags);
    ncm_mpd_song_list_init(&replacement_albums);
    ncm_mpd_song_list_init(&replacement_songs);
    assert(ncm_mpd_string_list_append(
        &replacement_tags, LIT_ARGS("Artist A")));
    assert(ncm_mpd_string_list_append(
        &replacement_tags, LIT_ARGS("Artist C")));
    assert(ncm_mpd_string_list_append(
        &replacement_tags, LIT_ARGS("Artist D")));
    assert(test_append_pipeline_song(
        &replacement_albums, LIT_ARGS("album-a.flac"),
        LIT_ARGS("Artist A"), NULL, 0, LIT_ARGS("Album A"),
        LIT_ARGS("2020"), LIT_ARGS("Album A"), 10));
    assert(test_append_pipeline_song(
        &replacement_albums, LIT_ARGS("album-c.flac"),
        LIT_ARGS("Artist A"), NULL, 0, LIT_ARGS("Album C"),
        LIT_ARGS("2020"), LIT_ARGS("Album C"), 20));
    assert(test_append_pipeline_song(
        &replacement_albums, LIT_ARGS("album-d.flac"),
        LIT_ARGS("Artist A"), NULL, 0, LIT_ARGS("Album D"),
        LIT_ARGS("2020"), LIT_ARGS("Album D"), 30));
    assert(test_append_pipeline_song(
        &replacement_songs, LIT_ARGS("a.flac"),
        LIT_ARGS("Artist A"), NULL, 0, LIT_ARGS("Album A"),
        LIT_ARGS("2020"), LIT_ARGS("A"), 10));
    assert(test_append_pipeline_song(
        &replacement_songs, LIT_ARGS("c.flac"),
        LIT_ARGS("Artist A"), NULL, 0, LIT_ARGS("Album A"),
        LIT_ARGS("2020"), LIT_ARGS("C"), 20));
    assert(test_append_pipeline_song(
        &replacement_songs, LIT_ARGS("d.flac"),
        LIT_ARGS("Artist A"), NULL, 0, LIT_ARGS("Album A"),
        LIT_ARGS("2020"), LIT_ARGS("D"), 30));
    fixture.tag_results = &replacement_tags;

    native_media_library_screen_init(
        &screen, test_hooks(&fixture), 0, 90, 0, 24,
        nc_color_default(), nc_border_none());
    tags_menu = nc_media_library_tag_menu_base(&screen.tags);
    albums_menu = nc_media_library_album_menu_base(&screen.albums);
    songs_menu = nc_media_library_song_menu_base(&screen.songs);
    ncm_error_clear(&error);

    assert(native_media_library_screen_add_tag(
        &screen, LIT_ARGS("Artist B"), 0));
    assert(native_media_library_screen_add_tag(
        &screen, LIT_ARGS("Artist C"), 0));
    assert(nc_menu_goto_selectable(tags_menu, 1));
    nc_screen_finish_list_change(
        native_media_library_screen_base(&screen));
    assert(native_media_library_screen_add_album(
        &screen, LIT_ARGS("Artist C"), LIT_ARGS("Album C"),
        LIT_ARGS("2020"), 0, false));
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("kept.flac")));
    assert(native_media_library_screen_add_song_copy(&screen, &song));

    assert(native_media_library_screen_set_active_column(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS));
    assert(native_media_library_screen_apply_filter(
        &screen, LIT_ARGS("Artist C"), &error));
    assert(nc_menu_item_count(tags_menu) == 1);
    native_media_library_screen_request_tags_update(&screen);
    assert(native_media_library_screen_update(&screen, &error));
    current_tag = native_media_library_screen_current_tag(&screen);
    assert(current_tag);
    assert(ncm_string_equal(current_tag->tag, current_tag->tag_len,
                            LIT_ARGS("Artist C")));
    assert(nc_menu_all_item_count(tags_menu) == 3);
    assert(nc_menu_item_count(tags_menu) == 1);
    assert(nc_menu_all_item_count(albums_menu) == 1);
    assert(nc_menu_all_item_count(songs_menu) == 1);

    native_media_library_screen_clear_filter(&screen);
    current_tag = native_media_library_screen_current_tag(&screen);
    assert(current_tag);
    assert(ncm_string_equal(current_tag->tag, current_tag->tag_len,
                            LIT_ARGS("Artist A")));
    assert(nc_menu_all_item_count(albums_menu) == 0);
    assert(nc_menu_all_item_count(songs_menu) == 0);
    assert(screen.update_timer.ns == global_timer.ns);

    assert(native_media_library_screen_add_album(
        &screen, LIT_ARGS("Artist A"), LIT_ARGS("Album B"),
        LIT_ARGS("2020"), 0, false));
    assert(native_media_library_screen_add_album(
        &screen, LIT_ARGS("Artist A"), LIT_ARGS("Album C"),
        LIT_ARGS("2020"), 0, false));
    assert(nc_menu_goto_selectable(albums_menu, 1));
    nc_screen_finish_list_change(
        native_media_library_screen_base(&screen));
    assert(native_media_library_screen_add_song_copy(&screen, &song));

    assert(native_media_library_screen_set_active_column(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS));
    assert(native_media_library_screen_apply_filter(
        &screen, LIT_ARGS("Album C"), &error));
    fixture.search_results = &replacement_albums;
    native_media_library_screen_request_albums_update(&screen);
    assert(native_media_library_screen_update(&screen, &error));
    current_album = native_media_library_screen_current_album(&screen);
    assert(current_album);
    assert(ncm_string_equal(current_album->album,
                            current_album->album_len,
                            LIT_ARGS("Album C")));
    assert(nc_menu_item_count(albums_menu) == 3);
    assert(nc_menu_position_is_separator(albums_menu, 1));
    assert(nc_media_library_album_menu_item_at(
               &screen.albums, NC_MENU_ITEMS_FILTERED, 2)
               ->all_tracks_entry);
    assert(nc_menu_all_item_count(songs_menu) == 1);

    native_media_library_screen_clear_filter(&screen);
    current_album = native_media_library_screen_current_album(&screen);
    assert(current_album);
    assert(ncm_string_equal(current_album->album,
                            current_album->album_len,
                            LIT_ARGS("Album A")));
    assert(nc_menu_all_item_count(songs_menu) == 0);

    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("b.flac")));
    assert(native_media_library_screen_add_song_copy(&screen, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("c.flac")));
    assert(native_media_library_screen_add_song_copy(&screen, &song));
    assert(nc_menu_goto_selectable(songs_menu, 1));
    assert(native_media_library_screen_set_active_column(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_SONGS));
    assert(native_media_library_screen_apply_filter(
        &screen, LIT_ARGS("C"), &error));

    fixture.search_results = &replacement_songs;
    native_media_library_screen_request_songs_update(&screen);
    assert(native_media_library_screen_update(&screen, &error));
    assert(nc_menu_all_item_count(songs_menu) == 3);
    assert(nc_menu_item_count(songs_menu) == 1);
    current_song = nc_media_library_song_menu_current(&screen.songs);
    assert(current_song);
    assert(ncm_string_equal(current_song->uri, current_song->uri_len,
                            LIT_ARGS("c.flac")));

    ncm_song_destroy(&song);
    native_media_library_screen_destroy(&screen);
    ncm_mpd_song_list_destroy(&replacement_songs);
    ncm_mpd_song_list_destroy(&replacement_albums);
    ncm_mpd_string_list_destroy(&replacement_tags);
    ncm_format_ast_destroy(&Config.song_library_format);

    global_timer = old_global_timer;
    Config.song_library_format = old_song_format;
    Config.media_lib_primary_tag = old_primary_tag;
    Config.regex_type = old_regex_type;
    Config.data_fetching_delay = old_data_fetching_delay;
    Config.media_library_sort_by_mtime = old_sort_by_mtime;
    Config.media_library_albums_split_by_date = old_split_by_date;
    return;
}

static void
test_media_library_mode_transitions(void) {
    NativeMediaLibraryScreen screen;
    NativeMediaLibraryHooks hooks = {0};
    char *value;
    int32 value_len;

    native_media_library_screen_init(&screen, hooks, 0, 90, 0, 24,
                                     nc_color_default(),
                                     nc_border_none());
    assert(!native_media_library_screen_previous_column_available(&screen));
    assert(!native_media_library_screen_next_column_available(&screen));
    assert(native_media_library_screen_add_tag(
        &screen, LIT_ARGS("Artist A"), 0));
    assert(native_media_library_screen_add_album(
        &screen, LIT_ARGS("Artist A"), LIT_ARGS("Album A"),
        LIT_ARGS("2026"), 0, false));
    assert(native_media_library_screen_next_column_available(&screen));
    native_media_library_screen_next_column(&screen);
    assert(native_media_library_screen_active_column(&screen)
           == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS);
    assert(native_media_library_screen_previous_column_available(&screen));
    assert(!native_media_library_screen_next_column_available(&screen));

    assert(native_media_library_screen_set_mode(
        &screen, NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS));
    assert(native_media_library_screen_mode(&screen)
           == NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS);
    assert(!native_media_library_screen_set_active_column(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS));
    assert(native_media_library_screen_add_album(
        &screen, LIT_ARGS("Artist B"), LIT_ARGS("Album B"),
        LIT_ARGS("2025"), 0, false));
    assert(native_media_library_screen_current_primary_tag_value(
        &screen, &value, &value_len));
    assert(ncm_string_equal(value, value_len, LIT_ARGS("Artist B")));
    assert(native_media_library_screen_toggle_columns_mode(&screen)
           == NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY);
    assert(!native_media_library_screen_current_primary_tag_value(
        &screen, &value, &value_len));
    assert(native_media_library_screen_toggle_columns_mode(&screen)
           == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS);
    assert(native_media_library_screen_set_active_column(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS));

    native_media_library_screen_destroy(&screen);
    return;
}

static void
test_media_library_filter_search_actions(void) {
    NativeMediaLibraryScreen screen;
    NativeMediaLibraryHooks hooks = {0};
    NcmSong song;
    NcmFormatAst old_song_format;
    enum NcmSongGetter title_getter;
    char *old_empty_tag;
    int32 old_empty_tag_len;
    int32 old_empty_tag_cap;
    uint32 old_regex_type;
    NcmError error;

    old_song_format = Config.song_library_format;
    old_empty_tag = Config.empty_tag;
    old_empty_tag_len = Config.empty_tag_len;
    old_empty_tag_cap = Config.empty_tag_cap;
    old_regex_type = Config.regex_type;
    ncm_format_ast_init(&Config.song_library_format);
    title_getter = NCM_SONG_GETTER_TITLE;
    assert(ncm_format_ast_append_first_of_getters(
        &Config.song_library_format, &title_getter, 1));
    Config.empty_tag = (char *)"Unknown Artist";
    Config.empty_tag_len = STRLIT_LEN("Unknown Artist");
    Config.empty_tag_cap = 0;
    Config.regex_type = NCM_REGEX_LITERAL_CASE_INSENSITIVE;

    native_media_library_screen_init(&screen, hooks, 0, 90, 0, 24,
                                     nc_color_default(),
                                     nc_border_none());
    ncm_song_init(&song);
    ncm_error_clear(&error);

    assert(native_media_library_screen_add_tag(&screen, NULL, 0, 0));
    assert(native_media_library_screen_search(
        &screen, LIT_ARGS("Unknown"), true, true, false, &error));

    assert(native_media_library_screen_set_active_column(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS));
    assert(native_media_library_screen_add_album(
        &screen, LIT_ARGS("Artist A"), NULL, 0, NULL, 0, 0, true));
    assert(native_media_library_screen_apply_filter(
        &screen, LIT_ARGS("no match"), &error));
    assert(nc_menu_item_count(
        nc_media_library_album_menu_base(&screen.albums)) == 1);
    assert(!native_media_library_screen_search(
        &screen, LIT_ARGS("All tracks"), true, true, false, &error));
    native_media_library_screen_clear_filter(&screen);

    assert(native_media_library_screen_set_active_column(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_SONGS));
    assert(test_set_library_song(
        &song, LIT_ARGS("uri-only.flac"), LIT_ARGS("Album A"),
        LIT_ARGS("2026"), LIT_ARGS("1"), LIT_ARGS("1"),
        LIT_ARGS("Needle Title"), 0));
    assert(native_media_library_screen_add_song_copy(&screen, &song));
    assert(native_media_library_screen_search(
        &screen, LIT_ARGS("Needle"), true, true, false, &error));
    assert(!native_media_library_screen_search(
        &screen, LIT_ARGS("uri-only"), true, true, false, &error));

    ncm_song_destroy(&song);
    native_media_library_screen_destroy(&screen);
    ncm_format_ast_destroy(&Config.song_library_format);
    Config.song_library_format = old_song_format;
    Config.empty_tag = old_empty_tag;
    Config.empty_tag_len = old_empty_tag_len;
    Config.empty_tag_cap = old_empty_tag_cap;
    Config.regex_type = old_regex_type;
    return;
}

static void
test_media_library_selected_songs(void) {
    NativeMediaLibraryScreen screen;
    MediaLibraryHookFixture fixture = {0};
    NativeMediaLibraryHooks hooks;
    NcmMpdSongList search_results;
    NcmSong song;
    NcmSong current_copy;
    NcmSongArray selected;
    NcMenu *tags_menu;
    NcMenu *songs_menu;
    NcmError error;
    bool old_split_by_date;
    enum mpd_tag_type old_primary_tag;

    old_split_by_date = Config.media_library_albums_split_by_date;
    old_primary_tag = Config.media_lib_primary_tag;
    Config.media_library_albums_split_by_date = true;
    Config.media_lib_primary_tag = MPD_TAG_ARTIST;

    ncm_mpd_song_list_init(&search_results);
    ncm_song_init(&song);
    ncm_song_init(&current_copy);
    ncm_song_array_init(&selected);
    assert(test_append_pipeline_song(
        &search_results, LIT_ARGS("tag-b.flac"),
        LIT_ARGS("Artist A"), NULL, 0, LIT_ARGS("Album B"),
        LIT_ARGS("2026"), LIT_ARGS("Beta"), 0));
    assert(test_append_pipeline_song(
        &search_results, LIT_ARGS("tag-a.flac"),
        LIT_ARGS("Artist A"), NULL, 0, LIT_ARGS("Album A"),
        LIT_ARGS("2026"), LIT_ARGS("Alpha"), 0));
    fixture.search_results = &search_results;
    hooks = test_hooks(&fixture);

    native_media_library_screen_init(&screen, hooks, 0, 90, 0, 24,
                                     nc_color_default(),
                                     nc_border_none());
    tags_menu = nc_media_library_tag_menu_base(&screen.tags);
    songs_menu = nc_media_library_song_menu_base(&screen.songs);
    ncm_error_clear(&error);

    assert(native_media_library_screen_add_tag(
        &screen, LIT_ARGS("Artist A"), 0));
    assert(native_media_library_screen_selected_songs_checked(
        &screen, &selected, &error));
    assert(selected.len == 2);
    assert(fixture.search_songs_calls == 1);
    assert(fixture.last_match_primary_tag);
    assert(ncm_string_equal(fixture.last_primary_value,
                            fixture.last_primary_value_len,
                            LIT_ARGS("Artist A")));
    ncm_song_array_clear(&selected);

    assert(native_media_library_screen_add_tag(
        &screen, LIT_ARGS("Artist B"), 0));
    assert(nc_menu_set_position_selected(tags_menu, 1, true));
    fixture.search_songs_calls = 0;
    fixture.add_songs_calls = 0;
    assert(native_media_library_screen_add_item_to_playlist(
        &screen, false, &error));
    assert(fixture.search_songs_calls == 1);
    assert(fixture.add_songs_calls == 1);
    assert(fixture.added_song_count == 2);
    assert(!fixture.added_with_play);
    assert(ncm_string_equal(fixture.last_primary_value,
                            fixture.last_primary_value_len,
                            LIT_ARGS("Artist A")));
    assert(nc_menu_set_position_selected(tags_menu, 1, false));

    assert(native_media_library_screen_add_album(
        &screen, LIT_ARGS("Artist A"), LIT_ARGS("Album A"),
        LIT_ARGS("2026"), 0, false));
    assert(native_media_library_screen_set_active_column(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS));
    assert(native_media_library_screen_selected_songs_checked(
        &screen, &selected, &error));
    assert(selected.len == 2);
    assert(fixture.last_match_primary_tag);
    assert(fixture.last_match_album);
    assert(fixture.last_match_date);
    assert(ncm_string_equal(fixture.last_album, fixture.last_album_len,
                            LIT_ARGS("Album A")));
    ncm_song_array_clear(&selected);

    assert(native_media_library_screen_current_album_songs(
        &screen, &selected, &error));
    assert(selected.len == 2);
    assert(fixture.search_songs_calls == 3);
    assert(fixture.last_match_primary_tag);
    assert(fixture.last_match_album);
    assert(ncm_string_equal(fixture.last_primary_value,
                            fixture.last_primary_value_len,
                            LIT_ARGS("Artist A")));
    ncm_song_array_clear(&selected);

    assert(native_media_library_screen_set_mode(
        &screen, NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY));
    assert(native_media_library_screen_add_album(
        &screen, NULL, 0, LIT_ARGS("Album Only"),
        LIT_ARGS("2026"), 0, false));
    assert(native_media_library_screen_selected_songs_checked(
        &screen, &selected, &error));
    assert(fixture.last_match_album);
    assert(!fixture.last_match_primary_tag);
    ncm_song_array_clear(&selected);

    assert(native_media_library_screen_set_mode(
        &screen, NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS));
    assert(native_media_library_screen_set_active_column(
        &screen, NATIVE_MEDIA_LIBRARY_COLUMN_SONGS));
    assert(ncm_song_set_uri(&song, LIT_ARGS("first.flac")));
    assert(native_media_library_screen_add_song_copy(&screen, &song));
    assert(ncm_song_set_uri(&song, LIT_ARGS("second.flac")));
    assert(native_media_library_screen_add_song_copy(&screen, &song));
    assert(nc_menu_goto_selectable(songs_menu, 1));
    assert(native_media_library_screen_visible_song_count(&screen) == 2);
    assert(native_media_library_screen_current_selected_song(
        &screen, &current_copy));
    assert(ncm_string_equal(current_copy.uri, current_copy.uri_len,
                            LIT_ARGS("second.flac")));
    assert(native_media_library_screen_selected_songs_checked(
        &screen, &selected, &error));
    assert(selected.len == 1);
    assert(nc_menu_set_position_selected(songs_menu, 0, true));
    ncm_song_array_clear(&selected);
    assert(native_media_library_screen_selected_songs_checked(
        &screen, &selected, &error));
    assert(selected.len == 1);

    fixture.add_songs_calls = 0;
    fixture.added_song_count = 0;
    fixture.added_uri_len = 0;
    assert(native_media_library_screen_item_available(&screen));
    assert(native_media_library_screen_add_item_to_playlist(
        &screen, true, &error));
    assert(fixture.add_songs_calls == 1);
    assert(fixture.added_song_count == 1);
    assert(fixture.added_with_play);
    assert(ncm_string_equal(fixture.added_uri, fixture.added_uri_len,
                            LIT_ARGS("second.flac")));

    ncm_song_array_destroy(&selected);
    ncm_song_destroy(&current_copy);
    ncm_song_destroy(&song);
    native_media_library_screen_destroy(&screen);
    ncm_mpd_song_list_destroy(&search_results);
    Config.media_library_albums_split_by_date = old_split_by_date;
    Config.media_lib_primary_tag = old_primary_tag;
    return;
}

static void
test_media_library_locate_song(void) {
    NativeMediaLibraryScreen screen;
    MediaLibraryHookFixture fixture = {0};
    NcmMpdSongList search_results;
    NcmSong song;
    NcmSong *current_song;
    NcMediaLibraryTagRow *current_tag;
    NcMediaLibraryAlbumRow *current_album;
    NcMenu *tags_menu;
    NcmError error;
    NcmTimePoint old_global_timer;
    enum mpd_tag_type old_primary_tag;
    uint32 old_regex_type;
    bool old_data_fetching_delay;
    bool old_sort_by_mtime;
    bool old_split_by_date;

    old_global_timer = global_timer;
    old_primary_tag = Config.media_lib_primary_tag;
    old_regex_type = Config.regex_type;
    old_data_fetching_delay = Config.data_fetching_delay;
    old_sort_by_mtime = Config.media_library_sort_by_mtime;
    old_split_by_date = Config.media_library_albums_split_by_date;

    global_timer.ns = 1000000000ll;
    Config.media_lib_primary_tag = MPD_TAG_ARTIST;
    Config.regex_type = NCM_REGEX_LITERAL_CASE_INSENSITIVE;
    Config.data_fetching_delay = false;
    Config.media_library_sort_by_mtime = false;
    Config.media_library_albums_split_by_date = true;

    ncm_mpd_song_list_init(&search_results);
    ncm_song_init(&song);
    assert(test_append_pipeline_song(
        &search_results, LIT_ARGS("target.flac"),
        LIT_ARGS("Artist A"), NULL, 0, LIT_ARGS("Album A"),
        NULL, 0, LIT_ARGS("Target"), 10));
    fixture.search_results = &search_results;

    assert(test_set_library_song(
        &song, LIT_ARGS("target.flac"), LIT_ARGS("Album A"),
        LIT_ARGS("2020"), NULL, 0, NULL, 0, LIT_ARGS("Target"),
        10));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST,
                            LIT_ARGS("Artist A")));

    native_media_library_screen_init(
        &screen, test_hooks(&fixture), 0, 90, 0, 24,
        nc_color_default(), nc_border_none());
    tags_menu = nc_media_library_tag_menu_base(&screen.tags);
    assert(native_media_library_screen_add_tag(
        &screen, LIT_ARGS("Artist Z"), 0));
    assert(native_media_library_screen_apply_filter(
        &screen, LIT_ARGS("Artist Z"), &error));
    assert(nc_menu_item_count(tags_menu) == 1);

    ncm_error_clear(&error);
    assert(native_media_library_screen_locate_song(&screen, &song,
                                                   &error));
    assert(error.code == 0);
    assert(fixture.search_songs_calls == 2);
    assert(!screen.column_state[
        NATIVE_MEDIA_LIBRARY_COLUMN_TAGS].filter_enabled);
    assert(nc_menu_item_count(tags_menu) == nc_menu_all_item_count(
        tags_menu));

    current_tag = native_media_library_screen_current_tag(&screen);
    assert(current_tag);
    assert(ncm_string_equal(current_tag->tag, current_tag->tag_len,
                            LIT_ARGS("Artist A")));
    current_album = native_media_library_screen_current_album(&screen);
    assert(current_album);
    assert(ncm_string_equal(current_album->album,
                            current_album->album_len,
                            LIT_ARGS("Album A")));
    assert(current_album->date_len == 0);
    assert(native_media_library_screen_active_column(&screen)
           == NATIVE_MEDIA_LIBRARY_COLUMN_SONGS);
    current_song = native_media_library_screen_visible_song_at(
        &screen, nc_menu_highlight(
            nc_media_library_song_menu_base(&screen.songs)));
    assert(current_song);
    assert(ncm_string_equal(current_song->uri, current_song->uri_len,
                            LIT_ARGS("target.flac")));

    native_media_library_screen_destroy(&screen);
    ncm_song_destroy(&song);
    ncm_mpd_song_list_destroy(&search_results);
    global_timer = old_global_timer;
    Config.media_lib_primary_tag = old_primary_tag;
    Config.regex_type = old_regex_type;
    Config.data_fetching_delay = old_data_fetching_delay;
    Config.media_library_sort_by_mtime = old_sort_by_mtime;
    Config.media_library_albums_split_by_date = old_split_by_date;
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
    bool result;

    fixture = user;
    fixture->list_tags_calls += 1;
    if (fixture->fail || fixture->fail_list_tags) {
        if (fixture->append_before_failure) {
            assert(ncm_mpd_string_list_append(
                tags, LIT_ARGS("Partial tag")));
        }
        ncm_error_set(error, EIO, STRLIT_ARGS("test tag error"));
        return false;
    }
    assert(tag_type == MPD_TAG_ARTIST);
    if (fixture->tag_results) {
        result = ncm_mpd_string_list_copy(tags, fixture->tag_results);
        if (!result) {
            ncm_error_set(error, ENOMEM,
                          STRLIT_ARGS("could not copy test tags"));
        }
        return result;
    }
    assert(ncm_mpd_string_list_append(tags, LIT_ARGS("Artist A")));
    assert(ncm_mpd_string_list_append(tags, LIT_ARGS("Artist B")));
    return true;
}

static bool
test_list_all_songs(void *user, NcmMpdSongList *songs,
                    NcmError *error) {
    MediaLibraryHookFixture *fixture;
    bool result;

    fixture = user;
    fixture->list_all_songs_calls += 1;
    if (fixture->fail || fixture->fail_list_all_songs) {
        if (fixture->append_before_failure) {
            assert(test_append_song(
                songs, LIT_ARGS("partial-list.flac")));
        }
        ncm_error_set(error, EIO, STRLIT_ARGS("test song-list error"));
        return false;
    }
    if (fixture->all_song_results) {
        result = ncm_mpd_song_list_copy(songs,
                                        fixture->all_song_results);
        if (!result) {
            ncm_error_set(error, ENOMEM,
                          STRLIT_ARGS("could not copy test songs"));
        }
        return result;
    }
    return test_append_song(songs, LIT_ARGS("all.flac"));
}

static bool
test_search_songs(void *user, NativeMediaLibrarySongQuery *query,
                  NcmMpdSongList *songs, NcmError *error) {
    MediaLibraryHookFixture *fixture;
    bool result;

    fixture = user;
    fixture->search_songs_calls += 1;
    fixture->last_primary_tag = query->primary_tag;
    fixture->last_match_primary_tag = query->match_primary_tag;
    fixture->last_match_album = query->match_album;
    fixture->last_match_date = query->match_date;
    test_record_query_value(
        fixture->last_primary_value,
        &fixture->last_primary_value_len,
        query->primary_value, query->primary_value_len);
    test_record_query_value(
        fixture->last_album, &fixture->last_album_len,
        query->album, query->album_len);
    test_record_query_value(
        fixture->last_date, &fixture->last_date_len,
        query->date, query->date_len);

    if (fixture->fail || fixture->fail_search_songs) {
        if (fixture->append_before_failure) {
            assert(test_append_song(
                songs, LIT_ARGS("partial-search.flac")));
        }
        ncm_error_set(error, EIO, STRLIT_ARGS("test search error"));
        return false;
    }
    if (fixture->search_results) {
        result = ncm_mpd_song_list_copy(songs,
                                        fixture->search_results);
        if (!result) {
            ncm_error_set(error, ENOMEM,
                          STRLIT_ARGS("could not copy search songs"));
        }
        return result;
    }
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
    fixture->added_uri_len = 0;
    if (songs->len > 0) {
        test_record_query_value(
            fixture->added_uri, &fixture->added_uri_len,
            songs->items[0].uri, songs->items[0].uri_len);
    }
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

static bool
test_append_pipeline_song(
    NcmMpdSongList *songs, char *uri, int32 uri_len,
    char *artist_one, int32 artist_one_len,
    char *artist_two, int32 artist_two_len,
    char *album, int32 album_len, char *date, int32 date_len,
    char *title, int32 title_len, time_t mtime) {
    NcmSong song;
    bool result;

    ncm_song_init(&song);
    result = test_set_library_song(
        &song, uri, uri_len, album, album_len, date, date_len,
        NULL, 0, NULL, 0, title, title_len, mtime);
    if (result && (artist_one != NULL)) {
        result = ncm_song_add_tag(&song, MPD_TAG_ARTIST,
                                  artist_one, artist_one_len);
    }
    if (result && (artist_two != NULL)) {
        result = ncm_song_add_tag(&song, MPD_TAG_ARTIST,
                                  artist_two, artist_two_len);
    }
    if (result) {
        result = ncm_mpd_song_list_append_copy(songs, &song);
    }
    ncm_song_destroy(&song);
    return result;
}

static void
test_record_query_value(char *dest, int32 *dest_len,
                        char *source, int32 source_len) {
    assert(dest);
    assert(dest_len);
    assert(source_len >= 0);
    assert(source_len < 128);
    assert((source != NULL) || (source_len == 0));

    for (int32 i = 0; i < source_len; i += 1) {
        dest[i] = source[i];
    }
    dest[source_len] = '\0';
    *dest_len = source_len;
    return;
}
