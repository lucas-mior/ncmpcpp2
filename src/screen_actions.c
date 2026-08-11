#if !defined(NCMPCPP_SCREEN_ACTIONS_C)
#define NCMPCPP_SCREEN_ACTIONS_C

#include "cbase.h"

#include "app_controller.h"
#include "c/ncm_base.h"
#include "screen_actions.h"
#include "screens/app_screens.h"
#include "screens/nc_search_engine.h"
#include "settings.h"

static NcScreen *
current_screen(void) {
    return app_controller_current_screen();
}

static bool
current_screen_is(int32 type) {
    NcScreen *screen;

    if ((screen = current_screen()) == NULL) {
        return false;
    }
    return nc_screen_type(screen) == type;
}

static StrBuilder *
current_screen_filter_buffer(void) {
    if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST)) {
        return &app_screen_playlist()->filter_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_BROWSER)) {
        return &app_screen_browser()->filter_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        PlaylistEditorScreen *screen;

        screen = app_screen_playlist_editor();
        if (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT) {
            return &screen->content_filter_constraint;
        }
        return &screen->playlist_filter_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_SEARCH_ENGINE)) {
        return &app_screen_search_engine()->filter_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_MEDIA_LIBRARY)) {
        return media_library_screen_active_filter_constraint(
            app_screen_media_library());
    }
#if defined(HAVE_TAGLIB_H)
    if (current_screen_is(NC_SCREEN_TYPE_TAG_EDITOR)) {
        TagEditorScreen *screen;

        screen = app_screen_tag_editor();
        if (screen->active_column == TAG_EDITOR_COLUMN_DIRECTORIES) {
            return &screen->directory_filter_constraint;
        }
        if (screen->active_column == TAG_EDITOR_COLUMN_TAGS) {
            return &screen->tag_filter_constraint;
        }
    }
#endif
    return NULL;
}

static StrBuilder *
current_screen_search_buffer(void) {
    if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST)) {
        return &app_screen_playlist()->search_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_BROWSER)) {
        return &app_screen_browser()->search_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        PlaylistEditorScreen *screen;

        screen = app_screen_playlist_editor();
        if (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT) {
            return &screen->content_search_constraint;
        }
        return &screen->playlist_search_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_SEARCH_ENGINE)) {
        return &app_screen_search_engine()->search_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_HELP)) {
        return &app_screen_help()->search_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_LASTFM)) {
        return &app_screen_lastfm()->search_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_LYRICS)) {
        return &app_screen_lyrics()->search_constraint;
    }
    if (current_screen_is(NC_SCREEN_TYPE_MEDIA_LIBRARY)) {
        return media_library_screen_active_search_constraint(
            app_screen_media_library());
    }
    if (current_screen_is(NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER)) {
        return &app_screen_selected_items_adder()->search_constraint;
    }
#if defined(HAVE_TAGLIB_H)
    if (current_screen_is(NC_SCREEN_TYPE_TAG_EDITOR)) {
        TagEditorScreen *screen;

        screen = app_screen_tag_editor();
        if (screen->active_column == TAG_EDITOR_COLUMN_DIRECTORIES) {
            return &screen->directory_search_constraint;
        }
        if (screen->active_column == TAG_EDITOR_COLUMN_TAGS) {
            return &screen->tag_search_constraint;
        }
    }
#endif
    return NULL;
}

static bool
current_screen_set_search_constraint(char *pattern, int32 pattern_len) {
    StrBuilder *buffer;

    if ((buffer = current_screen_search_buffer()) == NULL) {
        return false;
    }
    return sb_set(buffer, pattern, pattern_len);
}

static bool
current_screen_search_direction_forward(enum SearchDirection direction) {
    if (direction == NCM_SEARCH_DIRECTION_FORWARD) {
        return true;
    }
    return false;
}

static bool
current_screen_search_error(NcmError *ncm_error) {
    return ncm_error_is_set(ncm_error);
}

static void
current_screen_clear_current_search_constraint(void) {
    StrBuilder *buffer;

    if (current_screen_is(NC_SCREEN_TYPE_HELP)) {
        nc_help_screen_clear_search(app_screen_help());
        return;
    }
    if (current_screen_is(NC_SCREEN_TYPE_LASTFM)) {
        (void)lastfm_screen_find(app_screen_lastfm(), NULL, 0,
                                 NULL);
        return;
    }
    if (current_screen_is(NC_SCREEN_TYPE_LYRICS)) {
        (void)lyrics_screen_find(app_screen_lyrics(), NULL, 0,
                                 NULL);
        return;
    }
    if (current_screen_is(NC_SCREEN_TYPE_MEDIA_LIBRARY)) {
        media_library_screen_clear_search(
            app_screen_media_library());
        return;
    }
    if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        PlaylistEditorScreen *screen;

        screen = app_screen_playlist_editor();
        if (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT) {
            screen->content_search_enabled = false;
            sb_clear(&screen->content_search_constraint);
        } else {
            screen->playlist_search_enabled = false;
            sb_clear(&screen->playlist_search_constraint);
        }
        return;
    }
#if defined(HAVE_TAGLIB_H)
    if (current_screen_is(NC_SCREEN_TYPE_TAG_EDITOR)) {
        TagEditorScreen *screen;

        screen = app_screen_tag_editor();
        if (screen->active_column == TAG_EDITOR_COLUMN_DIRECTORIES) {
            screen->directory_search_enabled = false;
            sb_clear(&screen->directory_search_constraint);
        } else if (screen->active_column == TAG_EDITOR_COLUMN_TAGS) {
            screen->tag_search_enabled = false;
            sb_clear(&screen->tag_search_constraint);
        }
        return;
    }
#endif
    if ((buffer = current_screen_search_buffer())) {
        sb_clear(buffer);
    }
    return;
}

static void
current_screen_finish_immediate_change(void) {
    NcScreen *screen;

    if ((screen = current_screen()) == NULL) {
        return;
    }
    if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST)) {
        playlist_screen_request_highlighting(app_screen_playlist());
    }
    nc_screen_refresh_window(screen);
    return;
}

bool
current_screen_allows_filter(void) {
    return current_screen_filter_buffer();
}

NcmStringView
current_screen_current_filter(void) {
    StrBuilder *buffer;

    if ((buffer = current_screen_filter_buffer()) == NULL) {
        return ncm_string_view_make(NULL, 0);
    }
    return ncm_string_view_make(buffer->data, buffer->len);
}

bool
current_screen_apply_filter(char *pattern, int32 pattern_len, NcmError *ncm_error) {
    bool result;

    result = false;
    if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST)) {
        result = playlist_screen_apply_filter(
            app_screen_playlist(), pattern, pattern_len, ncm_error);
    } else if (current_screen_is(NC_SCREEN_TYPE_BROWSER)) {
        result = browser_screen_apply_filter(
            app_screen_browser(), pattern, pattern_len, ncm_error);
    } else if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        result = playlist_editor_screen_apply_active_filter(
            app_screen_playlist_editor(), pattern, pattern_len,
            Config.regex_flags, ncm_error);
    } else if (current_screen_is(NC_SCREEN_TYPE_SEARCH_ENGINE)) {
        result = search_engine_screen_apply_filter(
            app_screen_search_engine(), pattern, pattern_len, ncm_error);
    } else if (current_screen_is(NC_SCREEN_TYPE_MEDIA_LIBRARY)) {
        result = media_library_screen_apply_filter(
            app_screen_media_library(), pattern, pattern_len, ncm_error);
#if defined(HAVE_TAGLIB_H)
    } else if (current_screen_is(NC_SCREEN_TYPE_TAG_EDITOR)) {
        TagEditorScreen *screen;

        screen = app_screen_tag_editor();
        if (screen->active_column == TAG_EDITOR_COLUMN_DIRECTORIES) {
            result = tag_editor_screen_apply_directory_filter(
                screen, pattern, pattern_len, Config.regex_flags, ncm_error);
        } else if (screen->active_column == TAG_EDITOR_COLUMN_TAGS) {
            result = tag_editor_screen_apply_tag_filter(
                screen, pattern, pattern_len, Config.regex_flags, ncm_error);
        }
#endif
    }

    if (result) {
        current_screen_finish_immediate_change();
    }
    return result;
}

NcmStringView
current_screen_current_search_constraint(void) {
    StrBuilder *buffer;

    if ((buffer = current_screen_search_buffer()) == NULL) {
        return ncm_string_view_make(NULL, 0);
    }
    return ncm_string_view_make(buffer->data, buffer->len);
}

bool
current_screen_allows_search(void) {
    if (current_screen_is(NC_SCREEN_TYPE_SEARCH_ENGINE)) {
        return search_engine_screen_allows_search(
            app_screen_search_engine());
    }
    if (current_screen_is(NC_SCREEN_TYPE_TAG_EDITOR)) {
#if defined(HAVE_TAGLIB_H)
        TagEditorScreen *screen;

        screen = app_screen_tag_editor();
        return screen->active_column != TAG_EDITOR_COLUMN_TAG_TYPES;
#else
        return false;
#endif
    }
    return current_screen_search_buffer();
}

bool
current_screen_search(enum SearchDirection direction, char *pattern,
                      int32 pattern_len, bool wrap, bool skip_current,
                      NcmError *ncm_error) {
    bool attempted;
    bool forward;
    bool found;

    if ((pattern == NULL) || (pattern_len <= 0)) {
        if (current_screen_allows_search()) {
            current_screen_clear_current_search_constraint();
            current_screen_finish_immediate_change();
        }
        return false;
    }

    attempted = false;
    forward = current_screen_search_direction_forward(direction);
    found = false;

    if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST)) {
        attempted = true;
        found = playlist_screen_search(app_screen_playlist(),
                                       pattern, pattern_len, forward,
                                       wrap, skip_current, ncm_error);
    } else if (current_screen_is(NC_SCREEN_TYPE_BROWSER)) {
        attempted = true;
        found = browser_screen_search(app_screen_browser(), pattern,
                                      pattern_len, forward, wrap,
                                      skip_current, ncm_error);
    } else if (current_screen_is(NC_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        attempted = true;
        found = playlist_editor_screen_search_active(
            app_screen_playlist_editor(), pattern, pattern_len,
            Config.regex_flags, forward, wrap, skip_current, ncm_error);
    } else if (current_screen_is(NC_SCREEN_TYPE_SEARCH_ENGINE)) {
        attempted = true;
        found = search_engine_screen_search(
            app_screen_search_engine(), pattern, pattern_len, forward,
            wrap, skip_current, ncm_error);
    } else if (current_screen_is(NC_SCREEN_TYPE_HELP)) {
        attempted = true;
        found = nc_help_screen_find(app_screen_help(), pattern,
                                    pattern_len, ncm_error);
    } else if (current_screen_is(NC_SCREEN_TYPE_LASTFM)) {
        attempted = true;
        found = lastfm_screen_find(app_screen_lastfm(), pattern,
                                   pattern_len, ncm_error);
    } else if (current_screen_is(NC_SCREEN_TYPE_LYRICS)) {
        attempted = true;
        found = lyrics_screen_find(app_screen_lyrics(), pattern,
                                   pattern_len, ncm_error);
    } else if (current_screen_is(NC_SCREEN_TYPE_MEDIA_LIBRARY)) {
        attempted = true;
        found = media_library_screen_search(
            app_screen_media_library(), pattern, pattern_len, forward,
            wrap, skip_current, ncm_error);
    } else if (current_screen_is(NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER)) {
        attempted = true;
        found = selected_items_adder_screen_search(
            app_screen_selected_items_adder(), pattern, pattern_len,
            Config.regex_flags, forward, wrap, skip_current, ncm_error);
#if defined(HAVE_TAGLIB_H)
    } else if (current_screen_is(NC_SCREEN_TYPE_TAG_EDITOR)) {
        attempted = true;
        found = tag_editor_screen_search(app_screen_tag_editor(),
                                         pattern, pattern_len, forward,
                                         wrap, skip_current, ncm_error);
#endif
    }

    if (attempted && !current_screen_search_error(ncm_error)) {
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

#endif /* NCMPCPP_SCREEN_ACTIONS_C */
