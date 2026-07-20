#if !defined(NCM_SAMPLE_BUFFER_H)
#define NCM_SAMPLE_BUFFER_H

#include <stdbool.h>

#include "cbase/primitives.h"

typedef struct NcmSampleBuffer {
    int16 *data;
    int32 len;
    int32 cap;
} NcmSampleBuffer;

void ncm_sample_buffer_init(NcmSampleBuffer *buffer);
void ncm_sample_buffer_destroy(NcmSampleBuffer *buffer);
bool ncm_sample_buffer_put(NcmSampleBuffer *buffer,
                           int16 *samples, int32 samples_len);
int32 ncm_sample_buffer_get(NcmSampleBuffer *buffer,
                            int32 samples_len, int16 *dest, int32 dest_len);
int32 ncm_sample_buffer_get_clamped(NcmSampleBuffer *buffer,
                                    int32 samples_len,
                                    int16 *dest, int32 dest_len);
void ncm_sample_buffer_resize(NcmSampleBuffer *buffer, int32 cap);
void ncm_sample_buffer_clear(NcmSampleBuffer *buffer);
int32 ncm_sample_buffer_size(NcmSampleBuffer *buffer);
int32 ncm_sample_buffer_capacity(NcmSampleBuffer *buffer);

#endif /* NCM_SAMPLE_BUFFER_H */
