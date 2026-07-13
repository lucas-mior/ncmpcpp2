#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_string.h"
#include "screens/nc_search_engine.h"
#include "settings.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

#define SEARCH_TAG_CALL_CAPACITY 9

typedef struct MpdSearchFixture {
    int32 start_calls;
    int32 any_calls;
    int32 uri_calls;
    int32 tag_calls;
    int32 commit_calls;
    bool exact_match;
    bool fail_start;
    bool fail_add;
    bool fail_commit;
    NcmBuffer any;
    NcmBuffer uri;
    enum mpd_tag_type tags[SEARCH_TAG_CALL_CAPACITY];
    NcmBuffer tag_values[SEARCH_TAG_CALL_CAPACITY];
    NcmSongArray results;
} MpdSearchFixture;

typedef struct SearchSourceFixture {
    int32 database_calls;
    int32 playlist_calls;
    int32 status_calls;
    bool fail_database;
    bool fail_playlist;
    NcmBuffer status;
    NcmSongArray database;
    NcmSongArray playlist;
} SearchSourceFixture;

typedef struct ExternalHookFixture {
    int32 database_calls;
    int32 playlist_calls;
    int32 prompt_calls;
    int32 status_calls;
    int32 add_calls;
    enum NativeSearchEnginePromptResult prompt_result;
    bool play;
    NcmBuffer prompt_label;
    NcmBuffer prompt_initial;
    NcmBuffer status;
} ExternalHookFixture;

typedef struct WindowTrace {
    int32 display_calls;
    int32 menu_refresh_calls;
    int32 property_calls;
    int32 printed_len;
    int64 refresh_width;
    int64 refresh_height;
    char printed[512];
    char title[512];
} WindowTrace;

static MpdSearchFixture mpd_search_fixture;
static WindowTrace window_trace;
static int64 resize_x;
static int64 resize_width;
static int64 resize_main_y;
static int64 resize_main_height;

static char *constraint_rows[] = {
    (char *)"Any          : ",
    (char *)"Artist       : ",
    (char *)"Album Artist : ",
    (char *)"Title        : ",
    (char *)"Album        : ",
    (char *)"Filename     : ",
    (char *)"Composer     : ",
    (char *)"Performer    : ",
    (char *)"Genre        : ",
    (char *)"Date         : ",
    (char *)"Comment      : ",
};

static char *search_mode_rows[] = {
    (char *)"Search mode: Match if tag contains searched phrase "
            "(no regexes)",
    (char *)"Search mode: Match if tag contains searched phrase "
            "(regexes supported)",
    (char *)"Search mode: Match only if both values are the same",
};

static void test_search_query_construction(void);
static void test_owned_screen_state(void);
static void test_external_operation_hooks(void);
static void test_static_row_layout(void);
static void test_search_modes_and_sources(void);
static void test_static_row_updates(void);
static void test_search_result_row_layout(void);
static void test_blocked_constraint_flags(void);
static void test_search_reset_contract(void);
static void test_search_result_row_ownership(void);
static void test_search_selected_songs(void);
static void test_formatted_song_filter_and_find(void);
static void test_database_search_execution(void);
static void test_local_search_execution(void);
static void test_search_execution_failures(void);
static void test_native_row_activation(void);
static void test_native_display_and_column_title(void);
static void test_native_lifecycle(void);
static void test_native_mouse_behavior(void);
static void reset_window_trace(void);
static void assert_printed(char *expected, int32 expected_len);
static void init_screen(NativeSearchEngineScreen *screen);
static void add_result_songs(NativeSearchEngineScreen *screen);
static NcSearchRow *row_at(NativeSearchEngineScreen *screen, int64 pos);
static void assert_row_text(NativeSearchEngineScreen *screen, int64 pos,
                            char *expected, int32 expected_len);
static void assert_row_format(NativeSearchEngineScreen *screen, int64 pos,
                              int32 property, int32 position,
                              enum NcFormat format);
static void assert_song_uri(NcmSong *song, char *expected,
                            int32 expected_len);
static int32 string_len(char *string);
static bool format_song_title(void *user, NcmSong *song, NcmBuffer *text);
static void init_mpd_search_fixture(void);
static void destroy_mpd_search_fixture(void);
static void append_test_song(NcmSongArray *songs, char *uri,
                             int32 uri_len, char *artist,
                             int32 artist_len, char *title,
                             int32 title_len, char *genre,
                             int32 genre_len);
static bool search_list_database_songs(void *user,
                                       NcmSongArray *songs,
                                       NcmError *error);
static bool search_snapshot_playlist(void *user, NcmSongArray *songs,
                                     NcmError *error);
static void search_status_message(void *user, char *message,
                                  int32 message_len);
static bool list_database_songs(void *user, NcmSongArray *songs,
                                NcmError *error);
static bool snapshot_playlist(void *user, NcmSongArray *songs,
                              NcmError *error);
static enum NativeSearchEnginePromptResult prompt_constraint(
    void *user, char *label, int32 label_len, NcmBuffer *initial,
    NcmBuffer *result);
static void status_message(void *user, char *message, int32 message_len);
static bool add_song(void *user, NcmSong *song, bool play,
                     NcmError *error);
static bool mouse_add_song(void *user, NcmSong *song, bool play,
                           NcmError *error);
void __wrap_nc_window_set_title(NcWindow *window, char *title,
                                int32 title_len);
bool __wrap_ncm_mpd_client_start_search(NcmMpdClient *client,
                                        bool exact_match,
                                        NcmError *error);
bool __wrap_ncm_mpd_client_add_search_tag(NcmMpdClient *client,
                                          enum mpd_tag_type tag,
                                          char *value, NcmError *error);
bool __wrap_ncm_mpd_client_add_search_any(NcmMpdClient *client,
                                          char *value, NcmError *error);
bool __wrap_ncm_mpd_client_add_search_uri(NcmMpdClient *client,
                                          char *value, NcmError *error);
bool __wrap_ncm_mpd_client_commit_search_songs(
    NcmMpdClient *client, NcmMpdSongList *songs, NcmError *error);

int
main(void) {
    test_search_query_construction();
    test_owned_screen_state();
    test_external_operation_hooks();
    test_static_row_layout();
    test_search_modes_and_sources();
    test_static_row_updates();
    test_search_result_row_layout();
    test_blocked_constraint_flags();
    test_search_reset_contract();
    test_search_result_row_ownership();
    test_search_selected_songs();
    test_formatted_song_filter_and_find();
    test_database_search_execution();
    test_local_search_execution();
    test_search_execution_failures();
    test_native_row_activation();
    test_native_display_and_column_title();
    test_native_lifecycle();
    test_native_mouse_behavior();
    exit(EXIT_SUCCESS);
}

static void
init_screen(NativeSearchEngineScreen *screen) {
    native_search_engine_screen_init(screen, 0, 80, 0, 24,
                                     nc_color_default(),
                                     nc_border_none());
    return;
}

static void
add_result_songs(NativeSearchEngineScreen *screen) {
    NcmSong song;

    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("first.flac")));
    assert(native_search_engine_screen_add_song_copy(screen, &song));
    assert(ncm_song_set_uri(&song, LIT_ARGS("second.flac")));
    assert(native_search_engine_screen_add_song_copy(screen, &song));
    ncm_song_destroy(&song);
    return;
}

static NcSearchRow *
row_at(NativeSearchEngineScreen *screen, int64 pos) {
    return nc_search_row_menu_item_at(
        native_search_engine_screen_rows(screen), NC_MENU_ITEMS_ALL, pos);
}

static void
assert_row_text(NativeSearchEngineScreen *screen, int64 pos,
                char *expected, int32 expected_len) {
    NcSearchRow *row;

    row = row_at(screen, pos);
    assert(row != NULL);
    assert(!row->is_song);
    assert(ncm_string_equal(row->buffer.data, row->buffer.len,
                            expected, expected_len));
    return;
}

static void
assert_row_format(NativeSearchEngineScreen *screen, int64 pos,
                  int32 property, int32 position,
                  enum NcFormat format) {
    NcSearchRow *row;
    NcBufferProperty *properties;

    row = row_at(screen, pos);
    assert(row != NULL);
    assert(!row->is_song);
    assert(property >= 0);
    assert(property < nc_buffer_property_count(&row->buffer));
    properties = nc_buffer_properties(&row->buffer);
    assert(properties[property].type == NC_BUFFER_PROPERTY_FORMAT);
    assert(properties[property].position == position);
    assert(properties[property].value.format == format);
    return;
}

static int32
string_len(char *string) {
    int32 len;

    len = 0;
    while (string[len] != '\0') {
        len += 1;
    }
    return len;
}

static void
assert_song_uri(NcmSong *song, char *expected, int32 expected_len) {
    NcmStringView view;

    assert(ncm_song_uri_view(song, 0, &view));
    assert(ncm_string_equal(view.data, view.len,
                            expected, expected_len));
    return;
}

static void
test_owned_screen_state(void) {
    NativeSearchEngineScreen screen;
    NcmStringView view;

    init_screen(&screen);
    assert(!native_search_engine_screen_is_prepared(&screen));
    assert(!native_search_engine_screen_has_result_rows(&screen));
    assert(native_search_engine_screen_result_count(&screen) == 0);
    assert(!native_search_engine_screen_constraints_locked(&screen));

    view = native_search_engine_screen_title(&screen);
    assert(ncm_string_equal(view.data, view.len,
                            LIT_ARGS("Search engine")));
    assert(native_search_engine_screen_set_title(
        &screen, LIT_ARGS("Search results")));
    view = native_search_engine_screen_title(&screen);
    assert(ncm_string_equal(view.data, view.len,
                            LIT_ARGS("Search results")));
    assert(native_search_engine_screen_set_column_title(
        &screen, LIT_ARGS("Artist | Album | Title")));
    view = native_search_engine_screen_column_title(&screen);
    assert(ncm_string_equal(view.data, view.len,
                            LIT_ARGS("Artist | Album | Title")));

    assert(native_search_engine_screen_set_constraint(
        &screen, 3, LIT_ARGS("needle")));
    assert(native_search_engine_screen_set_search_mode(
        &screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_REGEX));
    native_search_engine_screen_set_search_source(&screen, false);
    native_search_engine_screen_prepare_static_rows(&screen);
    assert(native_search_engine_screen_is_prepared(&screen));

    add_result_songs(&screen);
    assert(native_search_engine_screen_add_result_summary(&screen, 2));
    assert(native_search_engine_screen_has_result_rows(&screen));
    assert(native_search_engine_screen_result_count(&screen) == 2);
    native_search_engine_screen_set_constraints_locked(&screen, true);
    assert(native_search_engine_screen_constraints_locked(&screen));

    native_search_engine_screen_clear(&screen);
    assert(!native_search_engine_screen_is_prepared(&screen));
    assert(!native_search_engine_screen_has_result_rows(&screen));
    assert(native_search_engine_screen_result_count(&screen) == 0);
    assert(!native_search_engine_screen_constraints_locked(&screen));
    assert(native_search_engine_screen_search_mode(&screen)
           == NATIVE_SEARCH_ENGINE_SEARCH_MODE_REGEX);
    assert(!native_search_engine_screen_searches_database(&screen));
    view = native_search_engine_screen_constraint(&screen, 3);
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("needle")));

    native_search_engine_screen_destroy(&screen);
    return;
}

static bool
list_database_songs(void *user, NcmSongArray *songs, NcmError *error) {
    ExternalHookFixture *fixture;
    NcmSong song;

    (void)error;
    fixture = user;
    fixture->database_calls += 1;
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("database.flac")));
    assert(ncm_song_array_append_copy(songs, &song));
    ncm_song_destroy(&song);
    return true;
}

static bool
snapshot_playlist(void *user, NcmSongArray *songs, NcmError *error) {
    ExternalHookFixture *fixture;
    NcmSong song;

    (void)error;
    fixture = user;
    fixture->playlist_calls += 1;
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("playlist.flac")));
    assert(ncm_song_array_append_copy(songs, &song));
    ncm_song_destroy(&song);
    return true;
}

static enum NativeSearchEnginePromptResult
prompt_constraint(void *user, char *label, int32 label_len,
                  NcmBuffer *initial, NcmBuffer *result) {
    ExternalHookFixture *fixture;

    fixture = user;
    fixture->prompt_calls += 1;
    assert(ncm_buffer_set(&fixture->prompt_label, label, label_len));
    assert(ncm_buffer_set(&fixture->prompt_initial,
                          initial->data, initial->len));
    if (fixture->prompt_result
        == NATIVE_SEARCH_ENGINE_PROMPT_ACCEPTED) {
        assert(ncm_buffer_set(result, LIT_ARGS("new title")));
    }
    return fixture->prompt_result;
}

static void
status_message(void *user, char *message, int32 message_len) {
    ExternalHookFixture *fixture;

    fixture = user;
    fixture->status_calls += 1;
    assert(ncm_buffer_set(&fixture->status, message, message_len));
    return;
}

static bool
add_song(void *user, NcmSong *song, bool play, NcmError *error) {
    ExternalHookFixture *fixture;
    NcmStringView uri;

    (void)error;
    fixture = user;
    fixture->add_calls += 1;
    fixture->play = play;
    assert(ncm_song_uri_view(song, 0, &uri));
    assert(ncm_string_equal(uri.data, uri.len, LIT_ARGS("result.flac")));
    return true;
}

static bool
mouse_add_song(void *user, NcmSong *song, bool play, NcmError *error) {
    ExternalHookFixture *fixture;

    (void)song;
    (void)error;
    fixture = user;
    fixture->add_calls += 1;
    fixture->play = play;
    return true;
}

static void
test_external_operation_hooks(void) {
    NativeSearchEngineScreen screen;
    NativeSearchEngineHooks hooks = {0};
    ExternalHookFixture fixture = {0};
    NcmSongArray songs;
    NcmBuffer prompt_result;
    NcmSong song;
    NcmError error;

    ncm_buffer_init(&fixture.prompt_label);
    ncm_buffer_init(&fixture.prompt_initial);
    ncm_buffer_init(&fixture.status);
    fixture.prompt_result = NATIVE_SEARCH_ENGINE_PROMPT_ACCEPTED;
    init_screen(&screen);
    ncm_song_array_init(&songs);
    ncm_buffer_init(&prompt_result);
    ncm_song_init(&song);

    hooks.list_database_songs = list_database_songs;
    hooks.snapshot_playlist = snapshot_playlist;
    hooks.prompt_constraint = prompt_constraint;
    hooks.status_message = status_message;
    hooks.add_song = add_song;
    hooks.user = &fixture;
    native_search_engine_screen_set_hooks(&screen, hooks);
    assert(native_search_engine_screen_set_constraint(
        &screen, 3, LIT_ARGS("old title")));

    ncm_error_clear(&error);
    assert(native_search_engine_screen_list_database_songs(
        &screen, &songs, &error));
    assert(fixture.database_calls == 1);
    assert(songs.len == 1);
    assert_song_uri(&songs.items[0], LIT_ARGS("database.flac"));

    ncm_song_array_clear(&songs);
    ncm_error_clear(&error);
    assert(native_search_engine_screen_snapshot_playlist(
        &screen, &songs, &error));
    assert(fixture.playlist_calls == 1);
    assert(songs.len == 1);
    assert_song_uri(&songs.items[0], LIT_ARGS("playlist.flac"));

    assert(native_search_engine_screen_prompt_constraint(
        &screen, 3, &prompt_result)
           == NATIVE_SEARCH_ENGINE_PROMPT_ACCEPTED);
    assert(fixture.prompt_calls == 1);
    assert(ncm_string_equal(fixture.prompt_label.data,
                            fixture.prompt_label.len,
                            LIT_ARGS("Title")));
    assert(ncm_string_equal(fixture.prompt_initial.data,
                            fixture.prompt_initial.len,
                            LIT_ARGS("old title")));
    assert(ncm_string_equal(prompt_result.data, prompt_result.len,
                            LIT_ARGS("new title")));

    native_search_engine_screen_status_message(
        &screen, LIT_ARGS("Searching..."));
    assert(fixture.status_calls == 1);
    assert(ncm_string_equal(fixture.status.data, fixture.status.len,
                            LIT_ARGS("Searching...")));

    assert(ncm_song_set_uri(&song, LIT_ARGS("result.flac")));
    ncm_error_clear(&error);
    assert(native_search_engine_screen_add_song(
        &screen, &song, true, &error));
    assert(fixture.add_calls == 1);
    assert(fixture.play);

    ncm_song_destroy(&song);
    ncm_buffer_destroy(&prompt_result);
    ncm_song_array_destroy(&songs);
    native_search_engine_screen_destroy(&screen);
    ncm_buffer_destroy(&fixture.status);
    ncm_buffer_destroy(&fixture.prompt_initial);
    ncm_buffer_destroy(&fixture.prompt_label);
    return;
}

static void
test_search_query_construction(void) {
    NativeSearchEngineScreen screen;
    NcmBuffer query;

    init_screen(&screen);
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
test_static_row_layout(void) {
    NativeSearchEngineScreen screen;
    NcMenu *menu;
    uint32 expected_flags;

    init_screen(&screen);
    native_search_engine_screen_prepare_static_rows(&screen);
    menu = native_search_engine_screen_menu(&screen);

    assert(nc_menu_all_item_count(menu)
           == NATIVE_SEARCH_ENGINE_RESET_BUTTON_ROW + 1);
    for (int32 i = 0; i <= NATIVE_SEARCH_ENGINE_RESET_BUTTON_ROW; i += 1) {
        expected_flags = 0;
        if (i == NATIVE_SEARCH_ENGINE_FIRST_SEPARATOR_ROW
            || i == NATIVE_SEARCH_ENGINE_SECOND_SEPARATOR_ROW) {
            expected_flags = NC_MENU_ITEM_SEPARATOR;
        }
        assert(nc_menu_item_flags_at(menu, NC_MENU_ITEMS_ALL, i)
               == expected_flags);
    }

    for (int32 i = 0; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        assert_row_text(&screen, i, constraint_rows[i],
                        string_len(constraint_rows[i]));
    }
    assert_row_text(&screen, NATIVE_SEARCH_ENGINE_SEARCH_SOURCE_ROW,
                    LIT_ARGS("Search in: Database"));
    assert_row_text(&screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_ROW,
                    search_mode_rows[
                        NATIVE_SEARCH_ENGINE_SEARCH_MODE_LITERAL],
                    string_len(search_mode_rows[
                        NATIVE_SEARCH_ENGINE_SEARCH_MODE_LITERAL]));
    assert_row_text(&screen, NATIVE_SEARCH_ENGINE_SEARCH_BUTTON_ROW,
                    LIT_ARGS("Search"));
    assert_row_text(&screen, NATIVE_SEARCH_ENGINE_RESET_BUTTON_ROW,
                    LIT_ARGS("Reset"));
    assert_row_format(&screen, 0, 0, 0, NC_FORMAT_BOLD);
    assert_row_format(&screen, 0, 1, 13, NC_FORMAT_NO_BOLD);
    assert_row_format(&screen, NATIVE_SEARCH_ENGINE_SEARCH_SOURCE_ROW,
                      0, 0, NC_FORMAT_BOLD);
    assert_row_format(&screen, NATIVE_SEARCH_ENGINE_SEARCH_SOURCE_ROW,
                      1, STRLIT_LEN("Search in:"), NC_FORMAT_NO_BOLD);
    assert_row_format(&screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_ROW,
                      0, 0, NC_FORMAT_BOLD);
    assert_row_format(&screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_ROW,
                      1, STRLIT_LEN("Search mode:"), NC_FORMAT_NO_BOLD);

    native_search_engine_screen_destroy(&screen);
    return;
}

static void
test_search_modes_and_sources(void) {
    NativeSearchEngineScreen screen;

    init_screen(&screen);
    for (uint32 mode = NATIVE_SEARCH_ENGINE_SEARCH_MODE_LITERAL;
         mode < NATIVE_SEARCH_ENGINE_SEARCH_MODE_LAST; mode += 1) {
        assert(native_search_engine_screen_set_search_mode(
            &screen, (enum NativeSearchEngineSearchMode)mode));
        assert(native_search_engine_screen_search_mode(&screen)
               == (enum NativeSearchEngineSearchMode)mode);

        native_search_engine_screen_set_search_source(&screen, true);
        assert(native_search_engine_screen_searches_database(&screen));
        native_search_engine_screen_prepare_static_rows(&screen);
        assert_row_text(&screen, NATIVE_SEARCH_ENGINE_SEARCH_SOURCE_ROW,
                        LIT_ARGS("Search in: Database"));
        assert_row_text(&screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_ROW,
                        search_mode_rows[mode],
                        string_len(search_mode_rows[mode]));

        native_search_engine_screen_set_search_source(&screen, false);
        assert(!native_search_engine_screen_searches_database(&screen));
        native_search_engine_screen_prepare_static_rows(&screen);
        assert_row_text(&screen, NATIVE_SEARCH_ENGINE_SEARCH_SOURCE_ROW,
                        LIT_ARGS("Search in: Current playlist"));
    }
    assert(!native_search_engine_screen_set_search_mode(
        &screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_LAST));

    native_search_engine_screen_destroy(&screen);
    return;
}

static void
test_static_row_updates(void) {
    NativeSearchEngineScreen screen;
    NcMenu *menu;
    int64 item_count;

    init_screen(&screen);
    native_search_engine_screen_prepare_static_rows(&screen);
    add_result_songs(&screen);
    assert(native_search_engine_screen_add_result_summary(&screen, 2));
    menu = native_search_engine_screen_menu(&screen);
    item_count = nc_menu_all_item_count(menu);
    nc_menu_highlight_position(menu,
                               NATIVE_SEARCH_ENGINE_STATIC_ROW_COUNT,
                               24);

    assert(native_search_engine_screen_set_constraint(
        &screen, 3, LIT_ARGS("updated title")));
    assert_row_text(&screen, 3, LIT_ARGS("Title        : updated title"));
    assert(nc_menu_all_item_count(menu) == item_count);
    assert(nc_menu_highlight(menu)
           == NATIVE_SEARCH_ENGINE_STATIC_ROW_COUNT);
    assert(native_search_engine_screen_has_result_rows(&screen));

    native_search_engine_screen_set_search_source(&screen, false);
    assert_row_text(&screen, NATIVE_SEARCH_ENGINE_SEARCH_SOURCE_ROW,
                    LIT_ARGS("Search in: Current playlist"));
    assert(nc_menu_all_item_count(menu) == item_count);

    assert(native_search_engine_screen_set_search_mode(
        &screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT));
    assert_row_text(&screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_ROW,
                    search_mode_rows[NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT],
                    string_len(search_mode_rows[
                        NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT]));
    assert(nc_menu_all_item_count(menu) == item_count);

    native_search_engine_screen_destroy(&screen);
    return;
}

static void
test_search_result_row_layout(void) {
    NativeSearchEngineScreen screen;
    NcMenu *menu;

    init_screen(&screen);
    native_search_engine_screen_prepare_static_rows(&screen);
    add_result_songs(&screen);
    assert(native_search_engine_screen_add_result_summary(&screen, 2));
    assert(native_search_engine_screen_has_result_rows(&screen));
    assert(native_search_engine_screen_result_count(&screen) == 2);
    menu = native_search_engine_screen_menu(&screen);

    assert(nc_menu_all_item_count(menu)
           == NATIVE_SEARCH_ENGINE_STATIC_ROW_COUNT + 2);
    assert(nc_menu_position_is_separator(
        menu, NATIVE_SEARCH_ENGINE_RESULT_SEPARATOR_ROW));
    assert(nc_menu_position_is_inactive(
        menu, NATIVE_SEARCH_ENGINE_RESULT_SUMMARY_ROW));
    assert(!nc_menu_position_is_selectable(
        menu, NATIVE_SEARCH_ENGINE_RESULT_SUMMARY_ROW));
    assert(nc_menu_position_is_separator(
        menu, NATIVE_SEARCH_ENGINE_RESULT_END_SEPARATOR_ROW));
    assert_row_text(&screen, NATIVE_SEARCH_ENGINE_RESULT_SUMMARY_ROW,
                    LIT_ARGS("Search results: Found 2 songs"));
    assert(row_at(&screen, NATIVE_SEARCH_ENGINE_STATIC_ROW_COUNT)->is_song);
    assert(row_at(&screen,
                  NATIVE_SEARCH_ENGINE_STATIC_ROW_COUNT + 1)->is_song);

    native_search_engine_screen_destroy(&screen);
    return;
}

static void
test_blocked_constraint_flags(void) {
    NativeSearchEngineScreen screen;
    NcMenu *menu;

    init_screen(&screen);
    native_search_engine_screen_prepare_static_rows(&screen);
    add_result_songs(&screen);
    assert(native_search_engine_screen_add_result_summary(&screen, 2));
    menu = native_search_engine_screen_menu(&screen);

    native_search_engine_screen_set_constraints_locked(&screen, true);
    for (int32 i = 0; i <= NATIVE_SEARCH_ENGINE_SEARCH_BUTTON_ROW; i += 1) {
        assert(nc_menu_position_is_inactive(menu, i));
    }
    assert(!nc_menu_position_is_inactive(
        menu, NATIVE_SEARCH_ENGINE_RESET_BUTTON_ROW));
    assert(nc_menu_position_is_separator(
        menu, NATIVE_SEARCH_ENGINE_FIRST_SEPARATOR_ROW));
    assert(nc_menu_position_is_separator(
        menu, NATIVE_SEARCH_ENGINE_SECOND_SEPARATOR_ROW));

    native_search_engine_screen_set_constraints_locked(&screen, false);
    assert(!native_search_engine_screen_constraints_locked(&screen));
    for (int32 i = 0; i <= NATIVE_SEARCH_ENGINE_SEARCH_BUTTON_ROW; i += 1) {
        assert(!nc_menu_position_is_inactive(menu, i));
    }
    assert(nc_menu_position_is_separator(
        menu, NATIVE_SEARCH_ENGINE_FIRST_SEPARATOR_ROW));
    assert(nc_menu_position_is_separator(
        menu, NATIVE_SEARCH_ENGINE_SECOND_SEPARATOR_ROW));

    native_search_engine_screen_destroy(&screen);
    return;
}

static void
test_search_reset_contract(void) {
    NativeSearchEngineScreen screen;
    NativeSearchEngineHooks hooks = {0};
    ExternalHookFixture fixture = {0};
    NcmStringView view;
    NcmError error;

    ncm_buffer_init(&fixture.status);
    init_screen(&screen);
    hooks.status_message = status_message;
    hooks.user = &fixture;
    native_search_engine_screen_set_hooks(&screen, hooks);
    assert(native_search_engine_screen_set_search_mode(
        &screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_REGEX));
    native_search_engine_screen_set_search_source(&screen, false);
    assert(native_search_engine_screen_set_constraint(
        &screen, 1, LIT_ARGS("artist")));
    assert(native_search_engine_screen_set_find_constraint(
        &screen, LIT_ARGS("previous find")));
    view = native_search_engine_screen_find_constraint(&screen);
    assert(ncm_string_equal(view.data, view.len,
                            LIT_ARGS("previous find")));
    assert(screen.match_to_pattern);
    native_search_engine_screen_prepare_static_rows(&screen);
    add_result_songs(&screen);
    assert(native_search_engine_screen_add_result_summary(&screen, 2));
    ncm_error_clear(&error);
    assert(native_search_engine_screen_apply_filter(
        &screen, LIT_ARGS("Search"), &error));
    assert(screen.filter_enabled);

    native_search_engine_screen_reset(&screen);

    assert(nc_menu_all_item_count(
        native_search_engine_screen_menu(&screen))
           == NATIVE_SEARCH_ENGINE_RESET_BUTTON_ROW + 1);
    assert(nc_menu_highlight(native_search_engine_screen_menu(&screen)) == 0);
    assert(nc_menu_beginning(native_search_engine_screen_menu(&screen)) == 0);
    assert(!screen.filter_enabled);
    assert(native_search_engine_screen_is_prepared(&screen));
    assert(!native_search_engine_screen_has_result_rows(&screen));
    assert(native_search_engine_screen_result_count(&screen) == 0);
    assert(!native_search_engine_screen_constraints_locked(&screen));
    assert(screen.filter_constraint.len == 0);
    assert(screen.search_constraint.len == 0);
    assert(!screen.match_to_pattern);
    view = native_search_engine_screen_find_constraint(&screen);
    assert(view.len == 0);
    assert(fixture.status_calls == 1);
    assert(ncm_string_equal(fixture.status.data, fixture.status.len,
                            LIT_ARGS("Search state reset")));
    assert(native_search_engine_screen_search_mode(&screen)
           == NATIVE_SEARCH_ENGINE_SEARCH_MODE_REGEX);
    assert(!native_search_engine_screen_searches_database(&screen));
    assert_row_text(&screen, NATIVE_SEARCH_ENGINE_SEARCH_SOURCE_ROW,
                    LIT_ARGS("Search in: Current playlist"));
    assert_row_text(&screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_ROW,
                    search_mode_rows[NATIVE_SEARCH_ENGINE_SEARCH_MODE_REGEX],
                    string_len(search_mode_rows[
                        NATIVE_SEARCH_ENGINE_SEARCH_MODE_REGEX]));
    for (int32 i = 0; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        assert(native_search_engine_screen_constraint(&screen, i).len == 0);
    }

    ncm_buffer_destroy(&fixture.status);
    native_search_engine_screen_destroy(&screen);
    return;
}

static void
test_search_result_row_ownership(void) {
    NativeSearchEngineScreen screen;
    NcmSong song;
    NcSearchRow *row;

    init_screen(&screen);
    ncm_song_init(&song);

    assert(ncm_song_set_uri(&song, LIT_ARGS("owned.flac")));
    assert(native_search_engine_screen_add_song_copy(&screen, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);

    row = row_at(&screen, 0);
    assert(row != NULL);
    assert(row->is_song);
    assert_song_uri(&row->song, LIT_ARGS("owned.flac"));

    ncm_song_destroy(&song);
    native_search_engine_screen_destroy(&screen);
    return;
}

static void
test_search_selected_songs(void) {
    NativeSearchEngineScreen screen;
    NcmSong song;
    NcmSongArray songs;
    NcMenu *menu;

    init_screen(&screen);
    ncm_song_init(&song);
    ncm_song_array_init(&songs);

    assert(ncm_song_set_uri(&song, LIT_ARGS("first.flac")));
    assert(native_search_engine_screen_add_song_copy(&screen, &song));
    assert(ncm_song_set_uri(&song, LIT_ARGS("second.flac")));
    assert(native_search_engine_screen_add_song_copy(&screen, &song));
    menu = native_search_engine_screen_menu(&screen);

    nc_menu_highlight_position(menu, 1, 24);
    assert(native_search_engine_screen_selected_songs(&screen, &songs));
    assert(songs.len == 1);
    assert_song_uri(&songs.items[0], LIT_ARGS("second.flac"));

    ncm_song_array_clear(&songs);
    assert(nc_menu_set_position_selected(menu, 0, true));
    assert(native_search_engine_screen_selected_songs(&screen, &songs));
    assert(songs.len == 1);
    assert_song_uri(&songs.items[0], LIT_ARGS("first.flac"));

    ncm_song_array_destroy(&songs);
    ncm_song_destroy(&song);
    native_search_engine_screen_destroy(&screen);
    return;
}

static bool
format_song_title(void *user, NcmSong *song, NcmBuffer *text) {
    NcmStringView title;

    (void)user;
    if (!ncm_song_tag_view(song, MPD_TAG_TITLE, 0, &title)) {
        return false;
    }
    ncm_buffer_append(text, title.data, title.len);
    return true;
}

static void
test_formatted_song_filter_and_find(void) {
    NativeSearchEngineScreen screen;
    NativeSearchEngineHooks hooks = {0};
    NcmSong song;
    NcBuffer buffer;
    NcMenu *menu;
    NcSearchRow *row;
    NcmError error;

    init_screen(&screen);
    nc_buffer_init(&buffer);
    nc_buffer_append_data(&buffer, LIT_ARGS("Static row"));
    assert(native_search_engine_screen_add_buffer(&screen, &buffer));
    nc_buffer_destroy(&buffer);
    hooks.format_song = format_song_title;
    native_search_engine_screen_set_hooks(&screen, hooks);
    ncm_song_init(&song);

    assert(ncm_song_set_uri(&song, LIT_ARGS("needle-in-uri.flac")));
    assert(ncm_song_add_tag(&song, MPD_TAG_TITLE, LIT_ARGS("Alpha")));
    assert(native_search_engine_screen_add_song_copy(&screen, &song));
    ncm_song_destroy(&song);
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("other.flac")));
    assert(ncm_song_add_tag(&song, MPD_TAG_TITLE,
                            LIT_ARGS("Needle Title")));
    assert(native_search_engine_screen_add_song_copy(&screen, &song));
    menu = native_search_engine_screen_menu(&screen);

    ncm_error_clear(&error);
    assert(native_search_engine_screen_apply_filter(
        &screen, LIT_ARGS("Needle"), &error));
    assert(nc_menu_filtered_item_count(menu) == 2);
    row = nc_menu_active_item_at(menu, 0);
    assert(row != NULL);
    assert(!row->is_song);
    row = nc_menu_active_item_at(menu, 1);
    assert(row != NULL);
    assert(row->is_song);
    assert_song_uri(&row->song, LIT_ARGS("other.flac"));

    native_search_engine_screen_clear_filter(&screen);
    ncm_error_clear(&error);
    assert(native_search_engine_screen_apply_filter(
        &screen, LIT_ARGS("needle-in-uri"), &error));
    assert(nc_menu_filtered_item_count(menu) == 1);
    row = nc_menu_active_item_at(menu, 0);
    assert(row != NULL);
    assert(!row->is_song);

    native_search_engine_screen_clear_filter(&screen);
    nc_menu_highlight_position(menu, 0, 24);
    ncm_error_clear(&error);
    assert(native_search_engine_screen_search(
        &screen, LIT_ARGS("Needle"), true, true, false, &error));
    assert(nc_menu_highlight(menu) == 2);

    ncm_song_destroy(&song);
    native_search_engine_screen_destroy(&screen);
    return;
}

static void
init_mpd_search_fixture(void) {
    mpd_search_fixture = (MpdSearchFixture){0};
    ncm_buffer_init(&mpd_search_fixture.any);
    ncm_buffer_init(&mpd_search_fixture.uri);
    for (int32 i = 0; i < SEARCH_TAG_CALL_CAPACITY; i += 1) {
        ncm_buffer_init(&mpd_search_fixture.tag_values[i]);
    }
    ncm_song_array_init(&mpd_search_fixture.results);
    return;
}

static void
destroy_mpd_search_fixture(void) {
    ncm_song_array_destroy(&mpd_search_fixture.results);
    for (int32 i = 0; i < SEARCH_TAG_CALL_CAPACITY; i += 1) {
        ncm_buffer_destroy(&mpd_search_fixture.tag_values[i]);
    }
    ncm_buffer_destroy(&mpd_search_fixture.uri);
    ncm_buffer_destroy(&mpd_search_fixture.any);
    return;
}

static void
append_test_song(NcmSongArray *songs, char *uri, int32 uri_len,
                 char *artist, int32 artist_len, char *title,
                 int32 title_len, char *genre, int32 genre_len) {
    NcmSong song;

    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, uri, uri_len));
    if (artist_len > 0) {
        assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST,
                                artist, artist_len));
    }
    if (title_len > 0) {
        assert(ncm_song_add_tag(&song, MPD_TAG_TITLE,
                                title, title_len));
    }
    if (genre_len > 0) {
        assert(ncm_song_add_tag(&song, MPD_TAG_GENRE,
                                genre, genre_len));
    }
    assert(ncm_song_array_append_copy(songs, &song));
    ncm_song_destroy(&song);
    return;
}

static bool
search_copy_songs(NcmSongArray *dest, NcmSongArray *source,
                  NcmError *error) {
    for (int32 i = 0; i < source->len; i += 1) {
        if (!ncm_song_array_append_copy(dest, &source->items[i])) {
            ncm_error_set(error, EIO, LIT_ARGS("copy failed"));
            return false;
        }
    }
    return true;
}

static bool
search_list_database_songs(void *user, NcmSongArray *songs,
                           NcmError *error) {
    SearchSourceFixture *fixture;

    fixture = user;
    fixture->database_calls += 1;
    if (fixture->fail_database) {
        ncm_error_set(error, EIO, LIT_ARGS("database list failed"));
        return false;
    }
    return search_copy_songs(songs, &fixture->database, error);
}

static bool
search_snapshot_playlist(void *user, NcmSongArray *songs,
                         NcmError *error) {
    SearchSourceFixture *fixture;

    fixture = user;
    fixture->playlist_calls += 1;
    if (fixture->fail_playlist) {
        ncm_error_set(error, EIO, LIT_ARGS("playlist snapshot failed"));
        return false;
    }
    return search_copy_songs(songs, &fixture->playlist, error);
}

static void
search_status_message(void *user, char *message, int32 message_len) {
    SearchSourceFixture *fixture;

    fixture = user;
    fixture->status_calls += 1;
    assert(ncm_buffer_set(&fixture->status, message, message_len));
    return;
}

bool
__wrap_ncm_mpd_client_start_search(NcmMpdClient *client,
                                   bool exact_match,
                                   NcmError *error) {
    (void)client;
    mpd_search_fixture.start_calls += 1;
    mpd_search_fixture.exact_match = exact_match;
    if (mpd_search_fixture.fail_start) {
        ncm_error_set(error, EIO, LIT_ARGS("start search failed"));
        return false;
    }
    return true;
}

bool
__wrap_ncm_mpd_client_add_search_tag(NcmMpdClient *client,
                                     enum mpd_tag_type tag,
                                     char *value, NcmError *error) {
    int32 call;

    (void)client;
    if (mpd_search_fixture.fail_add) {
        ncm_error_set(error, EIO, LIT_ARGS("add search failed"));
        return false;
    }
    call = mpd_search_fixture.tag_calls;
    assert(call < SEARCH_TAG_CALL_CAPACITY);
    mpd_search_fixture.tags[call] = tag;
    assert(ncm_buffer_set(&mpd_search_fixture.tag_values[call],
                          value, string_len(value)));
    mpd_search_fixture.tag_calls += 1;
    return true;
}

bool
__wrap_ncm_mpd_client_add_search_any(NcmMpdClient *client,
                                     char *value, NcmError *error) {
    (void)client;
    if (mpd_search_fixture.fail_add) {
        ncm_error_set(error, EIO, LIT_ARGS("add search failed"));
        return false;
    }
    mpd_search_fixture.any_calls += 1;
    assert(ncm_buffer_set(&mpd_search_fixture.any,
                          value, string_len(value)));
    return true;
}

bool
__wrap_ncm_mpd_client_add_search_uri(NcmMpdClient *client,
                                     char *value, NcmError *error) {
    (void)client;
    if (mpd_search_fixture.fail_add) {
        ncm_error_set(error, EIO, LIT_ARGS("add search failed"));
        return false;
    }
    mpd_search_fixture.uri_calls += 1;
    assert(ncm_buffer_set(&mpd_search_fixture.uri,
                          value, string_len(value)));
    return true;
}

bool
__wrap_ncm_mpd_client_commit_search_songs(
    NcmMpdClient *client, NcmMpdSongList *songs, NcmError *error) {
    (void)client;
    mpd_search_fixture.commit_calls += 1;
    if (mpd_search_fixture.fail_commit) {
        ncm_error_set(error, EIO, LIT_ARGS("commit search failed"));
        return false;
    }
    for (int32 i = 0; i < mpd_search_fixture.results.len; i += 1) {
        assert(ncm_mpd_song_list_append_copy(
            songs, &mpd_search_fixture.results.items[i]));
    }
    return true;
}

static void
assert_search_result(NativeSearchEngineScreen *screen, int32 result,
                     char *uri, int32 uri_len) {
    NcSearchRow *row;

    row = row_at(screen, NATIVE_SEARCH_ENGINE_STATIC_ROW_COUNT + result);
    assert(row != NULL);
    assert(row->is_song);
    assert_song_uri(&row->song, uri, uri_len);
    return;
}

static void
test_database_search_execution(void) {
    NativeSearchEngineScreen screen;
    NativeSearchEngineHooks hooks = {0};
    SearchSourceFixture source = {0};
    NcmMpdClient client = {0};
    NcmError error;
    bool old_block;

    old_block = Config.block_search_constraints_change;
    Config.block_search_constraints_change = true;
    ncm_buffer_init(&source.status);
    ncm_song_array_init(&source.database);
    ncm_song_array_init(&source.playlist);
    init_mpd_search_fixture();
    init_screen(&screen);
    hooks.status_message = search_status_message;
    hooks.user = &source;
    native_search_engine_screen_set_hooks(&screen, hooks);

    assert(native_search_engine_screen_set_constraint(
        &screen, 0, LIT_ARGS("any")));
    assert(native_search_engine_screen_set_constraint(
        &screen, 1, LIT_ARGS("artist")));
    assert(native_search_engine_screen_set_constraint(
        &screen, 2, LIT_ARGS("album artist")));
    assert(native_search_engine_screen_set_constraint(
        &screen, 3, LIT_ARGS("title")));
    assert(native_search_engine_screen_set_constraint(
        &screen, 4, LIT_ARGS("album")));
    assert(native_search_engine_screen_set_constraint(
        &screen, 5, LIT_ARGS("file.flac")));
    assert(native_search_engine_screen_set_constraint(
        &screen, 6, LIT_ARGS("composer")));
    assert(native_search_engine_screen_set_constraint(
        &screen, 7, LIT_ARGS("performer")));
    assert(native_search_engine_screen_set_constraint(
        &screen, 8, LIT_ARGS("genre")));
    assert(native_search_engine_screen_set_constraint(
        &screen, 9, LIT_ARGS("date")));
    assert(native_search_engine_screen_set_constraint(
        &screen, 10, LIT_ARGS("comment")));
    append_test_song(&mpd_search_fixture.results,
                     LIT_ARGS("first.flac"), NULL, 0,
                     NULL, 0, NULL, 0);
    append_test_song(&mpd_search_fixture.results,
                     LIT_ARGS("second.flac"), NULL, 0,
                     NULL, 0, NULL, 0);

    ncm_error_clear(&error);
    assert(native_search_engine_screen_execute_search(
        &screen, &client, &error));
    assert(mpd_search_fixture.start_calls == 1);
    assert(!mpd_search_fixture.exact_match);
    assert(mpd_search_fixture.any_calls == 1);
    assert(ncm_string_equal(mpd_search_fixture.any.data,
                            mpd_search_fixture.any.len,
                            LIT_ARGS("any")));
    assert(mpd_search_fixture.uri_calls == 1);
    assert(ncm_string_equal(mpd_search_fixture.uri.data,
                            mpd_search_fixture.uri.len,
                            LIT_ARGS("file.flac")));
    assert(mpd_search_fixture.tag_calls == SEARCH_TAG_CALL_CAPACITY);
    assert(mpd_search_fixture.tags[0] == MPD_TAG_ARTIST);
    assert(mpd_search_fixture.tags[1] == MPD_TAG_ALBUM_ARTIST);
    assert(mpd_search_fixture.tags[2] == MPD_TAG_TITLE);
    assert(mpd_search_fixture.tags[3] == MPD_TAG_ALBUM);
    assert(mpd_search_fixture.tags[4] == MPD_TAG_COMPOSER);
    assert(mpd_search_fixture.tags[5] == MPD_TAG_PERFORMER);
    assert(mpd_search_fixture.tags[6] == MPD_TAG_GENRE);
    assert(mpd_search_fixture.tags[7] == MPD_TAG_DATE);
    assert(mpd_search_fixture.tags[8] == MPD_TAG_COMMENT);
    assert(ncm_string_equal(mpd_search_fixture.tag_values[0].data,
                            mpd_search_fixture.tag_values[0].len,
                            LIT_ARGS("artist")));
    assert(ncm_string_equal(mpd_search_fixture.tag_values[1].data,
                            mpd_search_fixture.tag_values[1].len,
                            LIT_ARGS("album artist")));
    assert(ncm_string_equal(mpd_search_fixture.tag_values[2].data,
                            mpd_search_fixture.tag_values[2].len,
                            LIT_ARGS("title")));
    assert(ncm_string_equal(mpd_search_fixture.tag_values[3].data,
                            mpd_search_fixture.tag_values[3].len,
                            LIT_ARGS("album")));
    assert(ncm_string_equal(mpd_search_fixture.tag_values[4].data,
                            mpd_search_fixture.tag_values[4].len,
                            LIT_ARGS("composer")));
    assert(ncm_string_equal(mpd_search_fixture.tag_values[5].data,
                            mpd_search_fixture.tag_values[5].len,
                            LIT_ARGS("performer")));
    assert(ncm_string_equal(mpd_search_fixture.tag_values[6].data,
                            mpd_search_fixture.tag_values[6].len,
                            LIT_ARGS("genre")));
    assert(ncm_string_equal(mpd_search_fixture.tag_values[7].data,
                            mpd_search_fixture.tag_values[7].len,
                            LIT_ARGS("date")));
    assert(ncm_string_equal(mpd_search_fixture.tag_values[8].data,
                            mpd_search_fixture.tag_values[8].len,
                            LIT_ARGS("comment")));
    assert(mpd_search_fixture.commit_calls == 1);
    assert(native_search_engine_screen_result_count(&screen) == 2);
    assert(native_search_engine_screen_has_result_rows(&screen));
    assert(native_search_engine_screen_constraints_locked(&screen));
    assert_search_result(&screen, 0, LIT_ARGS("first.flac"));
    assert_search_result(&screen, 1, LIT_ARGS("second.flac"));
    assert_row_text(&screen, NATIVE_SEARCH_ENGINE_RESULT_SUMMARY_ROW,
                    LIT_ARGS("Search results: Found 2 songs"));
    assert(ncm_string_equal(source.status.data, source.status.len,
                            LIT_ARGS("Searching finished")));

    native_search_engine_screen_destroy(&screen);
    destroy_mpd_search_fixture();
    ncm_song_array_destroy(&source.playlist);
    ncm_song_array_destroy(&source.database);
    ncm_buffer_destroy(&source.status);

    source = (SearchSourceFixture){0};
    ncm_buffer_init(&source.status);
    ncm_song_array_init(&source.database);
    ncm_song_array_init(&source.playlist);
    init_mpd_search_fixture();
    init_screen(&screen);
    hooks.status_message = search_status_message;
    hooks.user = &source;
    native_search_engine_screen_set_hooks(&screen, hooks);
    assert(native_search_engine_screen_set_search_mode(
        &screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT));
    assert(native_search_engine_screen_set_constraint(
        &screen, 3, LIT_ARGS("Exact title")));
    append_test_song(&mpd_search_fixture.results,
                     LIT_ARGS("exact.flac"), NULL, 0,
                     NULL, 0, NULL, 0);

    ncm_error_clear(&error);
    assert(native_search_engine_screen_execute_search(
        &screen, &client, &error));
    assert(mpd_search_fixture.start_calls == 1);
    assert(mpd_search_fixture.exact_match);
    assert(mpd_search_fixture.tag_calls == 1);
    assert(mpd_search_fixture.tags[0] == MPD_TAG_TITLE);
    assert(ncm_string_equal(mpd_search_fixture.tag_values[0].data,
                            mpd_search_fixture.tag_values[0].len,
                            LIT_ARGS("Exact title")));
    assert_search_result(&screen, 0, LIT_ARGS("exact.flac"));

    native_search_engine_screen_destroy(&screen);
    destroy_mpd_search_fixture();
    ncm_song_array_destroy(&source.playlist);
    ncm_song_array_destroy(&source.database);
    ncm_buffer_destroy(&source.status);
    Config.block_search_constraints_change = old_block;
    return;
}

static void
test_local_search_execution(void) {
    NativeSearchEngineScreen screen;
    NativeSearchEngineHooks hooks = {0};
    SearchSourceFixture source = {0};
    NcmError error;
    uint32 old_regex_type;
    bool old_ignore_the;

    old_regex_type = Config.regex_type;
    old_ignore_the = Config.ignore_leading_the;
    Config.regex_type = NCM_REGEX_EXTENDED_CASE_INSENSITIVE;
    Config.ignore_leading_the = false;
    ncm_buffer_init(&source.status);
    ncm_song_array_init(&source.database);
    ncm_song_array_init(&source.playlist);
    append_test_song(&source.database, LIT_ARGS("first.flac"),
                     LIT_ARGS("Alpha"), LIT_ARGS("Needle title"),
                     LIT_ARGS("Rock"));
    append_test_song(&source.database, LIT_ARGS("second.flac"),
                     LIT_ARGS("Beta"), LIT_ARGS("Needle title"),
                     LIT_ARGS("Rock"));
    append_test_song(&source.database, LIT_ARGS("third.flac"),
                     LIT_ARGS("Alpha"), LIT_ARGS("Other title"),
                     LIT_ARGS("Rock"));
    init_screen(&screen);
    hooks.list_database_songs = search_list_database_songs;
    hooks.snapshot_playlist = search_snapshot_playlist;
    hooks.status_message = search_status_message;
    hooks.user = &source;
    native_search_engine_screen_set_hooks(&screen, hooks);
    assert(native_search_engine_screen_set_search_mode(
        &screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_REGEX));
    assert(native_search_engine_screen_set_constraint(
        &screen, 0, LIT_ARGS("needle")));
    assert(native_search_engine_screen_set_constraint(
        &screen, 1, LIT_ARGS("^Alpha$")));

    ncm_error_clear(&error);
    assert(native_search_engine_screen_execute_search(
        &screen, NULL, &error));
    assert(source.database_calls == 1);
    assert(source.playlist_calls == 0);
    assert(native_search_engine_screen_result_count(&screen) == 1);
    assert_search_result(&screen, 0, LIT_ARGS("first.flac"));
    native_search_engine_screen_destroy(&screen);

    ncm_song_array_clear(&source.playlist);
    source.playlist_calls = 0;
    append_test_song(&source.playlist, LIT_ARGS("literal.flac"),
                     LIT_ARGS("Alpha"), LIT_ARGS("Literal Needle"),
                     LIT_ARGS("Rock"));
    append_test_song(&source.playlist, LIT_ARGS("other.flac"),
                     LIT_ARGS("Beta"), LIT_ARGS("Other"),
                     LIT_ARGS("Jazz"));
    Config.regex_type = NCM_REGEX_LITERAL_CASE_INSENSITIVE;
    init_screen(&screen);
    native_search_engine_screen_set_hooks(&screen, hooks);
    native_search_engine_screen_set_search_source(&screen, false);
    assert(native_search_engine_screen_set_search_mode(
        &screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_LITERAL));
    assert(native_search_engine_screen_set_constraint(
        &screen, 3, LIT_ARGS("needle")));

    ncm_error_clear(&error);
    assert(native_search_engine_screen_execute_search(
        &screen, NULL, &error));
    assert(source.playlist_calls == 1);
    assert(native_search_engine_screen_result_count(&screen) == 1);
    assert_search_result(&screen, 0, LIT_ARGS("literal.flac"));
    native_search_engine_screen_destroy(&screen);

    ncm_song_array_clear(&source.playlist);
    source.playlist_calls = 0;
    append_test_song(&source.playlist, LIT_ARGS("dir/song.flac"),
                     LIT_ARGS("The Alpha"), LIT_ARGS("Exact"),
                     LIT_ARGS("Rock"));
    append_test_song(&source.playlist, LIT_ARGS("dir/other.flac"),
                     LIT_ARGS("Beta"), LIT_ARGS("Exact"),
                     LIT_ARGS("Rock"));
    Config.ignore_leading_the = true;
    init_screen(&screen);
    native_search_engine_screen_set_hooks(&screen, hooks);
    native_search_engine_screen_set_search_source(&screen, false);
    assert(native_search_engine_screen_set_search_mode(
        &screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT));
    assert(native_search_engine_screen_set_constraint(
        &screen, 0, LIT_ARGS("Alpha")));
    assert(native_search_engine_screen_set_constraint(
        &screen, 5, LIT_ARGS("song.flac")));

    ncm_error_clear(&error);
    assert(native_search_engine_screen_execute_search(
        &screen, NULL, &error));
    assert(source.playlist_calls == 1);
    assert(native_search_engine_screen_result_count(&screen) == 1);
    assert_search_result(&screen, 0, LIT_ARGS("dir/song.flac"));
    native_search_engine_screen_destroy(&screen);

    ncm_song_array_clear(&source.playlist);
    source.playlist_calls = 0;
    Config.regex_type = NCM_REGEX_EXTENDED_CASE_INSENSITIVE;
    append_test_song(&source.playlist, LIT_ARGS("one.flac"),
                     LIT_ARGS("Alpha"), LIT_ARGS("One"),
                     LIT_ARGS("Rock"));
    append_test_song(&source.playlist, LIT_ARGS("two.flac"),
                     LIT_ARGS("Beta"), LIT_ARGS("Two"),
                     LIT_ARGS("Jazz"));
    init_screen(&screen);
    native_search_engine_screen_set_hooks(&screen, hooks);
    native_search_engine_screen_set_search_source(&screen, false);
    assert(native_search_engine_screen_set_search_mode(
        &screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_REGEX));
    assert(native_search_engine_screen_set_constraint(
        &screen, 3, LIT_ARGS("[")));

    ncm_error_clear(&error);
    assert(native_search_engine_screen_execute_search(
        &screen, NULL, &error));
    assert(native_search_engine_screen_result_count(&screen) == 2);
    assert_search_result(&screen, 0, LIT_ARGS("one.flac"));
    assert_search_result(&screen, 1, LIT_ARGS("two.flac"));
    native_search_engine_screen_destroy(&screen);

    ncm_song_array_destroy(&source.playlist);
    ncm_song_array_destroy(&source.database);
    ncm_buffer_destroy(&source.status);
    Config.ignore_leading_the = old_ignore_the;
    Config.regex_type = old_regex_type;
    return;
}

static void
test_search_execution_failures(void) {
    NativeSearchEngineScreen screen;
    NativeSearchEngineHooks hooks = {0};
    SearchSourceFixture source = {0};
    NcmMpdClient client = {0};
    NcmError error;

    ncm_buffer_init(&source.status);
    ncm_song_array_init(&source.database);
    ncm_song_array_init(&source.playlist);
    init_mpd_search_fixture();
    init_screen(&screen);
    hooks.status_message = search_status_message;
    hooks.user = &source;
    native_search_engine_screen_set_hooks(&screen, hooks);

    ncm_error_clear(&error);
    assert(native_search_engine_screen_execute_search(
        &screen, &client, &error));
    assert(mpd_search_fixture.start_calls == 0);
    assert(native_search_engine_screen_result_count(&screen) == 0);
    assert(ncm_string_equal(source.status.data, source.status.len,
                            LIT_ARGS("No results found")));

    assert(native_search_engine_screen_set_constraint(
        &screen, 3, LIT_ARGS("needle")));
    ncm_error_clear(&error);
    assert(native_search_engine_screen_execute_search(
        &screen, &client, &error));
    assert(mpd_search_fixture.commit_calls == 1);
    assert(native_search_engine_screen_result_count(&screen) == 0);
    assert(ncm_string_equal(source.status.data, source.status.len,
                            LIT_ARGS("No results found")));

    mpd_search_fixture.fail_commit = true;
    ncm_error_clear(&error);
    assert(!native_search_engine_screen_execute_search(
        &screen, &client, &error));
    assert(error.code == EIO);
    assert(nc_menu_item_count(
        native_search_engine_screen_menu(&screen))
           == NATIVE_SEARCH_ENGINE_RESET_BUTTON_ROW + 1);
    assert(!native_search_engine_screen_has_result_rows(&screen));
    assert(ncm_string_equal(source.status.data, source.status.len,
                            LIT_ARGS("commit search failed")));

    native_search_engine_screen_destroy(&screen);
    destroy_mpd_search_fixture();
    ncm_song_array_destroy(&source.playlist);
    ncm_song_array_destroy(&source.database);
    ncm_buffer_destroy(&source.status);
    return;
}

static void
highlight_row(NativeSearchEngineScreen *screen, int32 row) {
    nc_menu_highlight_position(
        native_search_engine_screen_menu(screen), row,
        nc_window_height(native_search_engine_screen_window(screen)));
    return;
}

static void
test_native_row_activation(void) {
    NativeSearchEngineScreen screen;
    NativeSearchEngineHooks hooks = {0};
    ExternalHookFixture fixture = {0};
    NcmStringView constraint;
    NcmError error;
    enum DisplayMode old_display_mode;
    uint32 old_regex_type;
    bool old_search_in_db;

    ncm_buffer_init(&fixture.prompt_label);
    ncm_buffer_init(&fixture.prompt_initial);
    ncm_buffer_init(&fixture.status);
    fixture.prompt_result = NATIVE_SEARCH_ENGINE_PROMPT_ACCEPTED;
    old_search_in_db = Config.search_in_db;
    old_display_mode = Config.search_engine_display_mode;
    old_regex_type = Config.regex_type;
    Config.regex_type = NCM_REGEX_LITERAL_CASE_INSENSITIVE;

    init_screen(&screen);
    hooks.client = NULL;
    hooks.snapshot_playlist = snapshot_playlist;
    hooks.prompt_constraint = prompt_constraint;
    hooks.status_message = status_message;
    hooks.add_song = add_song;
    hooks.user = &fixture;
    native_search_engine_screen_set_hooks(&screen, hooks);
    native_search_engine_screen_prepare_static_rows(&screen);

    highlight_row(&screen, 3);
    assert(native_search_engine_screen_can_run_current(&screen));
    assert(nc_screen_can_run_current(&screen.screen));
    assert(native_search_engine_screen_run_current(&screen));
    constraint = native_search_engine_screen_constraint(&screen, 3);
    assert(ncm_string_equal(constraint.data, constraint.len,
                            LIT_ARGS("new title")));
    assert(fixture.prompt_calls == 1);

    fixture.prompt_result = NATIVE_SEARCH_ENGINE_PROMPT_ABORTED;
    highlight_row(&screen, 3);
    assert(!nc_screen_run_current(&screen.screen));
    constraint = native_search_engine_screen_constraint(&screen, 3);
    assert(ncm_string_equal(constraint.data, constraint.len,
                            LIT_ARGS("new title")));
    assert(ncm_string_equal(fixture.status.data, fixture.status.len,
                            LIT_ARGS("Action aborted")));

    fixture.prompt_result = NATIVE_SEARCH_ENGINE_PROMPT_ERROR;
    highlight_row(&screen, 3);
    assert(!nc_screen_run_current(&screen.screen));
    constraint = native_search_engine_screen_constraint(&screen, 3);
    assert(ncm_string_equal(constraint.data, constraint.len,
                            LIT_ARGS("new title")));
    assert(ncm_string_equal(
        fixture.status.data, fixture.status.len,
        LIT_ARGS("Unable to read search constraint")));
    assert(fixture.prompt_calls == 3);

    highlight_row(&screen, NATIVE_SEARCH_ENGINE_SEARCH_SOURCE_ROW);
    native_search_engine_screen_set_search_source(&screen, true);
    assert(nc_screen_run_current(&screen.screen));
    assert(!native_search_engine_screen_searches_database(&screen));
    assert(!Config.search_in_db);

    highlight_row(&screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_ROW);
    assert(nc_screen_run_current(&screen.screen));
    assert(native_search_engine_screen_search_mode(&screen)
           == NATIVE_SEARCH_ENGINE_SEARCH_MODE_REGEX);
    assert(nc_screen_run_current(&screen.screen));
    assert(native_search_engine_screen_search_mode(&screen)
           == NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT);
    assert(nc_screen_run_current(&screen.screen));
    assert(native_search_engine_screen_search_mode(&screen)
           == NATIVE_SEARCH_ENGINE_SEARCH_MODE_LITERAL);

    assert(native_search_engine_screen_set_constraint(
        &screen, 3, NULL, 0));
    assert(native_search_engine_screen_set_constraint(
        &screen, 0, LIT_ARGS("playlist")));
    highlight_row(&screen, NATIVE_SEARCH_ENGINE_SEARCH_BUTTON_ROW);
    assert(nc_screen_run_current(&screen.screen));
    assert(fixture.playlist_calls == 1);
    assert(native_search_engine_screen_result_count(&screen) == 1);

    native_search_engine_screen_set_constraints_locked(&screen, true);
    highlight_row(&screen, NATIVE_SEARCH_ENGINE_SEARCH_BUTTON_ROW);
    assert(!native_search_engine_screen_can_run_current(&screen));
    ncm_error_clear(&error);
    assert(!native_search_engine_screen_start_searching(
        &screen, NULL, &error));

    highlight_row(&screen, NATIVE_SEARCH_ENGINE_RESET_BUTTON_ROW);
    assert(nc_screen_run_current(&screen.screen));
    assert(native_search_engine_screen_result_count(&screen) == 0);
    constraint = native_search_engine_screen_constraint(&screen, 0);
    assert(constraint.len == 0);

    ncm_error_clear(&error);
    assert(native_search_engine_screen_start_searching(
        &screen, NULL, &error));
    assert(ncm_string_equal(fixture.status.data, fixture.status.len,
                            LIT_ARGS("No results found")));
    assert(fixture.playlist_calls == 1);

    add_result_songs(&screen);
    assert(native_search_engine_screen_add_result_summary(&screen, 2));
    highlight_row(&screen, NATIVE_SEARCH_ENGINE_STATIC_ROW_COUNT);
    assert(!native_search_engine_screen_can_run_current(&screen));
    assert(!native_search_engine_screen_run_current(&screen));

    Config.search_engine_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    assert(native_search_engine_screen_toggle_display_mode(&screen)
           == NCM_DISPLAY_MODE_COLUMNS);
    assert(Config.search_engine_display_mode == NCM_DISPLAY_MODE_COLUMNS);
    assert(native_search_engine_screen_toggle_display_mode(&screen)
           == NCM_DISPLAY_MODE_CLASSIC);
    assert(Config.search_engine_display_mode == NCM_DISPLAY_MODE_CLASSIC);

    native_search_engine_screen_destroy(&screen);
    Config.search_engine_display_mode = old_display_mode;
    Config.regex_type = old_regex_type;
    Config.search_in_db = old_search_in_db;
    ncm_buffer_destroy(&fixture.status);
    ncm_buffer_destroy(&fixture.prompt_initial);
    ncm_buffer_destroy(&fixture.prompt_label);
    return;
}


void
__wrap_nc_window_init(NcWindow *window, int64 start_x, int64 start_y,
                      int64 width, int64 height, char *title,
                      int32 title_len, NcColor color, NcBorder border) {
    (void)color;
    *window = (NcWindow){0};
    window->start_x = start_x;
    window->start_y = start_y;
    window->width = width;
    window->height = height;
    window->border = border;
    __wrap_nc_window_set_title(window, title, title_len);
    return;
}

void
__wrap_nc_window_destroy(NcWindow *window) {
    *window = (NcWindow){0};
    return;
}

void
__wrap_nc_window_set_title(NcWindow *window, char *title,
                           int32 title_len) {
    bool had_title;
    bool has_title;

    had_title = window->title_len > 0;
    has_title = title_len > 0;
    if (has_title && !had_title) {
        window->start_y += 2;
        window->height -= 2;
    } else if (!has_title && had_title) {
        window->start_y -= 2;
        window->height += 2;
    }

    window_trace.title[0] = '\0';
    if ((title != NULL) && (title_len > 0)) {
        assert(title_len < (int32)sizeof(window_trace.title));
        for (int32 i = 0; i < title_len; i += 1) {
            window_trace.title[i] = title[i];
        }
        window_trace.title[title_len] = '\0';
        window->title = window_trace.title;
    } else {
        window->title = NULL;
    }
    window->title_len = title_len;
    return;
}

void
__wrap_nc_window_move_to(NcWindow *window, int64 new_x, int64 new_y) {
    window->start_x = new_x;
    window->start_y = new_y;
    return;
}

void
__wrap_nc_window_resize(NcWindow *window, int64 width, int64 height) {
    window->width = width;
    window->height = height;
    return;
}

int64
__wrap_nc_window_width(NcWindow *window) {
    return window->width;
}

int64
__wrap_nc_window_height(NcWindow *window) {
    int64 height;

    height = window->height;
    if (window->title_len > 0) {
        height += 2;
    }
    return height;
}

void
__wrap_nc_window_display(NcWindow *window) {
    (void)window;
    window_trace.display_calls += 1;
    return;
}

bool
__wrap_nc_window_has_coords(NcWindow *window, int32 *x, int32 *y) {
    *x -= (int32)window->start_x;
    *y -= (int32)window->start_y;
    return (*x >= 0) && (*x < window->width)
        && (*y >= 0) && (*y < window->height);
}

void
__wrap_nc_window_print_char(NcWindow *window, char ch) {
    (void)window;
    assert(window_trace.printed_len
           < (int32)sizeof(window_trace.printed)-1);
    window_trace.printed[window_trace.printed_len] = ch;
    window_trace.printed_len += 1;
    window_trace.printed[window_trace.printed_len] = '\0';
    return;
}

void
__wrap_nc_buffer_apply_property(NcWindow *window,
                                NcBufferProperty *property) {
    (void)window;
    (void)property;
    window_trace.property_calls += 1;
    return;
}

void
__wrap_nc_menu_refresh(NcMenu *menu, NcWindow *window, int64 width,
                       int64 height) {
    (void)menu;
    (void)window;
    window_trace.menu_refresh_calls += 1;
    window_trace.refresh_width = width;
    window_trace.refresh_height = height;
    return;
}

void
__wrap_nc_screen_get_resize_params(NcScreen *screen, int64 *x_offset,
                                   int64 *width) {
    (void)screen;
    *x_offset = resize_x;
    *width = resize_width;
    return;
}

int64
__wrap_ui_state_main_start_y(void) {
    return resize_main_y;
}

int64
__wrap_ui_state_main_height(void) {
    return resize_main_height;
}

static void
reset_window_trace(void) {
    window_trace = (WindowTrace){0};
    return;
}

static void
assert_printed(char *expected, int32 expected_len) {
    assert(window_trace.printed_len == expected_len);
    assert(ncm_string_equal(window_trace.printed,
                            window_trace.printed_len,
                            expected, expected_len));
    return;
}

static void
test_native_display_and_column_title(void) {
    NativeSearchEngineScreen screen;
    NcmFormatAst old_classic;
    NcmFormatAst old_columns;
    NcmError error;
    ColumnArray old_column_array;
    Column columns[2];
    NcSearchRow *row;
    NcMenu *menu;
    NcmSong song;
    NcmBuffer formatted;
    bool old_titles_visibility;
    enum DisplayMode old_mode;

    reset_window_trace();
    old_classic = Config.song_list_format;
    old_columns = Config.song_columns_mode_format;
    old_column_array = Config.columns;
    old_titles_visibility = Config.titles_visibility;
    old_mode = Config.search_engine_display_mode;
    ncm_format_ast_init(&Config.song_list_format);
    ncm_format_ast_init(&Config.song_columns_mode_format);
    ncm_error_clear(&error);
    assert(ncm_format_parse(&Config.song_list_format,
                            LIT_ARGS("classic:{%a - }%t"),
                            NCM_FORMAT_FLAG_ALL, &error));
    assert(ncm_format_parse(&Config.song_columns_mode_format,
                            LIT_ARGS("%a"),
                            NCM_FORMAT_FLAG_ALL, &error));

    columns[0] = (Column){0};
    columns[0].name = (char *)"Artist";
    columns[0].name_len = STRLIT_LEN("Artist");
    columns[0].type = (char *)"a";
    columns[0].type_len = STRLIT_LEN("a");
    columns[0].width = 10;
    columns[0].stretch_limit = -1;
    columns[0].color = nc_color_default();
    columns[0].fixed = true;
    columns[1] = (Column){0};
    columns[1].type = (char *)"t";
    columns[1].type_len = STRLIT_LEN("t");
    columns[1].width = 10;
    columns[1].stretch_limit = -1;
    columns[1].color = nc_color_default();
    columns[1].fixed = true;
    Config.columns.items = columns;
    Config.columns.len = 2;
    Config.columns.cap = 2;
    Config.titles_visibility = false;

    init_screen(&screen);
    native_search_engine_screen_set_geometry(&screen, 0, 20, 0, 24);
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("song.flac")));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST, LIT_ARGS("Band")));
    assert(ncm_song_add_tag(&song, MPD_TAG_TITLE, LIT_ARGS("Track")));
    assert(native_search_engine_screen_add_song_copy(&screen, &song));
    ncm_song_destroy(&song);
    row = row_at(&screen, 0);
    menu = native_search_engine_screen_menu(&screen);

    reset_window_trace();
    native_search_engine_screen_set_display_mode(
        &screen, NCM_DISPLAY_MODE_CLASSIC);
    menu->display_callbacks.draw(menu, &screen.window, row, 0,
                                 menu->display_callbacks.user);
    assert_printed(LIT_ARGS("classic:Band - Track"));

    reset_window_trace();
    native_search_engine_screen_set_display_mode(
        &screen, NCM_DISPLAY_MODE_COLUMNS);
    menu->display_callbacks.draw(menu, &screen.window, row, 0,
                                 menu->display_callbacks.user);
    assert_printed(LIT_ARGS("Band      Track     "));
    assert(screen.column_title.len == 0);
    assert(screen.window.start_y == 0);
    assert(screen.window.height == 24);

    ncm_buffer_init(&formatted);
    assert(native_search_engine_screen_format_song_text(
        &screen, &row->song, &formatted));
    assert(ncm_string_equal(formatted.data, formatted.len,
                            LIT_ARGS("Band      Track     ")));
    ncm_buffer_destroy(&formatted);

    Config.titles_visibility = true;
    native_search_engine_screen_set_geometry(&screen, 0, 20, 0, 24);
    native_search_engine_screen_set_display_mode(
        &screen, NCM_DISPLAY_MODE_COLUMNS);
    assert(ncm_string_equal(screen.column_title.data,
                            screen.column_title.len,
                            LIT_ARGS("Artist    Title     ")));
    assert(screen.window.start_y == 2);
    assert(screen.window.height == 22);

    native_search_engine_screen_set_display_mode(
        &screen, NCM_DISPLAY_MODE_CLASSIC);
    assert(screen.column_title.len == 0);
    assert(screen.window.start_y == 0);
    assert(screen.window.height == 24);

    native_search_engine_screen_destroy(&screen);
    ncm_format_ast_destroy(&Config.song_list_format);
    ncm_format_ast_destroy(&Config.song_columns_mode_format);
    Config.song_list_format = old_classic;
    Config.song_columns_mode_format = old_columns;
    Config.columns = old_column_array;
    Config.titles_visibility = old_titles_visibility;
    Config.search_engine_display_mode = old_mode;
    return;
}

static void
test_native_lifecycle(void) {
    NativeSearchEngineScreen screen;
    NcScreen *base;
    NcMenu *menu;

    reset_window_trace();
    resize_x = 4;
    resize_width = 70;
    resize_main_y = 3;
    resize_main_height = 18;
    init_screen(&screen);
    base = native_search_engine_screen_base(&screen);
    menu = native_search_engine_screen_menu(&screen);

    assert(nc_screen_active_window(base) == &screen.window);
    assert(nc_screen_is_lockable(base));
    assert(nc_screen_is_mergable(base));
    assert(!native_search_engine_screen_is_prepared(&screen));
    nc_screen_switch_to(base);
    assert(native_search_engine_screen_is_prepared(&screen));

    reset_window_trace();
    nc_screen_refresh(base);
    assert(window_trace.display_calls == 1);
    assert(window_trace.menu_refresh_calls == 1);
    assert(window_trace.refresh_width == 80);
    assert(window_trace.refresh_height == 24);

    assert(nc_menu_highlight(menu) == 0);
    nc_screen_scroll(base, NC_SCROLL_DOWN);
    assert(nc_menu_highlight(menu) == 1);

    nc_screen_set_has_to_be_resized(base, true);
    nc_screen_resize(base);
    assert(!nc_screen_has_to_be_resized(base));
    assert(screen.start_x == resize_x);
    assert(screen.width == resize_width);
    assert(screen.main_start_y == resize_main_y);
    assert(screen.main_height == resize_main_height);

    nc_screen_request_update(base);
    assert(nc_screen_has_to_be_updated(base));
    nc_screen_update(base);
    assert(!nc_screen_has_to_be_updated(base));
    assert(ncm_string_equal(nc_screen_title(base),
                            STRLIT_LEN("Search engine"),
                            LIT_ARGS("Search engine")));

    native_search_engine_screen_destroy(&screen);
    return;
}

static void
test_native_mouse_behavior(void) {
    NativeSearchEngineScreen screen;
    NativeSearchEngineHooks hooks = {0};
    ExternalHookFixture external = {0};
    NcMenu *menu;
    MEVENT event;

    reset_window_trace();
    init_screen(&screen);
    native_search_engine_screen_prepare_static_rows(&screen);
    add_result_songs(&screen);
    assert(native_search_engine_screen_add_result_summary(&screen, 2));
    ncm_buffer_init(&external.prompt_label);
    ncm_buffer_init(&external.prompt_initial);
    ncm_buffer_init(&external.status);
    external.prompt_result = NATIVE_SEARCH_ENGINE_PROMPT_ACCEPTED;
    hooks.prompt_constraint = prompt_constraint;
    hooks.status_message = status_message;
    hooks.add_song = mouse_add_song;
    hooks.user = &external;
    native_search_engine_screen_set_hooks(&screen, hooks);
    menu = native_search_engine_screen_menu(&screen);

    event = (MEVENT){0};
    event.bstate = BUTTON3_PRESSED;
    nc_screen_mouse_button_pressed(&screen.screen, event);
    assert(external.prompt_calls == 1);

    event = (MEVENT){0};
    event.y = NATIVE_SEARCH_ENGINE_STATIC_ROW_COUNT;
    event.bstate = BUTTON1_PRESSED;
    nc_screen_mouse_button_pressed(&screen.screen, event);
    assert(external.add_calls == 1);
    assert(!external.play);

    event.bstate = BUTTON3_PRESSED;
    nc_screen_mouse_button_pressed(&screen.screen, event);
    assert(external.add_calls == 2);
    assert(external.play);

    nc_menu_highlight_position(menu, 0, 24);
    native_search_engine_screen_set_mouse_config(&screen, 2, false);
    event = (MEVENT){0};
    event.bstate = BUTTON5_PRESSED;
    nc_screen_mouse_button_pressed(&screen.screen, event);
    assert(nc_menu_highlight(menu) == 2);

    nc_menu_highlight_position(menu, 0, 24);
    native_search_engine_screen_set_mouse_config(&screen, 1, true);
    nc_screen_mouse_button_pressed(&screen.screen, event);
    assert(nc_menu_highlight(menu) > 2);

    native_search_engine_screen_destroy(&screen);
    ncm_buffer_destroy(&external.status);
    ncm_buffer_destroy(&external.prompt_initial);
    ncm_buffer_destroy(&external.prompt_label);
    return;
}
