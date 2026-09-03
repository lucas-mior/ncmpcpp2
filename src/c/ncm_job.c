#if !defined(NCM_JOB_C)
#define NCM_JOB_C

#include "cbase.h"

#include "c/ncm_c.h"

static void
ncm_job_destroy(NcmJob *job) {
    if (job == NULL) {
        return;
    }
    if (job->destroy) {
        job->destroy(job->user);
    }

    job->run = NULL;
    job->complete = NULL;
    job->destroy = NULL;
    job->user = NULL;
    ncm_error_clear(&job->ncm_error);
    job->status = -NCM_ERROR_INVALID_STATE;

    return;
}

static void
ncm_job_array_clear(NcmJob *items, int32 len) {
    for (int32 i = 0; i < len; i += 1) {
        ncm_job_destroy(&items[i]);
    }
    return;
}

static void
ncm_job_array_push(NcmJob **items, int32 *len, int32 *cap, NcmJob job) {
    int32 needed;
    int32 old_cap;
    int32 new_cap;

    needed = *len + 1;
    if (needed > *cap) {
        old_cap = *cap;
        new_cap = *cap;
        if (new_cap <= 0) {
            new_cap = 8;
        }
        while (new_cap < needed) {
            new_cap *= 2;
        }

        *items = realloc2(*items, old_cap, new_cap, SIZEOF(**items));
        *cap = new_cap;
    }

    (*items)[*len] = job;
    *len += 1;
    return;
}

static void *
ncm_job_queue_thread_main(void *user) {
    NcmJobQueue *queue = user;
    NcmJob job;
    int32 have_job;

    while (true) {
        pthread_mutex_lock(&queue->mutex);
        while ((queue->pending_len <= 0) && !queue->stopping) {
            pthread_cond_wait(&queue->cond, &queue->mutex);
        }
        if (queue->pending_len <= 0) {
            have_job = 0;
        } else {
            job = queue->pending[0];
            if (queue->pending_len > 1) {
                memmove64(&queue->pending[0], &queue->pending[1],
                          (queue->pending_len - 1)*SIZEOF(*queue->pending));
            }
            queue->pending_len -= 1;
            have_job = 1;
        }
        if ((have_job <= 0) && queue->stopping) {
            pthread_mutex_unlock(&queue->mutex);
            break;
        }
        pthread_mutex_unlock(&queue->mutex);

        if (have_job > 0) {
            ncm_error_clear(&job.ncm_error);
            if (job.run) {
                job.status = job.run(job.user, &job.ncm_error);
            } else {
                job.status = -EINVAL;
                ncm_error_set(&job.ncm_error, EINVAL,
                              STRLIT("job has no run callback"));
            }

            pthread_mutex_lock(&queue->mutex);
            ncm_job_array_push(&queue->completed, &queue->completed_len,
                               &queue->completed_cap, job);
            pthread_mutex_unlock(&queue->mutex);
        }
    }

    return NULL;
}

void
ncm_job_queue_init(NcmJobQueue *queue) {
    queue->pending = NULL;
    queue->completed = NULL;
    queue->pending_len = 0;
    queue->pending_cap = 0;
    queue->completed_len = 0;
    queue->completed_cap = 0;
    queue->started = false;
    queue->stopping = false;

    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);

    return;
}

int32
ncm_job_queue_start(NcmJobQueue *queue, NcmError *ncm_error) {
    int32 code;

    if (queue == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing job queue"));
    }
    if (queue->started) {
        return ncm_error_ok(ncm_error);
    }

    queue->stopping = false;
    code = pthread_create(&queue->thread, NULL,
                          ncm_job_queue_thread_main, queue);
    if (code != 0) {
        char message[256];
        int32 message_len;

        message_len = SNPRINTF(message, "pthread_create: %s",
                               strerror(code));
        ncm_error_set(ncm_error, code, message, message_len);
        return -code;
    }

    queue->started = true;
    return ncm_error_ok(ncm_error);
}

int32
ncm_job_queue_push(NcmJobQueue *queue, NcmJob job, NcmError *ncm_error) {
    if (queue == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing job queue"));
    }
    if (!queue->started) {
        return ncm_error_set_status(ncm_error, -NCM_ERROR_INVALID_STATE,
                                    STRLIT("job queue is not started"));
    }
    if (job.run == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing job callback"));
    }

    ncm_error_clear(&job.ncm_error);
    job.status = -NCM_ERROR_INVALID_STATE;

    pthread_mutex_lock(&queue->mutex);
    if (queue->stopping) {
        pthread_mutex_unlock(&queue->mutex);
        return ncm_error_set_status(ncm_error, -NCM_ERROR_INVALID_STATE,
                                    STRLIT("job queue is stopping"));
    }
    ncm_job_array_push(&queue->pending, &queue->pending_len,
                       &queue->pending_cap, job);
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);

    return ncm_error_ok(ncm_error);
}

int32
ncm_job_queue_dispatch_completed(NcmJobQueue *queue) {
    NcmJob *items;
    int32 len;
    int32 cap;

    if (queue == NULL) {
        return 0;
    }

    pthread_mutex_lock(&queue->mutex);

    items = queue->completed;
    len = queue->completed_len;
    cap = queue->completed_cap;
    queue->completed = NULL;
    queue->completed_len = 0;
    queue->completed_cap = 0;

    pthread_mutex_unlock(&queue->mutex);

    for (int32 i = 0; i < len; i += 1) {
        if (items[i].complete) {
            items[i].complete(items[i].status, &items[i].ncm_error,
                              items[i].user);
        }
        ncm_job_destroy(&items[i]);
    }
    free2(items, cap*SIZEOF(*items));

    return len;
}

void
ncm_job_queue_destroy(NcmJobQueue *queue) {
    if (queue == NULL) {
        return;
    }

    if (queue->started) {
        pthread_mutex_lock(&queue->mutex);
        queue->stopping = true;
        pthread_cond_signal(&queue->cond);
        pthread_mutex_unlock(&queue->mutex);

        pthread_join(queue->thread, NULL);
        queue->started = false;
        ncm_job_queue_dispatch_completed(queue);
    }

    ncm_job_array_clear(queue->pending, queue->pending_len);
    ncm_job_array_clear(queue->completed, queue->completed_len);

    free2(queue->pending, queue->pending_cap*SIZEOF(*queue->pending));
    free2(queue->completed, queue->completed_cap*SIZEOF(*queue->completed));

    pthread_cond_destroy(&queue->cond);
    pthread_mutex_destroy(&queue->mutex);

    queue->pending = NULL;
    queue->completed = NULL;
    queue->pending_len = 0;
    queue->pending_cap = 0;
    queue->completed_len = 0;
    queue->completed_cap = 0;
    queue->stopping = false;

    return;
}

int32
ncm_job_queue_pending_count(NcmJobQueue *queue) {
    int32 result;

    if (queue == NULL) {
        return 0;
    }

    pthread_mutex_lock(&queue->mutex);
    result = queue->pending_len;
    pthread_mutex_unlock(&queue->mutex);

    return result;
}

int32
ncm_job_queue_completed_count(NcmJobQueue *queue) {
    int32 result;

    if (queue == NULL) {
        return 0;
    }

    pthread_mutex_lock(&queue->mutex);
    result = queue->completed_len;
    pthread_mutex_unlock(&queue->mutex);
    return result;
}

#endif /* NCM_JOB_C */
