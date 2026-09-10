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

typedef struct NcmBindingActionArgument {
    char *data;
    int32 len;
    int32 cap;
} NcmBindingActionArgument;

typedef struct NcmBindingActionKeys {
    NcKey *data;
    int32 len;
} NcmBindingActionKeys;

typedef struct NcmBindingAction {
    enum NcmBindingActionKind kind;
    union {
        enum ActionType type;
        enum ScreenType screen_type;
        NcmBindingActionArgument argument;
        NcmBindingActionKeys keys;
    } value;
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

void binding_action_init(NcmBindingAction *);
void binding_action_destroy(NcmBindingAction *);

void binding_destroy(NcmBinding *);
void binding_clear(NcmBinding *);
void binding_append_action(NcmBinding *, NcmBindingAction *);
void binding_copy(NcmBinding *dest, NcmBinding *source);
bool binding_action_can_run(NcmBindingAction *, NcmBindingRuntime *);
bool binding_runtime_can_run_action(enum ActionType, void *);
int32 binding_runtime_run_action(enum ActionType, void *);
bool binding_runtime_current_screen_is(enum ScreenType, void *);
void binding_runtime_push_key(NcKey, void *);
int32 binding_runtime_run_external_command(char *, int32, void *);
int32 binding_runtime_run_external_console_command(char *, int32, void *);
NcmBindingRuntime *binding_default_runtime(void);
bool binding_can_execute_default(NcmBinding *);
int32 binding_execute_default(NcmBinding *);
bool binding_is_single_action_type(NcmBinding *, enum ActionType);

void ncm_command_destroy(NcmCommand *);

void ncm_key_bindings_init(NcmKeyBindings *);

void bindings_config_destroy(NcmBindingsConfiguration *);
void bindings_config_clear(NcmBindingsConfiguration *);
int32 bindings_config_read(NcmBindingsConfiguration *,
                               char *, int32, NcmError *);
void bindings_config_generate_defaults(NcmBindingsConfiguration *);
NcmCommand *bindings_config_find_command(NcmBindingsConfiguration *,
                                             char *, int32);
int32 bindings_config_get(NcmBindingsConfiguration *, NcKey,
                              NcmBindingSlice *);

NcKey ncm_read_key(NcWindow *);
int32 bindings_key_name(NcKey, char *, int32);

#endif /* BINDINGS_H */
