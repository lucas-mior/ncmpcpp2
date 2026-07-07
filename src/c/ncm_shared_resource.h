#if !defined(NCM_SHARED_RESOURCE_H)
#define NCM_SHARED_RESOURCE_H

#include <pthread.h>

#include "c/ncm_error.h"

NCM_EXTERN_C_BEGIN

typedef struct NcmSharedResource {
    pthread_mutex_t mutex;
    void *resource;
    int32 resource_size;
    bool owns_resource;
    bool initialized;
} NcmSharedResource;

bool ncm_shared_resource_init_borrowed(NcmSharedResource *shared,
                                       void *resource,
                                       NcmError *error);
bool ncm_shared_resource_init_owned(NcmSharedResource *shared,
                                    void *initial_value,
                                    int32 resource_size,
                                    NcmError *error);
void ncm_shared_resource_destroy(NcmSharedResource *shared);
void *ncm_shared_resource_acquire(NcmSharedResource *shared,
                                  NcmError *error);
void ncm_shared_resource_release(NcmSharedResource *shared,
                                 NcmError *error);

NCM_EXTERN_C_END

#endif /* NCM_SHARED_RESOURCE_H */
