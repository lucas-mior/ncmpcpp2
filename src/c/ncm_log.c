#include "c/ncm_log.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "cbase/base_macros.h"
#include "cbase/cbase.h"

static void ncm_log_set_errno_error(NcmError *error, int32 code,
                                    char *operation, char *path,
                                    int32 path_len);
static bool ncm_log_path_copy(char *path, int32 path_len, char **copy,
                              NcmError *error);

static void
ncm_log_set_errno_error(NcmError *error, int32 code, char *operation,
                        char *path, int32 path_len) {
    char message[256];
    int32 message_len;

    if (path == NULL) {
        message_len = snprintf(message, SIZEOF(message), "%s: %s",
                               operation, strerror(code));
    } else {
        message_len = snprintf(message, SIZEOF(message), "%s '%.*s': %s",
                               operation, path_len, path, strerror(code));
    }
    ncm_error_set(error, code, message, message_len);
    return;
}

static bool
ncm_log_path_copy(char *path, int32 path_len, char **copy,
                  NcmError *error) {
    if (copy == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing path copy output"));
        return false;
    }
    *copy = NULL;

    if (path == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing log path"));
        return false;
    }
    if (path_len < 0) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("negative log path length"));
        return false;
    }

    *copy = cbase_malloc(path_len + 1);
    cbase_memcpy(*copy, path, path_len);
    (*copy)[path_len] = '\0';
    return true;
}

void
ncm_log_init(NcmLog *log) {
    log->file = stderr;
    log->owns_file = false;
    return;
}

void
ncm_log_destroy(NcmLog *log) {
    if (log == NULL) {
        return;
    }
    if (log->owns_file && log->file) {
        fclose(log->file);
    }
    log->file = stderr;
    log->owns_file = false;
    return;
}

bool
ncm_log_open(NcmLog *log, char *path, int32 path_len, NcmError *error) {
    char *copy;
    FILE *file;

    if (log == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing log"));
        return false;
    }
    if (!ncm_log_path_copy(path, path_len, &copy, error)) {
        return false;
    }

    if ((file = fopen(copy, "a")) == NULL) {
        ncm_log_set_errno_error(error, errno, (char *)"open log",
                                path, path_len);
        cbase_free(copy, path_len + 1);
        return false;
    }

    if (log->owns_file && log->file) {
        fclose(log->file);
    }
    log->file = file;
    log->owns_file = true;
    cbase_free(copy, path_len + 1);
    ncm_error_clear(error);
    return true;
}

void
ncm_log_write(NcmLog *log, char *message, int32 message_len) {
    FILE *file;

    if ((message == NULL) || (message_len <= 0)) {
        return;
    }

    if ((log == NULL) || (log->file == NULL)) {
        file = stderr;
    } else {
        file = log->file;
    }

    fwrite(message, 1, (size_t)message_len, file);
    fflush(file);
    return;
}
