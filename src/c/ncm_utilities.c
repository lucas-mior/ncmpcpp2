#if !defined(NCM_UTILITIES_C)
#define NCM_UTILITIES_C

#include "cbase.h"
#include "ncmpcpp2.h"

#include "c/ncm_c.h"

static int32
ncm_system_command(char *command, int32 cmd_len,
                   bool block, int32 *status, NcmError *err) {
    String buffer = {0};
    Command process = {0};
    int32 rc;
    int32 result;

    if ((command == NULL) || (cmd_len < 0)) {
        return ncm_error_set_status(err, -EINVAL, SL("invalid shell command"));
    }

    if (block) {
        CMD_PUSH(&process, "/bin/sh", "-c");
        cmd_push_length(&process, command, cmd_len);

        result = cmd_run_sync(&process, &rc);
        if (result == 0) {
            if (status) {
                *status = rc;
            }
            result = ncm_error_ok(err);
        } else {
            result = ncm_error_set_status(err, process.error_status,
                                          SL("command failed"));
        }
        cmd_free(&process);
        return result;
    }

    STR_APPEND(&buffer, command, cmd_len);
    STR_APPEND(&buffer, " >/dev/null 2>&1 &");

    CMD_PUSH(&process, "/bin/sh", "-c", buffer.data);

    result = cmd_run_sync(&process, &rc);
    str_free(&buffer);
    if (result == 0) {
        if (status) {
            *status = rc;
        }
        result = ncm_error_ok(err);
    } else {
        result = ncm_error_set_status(err, process.error_status,
                                      SL("command failed"));
    }
    cmd_free(&process);
    return result;
}

int32
ncm_run_external_command(char *command, int32 cmd_len,
                         bool block, NcmError *err) {
    int32 status;

    return ncm_system_command(command, cmd_len, block, &status, err);
}

int32
ncm_run_external_console_command(char *command, int32 cmd_len, NcmError *err) {
    int32 status;

    return ncm_system_command(command, cmd_len, true, &status, err);
}

#endif /* NCM_UTILITIES_C */
