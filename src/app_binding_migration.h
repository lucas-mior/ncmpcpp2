#if !defined(NCMPCPP_APP_BINDING_MIGRATION_H)
#define NCMPCPP_APP_BINDING_MIGRATION_H

#include <stdbool.h>

#include "actions.h"
#include "bindings.h"

#if defined(__cplusplus)
extern "C" {
#endif

bool app_binding_migration_action_is_c_safe(enum NcmActionType type);
bool app_binding_migration_screen_is_c_only(enum ScreenType type);
bool app_binding_migration_action_is_c_safe_for_screen(
    enum NcmActionType type, enum ScreenType screen_type);
bool app_binding_migration_binding_is_c_safe(NcmBinding *binding);
bool app_binding_migration_binding_is_c_safe_for_screen(
    NcmBinding *binding, enum ScreenType screen_type);
bool app_binding_migration_binding_is_plain_action_sequence(
    NcmBinding *binding);
bool app_binding_migration_binding_is_hybrid_safe(NcmBinding *binding);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_APP_BINDING_MIGRATION_H */
