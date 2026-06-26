#if !defined(NCMPCPP_APP_BINDING_MIGRATION_C)
#define NCMPCPP_APP_BINDING_MIGRATION_C

#include "app_binding_migration.h"

static bool
app_binding_migration_action_kind_is_c_safe(NcmBindingAction *action) {
    if (action == NULL) {
        return false;
    }

    switch (action->kind) {
    case NCM_BINDING_ACTION_NORMAL:
    case NCM_BINDING_ACTION_REQUIRE_RUNNABLE:
        return app_binding_migration_action_is_c_safe(action->type);
    case NCM_BINDING_ACTION_PUSH_CHARACTERS:
    case NCM_BINDING_ACTION_REQUIRE_SCREEN:
    case NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND:
    case NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND:
        return true;
    default:
        break;
    }

    return false;
}

bool
app_binding_migration_action_is_c_safe(enum NcmActionType type) {
    return (type >= NCM_ACTION_DUMMY) && (type < NCM_ACTION_LAST);
}

bool
app_binding_migration_screen_is_c_only(enum ScreenType type) {
    switch (type) {
    case NCM_SCREEN_TYPE_BROWSER:
    case NCM_SCREEN_TYPE_HELP:
    case NCM_SCREEN_TYPE_LASTFM:
    case NCM_SCREEN_TYPE_LYRICS:
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
#if defined(ENABLE_OUTPUTS)
    case NCM_SCREEN_TYPE_OUTPUTS:
#endif
    case NCM_SCREEN_TYPE_PLAYLIST:
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
    case NCM_SCREEN_TYPE_SERVER_INFO:
    case NCM_SCREEN_TYPE_SONG_INFO:
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
#endif
#if defined(ENABLE_VISUALIZER)
    case NCM_SCREEN_TYPE_VISUALIZER:
#endif
        return true;
    case NCM_SCREEN_TYPE_UNKNOWN:
    case NCM_SCREEN_TYPE_LAST:
        return false;
    default:
        break;
    }

    return false;
}

bool
app_binding_migration_action_is_c_safe_for_screen(
    enum NcmActionType type, enum ScreenType screen_type) {
    if (!app_binding_migration_screen_is_c_only(screen_type)) {
        return false;
    }
    return app_binding_migration_action_is_c_safe(type);
}

bool
app_binding_migration_binding_is_c_safe(NcmBinding *binding) {
    if (binding == NULL) {
        return false;
    }
    if (binding->actions_len <= 0) {
        return false;
    }

    for (int32 i = 0; i < binding->actions_len; i += 1) {
        if (!app_binding_migration_action_kind_is_c_safe(
                binding->actions + i)) {
            return false;
        }
    }

    return true;
}

bool
app_binding_migration_binding_is_c_safe_for_screen(
    NcmBinding *binding, enum ScreenType screen_type) {
    if (!app_binding_migration_screen_is_c_only(screen_type)) {
        return false;
    }
    return app_binding_migration_binding_is_c_safe(binding);
}

#endif /* NCMPCPP_APP_BINDING_MIGRATION_C */
