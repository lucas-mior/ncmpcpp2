#include "c/ncm_sample_buffer.h"

#include "c/ncm_base.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"

void
ncm_sample_buffer_init(NcmSampleBuffer *buffer) {
    buffer->data = NULL;
    buffer->len = 0;
    buffer->cap = 0;
    return;
}

void
ncm_sample_buffer_destroy(NcmSampleBuffer *buffer) {
    if (buffer->data) {
        free2(buffer->data, buffer->cap*SIZEOF(*buffer->data));
    }

    buffer->data = NULL;
    buffer->len = 0;
    buffer->cap = 0;
    return;
}

void
ncm_sample_buffer_copy(NcmSampleBuffer *dest, NcmSampleBuffer *source) {
    if (dest == source) {
        return;
    }

    ncm_sample_buffer_resize(dest, source->cap);
    dest->len = source->len;
    if (source->len > 0) {
        memcpy64(dest->data, source->data,
               source->len*SIZEOF(*source->data));
    }
    return;
}

void
ncm_sample_buffer_move(NcmSampleBuffer *dest, NcmSampleBuffer *source) {
    if (dest == source) {
        return;
    }

    ncm_sample_buffer_destroy(dest);
    *dest = *source;
    ncm_sample_buffer_init(source);
    return;
}

bool
ncm_sample_buffer_put(NcmSampleBuffer *buffer,
                      int16 *samples, int32 samples_len) {
    int32 free_samples;
    int32 to_remove;
    int32 kept;

    if (samples_len <= 0) {
        return true;
    }
    if (samples_len > buffer->cap) {
        return false;
    }

    free_samples = buffer->cap - buffer->len;
    if (samples_len > free_samples) {
        to_remove = samples_len - free_samples;
        kept = buffer->len - to_remove;
        if (kept > 0) {
            memmove64(buffer->data, buffer->data + to_remove,
                    kept*SIZEOF(*buffer->data));
        }
        buffer->len -= to_remove;
    }

    memcpy64(buffer->data + buffer->len, samples,
           samples_len*SIZEOF(*buffer->data));
    buffer->len += samples_len;
    return true;
}

int32
ncm_sample_buffer_get(NcmSampleBuffer *buffer,
                      int32 samples_len, int16 *dest, int32 dest_len) {
    int32 result;
    int32 samples_lost;
    int32 dest_move_len;
    int32 remove_len;

    if (buffer->len <= 0) {
        return 0;
    }
    if (samples_len <= 0) {
        return 0;
    }

    result = samples_len;
    if (result > buffer->len) {
        result = buffer->len;
    }

    if (result >= dest_len) {
        samples_lost = result - dest_len;
        if (dest_len > 0) {
            memcpy64(dest, buffer->data + samples_lost,
                   dest_len*SIZEOF(*dest));
        }
    } else {
        dest_move_len = dest_len - result;
        if (dest_move_len > 0) {
            memmove64(dest, dest + result, dest_move_len*SIZEOF(*dest));
        }
        if (result > 0) {
            memcpy64(dest + dest_move_len, buffer->data,
                   result*SIZEOF(*dest));
        }
    }

    remove_len = buffer->len - result;
    if (remove_len > 0) {
        memmove64(buffer->data, buffer->data + result,
                remove_len*SIZEOF(*buffer->data));
    }
    buffer->len -= result;

    return result;
}

int32
ncm_sample_buffer_get_clamped(NcmSampleBuffer *buffer,
                              int64 samples_len,
                              int16 *dest, int32 dest_len) {
    if (samples_len > buffer->len) {
        samples_len = buffer->len;
    }
    if (samples_len <= 0) {
        return 0;
    }

    return ncm_sample_buffer_get(buffer, (int32)samples_len, dest, dest_len);
}

int32
ncm_sample_buffer_copy_data(NcmSampleBuffer *buffer,
                            int16 *dest, int32 dest_len) {
    int32 copied;

    if (dest_len <= 0) {
        return 0;
    }
    if (buffer->cap <= 0) {
        return 0;
    }

    copied = dest_len;
    if (copied > buffer->cap) {
        copied = buffer->cap;
    }

    memcpy64(dest, buffer->data, copied*SIZEOF(*dest));
    return copied;
}

void
ncm_sample_buffer_resize(NcmSampleBuffer *buffer, int32 cap) {
    if (cap < 0) {
        cap = 0;
    }

    if (cap == 0) {
        free2(buffer->data, buffer->cap*SIZEOF(*buffer->data));
        buffer->data = NULL;
    } else {
        buffer->data = realloc2(buffer->data, buffer->cap, cap,
                     SIZEOF(*buffer->data));
    }
    buffer->cap = cap;
    ncm_sample_buffer_clear(buffer);
    return;
}

void
ncm_sample_buffer_clear(NcmSampleBuffer *buffer) {
    buffer->len = 0;
    return;
}

int32
ncm_sample_buffer_size(NcmSampleBuffer *buffer) {
    return buffer->len;
}

int32
ncm_sample_buffer_capacity(NcmSampleBuffer *buffer) {
    return buffer->cap;
}
