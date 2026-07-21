#if !defined(NCM_PLAYLIST_SORT_C)
#define NCM_PLAYLIST_SORT_C

#include "c/ncm_playlist_sort.h"

#include <errno.h>
#include <stdint.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_base.h"
#include "c/ncm_comparators.h"
#include "c/ncm_mpd_client.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"

typedef struct NcmPlaylistSortContext {
    NcmSongArray *songs;
    enum NcmSongGetter *getters;
    int32 getters_len;
    bool ignore_leading_the;
} NcmPlaylistSortContext;

void
ncm_playlist_sort_plan_init(NcmPlaylistSortPlan *plan) {
    if (plan == NULL) {
        return;
    }

    plan->items = NULL;
    plan->len = 0;
    plan->cap = 0;
    return;
}

void
ncm_playlist_sort_plan_destroy(NcmPlaylistSortPlan *plan) {
    if (plan == NULL) {
        return;
    }
    if (plan->items) {
        free2(plan->items, plan->cap*SIZEOF(*plan->items));
    }

    ncm_playlist_sort_plan_init(plan);
    return;
}

static int32
ncm_playlist_sort_compare_key(NcmPlaylistSortContext *context,
                              int32 left_idx, int32 right_idx,
                              enum NcmSongGetter getter) {
    StrBuilder left;
    StrBuilder right;
    char *left_data;
    char *right_data;
    int32 result;

    left = ncm_song_getter_buffer(&context->songs->items[left_idx],
                                  getter, 0);
    right = ncm_song_getter_buffer(&context->songs->items[right_idx],
                                   getter, 0);
    left_data = left.data;
    right_data = right.data;
    if (left_data == NULL) {
        left_data = "";
    }
    if (right_data == NULL) {
        right_data = "";
    }

    result = ncm_compare_locale_strings(
        left_data, left.len, right_data, right.len,
        context->ignore_leading_the);
    sb_free(&right);
    sb_free(&left);
    return result;
}

static int32
ncm_playlist_sort_compare(NcmPlaylistSortContext *context,
                          int32 left_idx, int32 right_idx) {
    int32 left_position;
    int32 right_position;
    int32 result;

    for (int32 i = 0; i < context->getters_len; i += 1) {
        enum NcmSongGetter getter;

        getter = context->getters[i];
        if (getter == NCM_SONG_GETTER_NONE) {
            break;
        }
        result = ncm_playlist_sort_compare_key(
            context, left_idx, right_idx, getter);
        if (result != 0) {
            return result;
        }
    }

    left_position = ncm_song_position(&context->songs->items[left_idx]);
    right_position = ncm_song_position(&context->songs->items[right_idx]);
    if (left_position < right_position) {
        return -1;
    }
    if (left_position > right_position) {
        return 1;
    }
    if (left_idx < right_idx) {
        return -1;
    }
    if (left_idx > right_idx) {
        return 1;
    }
    return 0;
}

static void
ncm_playlist_sort_indices(NcmPlaylistSortContext *context,
                          int32 *order, int32 *temporary,
                          int32 first, int32 last) {
    int32 middle;
    int32 left;
    int32 right;
    int32 out;

    if (last - first <= 1) {
        return;
    }

    middle = first + (last - first)/2;
    ncm_playlist_sort_indices(context, order, temporary, first, middle);
    ncm_playlist_sort_indices(context, order, temporary, middle, last);

    left = first;
    right = middle;
    out = first;
    while ((left < middle) && (right < last)) {
        if (ncm_playlist_sort_compare(context, order[left],
                                      order[right]) <= 0) {
            temporary[out++] = order[left++];
        } else {
            temporary[out++] = order[right++];
        }
    }
    while (left < middle) {
        temporary[out++] = order[left++];
    }
    while (right < last) {
        temporary[out++] = order[right++];
    }
    for (int32 i = first; i < last; i += 1) {
        order[i] = temporary[i];
    }
    return;
}

bool
ncm_playlist_sort_plan_build(
    NcmPlaylistSortPlan *plan, NcmSongArray *songs,
    int32 start_position, enum NcmSongGetter *getters,
    int32 getters_len, bool ignore_leading_the, NcmError *error
) {
    NcmPlaylistSortPlan replacement = {0};
    NcmPlaylistSortContext context;
    int32 *order;
    int32 *temporary;
    int32 *current;
    int64 last_position;

    if (plan == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing sort plan"));
        return false;
    }
    if (songs == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing song array"));
        return false;
    }
    if (songs->len < 0) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("invalid song count"));
        return false;
    }
    if ((songs->len > 0) && (songs->items == NULL)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing songs"));
        return false;
    }
    if (getters_len < 0) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("invalid sort key count"));
        return false;
    }
    if ((getters_len > 0) && (getters == NULL)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing sort keys"));
        return false;
    }
    for (int32 i = 0; i < getters_len; i += 1) {
        if (getters[i] == NCM_SONG_GETTER_NONE) {
            break;
        }
        if ((getters[i] < NCM_SONG_GETTER_NONE)
            || (getters[i] > NCM_SONG_GETTER_PRIORITY)) {
            ncm_error_set(error, EINVAL,
                          STRLIT_ARGS("invalid playlist sort key"));
            return false;
        }
    }
    if (songs->len > 0) {
        last_position = start_position + songs->len - 1;
        if (last_position > UINT32_MAX) {
            ncm_error_set(error, EOVERFLOW,
                          STRLIT_ARGS("playlist sort range overflow"));
            return false;
        }
    }

    ncm_playlist_sort_plan_init(&replacement);
    if (songs->len <= 1) {
        ncm_playlist_sort_plan_destroy(plan);
        *plan = replacement;
        ncm_error_clear(error);
        return true;
    }

    order = malloc2(songs->len*SIZEOF(*order));
    temporary = malloc2(songs->len*SIZEOF(*temporary));
    current = malloc2(songs->len*SIZEOF(*current));
    replacement.items = malloc2(
        (songs->len - 1)*SIZEOF(*replacement.items));
    replacement.cap = songs->len - 1;

    for (int32 i = 0; i < songs->len; i += 1) {
        order[i] = i;
        current[i] = i;
    }

    context.songs = songs;
    context.getters = getters;
    context.getters_len = getters_len;
    context.ignore_leading_the = ignore_leading_the;
    ncm_playlist_sort_indices(&context, order, temporary, 0, songs->len);

    for (int32 i = 0; i < songs->len; i += 1) {
        int32 source_idx;

        source_idx = order[i];
        if (current[i] == source_idx) {
            continue;
        }
        for (int32 j = i + 1; j < songs->len; j += 1) {
            int32 swap_value;

            if (current[j] != source_idx) {
                continue;
            }
            replacement.items[replacement.len].from =
                start_position + i;
            replacement.items[replacement.len].to =
                start_position + j;
            replacement.len += 1;

            swap_value = current[i];
            current[i] = current[j];
            current[j] = swap_value;
            break;
        }
    }

    free2(current, songs->len*SIZEOF(*current));
    free2(temporary, songs->len*SIZEOF(*temporary));
    free2(order, songs->len*SIZEOF(*order));
    ncm_playlist_sort_plan_destroy(plan);
    *plan = replacement;
    ncm_error_clear(error);
    return true;
}

bool
ncm_playlist_sort_plan_execute(NcmPlaylistSortPlan *plan,
                               NcmMpdClient *client,
                               NcmError *error) {
    bool started;
    bool success;

    if (plan == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing sort plan"));
        return false;
    }
    if ((plan->len < 0) || (plan->cap < plan->len)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("invalid sort plan"));
        return false;
    }
    if ((plan->len > 0) && (plan->items == NULL)) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing sort operations"));
        return false;
    }
    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }
    if (plan->len <= 0) {
        ncm_error_clear(error);
        return true;
    }

    started = false;
    if ((success = ncm_mpd_client_start_command_list(client, error))) {
        started = true;
    }
    for (int32 i = 0; success && (i < plan->len); i += 1) {
        success = ncm_mpd_client_swap(client, plan->items[i].from,
                                      plan->items[i].to, error);
    }
    if (success) {
        success = ncm_mpd_client_commit_command_list(client, error);
    }
    if (!success && started && client->command_list_active) {
        client->command_list_active = false;
    }
    return success;
}

bool
ncm_playlist_sort_range(
    NcmSongArray *songs, int32 start_position,
    enum NcmSongGetter *getters, int32 getters_len,
    bool ignore_leading_the, NcmMpdClient *client,
    NcmError *error
) {
    NcmPlaylistSortPlan plan;
    bool success;

    ncm_playlist_sort_plan_init(&plan);
    success = ncm_playlist_sort_plan_build(
        &plan, songs, start_position, getters, getters_len,
        ignore_leading_the, error);
    if (success) {
        success = ncm_playlist_sort_plan_execute(&plan, client, error);
    }
    ncm_playlist_sort_plan_destroy(&plan);
    return success;
}

#endif /* NCM_PLAYLIST_SORT_C */
