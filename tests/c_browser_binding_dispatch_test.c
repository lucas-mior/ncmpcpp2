#include <assert.h>
#include <stdlib.h>

#include "actions_legacy_runtime.h"
#include "app_legacy_bridge.h"
#include "bindings.h"
#include "settings_legacy_runtime.h"

typedef struct TestState {
    enum NcmActionType can_run_types[64];
    enum NcmActionType run_types[64];
    enum NcmActionType legacy_action_types[64];
    int32 can_run_count;
    int32 run_count;
    int32 legacy_action_count;
    int32 unrelated_legacy_count;
} TestState;

typedef struct BrowserActionExpectation {
    enum NcmActionType type;
    bool migrated;
} BrowserActionExpectation;

static TestState test_state;

static void test_state_reset(void);
static NcmBinding test_binding(NcmBindingAction *actions,
                               int32 actions_len);
static void assert_no_legacy_dispatch(void);
static void assert_c_action(enum NcmActionType type);
static void assert_legacy_action(enum NcmActionType type);
static void test_browser_navigation_binding_uses_c_runtime(void);
static void test_browser_navigation_actions_use_c_runtime(void);
static void test_browser_migration_action_table_matches_dispatch(void);
static void test_browser_mixed_migration_binding_uses_c_runtime(void);
static void test_browser_c_only_common_actions_use_c_runtime(void);

enum ScreenType
__wrap_native_c_screens_current_type(void) {
    return NCM_SCREEN_TYPE_BROWSER;
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
__wrap_actions_legacy_runtime_execute_action(enum NcmActionType type) {
    test_state.legacy_action_types[test_state.legacy_action_count++] = type;
    return true;
}

bool
settings_legacy_runtime_sync_configuration(void) {
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
assert_no_legacy_dispatch(void) {
    assert(test_state.legacy_action_count == 0);
    assert(test_state.unrelated_legacy_count == 0);
    return;
}

static void
assert_c_action(enum NcmActionType type) {
    test_state_reset();

    assert(ncmpcpp_legacy_execute_action(type));
    assert(test_state.can_run_count == 0);
    assert(test_state.run_count == 1);
    assert(test_state.run_types[0] == type);
    assert_no_legacy_dispatch();
    return;
}

static void
assert_legacy_action(enum NcmActionType type) {
    test_state_reset();

    assert(ncmpcpp_legacy_execute_action(type));
    assert(test_state.can_run_count == 0);
    assert(test_state.run_count == 0);
    assert(test_state.legacy_action_count == 1);
    assert(test_state.legacy_action_types[0] == type);
    assert(test_state.unrelated_legacy_count == 0);
    return;
}

static void
test_browser_navigation_binding_uses_c_runtime(void) {
    NcmBindingAction actions[] = {
        {
            .kind = NCM_BINDING_ACTION_REQUIRE_RUNNABLE,
            .type = NCM_ACTION_ENTER_DIRECTORY,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_ENTER_DIRECTORY,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_JUMP_TO_PARENT_DIRECTORY,
        },
    };
    NcmBinding binding;

    test_state_reset();
    binding = test_binding(actions, NCM_ARRAY_LEN(actions));

    assert(ncmpcpp_legacy_execute_binding(&binding));
    assert(test_state.can_run_count == 6);
    assert(test_state.can_run_types[0] == NCM_ACTION_ENTER_DIRECTORY);
    assert(test_state.can_run_types[1] == NCM_ACTION_ENTER_DIRECTORY);
    assert(test_state.can_run_types[2]
           == NCM_ACTION_JUMP_TO_PARENT_DIRECTORY);
    assert(test_state.can_run_types[3] == NCM_ACTION_ENTER_DIRECTORY);
    assert(test_state.can_run_types[4] == NCM_ACTION_ENTER_DIRECTORY);
    assert(test_state.can_run_types[5]
           == NCM_ACTION_JUMP_TO_PARENT_DIRECTORY);
    assert(test_state.run_count == 2);
    assert(test_state.run_types[0] == NCM_ACTION_ENTER_DIRECTORY);
    assert(test_state.run_types[1]
           == NCM_ACTION_JUMP_TO_PARENT_DIRECTORY);
    assert_no_legacy_dispatch();
    return;
}

static void
test_browser_navigation_actions_use_c_runtime(void) {
    assert_c_action(NCM_ACTION_ENTER_DIRECTORY);
    assert_c_action(NCM_ACTION_JUMP_TO_PARENT_DIRECTORY);
    return;
}

static void
test_browser_migration_action_table_matches_dispatch(void) {
    static BrowserActionExpectation actions[] = {
        {
            .type = NCM_ACTION_SHOW_BROWSER,
            .migrated = true,
        },
        {
            .type = NCM_ACTION_ENTER_DIRECTORY,
            .migrated = true,
        },
        {
            .type = NCM_ACTION_JUMP_TO_PARENT_DIRECTORY,
            .migrated = true,
        },
        {
            .type = NCM_ACTION_TOGGLE_BROWSER_SORT_MODE,
            .migrated = true,
        },
        {
            .type = NCM_ACTION_CHANGE_BROWSE_MODE,
            .migrated = true,
        },
        {
            .type = NCM_ACTION_DELETE_BROWSER_ITEMS,
            .migrated = true,
        },
        {
            .type = NCM_ACTION_EDIT_DIRECTORY_NAME,
            .migrated = true,
        },
        {
            .type = NCM_ACTION_EDIT_PLAYLIST_NAME,
            .migrated = true,
        },
        {
            .type = NCM_ACTION_JUMP_TO_BROWSER,
            .migrated = true,
        },
        {
            .type = NCM_ACTION_JUMP_TO_PLAYLIST_EDITOR,
            .migrated = true,
        },
    };

    for (int32 i = 0; i < NCM_ARRAY_LEN(actions); i += 1) {
        if (actions[i].migrated) {
            assert_c_action(actions[i].type);
        } else {
            assert_legacy_action(actions[i].type);
        }
    }
    return;
}


static void
test_browser_c_only_common_actions_use_c_runtime(void) {
    assert_c_action(NCM_ACTION_SELECT_ITEM);
    assert_c_action(NCM_ACTION_APPLY_FILTER);
    assert_c_action(NCM_ACTION_TOGGLE_DISPLAY_MODE);
    return;
}

static void
test_browser_mixed_migration_binding_uses_c_runtime(void) {
    NcmBindingAction actions[] = {
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_ENTER_DIRECTORY,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_TOGGLE_BROWSER_SORT_MODE,
        },
    };
    NcmBinding binding;

    test_state_reset();
    binding = test_binding(actions, NCM_ARRAY_LEN(actions));

    assert(ncmpcpp_legacy_execute_binding(&binding));
    assert(test_state.can_run_count == 4);
    assert(test_state.can_run_types[0] == NCM_ACTION_ENTER_DIRECTORY);
    assert(test_state.can_run_types[1]
           == NCM_ACTION_TOGGLE_BROWSER_SORT_MODE);
    assert(test_state.can_run_types[2] == NCM_ACTION_ENTER_DIRECTORY);
    assert(test_state.can_run_types[3]
           == NCM_ACTION_TOGGLE_BROWSER_SORT_MODE);
    assert(test_state.run_count == 2);
    assert(test_state.run_types[0] == NCM_ACTION_ENTER_DIRECTORY);
    assert(test_state.run_types[1] == NCM_ACTION_TOGGLE_BROWSER_SORT_MODE);
    assert_no_legacy_dispatch();
    return;
}

int
main(void) {
    test_browser_navigation_binding_uses_c_runtime();
    test_browser_navigation_actions_use_c_runtime();
    test_browser_migration_action_table_matches_dispatch();
    test_browser_c_only_common_actions_use_c_runtime();
    test_browser_mixed_migration_binding_uses_c_runtime();
    exit(EXIT_SUCCESS);
}
