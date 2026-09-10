#if !defined(BINDINGS_C)
#define BINDINGS_C

#include "cbase.h"

#include "app_controller.h"
#include "bindings.h"
#include "c/ncm_c.h"
#include "screens/nc_screens.h"
#include "ui_state.h"

NcmBindingsConfiguration Bindings;

static void
ncm_bindings_error(NcmError *ncm_error, char *format, ...) {
    va_list args;
    char buffer[256];
    int32 len;

    va_start(args, format);
    len = vsnprintf(buffer, (size_t)SIZEOF(buffer), format, args);
    va_end(args);

    ASSERT_NON_NEGATIVE(len);
    if (len >= SIZEOF(buffer)) {
        len = SIZEOF(buffer) - 1;
    }

    ncm_error_set(ncm_error, NCM_ERROR_PARSE, buffer, len);
    return;
}

static char *
ncm_string_copy(char *string, int32 string_len, int32 *cap) {
    char *result;

    result = malloc2(string_len + 1);
    if (string_len > 0) {
        memcpy64(result, string, string_len);
    }
    result[string_len] = '\0';
    *cap = string_len + 1;
    return result;
}

static int32
ncm_trim_start(char *string, int32 string_len) {
    int32 result = 0;

    while (result < string_len) {
        uint8 c = (uint8)string[result];

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
        uint8 c = (uint8)string[string_len - 1];

        if (!isspace(c)) {
            break;
        }
        string_len -= 1;
    }
    return string_len;
}

static int32
ncm_extract_enclosed(char *line, int32 line_len, char open, char close,
                     StringView *result) {
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
        return -NCM_ERROR_NOT_FOUND;
    }

    result->data = line + start;
    result->len = end - start;
    return 0;
}

void
ncm_binding_action_init(NcmBindingAction *action) {
    action->kind = NCM_BINDING_ACTION_NORMAL;
    action->value.type = ACTION_DUMMY;
    return;
}

void
ncm_binding_action_destroy(NcmBindingAction *action) {
    switch (action->kind) {
    case NCM_BINDING_ACTION_PUSH_CHARACTERS:
        free2(action->value.keys.data,
              action->value.keys.len*SIZEOF(*action->value.keys.data));
        break;
    case NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND:
    case NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND:
        free2(action->value.argument.data, action->value.argument.cap);
        break;
    case NCM_BINDING_ACTION_NORMAL:
    case NCM_BINDING_ACTION_REQUIRE_SCREEN:
    case NCM_BINDING_ACTION_REQUIRE_RUNNABLE:
    default:
        break;
    }
    ncm_binding_action_init(action);
    return;
}

static void
ncm_binding_action_copy(NcmBindingAction *dest,
                        NcmBindingAction *source) {
    ncm_binding_action_init(dest);
    dest->kind = source->kind;

    switch (source->kind) {
    case NCM_BINDING_ACTION_NORMAL:
    case NCM_BINDING_ACTION_REQUIRE_RUNNABLE:
        dest->value.type = source->value.type;
        break;
    case NCM_BINDING_ACTION_REQUIRE_SCREEN:
        dest->value.screen_type = source->value.screen_type;
        break;
    case NCM_BINDING_ACTION_PUSH_CHARACTERS:
        dest->value.keys.len = source->value.keys.len;
        if (source->value.keys.len > 0) {
            int32 bytes;

            bytes = source->value.keys.len*SIZEOF(*dest->value.keys.data);
            dest->value.keys.data = malloc2(bytes);
            memcpy64(dest->value.keys.data, source->value.keys.data, bytes);
        } else {
            dest->value.keys.data = NULL;
        }
        break;
    case NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND:
    case NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND:
        dest->value.argument.len = source->value.argument.len;
        if (source->value.argument.len > 0) {
            dest->value.argument.data =
                ncm_string_copy(source->value.argument.data,
                                source->value.argument.len,
                                &dest->value.argument.cap);
        } else {
            dest->value.argument.data = NULL;
            dest->value.argument.cap = 0;
        }
        break;
    default:
        break;
    }
    return;
}

void
ncm_binding_destroy(NcmBinding *binding) {
    ncm_binding_clear(binding);
    free2(binding->actions, binding->actions_cap*SIZEOF(*binding->actions));
    *binding = (NcmBinding){0};
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

void
ncm_binding_append_action(NcmBinding *binding, NcmBindingAction *action) {
    NcmBindingAction copy;

    if (binding->actions_len >= binding->actions_cap) {
        int32 new_cap;

        if (binding->actions_cap == 0) {
            new_cap = 4;
        } else {
            new_cap = binding->actions_cap*2;
        }
        binding->actions = realloc2(binding->actions, binding->actions_cap,
                                    new_cap, SIZEOF(*binding->actions));
        binding->actions_cap = new_cap;
    }

    ncm_binding_action_copy(&copy, action);
    binding->actions[binding->actions_len] = copy;
    binding->actions_len += 1;
    return;
}

void
ncm_binding_copy(NcmBinding *dest, NcmBinding *source) {
    *dest = (NcmBinding){0};
    for (int32 i = 0; i < source->actions_len; i += 1) {
        ncm_binding_append_action(dest, source->actions + i);
    }
    return;
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
        if (runtime && runtime->can_run_action) {
            return runtime->can_run_action(action->value.type, runtime->user);
        }
        return ncm_action_can_run(action->value.type, NULL);
    case NCM_BINDING_ACTION_PUSH_CHARACTERS:
        return true;
    case NCM_BINDING_ACTION_REQUIRE_SCREEN:
        if ((runtime == NULL) || (runtime->current_screen_is == NULL)) {
            return false;
        }
        return runtime->current_screen_is(action->value.screen_type,
                                          runtime->user);
    case NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND:
    case NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND:
        return true;
    default:
        break;
    }

    return false;
}

bool
ncm_binding_runtime_can_run_action(enum ActionType type, void *user) {
    return ncm_action_runtime_can_run(user, type);
}

int32
ncm_binding_runtime_run_action(enum ActionType type, void *user) {
    return ncm_action_runtime_run(user, type);
}

bool
ncm_binding_runtime_current_screen_is(enum ScreenType screen_type, void *user) {
    NcScreen *screen;
    enum NcScreenType nc_type;

    (void)user;
    if ((screen = app_controller_current_screen()) == NULL) {
        return false;
    }

    nc_type = screen_type_to_nc_type(screen_type);
    return nc_screen_type(screen) == nc_type;
}

void
ncm_binding_runtime_push_key(NcKey key, void *user) {
    NcWindow *window;

    (void)user;
    if ((window = ui_state_footer_window())) {
        nc_window_push_key(window, key);
    }
    return;
}

int32
ncm_binding_runtime_run_external_command(char *command, int32 command_len,
                                         void *user) {
    (void)user;
    return ncm_run_external_command(command, command_len, true, NULL);
}

int32
ncm_binding_runtime_run_external_console_command(char *command,
                                                 int32 command_len,
                                                 void *user) {
    int32 status;

    (void)user;
    nc_pause_screen();
    status = ncm_run_external_console_command(command, command_len, NULL);
    nc_unpause_screen();
    return status;
}

NcmBindingRuntime *
ncm_binding_default_runtime(void) {
    static NcmBindingRuntime runtime;
    static bool initialized;

    if (!initialized) {
        runtime.can_run_action = ncm_binding_runtime_can_run_action;
        runtime.run_action = ncm_binding_runtime_run_action;
        runtime.current_screen_is = ncm_binding_runtime_current_screen_is;
        runtime.push_key = ncm_binding_runtime_push_key;
        runtime.run_external_command = ncm_binding_runtime_run_external_command;
        runtime.run_external_console_command =
            ncm_binding_runtime_run_external_console_command;
        runtime.user = ncm_action_runtime_global();
        initialized = true;
    }
    return &runtime;
}

bool
ncm_binding_can_execute_default(NcmBinding *binding) {
    NcmBindingRuntime *runtime;

    if ((binding == NULL) || (binding->actions_len <= 0)) {
        return false;
    }

    runtime = ncm_binding_default_runtime();
    for (int32 i = 0; i < binding->actions_len; i += 1) {
        if (!ncm_binding_action_can_run(binding->actions + i, runtime)) {
            return false;
        }
    }

    return true;
}

int32
ncm_binding_execute_default(NcmBinding *binding) {
    NcmBindingRuntime *runtime;
    int32 status;

    if (binding == NULL) {
        return -EINVAL;
    }
    if (binding->actions_len <= 0) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    runtime = ncm_binding_default_runtime();
    for (int32 i = 0; i < binding->actions_len; i += 1) {
        NcmBindingAction *action = &binding->actions[i];

        if (!ncm_binding_action_can_run(action, runtime)) {
            return -NCM_ERROR_UNAVAILABLE;
        }

        switch (action->kind) {
        case NCM_BINDING_ACTION_NORMAL:
            status = runtime->run_action(action->value.type, runtime->user);
            break;
        case NCM_BINDING_ACTION_PUSH_CHARACTERS:
            for (int32 j = 0; j < action->value.keys.len; j += 1) {
                runtime->push_key(action->value.keys.data[j], runtime->user);
            }
            status = 0;
            break;
        case NCM_BINDING_ACTION_REQUIRE_SCREEN:
        case NCM_BINDING_ACTION_REQUIRE_RUNNABLE:
            status = 0;
            break;
        case NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND:
            status = runtime->run_external_command(action->value.argument.data,
                                                   action->value.argument.len,
                                                   runtime->user);
            break;
        case NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND:
            status = runtime->run_external_console_command(
                action->value.argument.data, action->value.argument.len,
                runtime->user);
            break;
        default:
            return -NCM_ERROR_INVALID_STATE;
        }

        if (status < 0) {
            return status;
        }
    }

    return 0;
}

bool
ncm_binding_is_single_action_type(NcmBinding *binding,
                                  enum ActionType type) {
    if ((binding == NULL) || (binding->actions_len != 1)) {
        return false;
    }
    if (binding->actions[0].kind != NCM_BINDING_ACTION_NORMAL) {
        return false;
    }
    return binding->actions[0].value.type == type;
}

void
ncm_command_destroy(NcmCommand *command) {
    free2(command->name, command->name_cap);
    ncm_binding_destroy(&command->binding);
    *command = (NcmCommand){0};
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
ncm_bindings_config_destroy(NcmBindingsConfiguration *bindings) {
    ncm_bindings_config_clear(bindings);
    free2(bindings->commands,
          bindings->commands_cap*SIZEOF(*bindings->commands));
    free2(bindings->keys, bindings->keys_cap*SIZEOF(*bindings->keys));
    *bindings = (NcmBindingsConfiguration){0};
    return;
}

void
ncm_bindings_config_clear(NcmBindingsConfiguration *bindings) {
    for (int32 i = 0; i < bindings->commands_len; i += 1) {
        ncm_command_destroy(bindings->commands + i);
    }
    for (int32 i = 0; i < bindings->keys_len; i += 1) {
        NcmKeyBindings *key_bindings = bindings->keys + i;

        for (int32 j = 0; j < key_bindings->bindings_len; j += 1) {
            ncm_binding_destroy(key_bindings->bindings + j);
        }
        free2(key_bindings->bindings,
              key_bindings->bindings_cap*SIZEOF(*key_bindings->bindings));
        ncm_key_bindings_init(key_bindings);
    }
    bindings->commands_len = 0;
    bindings->keys_len = 0;
    return;
}

NcKey
ncm_read_key(NcWindow *window) {
    NcKey result = NC_KEY_NONE;
    StrBuilder tmp = {0};
    bool alt_pressed = false;

    while (true) {
        NcKey input;

        if ((input = nc_window_read_key(window)) == NC_KEY_NONE) {
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

        sb_append_byte(&tmp, (char)input);
        result = nc_key_parse(tmp.data, tmp.len);
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
    sb_free(&tmp);
    return result;
}

int32
ncm_bindings_key_name(NcKey key, char *buffer, int32 buffer_len) {
    return nc_key_name(key, buffer, buffer_len);
}

static void
ncm_binding_append_normal(NcmBinding *binding, enum ActionType type) {
    NcmBindingAction action;

    ncm_binding_action_init(&action);
    action.kind = NCM_BINDING_ACTION_NORMAL;
    action.value.type = type;
    ncm_binding_append_action(binding, &action);
    ncm_binding_action_destroy(&action);
    return;
}

static int32
ncm_bindings_command_lower_bound(NcmBindingsConfiguration *bindings,
                                 char *name, int32 name_len) {
    int32 first = 0;
    int32 count = bindings->commands_len;

    while (count > 0) {
        int32 step;
        int32 mid;
        int32 cmp;
        int32 min_len;

        step = count / 2;
        mid = first + step;
        min_len = bindings->commands[mid].name_len;
        if (name_len < min_len) {
            min_len = name_len;
        }
        cmp = memcmp64(bindings->commands[mid].name, name, min_len);
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
    return first;
}

static int32
ncm_bindings_key_lower_bound(NcmBindingsConfiguration *bindings, NcKey key) {
    int32 first = 0;
    int32 count = bindings->keys_len;

    while (count > 0) {
        int32 step = count / 2;
        int32 mid = first + step;

        if (bindings->keys[mid].key < key) {
            first = mid + 1;
            count -= step + 1;
        } else {
            count = step;
        }
    }
    return first;
}

static int32
ncm_bindings_command_index(NcmBindingsConfiguration *bindings, char *name,
                           int32 name_len) {
    int32 at;

    at = ncm_bindings_command_lower_bound(bindings, name, name_len);
    if ((at >= bindings->commands_len)
        || !STREQUAL(bindings->commands[at].name,
                     bindings->commands[at].name_len, name, name_len)) {
        return -1;
    }
    return at;
}

static int32
ncm_bindings_key_index(NcmBindingsConfiguration *bindings, NcKey key) {
    int32 at;

    at = ncm_bindings_key_lower_bound(bindings, key);
    if ((at >= bindings->keys_len) || (bindings->keys[at].key != key)) {
        return -1;
    }
    return at;
}

static void
ncm_bindings_bind(NcmBindingsConfiguration *bindings, NcKey key,
                  NcmBinding *binding) {
    int32 at;
    NcmKeyBindings *key_bindings;
    NcmBinding copy;

    at = ncm_bindings_key_index(bindings, key);
    if (at < 0) {
        NcmKeyBindings item;

        at = ncm_bindings_key_lower_bound(bindings, key);
        if (bindings->keys_len >= bindings->keys_cap) {
            int32 new_cap;

            if (bindings->keys_cap == 0) {
                new_cap = 16;
            } else {
                new_cap = bindings->keys_cap*2;
            }
            bindings->keys = realloc2(bindings->keys,
                                      bindings->keys_cap, new_cap,
                                      SIZEOF(*bindings->keys));
            bindings->keys_cap = new_cap;
        }
        if (at < bindings->keys_len) {
            memmove64(bindings->keys + at + 1, bindings->keys + at,
                      (bindings->keys_len - at)*SIZEOF(*bindings->keys));
        }
        ncm_key_bindings_init(&item);
        item.key = key;
        bindings->keys[at] = item;
        bindings->keys_len += 1;
    }

    key_bindings = bindings->keys + at;
    if (key_bindings->bindings_len >= key_bindings->bindings_cap) {
        int32 new_cap;

        if (key_bindings->bindings_cap == 0) {
            new_cap = 2;
        } else {
            new_cap = key_bindings->bindings_cap*2;
        }
        key_bindings->bindings = realloc2(key_bindings->bindings,
                                          key_bindings->bindings_cap, new_cap,
                                          SIZEOF(*key_bindings->bindings));
        key_bindings->bindings_cap = new_cap;
    }

    ncm_binding_copy(&copy, binding);
    key_bindings->bindings[key_bindings->bindings_len] = copy;
    key_bindings->bindings_len += 1;
    return;
}

static bool
ncm_bindings_key_is_unbound(NcmBindingsConfiguration *bindings, NcKey key) {
    int32 at;

    if (key == NC_KEY_NONE) {
        return false;
    }
    at = ncm_bindings_key_index(bindings, key);
    return at < 0;
}

static void
ncm_bindings_bind_sequence(NcmBindingsConfiguration *bindings,
                           char *key_name, int32 key_name_len,
                           enum ActionType *actions, int32 actions_len) {
    NcmBinding binding = {0};
    NcKey key = nc_key_parse(key_name, key_name_len);

    if (!ncm_bindings_key_is_unbound(bindings, key)) {
        return;
    }

    for (int32 i = 0; i < actions_len; i += 1) {
        ncm_binding_append_normal(&binding, actions[i]);
    }
    ncm_bindings_bind(bindings, key, &binding);
    ncm_binding_destroy(&binding);
    return;
}

static void
ncm_bindings_bind_group(NcmBindingsConfiguration *bindings,
                        char *key_name, int32 key_name_len,
                        enum ActionType *actions, int32 actions_len) {
    NcKey key = nc_key_parse(key_name, key_name_len);

    if (!ncm_bindings_key_is_unbound(bindings, key)) {
        return;
    }

    for (int32 i = 0; i < actions_len; i += 1) {
        NcmBinding binding = {0};
        ncm_binding_append_normal(&binding, actions[i]);
        ncm_bindings_bind(bindings, key, &binding);
        ncm_binding_destroy(&binding);
    }
    return;
}

#define NCM_DEFAULT_BINDINGS(XX_SEQUENCE, XX_GROUP)                       \
    XX_SEQUENCE("mouse", ACTION_MOUSE_EVENT)                             \
    XX_SEQUENCE("up", ACTION_SCROLL_UP)                                  \
    XX_SEQUENCE("shift-up", ACTION_SELECT_ITEM, ACTION_SCROLL_UP)         \
    XX_SEQUENCE("down", ACTION_SCROLL_DOWN)                              \
    XX_SEQUENCE("shift-down", ACTION_SELECT_ITEM, ACTION_SCROLL_DOWN)     \
    XX_SEQUENCE("[", ACTION_SCROLL_UP_ALBUM)                             \
    XX_SEQUENCE("]", ACTION_SCROLL_DOWN_ALBUM)                           \
    XX_SEQUENCE("{", ACTION_SCROLL_UP_ARTIST)                            \
    XX_SEQUENCE("}", ACTION_SCROLL_DOWN_ARTIST)                          \
    XX_SEQUENCE("page_up", ACTION_PAGE_UP)                               \
    XX_SEQUENCE("page_down", ACTION_PAGE_DOWN)                           \
    XX_SEQUENCE("home", ACTION_MOVE_HOME)                                \
    XX_SEQUENCE("end", ACTION_MOVE_END)                                  \
    XX_SEQUENCE("insert", ACTION_SELECT_ITEM)                            \
    XX_GROUP("enter", ACTION_ENTER_DIRECTORY, ACTION_TOGGLE_OUTPUT,       \
             ACTION_RUN_ACTION, ACTION_PLAY_ITEM)                        \
    XX_GROUP("space", ACTION_ADD_ITEM_TO_PLAYLIST,                       \
             ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE,                 \
             ACTION_TOGGLE_VISUALIZATION_TYPE)                           \
    XX_GROUP("delete", ACTION_DELETE_PLAYLIST_ITEMS,                     \
             ACTION_DELETE_BROWSER_ITEMS, ACTION_DELETE_STORED_PLAYLIST)  \
    XX_GROUP("right", ACTION_NEXT_COLUMN, ACTION_SLAVE_SCREEN,           \
             ACTION_VOLUME_UP)                                           \
    XX_SEQUENCE("+", ACTION_VOLUME_UP)                                   \
    XX_GROUP("left", ACTION_PREVIOUS_COLUMN, ACTION_MASTER_SCREEN,        \
             ACTION_VOLUME_DOWN)                                         \
    XX_SEQUENCE("-", ACTION_VOLUME_DOWN)                                 \
    XX_SEQUENCE(":", ACTION_EXECUTE_COMMAND)                             \
    XX_SEQUENCE("tab", ACTION_NEXT_SCREEN)                               \
    XX_SEQUENCE("shift-tab", ACTION_PREVIOUS_SCREEN)                     \
    XX_SEQUENCE("f1", ACTION_SHOW_HELP)                                  \
    XX_SEQUENCE("1", ACTION_SHOW_PLAYLIST)                               \
    XX_GROUP("2", ACTION_SHOW_BROWSER, ACTION_CHANGE_BROWSE_MODE)        \
    XX_GROUP("3", ACTION_SHOW_SEARCH_ENGINE, ACTION_RESET_SEARCH_ENGINE) \
    XX_GROUP("4", ACTION_SHOW_MEDIA_LIBRARY,                             \
             ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE)                   \
    XX_SEQUENCE("5", ACTION_SHOW_PLAYLIST_EDITOR)                        \
    XX_SEQUENCE("6", ACTION_SHOW_TAG_EDIT)                               \
    XX_SEQUENCE("7", ACTION_SHOW_OUTPUTS)                                \
    XX_SEQUENCE("8", ACTION_SHOW_VISUALIZER)                             \
    XX_SEQUENCE("@", ACTION_SHOW_SERVER_INFO)                            \
    XX_SEQUENCE("s", ACTION_STOP)                                        \
    XX_SEQUENCE("p", ACTION_PAUSE)                                       \
    XX_SEQUENCE(">", ACTION_NEXT)                                        \
    XX_SEQUENCE("<", ACTION_PREVIOUS)                                    \
    XX_GROUP("ctrl-h", ACTION_JUMP_TO_PARENT_DIRECTORY,                  \
             ACTION_REPLAY_SONG)                                         \
    XX_GROUP("backspace", ACTION_JUMP_TO_PARENT_DIRECTORY,               \
             ACTION_REPLAY_SONG, ACTION_PLAY)                            \
    XX_SEQUENCE("f", ACTION_SEEK_FORWARD)                                \
    XX_SEQUENCE("b", ACTION_SEEK_BACKWARD)                               \
    XX_SEQUENCE("r", ACTION_TOGGLE_REPEAT)                               \
    XX_SEQUENCE("z", ACTION_TOGGLE_RANDOM)                               \
    XX_GROUP("y", ACTION_SAVE_TAG_CHANGES, ACTION_START_SEARCHING,       \
             ACTION_TOGGLE_SINGLE)                                       \
    XX_SEQUENCE("R", ACTION_TOGGLE_CONSUME)                              \
    XX_SEQUENCE("Y", ACTION_TOGGLE_REPLAY_GAIN_MODE)                     \
    XX_SEQUENCE("T", ACTION_TOGGLE_ADD_MODE)                             \
    XX_SEQUENCE("|", ACTION_TOGGLE_MOUSE)                                \
    XX_SEQUENCE("#", ACTION_TOGGLE_BITRATE_VISIBILITY)                   \
    XX_SEQUENCE("Z", ACTION_SHUFFLE)                                     \
    XX_SEQUENCE("x", ACTION_TOGGLE_CROSSFADE)                            \
    XX_SEQUENCE("X", ACTION_SET_CROSSFADE)                               \
    XX_SEQUENCE("u", ACTION_UPDATE_DATABASE)                             \
    XX_GROUP("ctrl-s", ACTION_SORT_PLAYLIST,                             \
             ACTION_TOGGLE_BROWSER_SORT_MODE,                            \
             ACTION_TOGGLE_MEDIA_LIBRARY_SORT_MODE)                      \
    XX_SEQUENCE("ctrl-r", ACTION_REVERSE_PLAYLIST)                       \
    XX_SEQUENCE("ctrl-f", ACTION_APPLY_FILTER)                           \
    XX_SEQUENCE("ctrl-_", ACTION_SELECT_FOUND_ITEMS)                     \
    XX_GROUP("/", ACTION_FIND, ACTION_FIND_ITEM_FORWARD)                 \
    XX_GROUP("?", ACTION_FIND, ACTION_FIND_ITEM_BACKWARD)                \
    XX_SEQUENCE(".", ACTION_NEXT_FOUND_ITEM)                             \
    XX_SEQUENCE(",", ACTION_PREVIOUS_FOUND_ITEM)                         \
    XX_SEQUENCE("w", ACTION_TOGGLE_FIND_MODE)                            \
    XX_GROUP("e", ACTION_EDIT_SONG, ACTION_EDIT_LIBRARY_TAG,             \
             ACTION_EDIT_LIBRARY_ALBUM, ACTION_EDIT_DIRECTORY_NAME,       \
             ACTION_EDIT_PLAYLIST_NAME, ACTION_EDIT_LYRICS)              \
    XX_SEQUENCE("i", ACTION_SHOW_SONG_INFO)                              \
    XX_SEQUENCE("I", ACTION_SHOW_ARTIST_INFO)                            \
    XX_SEQUENCE("g", ACTION_JUMP_TO_POSITION_IN_SONG)                    \
    XX_SEQUENCE("l", ACTION_SHOW_LYRICS)                                 \
    XX_SEQUENCE("ctrl-v", ACTION_SELECT_RANGE)                           \
    XX_SEQUENCE("v", ACTION_REVERSE_SELECTION)                           \
    XX_SEQUENCE("V", ACTION_REMOVE_SELECTION)                            \
    XX_SEQUENCE("B", ACTION_SELECT_ALBUM)                                \
    XX_SEQUENCE("a", ACTION_ADD_SELECTED_ITEMS)                          \
    XX_GROUP("c", ACTION_CLEAR_PLAYLIST, ACTION_CLEAR_MAIN_PLAYLIST)     \
    XX_GROUP("C", ACTION_CROP_PLAYLIST, ACTION_CROP_MAIN_PLAYLIST)       \
    XX_GROUP("m", ACTION_MOVE_SORT_ORDER_UP,                             \
             ACTION_MOVE_SELECTED_ITEMS_UP)                              \
    XX_GROUP("n", ACTION_MOVE_SORT_ORDER_DOWN,                           \
             ACTION_MOVE_SELECTED_ITEMS_DOWN)                            \
    XX_SEQUENCE("M", ACTION_MOVE_SELECTED_ITEMS_TO)                      \
    XX_SEQUENCE("A", ACTION_ADD)                                         \
    XX_SEQUENCE("S", ACTION_SAVE_PLAYLIST)                               \
    XX_SEQUENCE("o", ACTION_JUMP_TO_PLAYING_SONG)                        \
    XX_GROUP("G", ACTION_JUMP_TO_BROWSER,                                \
             ACTION_JUMP_TO_PLAYLIST_EDITOR)                             \
    XX_SEQUENCE("~", ACTION_JUMP_TO_MEDIA_LIBRARY)                       \
    XX_SEQUENCE("E", ACTION_JUMP_TO_TAG_EDIT)                            \
    XX_SEQUENCE("U", ACTION_TOGGLE_PLAYING_SONG_CENTERING)               \
    XX_SEQUENCE("P", ACTION_TOGGLE_DISPLAY_MODE)                         \
    XX_SEQUENCE("\\", ACTION_TOGGLE_INTERFACE)                          \
    XX_SEQUENCE("!", ACTION_TOGGLE_SEPARATORS_BETWEEN_ALBUMS)            \
    XX_SEQUENCE("L", ACTION_TOGGLE_LYRICS_FETCHER)                       \
    XX_SEQUENCE("F", ACTION_FETCH_LYRICS_IN_BACKGROUND)                  \
    XX_SEQUENCE("alt-l", ACTION_TOGGLE_FETCHING_LYRICS_IN_BACKGROUND)    \
    XX_SEQUENCE("ctrl-l", ACTION_TOGGLE_SCREEN_LOCK)                     \
    XX_GROUP("`", ACTION_TOGGLE_LIBRARY_TAG_TYPE, ACTION_REFETCH_LYRICS, \
             ACTION_ADD_RANDOM_ITEMS)                                    \
    XX_SEQUENCE("ctrl-p", ACTION_SET_SELECTED_ITEMS_PRIORITY)            \
    XX_SEQUENCE("q", ACTION_QUIT)


enum NcmBindingDirectiveKind {
    NCM_BINDING_DIRECTIVE_DUMMY,
    NCM_BINDING_DIRECTIVE_PUSH_CHARACTER,
    NCM_BINDING_DIRECTIVE_PUSH_CHARACTERS,
    NCM_BINDING_DIRECTIVE_REQUIRE_SCREEN,
    NCM_BINDING_DIRECTIVE_REQUIRE_RUNNABLE,
    NCM_BINDING_DIRECTIVE_EXTERNAL_COMMAND,
    NCM_BINDING_DIRECTIVE_EXTERNAL_CONSOLE_COMMAND,
};

typedef struct NcmBindingDirective {
    char *name;
    int32 name_len;
    bool argument_required;
    enum NcmBindingDirectiveKind kind;
} NcmBindingDirective;

#define NCM_BINDING_DIRECTIVES(XX)                                      \
    XX(set_visualizer_sample_multiplier, false, DUMMY)                  \
    XX(push_character, true, PUSH_CHARACTER)                            \
    XX(push_characters, true, PUSH_CHARACTERS)                          \
    XX(require_screen, true, REQUIRE_SCREEN)                            \
    XX(require_runnable, true, REQUIRE_RUNNABLE)                        \
    XX(run_external_command, true, EXTERNAL_COMMAND)                    \
    XX(run_external_console_command, true, EXTERNAL_CONSOLE_COMMAND)

#define NCM_BINDING_DIRECTIVE_ENTRY(name, required, type) \
    {#name, STRLIT_LEN(#name), required, NCM_BINDING_DIRECTIVE_##type},

static NcmBindingDirective ncm_binding_directives[] = {
    NCM_BINDING_DIRECTIVES(NCM_BINDING_DIRECTIVE_ENTRY)
};

#undef NCM_BINDING_DIRECTIVE_ENTRY

static NcmBindingDirective *
ncm_binding_directive_find(char *name, int32 name_len) {
    for (int32 i = 0; i < SIZEOF(ncm_binding_directives)
                            / SIZEOF(ncm_binding_directives[0]); i += 1) {
        if (STREQUAL(name, name_len, ncm_binding_directives[i].name,
                     ncm_binding_directives[i].name_len)) {
            return ncm_binding_directives + i;
        }
    }
    return NULL;
}

static int32
ncm_binding_parse_directive(NcmBindingAction *action,
                            NcmBindingDirective *directive,
                            StringView argument, NcmError *ncm_error) {
    switch (directive->kind) {
    case NCM_BINDING_DIRECTIVE_DUMMY:
        action->kind = NCM_BINDING_ACTION_NORMAL;
        action->value.type = ACTION_DUMMY;
        return 0;
    case NCM_BINDING_DIRECTIVE_PUSH_CHARACTER: {
        NcKey action_key;

        action_key = nc_key_parse(argument.data, argument.len);
        if (action_key == NC_KEY_NONE) {
            ncm_bindings_error(ncm_error, "invalid character passed to "
                               "push_character: '%.*s'",
                               argument.len, argument.data);
            return -NCM_ERROR_PARSE;
        }
        action->kind = NCM_BINDING_ACTION_PUSH_CHARACTERS;
        action->value.keys.len = 1;
        action->value.keys.data = malloc2(SIZEOF(*action->value.keys.data));
        action->value.keys.data[0] = action_key;
        return 0;
    }
    case NCM_BINDING_DIRECTIVE_PUSH_CHARACTERS:
        if (argument.len <= 0) {
            ncm_bindings_error(ncm_error, "empty argument passed to "
                               "push_characters");
            return -NCM_ERROR_PARSE;
        }
        action->kind = NCM_BINDING_ACTION_PUSH_CHARACTERS;
        action->value.keys.len = argument.len;
        action->value.keys.data = malloc2(action->value.keys.len
                                          *SIZEOF(*action->value.keys.data));
        for (int32 i = 0; i < argument.len; i += 1) {
            action->value.keys.data[i] = (NcKey)(uint8)argument.data[i];
        }
        return 0;
    case NCM_BINDING_DIRECTIVE_REQUIRE_SCREEN:
        if (screen_type_parse(argument.data, argument.len,
                              &action->value.screen_type) < 0) {
            ncm_bindings_error(ncm_error, "unknown screen passed to "
                               "require_screen: '%.*s'",
                               argument.len, argument.data);
            return -NCM_ERROR_PARSE;
        }
        action->kind = NCM_BINDING_ACTION_REQUIRE_SCREEN;
        return 0;
    case NCM_BINDING_DIRECTIVE_REQUIRE_RUNNABLE:
        if (ncm_action_type_parse(argument.data, argument.len,
                                  &action->value.type) < 0) {
            ncm_bindings_error(ncm_error, "unknown action passed to "
                               "require_runnable: '%.*s'",
                               argument.len, argument.data);
            return -NCM_ERROR_PARSE;
        }
        action->kind = NCM_BINDING_ACTION_REQUIRE_RUNNABLE;
        return 0;
    case NCM_BINDING_DIRECTIVE_EXTERNAL_COMMAND:
    case NCM_BINDING_DIRECTIVE_EXTERNAL_CONSOLE_COMMAND:
        if (argument.len <= 0) {
            ncm_bindings_error(ncm_error, "empty command passed to %.*s",
                               directive->name_len, directive->name);
            return -NCM_ERROR_PARSE;
        }
        if (directive->kind == NCM_BINDING_DIRECTIVE_EXTERNAL_COMMAND) {
            action->kind = NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND;
        } else {
            action->kind = NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND;
        }
        action->value.argument.data = ncm_string_copy(
            argument.data, argument.len, &action->value.argument.cap);
        action->value.argument.len = argument.len;
        return 0;
    default:
        UNREACHABLE();
    }
}

static int32
ncm_binding_parse_action_line(NcmBindingAction *action, char *line,
                              int32 line_len, NcmError *ncm_error) {
    NcmBindingDirective *directive;
    StringView argument = {0};
    int32 name_len;

    name_len = 0;
    while ((name_len < line_len) && !isspace((uint8)line[name_len])) {
        name_len += 1;
    }

    directive = ncm_binding_directive_find(line, name_len);
    if (directive != NULL) {
        if (directive->argument_required) {
            if ((name_len == line_len)
                || (ncm_extract_enclosed(line + name_len,
                                         line_len - name_len,
                                         '"', '"', &argument) < 0)) {
                ncm_bindings_error(ncm_error, "missing quoted argument: '%.*s'",
                                   line_len, line);
                return -NCM_ERROR_PARSE;
            }
        }
        return ncm_binding_parse_directive(action, directive, argument,
                                           ncm_error);
    }

    if (name_len == line_len) {
        if (ncm_action_type_parse(line, name_len, &action->value.type) < 0) {
            ncm_bindings_error(ncm_error, "unknown action: '%.*s'",
                               name_len, line);
            return -NCM_ERROR_PARSE;
        }
        action->kind = NCM_BINDING_ACTION_NORMAL;
        return 0;
    }

    if (ncm_extract_enclosed(line + name_len, line_len - name_len,
                             '"', '"', &argument) < 0) {
        ncm_bindings_error(ncm_error, "missing quoted argument: '%.*s'",
                           line_len, line);
        return -NCM_ERROR_PARSE;
    }

    ncm_bindings_error(ncm_error, "unknown action: '%.*s'", line_len, line);
    return -NCM_ERROR_PARSE;
}

#undef NCM_BINDING_DIRECTIVES

static int32
ncm_bindings_finalize_definition(NcmBindingsConfiguration *bindings,
                                 int32 in_progress, NcmBinding *actions,
                                 NcKey key, char *key_name, int32 key_name_len,
                                 char *command_name, int32 command_name_len,
                                 bool command_immediate, NcmError *ncm_error) {
    if (in_progress == 0) {
        return 0;
    }
    if (actions->actions_len == 0) {
        if (in_progress == 1) {
            ncm_bindings_error(ncm_error,
                               "definition of command '%.*s' cannot be empty",
                               command_name_len, command_name);
        } else {
            ncm_bindings_error(ncm_error,
                               "definition of key '%.*s' cannot be empty",
                               key_name_len, key_name);
        }
        return -NCM_ERROR_PARSE;
    }

    if (in_progress == 1) {
        NcmCommand command = {0};
        int32 status;

        command.name = command_name;
        command.name_len = command_name_len;
        command.name_cap = command_name_len + 1;
        command.immediate = command_immediate;

        ncm_binding_copy(&command.binding, actions);

        if (ncm_bindings_command_index(bindings, command.name,
                                       command.name_len) >= 0) {
            ncm_bindings_error(ncm_error, "redefinition of command '%.*s'",
                               command.name_len, command.name);
            status = -NCM_ERROR_PARSE;
        } else {
            NcmCommand copy = {0};
            int32 at;

            if (bindings->commands_len >= bindings->commands_cap) {
                int32 new_cap;

                if (bindings->commands_cap == 0) {
                    new_cap = 8;
                } else {
                    new_cap = bindings->commands_cap*2;
                }
                bindings->commands = realloc2(bindings->commands,
                                              bindings->commands_cap, new_cap,
                                              SIZEOF(*bindings->commands));
                bindings->commands_cap = new_cap;
            }

            copy.name = ncm_string_copy(command.name, command.name_len,
                                        &copy.name_cap);
            copy.name_len = command.name_len;
            copy.immediate = command.immediate;
            ncm_binding_copy(&copy.binding, &command.binding);
            at = ncm_bindings_command_lower_bound(bindings, command.name,
                                                  command.name_len);
            if (at < bindings->commands_len) {
                memmove64(bindings->commands + at + 1, bindings->commands + at,
                          (bindings->commands_len - at)
                              *SIZEOF(*bindings->commands));
            }
            bindings->commands[at] = copy;
            bindings->commands_len += 1;
            status = 0;
        }

        ncm_binding_destroy(&command.binding);
        return status;
    }

    ncm_bindings_bind(bindings, key, actions);
    return 0;
}

int32
ncm_bindings_config_read(NcmBindingsConfiguration *bindings,
                         char *path, int32 path_len, NcmError *ncm_error) {
    enum {
        IN_PROGRESS_NONE = 0,
        IN_PROGRESS_COMMAND = 1,
        IN_PROGRESS_KEY = 2,
    };
    char *path_copy;
    char *content;
    char *content_end;
    char *line;
    int32 path_cap;
    int32 content_len;
    int32 in_progress;
    int32 line_no;
    int32 status;
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
    if (!ncm_fs_path_is_existing(path, path_len)) {
        free2(path_copy, path_cap);
        return 0;
    }
    if ((content_len = read_entire_file(path_copy, &content)) < 0) {
        int32 code;

        code = -content_len;
        ncm_bindings_error(ncm_error, "%.*s: read error: %s", path_len,
                           path, strerror(code));
        free2(path_copy, path_cap);
        return content_len;
    }

    in_progress = IN_PROGRESS_NONE;
    line_no = 0;
    status = 0;
    command_name = NULL;
    key_name = NULL;
    command_name_len = 0;
    key_name_len = 0;
    command_name_cap = 0;
    key_name_cap = 0;
    key = NC_KEY_NONE;
    command_immediate = false;
    actions = (NcmBinding){0};
    content_end = content + content_len;
    line = content;

    while ((status >= 0) && (line < content_end)) {
        char *current_line;
        char *line_end;
        char *next;
        int32 len;
        int32 start;
        StringView enclosed;

        current_line = line;
        if ((line_end = memchr64(current_line, '\n',
                                 content_end - current_line))) {
            len = (int32)(line_end - current_line);
            next = line_end + 1;
        } else {
            len = (int32)(content_end - current_line);
            next = content_end;
        }
        line_no += 1;
        line = next;
        len = ncm_trim_end(current_line, len);
        if ((len == 0) || (current_line[0] == '#')) {
            continue;
        }
        start = ncm_trim_start(current_line, len);

        if ((len - start >= 11)
            && STREQUAL(current_line + start, 11, "def_command")) {
            status = ncm_bindings_finalize_definition(bindings, in_progress,
                                                      &actions, key,
                                                      key_name, key_name_len,
                                                      command_name,
                                                      command_name_len,
                                                      command_immediate,
                                                      ncm_error);
            ncm_binding_clear(&actions);
            in_progress = IN_PROGRESS_NONE;
            if (status < 0) {
                break;
            }
            if (ncm_extract_enclosed(current_line + start, len - start,
                                     '"', '"', &enclosed) < 0) {
                ncm_bindings_error(ncm_error,
                                   "%.*s:%d: command must have non-empty name",
                                   path_len, path, line_no);
                status = -NCM_ERROR_PARSE;
                break;
            }
            if (enclosed.len <= 0) {
                ncm_bindings_error(ncm_error,
                                   "%.*s:%d: command must have non-empty name",
                                   path_len, path, line_no);
                status = -NCM_ERROR_PARSE;
                break;
            }
            free2(command_name, command_name_cap);
            command_name = ncm_string_copy(enclosed.data, enclosed.len,
                                           &command_name_cap);
            command_name_len = enclosed.len;
            if (ncm_extract_enclosed(current_line + start, len - start,
                                     '[', ']', &enclosed) < 0) {
                ncm_bindings_error(ncm_error, "%.*s:%d: missing command type",
                                   path_len, path, line_no);
                status = -NCM_ERROR_PARSE;
                break;
            }
            if (STREQUAL(enclosed.data, enclosed.len, "immediate")) {
                command_immediate = true;
            } else if (STREQUAL(enclosed.data, enclosed.len, "deferred")) {
                command_immediate = false;
            } else {
                ncm_bindings_error(ncm_error,
                                   "%.*s:%d: invalid command type '%.*s'",
                                   path_len, path, line_no,
                                   enclosed.len, enclosed.data);
                status = -NCM_ERROR_PARSE;
                break;
            }
            in_progress = IN_PROGRESS_COMMAND;
        } else if ((len - start >= 7)
                   && STREQUAL(current_line + start, 7, "def_key")) {
            status = ncm_bindings_finalize_definition(bindings, in_progress,
                                                      &actions, key,
                                                      key_name, key_name_len,
                                                      command_name,
                                                      command_name_len,
                                                      command_immediate,
                                                      ncm_error);
            ncm_binding_clear(&actions);
            in_progress = IN_PROGRESS_NONE;
            if (status < 0) {
                break;
            }
            if (ncm_extract_enclosed(current_line + start, len - start,
                                     '"', '"', &enclosed) < 0) {
                ncm_bindings_error(ncm_error, "%.*s:%d: invalid key", path_len,
                                   path, line_no);
                status = -NCM_ERROR_PARSE;
                break;
            }
            key = nc_key_parse(enclosed.data, enclosed.len);
            if (key == NC_KEY_NONE) {
                ncm_bindings_error(ncm_error, "%.*s:%d: invalid key '%.*s'",
                                   path_len, path, line_no, enclosed.len,
                                   enclosed.data);
                status = -NCM_ERROR_PARSE;
                break;
            }
            free2(key_name, key_name_cap);
            key_name = ncm_string_copy(enclosed.data, enclosed.len,
                                       &key_name_cap);
            key_name_len = enclosed.len;
            in_progress = IN_PROGRESS_KEY;
        } else if (isspace((uint8)current_line[0])) {
            NcmBindingAction action;
            int32 action_start;
            int32 action_len;

            action_start = ncm_trim_start(current_line, len);
            action_len = ncm_trim_end(current_line + action_start,
                                      len - action_start);
            ncm_binding_action_init(&action);
            status = ncm_binding_parse_action_line(
                &action, current_line + action_start, action_len, ncm_error);
            if (status < 0) {
                break;
            }
            ncm_binding_append_action(&actions, &action);
            ncm_binding_action_destroy(&action);
        } else {
            ncm_bindings_error(ncm_error, "%.*s:%d: invalid line '%.*s'",
                               path_len, path, line_no, len, current_line);
            status = -NCM_ERROR_PARSE;
        }
    }

    if (status >= 0) {
        status = ncm_bindings_finalize_definition(bindings, in_progress,
                                                  &actions, key,
                                                  key_name, key_name_len,
                                                  command_name,
                                                  command_name_len,
                                                  command_immediate,
                                                  ncm_error);
    }

    ncm_binding_destroy(&actions);
    free2(command_name, command_name_cap);
    free2(key_name, key_name_cap);
    free2(content, content_len + 1);
    free2(path_copy, path_cap);
    return status;
}

void
ncm_bindings_config_generate_defaults(NcmBindingsConfiguration *bindings) {
    NcmBinding binding = {0};

    ncm_binding_append_normal(&binding, ACTION_QUIT);
    ncm_bindings_bind(bindings, NC_KEY_EOF, &binding);
    ncm_binding_destroy(&binding);

#define BIND_DEFAULT_SEQUENCE(KEY, ...) do {                                \
    enum ActionType actions[] = { __VA_ARGS__ };                            \
    ncm_bindings_bind_sequence(bindings, STRLIT(KEY),                       \
                               actions, LENGTH(actions));                   \
} while (0);
#define BIND_DEFAULT_GROUP(KEY, ...) do {                                   \
    enum ActionType actions[] = { __VA_ARGS__ };                            \
    ncm_bindings_bind_group(bindings, STRLIT(KEY),                          \
                            actions, LENGTH(actions));                      \
} while (0);

    NCM_DEFAULT_BINDINGS(BIND_DEFAULT_SEQUENCE, BIND_DEFAULT_GROUP)

#undef BIND_DEFAULT_SEQUENCE
#undef BIND_DEFAULT_GROUP
    return;
}

NcmCommand *
ncm_bindings_config_find_command(NcmBindingsConfiguration *bindings,
                                 char *name, int32 name_len) {
    int32 at;

    at = ncm_bindings_command_index(bindings, name, name_len);
    if (at < 0) {
        return NULL;
    }
    return bindings->commands + at;
}

int32
ncm_bindings_config_get(NcmBindingsConfiguration *bindings, NcKey key,
                        NcmBindingSlice *result) {
    int32 at;

    if (result == NULL) {
        return -EINVAL;
    }

    result->data = NULL;
    result->len = 0;
    at = ncm_bindings_key_index(bindings, key);
    if (at < 0) {
        return 0;
    }
    result->data = bindings->keys[at].bindings;
    result->len = bindings->keys[at].bindings_len;
    return result->len;
}

#endif /* BINDINGS_C */
