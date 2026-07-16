#if !defined(NCM_MACRO_UTILITIES_H)
#define NCM_MACRO_UTILITIES_H

#include <stdbool.h>

#include "c/ncm_error.h"
#include "cbase/primitives.h"

bool ncm_macro_run_external_command(char *command, int32 command_len,
                                    bool block, NcmError *error);
bool ncm_macro_run_external_console_command(char *command,
                                            int32 command_len,
                                            NcmError *error);

#endif /* NCM_MACRO_UTILITIES_H */
