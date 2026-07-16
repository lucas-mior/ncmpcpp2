#if !defined(NCMPCPP_ACTIONS_LEGACY_RUNTIME_H)
#define NCMPCPP_ACTIONS_LEGACY_RUNTIME_H

#include <stdbool.h>

#include "actions.h"
#include "c/ncm_defs.h"
#include "c/ncm_error.h"
#include "screens/screen_type.h"

NCM_EXTERN_C_BEGIN

bool actions_legacy_runtime_playlist_highlight_mpd_position(int32 position);
bool actions_legacy_runtime_can_run_action(enum NcmActionType type);
bool actions_legacy_runtime_execute_action(enum NcmActionType type);
void actions_legacy_runtime_request_exit(void);
bool actions_legacy_runtime_exit_requested(void);

NCM_EXTERN_C_END

#endif /* NCMPCPP_ACTIONS_LEGACY_RUNTIME_H */
