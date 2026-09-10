#if !defined(BINDINGS_H)
#define BINDINGS_H

#include "cbase.h"

#include "actions.h"
#include "c/ncm_c.h"
#include "curses/nc_curses.h"
#include "screens/nc_screens.h"

enum BindingActionKind {
    BINDING_ACTION_NORMAL,
    BINDING_ACTION_PUSH_CHARACTERS,
    BINDING_ACTION_REQUIRE_SCREEN,
    BINDING_ACTION_REQUIRE_RUNNABLE,
    BINDING_ACTION_RUN_EXTERNAL_COMMAND,
    BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND,
};

typedef struct BindingActionArgument {
    char *data;
    int32 len;
    int32 cap;
} BindingActionArgument;

typedef struct BindingActionKeys {
    NcKey *data;
    int32 len;
} BindingActionKeys;

typedef struct BindingAction {
    enum BindingActionKind kind;
    union {
        enum ActionType type;
        enum ScreenType screen_type;
        BindingActionArgument argument;
        BindingActionKeys keys;
    } value;
} BindingAction;

typedef struct Binding {
    BindingAction *actions;
    int32 actions_len;
    int32 actions_cap;
} Binding;

typedef struct NcmCommand {
    char *name;
    int32 name_len;
    int32 name_cap;
    Binding binding;
    bool immediate;
} NcmCommand;

typedef struct NcmKeyBindings {
    Binding *bindings;
    int32 bindings_len;
    int32 bindings_cap;
    NcKey key;
} NcmKeyBindings;

typedef struct BindingSlice {
    Binding *data;
    int32 len;
} BindingSlice;

typedef int32 BindingActionRunner(BindingAction *action, void *user);
typedef bool BindingCanRunActionFn(enum ActionType type, void *user);
typedef int32 BindingRunActionFn(enum ActionType type, void *user);
typedef bool BindingCurrentScreenIsFn(enum ScreenType, void *user);
typedef void BindingPushKeyFn(NcKey key, void *user);
typedef int32 BindingRunExternalCommandFn(char *, int32, void *user);

typedef struct BindingRuntime {
    BindingCanRunActionFn *can_run_action;
    BindingRunActionFn *run_action;
    BindingCurrentScreenIsFn *current_screen_is;
    BindingPushKeyFn *push_key;
    BindingRunExternalCommandFn *run_external_command;
    BindingRunExternalCommandFn *run_external_console_command;
    void *user;
} BindingRuntime;

typedef struct BindingsConfiguration {
    NcmCommand *commands;
    NcmKeyBindings *keys;
    int32 commands_len;
    int32 commands_cap;
    int32 keys_len;
    int32 keys_cap;
} BindingsConfiguration;

extern BindingsConfiguration Bindings;

void binding_action_init(BindingAction *);
void binding_action_destroy(BindingAction *);

void binding_destroy(Binding *);
void binding_clear(Binding *);
void binding_append_action(Binding *, BindingAction *);
void binding_copy(Binding *dest, Binding *source);
bool binding_action_can_run(BindingAction *, BindingRuntime *);
bool binding_runtime_can_run_action(enum ActionType, void *);
int32 binding_runtime_run_action(enum ActionType, void *);
bool binding_runtime_current_screen_is(enum ScreenType, void *);
void binding_runtime_push_key(NcKey, void *);
int32 binding_runtime_run_external_command(char *, int32, void *);
int32 binding_runtime_run_external_console_command(char *, int32, void *);
BindingRuntime *binding_default_runtime(void);
bool binding_can_execute_default(Binding *);
int32 binding_execute_default(Binding *);
bool binding_is_single_action_type(Binding *, enum ActionType);

void ncm_command_destroy(NcmCommand *);

void ncm_key_bindings_init(NcmKeyBindings *);

void bindings_config_destroy(BindingsConfiguration *);
void bindings_config_clear(BindingsConfiguration *);
int32 bindings_config_read(BindingsConfiguration *,
                               char *, int32, NcmError *);
void bindings_config_generate_defaults(BindingsConfiguration *);
NcmCommand *bindings_config_find_command(BindingsConfiguration *,
                                             char *, int32);
int32 bindings_config_get(BindingsConfiguration *, NcKey,
                              BindingSlice *);

NcKey ncm_read_key(NcWindow *);
int32 bindings_key_name(NcKey, char *, int32);

#endif /* BINDINGS_H */
