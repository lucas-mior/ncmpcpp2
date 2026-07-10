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

int
main(void) {
    test_sort_actions_are_c_safe();
    exit(EXIT_SUCCESS);
}
