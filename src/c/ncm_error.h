#if !defined(NCM_ERROR_H)
#define NCM_ERROR_H

#include "c/ncm_defs.h"

typedef struct NcmError {
    char message[256];
    int32 code;
} NcmError;

void ncm_error_clear(NcmError *error);
void ncm_error_set(NcmError *error, int32 code,
                   char *message, int32 message_len);
bool ncm_error_is_set(NcmError *error);

#endif /* NCM_ERROR_H */
