#if !defined(NCMPCPP_BINDINGS_H)
#define NCMPCPP_BINDINGS_H

#include <stdbool.h>

#include "c/ncm_defs.h"
#include "c/ncm_error.h"
#include "actions.h"
#include "curses/nc_window.h"
#include "screens/screen_type.h"

enum NcmBindingActionKind {
    NCM_BINDING_ACTION_NORMAL,
    NCM_BINDING_ACTION_PUSH_CHARACTERS,
    NCM_BINDING_ACTION_REQUIRE_SCREEN,
    NCM_BINDING_ACTION_REQUIRE_RUNNABLE,
    NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND,
    NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND,
};

typedef struct NcmBindingAction {
    char *argument;
    NcKey *keys;

    int32 argument_len;
    int32 argument_cap;
    int32 keys_len;
    int32 keys_cap;

    enum NcmActionType type;
    enum NcmBindingActionKind kind;
    enum ScreenType screen_type;
} NcmBindingAction;

typedef struct NcmBinding {
    NcmBindingAction *actions;
    int32 actions_len;
    int32 actions_cap;
} NcmBinding;

typedef struct NcmCommand {
    char *name;
    int32 name_len;
    int32 name_cap;
    NcmBinding binding;
    bool immediate;
} NcmCommand;

typedef struct NcmKeyBindings {
    NcmBinding *bindings;
    int32 bindings_len;
    int32 bindings_cap;
    NcKey key;
} NcmKeyBindings;

typedef struct NcmBindingSlice {
    NcmBinding *data;
    int32 len;
} NcmBindingSlice;

typedef bool (*NcmBindingActionRunner)(NcmBindingAction *action,
                                       void *user);

typedef bool (*NcmBindingCanRunActionFn)(enum NcmActionType type,
                                         void *user);
typedef bool (*NcmBindingRunActionFn)(enum NcmActionType type,
                                      void *user);
typedef bool (*NcmBindingCurrentScreenIsFn)(enum ScreenType screen_type,
                                            void *user);
typedef void (*NcmBindingPushKeyFn)(NcKey key, void *user);
typedef bool (*NcmBindingRunExternalCommandFn)(char *command,
                                               int32 command_len,
                                               void *user);

typedef struct NcmBindingRuntime {
    NcmBindingCanRunActionFn can_run_action;
    NcmBindingRunActionFn run_action;
    NcmBindingCurrentScreenIsFn current_screen_is;
    NcmBindingPushKeyFn push_key;
    NcmBindingRunExternalCommandFn run_external_command;
    NcmBindingRunExternalCommandFn run_external_console_command;
    void *user;
} NcmBindingRuntime;

typedef struct NcmBindingsConfiguration {
    NcmCommand *commands;
    NcmKeyBindings *keys;
    int32 commands_len;
    int32 commands_cap;
    int32 keys_len;
    int32 keys_cap;
} NcmBindingsConfiguration;

extern NcmBindingsConfiguration Bindings;

void ncm_binding_action_init(NcmBindingAction *action);
void ncm_binding_action_destroy(NcmBindingAction *action);
bool ncm_binding_action_copy(NcmBindingAction *dest,
                             NcmBindingAction *source);

void ncm_binding_init(NcmBinding *binding);
void ncm_binding_destroy(NcmBinding *binding);
void ncm_binding_clear(NcmBinding *binding);
bool ncm_binding_append_action(NcmBinding *binding,
                               NcmBindingAction *action);
bool ncm_binding_copy(NcmBinding *dest, NcmBinding *source);
bool ncm_binding_is_single(NcmBinding *binding);
bool ncm_binding_action_can_run(NcmBindingAction *action,
                                NcmBindingRuntime *runtime);
bool ncm_binding_action_run(NcmBindingAction *action,
                            NcmBindingRuntime *runtime);
bool ncm_binding_can_execute_runtime(NcmBinding *binding,
                                     NcmBindingRuntime *runtime);
bool ncm_binding_execute_runtime(NcmBinding *binding,
                                 NcmBindingRuntime *runtime);
bool ncm_binding_runtime_can_run_action(enum NcmActionType type,
                                        void *user);
bool ncm_binding_runtime_run_action(enum NcmActionType type,
                                    void *user);
bool ncm_binding_runtime_current_screen_is(enum ScreenType screen_type,
                                           void *user);
void ncm_binding_runtime_push_key(NcKey key, void *user);
bool ncm_binding_runtime_run_external_command(char *command,
                                              int32 command_len,
                                              void *user);
bool ncm_binding_runtime_run_external_console_command(
    char *command, int32 command_len, void *user);
void ncm_binding_runtime_init(NcmBindingRuntime *runtime,
                              NcmActionRuntime *action_runtime);
NcmBindingRuntime *ncm_binding_default_runtime(void);
bool ncm_binding_can_execute_default(NcmBinding *binding);
bool ncm_binding_execute_default(NcmBinding *binding);
bool ncm_binding_is_single_action_type(NcmBinding *binding,
                                       enum NcmActionType type);

void ncm_command_init(NcmCommand *command);
void ncm_command_destroy(NcmCommand *command);

void ncm_key_bindings_init(NcmKeyBindings *key_bindings);
void ncm_key_bindings_destroy(NcmKeyBindings *key_bindings);

void ncm_bindings_configuration_init(NcmBindingsConfiguration *bindings);
void ncm_bindings_configuration_destroy(NcmBindingsConfiguration *bindings);
void ncm_bindings_configuration_clear(NcmBindingsConfiguration *bindings);
bool ncm_bindings_configuration_read(NcmBindingsConfiguration *bindings,
                                     char *path, int32 path_len,
                                     NcmError *error);
void ncm_bindings_configuration_generate_defaults(
    NcmBindingsConfiguration *bindings);
NcmCommand *ncm_bindings_configuration_find_command(
    NcmBindingsConfiguration *bindings, char *name, int32 name_len);
NcmBindingSlice ncm_bindings_configuration_get(
    NcmBindingsConfiguration *bindings, NcKey key);

NcKey ncm_bindings_string_to_key(char *string, int32 string_len);
void ncm_bindings_format_key(NcmBuffer *buffer, NcKey key);
NcKey ncm_read_key(NcWindow *window);
int32 ncm_bindings_key_name(NcKey key, char *buffer, int32 buffer_len);

#endif /* NCMPCPP_BINDINGS_H */
