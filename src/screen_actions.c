#include "screen_actions.h"

#include "app_binding_migration.h"
#include "app_controller.h"
#include "settings.h"
#include "screens/native_c_screens.h"

#include "c/ncm_base.h"

#if defined(__GNUC__)
extern bool actions_legacy_runtime_search_current_screen(
    enum SearchDirection direction, char *pattern, int32 pattern_len,
    bool wrap, bool skip_current, bool *handled,
    NcmError *error) __attribute__((weak));
extern void actions_legacy_runtime_clear_current_search(void)
    __attribute__((weak));
#endif

static NcScreen *current_screen(void);
static bool current_screen_is(int32 type);
static NcmBuffer *current_screen_filter_buffer(void);
static NcmBuffer *current_screen_search_buffer(void);
static bool current_screen_search_legacy(
    enum SearchDirection direction, char *pattern, int32 pattern_len,
    bool wrap, bool skip_current, bool *handled, NcmError *error);
static void current_screen_clear_legacy_search(void);
static bool current_screen_set_search_constraint(char *pattern,
                                                 int32 pattern_len);
static bool current_screen_search_direction_forward(
    enum SearchDirection direction);
static bool current_screen_search_error(NcmError *error);
static void current_screen_clear_current_search_constraint(void);
static void current_screen_finish_immediate_change(void);

static NcScreen *
current_screen(void) {
    return app_controller_current_screen();
}

static bool
current_screen_is(int32 type) {
    NcScreen *screen;

    screen = current_screen();
    if (screen == NULL) {
        return false;
    }
    return nc_screen_type(screen) == type;
}

static NcmBuffer *
current_screen_filter_buffer(void) {
    if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST)) {
        return &native_c_screen_playlist()->filter_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_BROWSER)) {
        return &native_c_screen_browser()->filter_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        NativePlaylistEditorScreen *screen;

        screen = native_c_screen_playlist_editor();
        if (screen->active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT) {
            return &screen->content_filter_constraint;
        }
        return &screen->playlist_filter_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_SEARCH_ENGINE)) {
        return &native_c_screen_search_engine()->filter_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_MEDIA_LIBRARY)) {
        return native_media_library_screen_active_filter_constraint(
            native_c_screen_media_library());
    }
    return NULL;
}

static NcmBuffer *
current_screen_search_buffer(void) {
    if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST)) {
        return &native_c_screen_playlist()->search_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_BROWSER)) {
        return &native_c_screen_browser()->search_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        NativePlaylistEditorScreen *screen;

        screen = native_c_screen_playlist_editor();
        if (screen->active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT) {
            return &screen->content_search_constraint;
        }
        return &screen->playlist_search_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_SEARCH_ENGINE)) {
        return &native_c_screen_search_engine()->search_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_MEDIA_LIBRARY)) {
        return native_media_library_screen_active_search_constraint(
            native_c_screen_media_library());
    }
    if (current_screen_is(NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER)) {
        return &native_c_screen_selected_items_adder()->search_constraint;
    }
#if defined(HAVE_TAGLIB_H)
    if (current_screen_is(NC_SCREEN_TYPE_TAG_EDITOR)) {
        NativeTagEditorScreen *screen;

        screen = native_c_screen_tag_editor();
        if (screen->active_column == NATIVE_TAG_EDITOR_COLUMN_DIRECTORIES) {
            return &screen->directory_search_constraint;
        }
        if (screen->active_column == NATIVE_TAG_EDITOR_COLUMN_TAGS) {
            return &screen->tag_search_constraint;
        }
    }
#endif
    return NULL;
}

static bool
current_screen_search_legacy(enum SearchDirection direction,
                             char *pattern, int32 pattern_len,
                             bool wrap, bool skip_current,
                             bool *handled, NcmError *error) {
    if (handled == NULL) {
        return false;
    }
    *handled = false;
    if (current_screen_is(NC_SCREEN_TYPE_SEARCH_ENGINE)
        || app_binding_migration_screen_is_c_only(
            native_c_screens_current_type())) {
        return false;
    }
#if defined(__GNUC__)
    if (actions_legacy_runtime_search_current_screen == NULL) {
        return false;
    }
    return actions_legacy_runtime_search_current_screen(
        direction, pattern, pattern_len, wrap, skip_current,
        handled, error);
#else
    (void)direction;
    (void)pattern;
    (void)pattern_len;
    (void)wrap;
    (void)skip_current;
    (void)error;
    return false;
#endif
}

static void
current_screen_clear_legacy_search(void) {
    if (current_screen_is(NC_SCREEN_TYPE_SEARCH_ENGINE)
        || app_binding_migration_screen_is_c_only(
            native_c_screens_current_type())) {
        return;
    }
#if defined(__GNUC__)
    if (actions_legacy_runtime_clear_current_search != NULL) {
        actions_legacy_runtime_clear_current_search();
    }
#endif
    return;
}

static bool
current_screen_set_search_constraint(char *pattern, int32 pattern_len) {
    NcmBuffer *buffer;

    buffer = current_screen_search_buffer();
    if (buffer == NULL) {
        return false;
    }
    return ncm_buffer_set(buffer, pattern, pattern_len);
}

static bool
current_screen_search_direction_forward(enum SearchDirection direction) {
    if (direction == NCM_SEARCH_DIRECTION_FORWARD) {
        return true;
    }
    return false;
}

static bool
current_screen_search_error(NcmError *error) {
    return ncm_error_is_set(error);
}

static void
current_screen_clear_current_search_constraint(void) {
    NcmBuffer *buffer;

    current_screen_clear_legacy_search();
    if (current_screen_is(NC_SCREEN_TYPE_MEDIA_LIBRARY)) {
        native_media_library_screen_clear_search(
            native_c_screen_media_library());
        return;
    }
    buffer = current_screen_search_buffer();
    if (buffer != NULL) {
        ncm_buffer_clear(buffer);
    }
    return;
}

static void
current_screen_finish_immediate_change(void) {
    NcScreen *screen;

    screen = current_screen();
    if (screen == NULL) {
        return;
    }
    if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST)) {
        native_playlist_screen_request_highlighting(
            native_c_screen_playlist());
    }
    nc_screen_refresh_window(screen);
    return;
}

bool
current_screen_allows_filter(void) {
    return current_screen_filter_buffer() != NULL;
}

NcmStringView
current_screen_current_filter(void) {
    NcmBuffer *buffer;

    buffer = current_screen_filter_buffer();
    if (buffer == NULL) {
        return ncm_string_view_make(NULL, 0);
    }
    return ncm_string_view_make(buffer->data, buffer->len);
}

bool
current_screen_apply_filter(char *pattern, int32 pattern_len,
                            NcmError *error) {
    bool result;

    result = false;
    if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST)) {
        result = native_playlist_screen_apply_filter(
            native_c_screen_playlist(), pattern, pattern_len, error);
    } else if (current_screen_is(NC_SCREEN_TYPE_BROWSER)) {
        result = native_browser_screen_apply_filter(
            native_c_screen_browser(), pattern, pattern_len, error);
    } else if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        result = native_playlist_editor_screen_apply_active_filter(
            native_c_screen_playlist_editor(), pattern, pattern_len,
            Config.regex_type, error);
    } else if (current_screen_is(NC_SCREEN_TYPE_SEARCH_ENGINE)) {
        result = native_search_engine_screen_apply_filter(
            native_c_screen_search_engine(), pattern, pattern_len, error);
    } else if (current_screen_is(NC_SCREEN_TYPE_MEDIA_LIBRARY)) {
        result = native_media_library_screen_apply_filter(
            native_c_screen_media_library(), pattern, pattern_len, error);
    }

    if (result) {
        current_screen_finish_immediate_change();
    }
    return result;
}


NcmStringView
current_screen_current_search_constraint(void) {
    NcmBuffer *buffer;

    buffer = current_screen_search_buffer();
    if (buffer == NULL) {
        return ncm_string_view_make(NULL, 0);
    }
    return ncm_string_view_make(buffer->data, buffer->len);
}

bool
current_screen_allows_search(void) {
    if (current_screen_is(NC_SCREEN_TYPE_SEARCH_ENGINE)) {
        return native_search_engine_screen_allows_search(
            native_c_screen_search_engine());
    }
    if (current_screen_is(NC_SCREEN_TYPE_TAG_EDITOR)) {
#if defined(HAVE_TAGLIB_H)
        NativeTagEditorScreen *screen;

        screen = native_c_screen_tag_editor();
        return screen->active_column != NATIVE_TAG_EDITOR_COLUMN_TAG_TYPES;
#else
        return false;
#endif
    }
    return current_screen_search_buffer() != NULL;
}

bool
current_screen_search(enum SearchDirection direction,
                      char *pattern, int32 pattern_len,
                      bool wrap, bool skip_current,
                      NcmError *error) {
    bool attempted;
    bool forward;
    bool found;

    if (pattern == NULL || pattern_len <= 0) {
        if (current_screen_allows_search()) {
            current_screen_clear_current_search_constraint();
            current_screen_finish_immediate_change();
        }
        return false;
    }

    attempted = false;
    forward = current_screen_search_direction_forward(direction);
    found = false;
    /*
     * Legacy-backed screens render their C++ menu. Search that menu before
     * falling back to a native-only screen.
     */
    found = current_screen_search_legacy(
        direction, pattern, pattern_len, wrap, skip_current,
        &attempted, error);
    if (!attempted) {
        if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST)) {
            attempted = true;
            found = native_playlist_screen_search(
                native_c_screen_playlist(), pattern, pattern_len, forward,
                wrap, skip_current, error);
        } else if (current_screen_is(NC_SCREEN_TYPE_BROWSER)) {
            attempted = true;
            found = native_browser_screen_search(
                native_c_screen_browser(), pattern, pattern_len, forward,
                wrap, skip_current, error);
        } else if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST_EDITOR)) {
            attempted = true;
            found = native_playlist_editor_screen_search_active(
                native_c_screen_playlist_editor(), pattern, pattern_len,
                Config.regex_type, forward, wrap, skip_current, error);
        } else if (current_screen_is(NC_SCREEN_TYPE_SEARCH_ENGINE)) {
            attempted = true;
            found = native_search_engine_screen_search(
                native_c_screen_search_engine(), pattern, pattern_len,
                forward, wrap, skip_current, error);
        } else if (current_screen_is(NC_SCREEN_TYPE_MEDIA_LIBRARY)) {
            attempted = true;
            found = native_media_library_screen_search(
                native_c_screen_media_library(), pattern, pattern_len,
                forward, wrap, skip_current, error);
        } else if (current_screen_is(
                       NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER)) {
            attempted = true;
            found = native_selected_items_adder_screen_search(
                native_c_screen_selected_items_adder(), pattern,
                pattern_len, Config.regex_type, forward, wrap,
                skip_current, error);
#if defined(HAVE_TAGLIB_H)
        } else if (current_screen_is(NC_SCREEN_TYPE_TAG_EDITOR)) {
            attempted = true;
            found = native_tag_editor_screen_search(
                native_c_screen_tag_editor(), pattern, pattern_len,
                forward, wrap, skip_current, error);
#endif
        }
    }

    if (attempted && !current_screen_search_error(error)) {
        (void)current_screen_set_search_constraint(pattern, pattern_len);
        current_screen_finish_immediate_change();
    }
    return found;
}

void
current_screen_clear_search_constraint(void) {
    current_screen_clear_current_search_constraint();
    return;
}
