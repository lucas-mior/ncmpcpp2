#if !defined(NCM_ERROR_C)
#define NCM_ERROR_C

#include "cbase.h"

#include "c/ncm_c.h"

static int32
ncm_error_code_from_status(int32 status) {
    if (status == MINOF(status)) {
        return EOVERFLOW;
    }
    if (status < 0) {
        return -status;
    }

    return status;
}

int32
ncm_status_from_error_code(int32 code) {
    if (code == 0) {
        return 0;
    }
    if (code < 0) {
        return code;
    }

    return -code;
}

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

int32
ncm_error_status(NcmError *ncm_error) {
    if (ncm_error == NULL) {
        return 0;
    }

    return ncm_status_from_error_code(ncm_error->code);
}

int32
ncm_error_set_status(NcmError *ncm_error, int32 status,
                     char *message, int32 message_len) {
    int32 result;

    if (status == 0) {
        ncm_error_clear(ncm_error);
        return 0;
    }

    result = ncm_status_from_error_code(status);
    if (status == MINOF(status)) {
        result = -EOVERFLOW;
    }

    ncm_error_set(ncm_error, ncm_error_code_from_status(status),
                  message, message_len);
    return result;
}

int32
ncm_error_ok(NcmError *ncm_error) {
    ncm_error_clear(ncm_error);
    return 0;
}

#endif /* NCM_ERROR_C */
