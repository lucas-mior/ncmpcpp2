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
void ncm_log_use_file(NcmLog *log, FILE *file);
bool ncm_log_open(NcmLog *log, char *path, int32 path_len,
                  NcmError *error);
void ncm_log_write(NcmLog *log, char *message, int32 message_len);
void ncm_log_vprintf(NcmLog *log, char *format, va_list ap);
void ncm_log_printf(NcmLog *log, char *format, ...)
    __attribute__((format(printf, 2, 3)));
NcmLog *ncm_log_default(void);
void ncm_log_default_use_stderr(void);
bool ncm_log_default_open(char *path, int32 path_len, NcmError *error);
void ncm_log_message(char *message, int32 message_len);
void ncm_log_messagef(char *format, ...)
    __attribute__((format(printf, 1, 2)));

#endif /* NCM_LOG_H */
