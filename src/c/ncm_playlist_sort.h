#if !defined(NCM_PLAYLIST_SORT_H)
#define NCM_PLAYLIST_SORT_H

#include "c/ncm_error.h"
#include "c/ncm_type_conversions.h"

typedef struct NcmMpdClient NcmMpdClient;
typedef struct NcmSongArray NcmSongArray;

typedef struct NcmPlaylistSortSwap {
    int32 from;
    int32 to;
} NcmPlaylistSortSwap;

typedef struct NcmPlaylistSortPlan {
    NcmPlaylistSortSwap *items;
    int32 len;
    int32 cap;
} NcmPlaylistSortPlan;

void ncm_playlist_sort_plan_init(NcmPlaylistSortPlan *plan);
void ncm_playlist_sort_plan_destroy(NcmPlaylistSortPlan *plan);
bool ncm_playlist_sort_plan_build(
    NcmPlaylistSortPlan *plan, NcmSongArray *songs,
    int32 start_position, enum NcmSongGetter *getters,
    int32 getters_len, bool ignore_leading_the, NcmError *error);
bool ncm_playlist_sort_plan_execute(NcmPlaylistSortPlan *plan,
                                    NcmMpdClient *client,
                                    NcmError *error);
bool ncm_playlist_sort_range(
    NcmSongArray *songs, int32 start_position,
    enum NcmSongGetter *getters, int32 getters_len,
    bool ignore_leading_the, NcmMpdClient *client,
    NcmError *error);

#endif /* NCM_PLAYLIST_SORT_H */
