#if !defined(NCM_SEARCH_PROMPT_C)
#define NCM_SEARCH_PROMPT_C

#include "c/ncm_search_prompt.h"

#include "cbase/util.c"

void
ncm_search_prompt_state_init(NcmSearchPromptState *state,
                             enum SearchDirection direction) {
    state->direction = direction;
    ncm_buffer_init(&state->last_text);
    state->start_position = 0;
    state->has_start_position = false;
    state->has_last_result = false;
    state->last_found = false;
    return;
}

void
ncm_search_prompt_state_destroy(NcmSearchPromptState *state) {
    ncm_buffer_destroy(&state->last_text);
    return;
}

void
ncm_search_prompt_state_set_start_position(NcmSearchPromptState *state,
                                           int32 position) {
    state->start_position = position;
    state->has_start_position = true;
    return;
}

bool
ncm_search_prompt_state_cached_result(NcmSearchPromptState *state,
                                      char *text, int32 text_len,
                                      bool *found) {
    if (!state->has_last_result) {
        return false;
    }
    if (text == NULL) {
        text = "";
        text_len = 0;
    }
    if (state->last_text.len != text_len) {
        return false;
    }
    if ((text_len > 0)
        && (memcmp64(state->last_text.data, text, text_len) != 0)) {
        return false;
    }

    if (found) {
        *found = state->last_found;
    }
    return true;
}

bool
ncm_search_prompt_state_finish_result(NcmSearchPromptState *state,
                                      char *text, int32 text_len,
                                      bool search_ok, bool found) {
    if (!search_ok) {
        return true;
    }
    if (text == NULL) {
        text = "";
        text_len = 0;
    }
    if (!ncm_buffer_set(&state->last_text, text, text_len)) {
        return false;
    }

    state->has_last_result = true;
    state->last_found = found;
    return true;
}

#endif /* NCM_SEARCH_PROMPT_C */
