#if !defined(BINDINGS_H)
#define BINDINGS_H

#include "cbase.h"

#include "actions.h"
#include "c/ncm_c.h"
#include "curses/nc_curses.h"
#include "screens/nc_screens.h"

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

    enum ActionType type;
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

typedef int32 NcmBindingActionRunner(NcmBindingAction *action, void *user);
typedef bool NcmBindingCanRunActionFn(enum ActionType type, void *user);
typedef int32 NcmBindingRunActionFn(enum ActionType type, void *user);
typedef bool NcmBindingCurrentScreenIsFn(enum ScreenType, void *user);
typedef void NcmBindingPushKeyFn(NcKey key, void *user);
typedef int32 NcmBindingRunExternalCommandFn(char *, int32, void *user);

typedef struct NcmBindingRuntime {
    NcmBindingCanRunActionFn *can_run_action;
    NcmBindingRunActionFn *run_action;
    NcmBindingCurrentScreenIsFn *current_screen_is;
    NcmBindingPushKeyFn *push_key;
    NcmBindingRunExternalCommandFn *run_external_command;
    NcmBindingRunExternalCommandFn *run_external_console_command;
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

void ncm_binding_action_init(NcmBindingAction *);
void ncm_binding_action_destroy(NcmBindingAction *);

void ncm_binding_destroy(NcmBinding *);
void ncm_binding_clear(NcmBinding *);
void ncm_binding_append_action(NcmBinding *, NcmBindingAction *);
void ncm_binding_copy(NcmBinding *dest, NcmBinding *source);
bool ncm_binding_action_can_run(NcmBindingAction *, NcmBindingRuntime *);
bool ncm_binding_runtime_can_run_action(enum ActionType, void *);
int32 ncm_binding_runtime_run_action(enum ActionType, void *);
bool ncm_binding_runtime_current_screen_is(enum ScreenType, void *);
void ncm_binding_runtime_push_key(NcKey, void *);
int32 ncm_binding_runtime_run_external_command(char *, int32, void *);
int32 ncm_binding_runtime_run_external_console_command(char *, int32, void *);
NcmBindingRuntime *ncm_binding_default_runtime(void);
bool ncm_binding_can_execute_default(NcmBinding *);
int32 ncm_binding_execute_default(NcmBinding *);
bool ncm_binding_is_single_action_type(NcmBinding *, enum ActionType);

void ncm_command_destroy(NcmCommand *);

void ncm_key_bindings_init(NcmKeyBindings *);

void ncm_bindings_config_destroy(NcmBindingsConfiguration *);
void ncm_bindings_config_clear(NcmBindingsConfiguration *);
int32 ncm_bindings_config_read(NcmBindingsConfiguration *,
                               char *, int32, NcmError *);
void ncm_bindings_config_generate_defaults(NcmBindingsConfiguration *);
NcmCommand *ncm_bindings_config_find_command(NcmBindingsConfiguration *,
                                             char *, int32);
int32 ncm_bindings_config_get(NcmBindingsConfiguration *, NcKey,
                              NcmBindingSlice *);

NcKey ncm_bindings_string_to_key(char *, int32);
NcKey ncm_read_key(NcWindow *);
int32 ncm_bindings_key_name(NcKey, char *, int32);

#endif /* BINDINGS_H */
