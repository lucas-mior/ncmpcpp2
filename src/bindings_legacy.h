#if !defined(NCMPCPP_BINDINGS_LEGACY_H)
#define NCMPCPP_BINDINGS_LEGACY_H

#include "actions_legacy.h"
#include "bindings.h"

NcmBindingRuntime *bindings_legacy_runtime(void);
bool bindings_legacy_execute(NcmBinding *binding);
bool bindings_legacy_has_runnable_action(NcmBinding *binding,
                                          Actions::Type type);

#endif /* NCMPCPP_BINDINGS_LEGACY_H */
