#include "app_binding_migration.h"

bool
app_binding_migration_action_is_c_safe(enum NcmActionType type) {
    switch (type) {
    case NCM_ACTION_DUMMY:
    case NCM_ACTION_QUIT:
    case NCM_ACTION_VOLUME_UP:
    case NCM_ACTION_VOLUME_DOWN:
    case NCM_ACTION_TOGGLE_REPEAT:
    case NCM_ACTION_TOGGLE_RANDOM:
    case NCM_ACTION_TOGGLE_SINGLE:
    case NCM_ACTION_TOGGLE_CONSUME:
    case NCM_ACTION_NEXT:
    case NCM_ACTION_PREVIOUS:
    case NCM_ACTION_PAUSE:
    case NCM_ACTION_PLAY:
    case NCM_ACTION_STOP:
    case NCM_ACTION_SEEK_FORWARD:
    case NCM_ACTION_SEEK_BACKWARD:
        return true;
    default:
        return false;
    }
}

bool
app_binding_migration_binding_is_c_safe(NcmBinding *binding) {
    if (binding == NULL) {
        return false;
    }

    for (int32 i = 0; i < binding->actions_len; i += 1) {
        NcmBindingAction *action;

        action = binding->actions + i;
        switch (action->kind) {
        case NCM_BINDING_ACTION_NORMAL:
        case NCM_BINDING_ACTION_REQUIRE_RUNNABLE:
            if (!app_binding_migration_action_is_c_safe(action->type)) {
                return false;
            }
            break;
        default:
            return false;
        }
    }

    return true;
}
