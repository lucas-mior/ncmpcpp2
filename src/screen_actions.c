#if !defined(SCREEN_ACTIONS_C)
#define SCREEN_ACTIONS_C

#include "cbase.h"

#include "app_controller.h"
#include "c/ncm_c.h"
#include "screen_actions.h"
#include "screens/nc_screens.h"
#include "settings.h"

static NcScreen *
current_screen(void) {
    return app_controller_current_screen();
}

static void
current_screen_finish_immediate_change(void) {
    NcScreen *screen = current_screen();

    ASSERT(screen != NULL);
    if (nc_screen_type(screen) == NC_SCREEN_TYPE_PLAYLIST) {
        playlist_screen_request_highlighting(app_screen_playlist());
    }
    nc_screen_refresh_window(screen);
    return;
}

bool
current_screen_can_filter(void) {
    return nc_screen_can_filter(current_screen());
}

StringView
current_screen_current_filter(void) {
    return nc_screen_current_filter(current_screen());
}

int32
current_screen_apply_filter(char *pattern, int32 pattern_len,
                            NcmError *ncm_error) {
    NcScreen *screen = current_screen();
    int32 status;

    status = nc_screen_apply_filter(screen, pattern, pattern_len,
                                    Config.regular_expressions, ncm_error);
    if (status < 0) {
        if (!ncm_error_is_set(ncm_error)) {
            return ncm_error_set_status(ncm_error, status,
                                        STRLIT("screen cannot filter"));
        }
        return status;
    }

    current_screen_finish_immediate_change();
    return ncm_error_ok(ncm_error);
}

StringView
current_screen_current_search_constraint(void) {
    return nc_screen_current_search_constraint(current_screen());
}

bool
current_screen_can_search(void) {
    return nc_screen_can_search(current_screen());
}

bool
current_screen_can_find(void) {
    return nc_screen_can_find(current_screen());
}

int32
current_screen_search(enum SearchDirection direction, char *pattern,
                      int32 pattern_len, bool wrap, bool skip_current,
                      NcmError *ncm_error) {
    NcScreen *screen = current_screen();
    int32 status;

    if ((pattern == NULL) || (pattern_len <= 0)) {
        if (nc_screen_can_search(screen)) {
            nc_screen_clear_search_constraint(screen);
            current_screen_finish_immediate_change();
        }
        return ncm_error_ok(ncm_error);
    }

    status = nc_screen_search(screen, direction, pattern, pattern_len,
                              Config.regular_expressions, wrap,
                              skip_current, ncm_error);
    if (status < 0) {
        if (!ncm_error_is_set(ncm_error)) {
            return ncm_error_set_status(ncm_error, status,
                                        STRLIT("screen cannot search"));
        }
        return status;
    }

    current_screen_finish_immediate_change();
    return ncm_error_ok(ncm_error);
}

void
current_screen_clear_search_constraint(void) {
    nc_screen_clear_search_constraint(current_screen());
    return;
}

#endif /* SCREEN_ACTIONS_C */
