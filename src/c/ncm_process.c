#include "c/ncm_process.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"

static void ncm_process_set_errno_error(NcmError *error, int32 code,
                                        char *operation);
static void ncm_process_set_text_error(NcmError *error, int32 code,
                                       char *text, int32 text_len);
static bool ncm_process_command_argv_ready(NcmProcessCommand *command,
                                           NcmError *error);
static int32 ncm_process_wait_status(int32 status);
static bool ncm_process_wait(pid_t child, int32 *status, NcmError *error);
static bool ncm_process_command_add_cstring(NcmProcessCommand *command,
                                            char *arg);

static void
ncm_process_set_errno_error(NcmError *error, int32 code, char *operation) {
    char message[256];
    int32 message_len;

    message_len = snprintf(message, SIZEOF(message), "%s: %s",
                           operation, strerror(code));
    ncm_error_set(error, code, message, message_len);
    return;
}

static void
ncm_process_set_text_error(NcmError *error, int32 code,
                           char *text, int32 text_len) {
    ncm_error_set(error, code, text, text_len);
    return;
}

static bool
ncm_process_command_argv_ready(NcmProcessCommand *command, NcmError *error) {
    if (command == NULL) {
        ncm_process_set_text_error(error, EINVAL,
                                   STRLIT_ARGS("missing command"));
        return false;
    }
    if (command->argc <= 0) {
        ncm_process_set_text_error(error, EINVAL,
                                   STRLIT_ARGS("empty command"));
        return false;
    }
    if (command->argv == NULL) {
        ncm_process_set_text_error(error, EINVAL,
                                   STRLIT_ARGS("command has no argv"));
        return false;
    }

    command->argv[command->argc] = NULL;
    return true;
}

static int32
ncm_process_wait_status(int32 status) {
    int32 result;

    if (WIFEXITED(status)) {
        result = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result = 128 + WTERMSIG(status);
    } else {
        result = 127;
    }

    return result;
}

static bool
ncm_process_wait(pid_t child, int32 *status, NcmError *error) {
    int32 wait_status;

    while (waitpid(child, &wait_status, 0) < 0) {
        if (errno != EINTR) {
            ncm_process_set_errno_error(error, errno, "waitpid");
            return false;
        }
    }

    if (status) {
        *status = ncm_process_wait_status(wait_status);
    }
    ncm_error_clear(error);
    return true;
}

static bool
ncm_process_command_add_cstring(NcmProcessCommand *command, char *arg) {
    int32 arg_len;

    arg_len = strlen32(arg);
    return ncm_process_command_add_arg(command, arg, arg_len);
}

void
ncm_process_command_init(NcmProcessCommand *command) {
    command->argv = NULL;
    command->argv_lens = NULL;
    command->argc = 0;
    command->cap = 0;
    return;
}

void
ncm_process_command_destroy(NcmProcessCommand *command) {
    if (command == NULL) {
        return;
    }

    for (int32 i = 0; i < command->argc; i += 1) {
        if (command->argv[i]) {
            free2(command->argv[i], command->argv_lens[i] + 1);
        }
    }
    if (command->argv) {
        free2(command->argv,
            (command->cap + 1)*SIZEOF(*command->argv));
    }
    if (command->argv_lens) {
        free2(command->argv_lens,
            command->cap*SIZEOF(*command->argv_lens));
    }
    ncm_process_command_init(command);
    return;
}

bool
ncm_process_command_add_arg(NcmProcessCommand *command,
                            char *arg, int32 arg_len) {
    int32 old_cap;
    int32 new_cap;
    char *copy;

    if (command == NULL) {
        return false;
    }
    if (arg == NULL) {
        return false;
    }
    if (arg_len < 0) {
        return false;
    }

    if (command->argc + 1 >= command->cap) {
        old_cap = command->cap;
        new_cap = command->cap;
        if (new_cap <= 0) {
            new_cap = 8;
        }
        while (command->argc + 1 >= new_cap) {
            new_cap *= 2;
        }
        command->argv = realloc2(command->argv,
                               old_cap + 1, new_cap + 1,
                               SIZEOF(*command->argv));
        command->argv_lens = realloc2(command->argv_lens,
                                    old_cap, new_cap,
                                    SIZEOF(*command->argv_lens));
        command->cap = new_cap;
    }

    copy = malloc2(arg_len + 1);
    memcpy64(copy, arg, arg_len);
    copy[arg_len] = '\0';
    command->argv[command->argc] = copy;
    command->argv_lens[command->argc] = arg_len;
    command->argc += 1;
    command->argv[command->argc] = NULL;
    return true;
}

bool
ncm_process_run_sync(NcmProcessCommand *command, int32 *status,
                     NcmError *error) {
    pid_t child;

    if (!ncm_process_command_argv_ready(command, error)) {
        return false;
    }

    switch (child = fork()) {
    case -1:
        ncm_process_set_errno_error(error, errno, "fork");
        return false;
    case 0:
        execvp(command->argv[0], command->argv);
        _exit(127);
    default:
        break;
    }

    return ncm_process_wait(child, status, error);
}

void
ncm_process_result_init(NcmProcessResult *result) {
    ncm_buffer_init(&result->output);
    result->status = 0;
    return;
}

void
ncm_process_result_destroy(NcmProcessResult *result) {
    if (result == NULL) {
        return;
    }
    ncm_buffer_destroy(&result->output);
    result->status = 0;
    return;
}

bool
ncm_process_run_capture(NcmProcessCommand *command,
                        NcmProcessResult *result,
                        NcmError *error) {
    int32 pipefd[2];
    pid_t child;
    char read_buffer[4096];
    int64 bytes_read;
    bool ok;

    if (result == NULL) {
        ncm_process_set_text_error(error, EINVAL,
                                   STRLIT_ARGS("missing process result"));
        return false;
    }
    if (!ncm_process_command_argv_ready(command, error)) {
        return false;
    }
    if (pipe(pipefd) != 0) {
        ncm_process_set_errno_error(error, errno, "pipe");
        return false;
    }

    switch (child = fork()) {
    case -1:
        close(pipefd[0]);
        close(pipefd[1]);
        ncm_process_set_errno_error(error, errno, "fork");
        return false;
    case 0:
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execvp(command->argv[0], command->argv);
        _exit(127);
    default:
        break;
    }

    close(pipefd[1]);
    ncm_buffer_clear(&result->output);
    while ((bytes_read = read(pipefd[0], read_buffer,
                              (size_t)SIZEOF(read_buffer))) > 0) {
        ncm_buffer_append(&result->output, read_buffer, (int32)bytes_read);
    }
    if (bytes_read < 0) {
        ncm_process_set_errno_error(error, errno, "read");
        close(pipefd[0]);
        ncm_process_wait(child, &result->status, NULL);
        return false;
    }

    close(pipefd[0]);
    ok = ncm_process_wait(child, &result->status, error);
    return ok;
}

bool
ncm_process_run_shell(char *command, int32 command_len,
                      int32 *status, NcmError *error) {
    NcmProcessCommand process;
    bool result;

    ncm_process_command_init(&process);
    result = ncm_process_command_add_cstring(&process, "/bin/sh");
    result = result
             && ncm_process_command_add_cstring(&process, "-c");
    result = result
             && ncm_process_command_add_arg(&process, command, command_len);
    if (result) {
        result = ncm_process_run_sync(&process, status, error);
    } else {
        ncm_process_set_text_error(error, EINVAL,
                                   STRLIT_ARGS("invalid shell command"));
    }
    ncm_process_command_destroy(&process);
    return result;
}
