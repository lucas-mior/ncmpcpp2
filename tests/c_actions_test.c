#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "actions.h"
#include "bindings.h"
#include "c/ncm_error.h"
#include "c/ncm_base.h"
#include "cbase/base_macros.h"
#include "cbase/cbase.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

#define REQUIRE(COND) do {                                                \
    if (!(COND)) {                                                        \
        fail(__FILE__, __LINE__, (char *)#COND);                          \
    }                                                                     \
} while (0)

#define REQUIRE_INT(ACTUAL, EXPECTED)                                     \
    require_int(__FILE__, __LINE__, (char *)#ACTUAL, ACTUAL, EXPECTED)

#define REQUIRE_STRING(ACTUAL, EXPECTED)                                  \
    require_string(__FILE__, __LINE__, (char *)#ACTUAL, ACTUAL,           \
                   LIT_ARGS(EXPECTED))

typedef struct ActionTestState {
    int32 calls;
} ActionTestState;

static void fail(char *file, int32 line, char *condition);
static void require_int(char *file, int32 line, char *name,
                        int32 actual, int32 expected);
static int32 test_string_len(char *string);
static void require_string(char *file, int32 line, char *name,
                           char *actual, char *expected,
                           int32 expected_len);
static bool can_run_true(void *user);
static bool can_run_false(void *user);
static bool run_record(void *user);
static void test_action_name_lookup(void);
static void test_search_engine_action_lookup(void);
static void test_duplicate_action_detection(void);
static void test_disabled_action_checks(void);
static void test_action_execution_paths(void);
static void test_custom_table_lookup(void);
static void test_action_callback_installation(void);
static void test_immediate_command_prompt_stop(void);

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

static int32
test_string_len(char *string) {
    int32 len;

    len = 0;
    while (string[len] != '\0') {
        len += 1;
    }
    return len;
}

static void
require_string(char *file, int32 line, char *name,
               char *actual, char *expected, int32 expected_len) {
    if ((test_string_len(actual) != expected_len)
        || (memcmp(actual, expected, (size_t)expected_len) != 0)) {
        fprintf(stderr, "%s:%d: %s: expected '%.*s', got '%s'\n",
                file, line, name, expected_len, expected, actual);
        exit(EXIT_FAILURE);
    }
    return;
}

static bool
can_run_true(void *user) {
    (void)user;
    return true;
}

static bool
can_run_false(void *user) {
    (void)user;
    return false;
}

static bool
run_record(void *user) {
    ActionTestState *state;

    state = user;
    state->calls += 1;
    return true;
}

static void
test_action_name_lookup(void) {
    enum NcmActionType type;
    NcmActionDef *action;
    NcmError error;

    ncm_error_clear(&error);
    REQUIRE(ncm_action_validate(&error));
    REQUIRE(!ncm_error_is_set(&error));
    REQUIRE_INT(ncm_action_count(), NCM_ACTION_LAST);

    action = ncm_action_get(NCM_ACTION_QUIT);
    REQUIRE(action != NULL);
    REQUIRE_INT(action->type, NCM_ACTION_QUIT);
    REQUIRE_STRING(action->name, "quit");
    REQUIRE_STRING(ncm_action_type_name(NCM_ACTION_QUIT), "quit");

    action = ncm_action_find(LIT_ARGS("show_browser"));
    REQUIRE(action != NULL);
    REQUIRE_INT(action->type, NCM_ACTION_SHOW_BROWSER);

    REQUIRE(ncm_action_type_parse(LIT_ARGS("pause"), &type));
    REQUIRE_INT(type, NCM_ACTION_PAUSE);
    REQUIRE(!ncm_action_type_parse(LIT_ARGS("not_an_action"), &type));
    REQUIRE(ncm_action_get(NCM_ACTION_MACRO_UTILITY) == NULL);
    return;
}

static void
test_search_engine_action_lookup(void) {
    NcmActionDef *action;

    action = ncm_action_find(LIT_ARGS("show_search_engine"));
    REQUIRE(action != NULL);
    REQUIRE_INT(action->type, NCM_ACTION_SHOW_SEARCH_ENGINE);

    action = ncm_action_find(LIT_ARGS("reset_search_engine"));
    REQUIRE(action != NULL);
    REQUIRE_INT(action->type, NCM_ACTION_RESET_SEARCH_ENGINE);

    action = ncm_action_find(LIT_ARGS("start_searching"));
    REQUIRE(action != NULL);
    REQUIRE_INT(action->type, NCM_ACTION_START_SEARCHING);

    action = ncm_action_find(LIT_ARGS("toggle_display_mode"));
    REQUIRE(action != NULL);
    REQUIRE_INT(action->type, NCM_ACTION_TOGGLE_DISPLAY_MODE);
    return;
}

static void
test_duplicate_action_detection(void) {
    NcmActionDef duplicate_type[] = {
        { (char *)"one", NCM_ACTION_DUMMY, can_run_true, run_record },
        { (char *)"two", NCM_ACTION_DUMMY, can_run_true, run_record },
    };
    NcmActionDef duplicate_name[] = {
        { (char *)"same", NCM_ACTION_DUMMY, can_run_true, run_record },
        { (char *)"same", NCM_ACTION_QUIT, can_run_true, run_record },
    };
    NcmError error;

    ncm_error_clear(&error);
    REQUIRE(!ncm_action_table_validate(duplicate_type,
                                       NCM_ARRAY_LEN(duplicate_type),
                                       &error));
    REQUIRE(ncm_error_is_set(&error));

    ncm_error_clear(&error);
    REQUIRE(!ncm_action_table_validate(duplicate_name,
                                       NCM_ARRAY_LEN(duplicate_name),
                                       &error));
    REQUIRE(ncm_error_is_set(&error));
    return;
}

static void
test_disabled_action_checks(void) {
    ActionTestState state;
    NcmActionDef disabled;

    state = (ActionTestState){0};
    disabled = (NcmActionDef){
        (char *)"disabled",
        NCM_ACTION_DUMMY,
        can_run_false,
        run_record,
    };

    REQUIRE(!ncm_action_def_can_run(&disabled, &state));
    REQUIRE(!ncm_action_def_run(&disabled, &state));
    REQUIRE_INT(state.calls, 0);
    REQUIRE(!ncm_action_can_run(NCM_ACTION_MACRO_UTILITY, &state));
    REQUIRE(!ncm_action_run(NCM_ACTION_MACRO_UTILITY, &state));
    return;
}

static void
test_action_execution_paths(void) {
    ActionTestState state;
    NcmActionDef action;

    state = (ActionTestState){0};
    action = (NcmActionDef){
        (char *)"record",
        NCM_ACTION_DUMMY,
        can_run_true,
        run_record,
    };

    REQUIRE(ncm_action_def_run(&action, &state));
    REQUIRE_INT(state.calls, 1);
    REQUIRE(ncm_action_run(NCM_ACTION_DUMMY, NULL));
    REQUIRE(!ncm_action_run(NCM_ACTION_PAUSE, NULL));
    return;
}


static void
test_action_callback_installation(void) {
    ActionTestState state;

    state = (ActionTestState){0};
    REQUIRE(ncm_action_set_callbacks(NCM_ACTION_DUMMY, can_run_true,
                                     run_record));
    REQUIRE(ncm_action_run(NCM_ACTION_DUMMY, &state));
    REQUIRE_INT(state.calls, 1);
    REQUIRE(!ncm_action_set_callbacks(NCM_ACTION_MACRO_UTILITY,
                                      can_run_true, run_record));
    return;
}

static void
test_custom_table_lookup(void) {
    NcmActionDef table[] = {
        { (char *)"quit", NCM_ACTION_QUIT, can_run_true, run_record },
        { (char *)"show_help", NCM_ACTION_SHOW_HELP,
          can_run_true, run_record },
    };
    NcmActionDef *action;
    NcmError error;

    ncm_error_clear(&error);
    REQUIRE(ncm_action_table_validate(table, NCM_ARRAY_LEN(table), &error));
    action = ncm_action_table_get(table, NCM_ARRAY_LEN(table),
                                  NCM_ACTION_SHOW_HELP);
    REQUIRE(action != NULL);
    REQUIRE_STRING(action->name, "show_help");
    action = ncm_action_table_find(table, NCM_ARRAY_LEN(table),
                                   LIT_ARGS("quit"));
    REQUIRE(action != NULL);
    REQUIRE_INT(action->type, NCM_ACTION_QUIT);
    return;
}

static void
test_command_set(NcmCommand *command, char *name, int32 name_len,
                 bool immediate) {
    ncm_command_init(command);
    command->name = cbase_malloc(name_len + 1);
    cbase_memcpy(command->name, name, name_len);
    command->name[name_len] = '\0';
    command->name_len = name_len;
    command->name_cap = name_len + 1;
    command->immediate = immediate;
    return;
}

static void
test_immediate_command_prompt_stop(void) {
    NcmBuffer previous;

    ncm_bindings_configuration_init(&Bindings);
    Bindings.commands = cbase_malloc(2*SIZEOF(*Bindings.commands));
    Bindings.commands_len = 2;
    Bindings.commands_cap = 2;
    test_command_set(Bindings.commands + 0, LIT_ARGS("later"), false);
    test_command_set(Bindings.commands + 1, LIT_ARGS("now"), true);

    ncm_buffer_init(&previous);
    REQUIRE(!ncm_action_immediate_command_prompt_should_stop(
        &previous, LIT_ARGS("later")));
    REQUIRE_STRING(previous.data, "later");
    REQUIRE(!ncm_action_immediate_command_prompt_should_stop(
        &previous, LIT_ARGS("later")));
    REQUIRE(ncm_action_immediate_command_prompt_should_stop(
        &previous, LIT_ARGS("now")));
    REQUIRE_STRING(previous.data, "now");

    ncm_buffer_destroy(&previous);
    ncm_bindings_configuration_destroy(&Bindings);
    return;
}

int
main(void) {
    test_action_name_lookup();
    test_search_engine_action_lookup();
    test_duplicate_action_detection();
    test_disabled_action_checks();
    test_action_execution_paths();
    test_custom_table_lookup();
    test_action_callback_installation();
    test_immediate_command_prompt_stop();
    exit(EXIT_SUCCESS);
}
