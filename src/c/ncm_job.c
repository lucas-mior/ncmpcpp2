#if !defined(NCM_JOB_C)
#define NCM_JOB_C

#include "c/ncm_job.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "cbase/base_macros.h"
#include "cbase/util.c"

static void
ncm_job_set_errno_error(NcmError *error, int32 code, char *operation) {
    char message[256];
    int32 message_len;

    message_len = SNPRINTF(message, "%s: %s", operation, strerror(code));
    ncm_error_set(error, code, message, message_len);

    return;
}

static bool
ncm_job_array_reserve(NcmJob **items, int32 *cap,
                      int32 len, int32 extra) {
    int32 needed;
    int32 old_cap;
    int32 new_cap;

    if (extra <= 0) {
        return true;
    }
    needed = len + extra;
    if (needed <= *cap) {
        return true;
    }

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

    return true;
}

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
    ncm_error_clear(&job->error);
    job->success = false;

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
    ncm_job_array_reserve(items, cap, *len, 1);
    (*items)[*len] = job;
    *len += 1;
    return;
}

static bool
ncm_job_queue_pop_pending_locked(NcmJobQueue *queue, NcmJob *job) {
    if (queue->pending_len <= 0) {
        return false;
    }

    *job = queue->pending[0];
    if (queue->pending_len > 1) {
        memmove64(&queue->pending[0], &queue->pending[1],
                (queue->pending_len - 1)*SIZEOF(*queue->pending));
    }
    queue->pending_len -= 1;
    return true;
}

static void
ncm_job_queue_push_completed_locked(NcmJobQueue *queue, NcmJob job) {
    ncm_job_array_push(&queue->completed,
                       &queue->completed_len, &queue->completed_cap,
                       job);
    return;
}

static void *
ncm_job_queue_thread_main(void *user) {
    NcmJobQueue *queue = user;
    NcmJob job;
    bool have_job;

    while (true) {
        pthread_mutex_lock(&queue->mutex);
        while ((queue->pending_len <= 0) && !queue->stopping) {
            pthread_cond_wait(&queue->cond, &queue->mutex);
        }
        have_job = ncm_job_queue_pop_pending_locked(queue, &job);
        if (!have_job && queue->stopping) {
            pthread_mutex_unlock(&queue->mutex);
            break;
        }
        pthread_mutex_unlock(&queue->mutex);

        if (have_job) {
            ncm_error_clear(&job.error);
            if (job.run) {
                job.success = job.run(job.user, &job.error);
            } else {
                job.success = false;
                ncm_error_set(&job.error, EINVAL,
                              STRLIT_ARGS("job has no run callback"));
            }

            pthread_mutex_lock(&queue->mutex);
            ncm_job_queue_push_completed_locked(queue, job);
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

bool
ncm_job_queue_start(NcmJobQueue *queue, NcmError *error) {
    int32 code;

    if (queue == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing job queue"));
        return false;
    }
    if (queue->started) {
        return true;
    }

    queue->stopping = false;
    code = pthread_create(&queue->thread, NULL,
                          ncm_job_queue_thread_main, queue);
    if (code != 0) {
        ncm_job_set_errno_error(error, code, "pthread_create");
        return false;
    }

    queue->started = true;
    ncm_error_clear(error);
    return true;
}

bool
ncm_job_queue_push(NcmJobQueue *queue, NcmJob job, NcmError *error) {
    if (queue == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing job queue"));
        return false;
    }
    if (!queue->started) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("job queue is not started"));
        return false;
    }
    if (job.run == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing job callback"));
        return false;
    }

    ncm_error_clear(&job.error);
    job.success = false;

    pthread_mutex_lock(&queue->mutex);
    if (queue->stopping) {
        pthread_mutex_unlock(&queue->mutex);
        ncm_error_set(error, EINVAL, STRLIT_ARGS("job queue is stopping"));
        return false;
    }
    ncm_job_array_push(&queue->pending, &queue->pending_len,
                       &queue->pending_cap, job);
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);

    ncm_error_clear(error);
    return true;
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
            items[i].complete(items[i].success, &items[i].error,
                              items[i].user);
        }
        ncm_job_destroy(&items[i]);
    }
    if (items) {
        free2(items, cap*SIZEOF(*items));
    }

    return len;
}

void
ncm_job_queue_stop(NcmJobQueue *queue) {
    if (queue == NULL) {
        return;
    }
    if (!queue->started) {
        return;
    }

    pthread_mutex_lock(&queue->mutex);
    queue->stopping = true;
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);

    pthread_join(queue->thread, NULL);
    queue->started = false;
    ncm_job_queue_dispatch_completed(queue);
    return;
}

void
ncm_job_queue_destroy(NcmJobQueue *queue) {
    if (queue == NULL) {
        return;
    }

    ncm_job_queue_stop(queue);
    ncm_job_array_clear(queue->pending, queue->pending_len);
    ncm_job_array_clear(queue->completed, queue->completed_len);

    if (queue->pending) {
        free2(queue->pending, queue->pending_cap*SIZEOF(*queue->pending));
    }
    if (queue->completed) {
        free2(queue->completed,
            queue->completed_cap*SIZEOF(*queue->completed));
    }

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
