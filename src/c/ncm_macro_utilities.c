#if !defined(NCM_MACRO_UTILITIES_C)
#define NCM_MACRO_UTILITIES_C

#include "c/ncm_macro_utilities.h"

#include <errno.h>
#include <stdlib.h>

#include "c/ncm_base.h"
#include "cbase/base_macros.h"
#include "cbase.h"

static bool
ncm_macro_system_command(char *command, int32 command_len,
                         bool block, int32 *status, NcmError *error) {
    StrBuilder buffer;
    Command process = {0};
    int32 rc;
    bool success;

    if (block) {
        if ((command == NULL) || (command_len < 0)) {
            ncm_error_set(error, EINVAL, STRLIT("invalid shell command"));
            return false;
        }

        command_push(&process, "/bin/sh");
        command_push(&process, "-c");
        command_push_length(&process, command, command_len);

        success = command_run_sync(&process, &rc);
        if (success) {
            if (status) {
                *status = rc;
            }
            ncm_error_clear(error);
        } else {
            ncm_error_set(error, process.error_status,
                          STRLIT("command failed"));
        }
        command_free(&process);
        return success;
    }

    sb_init(&buffer);
    if (command_len > 0) {
        SB_APPEND(&buffer, command, command_len);
    }
    SB_APPEND(&buffer, STRLIT(" >/dev/null 2>&1 &"));
    sb_append_byte(&buffer, '\0');

    command_push(&process, "/bin/sh");
    command_push(&process, "-c");
    command_push_length(&process, buffer.data, buffer.len - 1);

    success = command_run_sync(&process, &rc);
    sb_free(&buffer);
    if (success) {
        if (status) {
            *status = rc;
        }
        ncm_error_clear(error);
    } else {
        ncm_error_set(error, process.error_status,
                      STRLIT("command failed"));
    }
    command_free(&process);
    return success;
}

bool
ncm_macro_run_external_command(char *command, int32 command_len,
                               bool block, NcmError *error) {
    int32 status;

    return ncm_macro_system_command(command, command_len,
                                    block, &status, error);
}

bool
ncm_macro_run_external_console_command(char *command,
                                       int32 command_len,
                                       NcmError *error) {
    int32 status;

    return ncm_macro_system_command(command, command_len,
                                    true, &status, error);
}

#endif /* NCM_MACRO_UTILITIES_C */
