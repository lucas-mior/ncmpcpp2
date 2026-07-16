#include <assert.h>
#include <stdlib.h>

#include "app_binding_migration.h"
#include "cbase/base_macros.h"

static void
test_sort_actions_are_c_safe(void) {
    enum NcmActionType types[] = {
        NCM_ACTION_SORT_PLAYLIST,
        NCM_ACTION_MOVE_SORT_ORDER_UP,
        NCM_ACTION_MOVE_SORT_ORDER_DOWN,
        NCM_ACTION_RUN_ACTION,
    };
    NcmBindingAction actions[LENGTH(types)] = {0};
    NcmBinding binding = {
        .actions = actions,
        .actions_len = LENGTH(actions),
        .actions_cap = LENGTH(actions),
    };

    for (int32 i = 0; i < LENGTH(types); i += 1) {
        actions[i].kind = NCM_BINDING_ACTION_NORMAL;
        actions[i].type = types[i];
        assert(app_binding_migration_action_is_c_safe(types[i]));
    }

    assert(app_binding_migration_binding_is_c_safe(&binding));
    assert(app_binding_migration_binding_is_hybrid_safe(&binding));
    assert(app_binding_migration_binding_is_plain_action_sequence(
        &binding));
    return;
}

static void
test_navigation_is_c_safe_on_native_screens(void) {
    enum NcmActionType types[] = {
        NCM_ACTION_SCROLL_UP,
        NCM_ACTION_SCROLL_DOWN,
        NCM_ACTION_PAGE_UP,
        NCM_ACTION_PAGE_DOWN,
        NCM_ACTION_MOVE_HOME,
        NCM_ACTION_MOVE_END,
    };
    NcmBindingAction actions[LENGTH(types)] = {0};
    NcmBinding binding = {
        .actions = actions,
        .actions_len = LENGTH(actions),
        .actions_cap = LENGTH(actions),
    };

    assert(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG));
    assert(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_PLAYLIST));

    for (int32 i = 0; i < LENGTH(types); i += 1) {
        actions[i].kind = NCM_BINDING_ACTION_NORMAL;
        actions[i].type = types[i];
        assert(app_binding_migration_action_is_c_safe(types[i]));
        assert(app_binding_migration_action_is_c_safe_for_screen(
            types[i], NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG));
        assert(app_binding_migration_action_is_c_safe_for_screen(
            types[i], NCM_SCREEN_TYPE_PLAYLIST));
    }

    assert(app_binding_migration_binding_is_c_safe(&binding));
    assert(app_binding_migration_binding_is_c_safe_for_screen(
        &binding, NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG));
    assert(app_binding_migration_binding_is_c_safe_for_screen(
        &binding, NCM_SCREEN_TYPE_PLAYLIST));
    return;
}

static void
test_all_actions_are_c_safe_on_native_dialog(void) {
    NcmBindingAction action = {
        .kind = NCM_BINDING_ACTION_NORMAL,
        .type = NCM_ACTION_MOVE_SELECTED_ITEMS_TO,
    };
    NcmBinding binding = {
        .actions = &action,
        .actions_len = 1,
        .actions_cap = 1,
    };

    assert(app_binding_migration_action_is_c_safe_for_screen(
        action.type, NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG));
    assert(app_binding_migration_binding_is_c_safe_for_screen(
        &binding, NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG));
    return;
}

int
main(void) {
    test_sort_actions_are_c_safe();
    test_navigation_is_c_safe_on_native_screens();
    test_all_actions_are_c_safe_on_native_dialog();
    exit(EXIT_SUCCESS);
}
