#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "app_binding_migration.h"
#include "bindings.h"
#include "c/ncm_base.h"
#include "c/ncm_error.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

#define REQUIRE(COND) do {                                                \
    if (!(COND)) {                                                        \
        fail(__FILE__, __LINE__, (char *)#COND);                          \
    }                                                                     \
} while (0)

#define REQUIRE_INT(ACTUAL, EXPECTED)                                     \
    require_int(__FILE__, __LINE__, (char *)#ACTUAL, ACTUAL, EXPECTED)

#define REQUIRE_STRING(ACTUAL, ACTUAL_LEN, EXPECTED)                      \
    require_string(__FILE__, __LINE__, (char *)#ACTUAL,                   \
                   ACTUAL, ACTUAL_LEN, LIT_ARGS(EXPECTED))

typedef struct TestRunnerState {
    enum NcmActionType seen[8];
    int32 seen_len;
    int32 fail_at;
} TestRunnerState;

typedef struct TestRuntimeState {
    enum NcmActionType ran[8];
    NcKey pushed[8];
    char external[64];
    char console[64];

    int32 ran_len;
    int32 pushed_len;
    int32 external_len;
    int32 console_len;

    enum ScreenType current_screen;
    enum NcmActionType blocked_action;
    bool block_action;
} TestRuntimeState;

static void fail(char *file, int32 line, char *condition);
static void require_int(char *file, int32 line, char *name,
                        int32 actual, int32 expected);
static void require_string(char *file, int32 line, char *name,
                           char *actual, int32 actual_len,
                           char *expected, int32 expected_len);
static void write_file(char *path, char *data, int32 data_len);
static void append_action(NcmBinding *binding, enum NcmActionType type);
static void append_action_kind(NcmBinding *binding,
                               enum NcmBindingActionKind kind,
                               enum NcmActionType type);
static bool record_action(NcmBindingAction *action, void *user);
static bool runtime_can_run_action(enum NcmActionType type, void *user);
static bool runtime_run_action(enum NcmActionType type, void *user);
static bool runtime_current_screen_is(enum ScreenType screen_type,
                                      void *user);
static void runtime_push_key(NcKey key, void *user);
static bool runtime_run_external_command(char *command, int32 command_len,
                                         void *user);
static bool runtime_run_external_console_command(char *command,
                                                 int32 command_len,
                                                 void *user);
static NcmBindingRuntime runtime_make(TestRuntimeState *state);
static void test_default_bindings(void);
static void test_key_lookup(void);
static void test_command_lookup(void);
static void test_binding_file_parsing(void);
static void test_chained_action_execution(void);
static void test_runtime_action_execution(void);
static void test_runtime_can_execute_preflight(void);
static void test_app_binding_migration_c_safe_actions(void);
static void test_binding_action_formatting(void);
static void test_read_paths(void);
static void test_key_formatting(void);

static void
fail(char *file, int32 line, char *condition) {
    fprintf(stderr, "%s:%d: requirement failed: %s\n",
            file, line, condition);
    exit(EXIT_FAILURE);
}

static void
require_int(char *file, int32 line, char *name,
            int32 actual, int32 expected) {
    if (actual != expected) {
        fprintf(stderr, "%s:%d: %s: expected %d, got %d\n",
                file, line, name, expected, actual);
        exit(EXIT_FAILURE);
    }
    return;
}

static void
require_string(char *file, int32 line, char *name,
               char *actual, int32 actual_len,
               char *expected, int32 expected_len) {
    if (!ncm_string_equal(actual, actual_len, expected, expected_len)) {
        fprintf(stderr, "%s:%d: %s: expected '%.*s', got '%.*s'\n",
                file, line, name,
                expected_len, expected, actual_len, actual);
        exit(EXIT_FAILURE);
    }
    return;
}

static void
write_file(char *path, char *data, int32 data_len) {
    FILE *file;

    file = fopen(path, "w");
    REQUIRE(file != NULL);
    REQUIRE(fwrite(data, 1, (size_t)data_len, file) == (size_t)data_len);
    REQUIRE(fclose(file) == 0);
    return;
}

static void
append_action(NcmBinding *binding, enum NcmActionType type) {
    append_action_kind(binding, NCM_BINDING_ACTION_NORMAL, type);
    return;
}

static void
append_action_kind(NcmBinding *binding, enum NcmBindingActionKind kind,
                   enum NcmActionType type) {
    NcmBindingAction action;

    ncm_binding_action_init(&action);
    action.kind = kind;
    action.type = type;
    REQUIRE(ncm_binding_append_action(binding, &action));
    ncm_binding_action_destroy(&action);
    return;
}

static bool
record_action(NcmBindingAction *action, void *user) {
    TestRunnerState *state;

    state = user;
    state->seen[state->seen_len++] = action->type;
    if ((state->fail_at > 0) && (state->seen_len == state->fail_at)) {
        return false;
    }
    return true;
}

static bool
runtime_can_run_action(enum NcmActionType type, void *user) {
    TestRuntimeState *state;

    state = user;
    if (state->block_action && (state->blocked_action == type)) {
        return false;
    }
    return true;
}

static bool
runtime_run_action(enum NcmActionType type, void *user) {
    TestRuntimeState *state;

    state = user;
    state->ran[state->ran_len++] = type;
    return true;
}

static bool
runtime_current_screen_is(enum ScreenType screen_type, void *user) {
    TestRuntimeState *state;

    state = user;
    return state->current_screen == screen_type;
}

static void
runtime_push_key(NcKey key, void *user) {
    TestRuntimeState *state;

    state = user;
    state->pushed[state->pushed_len++] = key;
    return;
}

static bool
runtime_run_external_command(char *command, int32 command_len,
                             void *user) {
    TestRuntimeState *state;

    state = user;
    REQUIRE(command_len < (int32)SIZEOF(state->external));
    ncm_memcpy(state->external, command, command_len);
    state->external[command_len] = '\0';
    state->external_len = command_len;
    return true;
}

static bool
runtime_run_external_console_command(char *command, int32 command_len,
                                     void *user) {
    TestRuntimeState *state;

    state = user;
    REQUIRE(command_len < (int32)SIZEOF(state->console));
    ncm_memcpy(state->console, command, command_len);
    state->console[command_len] = '\0';
    state->console_len = command_len;
    return true;
}

static NcmBindingRuntime
runtime_make(TestRuntimeState *state) {
    NcmBindingRuntime runtime;

    runtime.can_run_action = runtime_can_run_action;
    runtime.run_action = runtime_run_action;
    runtime.current_screen_is = runtime_current_screen_is;
    runtime.push_key = runtime_push_key;
    runtime.run_external_command = runtime_run_external_command;
    runtime.run_external_console_command = runtime_run_external_console_command;
    runtime.user = state;
    return runtime;
}

static void
test_default_bindings(void) {
    NcmBindingsConfiguration bindings;
    NcmBindingSlice slice;
    NcKey key;

    ncm_bindings_configuration_init(&bindings);
    ncm_bindings_configuration_generate_defaults(&bindings);

    key = ncm_bindings_string_to_key(LIT_ARGS("q"));
    slice = ncm_bindings_configuration_get(&bindings, key);
    REQUIRE_INT(slice.len, 1);
    REQUIRE_INT(slice.data[0].actions_len, 1);
    REQUIRE_INT(slice.data[0].actions[0].type, NCM_ACTION_QUIT);

    key = ncm_bindings_string_to_key(LIT_ARGS("shift-up"));
    slice = ncm_bindings_configuration_get(&bindings, key);
    REQUIRE_INT(slice.len, 1);
    REQUIRE_INT(slice.data[0].actions_len, 2);
    REQUIRE_INT(slice.data[0].actions[0].type, NCM_ACTION_SELECT_ITEM);
    REQUIRE_INT(slice.data[0].actions[1].type, NCM_ACTION_SCROLL_UP);

    ncm_bindings_configuration_destroy(&bindings);
    return;
}

static void
test_key_lookup(void) {
    NcmBindingsConfiguration bindings;
    NcmBindingSlice slice;
    NcmError error;
    char template[] = "/tmp/ncmpcpp-bindings-key-XXXXXX";
    int32 fd;

    fd = mkstemp(template);
    REQUIRE(fd >= 0);
    REQUIRE(close(fd) == 0);
    write_file(template,
               LIT_ARGS("def_key \"ctrl-x\"\n"
                        "  quit\n"));

    ncm_error_clear(&error);
    ncm_bindings_configuration_init(&bindings);
    REQUIRE(ncm_bindings_configuration_read(&bindings, template,
                                            (int32)(SIZEOF(template) - 1),
                                            &error));
    slice = ncm_bindings_configuration_get(
        &bindings, ncm_bindings_string_to_key(LIT_ARGS("ctrl-x")));
    REQUIRE_INT(slice.len, 1);
    REQUIRE_INT(slice.data[0].actions_len, 1);
    REQUIRE_INT(slice.data[0].actions[0].type, NCM_ACTION_QUIT);

    ncm_bindings_configuration_destroy(&bindings);
    REQUIRE(unlink(template) == 0);
    return;
}

static void
test_command_lookup(void) {
    NcmBindingsConfiguration bindings;
    NcmCommand *command;
    NcmError error;
    char template[] = "/tmp/ncmpcpp-bindings-command-XXXXXX";
    int32 fd;

    fd = mkstemp(template);
    REQUIRE(fd >= 0);
    REQUIRE(close(fd) == 0);
    write_file(template,
               LIT_ARGS("def_command \"save-it\" [deferred]\n"
                        "  save_playlist\n"));

    ncm_error_clear(&error);
    ncm_bindings_configuration_init(&bindings);
    REQUIRE(ncm_bindings_configuration_read(&bindings, template,
                                            (int32)(SIZEOF(template) - 1),
                                            &error));
    command = ncm_bindings_configuration_find_command(
        &bindings, LIT_ARGS("save-it"));
    REQUIRE(command != NULL);
    REQUIRE(!command->immediate);
    REQUIRE_INT(command->binding.actions_len, 1);
    REQUIRE_INT(command->binding.actions[0].type, NCM_ACTION_SAVE_PLAYLIST);

    ncm_bindings_configuration_destroy(&bindings);
    REQUIRE(unlink(template) == 0);
    return;
}

static void
test_binding_file_parsing(void) {
    NcmBindingsConfiguration bindings;
    NcmBindingSlice slice;
    NcmError error;
    char template[] = "/tmp/ncmpcpp-bindings-parse-XXXXXX";
    int32 fd;

    fd = mkstemp(template);
    REQUIRE(fd >= 0);
    REQUIRE(close(fd) == 0);
    write_file(template,
               LIT_ARGS("def_key \"ctrl-y\"\n"
                        "  push_characters \"ab\"\n"
                        "  require_screen \"playlist\"\n"
                        "  quit\n"));

    ncm_error_clear(&error);
    ncm_bindings_configuration_init(&bindings);
    REQUIRE(ncm_bindings_configuration_read(&bindings, template,
                                            (int32)(SIZEOF(template) - 1),
                                            &error));
    slice = ncm_bindings_configuration_get(
        &bindings, ncm_bindings_string_to_key(LIT_ARGS("ctrl-y")));
    REQUIRE_INT(slice.len, 1);
    REQUIRE_INT(slice.data[0].actions_len, 3);
    REQUIRE_INT(slice.data[0].actions[0].kind,
                NCM_BINDING_ACTION_PUSH_CHARACTERS);
    REQUIRE_INT(slice.data[0].actions[0].keys_len, 2);
    REQUIRE_INT(slice.data[0].actions[0].keys[0], 'a');
    REQUIRE_INT(slice.data[0].actions[0].keys[1], 'b');
    REQUIRE_INT(slice.data[0].actions[1].kind,
                NCM_BINDING_ACTION_REQUIRE_SCREEN);
    REQUIRE_INT(slice.data[0].actions[2].type, NCM_ACTION_QUIT);

    ncm_bindings_configuration_destroy(&bindings);
    REQUIRE(unlink(template) == 0);
    return;
}

static void
test_chained_action_execution(void) {
    TestRunnerState state;
    NcmBinding binding;

    state = (TestRunnerState){0};
    ncm_binding_init(&binding);
    append_action(&binding, NCM_ACTION_SELECT_ITEM);
    append_action(&binding, NCM_ACTION_SCROLL_UP);
    append_action(&binding, NCM_ACTION_QUIT);

    REQUIRE(ncm_binding_execute(&binding, record_action, &state));
    REQUIRE_INT(state.seen_len, 3);
    REQUIRE_INT(state.seen[0], NCM_ACTION_SELECT_ITEM);
    REQUIRE_INT(state.seen[1], NCM_ACTION_SCROLL_UP);
    REQUIRE_INT(state.seen[2], NCM_ACTION_QUIT);

    state = (TestRunnerState){.fail_at = 2};
    REQUIRE(!ncm_binding_execute(&binding, record_action, &state));
    REQUIRE_INT(state.seen_len, 2);
    REQUIRE_INT(state.seen[0], NCM_ACTION_SELECT_ITEM);
    REQUIRE_INT(state.seen[1], NCM_ACTION_SCROLL_UP);

    ncm_binding_destroy(&binding);
    return;
}

static void
test_runtime_action_execution(void) {
    TestRuntimeState state;
    NcmBindingsConfiguration bindings;
    NcmBindingRuntime runtime;
    NcmBindingSlice slice;
    NcmError error;
    char template[] = "/tmp/ncmpcpp-bindings-runtime-XXXXXX";
    int32 fd;

    fd = mkstemp(template);
    REQUIRE(fd >= 0);
    REQUIRE(close(fd) == 0);
    write_file(template,
               LIT_ARGS("def_key \"ctrl-z\"\n"
                        "  require_screen \"playlist\"\n"
                        "  push_characters \"x\"\n"
                        "  run_external_command \"notify hi\"\n"
                        "  run_external_console_command \"clear\"\n"
                        "  quit\n"));

    ncm_error_clear(&error);
    ncm_bindings_configuration_init(&bindings);
    REQUIRE(ncm_bindings_configuration_read(&bindings, template,
                                            (int32)(SIZEOF(template) - 1),
                                            &error));
    slice = ncm_bindings_configuration_get(
        &bindings, ncm_bindings_string_to_key(LIT_ARGS("ctrl-z")));
    REQUIRE_INT(slice.len, 1);

    state = (TestRuntimeState){0};
    state.current_screen = NCM_SCREEN_TYPE_PLAYLIST;
    runtime = runtime_make(&state);
    REQUIRE(ncm_binding_execute_runtime(slice.data, &runtime));
    REQUIRE_INT(state.pushed_len, 1);
    REQUIRE_INT(state.pushed[0], 'x');
    REQUIRE_INT(state.ran_len, 1);
    REQUIRE_INT(state.ran[0], NCM_ACTION_QUIT);
    REQUIRE_STRING(state.external, state.external_len, "notify hi");
    REQUIRE_STRING(state.console, state.console_len, "clear");

    state = (TestRuntimeState){0};
    state.current_screen = NCM_SCREEN_TYPE_BROWSER;
    runtime = runtime_make(&state);
    REQUIRE(!ncm_binding_execute_runtime(slice.data, &runtime));

    ncm_bindings_configuration_destroy(&bindings);
    REQUIRE(unlink(template) == 0);
    return;
}

static void
test_runtime_can_execute_preflight(void) {
    TestRuntimeState state;
    NcmBinding binding;
    NcmBindingRuntime runtime;

    ncm_binding_init(&binding);
    append_action(&binding, NCM_ACTION_QUIT);
    append_action(&binding, NCM_ACTION_STOP);

    state = (TestRuntimeState){0};
    runtime = runtime_make(&state);
    REQUIRE(ncm_binding_can_execute_runtime(&binding, &runtime));
    REQUIRE(ncm_binding_execute_runtime(&binding, &runtime));
    REQUIRE_INT(state.ran_len, 2);
    REQUIRE_INT(state.ran[0], NCM_ACTION_QUIT);
    REQUIRE_INT(state.ran[1], NCM_ACTION_STOP);

    state = (TestRuntimeState){0};
    state.block_action = true;
    state.blocked_action = NCM_ACTION_STOP;
    runtime = runtime_make(&state);
    REQUIRE(!ncm_binding_can_execute_runtime(&binding, &runtime));
    REQUIRE_INT(state.ran_len, 0);

    ncm_binding_destroy(&binding);
    return;
}

static void
test_app_binding_migration_c_safe_actions(void) {
    NcmBinding binding;

    REQUIRE(app_binding_migration_action_is_c_safe(NCM_ACTION_DUMMY));
    REQUIRE(app_binding_migration_action_is_c_safe(NCM_ACTION_PLAY));
    REQUIRE(app_binding_migration_action_is_c_safe(NCM_ACTION_MOUSE_EVENT));
    REQUIRE(app_binding_migration_action_is_c_safe(NCM_ACTION_SCROLL_UP));
    REQUIRE(app_binding_migration_action_is_c_safe(NCM_ACTION_SHUFFLE));
    REQUIRE(app_binding_migration_action_is_c_safe(
        NCM_ACTION_TOGGLE_LIBRARY_TAG_TYPE));
    REQUIRE(app_binding_migration_action_is_c_safe(
        NCM_ACTION_SHOW_VISUALIZER));
    REQUIRE(app_binding_migration_action_is_c_safe(
        NCM_ACTION_TOGGLE_VISUALIZATION_TYPE));
    REQUIRE(!app_binding_migration_action_is_c_safe(NCM_ACTION_LAST));

    REQUIRE(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_BROWSER));
    REQUIRE(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_HELP));
    REQUIRE(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_LASTFM));
    REQUIRE(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_LYRICS));
    REQUIRE(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_MEDIA_LIBRARY));
    REQUIRE(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_PLAYLIST));
    REQUIRE(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_PLAYLIST_EDITOR));
    REQUIRE(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_SEARCH_ENGINE));
    REQUIRE(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER));
    REQUIRE(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_SERVER_INFO));
    REQUIRE(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_SONG_INFO));
    REQUIRE(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG));
#if defined(HAVE_TAGLIB_H)
    REQUIRE(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_TAG_EDITOR));
    REQUIRE(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_TINY_TAG_EDITOR));
#endif
#if defined(ENABLE_VISUALIZER)
    REQUIRE(app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_VISUALIZER));
#endif
    REQUIRE(!app_binding_migration_screen_is_c_only(
        NCM_SCREEN_TYPE_UNKNOWN));
    REQUIRE(!app_binding_migration_screen_is_c_only(NCM_SCREEN_TYPE_LAST));

    REQUIRE(app_binding_migration_action_is_c_safe_for_screen(
        NCM_ACTION_DELETE_BROWSER_ITEMS, NCM_SCREEN_TYPE_PLAYLIST));
    REQUIRE(app_binding_migration_action_is_c_safe_for_screen(
        NCM_ACTION_SHOW_VISUALIZER, NCM_SCREEN_TYPE_PLAYLIST));
    REQUIRE(!app_binding_migration_action_is_c_safe_for_screen(
        NCM_ACTION_PLAY, NCM_SCREEN_TYPE_UNKNOWN));

    ncm_binding_init(&binding);
    append_action(&binding, NCM_ACTION_QUIT);
    append_action(&binding, NCM_ACTION_TOGGLE_LIBRARY_TAG_TYPE);
    REQUIRE(app_binding_migration_binding_is_c_safe(&binding));
    REQUIRE(app_binding_migration_binding_is_hybrid_safe(&binding));
    REQUIRE(app_binding_migration_binding_is_plain_action_sequence(
        &binding));
    REQUIRE(app_binding_migration_binding_is_c_safe_for_screen(
        &binding, NCM_SCREEN_TYPE_MEDIA_LIBRARY));
    REQUIRE(!app_binding_migration_binding_is_c_safe_for_screen(
        &binding, NCM_SCREEN_TYPE_UNKNOWN));
    ncm_binding_destroy(&binding);

    ncm_binding_init(&binding);
    append_action_kind(&binding, NCM_BINDING_ACTION_REQUIRE_SCREEN,
                       NCM_ACTION_DUMMY);
    append_action_kind(&binding, NCM_BINDING_ACTION_PUSH_CHARACTERS,
                       NCM_ACTION_DUMMY);
    append_action_kind(&binding,
                       NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND,
                       NCM_ACTION_DUMMY);
    append_action(&binding, NCM_ACTION_QUIT);
    REQUIRE(app_binding_migration_binding_is_c_safe(&binding));
    REQUIRE(app_binding_migration_binding_is_hybrid_safe(&binding));
    REQUIRE(!app_binding_migration_binding_is_plain_action_sequence(
        &binding));
    ncm_binding_destroy(&binding);

    ncm_binding_init(&binding);
    REQUIRE(!app_binding_migration_binding_is_c_safe(&binding));
    REQUIRE(!app_binding_migration_binding_is_hybrid_safe(&binding));
    REQUIRE(!app_binding_migration_binding_is_plain_action_sequence(
        &binding));
    ncm_binding_destroy(&binding);

    return;
}

static void
test_binding_action_formatting(void) {
    NcmBindingsConfiguration bindings;
    NcmBindingSlice slice;
    NcmBuffer buffer;
    NcmError error;
    char template[] = "/tmp/ncmpcpp-bindings-format-XXXXXX";
    int32 fd;

    fd = mkstemp(template);
    REQUIRE(fd >= 0);
    REQUIRE(close(fd) == 0);
    write_file(template,
               LIT_ARGS("def_key \"ctrl-j\"\n"
                        "  require_screen \"playlist\"\n"
                        "  run_external_command \"echo ok\"\n"
                        "  quit\n"));

    ncm_error_clear(&error);
    ncm_bindings_configuration_init(&bindings);
    REQUIRE(ncm_bindings_configuration_read(&bindings, template,
                                            (int32)(SIZEOF(template) - 1),
                                            &error));
    slice = ncm_bindings_configuration_get(
        &bindings, ncm_bindings_string_to_key(LIT_ARGS("ctrl-j")));
    REQUIRE_INT(slice.len, 1);

    ncm_buffer_init(&buffer);
    ncm_binding_action_format(&buffer, slice.data[0].actions + 0);
    REQUIRE_STRING(buffer.data, buffer.len,
                   "require_screen \"playlist\"");
    ncm_buffer_clear(&buffer);
    ncm_binding_action_format(&buffer, slice.data[0].actions + 1);
    REQUIRE_STRING(buffer.data, buffer.len,
                   "run_external_command \"echo ok\"");
    ncm_buffer_clear(&buffer);
    ncm_binding_action_format(&buffer, slice.data[0].actions + 2);
    REQUIRE_STRING(buffer.data, buffer.len, "quit");
    ncm_buffer_destroy(&buffer);

    ncm_bindings_configuration_destroy(&bindings);
    REQUIRE(unlink(template) == 0);
    return;
}

static void
test_read_paths(void) {
    NcmBindingsConfiguration bindings;
    NcmCommand *command;
    NcmError error;
    char first[] = "/tmp/ncmpcpp-bindings-path-a-XXXXXX";
    char second[] = "/tmp/ncmpcpp-bindings-path-b-XXXXXX";
    char *paths[2];
    int32 path_lens[2];
    int32 fd;

    fd = mkstemp(first);
    REQUIRE(fd >= 0);
    REQUIRE(close(fd) == 0);
    fd = mkstemp(second);
    REQUIRE(fd >= 0);
    REQUIRE(close(fd) == 0);
    write_file(first,
               LIT_ARGS("def_command \"first-command\" [deferred]\n"
                        "  quit\n"));
    write_file(second,
               LIT_ARGS("def_key \"ctrl-q\"\n"
                        "  quit\n"));

    paths[0] = first;
    paths[1] = second;
    path_lens[0] = (int32)(SIZEOF(first) - 1);
    path_lens[1] = (int32)(SIZEOF(second) - 1);

    ncm_error_clear(&error);
    ncm_bindings_configuration_init(&bindings);
    REQUIRE(ncm_bindings_configuration_read_paths(&bindings, paths,
                                                  path_lens, 2, &error));
    command = ncm_bindings_configuration_find_command(
        &bindings, LIT_ARGS("first-command"));
    REQUIRE(command != NULL);
    REQUIRE_INT(command->binding.actions[0].type, NCM_ACTION_QUIT);

    ncm_bindings_configuration_destroy(&bindings);
    REQUIRE(unlink(first) == 0);
    REQUIRE(unlink(second) == 0);
    return;
}

static void
test_key_formatting(void) {
    NcmBuffer buffer;
    NcKey key;

    ncm_buffer_init(&buffer);
    key = ncm_bindings_string_to_key(LIT_ARGS("alt-shift-up"));
    ncm_bindings_format_key(&buffer, key);
    REQUIRE_STRING(buffer.data, buffer.len, "Alt-Shift-Up");
    ncm_buffer_destroy(&buffer);
    return;
}

int
main(void) {
    test_default_bindings();
    test_key_lookup();
    test_command_lookup();
    test_binding_file_parsing();
    test_chained_action_execution();
    test_runtime_action_execution();
    test_runtime_can_execute_preflight();
    test_app_binding_migration_c_safe_actions();
    test_binding_action_formatting();
    test_read_paths();
    test_key_formatting();
    exit(EXIT_SUCCESS);
}
