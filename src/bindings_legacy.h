#if !defined(NCMPCPP_BINDINGS_LEGACY_H)
#define NCMPCPP_BINDINGS_LEGACY_H

#include <string>
#include <vector>

#include "actions.h"
#include "bindings.h"
#include "curses/window.h"

NcKey readKey(NC::Window &window);

bool bindings_legacy_read_paths(const std::vector<std::string> &paths);
void bindings_legacy_generate_defaults(void);
NcmCommand *bindings_legacy_find_command(const std::string &name);
NcmBindingSlice bindings_legacy_get(NcKey key);
bool bindings_legacy_execute(NcmBinding *binding);
bool bindings_legacy_has_runnable_action(NcmBinding *binding,
                                          Actions::Type type);
bool bindings_legacy_is_single_action(NcmBinding *binding,
                                       Actions::Type type);
std::string bindings_legacy_action_name(NcmBindingAction *action);

#endif /* NCMPCPP_BINDINGS_LEGACY_H */
