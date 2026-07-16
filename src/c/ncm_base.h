#if !defined(NCM_BASE_H)
#define NCM_BASE_H

#include "c/ncm_defs.h"
#include "c/ncm_error.h"

#if defined(__cplusplus)
extern "C" {
#endif

void *cbase_malloc(int64 size);
void *cbase_realloc_array(void *old, int64 old_capacity,
                        int64 new_capacity, int64 obj_size);
void cbase_free(void *pointer, int64 size);
void cbase_memcpy(void *dest, void *source, int64 n);
void cbase_memmove(void *dest, void *source, int64 n);

void ncm_buffer_init(NcmBuffer *buffer);
void ncm_buffer_destroy(NcmBuffer *buffer);
void ncm_buffer_clear(NcmBuffer *buffer);
bool ncm_buffer_copy(NcmBuffer *dest, NcmBuffer *source);
void ncm_buffer_move(NcmBuffer *dest, NcmBuffer *source);
bool ncm_buffer_set(NcmBuffer *buffer, char *data, int32 data_len);
void ncm_buffer_reserve(NcmBuffer *buffer, int32 extra);
void ncm_buffer_append(NcmBuffer *buffer, char *data, int32 data_len);
void ncm_buffer_append_byte(NcmBuffer *buffer, char byte);
char *ncm_buffer_steal(NcmBuffer *buffer, int32 *len, int32 *cap);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_BASE_H */
