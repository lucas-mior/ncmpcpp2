#if !defined(NCMPCPP_TESTS_MACRO_PROCESS_C)
#define NCMPCPP_TESTS_MACRO_PROCESS_C

#define CBASE_IMPLEMENT
#include "cbase.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#include "c/ncm_error.c"
#include "c/ncm_macro_utilities.c"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

static void
macro_process_seed_error(NcmError *error) {
    ncm_error_set(error, 999, STRLIT("seed error"));
    return;
}

static int32
macro_process_temp_path(char *path, int32 path_cap, char *suffix) {
    int32 path_len;

    path_len = snprintf2(path, path_cap,
                         "/tmp/ncmpcpp2-macro-process-%lld-%s",
                         (llong)getpid(), suffix);
    return path_len;
}

static bool
macro_process_file_exists(char *path) {
    return access(path, F_OK) == 0;
}

static void
macro_process_wait_for_file(char *path) {
    for (int32 i = 0; i < 200; i += 1) {
        if (macro_process_file_exists(path)) {
            return;
        }
        usleep(10000);
    }

    ASSERT(macro_process_file_exists(path));
    return;
}

static void
macro_process_test_blocking_shell_syntax_and_status(void) {
    char command[] = "exit $((6*7))";
    NcmError error = {0};
    int32 status;
    bool success;

    macro_process_seed_error(&error);
    success = ncm_macro_system_command(command, strlen32(command),
                                       true, &status, &error);

    ASSERT(success);
    ASSERT_EQUAL(status, 42);
    ASSERT(!ncm_error_is_set(&error));
    return;
}

static void
macro_process_test_blocking_nonzero_exit_is_not_process_error(void) {
    char command[] = "exit 37";
    NcmError error = {0};
    int32 status;
    bool success;

    macro_process_seed_error(&error);
    success = ncm_macro_system_command(command, strlen32(command),
                                       true, &status, &error);

    ASSERT(success);
    ASSERT_EQUAL(status, 37);
    ASSERT(!ncm_error_is_set(&error));
    return;
}

static void
macro_process_test_empty_blocking_command_matches_shell(void) {
    char command[] = "";
    NcmError error = {0};
    int32 status;
    bool success;

    macro_process_seed_error(&error);
    success = ncm_macro_system_command(command, 0,
                                       true, &status, &error);

    ASSERT(success);
    ASSERT_ZERO(status);
    ASSERT(!ncm_error_is_set(&error));
    return;
}

static void
macro_process_test_external_console_command_uses_shell(void) {
    char command[] = "if true; then exit 0; else exit 31; fi";
    NcmError error = {0};
    bool success;

    macro_process_seed_error(&error);
    success = ncm_macro_run_external_console_command(command,
                                                     strlen32(command),
                                                     &error);

    ASSERT(success);
    ASSERT(!ncm_error_is_set(&error));
    return;
}

static void
macro_process_test_external_command_nonzero_exit_still_succeeds(void) {
    char command[] = "exit 19";
    NcmError error = {0};
    bool success;

    macro_process_seed_error(&error);
    success = ncm_macro_run_external_command(command, strlen32(command),
                                             true, &error);

    ASSERT(success);
    ASSERT(!ncm_error_is_set(&error));
    return;
}

static void
macro_process_test_invalid_blocking_command_sets_error(void) {
    NcmError error = {0};
    int32 status = -123;
    bool success;

    success = ncm_macro_system_command(NULL, 1,
                                       true, &status, &error);

    ASSERT(!success);
    ASSERT_EQUAL(status, -123);
    ASSERT(ncm_error_is_set(&error));
    ASSERT_EQUAL(error.code, EINVAL);
    ASSERT_EQUAL(error.message, "invalid shell command");
    return;
}

static void
macro_process_test_invalid_blocking_command_length_sets_error(void) {
    char command[] = "exit 0";
    NcmError error = {0};
    int32 status = -123;
    bool success;

    success = ncm_macro_system_command(command, -1,
                                       true, &status, &error);

    ASSERT(!success);
    ASSERT_EQUAL(status, -123);
    ASSERT(ncm_error_is_set(&error));
    ASSERT_EQUAL(error.code, EINVAL);
    ASSERT_EQUAL(error.message, "invalid shell command");
    return;
}

static void
macro_process_test_nonblocking_launches_shell_background_command(void) {
    char path[256];
    char command[384];
    NcmError error = {0};
    int32 path_len;
    int32 command_len;
    int32 status;
    bool success;

    path_len = macro_process_temp_path(path, SIZEOF(path), "async");
    unlink(path);
    command_len = snprintf2(command, SIZEOF(command),
                            "touch '%.*s'", path_len, path);

    macro_process_seed_error(&error);
    success = ncm_macro_system_command(command, command_len,
                                       false, &status, &error);

    ASSERT(success);
    ASSERT_ZERO(status);
    ASSERT(!ncm_error_is_set(&error));
    macro_process_wait_for_file(path);
    unlink(path);
    return;
}

int
main(void) {
    macro_process_test_blocking_shell_syntax_and_status();
    macro_process_test_blocking_nonzero_exit_is_not_process_error();
    macro_process_test_empty_blocking_command_matches_shell();
    macro_process_test_external_console_command_uses_shell();
    macro_process_test_external_command_nonzero_exit_still_succeeds();
    macro_process_test_invalid_blocking_command_sets_error();
    macro_process_test_invalid_blocking_command_length_sets_error();
    macro_process_test_nonblocking_launches_shell_background_command();
    return 0;
}

#endif /* NCMPCPP_TESTS_MACRO_PROCESS_C */
