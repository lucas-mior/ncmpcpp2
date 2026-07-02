#if !defined(NCM_BASE_H)
#define NCM_BASE_H

#include <stdbool.h>

#include "cbase/primitives.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcmStringView {
    char *data;
    int32 len;
} NcmStringView;

typedef struct NcmBuffer {
    char *data;
    int32 len;
    int32 cap;
} NcmBuffer;

typedef struct NcmError {
    char message[256];
    int32 code;
} NcmError;

void *ncm_malloc(int64 size);
void *ncm_realloc_array(void *old, int64 old_capacity,
                        int64 new_capacity, int64 obj_size);
void ncm_free(void *pointer, int64 size);
void ncm_memcpy(void *dest, void *source, int64 n);
void ncm_memmove(void *dest, void *source, int64 n);

void ncm_error_clear(NcmError *error);
void ncm_error_set(NcmError *error, int32 code,
                   char *message, int32 message_len);

void ncm_buffer_init(NcmBuffer *buffer);
void ncm_buffer_destroy(NcmBuffer *buffer);
void ncm_buffer_clear(NcmBuffer *buffer);
void ncm_buffer_reserve(NcmBuffer *buffer, int32 extra);
void ncm_buffer_append(NcmBuffer *buffer, char *data, int32 data_len);
void ncm_buffer_append_byte(NcmBuffer *buffer, char byte);
char *ncm_buffer_steal(NcmBuffer *buffer, int32 *len, int32 *cap);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_BASE_H */
