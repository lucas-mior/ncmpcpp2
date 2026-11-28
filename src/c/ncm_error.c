#if !defined(NCM_ERROR_C)
#define NCM_ERROR_C

#include "cbase.h"
#include "ncmpcpp2.h"

#include "c/ncm_c.h"

int32
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
    if (code == MINOF(code)) {
        return -EOVERFLOW;
    }
    if (code < 0) {
        return code;
    }

    return -code;
}

void
ncm_error_clear(NcmError *err) {
    if (err == NULL) {
        return;
    }

    err->message[0] = '\0';
    err->message_len = 0;
    err->code = 0;
    return;
}

void
ncm_error_set(NcmError *err, int32 code,
              char *message, int32 message_len) {
    int32 len;

    if (err == NULL) {
        return;
    }

    code = ncm_error_code_from_status(code);
    if (code == 0) {
        ncm_error_clear(err);
        return;
    }

    if ((message == NULL) || (message_len <= 0)) {
        ncm_error_clear(err);
        err->code = code;
        return;
    }

    len = message_len;
    if (len >= SIZEOF(err->message)) {
        len = SIZEOF(err->message) - 1;
    }

    memcpy64(err->message, message, len);
    err->message[len] = '\0';
    err->message_len = len;
    err->code = code;
    return;
}

bool
ncm_error_is_set(NcmError *err) {
    if (err == NULL) {
        return false;
    }

    return err->code != 0;
}

int32
ncm_error_status(NcmError *err) {
    if (err == NULL) {
        return 0;
    }

    return ncm_status_from_error_code(err->code);
}

int32
ncm_error_set_code(NcmError *err, int32 code,
                   char *message, int32 message_len) {
    int32 status;

    if (code == 0) {
        ncm_error_clear(err);
        return 0;
    }

    status = ncm_status_from_error_code(code);
    ncm_error_set(err, code, message, message_len);
    return status;
}

int32
ncm_error_set_status(NcmError *err, int32 status,
                     char *message, int32 message_len) {
    if (status == 0) {
        ncm_error_clear(err);
        return 0;
    }

    return ncm_error_set_code(err, ncm_error_code_from_status(status),
                              message, message_len);
}

int32
ncm_error_ok(NcmError *err) {
    ncm_error_clear(err);
    return 0;
}

#endif /* NCM_ERROR_C */
