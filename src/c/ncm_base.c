#include "c/ncm_base.h"

#include "cbase/base_macros.h"
#include "cbase/cbase.h"

void *
ncm_malloc(int64 size) {
    return cbase_malloc(size);
}

void *
ncm_realloc_array(void *old, int64 old_capacity,
                  int64 new_capacity, int64 obj_size) {
    return cbase_realloc_array(old, old_capacity, new_capacity, obj_size);
}

void
ncm_free(void *pointer, int64 size) {
    cbase_free(pointer, size);
    return;
}

void
ncm_memcpy(void *dest, void *source, int64 n) {
    cbase_memcpy(dest, source, n);
    return;
}

void
ncm_memmove(void *dest, void *source, int64 n) {
    cbase_memmove(dest, source, n);
    return;
}

void
ncm_buffer_init(NcmBuffer *buffer) {
    buffer->data = NULL;
    buffer->len = 0;
    buffer->cap = 0;
    return;
}

void
ncm_buffer_destroy(NcmBuffer *buffer) {
    if (buffer->data) {
        ncm_free(buffer->data, buffer->cap);
    }

    buffer->data = NULL;
    buffer->len = 0;
    buffer->cap = 0;
    return;
}

void
ncm_buffer_clear(NcmBuffer *buffer) {
    buffer->len = 0;
    if (buffer->data) {
        buffer->data[0] = '\0';
    }
    return;
}

bool
ncm_buffer_copy(NcmBuffer *dest, NcmBuffer *source) {
    if (dest == NULL) {
        return false;
    }
    if (dest == source) {
        return true;
    }
    if (source == NULL) {
        ncm_buffer_destroy(dest);
        return true;
    }

    ncm_buffer_clear(dest);
    ncm_buffer_append(dest, source->data, source->len);
    return true;
}

void
ncm_buffer_move(NcmBuffer *dest, NcmBuffer *source) {
    if (dest == NULL) {
        return;
    }
    if (dest == source) {
        return;
    }

    ncm_buffer_destroy(dest);
    if (source == NULL) {
        ncm_buffer_init(dest);
        return;
    }

    *dest = *source;
    ncm_buffer_init(source);
    return;
}

bool
ncm_buffer_set(NcmBuffer *buffer, char *data, int32 data_len) {
    if (buffer == NULL) {
        return false;
    }
    if (data_len < 0) {
        return false;
    }
    if ((data == NULL) && (data_len > 0)) {
        return false;
    }

    ncm_buffer_clear(buffer);
    ncm_buffer_append(buffer, data, data_len);
    return true;
}

void
ncm_buffer_reserve(NcmBuffer *buffer, int32 extra) {
    int32 needed;
    int32 old_cap;
    int32 new_cap;

    if (extra <= 0) {
        return;
    }

    needed = buffer->len + extra + 1;
    if (needed <= buffer->cap) {
        return;
    }

    old_cap = buffer->cap;
    new_cap = buffer->cap;
    if (new_cap <= 0) {
        new_cap = 16;
    }
    while (new_cap < needed) {
        new_cap *= 2;
    }

    buffer->data = ncm_realloc_array(buffer->data, old_cap, new_cap,
                                     SIZEOF(*buffer->data));
    buffer->cap = new_cap;
    return;
}

void
ncm_buffer_append(NcmBuffer *buffer, char *data, int32 data_len) {
    if (data_len <= 0) {
        return;
    }

    ncm_buffer_reserve(buffer, data_len);
    ncm_memcpy(buffer->data + buffer->len, data, data_len);
    buffer->len += data_len;
    buffer->data[buffer->len] = '\0';
    return;
}

void
ncm_buffer_append_byte(NcmBuffer *buffer, char byte) {
    ncm_buffer_reserve(buffer, 1);
    buffer->data[buffer->len] = byte;
    buffer->len += 1;
    buffer->data[buffer->len] = '\0';
    return;
}

char *
ncm_buffer_steal(NcmBuffer *buffer, int32 *len, int32 *cap) {
    char *data;

    data = buffer->data;
    if (len) {
        *len = buffer->len;
    }
    if (cap) {
        *cap = buffer->cap;
    }

    buffer->data = NULL;
    buffer->len = 0;
    buffer->cap = 0;

    return data;
}
