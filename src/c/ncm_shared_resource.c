#include "c/ncm_shared_resource.h"

#include <errno.h>
#include <stdio.h>

#include "c/ncm_base.h"
#include "cbase/base_macros.h"
#include "cbase/cbase.h"

static void ncm_shared_resource_set_pthread_error(NcmError *error,
                                                  int32 code,
                                                  char *operation);

static void
ncm_shared_resource_set_pthread_error(NcmError *error, int32 code,
                                      char *operation) {
    char message[256];
    int32 len;

    len = snprintf(message, (size_t)SIZEOF(message),
                   "pthread %s failed with error %d", operation, code);
    if (len < 0) {
        ncm_error_set(error, code, STRLIT_ARGS("pthread operation failed"));
        return;
    }
    if (len >= (int32)SIZEOF(message)) {
        len = (int32)SIZEOF(message) - 1;
    }

    ncm_error_set(error, code, message, len);
    return;
}

bool
ncm_shared_resource_init_borrowed(NcmSharedResource *shared,
                                  void *resource, NcmError *error) {
    int32 code;

    if (shared == NULL) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing shared resource"));
        return false;
    }
    if (resource == NULL) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing borrowed resource"));
        return false;
    }

    shared->resource = resource;
    shared->resource_size = 0;
    shared->owns_resource = false;
    shared->initialized = false;

    code = pthread_mutex_init(&shared->mutex, NULL);
    if (code != 0) {
        ncm_shared_resource_set_pthread_error(error, code, "mutex init");
        return false;
    }

    shared->initialized = true;
    ncm_error_clear(error);
    return true;
}

void
ncm_shared_resource_destroy(NcmSharedResource *shared) {
    if (shared == NULL) {
        return;
    }

    if (shared->owns_resource && shared->resource) {
        cbase_free(shared->resource, shared->resource_size);
    }
    if (shared->initialized) {
        pthread_mutex_destroy(&shared->mutex);
    }

    shared->resource = NULL;
    shared->resource_size = 0;
    shared->owns_resource = false;
    shared->initialized = false;
    return;
}

void *
ncm_shared_resource_acquire(NcmSharedResource *shared, NcmError *error) {
    int32 code;

    if ((shared == NULL) || !shared->initialized) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("shared resource is not initialized"));
        return NULL;
    }

    code = pthread_mutex_lock(&shared->mutex);
    if (code != 0) {
        ncm_shared_resource_set_pthread_error(error, code, "mutex lock");
        return NULL;
    }

    ncm_error_clear(error);
    return shared->resource;
}

void
ncm_shared_resource_release(NcmSharedResource *shared, NcmError *error) {
    int32 code;

    if ((shared == NULL) || !shared->initialized) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("shared resource is not initialized"));
        return;
    }

    code = pthread_mutex_unlock(&shared->mutex);
    if (code != 0) {
        ncm_shared_resource_set_pthread_error(error, code, "mutex unlock");
        return;
    }

    ncm_error_clear(error);
    return;
}
