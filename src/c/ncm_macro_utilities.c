#if !defined(NCM_MACRO_UTILITIES_C)
#define NCM_MACRO_UTILITIES_C

#include "cbase.h"

#include "c/ncm_c.h"

static bool
ncm_macro_system_command(char *command, int32 command_len,
                         bool block, int32 *status, NcmError *ncm_error) {
    StrBuilder buffer = {0};
    Command process = {0};
    int32 rc;
    bool success;

    if ((command == NULL) || (command_len < 0)) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("invalid shell command"));
        return false;
    }

    if (block) {
        COMMAND_PUSH(&process, "/bin/sh", "-c");
        command_push_length(&process, command, command_len);

        success = command_run_sync(&process, &rc) == 0;
        if (success) {
            if (status) {
                *status = rc;
            }
            ncm_error_clear(ncm_error);
        } else {
            ncm_error_set(ncm_error, process.error_status,
                          STRLIT("command failed"));
        }
        command_free(&process);
        return success;
    }

    SB_APPEND(&buffer, command, command_len);
    SB_APPEND(&buffer, " >/dev/null 2>&1 &");

    COMMAND_PUSH(&process, "/bin/sh", "-c", buffer.data);

    success = command_run_sync(&process, &rc) == 0;
    sb_free(&buffer);
    if (success) {
        if (status) {
            *status = rc;
        }
        ncm_error_clear(ncm_error);
    } else {
        ncm_error_set(ncm_error, process.error_status,
                      STRLIT("command failed"));
    }
    command_free(&process);
    return success;
}

bool
ncm_macro_run_external_command(char *command, int32 command_len,
                               bool block, NcmError *ncm_error) {
    int32 status;

    return ncm_macro_system_command(command, command_len,
                                    block, &status, ncm_error);
}

bool
ncm_macro_run_external_console_command(char *command,
                                       int32 command_len,
                                       NcmError *ncm_error) {
    int32 status;

    return ncm_macro_system_command(command, command_len,
                                    true, &status, ncm_error);
}

#endif /* NCM_MACRO_UTILITIES_C */
