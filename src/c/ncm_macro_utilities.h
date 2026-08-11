#if !defined(NCM_MACRO_UTILITIES_H)
#define NCM_MACRO_UTILITIES_H

#include "cbase.h"

#include "c/ncm_error.h"

bool ncm_macro_run_external_command(char *command, int32 command_len,
                                    bool block, NcmError *ncm_error);
bool ncm_macro_run_external_console_command(char *command,
                                            int32 command_len,
                                            NcmError *ncm_error);

#endif /* NCM_MACRO_UTILITIES_H */
