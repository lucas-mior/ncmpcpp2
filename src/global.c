#if !defined(NCMPCPP_GLOBAL_C)
#define NCMPCPP_GLOBAL_C

#include "cbase.h"

#include "global.h"

bool global_show_messages;
bool global_seeking_in_progress;
StrBuilder global_volume_state;
int64 global_timer;
NcmMpdClient global_mpd;

void
global_state_init(void) {
    global_volume_state = (StrBuilder){0};
    ncm_mpd_client_init(&global_mpd);
    return;
}

void
global_state_destroy(void) {
    ncm_mpd_client_destroy(&global_mpd);
    sb_free(&global_volume_state);
    return;
}

void
global_timer_update(void) {
    global_timer = time_monotonic_now();
    return;
}

int64
global_timer_elapsed_ms(int64 start) {
    return time_elapsed_ms(start, global_timer);
}

int64
global_timer_elapsed_seconds(int64 start) {
    return time_elapsed_ms(start, global_timer) / 1000;
}

void
global_volume_state_set(char *string, int32 string_len) {
    sb_clear(&global_volume_state);
    SB_APPEND(&global_volume_state, string, string_len);
    return;
}

void
global_volume_state_append(char *string, int32 string_len) {
    SB_APPEND(&global_volume_state, string, string_len);
    return;
}

char *
global_volume_state_cstr(void) {
    if (global_volume_state.data == NULL) {
        return "";
    }

    return global_volume_state.data;
}

int32
global_volume_state_len(void) {
    return global_volume_state.len;
}

#endif /* NCMPCPP_GLOBAL_C */
