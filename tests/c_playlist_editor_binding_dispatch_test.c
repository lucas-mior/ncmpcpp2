#include <assert.h>
#include <stdlib.h>

#include "actions_legacy_runtime.h"
#include "app_legacy_bridge.h"
#include "bindings.h"
#include "settings_legacy_runtime.h"

typedef struct TestState {
    enum NcmActionType can_run_types[64];
    enum NcmActionType run_types[64];
    int32 can_run_count;
    int32 run_count;
    int32 legacy_binding_count;
    int32 legacy_action_count;
    int32 unrelated_legacy_count;
    bool can_run_result;
} TestState;

static TestState test_state;

static void test_state_reset(void);
static NcmBinding test_binding(NcmBindingAction *actions,
                               int32 actions_len);
static void test_migrated_binding_uses_c_runtime(void);
static void test_migrated_action_uses_c_runtime(void);
static void test_playlist_editor_column_binding_uses_c_runtime(void);
static void test_playlist_editor_column_action_uses_c_runtime(void);
static void test_playlist_editor_selection_binding_uses_c_runtime(void);
static void test_playlist_editor_selection_action_uses_c_runtime(void);
static void test_playlist_editor_scroll_binding_uses_c_runtime(void);
static void test_playlist_editor_search_movement_uses_c_runtime(void);
static void test_playlist_editor_jump_to_playing_uses_c_runtime(void);
static void test_playlist_editor_action_parity_uses_c_runtime(void);
static void test_playlist_editor_disabled_action_does_not_fall_back(void);

enum ScreenType
__wrap_native_c_screens_current_type(void) {
    return NCM_SCREEN_TYPE_PLAYLIST_EDITOR;
}

bool
__wrap_ncm_action_runtime_can_run(NcmActionRuntime *runtime,
                                  enum NcmActionType type) {
    (void)runtime;
    test_state.can_run_types[test_state.can_run_count++] = type;
    return test_state.can_run_result;
}

bool
__wrap_ncm_action_runtime_run(NcmActionRuntime *runtime,
                              enum NcmActionType type) {
    (void)runtime;
    test_state.run_types[test_state.run_count++] = type;
    return true;
}

bool
__wrap_actions_legacy_runtime_execute_binding(NcmBinding *binding) {
    (void)binding;
    test_state.legacy_binding_count += 1;
    return true;
}

bool
__wrap_actions_legacy_runtime_execute_action(enum NcmActionType type) {
    (void)type;
    test_state.legacy_action_count += 1;
    return true;
}

bool
settings_legacy_runtime_sync_configuration(void) {
    test_state.unrelated_legacy_count += 1;
    return false;
}

void
actions_legacy_runtime_init_readline(void) {
    test_state.unrelated_legacy_count += 1;
    return;
}

void *
actions_legacy_runtime_window_create(int64 start_x, int64 start_y,
                                     int64 width, int64 height,
                                     NcColor color) {
    (void)start_x;
    (void)start_y;
    (void)width;
    (void)height;
    (void)color;
    test_state.unrelated_legacy_count += 1;
    return NULL;
}

void
actions_legacy_runtime_window_display(void *window) {
    (void)window;
    test_state.unrelated_legacy_count += 1;
    return;
}

void
actions_legacy_runtime_window_destroy(void *window) {
    (void)window;
    test_state.unrelated_legacy_count += 1;
    return;
}

void
actions_legacy_runtime_window_set_main_hook(void *window) {
    (void)window;
    test_state.unrelated_legacy_count += 1;
    return;
}

void
actions_legacy_runtime_window_clear_fd_callbacks(void *window) {
    (void)window;
    test_state.unrelated_legacy_count += 1;
    return;
}

NcWindow *
actions_legacy_runtime_window_native(void *window) {
    (void)window;
    test_state.unrelated_legacy_count += 1;
    return NULL;
}

void
actions_legacy_runtime_initialize_screens(void) {
    test_state.unrelated_legacy_count += 1;
    return;
}

void
actions_legacy_runtime_resize_screen(bool reload_main_window) {
    (void)reload_main_window;
    test_state.unrelated_legacy_count += 1;
    return;
}

bool
actions_legacy_runtime_update_environment(bool update_timer,
                                          bool refresh_window,
                                          bool mpd_sync) {
    (void)update_timer;
    (void)refresh_window;
    (void)mpd_sync;
    test_state.unrelated_legacy_count += 1;
    return false;
}

bool
actions_legacy_runtime_exit_requested(void) {
    test_state.unrelated_legacy_count += 1;
    return false;
}

static void
test_state_reset(void) {
    test_state = (TestState){0};
    test_state.can_run_result = true;
    return;
}

static NcmBinding
test_binding(NcmBindingAction *actions, int32 actions_len) {
    NcmBinding binding = {
        .actions = actions,
        .actions_len = actions_len,
        .actions_cap = actions_len,
    };

    return binding;
}

static void
test_migrated_binding_uses_c_runtime(void) {
    NcmBindingAction actions[] = {
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_APPLY_FILTER,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_QUIT,
        },
    };
    NcmBinding binding;

    test_state_reset();
    binding = test_binding(actions, NCM_ARRAY_LEN(actions));

    assert(ncmpcpp_legacy_execute_binding(&binding));
    assert(test_state.can_run_count == 4);
    assert(test_state.run_count == 2);
    assert(test_state.run_types[0] == NCM_ACTION_APPLY_FILTER);
    assert(test_state.run_types[1] == NCM_ACTION_QUIT);
    assert(test_state.legacy_binding_count == 0);
    assert(test_state.legacy_action_count == 0);
    assert(test_state.unrelated_legacy_count == 0);
    return;
}


static void
test_playlist_editor_column_binding_uses_c_runtime(void) {
    NcmBindingAction actions[] = {
        {
            .kind = NCM_BINDING_ACTION_REQUIRE_RUNNABLE,
            .type = NCM_ACTION_NEXT_COLUMN,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_NEXT_COLUMN,
        },
    };
    NcmBinding binding;

    test_state_reset();
    binding = test_binding(actions, NCM_ARRAY_LEN(actions));

    assert(ncmpcpp_legacy_execute_binding(&binding));
    assert(test_state.can_run_count == 4);
    assert(test_state.can_run_types[0] == NCM_ACTION_NEXT_COLUMN);
    assert(test_state.can_run_types[1] == NCM_ACTION_NEXT_COLUMN);
    assert(test_state.can_run_types[2] == NCM_ACTION_NEXT_COLUMN);
    assert(test_state.can_run_types[3] == NCM_ACTION_NEXT_COLUMN);
    assert(test_state.run_count == 1);
    assert(test_state.run_types[0] == NCM_ACTION_NEXT_COLUMN);
    assert(test_state.legacy_binding_count == 0);
    assert(test_state.legacy_action_count == 0);
    assert(test_state.unrelated_legacy_count == 0);
    return;
}

static void
test_playlist_editor_column_action_uses_c_runtime(void) {
    test_state_reset();

    assert(ncmpcpp_legacy_execute_action(NCM_ACTION_PREVIOUS_COLUMN));
    assert(test_state.can_run_count == 0);
    assert(test_state.run_count == 1);
    assert(test_state.run_types[0] == NCM_ACTION_PREVIOUS_COLUMN);
    assert(test_state.legacy_binding_count == 0);
    assert(test_state.legacy_action_count == 0);
    assert(test_state.unrelated_legacy_count == 0);
    return;
}


static void
test_playlist_editor_selection_binding_uses_c_runtime(void) {
    NcmBindingAction actions[] = {
        {
            .kind = NCM_BINDING_ACTION_REQUIRE_RUNNABLE,
            .type = NCM_ACTION_SELECT_ITEM,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_REVERSE_SELECTION,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_REMOVE_SELECTION,
        },
    };
    NcmBinding binding;

    test_state_reset();
    binding = test_binding(actions, NCM_ARRAY_LEN(actions));

    assert(ncmpcpp_legacy_execute_binding(&binding));
    assert(test_state.can_run_count == 6);
    assert(test_state.run_count == 2);
    assert(test_state.run_types[0] == NCM_ACTION_REVERSE_SELECTION);
    assert(test_state.run_types[1] == NCM_ACTION_REMOVE_SELECTION);
    assert(test_state.legacy_binding_count == 0);
    assert(test_state.legacy_action_count == 0);
    assert(test_state.unrelated_legacy_count == 0);
    return;
}

static void
test_playlist_editor_selection_action_uses_c_runtime(void) {
    test_state_reset();

    assert(ncmpcpp_legacy_execute_action(NCM_ACTION_SELECT_RANGE));
    assert(test_state.can_run_count == 0);
    assert(test_state.run_count == 1);
    assert(test_state.run_types[0] == NCM_ACTION_SELECT_RANGE);
    assert(test_state.legacy_binding_count == 0);
    assert(test_state.legacy_action_count == 0);
    assert(test_state.unrelated_legacy_count == 0);
    return;
}

static void
test_playlist_editor_scroll_binding_uses_c_runtime(void) {
    NcmBindingAction actions[] = {
        {
            .kind = NCM_BINDING_ACTION_REQUIRE_RUNNABLE,
            .type = NCM_ACTION_SCROLL_DOWN,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_PAGE_DOWN,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_MOVE_END,
        },
    };
    NcmBinding binding;

    test_state_reset();
    binding = test_binding(actions, NCM_ARRAY_LEN(actions));

    assert(ncmpcpp_legacy_execute_binding(&binding));
    assert(test_state.can_run_count == 6);
    assert(test_state.run_count == 2);
    assert(test_state.run_types[0] == NCM_ACTION_PAGE_DOWN);
    assert(test_state.run_types[1] == NCM_ACTION_MOVE_END);
    assert(test_state.legacy_binding_count == 0);
    assert(test_state.legacy_action_count == 0);
    assert(test_state.unrelated_legacy_count == 0);
    return;
}

static void
test_playlist_editor_search_movement_uses_c_runtime(void) {
    test_state_reset();

    assert(ncmpcpp_legacy_execute_action(NCM_ACTION_NEXT_FOUND_ITEM));
    assert(test_state.can_run_count == 0);
    assert(test_state.run_count == 1);
    assert(test_state.run_types[0] == NCM_ACTION_NEXT_FOUND_ITEM);
    assert(test_state.legacy_binding_count == 0);
    assert(test_state.legacy_action_count == 0);
    assert(test_state.unrelated_legacy_count == 0);
    return;
}

static void
test_playlist_editor_jump_to_playing_uses_c_runtime(void) {
    test_state_reset();

    assert(ncmpcpp_legacy_execute_action(NCM_ACTION_JUMP_TO_PLAYING_SONG));
    assert(test_state.can_run_count == 0);
    assert(test_state.run_count == 1);
    assert(test_state.run_types[0] == NCM_ACTION_JUMP_TO_PLAYING_SONG);
    assert(test_state.legacy_binding_count == 0);
    assert(test_state.legacy_action_count == 0);
    assert(test_state.unrelated_legacy_count == 0);
    return;
}

static void
test_playlist_editor_action_parity_uses_c_runtime(void) {
    enum NcmActionType actions[] = {
        NCM_ACTION_ADD_ITEM_TO_PLAYLIST,
        NCM_ACTION_PLAY_ITEM,
        NCM_ACTION_DELETE_PLAYLIST_ITEMS,
        NCM_ACTION_DELETE_STORED_PLAYLIST,
        NCM_ACTION_MOVE_SELECTED_ITEMS_UP,
        NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN,
        NCM_ACTION_MOVE_SELECTED_ITEMS_TO,
        NCM_ACTION_ADD,
        NCM_ACTION_LOAD,
        NCM_ACTION_TOGGLE_DISPLAY_MODE,
        NCM_ACTION_EDIT_PLAYLIST_NAME,
        NCM_ACTION_CROP_PLAYLIST,
        NCM_ACTION_CLEAR_PLAYLIST,
        NCM_ACTION_SHOW_PLAYLIST_EDITOR,
        NCM_ACTION_JUMP_TO_PLAYLIST_EDITOR,
    };

    test_state_reset();
    for (int32 i = 0; i < NCM_ARRAY_LEN(actions); i += 1) {
        assert(ncmpcpp_legacy_execute_action(actions[i]));
        assert(test_state.run_count == i + 1);
        assert(test_state.run_types[i] == actions[i]);
    }
    assert(test_state.can_run_count == 0);
    assert(test_state.legacy_binding_count == 0);
    assert(test_state.legacy_action_count == 0);
    assert(test_state.unrelated_legacy_count == 0);
    return;
}

static void
test_playlist_editor_disabled_action_does_not_fall_back(void) {
    NcmBindingAction actions[] = {
        {
            .kind = NCM_BINDING_ACTION_REQUIRE_RUNNABLE,
            .type = NCM_ACTION_LOAD,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_LOAD,
        },
    };
    NcmBinding binding;

    test_state_reset();
    test_state.can_run_result = false;
    binding = test_binding(actions, NCM_ARRAY_LEN(actions));

    assert(!ncmpcpp_legacy_execute_binding(&binding));
    assert(test_state.can_run_count > 0);
    assert(test_state.run_count == 0);
    assert(test_state.legacy_binding_count == 0);
    assert(test_state.legacy_action_count == 0);
    assert(test_state.unrelated_legacy_count == 0);
    return;
}

static void
test_migrated_action_uses_c_runtime(void) {
    test_state_reset();

    assert(ncmpcpp_legacy_execute_action(NCM_ACTION_APPLY_FILTER));
    assert(test_state.can_run_count == 0);
    assert(test_state.run_count == 1);
    assert(test_state.run_types[0] == NCM_ACTION_APPLY_FILTER);
    assert(test_state.legacy_binding_count == 0);
    assert(test_state.legacy_action_count == 0);
    assert(test_state.unrelated_legacy_count == 0);
    return;
}

int
main(void) {
    test_migrated_binding_uses_c_runtime();
    test_migrated_action_uses_c_runtime();
    test_playlist_editor_column_binding_uses_c_runtime();
    test_playlist_editor_column_action_uses_c_runtime();
    test_playlist_editor_selection_binding_uses_c_runtime();
    test_playlist_editor_selection_action_uses_c_runtime();
    test_playlist_editor_scroll_binding_uses_c_runtime();
    test_playlist_editor_search_movement_uses_c_runtime();
    test_playlist_editor_jump_to_playing_uses_c_runtime();
    test_playlist_editor_action_parity_uses_c_runtime();
    test_playlist_editor_disabled_action_does_not_fall_back();
    exit(EXIT_SUCCESS);
}
