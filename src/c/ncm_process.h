#if !defined(NCM_PROCESS_H)
#define NCM_PROCESS_H

#include "c/ncm_base.h"

typedef struct NcmProcessCommand {
    char **argv;
    int32 *argv_lens;
    int32 argc;
    int32 cap;
} NcmProcessCommand;

typedef struct NcmProcessResult {
    NcmBuffer output;
    int32 status;
} NcmProcessResult;

void ncm_process_command_init(NcmProcessCommand *command);
void ncm_process_command_destroy(NcmProcessCommand *command);
bool ncm_process_command_add_arg(NcmProcessCommand *command,
                                 char *arg, int32 arg_len);
bool ncm_process_run_sync(NcmProcessCommand *command, int32 *status,
                          NcmError *error);
bool ncm_process_run_shell(char *command, int32 command_len,
                           int32 *status, NcmError *error);

#endif /* NCM_PROCESS_H */
