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
        int32 left_idx = order[left];
        int32 right_idx = order[right];
        int32 result = 0;

        for (int32 i = 0; i < context->getters_len; i += 1) {
            enum NcmSongGetter getter = context->getters[i];
            StrBuilder left_buffer;
            StrBuilder right_buffer;
            char *left_data;
            char *right_data;

            if (getter == NCM_SONG_GETTER_NONE) {
                break;
            }

            left_buffer
                = ncm_song_getter_buffer(&context->songs->items[left_idx],
                                         getter, 0);
            right_buffer
                = ncm_song_getter_buffer(&context->songs->items[right_idx],
                                         getter, 0);

            left_data = left_buffer.data;
            right_data = right_buffer.data;
            if (left_data == NULL) {
                left_data = "";
            }
            if (right_data == NULL) {
                right_data = "";
            }

            result = ncm_compare_locale_strings(left_data, left_buffer.len,
                                                right_data, right_buffer.len,
                                                context->ignore_leading_the);

            sb_free(&right_buffer);
            sb_free(&left_buffer);
            if (result != 0) {
                break;
            }
        }

        if (result == 0) {
            int32 left_position;
            int32 right_position;

            left_position = ncm_song_position(&context->songs->items[left_idx]);
            right_position =
                ncm_song_position(&context->songs->items[right_idx]);
            if (left_position < right_position) {
                result = -1;
            } else if (left_position > right_position) {
                result = 1;
            } else if (left_idx < right_idx) {
                result = -1;
            } else if (left_idx > right_idx) {
                result = 1;
            }
        }

        if (result <= 0) {
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
ncm_playlist_sort_range(
    NcmSongArray *songs, int32 start_position,
    enum NcmSongGetter *getters, int32 getters_len,
    bool ignore_leading_the, NcmMpdClient *client,
    NcmError *ncm_error
) {
    NcmPlaylistSortPlan plan = {0};
    NcmPlaylistSortContext context = {
        .songs = songs,
        .getters = getters,
        .getters_len = getters_len,
        .ignore_leading_the = ignore_leading_the,
    };
    int32 *order;
    int32 *temporary;
    int32 *current;
    int64 last_position;
    int32 plan_items_len;
    bool started;
    int32 status;

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

    plan_items_len = 0;
    if (songs->len > 1) {
        order = malloc2(songs->len*SIZEOF(*order));
        temporary = malloc2(songs->len*SIZEOF(*temporary));
        current = malloc2(songs->len*SIZEOF(*current));
        plan_items_len = songs->len - 1;
        plan.items = malloc2(plan_items_len*SIZEOF(*plan.items));

        for (int32 i = 0; i < songs->len; i += 1) {
            order[i] = i;
            current[i] = i;
        }

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
                plan.items[plan.len].from = start_position + i;
                plan.items[plan.len].to = start_position + j;
                plan.len += 1;

                swap_value = current[i];
                current[i] = current[j];
                current[j] = swap_value;
                break;
            }
        }

        free2(current, songs->len*SIZEOF(*current));
        free2(temporary, songs->len*SIZEOF(*temporary));
        free2(order, songs->len*SIZEOF(*order));
    }

    status = ncm_error_ok(ncm_error);
    if (client == NULL) {
        free2(plan.items, plan_items_len*SIZEOF(*plan.items));
        return ncm_error_set_code(ncm_error, EINVAL,
                                  STRLIT("missing MPD client"));
    }
    if (plan.len <= 0) {
        free2(plan.items, plan_items_len*SIZEOF(*plan.items));
        return ncm_error_ok(ncm_error);
    }

    started = false;
    status = ncm_mpd_client_start_command_list(client, ncm_error);
    if (status == 0) {
        started = true;
    }
    for (int32 i = 0; (status == 0) && (i < plan.len); i += 1) {
        status = ncm_mpd_client_swap(client,
                                     plan.items[i].from, plan.items[i].to,
                                     ncm_error);
    }
    if (status == 0) {
        status = ncm_mpd_client_commit_command_list(client, ncm_error);
    }
    if ((status < 0) && started && client->command_list_active) {
        client->command_list_active = false;
    }

    free2(plan.items, plan_items_len*SIZEOF(*plan.items));
    return status;
}

#endif /* NCM_PLAYLIST_SORT_C */
