#include "c/ncm_random.h"

#include <errno.h>

#include "c/ncm_base.h"
#include "cbase/base_macros.h"

static uint64 ncm_random_splitmix64(uint64 *state);
static void ncm_random_swap_bytes(char *left, char *right, int32 size);

static uint64
ncm_random_splitmix64(uint64 *state) {
    uint64 z;

    *state += 0x9e3779b97f4a7c15ull;
    z = *state;
    z = (z ^ (z >> 30))*0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27))*0x94d049bb133111ebull;
    z = z ^ (z >> 31);
    return z;
}

static void
ncm_random_swap_bytes(char *left, char *right, int32 size) {
    char tmp;

    for (int32 i = 0; i < size; i += 1) {
        tmp = left[i];
        left[i] = right[i];
        right[i] = tmp;
    }
    return;
}

void
ncm_random_init(NcmRandom *random, uint64 seed) {
    if (seed == 0) {
        seed = 0x9e3779b97f4a7c15ull;
    }
    random->state = seed;
    return;
}

bool
ncm_random_seed_from_time(NcmRandom *random, NcmError *error) {
    NcmTimePoint point;

    if (random == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing random state"));
        return false;
    }
    if (!ncm_time_monotonic_now(&point, error)) {
        return false;
    }

    ncm_random_init(random, (uint64)point.ns);
    return true;
}

uint64
ncm_random_u64(NcmRandom *random) {
    if (random == NULL) {
        return 0;
    }

    return ncm_random_splitmix64(&random->state);
}

uint32
ncm_random_u32(NcmRandom *random) {
    return (uint32)(ncm_random_u64(random) >> 32);
}

uint32
ncm_random_range_u32(NcmRandom *random, uint32 upper_bound) {
    uint32 threshold;
    uint32 value;

    if (upper_bound <= 1) {
        return 0;
    }

    threshold = (uint32)(-upper_bound) % upper_bound;
    for (;;) {
        value = ncm_random_u32(random);
        if (value >= threshold) {
            return value % upper_bound;
        }
    }
}

int32
ncm_random_range_i32(NcmRandom *random, int32 upper_bound) {
    if (upper_bound <= 1) {
        return 0;
    }

    return (int32)ncm_random_range_u32(random, (uint32)upper_bound);
}

void
ncm_random_shuffle(NcmRandom *random, void *items,
                   int32 item_count, int32 item_size) {
    char *bytes;
    int32 j;

    if (items == NULL) {
        return;
    }
    if ((item_count <= 1) || (item_size <= 0)) {
        return;
    }

    bytes = items;
    for (int32 i = item_count - 1; i > 0; i -= 1) {
        j = ncm_random_range_i32(random, i + 1);
        if (j != i) {
            ncm_random_swap_bytes(bytes + i*item_size,
                                  bytes + j*item_size,
                                  item_size);
        }
    }
    return;
}
