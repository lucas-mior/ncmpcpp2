#if !defined(NCM_LOG_H)
#define NCM_LOG_H

#include <stdarg.h>
#include <stdio.h>

#include "c/ncm_base.h"

typedef struct NcmLog {
    FILE *file;
    bool owns_file;
} NcmLog;

void ncm_log_init(NcmLog *log);
void ncm_log_destroy(NcmLog *log);
bool ncm_log_open(NcmLog *log, char *path, int32 path_len,
                  NcmError *error);
void ncm_log_write(NcmLog *log, char *message, int32 message_len);

#endif /* NCM_LOG_H */
