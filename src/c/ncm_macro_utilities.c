#include "c/ncm_macro_utilities.h"

#include <errno.h>
#include <stdlib.h>

#include "c/ncm_base.h"
#include "c/ncm_process.h"
#include "cbase/base_macros.h"

static bool
ncm_macro_system_command(char *command, int32 command_len,
                         bool block, int32 *status, NcmError *error) {
    NcmBuffer buffer;
    int32 rc;

    if (block) {
        return ncm_process_run_shell(command, command_len, status, error);
    }

    ncm_buffer_init(&buffer);
    if (command_len > 0) {
        ncm_buffer_append(&buffer, command, command_len);
    }
    ncm_buffer_append(&buffer, STRLIT_ARGS(" >/dev/null 2>&1 &"));
    ncm_buffer_append_byte(&buffer, '\0');

    rc = system(buffer.data);
    ncm_buffer_destroy(&buffer);
    if (status) {
        *status = rc;
    }
    if (rc == -1) {
        ncm_error_set(error, errno, STRLIT_ARGS("system failed"));
        return false;
    }

    ncm_error_clear(error);
    return true;
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
