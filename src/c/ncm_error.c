#if !defined(NCM_ERROR_C)
#define NCM_ERROR_C

#include "cbase.h"

#include "c/ncm_base.h"
#include "c/ncm_error.h"

void
ncm_error_clear(NcmError *ncm_error) {
    if (ncm_error == NULL) {
        return;
    }

    ncm_error->message[0] = '\0';
    ncm_error->code = 0;
    return;
}

void
ncm_error_set(NcmError *ncm_error, int32 code,
              char *message, int32 message_len) {
    int32 len;

    if (ncm_error == NULL) {
        return;
    }

    if ((message == NULL) || (message_len <= 0)) {
        ncm_error_clear(ncm_error);
        ncm_error->code = code;
        return;
    }

    len = message_len;
    if (len >= SIZEOF(ncm_error->message)) {
        len = SIZEOF(ncm_error->message) - 1;
    }

    memcpy64(ncm_error->message, message, len);
    ncm_error->message[len] = '\0';
    ncm_error->code = code;
    return;
}

bool
ncm_error_is_set(NcmError *ncm_error) {
    if (ncm_error == NULL) {
        return false;
    }

    return ncm_error->code != 0;
}

#endif /* NCM_ERROR_C */
