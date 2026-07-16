#if !defined(NCM_BASE_H)
#define NCM_BASE_H

#include "c/ncm_defs.h"
#include "c/ncm_error.h"

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

#endif /* NCM_BASE_H */
