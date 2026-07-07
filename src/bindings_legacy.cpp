#include "bindings_legacy.h"

#include <string>

#include "macro_utilities.h"
#include "screens/screen_cpp_legacy.h"
#include "ui_state.h"

namespace {

NcmActionType to_ncm_action_type(Actions::Type type)
{
    return static_cast<NcmActionType>(type);
}

bool legacy_can_run_action(NcmActionType type, void *)
{
    Actions::BaseAction *action;

    action = Actions::runtimeAction(type);
    if (action == nullptr)
        return false;
    return action->canBeRun();
}

bool legacy_run_action(NcmActionType type, void *)
{
    Actions::BaseAction *action;

    action = Actions::runtimeAction(type);
    if (action == nullptr)
        return false;
    return action->execute();
}

bool legacy_current_screen_is(ScreenType screen_type, void *)
{
    return screenLegacyCurrent()->type() == screen_type;
}

void legacy_push_key(NcKey key, void *)
{
    static_cast<NC::Window *>(ui_state_footer_window())->pushChar(key);
}

bool legacy_run_external_command(char *command, int32 command_len, void *)
{
    std::string text;

    text.assign(command, static_cast<size_t>(command_len));
    runExternalCommand(text, true);
    return true;
}

bool legacy_run_external_console_command(char *command, int32 command_len,
                                         void *)
{
    std::string text;

    text.assign(command, static_cast<size_t>(command_len));
    runExternalConsoleCommand(text);
    return true;
}

} // namespace

NcmBindingRuntime *bindings_legacy_runtime(void)
{
    static NcmBindingRuntime runtime = {
        legacy_can_run_action,
        legacy_run_action,
        legacy_current_screen_is,
        legacy_push_key,
        legacy_run_external_command,
        legacy_run_external_console_command,
        nullptr,
    };

    return &runtime;
}

bool bindings_legacy_execute(NcmBinding *binding)
{
    return ncm_binding_execute_runtime(binding, bindings_legacy_runtime());
}

bool bindings_legacy_has_runnable_action(NcmBinding *binding,
                                         Actions::Type type)
{
    return ncm_binding_has_runnable_action(
        binding, to_ncm_action_type(type), bindings_legacy_runtime());
}
