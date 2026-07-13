#include <assert.h>
#include <stdlib.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_string.h"
#include "screens/nc_search_engine.h"
#include "settings.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

typedef struct SearchHookFixture {
    int32 calls;
    bool search_in_database;
    enum NativeSearchEngineSearchMode mode;
    int32 constraint_count;
    NcmBuffer constraint;
} SearchHookFixture;

typedef struct ExternalHookFixture {
    int32 database_calls;
    int32 playlist_calls;
    int32 prompt_calls;
    int32 status_calls;
    int32 add_calls;
    bool play;
    NcmBuffer prompt_label;
    NcmBuffer prompt_initial;
    NcmBuffer status;
} ExternalHookFixture;

typedef struct RunCurrentFixture {
    int32 calls;
    bool runnable;
} RunCurrentFixture;

typedef struct StaticRowFixture {
    int32 calls;
    int32 last_row;
} StaticRowFixture;

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
static void test_search_collection_hooks(void);
static void test_run_current_bridge(void);
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
static bool collect_results(void *user, bool search_in_database,
                            enum NativeSearchEngineSearchMode mode,
                            NcmBuffer *constraints, int32 constraint_count,
                            NcmSongArray *songs, NcmError *error);
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
static bool can_run_current(void *user);
static bool run_current(void *user);
static void static_row_changed(void *user, int32 row);

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
    test_search_collection_hooks();
    test_run_current_bridge();
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
    assert(ncm_buffer_set(result, LIT_ARGS("new title")));
    return NATIVE_SEARCH_ENGINE_PROMPT_ACCEPTED;
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
static_row_changed(void *user, int32 row) {
    StaticRowFixture *fixture;

    fixture = user;
    fixture->calls += 1;
    fixture->last_row = row;
    return;
}

static void
test_static_row_updates(void) {
    NativeSearchEngineScreen screen;
    NativeSearchEngineBridge bridge = {0};
    StaticRowFixture fixture = {0};
    NcMenu *menu;
    int64 item_count;

    init_screen(&screen);
    bridge.static_row_changed = static_row_changed;
    bridge.user = &fixture;
    native_search_engine_screen_set_bridge(&screen, bridge);

    native_search_engine_screen_prepare_static_rows(&screen);
    assert(fixture.calls == 1);
    assert(fixture.last_row == NATIVE_SEARCH_ENGINE_ALL_STATIC_ROWS);
    add_result_songs(&screen);
    assert(native_search_engine_screen_add_result_summary(&screen, 2));
    menu = native_search_engine_screen_menu(&screen);
    item_count = nc_menu_all_item_count(menu);
    nc_menu_highlight_position(menu,
                               NATIVE_SEARCH_ENGINE_STATIC_ROW_COUNT,
                               24);

    assert(native_search_engine_screen_set_constraint(
        &screen, 3, LIT_ARGS("updated title")));
    assert(fixture.calls == 2);
    assert(fixture.last_row == 3);
    assert_row_text(&screen, 3, LIT_ARGS("Title        : updated title"));
    assert(nc_menu_all_item_count(menu) == item_count);
    assert(nc_menu_highlight(menu)
           == NATIVE_SEARCH_ENGINE_STATIC_ROW_COUNT);
    assert(native_search_engine_screen_has_result_rows(&screen));

    native_search_engine_screen_set_search_source(&screen, false);
    assert(fixture.calls == 3);
    assert(fixture.last_row == NATIVE_SEARCH_ENGINE_SEARCH_SOURCE_ROW);
    assert_row_text(&screen, NATIVE_SEARCH_ENGINE_SEARCH_SOURCE_ROW,
                    LIT_ARGS("Search in: Current playlist"));
    assert(nc_menu_all_item_count(menu) == item_count);

    assert(native_search_engine_screen_set_search_mode(
        &screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT));
    assert(fixture.calls == 4);
    assert(fixture.last_row == NATIVE_SEARCH_ENGINE_SEARCH_MODE_ROW);
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
    NcMenu *menu;
    NcSearchRow *row;
    NcmError error;

    init_screen(&screen);
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
    assert(nc_menu_filtered_item_count(menu) == 1);
    row = nc_menu_active_item_at(menu, 0);
    assert(row != NULL);
    assert_song_uri(&row->song, LIT_ARGS("other.flac"));

    native_search_engine_screen_clear_filter(&screen);
    ncm_error_clear(&error);
    assert(native_search_engine_screen_apply_filter(
        &screen, LIT_ARGS("needle-in-uri"), &error));
    assert(nc_menu_filtered_item_count(menu) == 0);

    native_search_engine_screen_clear_filter(&screen);
    nc_menu_highlight_position(menu, 0, 24);
    ncm_error_clear(&error);
    assert(native_search_engine_screen_search(
        &screen, LIT_ARGS("Needle"), true, true, false, &error));
    assert(nc_menu_highlight(menu) == 1);

    ncm_song_destroy(&song);
    native_search_engine_screen_destroy(&screen);
    return;
}

static bool
collect_results(void *user, bool search_in_database,
                enum NativeSearchEngineSearchMode mode,
                NcmBuffer *constraints, int32 constraint_count,
                NcmSongArray *songs, NcmError *error) {
    SearchHookFixture *fixture;
    NcmSong song;

    (void)error;
    fixture = user;
    fixture->calls += 1;
    fixture->search_in_database = search_in_database;
    fixture->mode = mode;
    fixture->constraint_count = constraint_count;
    ncm_buffer_set(&fixture->constraint,
                   constraints[3].data, constraints[3].len);

    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("result.flac")));
    assert(ncm_song_array_append_copy(songs, &song));
    ncm_song_destroy(&song);
    return true;
}

static void
test_search_collection_hooks(void) {
    NativeSearchEngineScreen screen;
    NativeSearchEngineHooks hooks = {0};
    SearchHookFixture fixture;
    NcmSongArray songs;
    NcmError error;
    int32 expected_calls;

    fixture = (SearchHookFixture){0};
    ncm_buffer_init(&fixture.constraint);
    init_screen(&screen);
    ncm_song_array_init(&songs);
    hooks.collect_results = collect_results;
    hooks.user = &fixture;
    native_search_engine_screen_set_hooks(&screen, hooks);

    ncm_error_clear(&error);
    assert(native_search_engine_screen_collect_results(
        &screen, &songs, &error));
    assert(fixture.calls == 0);
    assert(songs.len == 0);

    assert(native_search_engine_screen_set_constraint(
        &screen, 3, LIT_ARGS("needle")));
    expected_calls = 0;
    for (uint32 source = 0; source < 2; source += 1) {
        for (uint32 mode = NATIVE_SEARCH_ENGINE_SEARCH_MODE_LITERAL;
             mode < NATIVE_SEARCH_ENGINE_SEARCH_MODE_LAST; mode += 1) {
            native_search_engine_screen_set_search_source(
                &screen, source != 0);
            assert(native_search_engine_screen_set_search_mode(
                &screen, (enum NativeSearchEngineSearchMode)mode));
            ncm_song_array_clear(&songs);
            ncm_error_clear(&error);
            assert(native_search_engine_screen_collect_results(
                &screen, &songs, &error));
            expected_calls += 1;
            assert(fixture.calls == expected_calls);
            assert(fixture.search_in_database == (source != 0));
            assert(fixture.mode
                   == (enum NativeSearchEngineSearchMode)mode);
            assert(fixture.constraint_count
                   == NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT);
            assert(ncm_string_equal(fixture.constraint.data,
                                    fixture.constraint.len,
                                    LIT_ARGS("needle")));
            assert(songs.len == 1);
            assert_song_uri(&songs.items[0], LIT_ARGS("result.flac"));
        }
    }

    ncm_song_array_destroy(&songs);
    native_search_engine_screen_destroy(&screen);
    ncm_buffer_destroy(&fixture.constraint);
    return;
}

static bool
can_run_current(void *user) {
    RunCurrentFixture *fixture;

    fixture = user;
    return fixture->runnable;
}

static bool
run_current(void *user) {
    RunCurrentFixture *fixture;

    fixture = user;
    fixture->calls += 1;
    return true;
}

static void
test_run_current_bridge(void) {
    NativeSearchEngineScreen screen;
    NativeSearchEngineBridge bridge = {0};
    RunCurrentFixture fixture = {0};
    NcScreen *base;

    init_screen(&screen);
    base = native_search_engine_screen_base(&screen);

    assert(!nc_screen_can_run_current(base));
    assert(!nc_screen_run_current(base));

    bridge.can_run_current = can_run_current;
    bridge.run_current = run_current;
    bridge.user = &fixture;
    native_search_engine_screen_set_bridge(&screen, bridge);

    assert(!nc_screen_can_run_current(base));
    assert(!nc_screen_run_current(base));
    assert(fixture.calls == 0);

    fixture.runnable = true;
    assert(nc_screen_can_run_current(base));
    assert(nc_screen_run_current(base));
    assert(fixture.calls == 1);

    native_search_engine_screen_destroy(&screen);
    return;
}
