#include "c/ncm_error.h"

#include "cbase/base_macros.h"
#include "c/ncm_base.h"

void
ncm_error_clear(NcmError *error) {
    if (error == NULL) {
        return;
    }

    error->message[0] = '\0';
    error->code = 0;
    return;
}

void
ncm_error_set(NcmError *error, int32 code,
              char *message, int32 message_len) {
    int32 len;

    if (error == NULL) {
        return;
    }

    if ((message == NULL) || (message_len <= 0)) {
        ncm_error_clear(error);
        error->code = code;
        return;
    }

    len = message_len;
    if (len >= (int32)SIZEOF(error->message)) {
        len = (int32)SIZEOF(error->message) - 1;
    }

    cbase_memcpy(error->message, message, len);
    error->message[len] = '\0';
    error->code = code;
    return;
}

bool
ncm_error_is_set(NcmError *error) {
    if (error == NULL) {
        return false;
    }

    return error->code != 0;
}
