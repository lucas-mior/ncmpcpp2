#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "app_controller.h"
#include "screen_actions.h"
#include "settings.h"
#include "ui_state.h"
#include "c/ncm_base.h"
#include "c/ncm_error.h"
#include "screens/native_c_screens.h"
#include "screens/nc_search_engine.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

typedef struct LegacySearchFixture {
    int32 search_hook_calls;
    int32 clear_hook_calls;
    int32 search_calls;
    int32 clear_calls;
    enum SearchDirection direction;
    NcmBuffer pattern;
    bool wrap;
    bool skip_current;
} LegacySearchFixture;

static LegacySearchFixture fixture;

static bool current_screen_uses_legacy_search(void);
static void test_legacy_screen_search_bridge(void);
static void test_search_engine_native_search(void);
static void test_native_screen_search_fallback(void);
static void test_c_only_screen_skips_legacy_search(void);
static void test_playlist_editor_skips_legacy_search(void);
static void test_tag_editor_skips_legacy_search(void);
static bool format_search_song(void *user, NcmSong *song,
                               NcmBuffer *text);

bool
actions_legacy_runtime_search_current_screen(
    enum SearchDirection direction, char *pattern, int32 pattern_len,
    bool wrap, bool skip_current, bool *handled, NcmError *error) {
    (void)error;
    fixture.search_hook_calls += 1;
    if (!current_screen_uses_legacy_search()) {
        *handled = false;
        return false;
    }

    fixture.search_calls += 1;
    fixture.direction = direction;
    fixture.wrap = wrap;
    fixture.skip_current = skip_current;
    ncm_buffer_set(&fixture.pattern, pattern, pattern_len);
    *handled = true;
    return true;
}

void
actions_legacy_runtime_clear_current_search(void) {
    fixture.clear_hook_calls += 1;
    if (current_screen_uses_legacy_search()) {
        fixture.clear_calls += 1;
    }
    return;
}

int
main(void) {
    ncm_buffer_init(&fixture.pattern);
    test_legacy_screen_search_bridge();
    test_search_engine_native_search();
    test_native_screen_search_fallback();
    test_c_only_screen_skips_legacy_search();
    test_playlist_editor_skips_legacy_search();
    test_tag_editor_skips_legacy_search();
    ncm_buffer_destroy(&fixture.pattern);
    exit(EXIT_SUCCESS);
}

static bool
current_screen_uses_legacy_search(void) {
    NcScreen *screen;

    screen = app_controller_current_screen();
    if (screen == NULL) {
        return false;
    }
    return nc_screen_type(screen) == NC_SCREEN_TYPE_BROWSER
        || nc_screen_type(screen) == NC_SCREEN_TYPE_SEARCH_ENGINE;
}

static void
test_legacy_screen_search_bridge(void) {
    NcmStringView constraint;
    NcmError error;

    app_controller_init();
    ui_state_set_screen_size(100, 30);
    ui_state_set_main_geometry(2, 26);
    native_c_screen_browser_register();
    assert(app_controller_switch_to_screen(
        native_c_screen_browser_native()));

    ncm_error_clear(&error);
    assert(current_screen_search(
        NCM_SEARCH_DIRECTION_BACKWARD, LIT_ARGS("needle"),
        true, true, &error));
    assert(!ncm_error_is_set(&error));
    assert(fixture.search_hook_calls == 1);
    assert(fixture.search_calls == 1);
    assert(fixture.direction == NCM_SEARCH_DIRECTION_BACKWARD);
    assert(fixture.wrap);
    assert(fixture.skip_current);
    assert(ncm_string_equal(fixture.pattern.data,
                            fixture.pattern.len,
                            LIT_ARGS("needle")));

    constraint = current_screen_current_search_constraint();
    assert(ncm_string_equal(constraint.data, constraint.len,
                            LIT_ARGS("needle")));

    current_screen_clear_search_constraint();
    assert(fixture.clear_hook_calls == 1);
    assert(fixture.clear_calls == 1);
    constraint = current_screen_current_search_constraint();
    assert(constraint.len == 0);
    return;
}

static void
test_search_engine_native_search(void) {
    NativeSearchEngineScreen *search;
    NativeSearchEngineHooks hooks = {0};
    NcmStringView constraint;
    NcmSong song;
    NcmError error;
    int32 clear_hook_calls;
    int32 search_hook_calls;

    app_controller_init();
    ui_state_set_screen_size(100, 30);
    ui_state_set_main_geometry(2, 26);
    native_c_screen_search_engine_register();
    assert(app_controller_switch_to_screen(
        native_c_screen_search_engine_native()));

    search = native_c_screen_search_engine();
    hooks.format_song = format_search_song;
    native_search_engine_screen_set_hooks(search, hooks);
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("result.flac")));
    assert(ncm_song_add_tag(&song, MPD_TAG_TITLE,
                            LIT_ARGS("result title")));
    assert(native_search_engine_screen_add_song_copy(search, &song));
    ncm_song_destroy(&song);

    search_hook_calls = fixture.search_hook_calls;
    ncm_error_clear(&error);
    assert(current_screen_search(
        NCM_SEARCH_DIRECTION_FORWARD, LIT_ARGS("result title"),
        false, false, &error));
    assert(!ncm_error_is_set(&error));
    assert(fixture.search_hook_calls == search_hook_calls);

    constraint = current_screen_current_search_constraint();
    assert(ncm_string_equal(constraint.data, constraint.len,
                            LIT_ARGS("result title")));

    clear_hook_calls = fixture.clear_hook_calls;
    current_screen_clear_search_constraint();
    assert(fixture.clear_hook_calls == clear_hook_calls);
    constraint = current_screen_current_search_constraint();
    assert(constraint.len == 0);
    return;
}

static bool
format_search_song(void *user, NcmSong *song, NcmBuffer *text) {
    NcmStringView view;

    (void)user;
    if (!ncm_song_tag_view(song, MPD_TAG_TITLE, 0, &view)) {
        return false;
    }
    return ncm_buffer_set(text, view.data, view.len);
}

static void
test_native_screen_search_fallback(void) {
    NativeMediaLibraryScreen *library;
    NcMenu *menu;
    NcmError error;
    int32 search_calls;

    app_controller_init();
    Config.media_lib_primary_tag = MPD_TAG_ARTIST;
    Config.regex_type = NCM_REGEX_LITERAL_CASE_INSENSITIVE;
    library = native_c_screen_media_library();
    native_media_library_screen_clear(library);
    assert(native_media_library_screen_set_mode(
        library, NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS));
    assert(native_media_library_screen_set_active_column(
        library, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS));
    assert(native_media_library_screen_add_tag(
        library, LIT_ARGS("Alpha"), 0));
    assert(native_media_library_screen_add_tag(
        library, LIT_ARGS("Needle"), 0));
    native_c_screen_media_library_register();
    assert(app_controller_switch_to_screen(
        native_c_screen_media_library_native()));

    menu = native_media_library_screen_active_menu(library);
    assert(nc_menu_highlight(menu) == 0);
    search_calls = fixture.search_calls;
    ncm_error_clear(&error);
    assert(current_screen_search(
        NCM_SEARCH_DIRECTION_FORWARD, LIT_ARGS("Needle"),
        true, false, &error));
    assert(!ncm_error_is_set(&error));
    assert(nc_menu_highlight(menu) == 1);
    assert(fixture.search_calls == search_calls);
    return;
}

static void
test_c_only_screen_skips_legacy_search(void) {
    NativeSelectedItemsAdderScreen *screen;
    NcmStringView constraint;
    NcMenu *menu;
    NcmError error;
    int32 clear_hook_calls;
    int32 search_hook_calls;

    app_controller_init();
    Config.regex_type = NCM_REGEX_LITERAL_CASE_INSENSITIVE;
    screen = native_c_screen_selected_items_adder();
    native_selected_items_adder_screen_populate_position_selector(screen);
    screen->active_menu = NATIVE_SELECTED_ITEMS_ADDER_MENU_POSITIONS;
    native_c_screen_selected_items_adder_register();
    assert(app_controller_switch_to_screen(
        native_c_screen_selected_items_adder_native()));

    menu = native_selected_items_adder_screen_active_menu(screen);
    assert(nc_menu_highlight(menu) == 0);
    search_hook_calls = fixture.search_hook_calls;
    ncm_error_clear(&error);
    assert(current_screen_search(
        NCM_SEARCH_DIRECTION_FORWARD, LIT_ARGS("current album"),
        true, false, &error));
    assert(!ncm_error_is_set(&error));
    assert(fixture.search_hook_calls == search_hook_calls);
    assert(nc_menu_highlight(menu) == 3);

    constraint = current_screen_current_search_constraint();
    assert(ncm_string_equal(constraint.data, constraint.len,
                            LIT_ARGS("current album")));
    clear_hook_calls = fixture.clear_hook_calls;
    current_screen_clear_search_constraint();
    assert(fixture.clear_hook_calls == clear_hook_calls);
    constraint = current_screen_current_search_constraint();
    assert(constraint.len == 0);
    return;
}


static void
test_playlist_editor_skips_legacy_search(void) {
    NativePlaylistEditorScreen *screen;
    NcmMpdPlaylistList playlists;
    NcmPlaylist playlist;
    NcmStringView constraint;
    NcmError error;
    int32 clear_hook_calls;
    int32 search_hook_calls;

    app_controller_init();
    ui_state_set_screen_size(100, 30);
    ui_state_set_main_geometry(2, 26);
    Config.regex_type = NCM_REGEX_LITERAL_CASE_INSENSITIVE;
    screen = native_c_screen_playlist_editor();
    native_playlist_editor_screen_clear(screen);

    ncm_mpd_playlist_list_init(&playlists);
    ncm_playlist_init(&playlist);
    assert(ncm_playlist_set(&playlist, LIT_ARGS("Needle list"), 0));
    assert(ncm_mpd_playlist_list_append_copy(&playlists, &playlist));
    ncm_playlist_destroy(&playlist);
    assert(native_playlist_editor_screen_load_playlists(screen,
                                                         &playlists));
    ncm_mpd_playlist_list_destroy(&playlists);

    native_c_screen_playlist_editor_register();
    assert(app_controller_switch_to_screen(
        native_c_screen_playlist_editor_native()));

    search_hook_calls = fixture.search_hook_calls;
    ncm_error_clear(&error);
    assert(current_screen_search(
        NCM_SEARCH_DIRECTION_FORWARD, LIT_ARGS("Needle"),
        true, false, &error));
    assert(!ncm_error_is_set(&error));
    assert(fixture.search_hook_calls == search_hook_calls);

    constraint = current_screen_current_search_constraint();
    assert(ncm_string_equal(constraint.data, constraint.len,
                            LIT_ARGS("Needle")));

    clear_hook_calls = fixture.clear_hook_calls;
    current_screen_clear_search_constraint();
    assert(fixture.clear_hook_calls == clear_hook_calls);
    assert(!screen->playlist_search_enabled);
    constraint = current_screen_current_search_constraint();
    assert(constraint.len == 0);
    return;
}

static void
test_tag_editor_skips_legacy_search(void) {
    NativeTagEditorScreen *screen;
    NcmMutableSong song;
    NcmStringView constraint;
    NcmError error;
    int32 clear_hook_calls;
    int32 search_hook_calls;

    app_controller_init();
    ui_state_set_screen_size(100, 30);
    ui_state_set_main_geometry(2, 26);
    Config.regex_type = NCM_REGEX_LITERAL_CASE_INSENSITIVE;
    screen = native_c_screen_tag_editor();
    native_tag_editor_screen_clear(screen);

    ncm_mutable_song_init(&song);
    assert(ncm_mutable_song_set_uri(&song, LIT_ARGS("song.flac")));
    assert(ncm_mutable_song_set_name(&song, LIT_ARGS("song.flac")));
    assert(ncm_mutable_song_set_original_tag(
               &song, NCM_TAGS_FIELD_ARTIST, 0, LIT_ARGS("Needle")));
    assert(native_tag_editor_screen_add_mutable_song(screen, &song));
    ncm_mutable_song_destroy(&song);

    native_tag_editor_screen_next_column(screen);
    assert(nc_menu_goto_selectable(nc_editor_string_menu_base(
               &screen->tag_types), 1));
    native_tag_editor_screen_next_column(screen);
    native_c_screen_tag_editor_register();
    assert(app_controller_switch_to_screen(
        native_c_screen_tag_editor_native()));

    search_hook_calls = fixture.search_hook_calls;
    ncm_error_clear(&error);
    assert(current_screen_search(
        NCM_SEARCH_DIRECTION_FORWARD, LIT_ARGS("Needle"),
        true, false, &error));
    assert(!ncm_error_is_set(&error));
    assert(fixture.search_hook_calls == search_hook_calls);

    constraint = current_screen_current_search_constraint();
    assert(ncm_string_equal(constraint.data, constraint.len,
                            LIT_ARGS("Needle")));

    clear_hook_calls = fixture.clear_hook_calls;
    current_screen_clear_search_constraint();
    assert(fixture.clear_hook_calls == clear_hook_calls);
    assert(!screen->tag_search_enabled);
    constraint = current_screen_current_search_constraint();
    assert(constraint.len == 0);
    return;
}
