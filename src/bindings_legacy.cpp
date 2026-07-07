#include "bindings_legacy.h"

#include <cassert>

#include "macro_utilities.h"
#include "c/ncm_base.h"
#include "screens/screen_legacy.h"
#include "ui_state_legacy.h"

namespace {

NcmActionType to_ncm_action_type(Actions::Type type)
{
    return static_cast<NcmActionType>(type);
}

Actions::BaseAction *legacy_action(NcmBindingAction *action)
{
    if (action->kind == NCM_BINDING_ACTION_NORMAL)
        return Actions::runtimeAction(action->type);
    if (action->kind == NCM_BINDING_ACTION_REQUIRE_RUNNABLE)
        return Actions::runtimeAction(action->type);
    return nullptr;
}

bool legacy_can_run(NcmBindingAction *action)
{
    Actions::BaseAction *runtime_action;

    switch (action->kind)
    {
    case NCM_BINDING_ACTION_NORMAL:
        runtime_action = legacy_action(action);
        assert(runtime_action != nullptr);
        return runtime_action->canBeRun();
    case NCM_BINDING_ACTION_PUSH_CHARACTERS:
        return true;
    case NCM_BINDING_ACTION_REQUIRE_SCREEN:
        return screenLegacyCurrent()->type() == action->screen_type;
    case NCM_BINDING_ACTION_REQUIRE_RUNNABLE:
        runtime_action = legacy_action(action);
        assert(runtime_action != nullptr);
        return runtime_action->canBeRun();
    case NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND:
    case NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND:
        return true;
    }
    return false;
}

bool legacy_run(NcmBindingAction *action, void *)
{
    Actions::BaseAction *runtime_action;
    std::string command;

    if (!legacy_can_run(action))
        return false;

    switch (action->kind)
    {
    case NCM_BINDING_ACTION_NORMAL:
        runtime_action = legacy_action(action);
        assert(runtime_action != nullptr);
        return runtime_action->execute();
    case NCM_BINDING_ACTION_PUSH_CHARACTERS:
        for (int32 i = 0; i < action->keys_len; i += 1)
            ui_state_legacy_footer_window()->pushChar(action->keys[i]);
        return true;
    case NCM_BINDING_ACTION_REQUIRE_SCREEN:
    case NCM_BINDING_ACTION_REQUIRE_RUNNABLE:
        return true;
    case NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND:
        command.assign(action->argument,
                       static_cast<size_t>(action->argument_len));
        runExternalCommand(command, true);
        return true;
    case NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND:
        command.assign(action->argument,
                       static_cast<size_t>(action->argument_len));
        runExternalConsoleCommand(command);
        return true;
    }
    return false;
}

} // namespace

bool bindings_legacy_read_paths(const std::vector<std::string> &paths)
{
    std::vector<char *> c_paths;
    std::vector<int32> c_path_lens;
    NcmError error;

    c_paths.reserve(paths.size());
    c_path_lens.reserve(paths.size());
    for (const auto &path : paths)
    {
        c_paths.push_back(const_cast<char *>(path.data()));
        c_path_lens.push_back(static_cast<int32>(path.size()));
    }

    ncm_error_clear(&error);
    return ncm_bindings_configuration_read_paths(
        &Bindings,
        c_paths.empty() ? nullptr : c_paths.data(),
        c_path_lens.empty() ? nullptr : c_path_lens.data(),
        static_cast<int32>(c_paths.size()),
        &error);
}

void bindings_legacy_generate_defaults(void)
{
    ncm_bindings_configuration_generate_defaults(&Bindings);
}

NcmCommand *bindings_legacy_find_command(const std::string &name)
{
    return ncm_bindings_configuration_find_command(
        &Bindings,
        const_cast<char *>(name.data()),
        static_cast<int32>(name.size()));
}

NcmBindingSlice bindings_legacy_get(NcKey key)
{
    return ncm_bindings_configuration_get(&Bindings, key);
}

bool bindings_legacy_execute(NcmBinding *binding)
{
    for (int32 i = 0; i < binding->actions_len; i += 1)
    {
        if (!legacy_run(binding->actions + i, nullptr))
            return false;
    }
    return true;
}

bool bindings_legacy_has_runnable_action(NcmBinding *binding,
                                         Actions::Type type)
{
    bool success = false;

    for (int32 i = 0; i < binding->actions_len; i += 1)
    {
        NcmBindingAction *action = binding->actions + i;

        if (!legacy_can_run(action))
            return false;
        if ((action->kind == NCM_BINDING_ACTION_NORMAL)
            && (action->type == to_ncm_action_type(type)))
            success = true;
    }
    return success;
}

bool bindings_legacy_is_single_action(NcmBinding *binding,
                                      Actions::Type type)
{
    if (!ncm_binding_is_single(binding))
        return false;
    if (binding->actions[0].kind != NCM_BINDING_ACTION_NORMAL)
        return false;
    return binding->actions[0].type == to_ncm_action_type(type);
}

std::string bindings_legacy_action_name(NcmBindingAction *action)
{
    std::string result;

    switch (action->kind)
    {
    case NCM_BINDING_ACTION_NORMAL:
        result = ncm_action_type_name(action->type);
        break;
    case NCM_BINDING_ACTION_PUSH_CHARACTERS:
        result = "push_characters \"";
        for (int32 i = 0; i < action->keys_len; i += 1)
        {
            NcmBuffer key_name;

            if (i > 0)
                result += ", ";
            ncm_buffer_init(&key_name);
            ncm_bindings_format_key(&key_name, action->keys[i]);
            result.append(key_name.data,
                          static_cast<size_t>(key_name.len));
            ncm_buffer_destroy(&key_name);
        }
        result += "\"";
        break;
    case NCM_BINDING_ACTION_REQUIRE_SCREEN:
        result = "require_screen \"";
        result += screenTypeToString(action->screen_type);
        result += "\"";
        break;
    case NCM_BINDING_ACTION_REQUIRE_RUNNABLE:
        result = "require_runnable \"";
        result += ncm_action_type_name(action->type);
        result += "\"";
        break;
    case NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND:
        result.assign("run_external_command \"",
                      sizeof("run_external_command \"") - 1);
        result.append(action->argument,
                      static_cast<size_t>(action->argument_len));
        result += "\"";
        break;
    case NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND:
        result.assign("run_external_console_command \"",
                      sizeof("run_external_console_command \"") - 1);
        result.append(action->argument,
                      static_cast<size_t>(action->argument_len));
        result += "\"";
        break;
    }
    return result;
}
