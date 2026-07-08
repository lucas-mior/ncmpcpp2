#include "bindings.h"

#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "app_controller.h"
#include "c/ncm_base.h"
#include "c/ncm_macro_utilities.h"
#include "screens/screen_type.h"
#include "ui_state.h"

#include "cbase/base_macros.h"

#define NCM_BINDINGS_ERROR_PARSE 1
#define NCM_BINDINGS_ERROR_MEMORY 2

NcmBindingsConfiguration Bindings;

static void ncm_bindings_error(NcmError *error, char *format, ...)
    __attribute__((format(printf, 2, 3)));
static int32 ncm_string_len(char *string);
static char *ncm_string_copy(char *string, int32 string_len, int32 *cap);
static bool ncm_string_equal(char *left, int32 left_len, char *right,
                             int32 right_len);
static int32 ncm_trim_start(char *string, int32 string_len);
static int32 ncm_trim_end(char *string, int32 string_len);
static bool ncm_extract_enclosed(char *line, int32 line_len,
                                 char open, char close,
                                 NcmStringView *result);
static bool ncm_parse_action_line(char *line, int32 line_len,
                                  NcmBindingAction *result,
                                  NcmError *error);
static bool ncm_binding_append_normal(NcmBinding *binding,
                                      enum NcmActionType type);
static bool ncm_bindings_bind(NcmBindingsConfiguration *bindings,
                              NcKey key, NcmBinding *binding);
static bool ncm_bindings_bind_single(NcmBindingsConfiguration *bindings,
                                     char *key_name,
                                     enum NcmActionType type);
static bool ncm_bindings_bind_chain2(NcmBindingsConfiguration *bindings,
                                     char *key_name,
                                     enum NcmActionType first,
                                     enum NcmActionType second);
static bool ncm_bindings_bind_group(NcmBindingsConfiguration *bindings,
                                    char *key_name,
                                    enum NcmActionType *actions,
                                    int32 actions_len);
static bool ncm_bindings_not_bound(NcmBindingsConfiguration *bindings,
                                   NcKey key);
static int32 ncm_bindings_command_lower_bound(
    NcmBindingsConfiguration *bindings, char *name, int32 name_len,
    bool *found);
static int32 ncm_bindings_key_lower_bound(NcmBindingsConfiguration *bindings,
                                          NcKey key, bool *found);
static bool ncm_bindings_insert_command(NcmBindingsConfiguration *bindings,
                                        NcmCommand *command,
                                        NcmError *error);
static bool ncm_bindings_finalize_definition(
    NcmBindingsConfiguration *bindings, int32 in_progress,
    NcmBinding *actions, NcKey key, char *key_name, int32 key_name_len,
    char *command_name, int32 command_name_len, bool command_immediate,
    NcmError *error);

static void
ncm_bindings_error(NcmError *error, char *format, ...) {
    va_list args;
    char buffer[256];
    int32 len;

    va_start(args, format);
    len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len < 0) {
        ncm_error_set(error, NCM_BINDINGS_ERROR_PARSE,
                      STRLIT_ARGS("bindings parse error"));
        return;
    }
    if (len >= (int32)sizeof(buffer)) {
        len = (int32)sizeof(buffer) - 1;
    }

    ncm_error_set(error, NCM_BINDINGS_ERROR_PARSE, buffer, len);
    return;
}

static int32
ncm_string_len(char *string) {
    int32 len;

    len = 0;
    while (string[len] != '\0') {
        len += 1;
    }
    return len;
}

static char *
ncm_string_copy(char *string, int32 string_len, int32 *cap) {
    char *result;

    result = ncm_malloc(string_len + 1);
    if (string_len > 0) {
        ncm_memcpy(result, string, string_len);
    }
    result[string_len] = '\0';
    *cap = string_len + 1;
    return result;
}

static bool
ncm_string_equal(char *left, int32 left_len, char *right,
                 int32 right_len) {
    if (left_len != right_len) {
        return false;
    }
    if (left_len == 0) {
        return true;
    }
    return memcmp(left, right, (size_t)left_len) == 0;
}

static int32
ncm_trim_start(char *string, int32 string_len) {
    int32 result;

    result = 0;
    while (result < string_len) {
        unsigned char c;

        c = (unsigned char)string[result];
        if (!isspace(c)) {
            break;
        }
        result += 1;
    }
    return result;
}

static int32
ncm_trim_end(char *string, int32 string_len) {
    while (string_len > 0) {
        unsigned char c;

        c = (unsigned char)string[string_len - 1];
        if (!isspace(c)) {
            break;
        }
        string_len -= 1;
    }
    return string_len;
}

static bool
ncm_extract_enclosed(char *line, int32 line_len, char open, char close,
                    NcmStringView *result) {
    int32 start;
    int32 end;

    result->data = NULL;
    result->len = 0;
    start = -1;
    end = -1;
    for (int32 i = 0; i < line_len; i += 1) {
        if ((start < 0) && (line[i] == open)) {
            start = i + 1;
            continue;
        }
        if ((start >= 0) && (line[i] == close)) {
            end = i;
            break;
        }
    }
    if ((start < 0) || (end < start)) {
        return false;
    }

    result->data = line + start;
    result->len = end - start;
    return true;
}

void
ncm_binding_action_init(NcmBindingAction *action) {
    action->argument = NULL;
    action->keys = NULL;
    action->argument_len = 0;
    action->argument_cap = 0;
    action->keys_len = 0;
    action->keys_cap = 0;
    action->type = NCM_ACTION_DUMMY;
    action->kind = NCM_BINDING_ACTION_NORMAL;
    action->screen_type = NCM_SCREEN_TYPE_UNKNOWN;
    return;
}

void
ncm_binding_action_destroy(NcmBindingAction *action) {
    if (action->argument) {
        ncm_free(action->argument, action->argument_cap);
    }
    if (action->keys) {
        ncm_free(action->keys, action->keys_cap*SIZEOF(*action->keys));
    }
    ncm_binding_action_init(action);
    return;
}

bool
ncm_binding_action_copy(NcmBindingAction *dest, NcmBindingAction *source) {
    ncm_binding_action_init(dest);
    dest->type = source->type;
    dest->kind = source->kind;
    dest->screen_type = source->screen_type;

    if (source->argument_len > 0) {
        dest->argument = ncm_string_copy(source->argument,
                                        source->argument_len,
                                        &dest->argument_cap);
        dest->argument_len = source->argument_len;
    }
    if (source->keys_len > 0) {
        dest->keys_cap = source->keys_len;
        dest->keys = ncm_malloc(dest->keys_cap*SIZEOF(*dest->keys));
        ncm_memcpy(dest->keys, source->keys,
                   source->keys_len*SIZEOF(*dest->keys));
        dest->keys_len = source->keys_len;
    }
    return true;
}

void
ncm_binding_init(NcmBinding *binding) {
    binding->actions = NULL;
    binding->actions_len = 0;
    binding->actions_cap = 0;
    return;
}

void
ncm_binding_destroy(NcmBinding *binding) {
    ncm_binding_clear(binding);
    if (binding->actions) {
        ncm_free(binding->actions,
                 binding->actions_cap*SIZEOF(*binding->actions));
    }
    ncm_binding_init(binding);
    return;
}

void
ncm_binding_clear(NcmBinding *binding) {
    for (int32 i = 0; i < binding->actions_len; i += 1) {
        ncm_binding_action_destroy(binding->actions + i);
    }
    binding->actions_len = 0;
    return;
}

bool
ncm_binding_append_action(NcmBinding *binding, NcmBindingAction *action) {
    NcmBindingAction copy;

    if (binding->actions_len == binding->actions_cap) {
        int32 new_cap;

        if (binding->actions_cap == 0) {
            new_cap = 4;
        } else {
            new_cap = binding->actions_cap*2;
        }
        binding->actions = ncm_realloc_array(binding->actions,
                                             binding->actions_cap,
                                             new_cap,
                                             SIZEOF(*binding->actions));
        binding->actions_cap = new_cap;
    }

    if (!ncm_binding_action_copy(&copy, action)) {
        return false;
    }
    binding->actions[binding->actions_len++] = copy;
    return true;
}

bool
ncm_binding_copy(NcmBinding *dest, NcmBinding *source) {
    ncm_binding_init(dest);
    for (int32 i = 0; i < source->actions_len; i += 1) {
        if (!ncm_binding_append_action(dest, source->actions + i)) {
            ncm_binding_destroy(dest);
            return false;
        }
    }
    return true;
}

bool
ncm_binding_is_single(NcmBinding *binding) {
    if (binding == NULL) {
        return false;
    }
    return binding->actions_len == 1;
}

bool
ncm_binding_execute(NcmBinding *binding,
                    NcmBindingActionRunner runner, void *user) {
    if (runner == NULL) {
        return false;
    }

    for (int32 i = 0; i < binding->actions_len; i += 1) {
        if (!runner(binding->actions + i, user)) {
            return false;
        }
    }

    return true;
}

bool
ncm_binding_action_can_run(NcmBindingAction *action,
                           NcmBindingRuntime *runtime) {
    if (action == NULL) {
        return false;
    }

    switch (action->kind) {
    case NCM_BINDING_ACTION_NORMAL:
    case NCM_BINDING_ACTION_REQUIRE_RUNNABLE:
        if ((runtime != NULL) && (runtime->can_run_action != NULL)) {
            return runtime->can_run_action(action->type, runtime->user);
        }
        return ncm_action_can_run(action->type, NULL);
    case NCM_BINDING_ACTION_PUSH_CHARACTERS:
        return true;
    case NCM_BINDING_ACTION_REQUIRE_SCREEN:
        if ((runtime == NULL) || (runtime->current_screen_is == NULL)) {
            return false;
        }
        return runtime->current_screen_is(action->screen_type, runtime->user);
    case NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND:
    case NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND:
        return true;
    }

    return false;
}

bool
ncm_binding_action_run(NcmBindingAction *action,
                       NcmBindingRuntime *runtime) {
    if (!ncm_binding_action_can_run(action, runtime)) {
        return false;
    }

    switch (action->kind) {
    case NCM_BINDING_ACTION_NORMAL:
        if ((runtime != NULL) && (runtime->run_action != NULL)) {
            return runtime->run_action(action->type, runtime->user);
        }
        return ncm_action_run(action->type, NULL);
    case NCM_BINDING_ACTION_PUSH_CHARACTERS:
        if ((runtime == NULL) || (runtime->push_key == NULL)) {
            return false;
        }
        for (int32 i = 0; i < action->keys_len; i += 1) {
            runtime->push_key(action->keys[i], runtime->user);
        }
        return true;
    case NCM_BINDING_ACTION_REQUIRE_SCREEN:
    case NCM_BINDING_ACTION_REQUIRE_RUNNABLE:
        return true;
    case NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND:
        if ((runtime == NULL)
            || (runtime->run_external_command == NULL)) {
            return false;
        }
        return runtime->run_external_command(action->argument,
                                             action->argument_len,
                                             runtime->user);
    case NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND:
        if ((runtime == NULL)
            || (runtime->run_external_console_command == NULL)) {
            return false;
        }
        return runtime->run_external_console_command(action->argument,
                                                     action->argument_len,
                                                     runtime->user);
    }

    return false;
}

bool
ncm_binding_can_execute_runtime(NcmBinding *binding,
                                NcmBindingRuntime *runtime) {
    if (binding == NULL) {
        return false;
    }

    for (int32 i = 0; i < binding->actions_len; i += 1) {
        if (!ncm_binding_action_can_run(binding->actions + i, runtime)) {
            return false;
        }
    }

    return true;
}

bool
ncm_binding_execute_runtime(NcmBinding *binding,
                            NcmBindingRuntime *runtime) {
    if (binding == NULL) {
        return false;
    }

    for (int32 i = 0; i < binding->actions_len; i += 1) {
        if (!ncm_binding_action_run(binding->actions + i, runtime)) {
            return false;
        }
    }

    return true;
}

bool
ncm_binding_has_runnable_action(NcmBinding *binding,
                                enum NcmActionType type,
                                NcmBindingRuntime *runtime) {
    bool success;

    if (binding == NULL) {
        return false;
    }

    success = false;
    for (int32 i = 0; i < binding->actions_len; i += 1) {
        NcmBindingAction *action;

        action = binding->actions + i;
        if (!ncm_binding_action_can_run(action, runtime)) {
            return false;
        }
        if ((action->kind == NCM_BINDING_ACTION_NORMAL)
            && (action->type == type)) {
            success = true;
        }
    }

    return success;
}

bool
ncm_binding_runtime_can_run_action(enum NcmActionType type,
                                   void *user) {
    return ncm_action_runtime_can_run(user, type);
}

bool
ncm_binding_runtime_run_action(enum NcmActionType type, void *user) {
    return ncm_action_runtime_run(user, type);
}

bool
ncm_binding_runtime_current_screen_is(enum ScreenType screen_type,
                                      void *user) {
    NcScreen *screen;
    int32 native_type;

    (void)user;
    screen = app_controller_current_screen();
    if (screen == NULL) {
        return false;
    }

    native_type = screen_type_to_native_type(screen_type);
    return nc_screen_type(screen) == native_type;
}

void
ncm_binding_runtime_push_key(NcKey key, void *user) {
    NcWindow *window;

    (void)user;
    window = ui_state_footer_window();
    if (window != NULL) {
        nc_window_push_key(window, key);
    }
    return;
}

bool
ncm_binding_runtime_run_external_command(char *command,
                                         int32 command_len,
                                         void *user) {
    NcmError error;

    (void)user;
    ncm_error_clear(&error);
    return ncm_macro_run_external_command(command, command_len, true,
                                          &error);
}

bool
ncm_binding_runtime_run_external_console_command(char *command,
                                                 int32 command_len,
                                                 void *user) {
    NcmError error;

    (void)user;
    ncm_error_clear(&error);
    return ncm_macro_run_external_console_command(command, command_len,
                                                  &error);
}

void
ncm_binding_runtime_init(NcmBindingRuntime *runtime,
                         NcmActionRuntime *action_runtime) {
    if (runtime == NULL) {
        return;
    }

    runtime->can_run_action = ncm_binding_runtime_can_run_action;
    runtime->run_action = ncm_binding_runtime_run_action;
    runtime->current_screen_is = ncm_binding_runtime_current_screen_is;
    runtime->push_key = ncm_binding_runtime_push_key;
    runtime->run_external_command =
        ncm_binding_runtime_run_external_command;
    runtime->run_external_console_command =
        ncm_binding_runtime_run_external_console_command;
    runtime->user = action_runtime;
    return;
}

NcmBindingRuntime *
ncm_binding_default_runtime(void) {
    static NcmBindingRuntime runtime;
    static bool initialized;

    if (!initialized) {
        ncm_binding_runtime_init(&runtime, ncm_action_runtime_global());
        initialized = true;
    }
    return &runtime;
}

bool
ncm_binding_can_execute_default(NcmBinding *binding) {
    return ncm_binding_can_execute_runtime(binding,
                                           ncm_binding_default_runtime());
}

bool
ncm_binding_execute_default(NcmBinding *binding) {
    return ncm_binding_execute_runtime(binding,
                                       ncm_binding_default_runtime());
}

bool
ncm_binding_has_runnable_action_default(NcmBinding *binding,
                                        enum NcmActionType type) {
    return ncm_binding_has_runnable_action(binding, type,
                                           ncm_binding_default_runtime());
}

bool
ncm_binding_is_single_action_type(NcmBinding *binding,
                                  enum NcmActionType type) {
    if (!ncm_binding_is_single(binding)) {
        return false;
    }
    if (binding->actions[0].kind != NCM_BINDING_ACTION_NORMAL) {
        return false;
    }
    return binding->actions[0].type == type;
}

void
ncm_binding_action_format(NcmBuffer *buffer,
                          NcmBindingAction *action) {
    if ((buffer == NULL) || (action == NULL)) {
        return;
    }

    switch (action->kind) {
    case NCM_BINDING_ACTION_NORMAL:
        ncm_buffer_append(buffer, ncm_action_type_name(action->type),
                          ncm_string_len(
                              ncm_action_type_name(action->type)));
        break;
    case NCM_BINDING_ACTION_PUSH_CHARACTERS:
        ncm_buffer_append(buffer, STRLIT_ARGS("push_characters \""));
        for (int32 i = 0; i < action->keys_len; i += 1) {
            if (i > 0) {
                ncm_buffer_append(buffer, STRLIT_ARGS(", "));
            }
            ncm_bindings_format_key(buffer, action->keys[i]);
        }
        ncm_buffer_append_byte(buffer, '"');
        break;
    case NCM_BINDING_ACTION_REQUIRE_SCREEN:
        ncm_buffer_append(buffer, STRLIT_ARGS("require_screen \""));
        ncm_buffer_append(buffer, screen_type_str(action->screen_type),
                          ncm_string_len(
                              screen_type_str(action->screen_type)));
        ncm_buffer_append_byte(buffer, '"');
        break;
    case NCM_BINDING_ACTION_REQUIRE_RUNNABLE:
        ncm_buffer_append(buffer, STRLIT_ARGS("require_runnable \""));
        ncm_buffer_append(buffer, ncm_action_type_name(action->type),
                          ncm_string_len(
                              ncm_action_type_name(action->type)));
        ncm_buffer_append_byte(buffer, '"');
        break;
    case NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND:
        ncm_buffer_append(buffer,
                          STRLIT_ARGS("run_external_command \""));
        ncm_buffer_append(buffer, action->argument, action->argument_len);
        ncm_buffer_append_byte(buffer, '"');
        break;
    case NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND:
        ncm_buffer_append(buffer,
                          STRLIT_ARGS(
                              "run_external_console_command \""));
        ncm_buffer_append(buffer, action->argument, action->argument_len);
        ncm_buffer_append_byte(buffer, '"');
        break;
    }

    return;
}

void
ncm_command_init(NcmCommand *command) {
    command->name = NULL;
    command->name_len = 0;
    command->name_cap = 0;
    command->immediate = false;
    ncm_binding_init(&command->binding);
    return;
}

void
ncm_command_destroy(NcmCommand *command) {
    if (command->name) {
        ncm_free(command->name, command->name_cap);
    }
    ncm_binding_destroy(&command->binding);
    ncm_command_init(command);
    return;
}

void
ncm_key_bindings_init(NcmKeyBindings *key_bindings) {
    key_bindings->bindings = NULL;
    key_bindings->bindings_len = 0;
    key_bindings->bindings_cap = 0;
    key_bindings->key = NC_KEY_NONE;
    return;
}

void
ncm_key_bindings_destroy(NcmKeyBindings *key_bindings) {
    for (int32 i = 0; i < key_bindings->bindings_len; i += 1) {
        ncm_binding_destroy(key_bindings->bindings + i);
    }
    if (key_bindings->bindings) {
        ncm_free(key_bindings->bindings,
                 key_bindings->bindings_cap*SIZEOF(*key_bindings->bindings));
    }
    ncm_key_bindings_init(key_bindings);
    return;
}

void
ncm_bindings_configuration_init(NcmBindingsConfiguration *bindings) {
    bindings->commands = NULL;
    bindings->keys = NULL;
    bindings->commands_len = 0;
    bindings->commands_cap = 0;
    bindings->keys_len = 0;
    bindings->keys_cap = 0;
    return;
}

void
ncm_bindings_configuration_destroy(NcmBindingsConfiguration *bindings) {
    ncm_bindings_configuration_clear(bindings);
    if (bindings->commands) {
        ncm_free(bindings->commands,
                 bindings->commands_cap*SIZEOF(*bindings->commands));
    }
    if (bindings->keys) {
        ncm_free(bindings->keys,
                 bindings->keys_cap*SIZEOF(*bindings->keys));
    }
    ncm_bindings_configuration_init(bindings);
    return;
}

void
ncm_bindings_configuration_clear(NcmBindingsConfiguration *bindings) {
    for (int32 i = 0; i < bindings->commands_len; i += 1) {
        ncm_command_destroy(bindings->commands + i);
    }
    for (int32 i = 0; i < bindings->keys_len; i += 1) {
        ncm_key_bindings_destroy(bindings->keys + i);
    }
    bindings->commands_len = 0;
    bindings->keys_len = 0;
    return;
}

NcKey
ncm_bindings_string_to_key(char *string, int32 string_len) {
    NcKey result;

    result = NC_KEY_NONE;
    if ((string_len == 6)
        && ncm_string_equal(string, 4, STRLIT_ARGS("ctrl"))
        && (string[4] == '-')) {
        char c;

        c = string[5];
        if ((c >= 'a') && (c <= 'z')) {
            result = NC_KEY_CTRL_A + (NcKey)(c - 'a');
        } else if (c == '[') {
            result = NC_KEY_CTRL_LEFT_BRACKET;
        } else if (c == '\\') {
            result = NC_KEY_CTRL_BACKSLASH;
        } else if (c == ']') {
            result = NC_KEY_CTRL_RIGHT_BRACKET;
        } else if (c == '^') {
            result = NC_KEY_CTRL_CARET;
        } else if (c == '_') {
            result = NC_KEY_CTRL_UNDERSCORE;
        }
    } else if ((string_len > 4)
               && ncm_string_equal(string, 3, STRLIT_ARGS("alt"))
               && (string[3] == '-')) {
        result = ncm_bindings_string_to_key(string + 4, string_len - 4);
        if (result != NC_KEY_NONE) {
            result |= NC_KEY_ALT;
        }
    } else if ((string_len > 5)
               && ncm_string_equal(string, 4, STRLIT_ARGS("ctrl"))
               && (string[4] == '-')) {
        result = ncm_bindings_string_to_key(string + 5, string_len - 5);
        if (result != NC_KEY_NONE) {
            result |= NC_KEY_CTRL;
        }
    } else if ((string_len > 6)
               && ncm_string_equal(string, 5, STRLIT_ARGS("shift"))
               && (string[5] == '-')) {
        result = ncm_bindings_string_to_key(string + 6, string_len - 6);
        if (result != NC_KEY_NONE) {
            result |= NC_KEY_SHIFT;
        }
    } else if (ncm_string_equal(string, string_len, STRLIT_ARGS("escape"))) {
        result = NC_KEY_ESCAPE;
    } else if (ncm_string_equal(string, string_len, STRLIT_ARGS("mouse"))) {
        result = NC_KEY_MOUSE;
    } else if (ncm_string_equal(string, string_len, STRLIT_ARGS("up"))) {
        result = NC_KEY_UP;
    } else if (ncm_string_equal(string, string_len, STRLIT_ARGS("down"))) {
        result = NC_KEY_DOWN;
    } else if (ncm_string_equal(string, string_len, STRLIT_ARGS("page_up"))) {
        result = NC_KEY_PAGE_UP;
    } else if (ncm_string_equal(string, string_len, STRLIT_ARGS("page_down"))) {
        result = NC_KEY_PAGE_DOWN;
    } else if (ncm_string_equal(string, string_len, STRLIT_ARGS("home"))) {
        result = NC_KEY_HOME;
    } else if (ncm_string_equal(string, string_len, STRLIT_ARGS("end"))) {
        result = NC_KEY_END;
    } else if (ncm_string_equal(string, string_len, STRLIT_ARGS("space"))) {
        result = NC_KEY_SPACE;
    } else if (ncm_string_equal(string, string_len, STRLIT_ARGS("enter"))) {
        result = NC_KEY_ENTER;
    } else if (ncm_string_equal(string, string_len, STRLIT_ARGS("insert"))) {
        result = NC_KEY_INSERT;
    } else if (ncm_string_equal(string, string_len, STRLIT_ARGS("delete"))) {
        result = NC_KEY_DELETE;
    } else if (ncm_string_equal(string, string_len, STRLIT_ARGS("left"))) {
        result = NC_KEY_LEFT;
    } else if (ncm_string_equal(string, string_len, STRLIT_ARGS("right"))) {
        result = NC_KEY_RIGHT;
    } else if (ncm_string_equal(string, string_len, STRLIT_ARGS("tab"))) {
        result = NC_KEY_TAB;
    } else if (ncm_string_equal(string, string_len, STRLIT_ARGS("backspace"))) {
        result = NC_KEY_BACKSPACE;
    } else if ((string_len >= 2) && (string_len <= 3) && (string[0] == 'f')) {
        int32 n;
        char buffer[4];

        ncm_memcpy(buffer, string + 1, string_len - 1);
        buffer[string_len - 1] = '\0';
        n = atoi(buffer);
        if ((n >= 1) && (n <= 12)) {
            result = NC_KEY_F1 + (NcKey)n - 1;
        }
    }

    if ((result == NC_KEY_NONE) && (string_len > 0)) {
        wchar_t wc;
        mbstate_t state;
        int64 converted;

        memset(&state, 0, sizeof(state));
        converted = (int64)mbrtowc(&wc, string, (size_t)string_len, &state);
        if ((converted == string_len) && (wc != 0)) {
            result = (NcKey)wc;
        }
    }

    return result;
}

void
ncm_bindings_format_key(NcmBuffer *buffer, NcKey key) {
    char name[64];
    int32 name_len;

    name_len = nc_key_name(key, name, (int32)SIZEOF(name));
    if (name_len < 0) {
        return;
    }

    ncm_buffer_append(buffer, name, name_len);
    return;
}

NcKey
ncm_read_key(NcWindow *window) {
    NcKey result;
    NcmBuffer tmp;
    bool alt_pressed;

    result = NC_KEY_NONE;
    alt_pressed = false;
    ncm_buffer_init(&tmp);

    while (true) {
        NcKey input;

        input = nc_window_read_key(window);
        if (input == NC_KEY_NONE) {
            break;
        }
        if (input & NC_KEY_ALT) {
            alt_pressed = true;
            input &= ~NC_KEY_ALT;
        }
        if (input > 255) {
            result = input;
            break;
        }

        ncm_buffer_append_byte(&tmp, (char)input);
        result = ncm_bindings_string_to_key(tmp.data, tmp.len);
        if (result != NC_KEY_NONE) {
            break;
        }
        if (tmp.len >= (int32)MB_CUR_MAX) {
            break;
        }
    }

    if (alt_pressed && (result != NC_KEY_NONE)) {
        result |= NC_KEY_ALT;
    }
    ncm_buffer_destroy(&tmp);
    return result;
}

NcKey
ncm_bindings_read_key(NcWindow *window) {
    return ncm_read_key(window);
}

int32
ncm_bindings_key_name(NcKey key, char *buffer, int32 buffer_len) {
    NcmBuffer key_name;
    int32 result;

    if ((buffer == NULL) || (buffer_len <= 0)) {
        return -1;
    }

    ncm_buffer_init(&key_name);
    ncm_bindings_format_key(&key_name, key);
    if (key_name.len >= buffer_len) {
        result = -1;
    } else {
        result = key_name.len;
        if (key_name.len > 0) {
            ncm_memcpy(buffer, key_name.data, key_name.len);
        }
        buffer[key_name.len] = '\0';
    }
    ncm_buffer_destroy(&key_name);
    return result;
}

static bool
ncm_parse_action_line(char *line, int32 line_len,
                      NcmBindingAction *result, NcmError *error) {
    int32 name_len;
    NcmStringView argument;

    ncm_binding_action_init(result);
    line_len = ncm_trim_end(line, line_len);
    name_len = 0;
    while ((name_len < line_len)
           && !isspace((unsigned char)line[name_len])) {
        name_len += 1;
    }

    if (ncm_string_equal(line, name_len,
                         STRLIT_ARGS("set_visualizer_sample_multiplier"))) {
        result->kind = NCM_BINDING_ACTION_NORMAL;
        result->type = NCM_ACTION_DUMMY;
        return true;
    }

    if (name_len == line_len) {
        if (!ncm_action_type_parse(line, name_len, &result->type)) {
            ncm_bindings_error(error, "unknown action: '%.*s'",
                               name_len, line);
            return false;
        }
        result->kind = NCM_BINDING_ACTION_NORMAL;
        return true;
    }

    if (!ncm_extract_enclosed(line + name_len, line_len - name_len,
                              '"', '"', &argument)) {
        ncm_bindings_error(error, "missing quoted argument: '%.*s'",
                           line_len, line);
        return false;
    }

    if (ncm_string_equal(line, name_len, STRLIT_ARGS("push_character"))) {
        NcKey key;

        key = ncm_bindings_string_to_key(argument.data, argument.len);
        if (key == NC_KEY_NONE) {
            ncm_bindings_error(error,
                               "invalid character passed to "
                               "push_character: '%.*s'",
                               argument.len, argument.data);
            return false;
        }
        result->kind = NCM_BINDING_ACTION_PUSH_CHARACTERS;
        result->keys_cap = 1;
        result->keys_len = 1;
        result->keys = ncm_malloc(SIZEOF(*result->keys));
        result->keys[0] = key;
        return true;
    }

    if (ncm_string_equal(line, name_len, STRLIT_ARGS("push_characters"))) {
        if (argument.len <= 0) {
            ncm_bindings_error(error,
                               (char *)"empty argument passed to "
                               "push_characters");
            return false;
        }
        result->kind = NCM_BINDING_ACTION_PUSH_CHARACTERS;
        result->keys_cap = argument.len;
        result->keys_len = argument.len;
        result->keys = ncm_malloc(result->keys_cap*SIZEOF(*result->keys));
        for (int32 i = 0; i < argument.len; i += 1) {
            result->keys[i] = (NcKey)(unsigned char)argument.data[i];
        }
        return true;
    }

    if (ncm_string_equal(line, name_len, STRLIT_ARGS("require_screen"))) {
        if (!screen_type_parse(argument.data, argument.len,
                               &result->screen_type)) {
            ncm_bindings_error(error,
                               "unknown screen passed to "
                               "require_screen: '%.*s'",
                               argument.len, argument.data);
            return false;
        }
        result->kind = NCM_BINDING_ACTION_REQUIRE_SCREEN;
        return true;
    }

    if (ncm_string_equal(line, name_len, STRLIT_ARGS("require_runnable"))) {
        if (!ncm_action_type_parse(argument.data, argument.len,
                                   &result->type)) {
            ncm_bindings_error(error,
                               "unknown action passed to "
                               "require_runnable: '%.*s'",
                               argument.len, argument.data);
            return false;
        }
        result->kind = NCM_BINDING_ACTION_REQUIRE_RUNNABLE;
        return true;
    }

    if (ncm_string_equal(line, name_len,
                         STRLIT_ARGS("run_external_command"))) {
        if (argument.len <= 0) {
            ncm_bindings_error(error,
                               (char *)"empty command passed to "
                               "run_external_command");
            return false;
        }
        result->kind = NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND;
        result->argument = ncm_string_copy(argument.data, argument.len,
                                           &result->argument_cap);
        result->argument_len = argument.len;
        return true;
    }

    if (ncm_string_equal(line, name_len,
                         STRLIT_ARGS("run_external_console_command"))) {
        if (argument.len <= 0) {
            ncm_bindings_error(error,
                               (char *)"empty command passed to "
                               "run_external_console_command");
            return false;
        }
        result->kind = NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND;
        result->argument = ncm_string_copy(argument.data, argument.len,
                                           &result->argument_cap);
        result->argument_len = argument.len;
        return true;
    }

    ncm_bindings_error(error, "unknown action: '%.*s'", line_len, line);
    return false;
}

static bool
ncm_binding_append_normal(NcmBinding *binding, enum NcmActionType type) {
    NcmBindingAction action;
    bool result;

    ncm_binding_action_init(&action);
    action.kind = NCM_BINDING_ACTION_NORMAL;
    action.type = type;
    result = ncm_binding_append_action(binding, &action);
    ncm_binding_action_destroy(&action);
    return result;
}

static int32
ncm_bindings_command_lower_bound(NcmBindingsConfiguration *bindings,
                                 char *name, int32 name_len,
                                 bool *found) {
    int32 first;
    int32 count;

    first = 0;
    count = bindings->commands_len;
    *found = false;
    while (count > 0) {
        int32 step;
        int32 mid;
        int32 cmp;
        int32 min_len;

        step = count/2;
        mid = first + step;
        min_len = bindings->commands[mid].name_len;
        if (name_len < min_len) {
            min_len = name_len;
        }
        cmp = memcmp(bindings->commands[mid].name, name, (size_t)min_len);
        if (cmp == 0) {
            if (bindings->commands[mid].name_len < name_len) {
                cmp = -1;
            } else if (bindings->commands[mid].name_len > name_len) {
                cmp = 1;
            }
        }
        if (cmp < 0) {
            first = mid + 1;
            count -= step + 1;
        } else {
            count = step;
        }
    }
    if ((first < bindings->commands_len)
        && ncm_string_equal(bindings->commands[first].name,
                            bindings->commands[first].name_len,
                            name, name_len)) {
        *found = true;
    }
    return first;
}

static int32
ncm_bindings_key_lower_bound(NcmBindingsConfiguration *bindings,
                             NcKey key, bool *found) {
    int32 first;
    int32 count;

    first = 0;
    count = bindings->keys_len;
    *found = false;
    while (count > 0) {
        int32 step;
        int32 mid;

        step = count/2;
        mid = first + step;
        if (bindings->keys[mid].key < key) {
            first = mid + 1;
            count -= step + 1;
        } else {
            count = step;
        }
    }
    if ((first < bindings->keys_len) && (bindings->keys[first].key == key)) {
        *found = true;
    }
    return first;
}

static bool
ncm_bindings_insert_command(NcmBindingsConfiguration *bindings,
                            NcmCommand *command, NcmError *error) {
    bool found;
    int32 at;
    NcmCommand copy;

    at = ncm_bindings_command_lower_bound(bindings, command->name,
                                          command->name_len, &found);
    if (found) {
        ncm_bindings_error(error, "redefinition of command '%.*s'",
                           command->name_len, command->name);
        return false;
    }

    if (bindings->commands_len == bindings->commands_cap) {
        int32 new_cap;

        if (bindings->commands_cap == 0) {
            new_cap = 8;
        } else {
            new_cap = bindings->commands_cap*2;
        }
        bindings->commands = ncm_realloc_array(bindings->commands,
                                               bindings->commands_cap,
                                               new_cap,
                                               SIZEOF(*bindings->commands));
        bindings->commands_cap = new_cap;
    }

    ncm_command_init(&copy);
    copy.name = ncm_string_copy(command->name, command->name_len,
                                &copy.name_cap);
    copy.name_len = command->name_len;
    copy.immediate = command->immediate;
    if (!ncm_binding_copy(&copy.binding, &command->binding)) {
        ncm_command_destroy(&copy);
        return false;
    }

    if (at < bindings->commands_len) {
        ncm_memmove(bindings->commands + at + 1, bindings->commands + at,
                    (bindings->commands_len - at)*SIZEOF(*bindings->commands));
    }
    bindings->commands[at] = copy;
    bindings->commands_len += 1;
    return true;
}

static bool
ncm_bindings_bind(NcmBindingsConfiguration *bindings, NcKey key,
                  NcmBinding *binding) {
    bool found;
    int32 at;
    NcmKeyBindings *key_bindings;
    NcmBinding copy;

    at = ncm_bindings_key_lower_bound(bindings, key, &found);
    if (!found) {
        NcmKeyBindings item;

        if (bindings->keys_len == bindings->keys_cap) {
            int32 new_cap;

            if (bindings->keys_cap == 0) {
                new_cap = 16;
            } else {
                new_cap = bindings->keys_cap*2;
            }
            bindings->keys = ncm_realloc_array(bindings->keys,
                                               bindings->keys_cap,
                                               new_cap,
                                               SIZEOF(*bindings->keys));
            bindings->keys_cap = new_cap;
        }
        if (at < bindings->keys_len) {
            ncm_memmove(bindings->keys + at + 1, bindings->keys + at,
                        (bindings->keys_len - at)*SIZEOF(*bindings->keys));
        }
        ncm_key_bindings_init(&item);
        item.key = key;
        bindings->keys[at] = item;
        bindings->keys_len += 1;
    }

    key_bindings = bindings->keys + at;
    if (key_bindings->bindings_len == key_bindings->bindings_cap) {
        int32 new_cap;

        if (key_bindings->bindings_cap == 0) {
            new_cap = 2;
        } else {
            new_cap = key_bindings->bindings_cap*2;
        }
        key_bindings->bindings = ncm_realloc_array(
            key_bindings->bindings, key_bindings->bindings_cap, new_cap,
            SIZEOF(*key_bindings->bindings));
        key_bindings->bindings_cap = new_cap;
    }

    if (!ncm_binding_copy(&copy, binding)) {
        return false;
    }
    key_bindings->bindings[key_bindings->bindings_len++] = copy;
    return true;
}

static bool
ncm_bindings_not_bound(NcmBindingsConfiguration *bindings, NcKey key) {
    bool found;

    if (key == NC_KEY_NONE) {
        return false;
    }
    (void)ncm_bindings_key_lower_bound(bindings, key, &found);
    return !found;
}

static bool
ncm_bindings_bind_single(NcmBindingsConfiguration *bindings, char *key_name,
                         enum NcmActionType type) {
    NcmBinding binding;
    NcKey key;
    bool result;

    key = ncm_bindings_string_to_key(key_name, ncm_string_len(key_name));
    if (!ncm_bindings_not_bound(bindings, key)) {
        return true;
    }
    ncm_binding_init(&binding);
    result = ncm_binding_append_normal(&binding, type);
    if (result) {
        result = ncm_bindings_bind(bindings, key, &binding);
    }
    ncm_binding_destroy(&binding);
    return result;
}

static bool
ncm_bindings_bind_chain2(NcmBindingsConfiguration *bindings, char *key_name,
                         enum NcmActionType first,
                         enum NcmActionType second) {
    NcmBinding binding;
    NcKey key;
    bool result;

    key = ncm_bindings_string_to_key(key_name, ncm_string_len(key_name));
    if (!ncm_bindings_not_bound(bindings, key)) {
        return true;
    }
    ncm_binding_init(&binding);
    result = ncm_binding_append_normal(&binding, first);
    if (result) {
        result = ncm_binding_append_normal(&binding, second);
    }
    if (result) {
        result = ncm_bindings_bind(bindings, key, &binding);
    }
    ncm_binding_destroy(&binding);
    return result;
}
static bool
ncm_bindings_bind_group(NcmBindingsConfiguration *bindings, char *key_name,
                        enum NcmActionType *actions, int32 actions_len) {
    NcKey key;

    key = ncm_bindings_string_to_key(key_name, ncm_string_len(key_name));
    if (!ncm_bindings_not_bound(bindings, key)) {
        return true;
    }

    for (int32 i = 0; i < actions_len; i += 1) {
        NcmBinding binding;
        bool result;

        ncm_binding_init(&binding);
        result = ncm_binding_append_normal(&binding, actions[i]);
        if (result) {
            result = ncm_bindings_bind(bindings, key, &binding);
        }
        ncm_binding_destroy(&binding);
        if (!result) {
            return false;
        }
    }
    return true;
}


static bool
ncm_bindings_finalize_definition(NcmBindingsConfiguration *bindings,
                                 int32 in_progress, NcmBinding *actions,
                                 NcKey key, char *key_name,
                                 int32 key_name_len, char *command_name,
                                 int32 command_name_len,
                                 bool command_immediate,
                                 NcmError *error) {
    NcmCommand command;
    bool result;

    if (in_progress == 0) {
        return true;
    }
    if (actions->actions_len == 0) {
        if (in_progress == 1) {
            ncm_bindings_error(error,
                               "definition of command '%.*s' cannot be empty",
                               command_name_len, command_name);
        } else {
            ncm_bindings_error(error,
                               "definition of key '%.*s' cannot be empty",
                               key_name_len, key_name);
        }
        return false;
    }

    if (in_progress == 1) {
        ncm_command_init(&command);
        command.name = command_name;
        command.name_len = command_name_len;
        command.name_cap = command_name_len + 1;
        command.immediate = command_immediate;
        if (!ncm_binding_copy(&command.binding, actions)) {
            return false;
        }
        result = ncm_bindings_insert_command(bindings, &command, error);
        ncm_binding_destroy(&command.binding);
        return result;
    }

    return ncm_bindings_bind(bindings, key, actions);
}

bool
ncm_bindings_configuration_read(NcmBindingsConfiguration *bindings,
                                char *path, int32 path_len,
                                NcmError *error) {
    enum {
        IN_PROGRESS_NONE = 0,
        IN_PROGRESS_COMMAND = 1,
        IN_PROGRESS_KEY = 2,
    };
    FILE *file;
    char *path_copy;
    int32 path_cap;
    char *line;
    size_t line_cap;
    int32 in_progress;
    int32 line_no;
    bool ok;
    NcmBinding actions;
    char *command_name;
    char *key_name;
    int32 command_name_len;
    int32 key_name_len;
    int32 command_name_cap;
    int32 key_name_cap;
    NcKey key;
    bool command_immediate;

    path_copy = ncm_string_copy(path, path_len, &path_cap);
    file = fopen(path_copy, "r");
    if (file == NULL) {
        ncm_free(path_copy, path_cap);
        return true;
    }

    line = NULL;
    line_cap = 0;
    in_progress = IN_PROGRESS_NONE;
    line_no = 0;
    ok = true;
    command_name = NULL;
    key_name = NULL;
    command_name_len = 0;
    key_name_len = 0;
    command_name_cap = 0;
    key_name_cap = 0;
    key = NC_KEY_NONE;
    command_immediate = false;
    ncm_binding_init(&actions);

    while (ok && (getline(&line, &line_cap, file) >= 0)) {
        int32 len;
        int32 start;
        NcmStringView enclosed;

        line_no += 1;
        len = ncm_string_len(line);
        len = ncm_trim_end(line, len);
        if ((len == 0) || (line[0] == '#')) {
            continue;
        }
        start = ncm_trim_start(line, len);

        if ((len - start >= 11)
            && ncm_string_equal(line + start, 11,
                                STRLIT_ARGS("def_command"))) {
            ok = ncm_bindings_finalize_definition(
                bindings, in_progress, &actions, key, key_name,
                key_name_len, command_name, command_name_len,
                command_immediate, error);
            ncm_binding_clear(&actions);
            in_progress = IN_PROGRESS_NONE;
            if (!ok) {
                break;
            }
            if (!ncm_extract_enclosed(line + start, len - start,
                                      '"', '"', &enclosed)) {
                ncm_bindings_error(error,
                                   "%.*s:%d: command must have non-empty name",
                                   path_len, path, line_no);
                ok = false;
                break;
            }
            if (enclosed.len <= 0) {
                ncm_bindings_error(error,
                                   "%.*s:%d: command must have non-empty name",
                                   path_len, path, line_no);
                ok = false;
                break;
            }
            if (command_name) {
                ncm_free(command_name, command_name_cap);
            }
            command_name = ncm_string_copy(enclosed.data, enclosed.len,
                                           &command_name_cap);
            command_name_len = enclosed.len;
            if (!ncm_extract_enclosed(line + start, len - start,
                                      '[', ']', &enclosed)) {
                ncm_bindings_error(error,
                                   "%.*s:%d: missing command type",
                                   path_len, path, line_no);
                ok = false;
                break;
            }
            if (ncm_string_equal(enclosed.data, enclosed.len,
                                 STRLIT_ARGS("immediate"))) {
                command_immediate = true;
            } else if (ncm_string_equal(enclosed.data, enclosed.len,
                                        STRLIT_ARGS("deferred"))) {
                command_immediate = false;
            } else {
                ncm_bindings_error(error,
                                   "%.*s:%d: invalid command type '%.*s'",
                                   path_len, path, line_no,
                                   enclosed.len, enclosed.data);
                ok = false;
                break;
            }
            in_progress = IN_PROGRESS_COMMAND;
        } else if ((len - start >= 7)
                   && ncm_string_equal(line + start, 7,
                                       STRLIT_ARGS("def_key"))) {
            ok = ncm_bindings_finalize_definition(
                bindings, in_progress, &actions, key, key_name,
                key_name_len, command_name, command_name_len,
                command_immediate, error);
            ncm_binding_clear(&actions);
            in_progress = IN_PROGRESS_NONE;
            if (!ok) {
                break;
            }
            if (!ncm_extract_enclosed(line + start, len - start,
                                      '"', '"', &enclosed)) {
                ncm_bindings_error(error, "%.*s:%d: invalid key",
                                   path_len, path, line_no);
                ok = false;
                break;
            }
            key = ncm_bindings_string_to_key(enclosed.data, enclosed.len);
            if (key == NC_KEY_NONE) {
                ncm_bindings_error(error, "%.*s:%d: invalid key '%.*s'",
                                   path_len, path, line_no,
                                   enclosed.len, enclosed.data);
                ok = false;
                break;
            }
            if (key_name) {
                ncm_free(key_name, key_name_cap);
            }
            key_name = ncm_string_copy(enclosed.data, enclosed.len,
                                       &key_name_cap);
            key_name_len = enclosed.len;
            in_progress = IN_PROGRESS_KEY;
        } else if (isspace((unsigned char)line[0])) {
            NcmBindingAction action;
            int32 action_start;
            int32 action_len;

            action_start = ncm_trim_start(line, len);
            action_len = ncm_trim_end(line + action_start,
                                      len - action_start);
            if (!ncm_parse_action_line(line + action_start, action_len,
                                       &action, error)) {
                ok = false;
                break;
            }
            ok = ncm_binding_append_action(&actions, &action);
            ncm_binding_action_destroy(&action);
        } else {
            ncm_bindings_error(error, "%.*s:%d: invalid line '%.*s'",
                               path_len, path, line_no, len, line);
            ok = false;
        }
    }

    if (ok) {
        ok = ncm_bindings_finalize_definition(
            bindings, in_progress, &actions, key, key_name, key_name_len,
            command_name, command_name_len, command_immediate, error);
    }

    ncm_binding_destroy(&actions);
    if (command_name) {
        ncm_free(command_name, command_name_cap);
    }
    if (key_name) {
        ncm_free(key_name, key_name_cap);
    }
    free(line);
    fclose(file);
    ncm_free(path_copy, path_cap);
    return ok;
}

bool
ncm_bindings_configuration_read_paths(NcmBindingsConfiguration *bindings,
                                      char **paths, int32 *path_lens,
                                      int32 paths_len, NcmError *error) {
    for (int32 i = 0; i < paths_len; i += 1) {
        if (!ncm_bindings_configuration_read(bindings, paths[i],
                                             path_lens[i], error)) {
            return false;
        }
    }
    return true;
}

void
ncm_bindings_configuration_generate_defaults(
    NcmBindingsConfiguration *bindings) {
    NcmBinding binding;

    ncm_binding_init(&binding);
    ncm_binding_append_normal(&binding, NCM_ACTION_QUIT);
    ncm_bindings_bind(bindings, NC_KEY_EOF, &binding);
    ncm_binding_destroy(&binding);

#define BIND(KEY, ACTION) \
    (void)ncm_bindings_bind_single(bindings, (char *)(KEY), ACTION)
#define CHAIN2(KEY, A, B) \
    (void)ncm_bindings_bind_chain2(bindings, (char *)(KEY), A, B)
#define GROUP(KEY, ...) do { \
    enum NcmActionType actions[] = { __VA_ARGS__ }; \
    (void)ncm_bindings_bind_group(bindings, (char *)(KEY), actions, \
                                  NCM_ARRAY_LEN(actions)); \
} while (0)

    BIND("mouse", NCM_ACTION_MOUSE_EVENT);
    BIND("up", NCM_ACTION_SCROLL_UP);
    CHAIN2("shift-up", NCM_ACTION_SELECT_ITEM, NCM_ACTION_SCROLL_UP);
    BIND("down", NCM_ACTION_SCROLL_DOWN);
    CHAIN2("shift-down", NCM_ACTION_SELECT_ITEM, NCM_ACTION_SCROLL_DOWN);
    BIND("[", NCM_ACTION_SCROLL_UP_ALBUM);
    BIND("]", NCM_ACTION_SCROLL_DOWN_ALBUM);
    BIND("{", NCM_ACTION_SCROLL_UP_ARTIST);
    BIND("}", NCM_ACTION_SCROLL_DOWN_ARTIST);
    BIND("page_up", NCM_ACTION_PAGE_UP);
    BIND("page_down", NCM_ACTION_PAGE_DOWN);
    BIND("home", NCM_ACTION_MOVE_HOME);
    BIND("end", NCM_ACTION_MOVE_END);
    BIND("insert", NCM_ACTION_SELECT_ITEM);
    GROUP("enter", NCM_ACTION_ENTER_DIRECTORY, NCM_ACTION_TOGGLE_OUTPUT,
          NCM_ACTION_RUN_ACTION, NCM_ACTION_PLAY_ITEM);
    GROUP("space", NCM_ACTION_ADD_ITEM_TO_PLAYLIST,
          NCM_ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE,
          NCM_ACTION_TOGGLE_VISUALIZATION_TYPE);
    GROUP("delete", NCM_ACTION_DELETE_PLAYLIST_ITEMS,
          NCM_ACTION_DELETE_BROWSER_ITEMS, NCM_ACTION_DELETE_STORED_PLAYLIST);
    GROUP("right", NCM_ACTION_NEXT_COLUMN, NCM_ACTION_SLAVE_SCREEN,
          NCM_ACTION_VOLUME_UP);
    BIND("+", NCM_ACTION_VOLUME_UP);
    GROUP("left", NCM_ACTION_PREVIOUS_COLUMN, NCM_ACTION_MASTER_SCREEN,
          NCM_ACTION_VOLUME_DOWN);
    BIND("-", NCM_ACTION_VOLUME_DOWN);
    BIND(":", NCM_ACTION_EXECUTE_COMMAND);
    BIND("tab", NCM_ACTION_NEXT_SCREEN);
    BIND("shift-tab", NCM_ACTION_PREVIOUS_SCREEN);
    BIND("f1", NCM_ACTION_SHOW_HELP);
    BIND("1", NCM_ACTION_SHOW_PLAYLIST);
    GROUP("2", NCM_ACTION_SHOW_BROWSER, NCM_ACTION_CHANGE_BROWSE_MODE);
    GROUP("3", NCM_ACTION_SHOW_SEARCH_ENGINE,
          NCM_ACTION_RESET_SEARCH_ENGINE);
    GROUP("4", NCM_ACTION_SHOW_MEDIA_LIBRARY,
          NCM_ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE);
    BIND("5", NCM_ACTION_SHOW_PLAYLIST_EDITOR);
    BIND("6", NCM_ACTION_SHOW_TAG_EDITOR);
    BIND("7", NCM_ACTION_SHOW_OUTPUTS);
    BIND("8", NCM_ACTION_SHOW_VISUALIZER);
    BIND("@", NCM_ACTION_SHOW_SERVER_INFO);
    BIND("s", NCM_ACTION_STOP);
    BIND("p", NCM_ACTION_PAUSE);
    BIND(">", NCM_ACTION_NEXT);
    BIND("<", NCM_ACTION_PREVIOUS);
    GROUP("ctrl-h", NCM_ACTION_JUMP_TO_PARENT_DIRECTORY,
          NCM_ACTION_REPLAY_SONG);
    GROUP("backspace", NCM_ACTION_JUMP_TO_PARENT_DIRECTORY,
          NCM_ACTION_REPLAY_SONG, NCM_ACTION_PLAY);
    BIND("f", NCM_ACTION_SEEK_FORWARD);
    BIND("b", NCM_ACTION_SEEK_BACKWARD);
    BIND("r", NCM_ACTION_TOGGLE_REPEAT);
    BIND("z", NCM_ACTION_TOGGLE_RANDOM);
    GROUP("y", NCM_ACTION_SAVE_TAG_CHANGES, NCM_ACTION_START_SEARCHING,
          NCM_ACTION_TOGGLE_SINGLE);
    BIND("R", NCM_ACTION_TOGGLE_CONSUME);
    BIND("Y", NCM_ACTION_TOGGLE_REPLAY_GAIN_MODE);
    BIND("T", NCM_ACTION_TOGGLE_ADD_MODE);
    BIND("|", NCM_ACTION_TOGGLE_MOUSE);
    BIND("#", NCM_ACTION_TOGGLE_BITRATE_VISIBILITY);
    BIND("Z", NCM_ACTION_SHUFFLE);
    BIND("x", NCM_ACTION_TOGGLE_CROSSFADE);
    BIND("X", NCM_ACTION_SET_CROSSFADE);
    BIND("u", NCM_ACTION_UPDATE_DATABASE);
    GROUP("ctrl-s", NCM_ACTION_SORT_PLAYLIST,
          NCM_ACTION_TOGGLE_BROWSER_SORT_MODE,
          NCM_ACTION_TOGGLE_MEDIA_LIBRARY_SORT_MODE);
    BIND("ctrl-r", NCM_ACTION_REVERSE_PLAYLIST);
    BIND("ctrl-f", NCM_ACTION_APPLY_FILTER);
    BIND("ctrl-_", NCM_ACTION_SELECT_FOUND_ITEMS);
    GROUP("/", NCM_ACTION_FIND, NCM_ACTION_FIND_ITEM_FORWARD);
    GROUP("?", NCM_ACTION_FIND, NCM_ACTION_FIND_ITEM_BACKWARD);
    BIND(".", NCM_ACTION_NEXT_FOUND_ITEM);
    BIND(",", NCM_ACTION_PREVIOUS_FOUND_ITEM);
    BIND("w", NCM_ACTION_TOGGLE_FIND_MODE);
    GROUP("e", NCM_ACTION_EDIT_SONG, NCM_ACTION_EDIT_LIBRARY_TAG,
          NCM_ACTION_EDIT_LIBRARY_ALBUM, NCM_ACTION_EDIT_DIRECTORY_NAME,
          NCM_ACTION_EDIT_PLAYLIST_NAME, NCM_ACTION_EDIT_LYRICS);
    BIND("i", NCM_ACTION_SHOW_SONG_INFO);
    BIND("I", NCM_ACTION_SHOW_ARTIST_INFO);
    BIND("g", NCM_ACTION_JUMP_TO_POSITION_IN_SONG);
    BIND("l", NCM_ACTION_SHOW_LYRICS);
    BIND("ctrl-v", NCM_ACTION_SELECT_RANGE);
    BIND("v", NCM_ACTION_REVERSE_SELECTION);
    BIND("V", NCM_ACTION_REMOVE_SELECTION);
    BIND("B", NCM_ACTION_SELECT_ALBUM);
    BIND("a", NCM_ACTION_ADD_SELECTED_ITEMS);
    GROUP("c", NCM_ACTION_CLEAR_PLAYLIST, NCM_ACTION_CLEAR_MAIN_PLAYLIST);
    GROUP("C", NCM_ACTION_CROP_PLAYLIST, NCM_ACTION_CROP_MAIN_PLAYLIST);
    GROUP("m", NCM_ACTION_MOVE_SORT_ORDER_UP,
          NCM_ACTION_MOVE_SELECTED_ITEMS_UP);
    GROUP("n", NCM_ACTION_MOVE_SORT_ORDER_DOWN,
          NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN);
    BIND("M", NCM_ACTION_MOVE_SELECTED_ITEMS_TO);
    BIND("A", NCM_ACTION_ADD);
    BIND("S", NCM_ACTION_SAVE_PLAYLIST);
    BIND("o", NCM_ACTION_JUMP_TO_PLAYING_SONG);
    GROUP("G", NCM_ACTION_JUMP_TO_BROWSER,
          NCM_ACTION_JUMP_TO_PLAYLIST_EDITOR);
    BIND("~", NCM_ACTION_JUMP_TO_MEDIA_LIBRARY);
    BIND("E", NCM_ACTION_JUMP_TO_TAG_EDITOR);
    BIND("U", NCM_ACTION_TOGGLE_PLAYING_SONG_CENTERING);
    BIND("P", NCM_ACTION_TOGGLE_DISPLAY_MODE);
    BIND("\\", NCM_ACTION_TOGGLE_INTERFACE);
    BIND("!", NCM_ACTION_TOGGLE_SEPARATORS_BETWEEN_ALBUMS);
    BIND("L", NCM_ACTION_TOGGLE_LYRICS_FETCHER);
    BIND("F", NCM_ACTION_FETCH_LYRICS_IN_BACKGROUND);
    BIND("alt-l", NCM_ACTION_TOGGLE_FETCHING_LYRICS_IN_BACKGROUND);
    BIND("ctrl-l", NCM_ACTION_TOGGLE_SCREEN_LOCK);
    GROUP("`", NCM_ACTION_TOGGLE_LIBRARY_TAG_TYPE,
          NCM_ACTION_REFETCH_LYRICS, NCM_ACTION_ADD_RANDOM_ITEMS);
    BIND("ctrl-p", NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY);
    BIND("q", NCM_ACTION_QUIT);

#undef BIND
#undef CHAIN2
#undef GROUP
    return;
}

NcmCommand *
ncm_bindings_configuration_find_command(NcmBindingsConfiguration *bindings,
                                        char *name, int32 name_len) {
    bool found;
    int32 at;

    at = ncm_bindings_command_lower_bound(bindings, name, name_len, &found);
    if (!found) {
        return NULL;
    }
    return bindings->commands + at;
}

NcmBindingSlice
ncm_bindings_configuration_get(NcmBindingsConfiguration *bindings,
                               NcKey key) {
    bool found;
    int32 at;
    NcmBindingSlice result;

    at = ncm_bindings_key_lower_bound(bindings, key, &found);
    if (!found) {
        result.data = NULL;
        result.len = 0;
        return result;
    }
    result.data = bindings->keys[at].bindings;
    result.len = bindings->keys[at].bindings_len;
    return result;
}
