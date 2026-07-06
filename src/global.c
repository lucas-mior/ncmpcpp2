#include "global.h"

#include <stddef.h>

bool global_show_messages;
bool global_seeking_in_progress;
NcmBuffer global_volume_state;
NcmTimePoint global_timer;
NcmRandom global_random;
NcmMpdClient global_mpd;

void
global_state_init(void) {
    ncm_buffer_init(&global_volume_state);
    ncm_mpd_client_init(&global_mpd);
    return;
}

void
global_state_destroy(void) {
    ncm_mpd_client_destroy(&global_mpd);
    ncm_buffer_destroy(&global_volume_state);
    return;
}

bool
global_timer_update(NcmError *error) {
    return ncm_time_monotonic_now(&global_timer, error);
}

int64
global_timer_elapsed_ms(NcmTimePoint start) {
    return ncm_time_elapsed_ms(start, global_timer);
}

int64
global_timer_elapsed_seconds(NcmTimePoint start) {
    return ncm_time_elapsed_ms(start, global_timer) / 1000;
}

void
global_volume_state_set(char *string, int32 string_len) {
    ncm_buffer_clear(&global_volume_state);
    ncm_buffer_append(&global_volume_state, string, string_len);
    return;
}

void
global_volume_state_append(char *string, int32 string_len) {
    ncm_buffer_append(&global_volume_state, string, string_len);
    return;
}

char *
global_volume_state_cstr(void) {
    if (global_volume_state.data == NULL) {
        return (char *)"";
    }

    return global_volume_state.data;
}

int32
global_volume_state_len(void) {
    return global_volume_state.len;
}
