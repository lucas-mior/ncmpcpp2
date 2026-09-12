#if !defined(NC_TAG_EDIT_C)
#define NC_TAG_EDIT_C

#include "cbase.h"
#include "ncmpcpp2.h"

#include "app_controller.h"
#include "c/ncm_c.h"
#include "global.h"
#include "screens/nc_screens.h"
#include "settings.h"
#include "statusbar.h"
#include "title.h"
#include "ui_state.h"

enum TagEditParserActionRow {
    TAG_EDIT_PARSER_ACTION_PATTERN = 0,
    TAG_EDIT_PARSER_ACTION_PREVIEW = 1,
    TAG_EDIT_PARSER_ACTION_LEGEND = 2,
    TAG_EDIT_PARSER_ACTION_PROCEED = 4,
    TAG_EDIT_PARSER_ACTION_CANCEL = 5,
    TAG_EDIT_PARSER_ACTION_RECENT_START = 9,
};

#define TAG_EDIT_PATTERN_HISTORY_MAX 30

#define TAG_EDIT_FILENAME_ROW (TAG_COUNT + 1)

static bool
tag_edit_choice_is_filename(int32 choice) {
    return choice == TAG_EDIT_FILENAME_ROW;
}

#define ENUM_NAME TagEditTagTypeAction
#define ENUM_PREFIX_ TAG_EDIT_TAG_TYPE_ACTION_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                   \
    XX(TAG_EDIT_TAG_TYPE_ACTION_NONE, none)                           \
    XX(TAG_EDIT_TAG_TYPE_ACTION_FIELD, Field)                         \
    XX(TAG_EDIT_TAG_TYPE_ACTION_NUMBER_TRACKS, Track number)          \
    XX(TAG_EDIT_TAG_TYPE_ACTION_FILENAME, Filename)                   \
    XX(TAG_EDIT_TAG_TYPE_ACTION_CAPITALIZE, Capitalize First Letters) \
    XX(TAG_EDIT_TAG_TYPE_ACTION_LOWER, lower all letters)             \
    XX(TAG_EDIT_TAG_TYPE_ACTION_RESET, Reset)                         \
    XX(TAG_EDIT_TAG_TYPE_ACTION_SAVE, Save)
#include "cbase/xenums.c"

typedef struct SaveContext SaveContext;

// callbacks

static void
tag_edit_append_formatted_color_end(NcBuffer *buffer, NcFormattedColor *color) {
    nc_buffer_add_formatted_color_end(buffer, buffer->len, color, 0);
    return;
}

static void
tag_edit_append_formatted_color(NcBuffer *buffer, NcFormattedColor *color) {
    nc_buffer_add_formatted_color(buffer, buffer->len, color, 0);
    return;
}

static void
tag_edit_draw_tag(NcMenu *menu, NcWindow *window, void *item,
                  int32 pos, void *user) {
    TagEditScreen *screen = user;
    MutableSong *song = item;
    NcBuffer buffer = {0};
    NcMenu *tag_types;
    int32 choice;

    (void)menu;
    (void)pos;

    ASSERT(screen != NULL);
    ASSERT(window != NULL);
    ASSERT(song != NULL);

    if (mutable_song_is_modified(song)) {
        nc_buffer_append_data(&buffer,
                              Config.modified_item_prefix.data,
                              Config.modified_item_prefix.len);
    }

    tag_types = nc_editor_string_menu_base(&screen->tag_types);
    choice = nc_menu_highlight(tag_types);
    if (choice < (int32)TAG_COUNT) {
        StrBuilder tag;
        enum TagType tag_type = ncm_song_info_tags[choice].tag;

        tag = mutable_song_tags_buffer(song, tag_type,
                                       Config.tags_separator,
                                       Config.tags_separator_len,
                                       Config.show_duplicate_tags);
        if (tag.len <= 0) {
            tag_edit_append_formatted_color(&buffer, &Config.empty_tag_color);
            nc_buffer_append_data(&buffer,
                                  Config.empty_tag_marker,
                                  Config.empty_tag_marker_len);
            tag_edit_append_formatted_color_end(&buffer,
                                                &Config.empty_tag_color);
        } else {
            nc_buffer_append_data(&buffer, tag.data, tag.len);
        }
        sb_free(&tag);
    } else if (tag_edit_choice_is_filename(choice)) {
        nc_buffer_append_data(&buffer, song->name, song->name_len);
        if (song->new_name && (song->new_name_len > 0)) {
            tag_edit_append_formatted_color(&buffer, &Config.color2);
            nc_buffer_append_data(&buffer, STRLIT(" -> "));
            tag_edit_append_formatted_color_end(&buffer, &Config.color2);
            nc_buffer_append_data(&buffer, song->new_name, song->new_name_len);
        }
    }

    {
        NcBufferProperty *properties = nc_buffer_properties(&buffer);
        char *data = nc_buffer_data(&buffer);
        int32 len = buffer.len;
        int32 property_len = ARRAY_LEN(buffer.properties);
        int32 property_index = 0;

        for (int32 i = 0; ; i += 1) {
            while ((property_index < property_len)
                   && (properties[property_index].position == i)) {
                nc_buffer_apply_property(window, &properties[property_index]);
                property_index += 1;
            }
            if (i >= len) {
                break;
            }
            nc_window_print_char(window, data[i]);
        }
    }

    nc_buffer_destroy(&buffer);
    return;
}

static TagEditScreen *
tag_edit_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcWindow *
tag_edit_active_window(NcScreen *screen) {
    TagEditScreen *editor = tag_edit_from_screen(screen);

    return tag_edit_screen_active_window(editor);
}

static NcMenu *
tag_edit_menu_capability(NcScreen *base) {
    return tag_edit_screen_active_menu(tag_edit_from_screen(base));
}

static int32
tag_edit_menu_height_capability(NcScreen *base) {
    TagEditScreen *screen = tag_edit_from_screen(base);

    return nc_window_height(tag_edit_screen_active_window(screen));
}

static bool
tag_edit_filter_available_capability(NcScreen *base) {
    TagEditScreen *screen = tag_edit_from_screen(base);

    return (screen->active_column == TAG_EDIT_COLUMN_DIRECTORIES)
           || (screen->active_column == TAG_EDIT_COLUMN_TAGS);
}

static StringView
tag_edit_filter_constraint_capability(NcScreen *base) {
    TagEditScreen *screen = tag_edit_from_screen(base);
    StrBuilder *constraint;

    if (screen->active_column == TAG_EDIT_COLUMN_DIRECTORIES) {
        constraint = &screen->directory_filter_constraint;
    } else if (screen->active_column == TAG_EDIT_COLUMN_TAGS) {
        constraint = &screen->tag_filter_constraint;
    } else {
        return ncm_string_view(NULL, 0);
    }
    return ncm_string_view(constraint->data, constraint->len);
}

static int32
tag_edit_filter_apply_capability(NcScreen *base,
                                 char *pattern, int32 pattern_len,
                                 uint32 regex_flags, NcmError *ncm_error) {
    TagEditScreen *screen = tag_edit_from_screen(base);

    if (screen->active_column == TAG_EDIT_COLUMN_DIRECTORIES) {
        return tag_edit_screen_apply_directory_filter(screen, pattern,
                                                      pattern_len,
                                                      regex_flags,
                                                      ncm_error);
    }
    if (screen->active_column == TAG_EDIT_COLUMN_TAGS) {
        return tag_edit_screen_apply_tag_filter(screen, pattern, pattern_len,
                                                regex_flags, ncm_error);
    }
    return ncm_error_set_code(ncm_error, NCM_ERROR_UNAVAILABLE,
                              STRLIT("tag editor cannot filter"));
}

static bool
tag_edit_search_available_capability(NcScreen *base) {
    TagEditScreen *screen = tag_edit_from_screen(base);

    return (screen->active_column == TAG_EDIT_COLUMN_DIRECTORIES)
           || (screen->active_column == TAG_EDIT_COLUMN_TAGS);
}

static StringView
tag_edit_search_constraint_capability(NcScreen *base) {
    TagEditScreen *screen = tag_edit_from_screen(base);
    StrBuilder *constraint;

    if (screen->active_column == TAG_EDIT_COLUMN_DIRECTORIES) {
        constraint = &screen->directory_search_constraint;
    } else if (screen->active_column == TAG_EDIT_COLUMN_TAGS) {
        constraint = &screen->tag_search_constraint;
    } else {
        return ncm_string_view(NULL, 0);
    }
    return ncm_string_view(constraint->data, constraint->len);
}

static void
tag_edit_search_clear_capability(NcScreen *base) {
    TagEditScreen *screen = tag_edit_from_screen(base);

    if (screen->active_column == TAG_EDIT_COLUMN_DIRECTORIES) {
        screen->directory_search_enabled = false;
        sb_clear(&screen->directory_search_constraint);
    } else if (screen->active_column == TAG_EDIT_COLUMN_TAGS) {
        screen->tag_search_enabled = false;
        sb_clear(&screen->tag_search_constraint);
    }
    return;
}

static int32
tag_edit_search_capability(NcScreen *base, enum SearchDirection direction,
                           char *pattern, int32 pattern_len,
                           uint32 regex_flags, bool wrap, bool skip_current,
                           NcmError *ncm_error) {
    bool forward;

    (void)regex_flags;
    forward = direction == NCM_SEARCH_DIRECTION_FORWARD;
    return tag_edit_screen_search(tag_edit_from_screen(base), pattern,
                                  pattern_len, forward, wrap, skip_current,
                                  ncm_error);
}

static int32
tag_edit_selected_songs_capability(NcScreen *base, NcmSongArray *songs) {
    return tag_edit_screen_selected_songs(tag_edit_from_screen(base), songs);
}

NC_SCREEN_COLUMN_CAPABILITY_CALLBACKS(tag_edit, TagEditScreen,
                                      tag_edit_screen_previous_column_available,
                                      tag_edit_screen_next_column_available,
                                      tag_edit_screen_previous_column,
                                      tag_edit_screen_next_column)

static NcMenu *
tag_edit_tag_menu_capability(NcScreen *base) {
    TagEditScreen *screen = tag_edit_from_screen(base);

    if (screen->active_focus != TAG_EDIT_FOCUS_TAGS) {
        return NULL;
    }
    return tag_edit_screen_active_menu(screen);
}

static int32
tag_edit_tag_at_capability(NcScreen *base, int32 pos,
                           enum SongGetter getter, StrBuilder *tag) {
    TagEditScreen *screen = tag_edit_from_screen(base);
    NcMenu *menu;

    if (screen->active_focus != TAG_EDIT_FOCUS_TAGS) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    menu = tag_edit_screen_active_menu(screen);
    return nc_screen_menu_mutable_song_tag_at(menu, pos, getter, tag);
}

static bool
tag_edit_focus_is_parser_helper(enum TagEditFocus focus) {
    return (focus == TAG_EDIT_FOCUS_PARSER_LEGEND)
           || (focus == TAG_EDIT_FOCUS_PARSER_PREVIEW);
}

static void
tag_edit_update_menu_highlights(TagEditScreen *screen) {
    NcMenu *directories;
    NcMenu *tag_types;
    NcMenu *tags;
    NcMenu *parser_dialog;
    NcMenu *parser_rows;
    NcMenu *parser_actions;
    NcMenu *active;

    directories = nc_editor_pair_menu_base(&screen->directories);
    tag_types = nc_editor_string_menu_base(&screen->tag_types);
    tags = nc_tag_row_menu_base(&screen->tags);
    parser_dialog = nc_editor_string_menu_base(&screen->parser_dialog);
    parser_rows = nc_editor_string_menu_base(&screen->parser_rows);
    parser_actions = nc_editor_string_menu_base(&screen->parser_actions);

    nc_menu_set_highlight_prefix(directories,
                                 &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(directories,
                                 &Config.current_item_inactive_column_suffix);
    nc_menu_set_highlight_prefix(tag_types,
                                 &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(tag_types,
                                 &Config.current_item_inactive_column_suffix);
    nc_menu_set_highlight_prefix(tags,
                                 &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(tags,
                                 &Config.current_item_inactive_column_suffix);
    nc_menu_set_highlight_prefix(parser_dialog,
                                 &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(parser_dialog,
                                 &Config.current_item_inactive_column_suffix);
    nc_menu_set_highlight_prefix(parser_rows,
                                 &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(parser_rows,
                                 &Config.current_item_inactive_column_suffix);
    nc_menu_set_highlight_prefix(parser_actions,
                                 &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(parser_actions,
                                 &Config.current_item_inactive_column_suffix);
    nc_menu_set_highlighting(
        parser_actions, !tag_edit_focus_is_parser_helper(screen->active_focus));

    if ((active = tag_edit_screen_active_menu(screen))) {
        nc_menu_set_highlight_prefix(active, &Config.current_item_prefix);
        nc_menu_set_highlight_suffix(active, &Config.current_item_suffix);
    }
    {
        NcBorder dialog_border = Config.window_border_color;
        NcBorder parser_border = Config.window_border_color;
        NcBorder helper_border = Config.window_border_color;

        if (screen->active_focus == TAG_EDIT_FOCUS_PARSER_CHOICE) {
            dialog_border = Config.active_window_border;
        } else if (screen->active_focus == TAG_EDIT_FOCUS_PARSER_ACTIONS) {
            parser_border = Config.active_window_border;
        } else if (tag_edit_focus_is_parser_helper(screen->active_focus)) {
            helper_border = Config.active_window_border;
        }

        nc_window_set_border(&screen->parser_dialog_window, dialog_border);
        nc_window_set_border(&screen->parser_window, parser_border);
        nc_window_set_border(&screen->parser_helper_window, helper_border);
    }
    return;
}

static void
tag_edit_update_titles(TagEditScreen *screen, bool update_windows) {
    ASSERT(screen != NULL);

    screen->last_known_directory_count =
        nc_menu_item_count(nc_editor_pair_menu_base(&screen->directories));
    screen->last_known_tag_count =
        nc_menu_item_count(nc_tag_row_menu_base(&screen->tags));

    sb_clear(&screen->directories_title);
    sb_clear(&screen->tag_types_title);
    sb_clear(&screen->tags_title);
    sb_clear(&screen->parser_dialog_title);
    sb_clear(&screen->parser_title);
    sb_clear(&screen->parser_helper_title);

    if (Config.titles_visibility) {
        SB_APPEND(&screen->directories_title, "Directories");
        SB_APPEND(&screen->tag_types_title, "Tag types");
        SB_APPEND(&screen->tags_title, "Tags");
        if (screen->parser_mode == TAG_EDIT_PARSER_MODE_TAGS_FROM_FILENAME) {
            SB_APPEND(&screen->parser_title, "Get tags from filename");
        } else if (screen->parser_mode == TAG_EDIT_PARSER_MODE_RENAME_FILES) {
            SB_APPEND(&screen->parser_title, "Rename files");
        } else {
            SB_APPEND(&screen->parser_title, "Pattern");
        }
        if ((screen->active_focus == TAG_EDIT_FOCUS_PARSER_LEGEND)
            || !screen->parser_preview_enabled) {
            SB_APPEND(&screen->parser_helper_title, "Legend");
        } else {
            SB_APPEND(&screen->parser_helper_title, "Preview");
        }
    }

    if (!update_windows) {
        return;
    }
    nc_window_set_title(&screen->directories_window,
                        screen->directories_title.data,
                        screen->directories_title.len);
    nc_window_set_title(&screen->tag_types_window,
                        screen->tag_types_title.data,
                        screen->tag_types_title.len);
    nc_window_set_title(&screen->tags_window,
                        screen->tags_title.data,
                        screen->tags_title.len);
    nc_window_set_title(&screen->parser_dialog_window,
                        screen->parser_dialog_title.data,
                        screen->parser_dialog_title.len);
    nc_window_set_title(&screen->parser_window,
                        screen->parser_title.data,
                        screen->parser_title.len);
    nc_window_set_title(&screen->parser_helper_window,
                        screen->parser_helper_title.data,
                        screen->parser_helper_title.len);

    return;
}

static void
tag_edit_refresh_menu(NcWindow *window, NcMenu *menu) {
    ASSERT(window != NULL);
    ASSERT(menu != NULL);
    nc_menu_prepare_refresh(menu, nc_window_height(window), NULL, NULL);
    nc_window_display(window);
    nc_menu_refresh(menu, window,
                    nc_window_width(window), nc_window_height(window));
    return;
}

static void
tag_edit_refresh_active_helper(TagEditScreen *screen) {
    NcBuffer display = {0};
    StrBuilder *source;

    if (screen->active_focus == TAG_EDIT_FOCUS_PARSER_PREVIEW) {
        source = &screen->parser_preview;
    } else {
        source = &screen->parser_legend;
    }
    if (source->data && (source->len > 0)) {
        nc_buffer_append_data(&display, source->data, source->len);
    }
    nc_scrollpad_flush(&screen->parser_helper_scrollpad,
                       &screen->parser_helper_window, &display);
    nc_window_refresh_border(&screen->parser_helper_window);
    nc_scrollpad_refresh(&screen->parser_helper_scrollpad,
                         &screen->parser_helper_window);
    nc_buffer_destroy(&display);
    return;
}

static int32
tag_edit_separator_width(TagEditScreen *screen) {
    ASSERT(screen != NULL);
    if (screen->width >= 5) {
        return 1;
    }
    return 0;
}

static void
tag_edit_refresh(NcScreen *screen) {
    TagEditScreen *editor = tag_edit_from_screen(screen);

    tag_edit_update_titles(editor, true);
    tag_edit_update_menu_highlights(editor);
    if (editor->active_focus == TAG_EDIT_FOCUS_PARSER_CHOICE) {
        NcMenu *menu = nc_editor_string_menu_base(&editor->parser_dialog);
        tag_edit_refresh_menu(&editor->parser_dialog_window, menu);
        return;
    }
    if ((editor->active_focus == TAG_EDIT_FOCUS_PARSER_ACTIONS)
        || tag_edit_focus_is_parser_helper(editor->active_focus)) {
        NcMenu *menu = nc_editor_string_menu_base(&editor->parser_actions);
        tag_edit_refresh_menu(&editor->parser_window, menu);
        tag_edit_refresh_active_helper(editor);
        return;
    }

    {
        NcMenu *menu = nc_editor_pair_menu_base(&editor->directories);
        tag_edit_refresh_menu(&editor->directories_window, menu);
    }
    if (tag_edit_separator_width(editor) > 0) {
        nc_screen_draw_vertical_separator(editor->middle_start_x - 1);
        nc_screen_draw_vertical_separator(editor->right_start_x - 1);
    }
    {
        NcMenu *menu = nc_editor_string_menu_base(&editor->tag_types);
        tag_edit_refresh_menu(&editor->tag_types_window, menu);
    }
    tag_edit_refresh_menu(&editor->tags_window,
                          nc_tag_row_menu_base(&editor->tags));
    return;
}

static void
tag_edit_refresh_window(NcScreen *screen) {
    TagEditScreen *editor = tag_edit_from_screen(screen);

    tag_edit_update_titles(editor, true);
    tag_edit_update_menu_highlights(editor);
    if (tag_edit_focus_is_parser_helper(editor->active_focus)) {
        NcMenu *menu = nc_editor_string_menu_base(&editor->parser_actions);
        tag_edit_refresh_menu(&editor->parser_window, menu);
        tag_edit_refresh_active_helper(editor);
        return;
    }

    {
        NcMenu *menu = tag_edit_screen_active_menu(editor);
        NcWindow *window = tag_edit_screen_active_window(editor);
        tag_edit_refresh_menu(window, menu);
    }
    return;
}

static void
tag_edit_finish_tag_type_change(TagEditScreen *screen, bool refresh_tags) {
    NcMenu *menu;
    int32 highlight;

    ASSERT(screen != NULL);
    if (screen->active_focus != TAG_EDIT_FOCUS_TAG_TYPES) {
        return;
    }
    menu = nc_editor_string_menu_base(&screen->tag_types);
    highlight = nc_menu_highlight(menu);
    if (screen->last_tag_type_highlight == highlight) {
        return;
    }
    screen->last_tag_type_highlight = highlight;
    if (refresh_tags) {
        tag_edit_refresh_menu(&screen->tags_window,
                              nc_tag_row_menu_base(&screen->tags));
    }
    return;
}

static void
tag_edit_scroll(NcScreen *screen, enum NcScroll where) {
    TagEditScreen *editor = tag_edit_from_screen(screen);
    NcMenu *menu = tag_edit_screen_active_menu(editor);
    NcWindow *window = tag_edit_screen_active_window(editor);

    if (menu) {
        nc_menu_scroll_selectable(menu, nc_window_height(window), where);
    } else if (tag_edit_focus_is_parser_helper(editor->active_focus)) {
        nc_scrollpad_scroll(&editor->parser_helper_scrollpad, window, where);
        tag_edit_refresh_active_helper(editor);
    } else if (window) {
        nc_window_scroll(window, where);
    }
    tag_edit_screen_finish_directory_change(editor);
    tag_edit_finish_tag_type_change(editor, true);
    tag_edit_update_menu_highlights(editor);
    return;
}

static enum TagEditTagTypeAction
tag_edit_current_tag_type_action(TagEditScreen *screen,
                                 enum TagType *tag_type) {
    NcMenu *menu;
    StrBuilder *row;
    enum TagEditTagTypeAction action;
    int32 choice;

    ASSERT(screen != NULL);
    ASSERT(tag_type != NULL);

    *tag_type = TAG_COUNT;
    menu = nc_editor_string_menu_base(&screen->tag_types);
    choice = nc_menu_highlight(menu);
    if (((row = nc_menu_current_item(menu)) == NULL)
        || !nc_menu_current_is_selectable(menu)) {
        return TAG_EDIT_TAG_TYPE_ACTION_NONE;
    }

    if (choice < (int32)TAG_COUNT) {
        *tag_type = ncm_song_info_tags[choice].tag;
        if ((ncm_song_info_tags[choice].tag == TAG_TRACK)
            && (screen->active_focus == TAG_EDIT_FOCUS_TAG_TYPES)) {
            return TAG_EDIT_TAG_TYPE_ACTION_NUMBER_TRACKS;
        }
        return TAG_EDIT_TAG_TYPE_ACTION_FIELD;
    }
    if (tag_edit_choice_is_filename(choice)) {
        return TAG_EDIT_TAG_TYPE_ACTION_FILENAME;
    }

    action = TAG_EDIT_TAG_TYPE_ACTION_parse(row->data, row->len);
    if (action != TAG_EDIT_TAG_TYPE_ACTION_COUNT) {
        return action;
    }
    return TAG_EDIT_TAG_TYPE_ACTION_NONE;
}

static bool
tag_edit_can_run_current(NcScreen *screen) {
    TagEditScreen *editor = tag_edit_from_screen(screen);
    NcMenu *menu;
    enum TagType tag_type;

    switch (editor->active_focus) {
    case TAG_EDIT_FOCUS_DIRECTORIES:
    case TAG_EDIT_FOCUS_PARSER_CHOICE:
        menu = tag_edit_screen_active_menu(editor);
        ASSERT(menu != NULL);
        return nc_menu_current_is_selectable(menu);
    case TAG_EDIT_FOCUS_PARSER_ACTIONS:
        menu = tag_edit_screen_active_menu(editor);
        ASSERT(menu != NULL);
        if (!nc_menu_current_is_selectable(menu)) {
            return false;
        }
        switch (nc_menu_highlight(menu)) {
        case TAG_EDIT_PARSER_ACTION_PATTERN:
        case TAG_EDIT_PARSER_ACTION_PREVIEW:
        case TAG_EDIT_PARSER_ACTION_LEGEND:
        case TAG_EDIT_PARSER_ACTION_PROCEED:
        case TAG_EDIT_PARSER_ACTION_CANCEL:
            return true;
        default:
            return nc_menu_highlight(menu)
                   >= (int32)TAG_EDIT_PARSER_ACTION_RECENT_START;
        }
    case TAG_EDIT_FOCUS_TAG_TYPES:
        if (nc_menu_item_count(nc_tag_row_menu_base(&editor->tags)) <= 0) {
            return false;
        }
        return tag_edit_current_tag_type_action(editor, &tag_type)
               != TAG_EDIT_TAG_TYPE_ACTION_NONE;
    case TAG_EDIT_FOCUS_TAGS:
        if (nc_menu_item_count(nc_tag_row_menu_base(&editor->tags)) <= 0) {
            return false;
        }
        switch (tag_edit_current_tag_type_action(editor, &tag_type)) {
        case TAG_EDIT_TAG_TYPE_ACTION_FIELD:
        case TAG_EDIT_TAG_TYPE_ACTION_FILENAME:
            return true;
        case TAG_EDIT_TAG_TYPE_ACTION_NONE:
        case TAG_EDIT_TAG_TYPE_ACTION_NUMBER_TRACKS:
        case TAG_EDIT_TAG_TYPE_ACTION_CAPITALIZE:
        case TAG_EDIT_TAG_TYPE_ACTION_LOWER:
        case TAG_EDIT_TAG_TYPE_ACTION_RESET:
        case TAG_EDIT_TAG_TYPE_ACTION_SAVE:
            return false;
        case TAG_EDIT_TAG_TYPE_ACTION_COUNT:
        default:
            break;
        }
        return false;
    case TAG_EDIT_FOCUS_PARSER_LEGEND:
    case TAG_EDIT_FOCUS_PARSER_PREVIEW:
        return false;
    case TAG_EDIT_FOCUS_COUNT:
    default:
        break;
    }
    return false;
}

static void
tag_edit_status_message(TagEditScreen *screen,
                        char *message, int32 message_len) {
    if (screen->hooks.status_message) {
        screen->hooks.status_message(screen->hooks.user, message, message_len);
    }
    return;
}

static void
tag_edit_set_focus(TagEditScreen *screen, enum TagEditFocus focus) {
    enum TagEditFocus old_focus;

    ASSERT(screen != NULL);

    old_focus = screen->active_focus;
    screen->active_focus = focus;

    if (focus == TAG_EDIT_FOCUS_DIRECTORIES) {
        screen->active_column = TAG_EDIT_COLUMN_DIRECTORIES;
    } else if (focus == TAG_EDIT_FOCUS_TAG_TYPES) {
        screen->active_column = TAG_EDIT_COLUMN_TAG_TYPES;
    } else if (focus == TAG_EDIT_FOCUS_TAGS) {
        screen->active_column = TAG_EDIT_COLUMN_TAGS;
    } else if (focus == TAG_EDIT_FOCUS_PARSER_LEGEND) {
        screen->parser_preview_enabled = false;
        if (old_focus != focus) {
            nc_scrollpad_reset(&screen->parser_helper_scrollpad);
        }
    } else if (focus == TAG_EDIT_FOCUS_PARSER_PREVIEW) {
        screen->parser_preview_enabled = true;
        if (old_focus != focus) {
            nc_scrollpad_reset(&screen->parser_helper_scrollpad);
        }
    }

    tag_edit_update_menu_highlights(screen);
    return;
}

static bool
tag_edit_confirm(TagEditScreen *screen, char *message, int32 message_len) {
    if (screen->hooks.confirm == NULL) {
        return false;
    }
    return screen->hooks.confirm(screen->hooks.user, message, message_len);
}

static void
tag_edit_refresh_if_visible(TagEditScreen *screen) {
    NcScreen *base;

    ASSERT(screen != NULL);

    base = tag_edit_screen_base(screen);
    if (app_controller_is_screen_visible(base)) {
        nc_screen_refresh(base);
    }
    return;
}

static void
tag_edit_set_pattern(TagEditScreen *screen, char *pattern, int32 pattern_len) {
    ASSERT(screen != NULL);
    sb_set(&screen->pattern, pattern, pattern_len);
    return;
}

static char
tag_edit_ascii_lower(char c) {
    if ((c >= 'A') && (c <= 'Z')) {
        c = (char)(c - 'A' + 'a');
    }
    return c;
}

static void
tag_edit_append_lowercase(StrBuilder *buffer, char *data, int32 len) {
    for (int32 i = 0; i < len; i += 1) {
        sb_append_byte(buffer, tag_edit_ascii_lower(data[i]));
    }
    return;
}

static void
tag_edit_append_parser_legend_entry(StrBuilder *legend, char tag_char,
                                    char *name, int32 name_len) {
    sb_append_byte(legend, '%');
    sb_append_byte(legend, tag_char);
    SB_APPEND(legend, " - ");
    tag_edit_append_lowercase(legend, name, name_len);
    sb_append_byte(legend, '\n');
    return;
}

static void
tag_edit_append_parser_legend_field(StrBuilder *legend,
                                    enum TagType tag_type) {
    char *name;
    int32 name_len;
    char tag_char;

    tag_char = ncm_tag_type_format_char(tag_type);
    name_len = ncm_tag_type_parser_name_len(tag_type, &name);
    if ((tag_char == '\0') || (name_len <= 0)) {
        return;
    }

    tag_edit_append_parser_legend_entry(legend, tag_char, name, name_len);
    if (tag_type == TAG_TRACK) {
        tag_char = ncm_song_getter_format_char(SONG_GETTER_TRACK_TOTAL);
        name_len = SONG_GETTER_alias_len(SONG_GETTER_TRACK_TOTAL, &name);
        tag_edit_append_parser_legend_entry(legend, tag_char, name, name_len);
    }
    return;
}

#define TAG_EDIT_APPEND_PARSER_FIELD(suffix, display, tag_char) \
    tag_edit_append_parser_legend_field(&screen->parser_legend,                \
                                        CAT(TAG_, suffix));

static void
tag_edit_build_parser_legend(TagEditScreen *screen) {
    NcMenu *tags;
    int32 count;

    sb_clear(&screen->parser_legend);

    TAG_DEFS(TAG_EDIT_APPEND_PARSER_FIELD)
    SB_APPEND(&screen->parser_legend, "\nFiles:\n");

    tags = nc_tag_row_menu_base(&screen->tags);
    count = nc_menu_item_count(tags);
    for (int32 i = 0; i < count; i += 1) {
        MutableSong *song = nc_menu_active_item_at(tags, i);

        ASSERT(song != NULL);
        if (song->name == NULL) {
            continue;
        }
        SB_APPEND(&screen->parser_legend, " * ");
        SB_APPEND(&screen->parser_legend, song->name, song->name_len);
        sb_append_byte(&screen->parser_legend, '\n');
    }
    return;
}

#undef TAG_EDIT_APPEND_PARSER_FIELD

static int32
tag_edit_find_recent_pattern(TagEditScreen *screen,
                               char *pattern, int32 pattern_len) {
    if (pattern_len <= 0) {
        return -1;
    }
    for (int32 i = 0; i < screen->recent_patterns.len; i += 1) {
        StrBuilder *item = &screen->recent_patterns.items[i];

        if (STREQUAL(item->data, item->len, pattern, pattern_len)) {
            return i;
        }
    }
    return -1;
}

static void
tag_edit_history_path(StrBuilder *path) {
    ASSERT(path != NULL);
    if (Config.ncmpcpp_directory && (Config.ncmpcpp_directory_len > 0)) {
        ncm_fs_join(path,
                    Config.ncmpcpp_directory,
                    Config.ncmpcpp_directory_len,
                    STRLIT("patterns.list"));
        return;
    }
    sb_set(path, STRLIT("patterns.list"));
    return;
}

static int32
tag_edit_save_recent_patterns(TagEditScreen *screen) {
    StrBuilder path = {0};
    FILE *file;
    int32 limit;
    int32 status;

    tag_edit_history_path(&path);
    file = fopen(path.data, "w");
    if (file == NULL) {
        status = errno ? -errno : -EIO;
        sb_free(&path);
        return status;
    }
    status = 0;
    limit = screen->recent_patterns.len;
    if (limit > TAG_EDIT_PATTERN_HISTORY_MAX) {
        limit = TAG_EDIT_PATTERN_HISTORY_MAX;
    }
    for (int32 i = 0; i < limit; i += 1) {
        StrBuilder *pattern;

        pattern = &screen->recent_patterns.items[i];
        if ((pattern->len > 0)
            && (fwrite64(pattern->data, 1, pattern->len, file)
                != pattern->len)) {
            status = -EIO;
            break;
        }
        if (fputc('\n', file) == EOF) {
            status = errno ? -errno : -EIO;
            break;
        }
    }
    if ((fclose(file) == EOF) && (status == 0)) {
        status = errno ? -errno : -EIO;
    }
    sb_free(&path);
    return status;
}

static bool
tag_edit_prompt_tag_value(TagEditScreen *screen,
                          enum TagType tag_type, bool all_targets) {
    MutableSong *song;
    StrBuilder initial;
    StrBuilder input = {0};
    char *label;
    int32 label_len;
    enum TagEditPromptResult prompt_result;
    bool result;

    ASSERT(screen != NULL);
    if ((uint32)tag_type >= TAG_COUNT) {
        return false;
    }
    song = nc_tag_row_menu_current(&screen->tags);
    ASSERT(song != NULL);

    label_len = TAG_alias_len(tag_type, &label);
    initial = mutable_song_tags_buffer(song, tag_type,
                                       Config.tags_separator,
                                       Config.tags_separator_len,
                                       Config.show_duplicate_tags);
    if (screen->hooks.prompt == NULL) {
        prompt_result = TAG_EDIT_PROMPT_ERROR;
    } else {
        StringView initial_view;

        ncm_string_view_set(&initial_view, initial.data, initial.len);
        prompt_result = screen->hooks.prompt(screen->hooks.user, label,
                                             label_len, initial_view, &input);
    }
    sb_free(&initial);

    if (prompt_result == TAG_EDIT_PROMPT_ABORTED) {
        tag_edit_status_message(screen, STRLIT("Action aborted"));
        sb_free(&input);
        return false;
    }
    if (prompt_result != TAG_EDIT_PROMPT_ACCEPTED) {
        sb_free(&input);
        return false;
    }

    if (all_targets) {
        tag_edit_screen_apply_tag_to_selection(screen, tag_type,
                                               sb_opt_cstr(&input), input.len,
                                               Config.tags_separator,
                                               Config.tags_separator_len);
    } else {
        mutable_song_set_tags(song, tag_type, sb_opt_cstr(&input), input.len,
                              Config.tags_separator, Config.tags_separator_len);
    }
    result = true;
    sb_free(&input);
    return result;
}

static int32
tag_edit_build_parser_preview(TagEditScreen *screen,
                                bool apply, bool *success) {
    NcMenu *tags;
    int32 count;
    int32 status;

    ASSERT(screen != NULL);
    ASSERT(success != NULL);

    *success = true;
    tag_edit_status_message(screen, STRLIT("Parsing..."));
    sb_clear(&screen->parser_preview);
    tags = nc_tag_row_menu_base(&screen->tags);
    count = nc_menu_item_count(tags);
    for (int32 i = 0; i < count; i += 1) {
        MutableSong *song;

        song = nc_menu_active_item_at(tags, i);
        ASSERT(song != NULL);
        if (screen->parser_mode == TAG_EDIT_PARSER_MODE_TAGS_FROM_FILENAME) {
            if (!apply && song->name) {
                SB_APPEND(&screen->parser_preview, song->name, song->name_len);
                SB_APPEND(&screen->parser_preview, ":\n");
            }
            status = tag_edit_parse_filename(song,
                                             screen->pattern.data,
                                             screen->pattern.len,
                                             !apply, &screen->parser_preview);
            if ((status < 0) && !apply) {
                SB_APPEND(&screen->parser_preview,
                          "Error while parsing filename!\n");
            }
            if (!apply) {
                sb_append_byte(&screen->parser_preview, '\n');
            }
        } else if (screen->parser_mode == TAG_EDIT_PARSER_MODE_RENAME_FILES) {
            StrBuilder stem = {0};
            StrBuilder new_name = {0};
            int32 extension_start;

            status = tag_edit_generate_filename(song,
                                                screen->pattern.data,
                                                screen->pattern.len, &stem);
            if (status < 0) {
                sb_free(&new_name);
                sb_free(&stem);
                return status;
            }
            extension_start = -1;
            if ((song->name != NULL) && (song->name_len > 0)) {
                for (int32 j = song->name_len - 1; j > 0; j -= 1) {
                    if (song->name[j] == '.') {
                        extension_start = j;
                        break;
                    }
                }
            }
            SB_APPEND(&new_name, stem.data, stem.len);
            if ((extension_start >= 0) && song->name) {
                SB_APPEND(&new_name,
                          song->name + extension_start,
                          song->name_len - extension_start);
            }
            if (apply && (stem.len <= 0)) {
                sb_clear(&screen->parser_preview);
                SB_APPEND(&screen->parser_preview, "File \"");
                SB_APPEND(&screen->parser_preview, song->name, song->name_len);
                SB_APPEND(&screen->parser_preview, "\" would have an empty name");
                tag_edit_status_message(screen, screen->parser_preview.data,
                                        screen->parser_preview.len);
                screen->parser_preview_enabled = true;
                *success = false;
                sb_free(&new_name);
                sb_free(&stem);
                return 0;
            }
            if (apply) {
                mutable_song_set_new_name(song, new_name.data, new_name.len);
            } else {
                SB_APPEND(&screen->parser_preview, song->name, song->name_len);
                SB_APPEND(&screen->parser_preview, " -> ");
                if (new_name.len > 0) {
                    SB_APPEND(&screen->parser_preview,
                              new_name.data, new_name.len);
                } else if (Config.empty_tag_marker) {
                    SB_APPEND(&screen->parser_preview,
                              Config.empty_tag_marker,
                              Config.empty_tag_marker_len);
                }
                SB_APPEND(&screen->parser_preview, "\n\n");
            }
            sb_free(&new_name);
            sb_free(&stem);
        }
    }
    if (!apply) {
        screen->parser_preview_enabled = true;
    }
    return 0;
}

static int32
tag_edit_run_current(NcScreen *screen) {
    TagEditScreen *editor = tag_edit_from_screen(screen);

    switch (editor->active_focus) {
    case TAG_EDIT_FOCUS_DIRECTORIES:
        if (tag_edit_screen_enter_directory(editor) == 0) {
            return 0;
        }
        return -NCM_ERROR_UNAVAILABLE;
    case TAG_EDIT_FOCUS_TAG_TYPES: {
        enum TagType tag_type;

        switch (tag_edit_current_tag_type_action(editor, &tag_type)) {
        case TAG_EDIT_TAG_TYPE_ACTION_FIELD:
            if (tag_edit_prompt_tag_value(editor, tag_type, true)) {
                return 0;
            }
            return -NCM_ERROR_UNAVAILABLE;
        case TAG_EDIT_TAG_TYPE_ACTION_NUMBER_TRACKS:
            if (!tag_edit_confirm(editor, STRLIT("Number tracks?"))) {
                return -NCM_ERROR_UNAVAILABLE;
            }
            tag_edit_screen_number_tracks(editor,
                                          Config.tag_edit_extended_numeration);
            tag_edit_status_message(editor, STRLIT("Tracks numbered"));
            return 0;
        case TAG_EDIT_TAG_TYPE_ACTION_FILENAME:
            tag_edit_screen_show_parser_dialog(editor);
            return 0;
        case TAG_EDIT_TAG_TYPE_ACTION_CAPITALIZE:
            tag_edit_status_message(editor, STRLIT("Processing..."));
            tag_edit_screen_capitalize_first_letters(editor);
            tag_edit_status_message(editor, STRLIT("Done"));
            return 0;
        case TAG_EDIT_TAG_TYPE_ACTION_LOWER:
            tag_edit_status_message(editor, STRLIT("Processing..."));
            tag_edit_screen_lower_all_letters(editor);
            tag_edit_status_message(editor, STRLIT("Done"));
            return 0;
        case TAG_EDIT_TAG_TYPE_ACTION_RESET:
            tag_edit_screen_clear_modifications(editor);
            tag_edit_status_message(editor, STRLIT("Changes reset"));
            return 0;
        case TAG_EDIT_TAG_TYPE_ACTION_SAVE:
            if (tag_edit_screen_save_modified(editor,
                                              Config.mpd_music_dir) > 0) {
                return 0;
            }
            return -NCM_ERROR_UNAVAILABLE;
        case TAG_EDIT_TAG_TYPE_ACTION_NONE:
        case TAG_EDIT_TAG_TYPE_ACTION_COUNT:
        default:
            return -NCM_ERROR_UNAVAILABLE;
        }
    }
    case TAG_EDIT_FOCUS_TAGS: {
        enum TagEditTagTypeAction action;
        enum TagType tag_type;
        NcMenu *tags;
        bool result;

        action = tag_edit_current_tag_type_action(editor, &tag_type);
        if (action == TAG_EDIT_TAG_TYPE_ACTION_FIELD) {
            result = tag_edit_prompt_tag_value(editor, tag_type, false);
        } else if (action == TAG_EDIT_TAG_TYPE_ACTION_FILENAME) {
            MutableSong *song;
            StringView current_name;
            StringView initial;
            StrBuilder input = {0};
            enum TagEditPromptResult prompt_result;
            int32 dot = -1;

            ASSERT(editor != NULL);
            song = nc_tag_row_menu_current(&editor->tags);
            ASSERT(song != NULL);
            if (!mutable_song_has_new_name_view(song, &current_name)) {
                current_name.data = song->name;
                current_name.len = song->name_len;
            }
            initial = current_name;
            for (int32 i = 0; i < current_name.len; i += 1) {
                if (current_name.data[i] == '.') {
                    dot = i;
                }
            }
            if (dot >= 0) {
                initial.len = dot;
            }

            if (editor->hooks.prompt == NULL) {
                prompt_result = TAG_EDIT_PROMPT_ERROR;
            } else {
                prompt_result = editor->hooks.prompt(editor->hooks.user,
                                                     STRLIT("New filename"),
                                                     initial, &input);
            }
            if (prompt_result == TAG_EDIT_PROMPT_ABORTED) {
                tag_edit_status_message(editor, STRLIT("Action aborted"));
                result = false;
            } else if (prompt_result != TAG_EDIT_PROMPT_ACCEPTED) {
                result = false;
            } else if (input.len <= 0) {
                result = true;
            } else {
                StringView stem_name;
                StrBuilder new_name = {0};
                int32 stem_dot = -1;

                if (!mutable_song_has_new_name_view(song, &stem_name)) {
                    stem_name.data = song->name;
                    stem_name.len = song->name_len;
                }
                for (int32 i = 0; i < stem_name.len; i += 1) {
                    if (stem_name.data[i] == '.') {
                        stem_dot = i;
                    }
                }
                SB_APPEND(&new_name, input.data, input.len);
                if (stem_dot >= 0) {
                    SB_APPEND(&new_name,
                              stem_name.data + stem_dot,
                              stem_name.len - stem_dot);
                }
                mutable_song_set_new_name(song, new_name.data, new_name.len);
                sb_free(&new_name);
                result = true;
            }
            sb_free(&input);
        } else {
            return -NCM_ERROR_UNAVAILABLE;
        }

        if (result) {
            tags = nc_tag_row_menu_base(&editor->tags);
            nc_menu_scroll_selectable(tags,
                                      nc_window_height(&editor->tags_window),
                                      NC_SCROLL_DOWN);
            return 0;
        }
        return -NCM_ERROR_UNAVAILABLE;
    }
    case TAG_EDIT_FOCUS_PARSER_CHOICE: {
        NcMenu *menu;
        int32 choice;
        enum TagEditParserMode mode;

        menu = nc_editor_string_menu_base(&editor->parser_dialog);
        if (!nc_menu_current_is_selectable(menu)) {
            return -NCM_ERROR_UNAVAILABLE;
        }
        choice = nc_menu_highlight(menu);
        if (choice == 0) {
            mode = TAG_EDIT_PARSER_MODE_TAGS_FROM_FILENAME;
            tag_edit_screen_show_parser_actions(editor, mode);
            return 0;
        }
        if (choice == 1) {
            mode = TAG_EDIT_PARSER_MODE_RENAME_FILES;
            tag_edit_screen_show_parser_actions(editor, mode);
            return 0;
        }
        if (choice == 2) {
            tag_edit_screen_close_parser(editor);
            return 0;
        }
        return -NCM_ERROR_UNAVAILABLE;
    }
    case TAG_EDIT_FOCUS_PARSER_ACTIONS: {
        NcMenu *menu;
        int32 choice;
        bool success;
        int32 status;

        menu = nc_editor_string_menu_base(&editor->parser_actions);
        if (!nc_menu_current_is_selectable(menu)) {
            return -NCM_ERROR_UNAVAILABLE;
        }
        choice = nc_menu_highlight(menu);
        if (choice == TAG_EDIT_PARSER_ACTION_PATTERN) {
            bool result = false;

            if (editor->hooks.prompt != NULL) {
                StrBuilder input = {0};
                StringView initial;
                enum TagEditPromptResult prompt_result;

                initial.data = editor->pattern.data;
                initial.len = editor->pattern.len;
                prompt_result = editor->hooks.prompt(editor->hooks.user,
                                                     STRLIT("Pattern"),
                                                     initial, &input);
                if (prompt_result == TAG_EDIT_PROMPT_ABORTED) {
                    tag_edit_status_message(editor, STRLIT("Action aborted"));
                } else if (prompt_result != TAG_EDIT_PROMPT_ERROR) {
                    tag_edit_set_pattern(editor, input.data, input.len);
                    tag_edit_screen_prepare_parser_rows(editor,
                                                        editor->parser_mode,
                                                        editor->pattern.data,
                                                        editor->pattern.len);
                    result = true;
                }
                sb_free(&input);
            }
            if (result) {
                tag_edit_set_focus(editor, TAG_EDIT_FOCUS_PARSER_ACTIONS);
                nc_menu_goto_selectable(menu, TAG_EDIT_PARSER_ACTION_PATTERN);
                return 0;
            }
            return -NCM_ERROR_UNAVAILABLE;
        }
        if (choice == TAG_EDIT_PARSER_ACTION_PREVIEW) {
            status = tag_edit_build_parser_preview(editor, false, &success);
            if (status < 0) {
                return -NCM_ERROR_UNAVAILABLE;
            }
            tag_edit_screen_show_parser_preview(editor);
            tag_edit_status_message(editor, STRLIT("Operation finished"));
            return 0;
        }
        if (choice == TAG_EDIT_PARSER_ACTION_LEGEND) {
            tag_edit_build_parser_legend(editor);
            tag_edit_screen_show_parser_legend(editor);
            return 0;
        }
        if (choice == TAG_EDIT_PARSER_ACTION_PROCEED) {
            status = tag_edit_build_parser_preview(editor, true, &success);
            if (status < 0) {
                return -NCM_ERROR_UNAVAILABLE;
            }
            if (success) {
                StrBuilderArray replacement = {0};
                StrBuilder first = {0};
                int32 existing;

                if (editor->pattern.len <= 0) {
                    return -NCM_ERROR_UNAVAILABLE;
                }

                sb_set(&first, editor->pattern.data, editor->pattern.len);
                str_builder_array_append_copy(&replacement, &first);
                sb_free(&first);
                existing = tag_edit_find_recent_pattern(editor,
                                                        editor->pattern.data,
                                                        editor->pattern.len);
                for (int32 i = 0; i < editor->recent_patterns.len; i += 1) {
                    StrBuilder *pattern;
                    if (i == existing) {
                        continue;
                    }

                    pattern = &editor->recent_patterns.items[i];
                    str_builder_array_append_copy(&replacement, pattern);
                }
                str_builder_array_move(&editor->recent_patterns,
                                       &replacement);
                str_builder_array_destroy(&replacement);
                tag_edit_screen_prepare_parser_rows(editor,
                                                    editor->parser_mode,
                                                    editor->pattern.data,
                                                    editor->pattern.len);
                tag_edit_save_recent_patterns(editor);
                tag_edit_status_message(editor, STRLIT("Operation finished"));
                tag_edit_screen_close_parser(editor);
                return 0;
            }
            return -NCM_ERROR_UNAVAILABLE;
        }
        if (choice == TAG_EDIT_PARSER_ACTION_CANCEL) {
            tag_edit_save_recent_patterns(editor);
            tag_edit_screen_close_parser(editor);
            return 0;
        }
        if (choice >= (int32)TAG_EDIT_PARSER_ACTION_RECENT_START) {
            StrBuilder *row;

            if ((row = nc_menu_active_item_at(menu, choice))) {
                tag_edit_set_pattern(editor, row->data, row->len);
                tag_edit_screen_prepare_parser_rows(editor, editor->parser_mode,
                                                    editor->pattern.data,
                                                    editor->pattern.len);
                tag_edit_set_focus(editor, TAG_EDIT_FOCUS_PARSER_ACTIONS);
                nc_menu_goto_selectable(menu, TAG_EDIT_PARSER_ACTION_PATTERN);
                return 0;
            }
        }
        return -NCM_ERROR_UNAVAILABLE;
    }
    case TAG_EDIT_FOCUS_PARSER_LEGEND:
    case TAG_EDIT_FOCUS_PARSER_PREVIEW:
    case TAG_EDIT_FOCUS_COUNT:
    default:
        return -NCM_ERROR_UNAVAILABLE;
    }
}

static void
tag_edit_switch_to(NcScreen *screen) {
    nc_screen_switcher_finish_switch(screen);
    ncm_title_draw_header(STRLIT("Tag editor"));
    tag_edit_refresh(screen);
    return;
}

static void
tag_edit_resize(NcScreen *screen) {
    TagEditScreen *editor = tag_edit_from_screen(screen);
    int32 start_x;
    int32 width;

    nc_screen_switcher_get_resize_params(screen, &start_x, &width, true);
    tag_edit_screen_set_geometry(editor, start_x, width,
                                 ui_state_main_start_y(),
                                 ui_state_main_height());
    nc_screen_clear_resize_request(screen);
    return;
}

static char *
tag_edit_title(NcScreen *screen) {
    (void)screen;
    return "Tag editor";
}

static bool
tag_edit_current_directory_path(TagEditScreen *screen,
                                  char **path, int32 *path_len) {
    StrBuilderPair *pair;

    ASSERT(screen != NULL);
    ASSERT(path != NULL);
    ASSERT(path_len != NULL);

    *path = NULL;
    *path_len = 0;
    pair = nc_editor_pair_menu_current(&screen->directories);
    if (pair == NULL) {
        return false;
    }
    ASSERT(pair->second.data != NULL);
    *path = pair->second.data;
    *path_len = pair->second.len;
    return true;
}

static void
tag_edit_observe_current_directory(TagEditScreen *screen) {
    char *path;
    int32 path_len;
    NcMenu *menu;

    ASSERT(screen != NULL);
    menu = nc_editor_pair_menu_base(&screen->directories);
    screen->last_directory_highlight = nc_menu_highlight(menu);
    if (!tag_edit_current_directory_path(screen, &path, &path_len)) {
        sb_clear(&screen->observed_dir);
        screen->observed_dir_valid = false;
        return;
    }
    sb_set(&screen->observed_dir, path, path_len);
    screen->observed_dir_valid = true;
    return;
}

static void
tag_edit_restore_current_directory(TagEditScreen *screen, StrBuilder *path) {
    NcMenu *menu;

    ASSERT(screen != NULL);
    ASSERT(path != NULL);
    if (path->len <= 0) {
        return;
    }
    menu = nc_editor_pair_menu_base(&screen->directories);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        StrBuilderPair *pair = nc_menu_active_item_at(menu, i);

        ASSERT(pair != NULL);
        ASSERT(pair->second.data != NULL);

        if (STREQUAL(pair->second.data, pair->second.len,
                     path->data, path->len)) {
            nc_menu_goto_selectable(menu, i);
            return;
        }
    }
    return;
}

static int32
tag_edit_reload_directories_from_mpd(TagEditScreen *screen,
                                       MpdClient *client,
                                       NcmError *ncm_error) {
    NcmDirectoryArray directories = {0};
    StrBuilder preserved = {0};
    char *dir;
    int32 status;

    {
        char *data;
        int32 data_len;

        if (tag_edit_current_directory_path(screen, &data, &data_len)) {
            sb_set(&preserved, data, data_len);
        }
    }
    if ((preserved.len <= 0) && (screen->highlighted_dir.len > 0)) {
        sb_set(&preserved,
               screen->highlighted_dir.data, screen->highlighted_dir.len);
    }
    dir = screen->current_dir.data;
    if (dir == NULL) {
        dir = "/";
    }

    status = ncm_mpd_client_get_directory_list(client, dir, &directories,
                                               ncm_error);
    if (status < 0) {
        sb_free(&preserved);
        ncm_directory_array_destroy(&directories);
        return status;
    }

    for (int32 i = 1; i < directories.len; i += 1) {
        NcmDirectory current = {0};
        int32 j = i;

        ncm_directory_move(&current, &directories.items[i]);
        while (j > 0) {
            NcmDirectory *left = &directories.items[j - 1];
            int32 comparison;

            if (left->path == NULL) {
                comparison = current.path == NULL ? 0 : -1;
            } else if (current.path == NULL) {
                comparison = 1;
            } else {
                int32 left_start;
                int32 right_start;

                left_start = ncm_path_basename_start(left->path,
                                                     left->path_len);
                right_start = ncm_path_basename_start(current.path,
                                                      current.path_len);
                comparison =
                    ncm_compare_locale_strings(left->path + left_start,
                                               left->path_len - left_start,
                                               current.path + right_start,
                                               current.path_len - right_start,
                                               Config.ignore_leading_the);
            }
            if (comparison <= 0) {
                break;
            }
            ncm_directory_move(&directories.items[j],
                               &directories.items[j - 1]);
            j -= 1;
        }
        ncm_directory_move(&directories.items[j], &current);
        ncm_directory_destroy(&current);
    }

    nc_menu_show_all_items(nc_editor_pair_menu_base(&screen->directories));
    nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
    {
        char *control_dir = screen->current_dir.data;
        int32 control_dir_len = screen->current_dir.len;

        if ((control_dir == NULL) || (control_dir_len <= 0)
            || STREQUAL(control_dir, control_dir_len, "/")) {
            tag_edit_screen_add_directory(screen, STRLIT("."), STRLIT("/"));
        } else {
            int32 parent_len;

            parent_len = ncm_string_parent_directory_len(control_dir,
                                                         control_dir_len);
            if (parent_len <= 0) {
                tag_edit_screen_add_directory(screen,
                                              STRLIT(".."), STRLIT("/"));
            } else {
                tag_edit_screen_add_directory(screen,
                                              STRLIT(".."),
                                              control_dir, parent_len);
            }
        }
    }
    for (int32 i = 0; i < directories.len; i += 1) {
        NcmDirectory *directory = &directories.items[i];
        StringView path;
        int32 basename_start;

        if (!ncm_directory_has_path_view(directory, &path)) {
            continue;
        }
        basename_start = ncm_path_basename_start(path.data, path.len);
        tag_edit_screen_add_directory(screen,
                                      path.data + basename_start,
                                      path.len - basename_start,
                                      path.data, path.len);
    }

    tag_edit_restore_current_directory(screen, &preserved);
    if (screen->directory_filter_enabled) {
        nc_menu_apply_filter(nc_editor_pair_menu_base(&screen->directories));
        tag_edit_restore_current_directory(screen, &preserved);
    }
    tag_edit_observe_current_directory(screen);
    sb_clear(&screen->highlighted_dir);
    screen->directories_update_requested = false;

    sb_free(&preserved);
    ncm_directory_array_destroy(&directories);
    return 0;
}

static int32
tag_edit_reload_songs_from_mpd(TagEditScreen *screen,
                                 MpdClient *client, NcmError *ncm_error) {
    NcmMpdSongList list = {0};
    NcmSongArray songs = {0};
    StrBuilder preserved_uri = {0};
    char *path;
    int32 path_len;
    int32 status;

    if (!tag_edit_current_directory_path(screen, &path, &path_len)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing directory"));
    }

    {
        MutableSong *current;

        if ((current = nc_tag_row_menu_current(&screen->tags))) {
            if (current->uri && (current->uri_len > 0)) {
                sb_set(&preserved_uri, current->uri, current->uri_len);
            }
        }
    }

    status = ncm_mpd_client_get_songs(client, path, &list, ncm_error);
    if (status < 0) {
        sb_free(&preserved_uri);
        ncm_song_array_destroy(&songs);
        ncm_mpd_song_list_destroy(&list);
        return status;
    }

    ncm_mpd_song_list_to_song_array(&list, &songs);

    for (int32 i = 1; i < songs.len; i += 1) {
        NcmSong current = {0};
        int32 j = i;

        ncm_song_move(&current, &songs.items[i]);
        while (j > 0) {
            NcmSong *left = &songs.items[j - 1];
            StringView left_uri;
            StringView right_uri;
            int32 comparison;

            if (!ncm_song_has_uri_view(left, 0, &left_uri)) {
                comparison =
                    ncm_song_has_uri_view(&current, 0, &right_uri) ? -1 : 0;
            } else if (!ncm_song_has_uri_view(&current, 0, &right_uri)) {
                comparison = 1;
            } else {
                comparison =
                    ncm_compare_locale_strings(left_uri.data, left_uri.len,
                                               right_uri.data, right_uri.len,
                                               Config.ignore_leading_the);
            }
            if (comparison <= 0) {
                break;
            }
            ncm_song_move(&songs.items[j], &songs.items[j - 1]);
            j -= 1;
        }
        ncm_song_move(&songs.items[j], &current);
        ncm_song_destroy(&current);
    }

    tag_edit_screen_load_songs(screen, &songs);

    if (screen->tag_filter_enabled) {
        nc_menu_apply_filter(nc_tag_row_menu_base(&screen->tags));
    }
    if (preserved_uri.len > 0) {
        NcMenu *menu = nc_tag_row_menu_base(&screen->tags);

        for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
            MutableSong *item = nc_menu_active_item_at(menu, i);

            if (item->uri == NULL) {
                continue;
            }

            if (STREQUAL(item->uri, item->uri_len,
                         preserved_uri.data, preserved_uri.len)) {
                nc_menu_goto_selectable(menu, i);
                break;
            }
        }
    }
    screen->tags_update_requested = false;
    tag_edit_update_titles(screen, true);

    sb_free(&preserved_uri);
    ncm_song_array_destroy(&songs);
    ncm_mpd_song_list_destroy(&list);
    return 0;
}

static void
tag_edit_report_error(char *context, int32 context_len, NcmError *ncm_error) {
    StrBuilder message = {0};

    SB_APPEND(&message, context, context_len);
    if (ncm_error && (ncm_error->message[0] != 0)) {
        SB_APPEND(&message, ": ");
        SB_APPEND(&message, ncm_error->message, ncm_error->message_len);
    }
    ncm_statusbar_print(Config.message_delay_time, message.data, message.len);
    sb_free(&message);
    return;
}

static void
tag_edit_update(NcScreen *screen) {
    TagEditScreen *editor = tag_edit_from_screen(screen);
    NcmError ncm_error;
    int32 status;
    bool changed = false;
    bool continue_update = true;

    tag_edit_screen_finish_directory_change(editor);
    ncm_error_clear(&ncm_error);
    if (editor->directories_update_requested
        || (nc_menu_item_count(nc_editor_pair_menu_base(&editor->directories))
            <= 0)) {
        status = tag_edit_reload_directories_from_mpd(editor, &global_mpd,
                                                      &ncm_error);
        if (status < 0) {
            editor->directories_update_requested = false;
            tag_edit_report_error(STRLIT("Could not fetch directories"),
                                  &ncm_error);
            ncm_error_clear(&ncm_error);
            tag_edit_update_titles(editor, true);
            continue_update = false;
        } else {
            changed = true;
        }
    }

    if (continue_update) {
        tag_edit_screen_finish_directory_change(editor);
        if (!editor->tags_update_requested
            && (nc_menu_item_count(nc_tag_row_menu_base(&editor->tags)) > 0)) {
            tag_edit_update_titles(editor, true);
            continue_update = false;
        }
    }

    if (continue_update) {
        ncm_error_clear(&ncm_error);
        status = tag_edit_reload_songs_from_mpd(editor, &global_mpd,
                                                &ncm_error);
        if (status < 0) {
            editor->tags_update_requested = false;
            tag_edit_report_error(STRLIT("Could not fetch songs"), &ncm_error);
            ncm_error_clear(&ncm_error);
            tag_edit_update_titles(editor, true);
        } else {
            changed = true;
            tag_edit_update_titles(editor, true);
        }
    }

    nc_screen_clear_update_request(screen);
    if (changed && app_controller_is_screen_visible(screen)) {
        nc_screen_refresh(screen);
    }
    return;
}

static enum TagEditFocus
tag_edit_current_helper_focus(TagEditScreen *screen) {
    ASSERT(screen != NULL);
    if (!screen->parser_preview_enabled) {
        return TAG_EDIT_FOCUS_PARSER_LEGEND;
    }
    return TAG_EDIT_FOCUS_PARSER_PREVIEW;
}

static bool
tag_edit_mouse_move_to_parser_focus(TagEditScreen *screen,
                                      enum TagEditFocus focus) {
    ASSERT(screen != NULL);
    if (focus == TAG_EDIT_FOCUS_PARSER_CHOICE) {
        tag_edit_set_focus(screen, focus);
        return true;
    }
    if (screen->parser_mode == TAG_EDIT_PARSER_MODE_NONE) {
        return false;
    }
    if ((focus == TAG_EDIT_FOCUS_PARSER_ACTIONS)
        || tag_edit_focus_is_parser_helper(focus)) {
        tag_edit_set_focus(screen, focus);
        return true;
    }
    return false;
}

static bool
tag_edit_focus_is_main(enum TagEditFocus focus) {
    return (focus == TAG_EDIT_FOCUS_DIRECTORIES)
           || (focus == TAG_EDIT_FOCUS_TAG_TYPES)
           || (focus == TAG_EDIT_FOCUS_TAGS);
}

static bool
tag_edit_mouse_move_to_column(TagEditScreen *screen,
                                enum TagEditColumn column) {
    ASSERT(screen != NULL);
    if (!tag_edit_focus_is_main(screen->active_focus)) {
        return false;
    }
    if (((screen->active_focus == TAG_EDIT_FOCUS_DIRECTORIES)
         && (column == TAG_EDIT_COLUMN_DIRECTORIES))
        || ((screen->active_focus == TAG_EDIT_FOCUS_TAG_TYPES)
            && (column == TAG_EDIT_COLUMN_TAG_TYPES))
        || ((screen->active_focus == TAG_EDIT_FOCUS_TAGS)
            && (column == TAG_EDIT_COLUMN_TAGS))) {
        return true;
    }
    while (screen->active_column < column) {
        if (!tag_edit_screen_next_column_available(screen)) {
            return false;
        }
        tag_edit_screen_next_column(screen);
    }
    while (screen->active_column > column) {
        if (!tag_edit_screen_previous_column_available(screen)) {
            return false;
        }
        tag_edit_screen_previous_column(screen);
    }
    tag_edit_update_menu_highlights(screen);
    return true;
}

static void
tag_edit_mouse_scroll_menu(NcMenu *menu, NcWindow *window,
                           enum NcScroll where) {
    enum NcScroll effective = where;
    int32 count = Config.lines_scrolled;

    ASSERT(menu != NULL);
    ASSERT(window != NULL);

    if (Config.mouse_list_scroll_whole_page) {
        count = 1;
        if (where == NC_SCROLL_DOWN) {
            effective = NC_SCROLL_PAGE_DOWN;
        } else if (where == NC_SCROLL_UP) {
            effective = NC_SCROLL_PAGE_UP;
        }
    }
    if (count < 1) {
        count = 1;
    }
    for (int32 i = 0; i < count; i += 1) {
        nc_menu_scroll_selectable(menu, nc_window_height(window), effective);
    }
    return;
}

static void
tag_edit_mouse_scroll(TagEditScreen *screen, enum NcScroll where) {
    NcMenu *menu;
    NcWindow *window;

    ASSERT(screen != NULL);

    menu = tag_edit_screen_active_menu(screen);
    window = tag_edit_screen_active_window(screen);
    tag_edit_mouse_scroll_menu(menu, window, where);
    tag_edit_screen_finish_directory_change(screen);
    tag_edit_finish_tag_type_change(screen, true);
    return;
}

static int32
tag_edit_run_current_action(TagEditScreen *screen) {
    ASSERT(screen != NULL);
    return nc_screen_run_current(tag_edit_screen_base(screen));
}

static void
tag_edit_mouse_callback(NcScreen *screen, MEVENT event) {
    TagEditScreen *editor = tag_edit_from_screen(screen);
    int32 x;
    int32 y;

    if (!tag_edit_focus_is_main(editor->active_focus)) {
        x = event.x;
        y = event.y;
        if (nc_window_has_coords(&editor->parser_dialog_window, &x, &y)) {
            enum TagEditFocus focus = TAG_EDIT_FOCUS_PARSER_CHOICE;
            NcMenu *menu = nc_editor_string_menu_base(&editor->parser_dialog);

            if (!tag_edit_mouse_move_to_parser_focus(editor, focus)) {
                return;
            }
            if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
                if ((y >= 0) && (y < nc_menu_item_count(menu))
                    && (nc_menu_goto_selectable(menu, y) >= 0)
                    && (event.bstate & BUTTON3_PRESSED)) {
                    tag_edit_run_current_action(editor);
                }
            } else if (event.bstate & BUTTON5_PRESSED) {
                tag_edit_mouse_scroll_menu(menu, &editor->parser_dialog_window,
                                           NC_SCROLL_DOWN);
            } else if (event.bstate & BUTTON4_PRESSED) {
                tag_edit_mouse_scroll_menu(menu, &editor->parser_dialog_window,
                                           NC_SCROLL_UP);
            }
            nc_screen_refresh(screen);
            return;
        }

        x = event.x;
        y = event.y;
        if (nc_window_has_coords(&editor->parser_window, &x, &y)) {
            enum TagEditFocus focus = TAG_EDIT_FOCUS_PARSER_ACTIONS;
            NcMenu *menu;

            menu = nc_editor_string_menu_base(&editor->parser_actions);
            if (!tag_edit_mouse_move_to_parser_focus(editor, focus)) {
                return;
            }
            if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
                if ((y >= 0) && (y < nc_menu_item_count(menu))
                    && (nc_menu_goto_selectable(menu, y) >= 0)
                    && (event.bstate & BUTTON3_PRESSED)) {
                    tag_edit_run_current_action(editor);
                }
            } else if (event.bstate & BUTTON5_PRESSED) {
                tag_edit_mouse_scroll_menu(menu, &editor->parser_window,
                                           NC_SCROLL_DOWN);
            } else if (event.bstate & BUTTON4_PRESSED) {
                tag_edit_mouse_scroll_menu(menu, &editor->parser_window,
                                           NC_SCROLL_UP);
            }
            nc_screen_refresh(screen);
            return;
        }

        x = event.x;
        y = event.y;
        if (nc_window_has_coords(&editor->parser_helper_window, &x, &y)) {
            enum TagEditFocus focus;

            focus = tag_edit_current_helper_focus(editor);
            if (!tag_edit_mouse_move_to_parser_focus(editor, focus)) {
                return;
            }
            if (event.bstate & BUTTON5_PRESSED) {
                nc_scrollpad_scroll(&editor->parser_helper_scrollpad,
                                    &editor->parser_helper_window,
                                    NC_SCROLL_DOWN);
                tag_edit_refresh_active_helper(editor);
            } else if (event.bstate & BUTTON4_PRESSED) {
                nc_scrollpad_scroll(&editor->parser_helper_scrollpad,
                                    &editor->parser_helper_window,
                                    NC_SCROLL_UP);
                tag_edit_refresh_active_helper(editor);
            }
            return;
        }
        return;
    }

    x = event.x;
    y = event.y;
    if (nc_window_has_coords(&editor->directories_window, &x, &y)) {
        if (!tag_edit_mouse_move_to_column(editor,
                                           TAG_EDIT_COLUMN_DIRECTORIES)) {
            return;
        }
        if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
            NcMenu *menu = nc_editor_pair_menu_base(&editor->directories);

            if ((y >= 0) && (y < nc_menu_item_count(menu))
                && (nc_menu_goto_selectable(menu, y) >= 0)) {
                tag_edit_screen_finish_directory_change(editor);
                if (event.bstate & BUTTON1_PRESSED) {
                    tag_edit_screen_enter_directory(editor);
                }
            }
        } else if (event.bstate & BUTTON5_PRESSED) {
            tag_edit_mouse_scroll(editor, NC_SCROLL_DOWN);
        } else if (event.bstate & BUTTON4_PRESSED) {
            tag_edit_mouse_scroll(editor, NC_SCROLL_UP);
        }
        tag_edit_screen_finish_directory_change(editor);
        nc_screen_refresh(screen);
        return;
    }

    x = event.x;
    y = event.y;
    if (nc_window_has_coords(&editor->tag_types_window, &x, &y)) {
        if (!tag_edit_mouse_move_to_column(editor, TAG_EDIT_COLUMN_TAG_TYPES)) {
            return;
        }
        if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
            NcMenu *menu = nc_editor_string_menu_base(&editor->tag_types);

            if ((y >= 0) && (y < nc_menu_item_count(menu))
                && (nc_menu_goto_selectable(menu, y) >= 0)) {
                tag_edit_finish_tag_type_change(editor, true);
                if (event.bstate & BUTTON3_PRESSED) {
                    tag_edit_run_current_action(editor);
                }
            }
        } else if (event.bstate & BUTTON5_PRESSED) {
            tag_edit_mouse_scroll(editor, NC_SCROLL_DOWN);
        } else if (event.bstate & BUTTON4_PRESSED) {
            tag_edit_mouse_scroll(editor, NC_SCROLL_UP);
        }
        tag_edit_finish_tag_type_change(editor, true);
        nc_screen_refresh(screen);
        return;
    }

    x = event.x;
    y = event.y;
    if (nc_window_has_coords(&editor->tags_window, &x, &y)) {
        if (!tag_edit_mouse_move_to_column(editor, TAG_EDIT_COLUMN_TAGS)) {
            return;
        }
        if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
            NcMenu *menu = nc_tag_row_menu_base(&editor->tags);

            if ((y >= 0) && (y < nc_menu_item_count(menu))
                && (nc_menu_goto_selectable(menu, y) >= 0)
                && (event.bstate & BUTTON3_PRESSED)) {
                tag_edit_run_current_action(editor);
            }
        } else if (event.bstate & BUTTON5_PRESSED) {
            tag_edit_mouse_scroll(editor, NC_SCROLL_DOWN);
        } else if (event.bstate & BUTTON4_PRESSED) {
            tag_edit_mouse_scroll(editor, NC_SCROLL_UP);
        }
        nc_screen_refresh(screen);
        return;
    }
    return;
}

static void
tag_edit_destroy_callback(NcScreen *screen) {
    tag_edit_screen_destroy(tag_edit_from_screen(screen));
    return;
}

static NcScreenOps tag_edit_callbacks = {
    .capabilities = NC_SCREEN_CAPABILITY_MENU
                    |NC_SCREEN_CAPABILITY_FILTER
                    |NC_SCREEN_CAPABILITY_SEARCH
                    |NC_SCREEN_CAPABILITY_SONGS
                    |NC_SCREEN_CAPABILITY_COLUMNS
                    |NC_SCREEN_CAPABILITY_TAGS,
    .active_window = tag_edit_active_window,
    .refresh = tag_edit_refresh,
    .refresh_window = tag_edit_refresh_window,
    .scroll = tag_edit_scroll,
    .can_run_current = tag_edit_can_run_current,
    .run_current = tag_edit_run_current,
    .switch_to = tag_edit_switch_to,
    .resize = tag_edit_resize,
    .title = tag_edit_title,
    .update = tag_edit_update,
    .mouse_button_pressed = tag_edit_mouse_callback,
    .lockable = true,
    .mergable = true,
    .destroy = tag_edit_destroy_callback,
    .current_menu = tag_edit_menu_capability,
    .current_menu_height = tag_edit_menu_height_capability,
    .can_filter = tag_edit_filter_available_capability,
    .current_filter = tag_edit_filter_constraint_capability,
    .apply_filter = tag_edit_filter_apply_capability,
    .can_search = tag_edit_search_available_capability,
    .current_search_constraint = tag_edit_search_constraint_capability,
    .clear_search_constraint = tag_edit_search_clear_capability,
    .search = tag_edit_search_capability,
    .selected_songs = tag_edit_selected_songs_capability,
    .previous_column_available = tag_edit_previous_column_available_capability,
    .next_column_available = tag_edit_next_column_available_capability,
    .previous_column = tag_edit_previous_column_capability,
    .next_column = tag_edit_next_column_capability,
    .tag_menu = tag_edit_tag_menu_capability,
    .song_tag_at = tag_edit_tag_at_capability,
};

typedef struct TagEditSearchContext {
    TagEditScreen *screen;
    NcmRegex *regex;
} TagEditSearchContext;

typedef struct TagSetter {
    enum TagType tag_type;
    char *value;
    char *separator;
    int32 value_len;
    int32 separator_len;
} TagSetter;

typedef struct TrackNumberer {
    int32 current;
    int32 total;
    bool extended;
} TrackNumberer;

struct SaveContext {
    TagEditScreen *screen;
    StrBuilder shared_directory;
    char *music_dir;
    int32 target_count;
    int32 modified_count;
    int32 write_count;
    bool shared_directory_valid;
};

static void
tag_edit_append_string_row(NcEditorStringMenu *menu,
                             char *data, int32 data_len, uint32 flags) {
    StrBuilder string = {0};

    sb_set(&string, data, data_len);
    nc_editor_string_menu_add_with_flags(menu, &string, flags);
    sb_free(&string);
    return;
}

static void
tag_edit_configure_menu(NcMenu *menu) {
    ASSERT(menu != NULL);
    nc_menu_set_selected_prefix(menu, &Config.selected_item_prefix);
    nc_menu_set_selected_suffix(menu, &Config.selected_item_suffix);
    nc_menu_set_cyclic_scrolling(menu, Config.cyclic_scrolling);
    nc_menu_set_centered_cursor(menu, Config.centered_cursor);
    return;
}

static void
tag_edit_draw_directory(NcMenu *menu, NcWindow *window, void *item,
                          int32 pos, void *user) {
    StrBuilderPair *pair = item;

    (void)menu;
    (void)pos;
    (void)user;

    ASSERT(window != NULL);
    ASSERT(pair != NULL);
    ASSERT(pair->first.data != NULL);

    nc_window_print_data(window, pair->first.data, pair->first.len);
    return;
}

static bool
tag_edit_directory_matches_regex(StrBuilderPair *pair,
                                   NcmRegex *regex, bool filter) {
    ASSERT(pair != NULL);
    ASSERT(pair->first.data != NULL);
    if (STREQUAL(pair->first.data, pair->first.len, ".")) {
        return filter;
    }
    if (STREQUAL(pair->first.data, pair->first.len, "..")) {
        return filter;
    }
    return ncm_regex_matches(regex, pair->first.data, pair->first.len);
}

static bool
tag_edit_directory_filter(NcMenu *menu, void *item, void *user) {
    TagEditScreen *screen = user;
    StrBuilderPair *pair = item;

    (void)menu;
    if (!screen->directory_filter_enabled) {
        return true;
    }
    return tag_edit_directory_matches_regex(pair,
                                            &screen->directory_filter_regex,
                                            true);
}

static NcMenuDisplayCallbacks
tag_edit_directory_display_callbacks(TagEditScreen *screen) {
    NcMenuDisplayCallbacks callbacks = {0};

    callbacks.draw = tag_edit_draw_directory;
    callbacks.matches_filter = tag_edit_directory_filter;
    callbacks.user = screen;

    return callbacks;
}

static void
tag_edit_draw_string(NcMenu *menu, NcWindow *window, void *item,
                       int32 pos, void *user) {
    StrBuilder *string = item;

    (void)menu;
    (void)pos;
    (void)user;

    ASSERT(window != NULL);
    ASSERT(string != NULL);
    ASSERT(string->data != NULL);

    nc_window_print_data(window, string->data, string->len);
    return;
}

static NcMenuDisplayCallbacks
tag_edit_tag_type_display_callbacks(TagEditScreen *screen) {
    NcMenuDisplayCallbacks callbacks = {0};

    callbacks.draw = tag_edit_draw_string;
    callbacks.user = screen;
    return callbacks;
}

static bool
tag_edit_tag_matches_regex(TagEditScreen *screen,
                             MutableSong *song, NcmRegex *regex) {
    StrBuilder buffer = {0};
    NcMenu *tag_types;
    enum TagType tag_type;
    int32 choice;
    bool found;

    ASSERT(screen != NULL);
    ASSERT(song != NULL);
    ASSERT(regex != NULL);

    tag_types = nc_editor_string_menu_base(&screen->tag_types);
    choice = nc_menu_highlight(tag_types);
    if (choice < (int32)TAG_COUNT) {
        tag_type = ncm_song_info_tags[choice].tag;
    } else if (tag_edit_choice_is_filename(choice)) {
        tag_type = TAG_COUNT;
    } else {
        return false;
    }

    tag_edit_song_display_value(song, tag_type, &buffer);
    if (buffer.len <= 0) {
        SB_APPEND(&buffer,
                  Config.empty_tag_marker, Config.empty_tag_marker_len);
    }
    found = ncm_regex_matches(regex, buffer.data, buffer.len);
    sb_free(&buffer);
    return found;
}

static bool
tag_edit_tag_filter(NcMenu *menu, void *item, void *user) {
    TagEditScreen *screen = user;
    MutableSong *song = item;

    (void)menu;
    if (!screen->tag_filter_enabled) {
        return true;
    }
    return tag_edit_tag_matches_regex(screen, song, &screen->tag_filter_regex);
}

static NcMenuDisplayCallbacks
tag_edit_tag_display_callbacks(TagEditScreen *screen) {
    NcMenuDisplayCallbacks callbacks = {0};

    callbacks.draw = tag_edit_draw_tag;
    callbacks.matches_filter = tag_edit_tag_filter;
    callbacks.user = screen;

    return callbacks;
}

static void
tag_edit_configure_menus(TagEditScreen *screen) {
    NcMenu *directories = nc_editor_pair_menu_base(&screen->directories);
    NcMenu *tag_types = nc_editor_string_menu_base(&screen->tag_types);
    NcMenu *tags = nc_tag_row_menu_base(&screen->tags);
    NcMenu *parser_dialog = nc_editor_string_menu_base(&screen->parser_dialog);
    NcMenu *parser_rows = nc_editor_string_menu_base(&screen->parser_rows);
    NcMenu *parser_actions =
        nc_editor_string_menu_base(&screen->parser_actions);

    tag_edit_configure_menu(directories);
    tag_edit_configure_menu(tag_types);
    tag_edit_configure_menu(tags);
    tag_edit_configure_menu(parser_dialog);
    tag_edit_configure_menu(parser_rows);
    tag_edit_configure_menu(parser_actions);

    nc_menu_set_display_callbacks(directories,
                                  tag_edit_directory_display_callbacks(screen));
    nc_menu_set_display_callbacks(tag_types,
                                  tag_edit_tag_type_display_callbacks(screen));
    nc_menu_set_display_callbacks(tags, tag_edit_tag_display_callbacks(screen));
    nc_menu_set_display_callbacks(parser_dialog,
                                  tag_edit_tag_type_display_callbacks(screen));
    nc_menu_set_display_callbacks(parser_rows,
                                  tag_edit_tag_type_display_callbacks(screen));
    nc_menu_set_display_callbacks(parser_actions,
                                  tag_edit_tag_type_display_callbacks(screen));

    tag_edit_update_menu_highlights(screen);
    return;
}

static void
tag_edit_layout(TagEditScreen *screen) {
    int32 separator_width;
    int32 parser_dialog_x_space;
    int32 parser_dialog_y_space;
    int32 parser_x_space;
    int32 parser_y_space;
    int32 screen_height;

    if (screen->width < 1) {
        screen->width = 1;
    }
    if (screen->main_height < 1) {
        screen->main_height = 1;
    }

    separator_width = tag_edit_separator_width(screen);
    screen->middle_width = MIN(26, screen->width - 2*separator_width);
    if (screen->middle_width < 1) {
        screen->middle_width = 1;
    }
    screen->left_width = (screen->width - screen->middle_width)/2;
    if (screen->left_width < 1) {
        screen->left_width = 1;
    }
    if ((screen->left_width + screen->middle_width
         + 2*separator_width) > screen->width) {
        screen->left_width = screen->width - screen->middle_width
                             - 2*separator_width;
    }
    if (screen->left_width < 0) {
        screen->left_width = 0;
    }
    screen->middle_start_x = screen->start_x + screen->left_width
                             + separator_width;
    screen->right_start_x = screen->middle_start_x + screen->middle_width
                            + separator_width;
    screen->right_width = screen->width - screen->left_width
                          - screen->middle_width - 2*separator_width;
    if (screen->right_width < 1) {
        screen->right_width = 1;
    }

    screen->parser_dialog_width = MIN(30, screen->width);
    screen->parser_dialog_height = MIN(5, screen->main_height);
    if (screen->parser_dialog_width < 1) {
        screen->parser_dialog_width = 1;
    }
    if (screen->parser_dialog_height < 1) {
        screen->parser_dialog_height = 1;
    }

    screen->parser_width = screen->width*9/10;
    if (screen->parser_width < 1) {
        screen->parser_width = 1;
    }
    screen_height = ui_state_screen_height();
    screen->parser_height = MIN(screen_height*8/10, screen->main_height);
    if (screen->parser_height < 1) {
        screen->parser_height = 1;
    }
    screen->parser_width_one = screen->parser_width/2;
    if (screen->parser_width_one < 1) {
        screen->parser_width_one = 1;
    }
    screen->parser_width_two = screen->parser_width
                               - screen->parser_width_one;
    if (screen->parser_width_two < 1) {
        screen->parser_width_two = 1;
    }

    parser_dialog_x_space = screen->width - screen->parser_dialog_width;
    parser_dialog_y_space = screen->main_height
                            - screen->parser_dialog_height;
    parser_x_space = screen->width - screen->parser_width;
    parser_y_space = screen->main_height - screen->parser_height;
    if (parser_dialog_x_space < 0) {
        parser_dialog_x_space = 0;
    }
    if (parser_dialog_y_space < 0) {
        parser_dialog_y_space = 0;
    }
    if (parser_x_space < 0) {
        parser_x_space = 0;
    }
    if (parser_y_space < 0) {
        parser_y_space = 0;
    }

    screen->parser_dialog_start_x = screen->start_x
                                    + parser_dialog_x_space/2;
    screen->parser_dialog_start_y = screen->main_start_y
                                    + parser_dialog_y_space/2;
    screen->parser_start_x = screen->start_x + parser_x_space/2;
    screen->parser_start_y = screen->main_start_y + parser_y_space/2;
    screen->parser_helper_start_x = screen->parser_start_x
                                    + screen->parser_width_one;

    nc_window_move_to(&screen->directories_window,
                      screen->start_x, screen->main_start_y);
    nc_window_resize(&screen->directories_window,
                     screen->left_width, screen->main_height);
    nc_window_move_to(&screen->tag_types_window,
                      screen->middle_start_x, screen->main_start_y);
    nc_window_resize(&screen->tag_types_window,
                     screen->middle_width, screen->main_height);
    nc_window_move_to(&screen->tags_window,
                      screen->right_start_x, screen->main_start_y);
    nc_window_resize(&screen->tags_window,
                     screen->right_width, screen->main_height);

    nc_window_move_to(&screen->parser_dialog_window,
                      screen->parser_dialog_start_x,
                      screen->parser_dialog_start_y);
    nc_window_resize(&screen->parser_dialog_window, screen->parser_dialog_width,
                     screen->parser_dialog_height);
    nc_window_move_to(&screen->parser_window, screen->parser_start_x,
                      screen->parser_start_y);
    nc_window_resize(&screen->parser_window, screen->parser_width_one,
                     screen->parser_height);
    nc_window_move_to(&screen->parser_helper_window,
                      screen->parser_helper_start_x, screen->parser_start_y);
    nc_scrollpad_resize(&screen->parser_helper_scrollpad,
                        &screen->parser_helper_window,
                        screen->parser_width_two, screen->parser_height);
    return;
}

void
tag_edit_screen_init(TagEditScreen *screen, int32 start_x, int32 width,
                     int32 main_start_y, int32 main_height,
                     NcColor color, NcBorder border) {
    nc_editor_pair_menu_init(&screen->directories);
    nc_editor_string_menu_init(&screen->tag_types);
    nc_tag_row_menu_init(&screen->tags);

    nc_editor_string_menu_init(&screen->parser_dialog);
    nc_editor_string_menu_init(&screen->parser_rows);
    nc_editor_string_menu_init(&screen->parser_actions);

    screen->hooks = (TagEditHooks){0};
    screen->current_dir = (StrBuilder){0};
    screen->displayed_dir = (StrBuilder){0};
    screen->observed_dir = (StrBuilder){0};
    screen->highlighted_dir = (StrBuilder){0};
    screen->directories_title = (StrBuilder){0};
    screen->tag_types_title = (StrBuilder){0};
    screen->tags_title = (StrBuilder){0};
    screen->parser_dialog_title = (StrBuilder){0};
    screen->parser_title = (StrBuilder){0};
    screen->parser_helper_title = (StrBuilder){0};
    screen->parser_legend = (StrBuilder){0};
    screen->parser_preview = (StrBuilder){0};

    screen->recent_patterns = (StrBuilderArray){0};

    screen->directory_filter_constraint = (StrBuilder){0};
    screen->tag_filter_constraint = (StrBuilder){0};
    screen->directory_search_constraint = (StrBuilder){0};
    screen->tag_search_constraint = (StrBuilder){0};
    screen->pattern = (StrBuilder){0};
    screen->directory_filter_regex = (NcmRegex){0};
    screen->tag_filter_regex = (NcmRegex){0};
    screen->directory_search_regex = (NcmRegex){0};
    screen->tag_search_regex = (NcmRegex){0};

    tag_edit_update_titles(screen, false);

    nc_window_init(&screen->directories_window,
                   start_x, main_start_y, width, main_height,
                   screen->directories_title.data,
                   screen->directories_title.len, color, border);
    nc_window_init(&screen->tag_types_window,
                   start_x, main_start_y, width, main_height,
                   screen->tag_types_title.data, screen->tag_types_title.len,
                   color, border);
    nc_window_init(&screen->tags_window,
                   start_x, main_start_y, width, main_height,
                   screen->tags_title.data, screen->tags_title.len,
                   color, border);
    nc_window_init(&screen->parser_dialog_window,
                   start_x, main_start_y, width, main_height,
                   screen->parser_dialog_title.data,
                   screen->parser_dialog_title.len,
                   color, Config.window_border_color);
    nc_window_init(&screen->parser_window,
                   start_x, main_start_y, width, main_height,
                   screen->parser_title.data, screen->parser_title.len,
                   color, Config.window_border_color);
    nc_window_init(&screen->parser_helper_window,
                   start_x, main_start_y, width, main_height,
                   screen->parser_helper_title.data,
                   screen->parser_helper_title.len,
                   color, Config.window_border_color);
    nc_scrollpad_init(&screen->parser_helper_scrollpad,
                      nc_window_height(&screen->parser_helper_window));

    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    screen->active_column = TAG_EDIT_COLUMN_DIRECTORIES;
    screen->active_focus = TAG_EDIT_FOCUS_DIRECTORIES;
    screen->last_directory_highlight = -1;
    screen->last_tag_type_highlight = -1;
    screen->last_known_directory_count = 0;
    screen->last_known_tag_count = 0;
    screen->window_timeout_ms = -1;
    screen->parser_mode = TAG_EDIT_PARSER_MODE_NONE;
    screen->directories_update_requested = false;
    screen->tags_update_requested = false;
    screen->directory_filter_enabled = false;
    screen->tag_filter_enabled = false;
    screen->directory_search_enabled = false;
    screen->tag_search_enabled = false;
    screen->parser_preview_enabled = true;
    screen->recent_patterns_loaded = false;
    screen->displayed_dir_valid = false;
    screen->observed_dir_valid = false;
    screen->registered = false;

    tag_edit_screen_set_current_dir(screen, STRLIT("/"));
    {
        NcEditorStringMenu *menu = &screen->tag_types;

        nc_menu_clear_items(nc_editor_string_menu_base(menu));
        for (uint32 i = 0; i < TAG_COUNT; i += 1) {
            tag_edit_append_string_row(menu, ncm_song_info_tags[i].name,
                                       ncm_song_info_tags[i].name_len,
                                       NC_MENU_ITEM_SELECTABLE);
        }
        nc_editor_string_menu_add_separator(menu);
        {
            char *label;
            int32 label_len;

            label_len = SONG_GETTER_alias_len(SONG_GETTER_NAME, &label);
            tag_edit_append_string_row(menu, label, label_len,
                                       NC_MENU_ITEM_SELECTABLE);
        }
        nc_editor_string_menu_add_separator(menu);
        if (Config.titles_visibility) {
            tag_edit_append_string_row(menu, STRLIT("Options"),
                                       NC_MENU_ITEM_INACTIVE);
            nc_editor_string_menu_add_separator(menu);
        }
        tag_edit_append_string_row(menu, STRLIT("Capitalize First Letters"),
                                   NC_MENU_ITEM_SELECTABLE);
        tag_edit_append_string_row(menu, STRLIT("lower all letters"),
                                   NC_MENU_ITEM_SELECTABLE);
        nc_editor_string_menu_add_separator(menu);
        tag_edit_append_string_row(menu, STRLIT("Reset"),
                                   NC_MENU_ITEM_SELECTABLE);
        tag_edit_append_string_row(menu, STRLIT("Save"),
                                   NC_MENU_ITEM_SELECTABLE);
    }
    tag_edit_layout(screen);
    tag_edit_configure_menus(screen);
    tag_edit_observe_current_directory(screen);
    nc_screen_init_ops(&screen->screen, tag_edit_callbacks, screen,
                       NC_SCREEN_TYPE_TAG_EDIT);
    tag_edit_screen_prepare_parser_rows(screen, TAG_EDIT_PARSER_MODE_NONE,
                                        NULL, 0);
    return;
}

void
tag_edit_screen_destroy(TagEditScreen *screen) {
    app_controller_unregister_screen(tag_edit_screen_base(screen));
    ncm_regex_destroy(&screen->tag_search_regex);
    ncm_regex_destroy(&screen->directory_search_regex);
    ncm_regex_destroy(&screen->tag_filter_regex);
    ncm_regex_destroy(&screen->directory_filter_regex);

    sb_free(&screen->pattern);
    sb_free(&screen->tag_search_constraint);
    sb_free(&screen->directory_search_constraint);
    sb_free(&screen->tag_filter_constraint);
    sb_free(&screen->directory_filter_constraint);

    str_builder_array_destroy(&screen->recent_patterns);

    sb_free(&screen->parser_preview);
    sb_free(&screen->parser_legend);
    sb_free(&screen->parser_helper_title);
    sb_free(&screen->parser_title);
    sb_free(&screen->parser_dialog_title);
    sb_free(&screen->tags_title);
    sb_free(&screen->tag_types_title);
    sb_free(&screen->directories_title);
    sb_free(&screen->highlighted_dir);
    sb_free(&screen->observed_dir);
    sb_free(&screen->displayed_dir);
    sb_free(&screen->current_dir);

    nc_window_destroy(&screen->parser_helper_window);
    nc_window_destroy(&screen->parser_window);
    nc_window_destroy(&screen->parser_dialog_window);
    nc_window_destroy(&screen->tags_window);
    nc_window_destroy(&screen->tag_types_window);
    nc_window_destroy(&screen->directories_window);

    nc_editor_string_menu_destroy(&screen->parser_actions);
    nc_editor_string_menu_destroy(&screen->parser_rows);
    nc_editor_string_menu_destroy(&screen->parser_dialog);
    nc_tag_row_menu_destroy(&screen->tags);
    nc_editor_string_menu_destroy(&screen->tag_types);
    nc_editor_pair_menu_destroy(&screen->directories);

    screen->registered = false;
    return;
}

NcScreen *
tag_edit_screen_base(TagEditScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->screen;
}

void
tag_edit_screen_set_hooks(TagEditScreen *screen, TagEditHooks hooks) {
    if (screen == NULL) {
        return;
    }
    screen->hooks = hooks;
    return;
}

NcMenu *
tag_edit_screen_active_menu(TagEditScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    switch (screen->active_focus) {
    case TAG_EDIT_FOCUS_DIRECTORIES:
        return nc_editor_pair_menu_base(&screen->directories);
    case TAG_EDIT_FOCUS_TAG_TYPES:
        return nc_editor_string_menu_base(&screen->tag_types);
    case TAG_EDIT_FOCUS_TAGS:
        return nc_tag_row_menu_base(&screen->tags);
    case TAG_EDIT_FOCUS_PARSER_CHOICE:
        return nc_editor_string_menu_base(&screen->parser_dialog);
    case TAG_EDIT_FOCUS_PARSER_ACTIONS:
        return nc_editor_string_menu_base(&screen->parser_actions);
    case TAG_EDIT_FOCUS_PARSER_LEGEND:
    case TAG_EDIT_FOCUS_PARSER_PREVIEW:
        return NULL;
    case TAG_EDIT_FOCUS_COUNT:
    default:
        break;
    }
    return NULL;
}

NcWindow *
tag_edit_screen_active_window(TagEditScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    switch (screen->active_focus) {
    case TAG_EDIT_FOCUS_DIRECTORIES:
        return &screen->directories_window;
    case TAG_EDIT_FOCUS_TAG_TYPES:
        return &screen->tag_types_window;
    case TAG_EDIT_FOCUS_TAGS:
        return &screen->tags_window;
    case TAG_EDIT_FOCUS_PARSER_CHOICE:
        return &screen->parser_dialog_window;
    case TAG_EDIT_FOCUS_PARSER_ACTIONS:
        return &screen->parser_window;
    case TAG_EDIT_FOCUS_PARSER_LEGEND:
    case TAG_EDIT_FOCUS_PARSER_PREVIEW:
        return &screen->parser_helper_window;
    case TAG_EDIT_FOCUS_COUNT:
    default:
        break;
    }
    return NULL;
}

void
tag_edit_screen_set_geometry(TagEditScreen *screen,
                             int32 start_x, int32 width,
                             int32 main_start_y, int32 main_height) {
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    tag_edit_layout(screen);
    tag_edit_configure_menus(screen);
    tag_edit_update_titles(screen, true);
    return;
}

void
tag_edit_screen_clear_directories(TagEditScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
    return;
}

void
tag_edit_screen_clear_stale_tags(TagEditScreen *screen) {
    nc_menu_clear_items(nc_tag_row_menu_base(&screen->tags));
    sb_clear(&screen->displayed_dir);
    screen->displayed_dir_valid = false;
    screen->tags_update_requested = true;
    screen->last_known_tag_count = 0;
    tag_edit_update_titles(screen, true);
    return;
}

void
tag_edit_screen_finish_directory_change(TagEditScreen *screen) {
    char *path;
    int32 path_len;
    NcMenu *menu;
    bool changed;

    if (screen->active_focus != TAG_EDIT_FOCUS_DIRECTORIES) {
        return;
    }

    menu = nc_editor_pair_menu_base(&screen->directories);
    if (!tag_edit_current_directory_path(screen, &path, &path_len)) {
        changed = screen->observed_dir_valid;
        tag_edit_observe_current_directory(screen);
    } else {
        changed = !screen->observed_dir_valid
                  || !STREQUAL(screen->observed_dir.data,
                               screen->observed_dir.len, path, path_len)
                  || (screen->last_directory_highlight
                      != nc_menu_highlight(menu));
        if (changed) {
            tag_edit_observe_current_directory(screen);
        }
    }
    if (changed) {
        tag_edit_screen_clear_stale_tags(screen);
    }
    return;
}

void
tag_edit_screen_set_current_dir(TagEditScreen *screen,
                                char *dir, int32 dir_len) {
    bool changed;

    changed = screen->current_dir.data
              && !STREQUAL(screen->current_dir.data, screen->current_dir.len,
                           dir, dir_len);
    sb_set(&screen->current_dir, dir, dir_len);
    screen->directories_update_requested = true;
    if (changed) {
        tag_edit_screen_clear_stale_tags(screen);
    }
    return;
}

int32
tag_edit_screen_current_dir(TagEditScreen *screen, StringView *view) {
    if (view) {
        *view = (StringView){0};
    }
    if ((screen == NULL) || (view == NULL)) {
        return -EINVAL;
    }
    ncm_string_view_set(view,
                        screen->current_dir.data, screen->current_dir.len);
    if (screen->current_dir.data == NULL) {
        return -NCM_ERROR_NOT_FOUND;
    }
    return 0;
}

int32
tag_edit_screen_current_directory_path(TagEditScreen *screen,
                                       StringView *view) {
    char *path;
    int32 path_len;

    *view = (StringView){0};
    if (!tag_edit_current_directory_path(screen, &path, &path_len)) {
        return -NCM_ERROR_NOT_FOUND;
    }
    ncm_string_view_set(view, path, path_len);
    return 0;
}

int32
tag_edit_screen_enter_directory(TagEditScreen *screen) {
    StringView path = {0};
    NcmDirectoryArray directories = {0};
    NcmError ncm_error;
    int32 status;
    bool has_subdirectories;

    if (screen == NULL) {
        return -EINVAL;
    }
    if (screen->active_focus != TAG_EDIT_FOCUS_DIRECTORIES) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    status = tag_edit_screen_current_directory_path(screen, &path);
    if (status < 0) {
        return status;
    }

    has_subdirectories = false;
    if (path.len > 0) {
        ncm_error_clear(&ncm_error);
        has_subdirectories =
            (ncm_mpd_client_get_directory_list(&global_mpd, path.data,
                                               &directories, &ncm_error) == 0)
            && (directories.len > 0);
        ncm_error_clear(&ncm_error);
        ncm_directory_array_destroy(&directories);
    }
    if (!has_subdirectories) {
        tag_edit_status_message(screen, STRLIT("No subdirectories found"));
        return -NCM_ERROR_NOT_FOUND;
    }
    sb_clear(&screen->highlighted_dir);
    tag_edit_screen_set_current_dir(screen, path.data, path.len);
    nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
    tag_edit_screen_clear_stale_tags(screen);
    screen->directories_update_requested = true;
    screen->observed_dir_valid = false;
    tag_edit_update_titles(screen, true);

    return 0;
}

int32
tag_edit_screen_go_to_parent(TagEditScreen *screen) {
    StrBuilder parent = {0};
    int32 parent_len;

    if (screen == NULL) {
        return -EINVAL;
    }
    if (screen->active_focus != TAG_EDIT_FOCUS_DIRECTORIES) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if ((screen->current_dir.data == NULL) || (screen->current_dir.len <= 0)
        || STREQUAL(screen->current_dir.data, screen->current_dir.len, "/")) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    sb_set(&screen->highlighted_dir,
           screen->current_dir.data, screen->current_dir.len);
    parent_len = ncm_string_parent_directory_len(screen->current_dir.data,
                                                 screen->current_dir.len);
    if (parent_len <= 0) {
        sb_set(&parent, STRLIT("/"));
    } else {
        sb_set(&parent, screen->current_dir.data, parent_len);
    }
    tag_edit_screen_set_current_dir(screen, parent.data, parent.len);
    sb_free(&parent);

    nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
    tag_edit_screen_clear_stale_tags(screen);
    screen->directories_update_requested = true;
    screen->observed_dir_valid = false;
    tag_edit_update_titles(screen, true);
    return 0;
}

int32
tag_edit_screen_locate_song(TagEditScreen *screen, NcmSong *song) {
    StringView directory;
    StringView uri;
    StrBuilder parent = {0};
    NcmError ncm_error;
    int32 parent_len;
    int32 status;

    if ((screen == NULL) || (song == NULL)) {
        return -EINVAL;
    }
    if (!ncm_song_has_uri_view(song, 0, &uri) || (uri.len <= 0)) {
        return -NCM_ERROR_NOT_FOUND;
    }
    if (!ncm_string_contains_char(uri.data, uri.len, '/')) {
        return -NCM_ERROR_NOT_FOUND;
    }
    if (!ncm_song_has_directory_view(song, 0, &directory)
        || (directory.len <= 0)) {
        return -NCM_ERROR_NOT_FOUND;
    }

    parent_len = ncm_string_parent_directory_len(directory.data, directory.len);
    if (parent_len <= 0) {
        sb_set(&parent, STRLIT("/"));
    } else {
        sb_set(&parent, directory.data, parent_len);
    }
    tag_edit_screen_set_current_dir(screen, parent.data, parent.len);
    sb_set(&screen->highlighted_dir, directory.data, directory.len);

    nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
    ncm_error_clear(&ncm_error);
    status = tag_edit_reload_directories_from_mpd(screen, &global_mpd,
                                                  &ncm_error);
    if (status == 0) {
        NcMenu *menu = nc_editor_pair_menu_base(&screen->directories);
        bool found = false;

        for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
            StrBuilderPair *item;

            item = nc_menu_active_item_at(menu, i);
            ASSERT(item != NULL);
            ASSERT(item->second.data != NULL);
            if (STREQUAL(item->second.data, item->second.len,
                         directory.data, directory.len)) {
                nc_menu_goto_selectable(menu, i);
                tag_edit_observe_current_directory(screen);
                found = true;
                break;
            }
        }
        if (!found) {
            status = -NCM_ERROR_NOT_FOUND;
        }
    }
    if (status == 0) {
        tag_edit_screen_clear_stale_tags(screen);
        ncm_error_clear(&ncm_error);
        status = tag_edit_reload_songs_from_mpd(screen, &global_mpd,
                                                &ncm_error);
    }
    if (status == 0) {
        NcMenu *menu = nc_tag_row_menu_base(&screen->tags);
        bool found = false;

        nc_menu_reset(nc_editor_string_menu_base(&screen->tag_types));
        tag_edit_set_focus(screen, TAG_EDIT_FOCUS_TAGS);
        for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
            MutableSong *item;

            item = nc_menu_active_item_at(menu, i);
            ASSERT(item != NULL);
            ASSERT(item->uri != NULL);
            if (STREQUAL(item->uri, item->uri_len, uri.data, uri.len)) {
                nc_menu_goto_selectable(menu, i);
                found = true;
                break;
            }
        }
        if (!found) {
            status = -NCM_ERROR_NOT_FOUND;
        }
    }

    sb_free(&parent);
    tag_edit_update_titles(screen, true);
    return status;
}

static bool
tag_edit_current_directory_pair(TagEditScreen *screen, StrBuilderPair **pair) {
    StrBuilderPair *current;

    ASSERT(screen != NULL);
    ASSERT(pair != NULL);

    *pair = NULL;
    current = nc_editor_pair_menu_current(&screen->directories);
    if (current == NULL) {
        return false;
    }
    ASSERT(current->first.data != NULL);
    ASSERT(current->second.data != NULL);
    *pair = current;
    return true;
}

bool
tag_edit_screen_rename_directory_available(TagEditScreen *screen,
                                             char *music_dir,
                                             int32 music_dir_len) {
    StrBuilderPair *pair;

    if ((screen == NULL) || (music_dir == NULL) || (music_dir_len <= 0)) {
        return false;
    }
    if (screen->active_focus != TAG_EDIT_FOCUS_DIRECTORIES) {
        return false;
    }
    if (!tag_edit_current_directory_pair(screen, &pair)) {
        return false;
    }
    if (STREQUAL(pair->first.data, pair->first.len, ".")
        || STREQUAL(pair->first.data, pair->first.len, "..")) {
        return false;
    }
    return true;
}

int32
tag_edit_screen_rename_current_directory(TagEditScreen *screen,
                                         char *music_dir,
                                         int32 music_dir_len) {
    StrBuilderPair *pair;
    StringView initial;
    StrBuilder name = {0};
    StrBuilder old_path = {0};
    StrBuilder new_path = {0};
    StrBuilder new_relative = {0};
    NcmError ncm_error;
    enum TagEditPromptResult result;
    int32 status;

    if (!tag_edit_screen_rename_directory_available(screen,
                                                    music_dir, music_dir_len)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if ((screen->hooks.prompt == NULL)
        || !tag_edit_current_directory_pair(screen, &pair)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    ncm_string_view_set(&initial, pair->first.data, pair->first.len);
    result = screen->hooks.prompt(screen->hooks.user, STRLIT("Directory: "),
                                  initial, &name);
    if (result == TAG_EDIT_PROMPT_ABORTED) {
        sb_free(&name);
        return 0;
    }
    if (result != TAG_EDIT_PROMPT_ACCEPTED) {
        sb_free(&name);
        return -NCM_ERROR_UNAVAILABLE;
    }
    if ((name.len <= 0)
        || STREQUAL(name.data, name.len, pair->first.data, pair->first.len)) {
        sb_free(&name);
        return 0;
    }

    ncm_fs_join(&old_path, music_dir, music_dir_len,
                pair->second.data, pair->second.len);
    ncm_fs_join(&new_relative,
                screen->current_dir.data, screen->current_dir.len,
                name.data, name.len);
    ncm_fs_join(&new_path, music_dir, music_dir_len,
                new_relative.data, new_relative.len);

    ncm_error_clear(&ncm_error);
    status = ncm_fs_rename(old_path.data, old_path.len,
                           new_path.data, new_path.len, &ncm_error);
    if (status < 0) {
        StrBuilder message = {0};
        int32 error_len;

        SB_APPEND(&message, "Couldn't rename \"");
        SB_APPEND(&message, pair->first.data, pair->first.len);
        SB_APPEND(&message, "\": ");
        if (ncm_error_is_set(&ncm_error)) {
            error_len = ncm_error.message_len;
            SB_APPEND(&message, ncm_error.message, error_len);
        } else {
            SB_APPEND(&message, "unknown error");
        }
        tag_edit_status_message(screen, message.data, message.len);
        sb_free(&message);
    }
    if (status == 0) {
        StrBuilder message = {0};

        SB_APPEND(&message, "Directory renamed to \"");
        SB_APPEND(&message, name.data, name.len);
        SB_APPEND(&message, "\"");
        tag_edit_status_message(screen, message.data, message.len);
        sb_free(&message);
        if (screen->hooks.update_directory) {
            screen->hooks.update_directory(screen->hooks.user,
                                           screen->current_dir.data,
                                           screen->current_dir.len);
        }
        sb_set(&screen->highlighted_dir, new_relative.data, new_relative.len);
        screen->directories_update_requested = true;
        tag_edit_update_titles(screen, true);
    }

    sb_free(&new_relative);
    sb_free(&new_path);
    sb_free(&old_path);
    sb_free(&name);
    return status;
}

void
tag_edit_screen_add_directory(TagEditScreen *screen,
                              char *label, int32 label_len,
                              char *path, int32 path_len) {
    StrBuilderPair pair = {0};

    sb_set(&pair.first, label, label_len);
    sb_set(&pair.second, path, path_len);
    nc_editor_pair_menu_add(&screen->directories, &pair);
    screen->last_known_directory_count =
        nc_menu_item_count(nc_editor_pair_menu_base(&screen->directories));
    tag_edit_update_titles(screen, true);
    sb_free(&pair.second);
    sb_free(&pair.first);
    return;
}

void
tag_edit_screen_load_songs(TagEditScreen *screen, NcmSongArray *songs) {
    char *path;
    int32 path_len;

    nc_menu_clear_items(nc_tag_row_menu_base(&screen->tags));
    for (int32 i = 0; i < songs->len; i += 1) {
        MutableSong mutable_song = {0};

        mutable_song_load_originals_from_song(&mutable_song, &songs->items[i]);
        tag_edit_screen_add_mutable_song(screen, &mutable_song);
        mutable_song_destroy(&mutable_song);
    }
    if (tag_edit_current_directory_path(screen, &path, &path_len)) {
        sb_set(&screen->displayed_dir, path, path_len);
        screen->displayed_dir_valid = true;
    } else {
        sb_clear(&screen->displayed_dir);
        screen->displayed_dir_valid = false;
    }
    screen->tags_update_requested = false;
    screen->last_known_tag_count =
        nc_menu_item_count(nc_tag_row_menu_base(&screen->tags));
    tag_edit_update_titles(screen, true);
    return;
}

void
tag_edit_screen_add_mutable_song(TagEditScreen *screen, MutableSong *song) {
    nc_tag_row_menu_add(&screen->tags, song);
    screen->last_known_tag_count =
        nc_menu_item_count(nc_tag_row_menu_base(&screen->tags));
    tag_edit_update_titles(screen, true);
    return;
}

static void
tag_edit_copy_selected_song_at(TagEditScreen *screen,
                                 NcmSongArray *songs, int32 pos) {
    MutableSong *source;
    NcmSong song = {0};

    source = nc_menu_active_item_at(nc_tag_row_menu_base(&screen->tags), pos);
    ASSERT(source != NULL);
    ASSERT(source->uri != NULL);
    ASSERT_POSITIVE(source->uri_len);

    ncm_song_set_uri(&song, source->uri, source->uri_len);
    ncm_song_set_duration(&song, source->duration);
    ncm_song_set_mtime(&song, source->mtime);
    for (int32 i = 0; i < source->tags_len; i += 1) {
        MutableSongTag *tag = &source->tags[i];
        enum TagType type = tag->type;
        char *value = tag->original;
        int32 value_len = tag->original_len;

        if ((type == TAG_COUNT) || (value == NULL) || (value_len <= 0)) {
            continue;
        }
        ncm_song_add_tag(&song, type, value, value_len);
    }

    ncm_song_array_append_move(songs, &song);
    ncm_song_destroy(&song);
    return;
}

int32
tag_edit_screen_selected_songs(TagEditScreen *screen, NcmSongArray *songs) {
    NcMenu *menu;

    if ((screen == NULL) || (songs == NULL)) {
        return -EINVAL;
    }
    ncm_song_array_clear(songs);
    if (screen->active_focus != TAG_EDIT_FOCUS_TAGS) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    menu = nc_tag_row_menu_base(&screen->tags);
    if (!nc_menu_has_selected(menu)) {
        if (nc_menu_item_count(menu) <= 0) {
            return 0;
        }
        tag_edit_copy_selected_song_at(screen, songs, nc_menu_highlight(menu));
        return 0;
    }

    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (!nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        tag_edit_copy_selected_song_at(screen, songs, i);
    }
    return 0;
}

bool
tag_edit_screen_previous_column_available(TagEditScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (screen->active_focus == TAG_EDIT_FOCUS_TAGS) {
        NcMenu *menu = nc_editor_string_menu_base(&screen->tag_types);
        return nc_menu_item_count(menu) > 0;
    }
    if (screen->active_focus == TAG_EDIT_FOCUS_TAG_TYPES) {
        NcMenu *menu = nc_editor_pair_menu_base(&screen->directories);
        if (nc_menu_item_count(menu) <= 0) {
            return false;
        }
        return true;
    }
    if (tag_edit_focus_is_parser_helper(screen->active_focus)) {
        NcMenu *menu = nc_editor_string_menu_base(&screen->parser_actions);
        return nc_menu_item_count(menu) > 0;
    }
    return false;
}

bool
tag_edit_screen_next_column_available(TagEditScreen *screen) {
    NcMenu *tag_types;
    NcMenu *tags;
    int32 choice;

    if (screen == NULL) {
        return false;
    }
    if (screen->active_focus == TAG_EDIT_FOCUS_DIRECTORIES) {
        tag_types = nc_editor_string_menu_base(&screen->tag_types);
        tags = nc_tag_row_menu_base(&screen->tags);
        return (nc_menu_item_count(tag_types) > 0)
               && (nc_menu_item_count(tags) > 0);
    }
    if (screen->active_focus == TAG_EDIT_FOCUS_TAG_TYPES) {
        tag_types = nc_editor_string_menu_base(&screen->tag_types);
        tags = nc_tag_row_menu_base(&screen->tags);
        choice = nc_menu_highlight(tag_types);
        return (nc_menu_item_count(tags) > 0)
               && ((choice < (int32)TAG_COUNT)
                   || tag_edit_choice_is_filename(choice));
    }
    if (screen->active_focus == TAG_EDIT_FOCUS_PARSER_ACTIONS) {
        return true;
    }
    return false;
}

void
tag_edit_screen_previous_column(TagEditScreen *screen) {
    if (!tag_edit_screen_previous_column_available(screen)) {
        return;
    }
    if (screen->active_focus == TAG_EDIT_FOCUS_TAGS) {
        tag_edit_set_focus(screen, TAG_EDIT_FOCUS_TAG_TYPES);
    } else if (screen->active_focus == TAG_EDIT_FOCUS_TAG_TYPES) {
        NcMenu *menu = nc_tag_row_menu_base(&screen->tags);
        bool modified = false;

        for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
            MutableSong *song = nc_menu_active_item_at(menu, i);

            ASSERT(song != NULL);
            if (mutable_song_is_modified(song)) {
                modified = true;
                break;
            }
        }
        if (modified
            && !tag_edit_confirm(screen,
                                 STRLIT("There are pending changes, "
                                        "are you sure?"))) {
            return;
        }
        tag_edit_set_focus(screen, TAG_EDIT_FOCUS_DIRECTORIES);
    } else if (tag_edit_focus_is_parser_helper(screen->active_focus)) {
        tag_edit_set_focus(screen, TAG_EDIT_FOCUS_PARSER_ACTIONS);
    }
    tag_edit_finish_tag_type_change(screen, false);
    tag_edit_refresh_if_visible(screen);
    return;
}

void
tag_edit_screen_next_column(TagEditScreen *screen) {
    if (!tag_edit_screen_next_column_available(screen)) {
        return;
    }
    if (screen->active_focus == TAG_EDIT_FOCUS_DIRECTORIES) {
        tag_edit_set_focus(screen, TAG_EDIT_FOCUS_TAG_TYPES);
    } else if (screen->active_focus == TAG_EDIT_FOCUS_TAG_TYPES) {
        tag_edit_set_focus(screen, TAG_EDIT_FOCUS_TAGS);
    } else if (screen->active_focus == TAG_EDIT_FOCUS_PARSER_ACTIONS) {
        tag_edit_set_focus(screen, tag_edit_current_helper_focus(screen));
    }
    tag_edit_finish_tag_type_change(screen, false);
    tag_edit_refresh_if_visible(screen);
    return;
}

static int32
tag_edit_for_each_target(TagEditScreen *screen,
                         int32 (*callback)(MutableSong *song, void *user),
                         void *user) {
    NcMenu *menu = nc_tag_row_menu_base(&screen->tags);
    bool has_selected = nc_menu_has_selected(menu);
    int32 count = 0;
    int32 status;

    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        MutableSong *song;

        if (has_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }

        song = nc_menu_active_item_at(menu, i);
        ASSERT(song != NULL);
        status = callback(song, user);
        if (status < 0) {
            return status;
        }
        count += 1;
    }
    return count;
}

static int32
tag_edit_set_song_tag_callback(MutableSong *song, void *user) {
    TagSetter *setter = user;

    mutable_song_set_tags(song, setter->tag_type,
                          setter->value, setter->value_len,
                          setter->separator, setter->separator_len);
    return 0;
}

int32
tag_edit_screen_apply_tag_to_selection(TagEditScreen *screen,
                                       enum TagType tag_type,
                                       char *value, int32 value_len,
                                       char *separator, int32 separator_len) {
    TagSetter setter;

    if ((screen == NULL) || (value == NULL)) {
        return -EINVAL;
    }
    if ((uint32)tag_type >= TAG_COUNT) {
        return -EINVAL;
    }
    if ((value_len < 0) || (separator_len < 0)) {
        return -EINVAL;
    }

    setter.tag_type = tag_type;
    setter.value = value;
    setter.value_len = value_len;
    setter.separator = separator;
    setter.separator_len = separator_len;
    return tag_edit_for_each_target(screen,
                                    tag_edit_set_song_tag_callback, &setter);
}

static int32
tag_edit_number_song_callback(MutableSong *song, void *user) {
    TrackNumberer *numberer = user;
    StringView view;
    char buffer[64];
    int32 len;

    if (numberer->extended) {
        len = SNPRINTF(buffer, "%d/%d", numberer->current, numberer->total);
    } else {
        len = SNPRINTF(buffer, "%d", numberer->current);
    }

    numberer->current += 1;
    mutable_song_set_tag(song, TAG_TRACK, 0, buffer, len);
    for (int32 i = 1;
         mutable_song_has_tag_view(song, TAG_TRACK, i, &view);
         i += 1) {
        mutable_song_set_tag(song, TAG_TRACK, i, STRLIT(""));
    }
    return 0;
}

int32
tag_edit_screen_number_tracks(TagEditScreen *screen, bool extended) {
    TrackNumberer numberer = {0};
    NcMenu *menu = nc_tag_row_menu_base(&screen->tags);

    numberer.current = 1;
    if (nc_menu_has_selected(menu)) {
        numberer.total = nc_menu_selected_count(menu);
    } else {
        numberer.total = nc_menu_item_count(menu);
    }
    numberer.extended = extended;
    return tag_edit_for_each_target(screen,
                                    tag_edit_number_song_callback, &numberer);
}

static int32
tag_edit_capitalize_song_callback(MutableSong *song, void *user) {
    (void)user;

    for (uint32 fi = 0; fi < TAG_COUNT; fi += 1) {
        enum TagType tag_type = ncm_song_info_tags[fi].tag;

        for (int32 i = 0; ; i += 1) {
            StringView view;
            StrBuilder converted = {0};
            int32 converted_len;

            if (!mutable_song_has_tag_view(song, tag_type, i, &view)) {
                break;
            }

            converted_len = utf8_capitalize_first_letters(view.data, view.len,
                                                          NULL, 0);
            sb_reserve(&converted, converted_len);
            converted.len = utf8_capitalize_first_letters(view.data, view.len,
                                                          converted.data,
                                                          converted_len);
            if (converted.data) {
                converted.data[converted.len] = '\0';
            }
            mutable_song_set_tag(song, tag_type, i,
                                 converted.data, converted.len);
            sb_free(&converted);
        }
    }

    return 0;
}

void
tag_edit_screen_capitalize_first_letters(TagEditScreen *screen) {
    tag_edit_for_each_target(screen, tag_edit_capitalize_song_callback, NULL);
    return;
}

static int32
tag_edit_lower_song_callback(MutableSong *song, void *user) {
    (void)user;
    for (uint32 j = 0; j < TAG_COUNT; j += 1) {
        enum TagType tag_type = ncm_song_info_tags[j].tag;

        for (int32 i = 0; ; i += 1) {
            StringView view;
            StrBuilder buffer = {0};

            if (!mutable_song_has_tag_view(song, tag_type, i, &view)) {
                break;
            }
            SB_APPEND(&buffer, view.data, view.len);
            if (buffer.data != NULL) {
                ncm_string_lowercase_ascii(buffer.data, buffer.len);
            }
            mutable_song_set_tag(song, tag_type, i, buffer.data, buffer.len);
            sb_free(&buffer);
        }
    }
    return 0;
}

void
tag_edit_screen_lower_all_letters(TagEditScreen *screen) {
    tag_edit_for_each_target(screen, tag_edit_lower_song_callback, NULL);
    return;
}

void
tag_edit_screen_clear_modifications(TagEditScreen *screen) {
    NcMenu *menu = nc_tag_row_menu_base(&screen->tags);

    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        MutableSong *song = nc_menu_active_item_at(menu, i);
        ASSERT(song != NULL);
        mutable_song_clear_modifications(song);
    }
    return;
}

static int32
tag_edit_save_song_callback(MutableSong *song, void *user) {
    SaveContext *context = user;
    int32 status;
    int32 error_code;

    context->target_count += 1;
    {
        char *directory = song->directory;
        int32 directory_len = song->directory_len;

        if ((directory == NULL) && song->uri) {
            directory = song->uri;
            directory_len = ncm_string_parent_directory_len(song->uri,
                                                            song->uri_len);
        }
        if (directory == NULL) {
            directory = "";
            directory_len = 0;
        }

        if (!context->shared_directory_valid) {
            sb_set(&context->shared_directory, directory, directory_len);
            context->shared_directory_valid = true;
        } else if (!STREQUAL(context->shared_directory.data,
                             context->shared_directory.len,
                             directory, directory_len)) {
            StrBuilder shared = {0};

            shared = ncm_string_shared_directory(context->shared_directory.data,
                                                 context->shared_directory.len,
                                                 directory, directory_len);
            sb_free(&context->shared_directory);
            context->shared_directory = shared;
        }
    }
    if (!mutable_song_is_modified(song)) {
        return 0;
    }

    context->modified_count += 1;
    {
        StrBuilder message = {0};

        SB_APPEND(&message, "Writing tags in \"");
        if (song->name) {
            SB_APPEND(&message, song->name, song->name_len);
        }
        SB_APPEND(&message, "\"...");
        tag_edit_status_message(context->screen, message.data, message.len);
        sb_free(&message);
    }

    status = mutable_song_write(song, context->music_dir);
    if (status < 0) {
        StrBuilder message = {0};
        char *system_error;

        error_code = -status;
        if (error_code >= NCM_ERROR_PROJECT_BASE) {
            error_code = EIO;
        }
        system_error = strerror(error_code);

        SB_APPEND(&message, "Error while writing tags to \"");
        if (song->name) {
            SB_APPEND(&message, song->name, song->name_len);
        }
        SB_APPEND(&message, "\": ");
        SB_APPEND(&message, system_error, strlen32(system_error));
        tag_edit_status_message(context->screen, message.data, message.len);

        sb_free(&message);
        return status;
    }

    context->write_count += 1;
    mutable_song_clear_modifications(song);
    return 0;
}

int32
tag_edit_screen_save_modified(TagEditScreen *screen, char *music_dir) {
    SaveContext context = {0};
    int32 status;

    if (screen == NULL) {
        return -EINVAL;
    }

    tag_edit_status_message(screen, STRLIT("Writing changes..."));

    context.screen = screen;
    context.music_dir = music_dir;

    status = tag_edit_for_each_target(screen,
                                      tag_edit_save_song_callback, &context);
    if (status < 0) {
        sb_free(&context.shared_directory);
        tag_edit_screen_clear_stale_tags(screen);
        return status;
    }

    tag_edit_status_message(screen, STRLIT("Tags updated"));
    nc_menu_reset(nc_editor_string_menu_base(&screen->tag_types));
    tag_edit_set_focus(screen, TAG_EDIT_FOCUS_DIRECTORIES);
    if (context.shared_directory_valid
        && (screen->hooks.update_directory != NULL)) {
        sb_reserve(&context.shared_directory, 1);
        context.shared_directory.data[context.shared_directory.len] = '\0';
        screen->hooks.update_directory(screen->hooks.user,
                                       context.shared_directory.data,
                                       context.shared_directory.len);
    }
    sb_free(&context.shared_directory);
    return context.target_count;
}

bool
tag_edit_screen_save_action_available(TagEditScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->active_focus == TAG_EDIT_FOCUS_TAG_TYPES;
}

static int32
tag_edit_compile_constraint(NcmRegex *regex,
                            char *pattern, int32 pattern_len,
                            uint32 regex_flags, NcmError *ncm_error) {
    NcmRegex compiled = {0};
    int32 status;

    ASSERT(regex != NULL);

    if ((status = ncm_regex_compile(&compiled, pattern, pattern_len,
                                    regex_flags, ncm_error)) < 0) {
        ncm_regex_destroy(&compiled);
        return status;
    }
    ncm_regex_destroy(regex);
    *regex = compiled;
    return ncm_error_ok(ncm_error);
}

int32
tag_edit_screen_apply_directory_filter(TagEditScreen *screen,
                                       char *pattern, int32 pattern_len,
                                       uint32 regex_flags,
                                       NcmError *ncm_error) {
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing tag editor"));
    }
    if ((pattern == NULL) || (pattern_len <= 0)) {
        ncm_regex_destroy(&screen->directory_filter_regex);
        screen->directory_filter_regex = (NcmRegex){0};
        sb_clear(&screen->directory_filter_constraint);
        screen->directory_filter_enabled = false;
        nc_menu_show_all_items(nc_editor_pair_menu_base(&screen->directories));
        tag_edit_update_titles(screen, true);

        return ncm_error_ok(ncm_error);
    }
    if ((status = tag_edit_compile_constraint(&screen->directory_filter_regex,
                                              pattern, pattern_len, regex_flags,
                                              ncm_error)) < 0) {
        return status;
    }
    sb_set(&screen->directory_filter_constraint, pattern, pattern_len);
    {
        NcMenu *menu = nc_editor_pair_menu_base(&screen->directories);
        NcMenuDisplayCallbacks callbacks;

        callbacks = tag_edit_directory_display_callbacks(screen);
        nc_menu_set_display_callbacks(menu, callbacks);
    }
    screen->directory_filter_enabled = true;
    nc_menu_apply_filter(nc_editor_pair_menu_base(&screen->directories));
    tag_edit_update_titles(screen, true);
    return ncm_error_ok(ncm_error);
}

int32
tag_edit_screen_apply_tag_filter(TagEditScreen *screen,
                                 char *pattern, int32 pattern_len,
                                 uint32 regex_flags, NcmError *ncm_error) {
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing tag editor"));
    }
    if ((pattern == NULL) || (pattern_len <= 0)) {
        ncm_regex_destroy(&screen->tag_filter_regex);
        screen->tag_filter_regex = (NcmRegex){0};
        sb_clear(&screen->tag_filter_constraint);
        screen->tag_filter_enabled = false;
        nc_menu_show_all_items(nc_tag_row_menu_base(&screen->tags));
        tag_edit_update_titles(screen, true);
        return ncm_error_ok(ncm_error);
    }
    if ((status = tag_edit_compile_constraint(&screen->tag_filter_regex,
                                              pattern, pattern_len, regex_flags,
                                              ncm_error)) < 0) {
        return status;
    }
    sb_set(&screen->tag_filter_constraint, pattern, pattern_len);
    nc_menu_set_display_callbacks(nc_tag_row_menu_base(&screen->tags),
                                  tag_edit_tag_display_callbacks(screen));
    screen->tag_filter_enabled = true;
    nc_menu_apply_filter(nc_tag_row_menu_base(&screen->tags));
    tag_edit_update_titles(screen, true);
    return ncm_error_ok(ncm_error);
}

static bool
tag_edit_search_position(NcMenu *menu, int32 pos, void *user) {
    TagEditSearchContext *context = user;
    TagEditScreen *screen = context->screen;

    if (screen->active_focus == TAG_EDIT_FOCUS_TAGS) {
        MutableSong *song = nc_menu_active_item_at(menu, pos);
        return tag_edit_tag_matches_regex(screen, song, context->regex);
    }
    if (screen->active_focus == TAG_EDIT_FOCUS_DIRECTORIES) {
        StrBuilderPair *pair = nc_menu_active_item_at(menu, pos);
        return tag_edit_directory_matches_regex(pair, context->regex, false);
    }
    return false;
}

int32
tag_edit_screen_search(TagEditScreen *screen, char *pattern, int32 pattern_len,
                       bool forward, bool wrap, bool skip_current,
                       NcmError *ncm_error) {
    TagEditSearchContext context;
    NcmRegex *regex;
    StrBuilder *constraint;
    bool *enabled;
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing tag editor"));
    }
    if ((pattern == NULL) || (pattern_len <= 0)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing search pattern"));
    }
    if ((screen->active_focus != TAG_EDIT_FOCUS_DIRECTORIES)
        && (screen->active_focus != TAG_EDIT_FOCUS_TAGS)) {
        return ncm_error_set_code(ncm_error, NCM_ERROR_UNAVAILABLE,
                                  STRLIT("tag editor cannot search"));
    }

    if (screen->active_focus == TAG_EDIT_FOCUS_TAGS) {
        regex = &screen->tag_search_regex;
        constraint = &screen->tag_search_constraint;
        enabled = &screen->tag_search_enabled;
    } else {
        regex = &screen->directory_search_regex;
        constraint = &screen->directory_search_constraint;
        enabled = &screen->directory_search_enabled;
    }
    if ((status = tag_edit_compile_constraint(regex, pattern, pattern_len,
                                              Config.regular_expressions,
                                              ncm_error)) < 0) {
        return status;
    }
    sb_set(constraint, pattern, pattern_len);
    *enabled = true;

    {
        NcMenu *menu = tag_edit_screen_active_menu(screen);
        NcWindow *window = tag_edit_screen_active_window(screen);
        bool found;

        context.screen = screen;
        context.regex = regex;

        found = nc_menu_search_selectable(menu, nc_window_height(window),
                                          forward, wrap, skip_current,
                                          tag_edit_search_position, &context,
                                          NULL) == 0;
        if (found) {
            tag_edit_screen_finish_directory_change(screen);
            return 1;
        }
    }
    return 0;
}

static void
tag_edit_reset_parser_navigation(TagEditScreen *screen) {
    ASSERT(screen != NULL);
    nc_menu_reset(nc_editor_string_menu_base(&screen->parser_dialog));
    nc_menu_reset(nc_editor_string_menu_base(&screen->parser_rows));
    nc_menu_reset(nc_editor_string_menu_base(&screen->parser_actions));
    return;
}

static void
tag_edit_append_parser_separator(TagEditScreen *screen) {
    nc_editor_string_menu_add_separator(&screen->parser_rows);
    nc_editor_string_menu_add_separator(&screen->parser_actions);
    return;
}

static void
tag_edit_append_parser_row(NcEditorStringMenu *menu,
                           char *data, int32 data_len, uint32 flags) {
    StrBuilder string = {0};

    sb_set(&string, data, data_len);
    nc_editor_string_menu_add_with_flags(menu, &string, flags);
    sb_free(&string);
    return;
}

static void
tag_edit_append_parser_action_row(TagEditScreen *screen,
                                  char *data, int32 data_len, uint32 flags) {
    tag_edit_append_parser_row(&screen->parser_rows, data, data_len, flags);
    tag_edit_append_parser_row(&screen->parser_actions, data, data_len, flags);
    return;
}

static void
tag_edit_append_parser_action_label(TagEditScreen *screen,
                                    char *label, int32 label_len) {
    tag_edit_append_parser_action_row(screen, label, label_len,
                                      NC_MENU_ITEM_SELECTABLE);
    return;
}

void
tag_edit_screen_prepare_parser_rows(TagEditScreen *screen,
                                    enum TagEditParserMode mode,
                                    char *pattern, int32 pattern_len) {
    screen->parser_mode = mode;
    if (pattern) {
        tag_edit_set_pattern(screen, pattern, pattern_len);
    } else if ((mode != TAG_EDIT_PARSER_MODE_NONE) && (screen->pattern.len <= 0)
               && Config.default_tag_edit_pattern) {
        tag_edit_set_pattern(screen, Config.default_tag_edit_pattern,
                             Config.default_tag_edit_pattern_len);
    }

    nc_menu_clear_items(nc_editor_string_menu_base(&screen->parser_dialog));
    nc_menu_clear_items(nc_editor_string_menu_base(&screen->parser_rows));
    nc_menu_clear_items(nc_editor_string_menu_base(&screen->parser_actions));

    tag_edit_append_parser_row(&screen->parser_dialog,
                               STRLIT("Get tags from filename"),
                               NC_MENU_ITEM_SELECTABLE);
    tag_edit_append_parser_row(&screen->parser_dialog,
                               STRLIT("Rename files"),
                               NC_MENU_ITEM_SELECTABLE);
    tag_edit_append_parser_row(&screen->parser_dialog,
                               STRLIT("Cancel"),
                               NC_MENU_ITEM_SELECTABLE);

    if (mode == TAG_EDIT_PARSER_MODE_NONE) {
        tag_edit_reset_parser_navigation(screen);
        return;
    }

    tag_edit_append_parser_row(&screen->parser_rows,
                               STRLIT("Get tags from filename"),
                               NC_MENU_ITEM_SELECTABLE);
    tag_edit_append_parser_row(&screen->parser_rows,
                               STRLIT("Rename files"),
                               NC_MENU_ITEM_SELECTABLE);
    tag_edit_append_parser_row(&screen->parser_rows,
                               STRLIT("Cancel"),
                               NC_MENU_ITEM_SELECTABLE);

    {
        StrBuilder row = {0};

        SB_APPEND(&row, "Pattern: ");
        SB_APPEND(&row, screen->pattern.data, screen->pattern.len);
        tag_edit_append_parser_action_label(screen, row.data, row.len);
        sb_free(&row);
    }
    tag_edit_append_parser_action_label(screen, STRLIT("Preview"));
    tag_edit_append_parser_action_label(screen, STRLIT("Legend"));
    tag_edit_append_parser_separator(screen);
    tag_edit_append_parser_action_label(screen, STRLIT("Proceed"));
    tag_edit_append_parser_action_label(screen, STRLIT("Cancel"));
    if (screen->recent_patterns.len > 0) {
        tag_edit_append_parser_separator(screen);
        tag_edit_append_parser_action_row(screen, STRLIT("Recent patterns"),
                                          NC_MENU_ITEM_INACTIVE);
        tag_edit_append_parser_separator(screen);
        for (int32 i = 0; i < screen->recent_patterns.len; i += 1) {
            StrBuilder *recent_pattern = &screen->recent_patterns.items[i];
            tag_edit_append_parser_action_label(screen,
                                                recent_pattern->data,
                                                recent_pattern->len);
        }
    }
    tag_edit_build_parser_legend(screen);
    tag_edit_reset_parser_navigation(screen);
    return;
}

void
tag_edit_screen_show_parser_dialog(TagEditScreen *screen) {
    NcMenu *menu = nc_editor_string_menu_base(&screen->parser_dialog);

    if (nc_menu_item_count(menu) <= 0) {
        tag_edit_screen_prepare_parser_rows(screen, TAG_EDIT_PARSER_MODE_NONE,
                                            NULL, 0);
    }
    screen->parser_mode = TAG_EDIT_PARSER_MODE_NONE;
    tag_edit_set_focus(screen, TAG_EDIT_FOCUS_PARSER_CHOICE);
    return;
}

void
tag_edit_screen_show_parser_actions(TagEditScreen *screen,
                                    enum TagEditParserMode mode) {
    if (mode == TAG_EDIT_PARSER_MODE_NONE) {
        return;
    }
    if (!screen->recent_patterns_loaded) {
        StrBuilder path = {0};
        StrBuilder line = {0};
        FILE *file;
        int32 status;

        screen->recent_patterns_loaded = true;
        tag_edit_history_path(&path);
        status = 0;
        file = fopen(path.data, "r");
        if (file == NULL) {
            if (errno != ENOENT) {
                status = errno ? -errno : -EIO;
            }
        } else {
            while (true) {
                bool read_line = false;
                int32 ch;

                sb_clear(&line);
                while (true) {
                    ch = fgetc(file);
                    if (ch == EOF) {
                        break;
                    }
                    read_line = true;
                    if (ch == '\n') {
                        break;
                    }
                    sb_append_byte(&line, (char)ch);
                }
                if (ferror(file)) {
                    status = -EIO;
                }
                while ((status >= 0) && (line.len > 0)
                       && ((line.data[line.len - 1] == '\n')
                           || (line.data[line.len - 1] == '\r'))) {
                    line.len -= 1;
                    line.data[line.len] = '\0';
                }
                if ((status < 0) || !read_line) {
                    break;
                }
                if ((line.len > 0)
                    && (tag_edit_find_recent_pattern(screen,
                                                     line.data, line.len)
                                                      < 0)) {
                    StrBuilder *item;

                    item = str_builder_array_append(&screen->recent_patterns);
                    ASSERT(item != NULL);
                    sb_set(item, line.data, line.len);
                }
                if (ch == EOF) {
                    break;
                }
            }
            if ((fclose(file) == EOF) && (status == 0)) {
                status = errno ? -errno : -EIO;
            }
        }
        sb_free(&line);
        sb_free(&path);
        if (status < 0) {
            return;
        }
    }
    if ((screen->pattern.len <= 0) && (screen->recent_patterns.len > 0)) {
        StrBuilder *pattern = &screen->recent_patterns.items[0];
        tag_edit_set_pattern(screen, pattern->data, pattern->len);
    }
    tag_edit_screen_prepare_parser_rows(screen, mode,
                                        screen->pattern.data,
                                        screen->pattern.len);
    tag_edit_build_parser_legend(screen);
    screen->parser_preview_enabled = false;
    tag_edit_set_focus(screen, TAG_EDIT_FOCUS_PARSER_ACTIONS);
    return;
}

void
tag_edit_screen_show_parser_legend(TagEditScreen *screen) {
    if (screen->parser_mode == TAG_EDIT_PARSER_MODE_NONE) {
        return;
    }
    tag_edit_set_focus(screen, TAG_EDIT_FOCUS_PARSER_LEGEND);
    return;
}

void
tag_edit_screen_show_parser_preview(TagEditScreen *screen) {
    if (screen->parser_mode == TAG_EDIT_PARSER_MODE_NONE) {
        return;
    }
    tag_edit_set_focus(screen, TAG_EDIT_FOCUS_PARSER_PREVIEW);
    return;
}

void
tag_edit_screen_close_parser(TagEditScreen *screen) {
    screen->parser_mode = TAG_EDIT_PARSER_MODE_NONE;
    tag_edit_set_focus(screen, TAG_EDIT_FOCUS_TAG_TYPES);
    return;
}

static bool
tag_edit_next_mask_tag(char *mask, int32 mask_len, int32 start,
                       int32 *percent_pos, char *tag_char) {
    for (int32 i = start; i + 1 < mask_len; i += 1) {
        if (mask[i] == '%') {
            *percent_pos = i;
            *tag_char = mask[i + 1];
            return true;
        }
    }
    return false;
}

int32
tag_edit_parse_filename(MutableSong *song, char *mask, int32 mask_len,
                        bool preview, StrBuilder *preview_buffer) {
    StrBuilder file = {0};
    StrBuilder track_number = {0};
    StrBuilder track_total = {0};
    int32 mask_pos;
    int32 file_pos;
    int32 percent_pos;
    int32 name_len;
    char tag_char;
    bool has_track_number = false;
    bool has_track_total = false;

    if (mask_len < 0) {
        return -EINVAL;
    }
    if (song->name == NULL) {
        return -NCM_ERROR_NOT_FOUND;
    }

    name_len = song->name_len;
    for (int32 i = song->name_len - 1; i >= 0; i -= 1) {
        if (song->name[i] == '.') {
            name_len = i;
            break;
        }
    }
    SB_APPEND(&file, song->name, name_len);

    mask_pos = 0;
    file_pos = 0;
    while (tag_edit_next_mask_tag(mask, mask_len, mask_pos,
                                  &percent_pos, &tag_char)) {
        int32 next_mask_pos;
        int32 next_percent_pos;
        int32 value_end;
        int32 separator_len;
        char next_tag_char;
        enum TagType tag_type;
        bool recognized;

        separator_len = percent_pos - mask_pos;
        if ((separator_len > 0) && (((file_pos + separator_len) > file.len)
                || !STREQUAL(file.data + file_pos, separator_len,
                             mask + mask_pos, separator_len))) {
            sb_free(&track_total);
            sb_free(&track_number);
            sb_free(&file);
            return -NCM_ERROR_PARSE;
        }
        file_pos += separator_len;
        next_mask_pos = percent_pos + 2;
        if (tag_edit_next_mask_tag(mask, mask_len, next_mask_pos,
                                   &next_percent_pos, &next_tag_char)) {
            int32 literal_len = next_percent_pos - next_mask_pos;
            int32 found = -1;

            if (literal_len <= 0) {
                found = file_pos;
            } else {
                for (int32 i = file_pos; i + literal_len <= file.len; i += 1) {
                    if (STREQUAL(file.data + i, literal_len,
                                 mask + next_mask_pos, literal_len)) {
                        found = i;
                        break;
                    }
                }
            }
            if (found < 0) {
                sb_free(&track_total);
                sb_free(&track_number);
                sb_free(&file);
                return -NCM_ERROR_PARSE;
            }
            value_end = found;
        } else {
            value_end = file.len;
        }

        for (int32 i = file_pos; i < value_end; i += 1) {
            if (file.data[i] == '_') {
                file.data[i] = ' ';
            }
        }

        tag_type = ncm_char_to_tag_type(tag_char);
        recognized = tag_type != TAG_COUNT;
        if (tag_char == ncm_song_getter_format_char(
                SONG_GETTER_TRACK_NUMBER)) {
            sb_set(&track_number, file.data + file_pos, value_end - file_pos);
            has_track_number = true;
            recognized = true;
        } else if (tag_char == ncm_song_getter_format_char(
                       SONG_GETTER_TRACK_TOTAL)) {
            sb_set(&track_total, file.data + file_pos, value_end - file_pos);
            has_track_total = true;
            recognized = true;
        } else if (!preview && recognized) {
            mutable_song_set_tags(song, tag_type,
                                  file.data + file_pos,
                                  value_end - file_pos, NULL, 0);
        }

        if (preview && preview_buffer && recognized) {
            sb_append_byte(preview_buffer, '%');
            sb_append_byte(preview_buffer, tag_char);
            SB_APPEND(preview_buffer, ": ");
            SB_APPEND(preview_buffer,
                      file.data + file_pos, value_end - file_pos);
            sb_append_byte(preview_buffer, '\n');
        }
        file_pos = value_end;
        mask_pos = percent_pos + 2;
    }

    if (!preview && has_track_number) {
        StrBuilder track = {0};

        SB_APPEND(&track, track_number.data, track_number.len);
        if (has_track_total && (track_total.len > 0)) {
            sb_append_byte(&track, '/');
            SB_APPEND(&track, track_total.data, track_total.len);
        }
        mutable_song_set_tags(song, TAG_TRACK,
                              track.data, track.len, NULL, 0);
        sb_free(&track);
    } else if (!preview && has_track_total && (track_total.len > 0)) {
        StrBuilder current = {0};
        StrBuilder track = {0};
        int32 slash;
        int32 number_len;

        mutable_song_get_tag_buffer(song, TAG_TRACK, 0, &current);
        slash = ncm_string_find_char(current.data, current.len, '/');
        if (slash >= 0) {
            number_len = slash;
        } else {
            number_len = current.len;
        }
        if (number_len > 0) {
            SB_APPEND(&track, current.data, number_len);
            sb_append_byte(&track, '/');
            SB_APPEND(&track, track_total.data, track_total.len);
            mutable_song_set_tags(song, TAG_TRACK,
                                  track.data, track.len, NULL, 0);
        }
        sb_free(&track);
        sb_free(&current);
    }

    sb_free(&track_total);
    sb_free(&track_number);
    sb_free(&file);
    return 0;
}

int32
tag_edit_generate_filename(MutableSong *song,
                           char *pattern, int32 pattern_len,
                           StrBuilder *filename) {
    NcmFormatAst ast = {0};
    NcmSong format_song = {0};
    StrBuilder rendered = {0};
    NcmError ncm_error;
    int32 status;

    if (pattern_len < 0) {
        return -EINVAL;
    }
    ncm_error_clear(&ncm_error);
    status = ncm_format_parse(&ast, pattern, pattern_len,
                              NCM_FORMAT_FLAG_TAG, &ncm_error);
    if (status < 0) {
        ncm_error_clear(&ncm_error);
        ncm_format_ast_destroy(&ast);
        ncm_song_destroy(&format_song);
        return status;
    }
    {
        StrBuilder uri = {0};

        if (song->uri && (song->uri_len >= 0)) {
            SB_APPEND(&uri, song->uri, song->uri_len);
        } else if (song->directory && (song->directory_len > 0)
                   && song->name && (song->name_len >= 0)) {
            ncm_fs_join(&uri, song->directory, song->directory_len,
                        song->name, song->name_len);
        } else if (song->name && (song->name_len >= 0)) {
            SB_APPEND(&uri, song->name, song->name_len);
        }

        if (uri.data == NULL) {
            ncm_song_set_uri(&format_song, STRLIT(""));
        } else {
            ncm_song_set_uri(&format_song, uri.data, uri.len);
        }
        sb_free(&uri);

        ncm_song_set_duration(&format_song, song->duration);
        ncm_song_set_mtime(&format_song, song->mtime);
        for (int32 i = 0; i < song->tags_len; i += 1) {
            MutableSongTag *tag = &song->tags[i];
            enum TagType type = tag->type;
            char *value;
            int32 value_len;

            if (type == TAG_COUNT) {
                continue;
            }

            if (tag->modified) {
                value = tag->value;
                value_len = tag->value_len;
            } else {
                value = tag->original;
                value_len = tag->original_len;
            }
            if ((value == NULL) || (value_len <= 0)) {
                continue;
            }
            ncm_song_add_tag(&format_song, type, value, value_len);
        }
    }
    rendered = ncm_format_render_string(&ast, &format_song);
    SB_APPEND(filename, rendered.data, rendered.len);
    sb_free(&rendered);
    {
        bool win32_compatible = Config.generate_win32_compatible_filenames;
        ncm_string_remove_invalid_filename_chars(filename->data, &filename->len,
                                                 win32_compatible);
    }
    if (filename->data) {
        filename->data[filename->len] = '\0';
    }
    ncm_format_ast_destroy(&ast);
    ncm_song_destroy(&format_song);
    return 0;
}

int32
tag_edit_song_display_value(MutableSong *song, enum TagType tag_type,
                              StrBuilder *buffer) {
    StrBuilder tag = {0};

    if (tag_type == TAG_COUNT) {
        SB_APPEND(buffer, song->name, song->name_len);
        if (song->new_name && (song->new_name_len > 0)) {
            SB_APPEND(buffer, " -> ");
            SB_APPEND(buffer, song->new_name, song->new_name_len);
        }
        return 0;
    }
    if ((uint32)tag_type >= TAG_COUNT) {
        return -EINVAL;
    }

    mutable_song_get_tag_buffer(song, tag_type, 0, &tag);
    SB_APPEND(buffer, tag.data, tag.len);
    sb_free(&tag);
    return 0;
}

#endif /* NC_TAG_EDIT_C */
