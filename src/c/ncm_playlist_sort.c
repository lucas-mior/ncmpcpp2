#if !defined(NCM_PLAYLIST_SORT_C)
#define NCM_PLAYLIST_SORT_C

#include "cbase.h"

#include "c/ncm_c.h"

typedef struct NcmPlaylistSortContext {
    NcmSongArray *songs;
    enum NcmSongGetter *getters;
    int32 getters_len;
    bool ignore_leading_the;
} NcmPlaylistSortContext;

void
ncm_playlist_sort_plan_destroy(NcmPlaylistSortPlan *plan) {
    free2(plan->items, plan->cap*SIZEOF(*plan->items));
    *plan = (NcmPlaylistSortPlan){0};
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

    result = ncm_compare_locale_strings(left_data, left.len,
                                        right_data, right.len,
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
        enum NcmSongGetter getter = context->getters[i];

        if (getter == NCM_SONG_GETTER_NONE) {
            break;
        }
        result = ncm_playlist_sort_compare_key(context,
                                               left_idx, right_idx, getter);
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

int32
ncm_playlist_sort_plan_build(
    NcmPlaylistSortPlan *plan, NcmSongArray *songs,
    int32 start_position, enum NcmSongGetter *getters,
    int32 getters_len, bool ignore_leading_the, NcmError *ncm_error
) {
    NcmPlaylistSortPlan replacement = {0};
    NcmPlaylistSortContext context;
    int32 *order;
    int32 *temporary;
    int32 *current;
    int64 last_position;

    if (plan == NULL) {
        return ncm_error_set_code(ncm_error, EINVAL,
                                  STRLIT("missing sort plan"));
    }
    if (songs == NULL) {
        return ncm_error_set_code(ncm_error, EINVAL,
                                  STRLIT("missing song array"));
    }
    if (songs->len < 0) {
        return ncm_error_set_code(ncm_error, EINVAL,
                                  STRLIT("invalid song count"));
    }
    if ((songs->len > 0) && (songs->items == NULL)) {
        return ncm_error_set_code(ncm_error, EINVAL,
                                  STRLIT("missing songs"));
    }
    if (start_position < 0) {
        return ncm_error_set_code(ncm_error, EINVAL,
                                  STRLIT("invalid sort start position"));
    }
    if (getters_len < 0) {
        return ncm_error_set_code(ncm_error, EINVAL,
                                  STRLIT("invalid sort key count"));
    }
    if ((getters_len > 0) && (getters == NULL)) {
        return ncm_error_set_code(ncm_error, EINVAL,
                                  STRLIT("missing sort keys"));
    }
    for (int32 i = 0; i < getters_len; i += 1) {
        if (getters[i] == NCM_SONG_GETTER_NONE) {
            break;
        }
        if ((getters[i] < NCM_SONG_GETTER_NONE)
            || (getters[i] > NCM_SONG_GETTER_PRIORITY)) {
            return ncm_error_set_code(ncm_error, EINVAL,
                                      STRLIT("invalid playlist sort key"));
        }
    }
    if (songs->len > 0) {
        last_position = (int64)start_position + songs->len - 1;
        if (last_position > UINT32_MAX) {
            return ncm_error_set_code(ncm_error, EOVERFLOW,
                                      STRLIT("playlist sort range overflow"));
        }
    }

    replacement = (NcmPlaylistSortPlan){0};
    if (songs->len <= 1) {
        ncm_playlist_sort_plan_destroy(plan);
        *plan = replacement;
        return ncm_error_ok(ncm_error);
    }

    order = malloc2(songs->len*SIZEOF(*order));
    temporary = malloc2(songs->len*SIZEOF(*temporary));
    current = malloc2(songs->len*SIZEOF(*current));
    replacement.items = malloc2((songs->len - 1)*SIZEOF(*replacement.items));
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
        int32 source_idx = order[i];

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
    return ncm_error_ok(ncm_error);
}

int32
ncm_playlist_sort_plan_execute(NcmPlaylistSortPlan *plan,
                               NcmMpdClient *client,
                               NcmError *ncm_error) {
    bool started;
    int32 status;

    if (plan == NULL) {
        return ncm_error_set_code(ncm_error, EINVAL,
                                  STRLIT("missing sort plan"));
    }
    if ((plan->len < 0) || (plan->cap < plan->len)) {
        return ncm_error_set_code(ncm_error, EINVAL,
                                  STRLIT("invalid sort plan"));
    }
    if ((plan->len > 0) && (plan->items == NULL)) {
        return ncm_error_set_code(ncm_error, EINVAL,
                                  STRLIT("missing sort operations"));
    }
    if (client == NULL) {
        return ncm_error_set_code(ncm_error, EINVAL,
                                  STRLIT("missing MPD client"));
    }
    if (plan->len <= 0) {
        return ncm_error_ok(ncm_error);
    }

    started = false;
    status = ncm_mpd_client_start_command_list(client, ncm_error);
    if (status == 0) {
        started = true;
    }
    for (int32 i = 0; (status == 0) && (i < plan->len); i += 1) {
        status = ncm_mpd_client_swap(client,
                                     plan->items[i].from, plan->items[i].to,
                                     ncm_error);
    }
    if (status == 0) {
        status = ncm_mpd_client_commit_command_list(client, ncm_error);
    }
    if ((status < 0) && started && client->command_list_active) {
        client->command_list_active = false;
    }
    return status;
}

int32
ncm_playlist_sort_range(
    NcmSongArray *songs, int32 start_position,
    enum NcmSongGetter *getters, int32 getters_len,
    bool ignore_leading_the, NcmMpdClient *client,
    NcmError *ncm_error
) {
    NcmPlaylistSortPlan plan = {0};
    int32 status;

    status = ncm_playlist_sort_plan_build(
        &plan, songs, start_position, getters, getters_len,
        ignore_leading_the, ncm_error);
    if (status == 0) {
        status = ncm_playlist_sort_plan_execute(&plan, client, ncm_error);
    }
    ncm_playlist_sort_plan_destroy(&plan);
    return status;
}

#endif /* NCM_PLAYLIST_SORT_C */
