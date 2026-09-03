#if !defined(NCM_SEARCH_PROMPT_C)
#define NCM_SEARCH_PROMPT_C

#include "cbase.h"

#include "c/ncm_c.h"

void
ncm_search_prompt_state_init(NcmSearchPromptState *state,
                             enum SearchDirection direction) {
    state->last_text = (StrBuilder){0};

    state->direction = direction;
    state->start_position = 0;
    state->has_start_position = false;
    state->has_last_result = false;
    state->last_found = false;

    return;
}

void
ncm_search_prompt_state_destroy(NcmSearchPromptState *state) {
    sb_free(&state->last_text);
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
ncm_search_prompt_state_has_cached_result(NcmSearchPromptState *state,
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

int32
ncm_search_prompt_state_finish_result(NcmSearchPromptState *state,
                                      char *text, int32 text_len,
                                      bool search_ok, bool found) {
    int32 status;

    if (!search_ok) {
        return 0;
    }
    if (text == NULL) {
        text = "";
        text_len = 0;
    }
    status = sb_set(&state->last_text, text, text_len);
    if (status < 0) {
        return status;
    }

    state->has_last_result = true;
    state->last_found = found;
    return 0;
}

#endif /* NCM_SEARCH_PROMPT_C */
