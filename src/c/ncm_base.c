#if !defined(NCM_BASE_C)
#define NCM_BASE_C

#include "c/ncm_base.h"

#include "cbase/util.c"

void
ncm_buffer_init(NcmBuffer *buffer) {
    sb_init(buffer);
    return;
}

void
ncm_buffer_destroy(NcmBuffer *buffer) {
    sb_free(buffer);
    return;
}

void
ncm_buffer_clear(NcmBuffer *buffer) {
    sb_clear(buffer);
    return;
}

bool
ncm_buffer_copy(NcmBuffer *dest, NcmBuffer *source) {
    return sb_copy(dest, source);
}

void
ncm_buffer_move(NcmBuffer *dest, NcmBuffer *source) {
    sb_move(dest, source);
    return;
}

bool
ncm_buffer_set(NcmBuffer *buffer, char *data, int32 data_len) {
    return sb_set(buffer, data, data_len);
}

void
ncm_buffer_reserve(NcmBuffer *buffer, int32 extra) {
    sb_reserve(buffer, extra);
    return;
}

void
ncm_buffer_append(NcmBuffer *buffer, char *data, int32 data_len) {
    sb_append(buffer, data, data_len);
    return;
}

void
ncm_buffer_append_byte(NcmBuffer *buffer, char byte) {
    sb_append_byte(buffer, byte);
    return;
}

char *
ncm_buffer_steal(NcmBuffer *buffer, int32 *len, int32 *cap) {
    return sb_steal(buffer, len, cap);
}

#endif /* NCM_BASE_C */
