#if !defined(NCMPCPP_ACTIONS_LEGACY_RUNTIME_H)
#define NCMPCPP_ACTIONS_LEGACY_RUNTIME_H

#include <stdbool.h>

#include "actions.h"
#include "bindings.h"
#include "c/ncm_defs.h"
#include "c/ncm_error.h"
#include "curses/nc_window.h"
#include "screens/screen_type.h"

NCM_EXTERN_C_BEGIN

void actions_legacy_runtime_init_readline(void);

void *actions_legacy_runtime_window_create(int64 start_x,
                                           int64 start_y,
                                           int64 width,
                                           int64 height,
                                           NcColor color);
void actions_legacy_runtime_window_display(void *window);
void actions_legacy_runtime_window_destroy(void *window);
void actions_legacy_runtime_window_set_main_hook(void *window);
void actions_legacy_runtime_window_clear_fd_callbacks(void *window);
NcWindow *actions_legacy_runtime_window_native(void *window);

void actions_legacy_runtime_initialize_screens(void);
void actions_legacy_runtime_resize_screen(bool reload_main_window);
bool actions_legacy_runtime_playlist_highlight_mpd_position(int32 position);
void actions_legacy_runtime_browser_fetch_supported_extensions(void);
bool actions_legacy_runtime_update_environment(bool update_timer,
                                                bool refresh_window,
                                                bool mpd_sync);
bool actions_legacy_runtime_search_current_screen(
    enum SearchDirection direction, char *pattern, int32 pattern_len,
    bool wrap, bool skip_current, bool *handled, NcmError *error);
void actions_legacy_runtime_clear_current_search(void);
bool actions_legacy_runtime_execute_binding(NcmBinding *binding);
bool actions_legacy_runtime_execute_action(enum NcmActionType type);
void actions_legacy_runtime_request_exit(void);
bool actions_legacy_runtime_exit_requested(void);

NCM_EXTERN_C_END

#endif /* NCMPCPP_ACTIONS_LEGACY_RUNTIME_H */
