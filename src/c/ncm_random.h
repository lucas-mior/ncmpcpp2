#if !defined(NCM_RANDOM_H)
#define NCM_RANDOM_H

#include "c/ncm_time.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcmRandom {
    uint64 state;
} NcmRandom;

void ncm_random_init(NcmRandom *random, uint64 seed);
bool ncm_random_seed_from_time(NcmRandom *random, NcmError *error);
uint64 ncm_random_u64(NcmRandom *random);
uint32 ncm_random_u32(NcmRandom *random);
uint32 ncm_random_range_u32(NcmRandom *random, uint32 upper_bound);
int32 ncm_random_range_i32(NcmRandom *random, int32 upper_bound);
void ncm_random_shuffle(NcmRandom *random, void *items,
                        int32 item_count, int32 item_size);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_RANDOM_H */
