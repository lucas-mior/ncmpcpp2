#include <assert.h>
#include <stdlib.h>

#include "actions_legacy_runtime.h"
#include "app_legacy_bridge.h"
#include "bindings.h"
#include "settings_legacy_runtime.h"

typedef struct TestState {
    enum NcmActionType can_run_types[64];
    enum NcmActionType run_types[64];
    enum ScreenType current_screen;

    int32 can_run_count;
    int32 run_count;
    int32 legacy_binding_count;
    int32 legacy_action_count;
    int32 unrelated_legacy_count;
} TestState;

static TestState test_state;

static void test_state_reset(enum ScreenType screen_type);
static NcmBinding test_binding(NcmBindingAction *actions,
                               int32 actions_len);
static void assert_no_legacy_dispatch(void);
static void test_tag_editor_core_binding_uses_c_runtime(void);
static void test_tag_editor_screen_binding_uses_c_runtime(void);
static void test_tag_editor_direct_action_uses_c_runtime(void);
static void test_tag_editor_rename_action_uses_c_runtime(void);
static void test_tag_editor_unsupported_binding_is_rejected(void);
static void test_tag_editor_unsupported_action_is_rejected(void);
static void test_show_tag_editor_from_hybrid_uses_c_runtime(void);
static void test_jump_to_tag_editor_from_hybrid_uses_c_runtime(void);
static void test_hybrid_binding_to_tag_editor_uses_c_runtime(void);

enum ScreenType
__wrap_native_c_screens_current_type(void) {
    return test_state.current_screen;
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
test_state_reset(enum ScreenType screen_type) {
    test_state = (TestState){0};
    test_state.current_screen = screen_type;
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
    assert(test_state.legacy_binding_count == 0);
    assert(test_state.legacy_action_count == 0);
    assert(test_state.unrelated_legacy_count == 0);
    return;
}

static void
test_tag_editor_core_binding_uses_c_runtime(void) {
    NcmBindingAction actions[] = {
        {
            .kind = NCM_BINDING_ACTION_REQUIRE_RUNNABLE,
            .type = NCM_ACTION_RUN_ACTION,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_RUN_ACTION,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_JUMP_TO_PARENT_DIRECTORY,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_ENTER_DIRECTORY,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_SAVE_TAG_CHANGES,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_EDIT_DIRECTORY_NAME,
        },
    };
    NcmBinding binding;

    test_state_reset(NCM_SCREEN_TYPE_TAG_EDITOR);
    binding = test_binding(actions, NCM_ARRAY_LEN(actions));

    assert(ncmpcpp_legacy_execute_binding(&binding));
    assert(test_state.can_run_count == 12);
    assert(test_state.can_run_types[0] == NCM_ACTION_RUN_ACTION);
    assert(test_state.can_run_types[1] == NCM_ACTION_RUN_ACTION);
    assert(test_state.run_count == 5);
    assert(test_state.run_types[0] == NCM_ACTION_RUN_ACTION);
    assert(test_state.run_types[1] == NCM_ACTION_JUMP_TO_PARENT_DIRECTORY);
    assert(test_state.run_types[2] == NCM_ACTION_ENTER_DIRECTORY);
    assert(test_state.run_types[3] == NCM_ACTION_SAVE_TAG_CHANGES);
    assert(test_state.run_types[4] == NCM_ACTION_EDIT_DIRECTORY_NAME);
    assert_no_legacy_dispatch();
    return;
}

static void
test_tag_editor_screen_binding_uses_c_runtime(void) {
    NcmBindingAction actions[] = {
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_MOUSE_EVENT,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_NEXT_COLUMN,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_PREVIOUS_COLUMN,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
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
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_APPLY_FILTER,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_FIND_ITEM_FORWARD,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_NEXT_FOUND_ITEM,
        },
    };
    NcmBinding binding;

    test_state_reset(NCM_SCREEN_TYPE_TAG_EDITOR);
    binding = test_binding(actions, NCM_ARRAY_LEN(actions));

    assert(ncmpcpp_legacy_execute_binding(&binding));
    assert(test_state.can_run_count == 18);
    assert(test_state.run_count == 9);
    assert(test_state.run_types[0] == NCM_ACTION_MOUSE_EVENT);
    assert(test_state.run_types[1] == NCM_ACTION_NEXT_COLUMN);
    assert(test_state.run_types[2] == NCM_ACTION_PREVIOUS_COLUMN);
    assert(test_state.run_types[3] == NCM_ACTION_SELECT_ITEM);
    assert(test_state.run_types[4] == NCM_ACTION_REVERSE_SELECTION);
    assert(test_state.run_types[5] == NCM_ACTION_REMOVE_SELECTION);
    assert(test_state.run_types[6] == NCM_ACTION_APPLY_FILTER);
    assert(test_state.run_types[7] == NCM_ACTION_FIND_ITEM_FORWARD);
    assert(test_state.run_types[8] == NCM_ACTION_NEXT_FOUND_ITEM);
    assert_no_legacy_dispatch();
    return;
}

static void
test_tag_editor_direct_action_uses_c_runtime(void) {
    test_state_reset(NCM_SCREEN_TYPE_TAG_EDITOR);

    assert(ncmpcpp_legacy_execute_action(NCM_ACTION_ENTER_DIRECTORY));
    assert(test_state.can_run_count == 0);
    assert(test_state.run_count == 1);
    assert(test_state.run_types[0] == NCM_ACTION_ENTER_DIRECTORY);
    assert_no_legacy_dispatch();
    return;
}

static void
test_tag_editor_rename_action_uses_c_runtime(void) {
    test_state_reset(NCM_SCREEN_TYPE_TAG_EDITOR);

    assert(ncmpcpp_legacy_execute_action(NCM_ACTION_EDIT_DIRECTORY_NAME));
    assert(test_state.can_run_count == 0);
    assert(test_state.run_count == 1);
    assert(test_state.run_types[0] == NCM_ACTION_EDIT_DIRECTORY_NAME);
    assert_no_legacy_dispatch();
    return;
}

static void
test_tag_editor_unsupported_binding_is_rejected(void) {
    NcmBindingAction action = {
        .kind = NCM_BINDING_ACTION_NORMAL,
        .type = NCM_ACTION_DELETE_BROWSER_ITEMS,
    };
    NcmBinding binding;

    test_state_reset(NCM_SCREEN_TYPE_TAG_EDITOR);
    binding = test_binding(&action, 1);

    assert(!ncmpcpp_legacy_execute_binding(&binding));
    assert(test_state.can_run_count == 0);
    assert(test_state.run_count == 0);
    assert_no_legacy_dispatch();
    return;
}

static void
test_tag_editor_unsupported_action_is_rejected(void) {
    test_state_reset(NCM_SCREEN_TYPE_TAG_EDITOR);

    assert(!ncmpcpp_legacy_execute_action(NCM_ACTION_DELETE_BROWSER_ITEMS));
    assert(test_state.can_run_count == 0);
    assert(test_state.run_count == 0);
    assert_no_legacy_dispatch();
    return;
}

static void
test_show_tag_editor_from_hybrid_uses_c_runtime(void) {
    test_state_reset(NCM_SCREEN_TYPE_BROWSER);

    assert(ncmpcpp_legacy_execute_action(NCM_ACTION_SHOW_TAG_EDITOR));
    assert(test_state.can_run_count == 0);
    assert(test_state.run_count == 1);
    assert(test_state.run_types[0] == NCM_ACTION_SHOW_TAG_EDITOR);
    assert_no_legacy_dispatch();
    return;
}

static void
test_jump_to_tag_editor_from_hybrid_uses_c_runtime(void) {
    test_state_reset(NCM_SCREEN_TYPE_BROWSER);

    assert(ncmpcpp_legacy_execute_action(NCM_ACTION_JUMP_TO_TAG_EDITOR));
    assert(test_state.can_run_count == 0);
    assert(test_state.run_count == 1);
    assert(test_state.run_types[0] == NCM_ACTION_JUMP_TO_TAG_EDITOR);
    assert_no_legacy_dispatch();
    return;
}

static void
test_hybrid_binding_to_tag_editor_uses_c_runtime(void) {
    NcmBindingAction actions[] = {
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_SHOW_TAG_EDITOR,
        },
        {
            .kind = NCM_BINDING_ACTION_NORMAL,
            .type = NCM_ACTION_JUMP_TO_TAG_EDITOR,
        },
    };
    NcmBinding binding;

    test_state_reset(NCM_SCREEN_TYPE_BROWSER);
    binding = test_binding(actions, NCM_ARRAY_LEN(actions));

    assert(ncmpcpp_legacy_execute_binding(&binding));
    assert(test_state.can_run_count == 4);
    assert(test_state.run_count == 2);
    assert(test_state.run_types[0] == NCM_ACTION_SHOW_TAG_EDITOR);
    assert(test_state.run_types[1] == NCM_ACTION_JUMP_TO_TAG_EDITOR);
    assert_no_legacy_dispatch();
    return;
}

int
main(void) {
#if defined(HAVE_TAGLIB_H)
    test_tag_editor_core_binding_uses_c_runtime();
    test_tag_editor_screen_binding_uses_c_runtime();
    test_tag_editor_direct_action_uses_c_runtime();
    test_tag_editor_rename_action_uses_c_runtime();
    test_tag_editor_unsupported_binding_is_rejected();
    test_tag_editor_unsupported_action_is_rejected();
    test_show_tag_editor_from_hybrid_uses_c_runtime();
    test_jump_to_tag_editor_from_hybrid_uses_c_runtime();
    test_hybrid_binding_to_tag_editor_uses_c_runtime();
#endif
    exit(EXIT_SUCCESS);
}
