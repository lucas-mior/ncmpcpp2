#include <assert.h>
#include <stdlib.h>

#include "app_legacy_bridge.h"
#include "bindings.h"
#include "settings_legacy_runtime.h"

typedef struct TestState {
    enum NcmActionType can_run_types[8];
    enum NcmActionType run_types[8];
    int32 can_run_count;
    int32 run_count;
    int32 legacy_action_count;
    int32 unrelated_legacy_count;
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
static void test_playlist_editor_load_action_uses_c_runtime(void);
static void test_playlist_editor_load_binding_uses_c_runtime(void);

enum ScreenType
__wrap_native_c_screens_current_type(void) {
    return NCM_SCREEN_TYPE_PLAYLIST_EDITOR;
}

bool
__wrap_ncm_action_runtime_can_run(NcmActionRuntime *runtime,
                                  enum NcmActionType type) {
    (void)runtime;
    test_state.can_run_types[test_state.can_run_count++] = type;
    return true;
}

bool
__wrap_ncm_action_runtime_run(NcmActionRuntime *runtime,
                              enum NcmActionType type) {
    (void)runtime;
    test_state.run_types[test_state.run_count++] = type;
    return true;
}


bool
settings_legacy_runtime_sync_configuration(void) {
    test_state.unrelated_legacy_count += 1;
    return false;
}


static void
test_state_reset(void) {
    test_state = (TestState){0};
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
    assert(test_state.legacy_action_count == 0);
    assert(test_state.unrelated_legacy_count == 0);
    return;
}


static void
test_playlist_editor_load_action_uses_c_runtime(void) {
    test_state_reset();

    assert(ncmpcpp_legacy_execute_action(NCM_ACTION_LOAD));
    assert(test_state.can_run_count == 0);
    assert(test_state.run_count == 1);
    assert(test_state.run_types[0] == NCM_ACTION_LOAD);
    assert(test_state.legacy_action_count == 0);
    assert(test_state.unrelated_legacy_count == 0);
    return;
}

static void
test_playlist_editor_load_binding_uses_c_runtime(void) {
    NcmBindingAction actions[] = {
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_LOAD,
        },
    };
    NcmBinding binding;

    test_state_reset();
    binding = test_binding(actions, NCM_ARRAY_LEN(actions));

    assert(ncmpcpp_legacy_execute_binding(&binding));
    assert(test_state.can_run_count == 2);
    assert(test_state.can_run_types[0] == NCM_ACTION_LOAD);
    assert(test_state.can_run_types[1] == NCM_ACTION_LOAD);
    assert(test_state.run_count == 1);
    assert(test_state.run_types[0] == NCM_ACTION_LOAD);
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
    test_playlist_editor_load_action_uses_c_runtime();
    test_playlist_editor_load_binding_uses_c_runtime();
    exit(EXIT_SUCCESS);
}
