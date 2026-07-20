#if !defined(NCM_SEARCH_PROMPT_H)
#define NCM_SEARCH_PROMPT_H

#include <stdbool.h>

#include "c/ncm_base.h"
#include "c/ncm_enums.h"

typedef struct NcmSearchPromptState {
    enum SearchDirection direction;
    NcmBuffer last_text;
    int32 start_position;

    bool has_start_position;
    bool has_last_result;
    bool last_found;
} NcmSearchPromptState;

void ncm_search_prompt_state_init(NcmSearchPromptState *state,
                                  enum SearchDirection direction);
void ncm_search_prompt_state_destroy(NcmSearchPromptState *state);
void ncm_search_prompt_state_set_start_position(
    NcmSearchPromptState *state, int32 position);
bool ncm_search_prompt_state_cached_result(NcmSearchPromptState *state,
                                           char *text, int32 text_len,
                                           bool *found);
bool ncm_search_prompt_state_finish_result(NcmSearchPromptState *state,
                                           char *text, int32 text_len,
                                           bool search_ok, bool found);

#endif /* NCM_SEARCH_PROMPT_H */
