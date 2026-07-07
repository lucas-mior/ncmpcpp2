#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static void fail(char *file, int32 line, char *condition);
static void require_int(char *file, int32 line, char *name,
                        int32 actual, int32 expected);
static void require_string(char *file, int32 line, char *name,
                           char *actual, int32 actual_len,
                           char *expected, int32 expected_len);
static void write_file(char *path, char *data, int32 data_len);
static void append_action(NcmBinding *binding, enum NcmActionType type);
static bool record_action(NcmBindingAction *action, void *user);
static void test_default_bindings(void);
static void test_key_lookup(void);
static void test_command_lookup(void);
static void test_binding_file_parsing(void);
static void test_chained_action_execution(void);
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
    NcmBindingAction action;

    ncm_binding_action_init(&action);
    action.kind = NCM_BINDING_ACTION_NORMAL;
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
                                            (int32)strlen(template),
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
                                            (int32)strlen(template),
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
                                            (int32)strlen(template),
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
    test_key_formatting();
    exit(EXIT_SUCCESS);
}
