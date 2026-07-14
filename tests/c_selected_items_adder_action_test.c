#include <assert.h>
#include <stdlib.h>

#include "actions.h"
#include "app_binding_migration.h"
#include "c/ncm_app_arrays.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "screens/native_c_screens.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

typedef struct SelectedItemsActionTestState {
    NcmSongArray source_songs;
    NcmSongArray opened_songs;
    enum ScreenType current_type;
    int32 open_calls;
    bool open_result;
} SelectedItemsActionTestState;

static SelectedItemsActionTestState test_state;

static void test_state_init(void);
static void test_state_destroy(void);
static void test_state_set_song(char *uri, int32 uri_len);
static void test_add_selected_items_binding_is_c_safe(void);
static void test_selected_items_adder_screen_is_c_only(void);
static void test_playlist_action_opens_native_dialog(void);
static void test_tag_editor_action_opens_native_dialog(void);
static void test_empty_selection_does_not_open_dialog(void);

int
main(void) {
    test_state_init();
    test_add_selected_items_binding_is_c_safe();
    test_selected_items_adder_screen_is_c_only();
    test_playlist_action_opens_native_dialog();
    test_tag_editor_action_opens_native_dialog();
    test_empty_selection_does_not_open_dialog();
    test_state_destroy();
    exit(EXIT_SUCCESS);
}

enum ScreenType
__wrap_native_c_screens_current_type(void) {
    return test_state.current_type;
}

NativePlaylistScreen *
__wrap_native_c_screen_playlist(void) {
    return (NativePlaylistScreen *)&test_state;
}

bool
__wrap_native_playlist_screen_selected_songs(
    NativePlaylistScreen *screen, NcmSongArray *songs) {
    (void)screen;
    return ncm_song_array_copy(songs, &test_state.source_songs);
}

NativeTagEditorScreen *
__wrap_native_c_screen_tag_editor(void) {
    return (NativeTagEditorScreen *)&test_state;
}

bool
__wrap_native_tag_editor_screen_selected_songs(
    NativeTagEditorScreen *screen, NcmSongArray *songs) {
    (void)screen;
    return ncm_song_array_copy(songs, &test_state.source_songs);
}

bool
__wrap_native_c_screen_selected_items_adder_open(
    NcmSongArray *songs, NcmError *error) {
    test_state.open_calls += 1;
    ncm_error_clear(error);
    if (!test_state.open_result) {
        ncm_error_set(error, 1, LIT_ARGS("dialog open failed"));
        return false;
    }
    return ncm_song_array_copy(&test_state.opened_songs, songs);
}

static void
test_state_init(void) {
    ncm_song_array_init(&test_state.source_songs);
    ncm_song_array_init(&test_state.opened_songs);
    test_state.current_type = NCM_SCREEN_TYPE_UNKNOWN;
    test_state.open_calls = 0;
    test_state.open_result = true;
    return;
}

static void
test_state_destroy(void) {
    ncm_song_array_destroy(&test_state.opened_songs);
    ncm_song_array_destroy(&test_state.source_songs);
    return;
}

static void
test_state_set_song(char *uri, int32 uri_len) {
    NcmSong song;

    ncm_song_array_clear(&test_state.source_songs);
    ncm_song_array_clear(&test_state.opened_songs);
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, uri, uri_len));
    assert(ncm_song_array_append_copy(&test_state.source_songs, &song));
    ncm_song_destroy(&song);
    return;
}

static void
test_add_selected_items_binding_is_c_safe(void) {
    NcmBindingAction action = {
        .kind = NCM_BINDING_ACTION_NORMAL,
        .type = NCM_ACTION_ADD_SELECTED_ITEMS,
    };
    NcmBinding binding = {
        .actions = &action,
        .actions_len = 1,
        .actions_cap = 1,
    };

    assert(app_binding_migration_action_is_c_safe(
        NCM_ACTION_ADD_SELECTED_ITEMS));
    assert(app_binding_migration_binding_is_c_safe(&binding));
    assert(app_binding_migration_binding_is_hybrid_safe(&binding));
    assert(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER));
    return;
}

static void
test_selected_items_adder_screen_is_c_only(void) {
    enum NcmActionType types[] = {
        NCM_ACTION_MOUSE_EVENT,
        NCM_ACTION_SCROLL_UP,
        NCM_ACTION_SCROLL_DOWN,
        NCM_ACTION_PAGE_UP,
        NCM_ACTION_PAGE_DOWN,
        NCM_ACTION_MOVE_HOME,
        NCM_ACTION_MOVE_END,
        NCM_ACTION_ADD,
        NCM_ACTION_NEXT_FOUND_ITEM,
        NCM_ACTION_PREVIOUS_FOUND_ITEM,
        NCM_ACTION_TOGGLE_FIND_MODE,
        NCM_ACTION_NEXT_SCREEN,
        NCM_ACTION_PREVIOUS_SCREEN,
    };
    NcmBindingAction action = {
        .kind = NCM_BINDING_ACTION_NORMAL,
    };
    NcmBinding binding = {
        .actions = &action,
        .actions_len = 1,
        .actions_cap = 1,
    };

    assert(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER));
    for (int32 i = 0; i < LENGTH(types); i += 1) {
        action.type = types[i];
        assert(app_binding_migration_action_is_c_safe_for_screen(
            action.type, NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER));
        assert(app_binding_migration_binding_is_c_safe_for_screen(
            &binding, NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER));
    }

    action.type = NCM_ACTION_RUN_ACTION;
    assert(app_binding_migration_action_is_c_safe_for_screen(
        action.type, NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER));
    action.type = NCM_ACTION_MOVE_SELECTED_ITEMS_TO;
    assert(!app_binding_migration_action_is_c_safe_for_screen(
        action.type, NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER));
    assert(!app_binding_migration_binding_is_c_safe_for_screen(
        &binding, NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER));
    return;
}

static void
test_playlist_action_opens_native_dialog(void) {
    NcmStringView uri;

    test_state_set_song(LIT_ARGS("playlist.flac"));
    test_state.current_type = NCM_SCREEN_TYPE_PLAYLIST;
    test_state.open_calls = 0;

    assert(ncm_action_runtime_can_run(NULL,
                                      NCM_ACTION_ADD_SELECTED_ITEMS));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_ADD_SELECTED_ITEMS));
    assert(test_state.open_calls == 1);
    assert(test_state.opened_songs.len == 1);
    assert(ncm_song_uri_view(&test_state.opened_songs.items[0], 0, &uri));
    assert(ncm_string_equal(uri.data, uri.len,
                            LIT_ARGS("playlist.flac")));
    return;
}

static void
test_tag_editor_action_opens_native_dialog(void) {
    NcmStringView uri;

    test_state_set_song(LIT_ARGS("tag-editor.flac"));
    test_state.current_type = NCM_SCREEN_TYPE_TAG_EDITOR;
    test_state.open_calls = 0;

    assert(ncm_action_runtime_can_run(NULL,
                                      NCM_ACTION_ADD_SELECTED_ITEMS));
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_ADD_SELECTED_ITEMS));
    assert(test_state.open_calls == 1);
    assert(test_state.opened_songs.len == 1);
    assert(ncm_song_uri_view(&test_state.opened_songs.items[0], 0, &uri));
    assert(ncm_string_equal(uri.data, uri.len,
                            LIT_ARGS("tag-editor.flac")));
    return;
}

static void
test_empty_selection_does_not_open_dialog(void) {
    ncm_song_array_clear(&test_state.source_songs);
    ncm_song_array_clear(&test_state.opened_songs);
    test_state.current_type = NCM_SCREEN_TYPE_PLAYLIST;
    test_state.open_calls = 0;

    assert(!ncm_action_runtime_can_run(NULL,
                                       NCM_ACTION_ADD_SELECTED_ITEMS));
    assert(!ncm_action_runtime_run(NULL, NCM_ACTION_ADD_SELECTED_ITEMS));
    assert(test_state.open_calls == 0);
    return;
}
