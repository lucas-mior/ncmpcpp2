#if !defined(NCM_JOB_H)
#define NCM_JOB_H

#include <pthread.h>

#include "c/ncm_base.h"

typedef bool (*NcmJobRunCallback)(void *user, NcmError *error);
typedef void (*NcmJobCompleteCallback)(bool success, NcmError *error,
                                       void *user);
typedef void (*NcmJobDestroyCallback)(void *user);

typedef struct NcmJob {
    NcmJobRunCallback run;
    NcmJobCompleteCallback complete;
    NcmJobDestroyCallback destroy;
    void *user;
    NcmError error;
    bool success;
} NcmJob;

typedef struct NcmJobQueue {
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    NcmJob *pending;
    NcmJob *completed;

    int32 pending_len;
    int32 pending_cap;
    int32 completed_len;
    int32 completed_cap;

    bool started;
    bool stopping;
} NcmJobQueue;

void ncm_job_queue_init(NcmJobQueue *queue);
bool ncm_job_queue_start(NcmJobQueue *queue, NcmError *error);
bool ncm_job_queue_push(NcmJobQueue *queue, NcmJob job,
                        NcmError *error);
int32 ncm_job_queue_dispatch_completed(NcmJobQueue *queue);
void ncm_job_queue_stop(NcmJobQueue *queue);
void ncm_job_queue_destroy(NcmJobQueue *queue);
int32 ncm_job_queue_pending_count(NcmJobQueue *queue);
int32 ncm_job_queue_completed_count(NcmJobQueue *queue);

#endif /* NCM_JOB_H */
