#if !defined(NC_TAG_EDITOR_C)
#define NC_TAG_EDITOR_C

#include "cbase.h"

#include "app_controller.h"
#include "c/ncm_c.h"
#include "global.h"
#include "screens/nc_screens.h"
#include "settings.h"
#include "statusbar.h"
#include "title.h"
#include "ui_state.h"

enum TagEditorParserActionRow {
    TAG_EDITOR_PARSER_ACTION_PATTERN = 0,
    TAG_EDITOR_PARSER_ACTION_PREVIEW = 1,
    TAG_EDITOR_PARSER_ACTION_LEGEND = 2,
    TAG_EDITOR_PARSER_ACTION_PROCEED = 4,
    TAG_EDITOR_PARSER_ACTION_CANCEL = 5,
    TAG_EDITOR_PARSER_ACTION_RECENT_START = 9,
};

#define TAG_EDITOR_PATTERN_HISTORY_MAX 30

#define ENUM_NAME TagEditorTagTypeAction
#define ENUM_PREFIX_ TAG_EDITOR_TAG_TYPE_ACTION_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                            \
    XX(TAG_EDITOR_TAG_TYPE_ACTION_NONE, none)                  \
    XX(TAG_EDITOR_TAG_TYPE_ACTION_FIELD, Field)                \
    XX(TAG_EDITOR_TAG_TYPE_ACTION_NUMBER_TRACKS, Track number) \
    XX(TAG_EDITOR_TAG_TYPE_ACTION_FILENAME, Filename)          \
    XX(TAG_EDITOR_TAG_TYPE_ACTION_CAPITALIZE, Capitalize First Letters) \
    XX(TAG_EDITOR_TAG_TYPE_ACTION_LOWER, lower all letters)    \
    XX(TAG_EDITOR_TAG_TYPE_ACTION_RESET, Reset)                \
    XX(TAG_EDITOR_TAG_TYPE_ACTION_SAVE, Save)
#include "cbase/xenums.c"

typedef struct SaveContext SaveContext;

// callbacks
static NcWindow *tag_editor_active_window(NcScreen *screen);
static void tag_editor_refresh(NcScreen *screen);
static void tag_editor_refresh_window(NcScreen *screen);
static void tag_editor_scroll(NcScreen *screen, enum NcScroll where);
static bool tag_editor_can_run_current(NcScreen *screen);
static int32 tag_editor_run_current(NcScreen *screen);
static void tag_editor_switch_to(NcScreen *screen);
static void tag_editor_resize(NcScreen *screen);
static char *tag_editor_title(NcScreen *screen);
static void tag_editor_update(NcScreen *screen);
static void tag_editor_destroy_callback(NcScreen *screen);
static void tag_editor_mouse_callback(NcScreen *, MEVENT);

static void
tag_editor_append_formatted_color_end(NcBuffer *buffer,
                                      NcFormattedColor *color) {
    nc_buffer_add_formatted_color_end(buffer, buffer->len, color, 0);
    return;
}

static void
tag_editor_append_formatted_color(NcBuffer *buffer, NcFormattedColor *color) {
    nc_buffer_add_formatted_color(buffer, buffer->len, color, 0);
    return;
}

static void
tag_editor_draw_tag(NcMenu *menu, NcWindow *window, void *item,
                    int32 pos, void *user) {
    TagEditorScreen *screen = user;
    NcmMutableSong *song = item;
    NcBuffer buffer = {0};
    NcMenu *tag_types;
    int32 choice;

    (void)menu;
    (void)pos;

    ASSERT(screen != NULL);
    ASSERT(window != NULL);
    ASSERT(song != NULL);

    if (ncm_mutable_song_is_modified(song)) {
        nc_buffer_append_data(&buffer,
                              Config.modified_item_prefix.data,
                              Config.modified_item_prefix.len);
    }

    tag_types = nc_editor_string_menu_base(&screen->tag_types);
    choice = nc_menu_highlight(tag_types);
    if ((choice >= 0) && (choice < 11)) {
        StrBuilder tag;
        enum NcmTagsField field = ncm_song_info_tags[choice].field;

        tag = ncm_mutable_song_tags_buffer(
            song, field, Config.tags_separator, Config.tags_separator_len,
            Config.show_duplicate_tags);
        if (tag.len <= 0) {
            tag_editor_append_formatted_color(&buffer,
                                              &Config.empty_tag_color);
            nc_buffer_append_data(&buffer,
                                  Config.empty_tag_marker, Config.empty_tag_marker_len);
            tag_editor_append_formatted_color_end(
                &buffer, &Config.empty_tag_color);
        } else {
            nc_buffer_append_data(&buffer, tag.data, tag.len);
        }
        sb_free(&tag);
    } else if (choice == 12) {
        nc_buffer_append_data(&buffer, song->name, song->name_len);
        if (song->new_name && (song->new_name_len > 0)) {
            tag_editor_append_formatted_color(&buffer, &Config.color2);
            nc_buffer_append_data(&buffer, STRLIT(" -> "));
            tag_editor_append_formatted_color_end(&buffer, &Config.color2);
            nc_buffer_append_data(&buffer, song->new_name, song->new_name_len);
        }
    }

    {
        NcBufferProperty *properties = nc_buffer_properties(&buffer);
        char *data = nc_buffer_data(&buffer);
        int32 len = buffer.len;
        int32 property_count = ARRAY_LEN(buffer.properties);
        int32 property_index = 0;

        for (int32 i = 0; ; i += 1) {
            while ((property_index < property_count)
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

static bool tag_editor_focus_is_main(enum TagEditorFocus);
static void tag_editor_layout(TagEditorScreen *);

static NcScreenOps tag_editor_callbacks = {
    .active_window = tag_editor_active_window,
    .refresh = tag_editor_refresh,
    .refresh_window = tag_editor_refresh_window,
    .scroll = tag_editor_scroll,
    .can_run_current = tag_editor_can_run_current,
    .run_current = tag_editor_run_current,
    .switch_to = tag_editor_switch_to,
    .resize = tag_editor_resize,
    .title = tag_editor_title,
    .update = tag_editor_update,
    .mouse_button_pressed = tag_editor_mouse_callback,
    .lockable = true,
    .mergable = true,
    .destroy = tag_editor_destroy_callback,
};

typedef struct TagEditorSearchContext {
    TagEditorScreen *screen;
    NcmRegex *regex;
} TagEditorSearchContext;

typedef struct TagSetter {
    enum NcmTagsField field;
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
    TagEditorScreen *screen;
    StrBuilder shared_directory;
    char *music_dir;
    int32 target_count;
    int32 modified_count;
    int32 write_count;
    bool shared_directory_valid;
};

static void
tag_editor_append_string_row(NcEditorStringMenu *menu,
                             char *data, int32 data_len, uint32 flags) {
    StrBuilder string = {0};

    sb_set(&string, data, data_len);
    nc_editor_string_menu_add_with_flags(menu, &string, flags);
    sb_free(&string);
    return;
}

static void
tag_editor_configure_menu(NcMenu *menu) {
    ASSERT(menu != NULL);
    nc_menu_set_selected_prefix(menu, &Config.selected_item_prefix);
    nc_menu_set_selected_suffix(menu, &Config.selected_item_suffix);
    nc_menu_set_cyclic_scrolling(menu, Config.cyclic_scrolling);
    nc_menu_set_centered_cursor(menu, Config.centered_cursor);
    return;
}

static void
tag_editor_draw_directory(NcMenu *menu, NcWindow *window, void *item,
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
tag_editor_directory_matches_regex(StrBuilderPair *pair,
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
tag_editor_directory_filter(NcMenu *menu, void *item, void *user) {
    TagEditorScreen *screen = user;
    StrBuilderPair *pair = item;

    (void)menu;
    if (!screen->directory_filter_enabled) {
        return true;
    }
    return tag_editor_directory_matches_regex(pair,
                                              &screen->directory_filter_regex,
                                              true);
}

static NcMenuDisplayCallbacks
tag_editor_directory_display_callbacks(TagEditorScreen *screen) {
    NcMenuDisplayCallbacks callbacks = {0};

    callbacks.draw = tag_editor_draw_directory;
    callbacks.matches_filter = tag_editor_directory_filter;
    callbacks.user = screen;

    return callbacks;
}

static void
tag_editor_draw_string(NcMenu *menu, NcWindow *window, void *item,
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
tag_editor_tag_type_display_callbacks(TagEditorScreen *screen) {
    NcMenuDisplayCallbacks callbacks = {0};

    callbacks.draw = tag_editor_draw_string;
    callbacks.user = screen;
    return callbacks;
}

static bool
tag_editor_tag_matches_regex(TagEditorScreen *screen,
                             NcmMutableSong *song, NcmRegex *regex) {
    StrBuilder buffer = {0};
    NcMenu *tag_types;
    enum NcmTagsField field;
    int32 choice;
    bool found;

    ASSERT(screen != NULL);
    ASSERT(song != NULL);
    ASSERT(regex != NULL);

    tag_types = nc_editor_string_menu_base(&screen->tag_types);
    choice = nc_menu_highlight(tag_types);
    if ((choice >= 0) && (choice < 11)) {
        field = ncm_song_info_tags[choice].field;
    } else if (choice == 12) {
        field = NCM_TAGS_FIELD_COUNT;
    } else {
        return false;
    }

    tag_editor_song_display_value(song, field, &buffer);
    if (buffer.len <= 0) {
        SB_APPEND(&buffer, Config.empty_tag_marker, Config.empty_tag_marker_len);
    }
    found = ncm_regex_matches(regex, buffer.data, buffer.len);
    sb_free(&buffer);
    return found;
}

static bool
tag_editor_tag_filter(NcMenu *menu, void *item, void *user) {
    TagEditorScreen *screen = user;
    NcmMutableSong *song = item;

    (void)menu;
    if (!screen->tag_filter_enabled) {
        return true;
    }
    return tag_editor_tag_matches_regex(screen, song,
                                        &screen->tag_filter_regex);
}

static NcMenuDisplayCallbacks
tag_editor_tag_display_callbacks(TagEditorScreen *screen) {
    NcMenuDisplayCallbacks callbacks = {0};

    callbacks.draw = tag_editor_draw_tag;
    callbacks.matches_filter = tag_editor_tag_filter;
    callbacks.user = screen;

    return callbacks;
}

static bool
tag_editor_focus_is_parser_helper(enum TagEditorFocus focus) {
    return (focus == TAG_EDITOR_FOCUS_PARSER_LEGEND)
           || (focus == TAG_EDITOR_FOCUS_PARSER_PREVIEW);
}

static void
tag_editor_update_menu_highlights(TagEditorScreen *screen) {
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

    if ((active = tag_editor_screen_active_menu(screen))) {
        nc_menu_set_highlight_prefix(active, &Config.current_item_prefix);
        nc_menu_set_highlight_suffix(active, &Config.current_item_suffix);
    }
    {
        NcBorder dialog_border = Config.window_border_color;
        NcBorder parser_border = Config.window_border_color;
        NcBorder helper_border = Config.window_border_color;

        if (screen->active_focus == TAG_EDITOR_FOCUS_PARSER_CHOICE) {
            dialog_border = Config.active_window_border;
        } else if (screen->active_focus
                   == TAG_EDITOR_FOCUS_PARSER_ACTIONS) {
            parser_border = Config.active_window_border;
        } else if (tag_editor_focus_is_parser_helper(
                       screen->active_focus)) {
            helper_border = Config.active_window_border;
        }

        nc_window_set_border(&screen->parser_dialog_window, dialog_border);
        nc_window_set_border(&screen->parser_window, parser_border);
        nc_window_set_border(&screen->parser_helper_window, helper_border);
    }
    return;
}

static void
tag_editor_configure_menus(TagEditorScreen *screen) {
    NcMenu *directories = nc_editor_pair_menu_base(&screen->directories);
    NcMenu *tag_types = nc_editor_string_menu_base(&screen->tag_types);
    NcMenu *tags = nc_tag_row_menu_base(&screen->tags);
    NcMenu *parser_dialog = nc_editor_string_menu_base(&screen->parser_dialog);
    NcMenu *parser_rows = nc_editor_string_menu_base(&screen->parser_rows);
    NcMenu *parser_actions =
        nc_editor_string_menu_base(&screen->parser_actions);

    tag_editor_configure_menu(directories);
    tag_editor_configure_menu(tag_types);
    tag_editor_configure_menu(tags);
    tag_editor_configure_menu(parser_dialog);
    tag_editor_configure_menu(parser_rows);
    tag_editor_configure_menu(parser_actions);

    nc_menu_set_display_callbacks(
        directories, tag_editor_directory_display_callbacks(screen));
    nc_menu_set_display_callbacks(
        tag_types, tag_editor_tag_type_display_callbacks(screen));
    nc_menu_set_display_callbacks(tags, tag_editor_tag_display_callbacks(
        screen));
    nc_menu_set_display_callbacks(
        parser_dialog, tag_editor_tag_type_display_callbacks(screen));
    nc_menu_set_display_callbacks(
        parser_rows, tag_editor_tag_type_display_callbacks(screen));
    nc_menu_set_display_callbacks(
        parser_actions, tag_editor_tag_type_display_callbacks(screen));

    tag_editor_update_menu_highlights(screen);
    return;
}

static void
tag_editor_update_titles(TagEditorScreen *screen,
                         bool update_windows) {
    ASSERT(screen != NULL);

    screen->last_known_directory_count = nc_menu_item_count(
        nc_editor_pair_menu_base(&screen->directories));
    screen->last_known_tag_count = nc_menu_item_count(
        nc_tag_row_menu_base(&screen->tags));

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
        if (screen->parser_mode == TAG_EDITOR_PARSER_TAGS_FROM_FILENAME) {
            SB_APPEND(&screen->parser_title, "Get tags from filename");
        } else if (screen->parser_mode == TAG_EDITOR_PARSER_RENAME_FILES) {
            SB_APPEND(&screen->parser_title, "Rename files");
        } else {
            SB_APPEND(&screen->parser_title, "Pattern");
        }
        if ((screen->active_focus == TAG_EDITOR_FOCUS_PARSER_LEGEND)
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
    nc_window_set_title(&screen->tags_window, screen->tags_title.data,
                        screen->tags_title.len);
    nc_window_set_title(&screen->parser_dialog_window,
                        screen->parser_dialog_title.data,
                        screen->parser_dialog_title.len);
    nc_window_set_title(&screen->parser_window, screen->parser_title.data,
                        screen->parser_title.len);
    nc_window_set_title(&screen->parser_helper_window,
                        screen->parser_helper_title.data,
                        screen->parser_helper_title.len);

    return;
}

static bool
tag_editor_current_directory_path(TagEditorScreen *screen,
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
tag_editor_observe_current_directory(TagEditorScreen *screen) {
    char *path;
    int32 path_len;
    NcMenu *menu;

    ASSERT(screen != NULL);
    menu = nc_editor_pair_menu_base(&screen->directories);
    screen->last_directory_highlight = nc_menu_highlight(menu);
    if (!tag_editor_current_directory_path(screen, &path, &path_len)) {
        sb_clear(&screen->observed_dir);
        screen->observed_dir_valid = false;
        return;
    }
    sb_set(&screen->observed_dir, path, path_len);
    screen->observed_dir_valid = true;
    return;
}

void
tag_editor_screen_init(TagEditorScreen *screen,
                       int32 start_x, int32 width,
                       int32 main_start_y, int32 main_height,
                       NcColor color, NcBorder border) {
    nc_editor_pair_menu_init(&screen->directories);
    nc_editor_string_menu_init(&screen->tag_types);
    nc_tag_row_menu_init(&screen->tags);

    nc_editor_string_menu_init(&screen->parser_dialog);
    nc_editor_string_menu_init(&screen->parser_rows);
    nc_editor_string_menu_init(&screen->parser_actions);

    screen->hooks = (TagEditorHooks){0};
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

    tag_editor_update_titles(screen, false);

    nc_window_init(&screen->directories_window,
                   start_x, main_start_y, width, main_height,
                   screen->directories_title.data,
                   screen->directories_title.len,
                   color, border);
    nc_window_init(&screen->tag_types_window,
                   start_x, main_start_y, width, main_height,
                   screen->tag_types_title.data,
                   screen->tag_types_title.len,
                   color, border);
    nc_window_init(&screen->tags_window,
                   start_x, main_start_y, width, main_height,
                   screen->tags_title.data,
                   screen->tags_title.len,
                   color, border);
    nc_window_init(&screen->parser_dialog_window,
                   start_x, main_start_y, width, main_height,
                   screen->parser_dialog_title.data,
                   screen->parser_dialog_title.len,
                   color, Config.window_border_color);
    nc_window_init(&screen->parser_window,
                   start_x, main_start_y, width, main_height,
                   screen->parser_title.data,
                   screen->parser_title.len,
                   color, Config.window_border_color);
    nc_window_init(&screen->parser_helper_window,
                   start_x, main_start_y, width, main_height,
                   screen->parser_helper_title.data,
                   screen->parser_helper_title.len,
                   color, Config.window_border_color);

    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    screen->active_column = TAG_EDITOR_COLUMN_DIRECTORIES;
    screen->active_focus = TAG_EDITOR_FOCUS_DIRECTORIES;
    screen->last_directory_highlight = -1;
    screen->last_tag_type_highlight = -1;
    screen->last_known_directory_count = 0;
    screen->last_known_tag_count = 0;
    screen->window_timeout_ms = -1;
    screen->parser_mode = TAG_EDITOR_PARSER_NONE;
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

    tag_editor_screen_set_current_dir(screen, STRLIT("/"));
    {
        NcEditorStringMenu *menu = &screen->tag_types;

        nc_menu_clear_items(nc_editor_string_menu_base(menu));
        for (int32 i = 0; ncm_song_info_tags[i].name; i += 1) {
            int32 name_len = 0;

            while (ncm_song_info_tags[i].name[name_len] != '\0') {
                name_len += 1;
            }
            tag_editor_append_string_row(
                menu, ncm_song_info_tags[i].name, name_len,
                NC_MENU_ITEM_SELECTABLE);
        }
        nc_editor_string_menu_add_separator(menu);
        tag_editor_append_string_row(menu, STRLIT("Filename"),
                                     NC_MENU_ITEM_SELECTABLE);
        nc_editor_string_menu_add_separator(menu);
        if (Config.titles_visibility) {
            tag_editor_append_string_row(menu, STRLIT("Options"),
                                         NC_MENU_ITEM_INACTIVE);
            nc_editor_string_menu_add_separator(menu);
        }
        tag_editor_append_string_row(menu, STRLIT("Capitalize First Letters"),
                                     NC_MENU_ITEM_SELECTABLE);
        tag_editor_append_string_row(menu, STRLIT("lower all letters"),
                                     NC_MENU_ITEM_SELECTABLE);
        nc_editor_string_menu_add_separator(menu);
        tag_editor_append_string_row(menu, STRLIT("Reset"),
                                     NC_MENU_ITEM_SELECTABLE);
        tag_editor_append_string_row(menu, STRLIT("Save"),
                                     NC_MENU_ITEM_SELECTABLE);
    }
    tag_editor_layout(screen);
    tag_editor_configure_menus(screen);
    tag_editor_observe_current_directory(screen);
    nc_screen_init_ops(&screen->screen, tag_editor_callbacks, screen,
                       NC_SCREEN_TYPE_TAG_EDITOR);
    tag_editor_screen_prepare_parser_rows(screen,
                                          TAG_EDITOR_PARSER_NONE, NULL, 0);
    return;
}

void
tag_editor_screen_destroy(TagEditorScreen *screen) {
    app_controller_unregister_screen(tag_editor_screen_base(screen));
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
tag_editor_screen_base(TagEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->screen;
}

void
tag_editor_screen_set_hooks(TagEditorScreen *screen,
                            TagEditorHooks hooks) {
    if (screen == NULL) {
        return;
    }
    screen->hooks = hooks;
    return;
}

NcMenu *
tag_editor_screen_active_menu(TagEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    switch (screen->active_focus) {
    case TAG_EDITOR_FOCUS_DIRECTORIES:
        return nc_editor_pair_menu_base(&screen->directories);
    case TAG_EDITOR_FOCUS_TAG_TYPES:
        return nc_editor_string_menu_base(&screen->tag_types);
    case TAG_EDITOR_FOCUS_TAGS:
        return nc_tag_row_menu_base(&screen->tags);
    case TAG_EDITOR_FOCUS_PARSER_CHOICE:
        return nc_editor_string_menu_base(&screen->parser_dialog);
    case TAG_EDITOR_FOCUS_PARSER_ACTIONS:
        return nc_editor_string_menu_base(&screen->parser_actions);
    case TAG_EDITOR_FOCUS_PARSER_LEGEND:
    case TAG_EDITOR_FOCUS_PARSER_PREVIEW:
        return NULL;
    case TAG_EDITOR_FOCUS_COUNT:
    default:
        break;
    }
    return NULL;
}

NcWindow *
tag_editor_screen_active_window(TagEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    switch (screen->active_focus) {
    case TAG_EDITOR_FOCUS_DIRECTORIES:
        return &screen->directories_window;
    case TAG_EDITOR_FOCUS_TAG_TYPES:
        return &screen->tag_types_window;
    case TAG_EDITOR_FOCUS_TAGS:
        return &screen->tags_window;
    case TAG_EDITOR_FOCUS_PARSER_CHOICE:
        return &screen->parser_dialog_window;
    case TAG_EDITOR_FOCUS_PARSER_ACTIONS:
        return &screen->parser_window;
    case TAG_EDITOR_FOCUS_PARSER_LEGEND:
    case TAG_EDITOR_FOCUS_PARSER_PREVIEW:
        return &screen->parser_helper_window;
    case TAG_EDITOR_FOCUS_COUNT:
    default:
        break;
    }
    return NULL;
}

void
tag_editor_screen_set_geometry(TagEditorScreen *screen,
                               int32 start_x, int32 width,
                               int32 main_start_y, int32 main_height) {
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    tag_editor_layout(screen);
    tag_editor_configure_menus(screen);
    tag_editor_update_titles(screen, true);
    return;
}

void
tag_editor_screen_clear_directories(TagEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
    return;
}

void
tag_editor_screen_clear_stale_tags(TagEditorScreen *screen) {
    nc_menu_clear_items(nc_tag_row_menu_base(&screen->tags));
    sb_clear(&screen->displayed_dir);
    screen->displayed_dir_valid = false;
    screen->tags_update_requested = true;
    screen->last_known_tag_count = 0;
    tag_editor_update_titles(screen, true);
    return;
}

void
tag_editor_screen_finish_directory_change(TagEditorScreen *screen) {
    char *path;
    int32 path_len;
    NcMenu *menu;
    bool changed;

    if (screen->active_focus != TAG_EDITOR_FOCUS_DIRECTORIES) {
        return;
    }

    menu = nc_editor_pair_menu_base(&screen->directories);
    if (!tag_editor_current_directory_path(screen, &path, &path_len)) {
        changed = screen->observed_dir_valid;
        tag_editor_observe_current_directory(screen);
    } else {
        changed = !screen->observed_dir_valid
                  || !STREQUAL(screen->observed_dir.data,
                               screen->observed_dir.len,
                               path, path_len)
                  || (screen->last_directory_highlight
                      != nc_menu_highlight(menu));
        if (changed) {
            tag_editor_observe_current_directory(screen);
        }
    }
    if (changed) {
        tag_editor_screen_clear_stale_tags(screen);
    }
    return;
}

void
tag_editor_screen_set_current_dir(TagEditorScreen *screen,
                                  char *dir, int32 dir_len) {
    bool changed;

    changed = screen->current_dir.data
              && !STREQUAL(screen->current_dir.data, screen->current_dir.len,
                           dir, dir_len);
    sb_set(&screen->current_dir, dir, dir_len);
    screen->directories_update_requested = true;
    if (changed) {
        tag_editor_screen_clear_stale_tags(screen);
    }
    return;
}

int32
tag_editor_screen_current_dir(TagEditorScreen *screen,
                              NcmStringView *view) {
    if (view) {
        *view = (NcmStringView){0};
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
tag_editor_screen_current_directory_path(TagEditorScreen *screen,
                                         NcmStringView *view) {
    char *path;
    int32 path_len;

    *view = (NcmStringView){0};
    if (!tag_editor_current_directory_path(screen, &path, &path_len)) {
        return -NCM_ERROR_NOT_FOUND;
    }
    ncm_string_view_set(view, path, path_len);
    return 0;
}

static void
tag_editor_status_message(TagEditorScreen *screen,
                          char *message, int32 message_len) {
    if (screen->hooks.status_message) {
        screen->hooks.status_message(screen->hooks.user, message,
                                     message_len);
    }
    return;
}

int32
tag_editor_screen_enter_directory(TagEditorScreen *screen) {
    NcmStringView path = {0};
    NcmDirectoryArray directories = {0};
    NcmError ncm_error;
    int32 status;
    bool has_subdirectories;

    if (screen == NULL) {
        return -EINVAL;
    }
    if (screen->active_focus != TAG_EDITOR_FOCUS_DIRECTORIES) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    status = tag_editor_screen_current_directory_path(screen, &path);
    if (status < 0) {
        return status;
    }
    has_subdirectories = false;
    if (path.len > 0) {
        ncm_error_clear(&ncm_error);
        has_subdirectories = (ncm_mpd_client_get_directory_list(
                                  &global_mpd, path.data, &directories,
                                  &ncm_error) == 0)
                             && (directories.len > 0);
        ncm_error_clear(&ncm_error);
        ncm_directory_array_destroy(&directories);
    }
    if (!has_subdirectories) {
        tag_editor_status_message(screen, STRLIT("No subdirectories found"));
        return -NCM_ERROR_NOT_FOUND;
    }
    sb_clear(&screen->highlighted_dir);
    tag_editor_screen_set_current_dir(screen, path.data, path.len);
    nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
    tag_editor_screen_clear_stale_tags(screen);
    screen->directories_update_requested = true;
    screen->observed_dir_valid = false;
    tag_editor_update_titles(screen, true);
    return 0;
}

int32
tag_editor_screen_go_to_parent(TagEditorScreen *screen) {
    StrBuilder parent = {0};
    int32 parent_len;

    if (screen == NULL) {
        return -EINVAL;
    }
    if (screen->active_focus != TAG_EDITOR_FOCUS_DIRECTORIES) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if ((screen->current_dir.data == NULL)
        || (screen->current_dir.len <= 0)
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
    tag_editor_screen_set_current_dir(screen, parent.data, parent.len);
    sb_free(&parent);

    nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
    tag_editor_screen_clear_stale_tags(screen);
    screen->directories_update_requested = true;
    screen->observed_dir_valid = false;
    tag_editor_update_titles(screen, true);
    return 0;
}

static void
tag_editor_restore_current_directory(TagEditorScreen *screen,
                                     StrBuilder *path) {
    NcMenu *menu;

    ASSERT(screen != NULL);
    ASSERT(path != NULL);
    if (path->len <= 0) {
        return;
    }
    menu = nc_editor_pair_menu_base(&screen->directories);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        StrBuilderPair *pair;

        pair = nc_menu_active_item_at(menu, i);
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
tag_editor_reload_directories_from_mpd(TagEditorScreen *screen,
                                       NcmMpdClient *client,
                                       NcmError *ncm_error) {
    NcmDirectoryArray directories = {0};
    StrBuilder preserved = {0};
    char *dir;
    int32 status;

    {
        char *data;
        int32 data_len;

        if (tag_editor_current_directory_path(screen, &data, &data_len)) {
            sb_set(&preserved, data, data_len);
        }
    }
    if ((preserved.len <= 0) && (screen->highlighted_dir.len > 0)) {
        sb_set(&preserved, screen->highlighted_dir.data,
               screen->highlighted_dir.len);
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

                left_start = ncm_string_basename_start(
                    left->path, left->path_len);
                right_start = ncm_string_basename_start(
                    current.path, current.path_len);
                comparison = ncm_compare_locale_strings(
                    left->path + left_start, left->path_len - left_start,
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

    nc_menu_show_all_items(nc_editor_pair_menu_base(
        &screen->directories));
    nc_menu_clear_items(nc_editor_pair_menu_base(
        &screen->directories));
    {
        char *control_dir = screen->current_dir.data;
        int32 control_dir_len = screen->current_dir.len;

        if ((control_dir == NULL) || (control_dir_len <= 0)
            || STREQUAL(control_dir, control_dir_len, "/")) {
            tag_editor_screen_add_directory(screen, STRLIT("."), STRLIT("/"));
        } else {
            int32 parent_len;

            parent_len = ncm_string_parent_directory_len(
                control_dir, control_dir_len);
            if (parent_len <= 0) {
                tag_editor_screen_add_directory(screen, STRLIT(".."),
                                                STRLIT("/"));
            } else {
                tag_editor_screen_add_directory(screen, STRLIT(".."),
                                                control_dir, parent_len);
            }
        }
    }
    for (int32 i = 0; i < directories.len; i += 1) {
        NcmDirectory *directory = &directories.items[i];
        NcmStringView path;
        int32 basename_start;

        if (!ncm_directory_has_path_view(directory, &path)) {
            continue;
        }
        basename_start = ncm_string_basename_start(path.data, path.len);
        tag_editor_screen_add_directory(
            screen, path.data + basename_start,
            path.len - basename_start, path.data, path.len);
    }

    tag_editor_restore_current_directory(screen, &preserved);
    if (screen->directory_filter_enabled) {
        nc_menu_apply_filter(nc_editor_pair_menu_base(
            &screen->directories));
        tag_editor_restore_current_directory(screen, &preserved);
    }
    tag_editor_observe_current_directory(screen);
    sb_clear(&screen->highlighted_dir);
    screen->directories_update_requested = false;

    sb_free(&preserved);
    ncm_directory_array_destroy(&directories);
    return 0;
}

static int32
tag_editor_reload_songs_from_mpd(TagEditorScreen *screen,
                                 NcmMpdClient *client, NcmError *ncm_error) {
    NcmMpdSongList list = {0};
    NcmSongArray songs = {0};
    StrBuilder preserved_uri = {0};
    char *path;
    int32 path_len;
    int32 status;

    if (!tag_editor_current_directory_path(screen, &path, &path_len)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing directory"));
    }

    {
        NcmMutableSong *current;

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
            NcmStringView left_uri;
            NcmStringView right_uri;
            int32 comparison;

            if (!ncm_song_has_uri_view(left, 0, &left_uri)) {
                comparison = ncm_song_has_uri_view(
                    &current, 0, &right_uri) ? -1 : 0;
            } else if (!ncm_song_has_uri_view(
                           &current, 0, &right_uri)) {
                comparison = 1;
            } else {
                comparison = ncm_compare_locale_strings(
                    left_uri.data, left_uri.len,
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

    tag_editor_screen_load_songs(screen, &songs);

    if (screen->tag_filter_enabled) {
        nc_menu_apply_filter(nc_tag_row_menu_base(&screen->tags));
    }
    if (preserved_uri.len > 0) {
        NcMenu *menu = nc_tag_row_menu_base(&screen->tags);

        for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
            NcmMutableSong *item;

            item = nc_menu_active_item_at(menu, i);
            if ((item->uri != NULL)
                && STREQUAL(item->uri, item->uri_len,
                            preserved_uri.data, preserved_uri.len)) {
                nc_menu_goto_selectable(menu, i);
                break;
            }
        }
    }
    screen->tags_update_requested = false;
    tag_editor_update_titles(screen, true);

    sb_free(&preserved_uri);
    ncm_song_array_destroy(&songs);
    ncm_mpd_song_list_destroy(&list);
    return 0;
}

static void
tag_editor_set_focus(TagEditorScreen *screen,
                     enum TagEditorFocus focus) {
    ASSERT(screen != NULL);

    screen->active_focus = focus;

    if (focus == TAG_EDITOR_FOCUS_DIRECTORIES) {
        screen->active_column = TAG_EDITOR_COLUMN_DIRECTORIES;
    } else if (focus == TAG_EDITOR_FOCUS_TAG_TYPES) {
        screen->active_column = TAG_EDITOR_COLUMN_TAG_TYPES;
    } else if (focus == TAG_EDITOR_FOCUS_TAGS) {
        screen->active_column = TAG_EDITOR_COLUMN_TAGS;
    } else if (focus == TAG_EDITOR_FOCUS_PARSER_LEGEND) {
        screen->parser_preview_enabled = false;
    } else if (focus == TAG_EDITOR_FOCUS_PARSER_PREVIEW) {
        screen->parser_preview_enabled = true;
    }

    tag_editor_update_menu_highlights(screen);
    return;
}

int32
tag_editor_screen_locate_song(TagEditorScreen *screen,
                              NcmSong *song) {
    NcmStringView directory;
    NcmStringView uri;
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

    parent_len = ncm_string_parent_directory_len(directory.data,
                                                 directory.len);
    if (parent_len <= 0) {
        sb_set(&parent, STRLIT("/"));
    } else {
        sb_set(&parent, directory.data, parent_len);
    }
    tag_editor_screen_set_current_dir(screen, parent.data, parent.len);
    sb_set(&screen->highlighted_dir, directory.data, directory.len);

    nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
    ncm_error_clear(&ncm_error);
    status = tag_editor_reload_directories_from_mpd(
        screen, &global_mpd, &ncm_error);
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
                tag_editor_observe_current_directory(screen);
                found = true;
                break;
            }
        }
        if (!found) {
            status = -NCM_ERROR_NOT_FOUND;
        }
    }
    if (status == 0) {
        tag_editor_screen_clear_stale_tags(screen);
        ncm_error_clear(&ncm_error);
        status = tag_editor_reload_songs_from_mpd(
            screen, &global_mpd, &ncm_error);
    }
    if (status == 0) {
        NcMenu *menu = nc_tag_row_menu_base(&screen->tags);
        bool found = false;

        nc_menu_reset(nc_editor_string_menu_base(&screen->tag_types));
        tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_TAGS);
        for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
            NcmMutableSong *item;

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
    tag_editor_update_titles(screen, true);
    return status;
}

static bool
tag_editor_current_directory_pair(TagEditorScreen *screen,
                                  StrBuilderPair **pair) {
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
tag_editor_screen_rename_directory_available(TagEditorScreen *screen,
                                             char *music_dir,
                                             int32 music_dir_len) {
    StrBuilderPair *pair;

    if ((screen == NULL) || (music_dir == NULL) || (music_dir_len <= 0)) {
        return false;
    }
    if (screen->active_focus != TAG_EDITOR_FOCUS_DIRECTORIES) {
        return false;
    }
    if (!tag_editor_current_directory_pair(screen, &pair)) {
        return false;
    }
    if (STREQUAL(pair->first.data, pair->first.len, ".")
        || STREQUAL(pair->first.data, pair->first.len, "..")) {
        return false;
    }
    return true;
}

int32
tag_editor_screen_rename_current_directory(TagEditorScreen *screen,
                                           char *music_dir,
                                           int32 music_dir_len) {
    StrBuilderPair *pair;
    NcmStringView initial;
    StrBuilder name = {0};
    StrBuilder old_path = {0};
    StrBuilder new_path = {0};
    StrBuilder new_relative = {0};
    NcmError ncm_error;
    enum TagEditorPromptResult result;
    int32 status;

    if (!tag_editor_screen_rename_directory_available(
        screen, music_dir, music_dir_len)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if ((screen->hooks.prompt == NULL) || !tag_editor_current_directory_pair(
        screen, &pair)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    ncm_string_view_set(&initial, pair->first.data, pair->first.len);
    result = screen->hooks.prompt(
        screen->hooks.user, STRLIT("Directory: "), initial, &name);
    if (result == TAG_EDITOR_PROMPT_ABORTED) {
        sb_free(&name);
        return 0;
    }
    if (result != TAG_EDITOR_PROMPT_ACCEPTED) {
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
    ncm_fs_join(&new_relative, screen->current_dir.data,
                screen->current_dir.len, name.data, name.len);
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
            error_len = strlen32(ncm_error.message);
            SB_APPEND(&message, ncm_error.message, error_len);
        } else {
            SB_APPEND(&message, "unknown error");
        }
        tag_editor_status_message(screen, message.data, message.len);
        sb_free(&message);
    }
    if (status == 0) {
        StrBuilder message = {0};

        SB_APPEND(&message, "Directory renamed to \"");
        SB_APPEND(&message, name.data, name.len);
        SB_APPEND(&message, "\"");
        tag_editor_status_message(screen, message.data, message.len);
        sb_free(&message);
        if (screen->hooks.update_directory) {
            screen->hooks.update_directory(
                screen->hooks.user, screen->current_dir.data,
                screen->current_dir.len);
        }
        sb_set(&screen->highlighted_dir, new_relative.data, new_relative.len);
        screen->directories_update_requested = true;
        tag_editor_update_titles(screen, true);
    }

    sb_free(&new_relative);
    sb_free(&new_path);
    sb_free(&old_path);
    sb_free(&name);
    return status;
}

void
tag_editor_screen_add_directory(TagEditorScreen *screen,
                                char *label, int32 label_len,
                                char *path, int32 path_len) {
    StrBuilderPair pair = {0};

    sb_set(&pair.first, label, label_len);
    sb_set(&pair.second, path, path_len);
    nc_editor_pair_menu_add(&screen->directories, &pair);
    screen->last_known_directory_count = nc_menu_item_count(
        nc_editor_pair_menu_base(&screen->directories));
    tag_editor_update_titles(screen, true);
    sb_free(&pair.second);
    sb_free(&pair.first);
    return;
}

void
tag_editor_screen_load_songs(TagEditorScreen *screen,
                             NcmSongArray *songs) {
    char *path;
    int32 path_len;

    nc_menu_clear_items(nc_tag_row_menu_base(&screen->tags));
    for (int32 i = 0; i < songs->len; i += 1) {
        NcmMutableSong mutable_song = {0};

        ncm_mutable_song_load_originals_from_song(&mutable_song,
                                                  &songs->items[i]);
        tag_editor_screen_add_mutable_song(screen, &mutable_song);
        ncm_mutable_song_destroy(&mutable_song);
    }
    if (tag_editor_current_directory_path(screen, &path, &path_len)) {
        sb_set(&screen->displayed_dir, path, path_len);
        screen->displayed_dir_valid = true;
    } else {
        sb_clear(&screen->displayed_dir);
        screen->displayed_dir_valid = false;
    }
    screen->tags_update_requested = false;
    screen->last_known_tag_count = nc_menu_item_count(
        nc_tag_row_menu_base(&screen->tags));
    tag_editor_update_titles(screen, true);
    return;
}

void
tag_editor_screen_add_mutable_song(TagEditorScreen *screen,
                                   NcmMutableSong *song) {
    nc_tag_row_menu_add(&screen->tags, song);
    screen->last_known_tag_count = nc_menu_item_count(
        nc_tag_row_menu_base(&screen->tags));
    tag_editor_update_titles(screen, true);
    return;
}

static void
tag_editor_copy_selected_song_at(TagEditorScreen *screen,
                                 NcmSongArray *songs, int32 pos) {
    NcmMutableSong *source;
    NcmSong song = {0};

    source = nc_menu_active_item_at(nc_tag_row_menu_base(&screen->tags), pos);
    ASSERT(source != NULL);
    ASSERT(source->uri != NULL);
    ASSERT_POSITIVE(source->uri_len);

    ncm_song_set_uri(&song, source->uri, source->uri_len);
    ncm_song_set_duration(&song, source->duration);
    ncm_song_set_mtime(&song, source->mtime);
    for (int32 i = 0; i < source->tags_len; i += 1) {
        NcmMutableSongTag *tag;
        enum mpd_tag_type type;
        char *value;
        int32 value_len;

        tag = &source->tags[i];
        type = ncm_tags_field_to_tag_type(tag->field);
        value = tag->original;
        value_len = tag->original_len;
        if ((type == MPD_TAG_UNKNOWN) || (value == NULL)
            || (value_len <= 0)) {
            continue;
        }
        ncm_song_add_tag(&song, type, value, value_len);
    }

    ncm_song_array_append_move(songs, &song);
    ncm_song_destroy(&song);
    return;
}

int32
tag_editor_screen_selected_songs(TagEditorScreen *screen,
                                 NcmSongArray *songs) {
    NcMenu *menu;

    if ((screen == NULL) || (songs == NULL)) {
        return -EINVAL;
    }
    ncm_song_array_clear(songs);
    if (screen->active_focus != TAG_EDITOR_FOCUS_TAGS) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    menu = nc_tag_row_menu_base(&screen->tags);
    if (!nc_menu_has_selected(menu)) {
        if (nc_menu_is_empty(menu)) {
            return 0;
        }
        tag_editor_copy_selected_song_at(screen, songs,
                                         nc_menu_highlight(menu));
        return 0;
    }

    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (!nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        tag_editor_copy_selected_song_at(screen, songs, i);
    }
    return 0;
}

bool
tag_editor_screen_previous_column_available(TagEditorScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (screen->active_focus == TAG_EDITOR_FOCUS_TAGS) {
        return !nc_menu_is_empty(
            nc_editor_string_menu_base(&screen->tag_types));
    }
    if (screen->active_focus == TAG_EDITOR_FOCUS_TAG_TYPES) {
        if (nc_menu_is_empty(nc_editor_pair_menu_base(&screen->directories))) {
            return false;
        }
        return true;
    }
    if (tag_editor_focus_is_parser_helper(screen->active_focus)) {
        return !nc_menu_is_empty(nc_editor_string_menu_base(
            &screen->parser_actions));
    }
    return false;
}

bool
tag_editor_screen_next_column_available(TagEditorScreen *screen) {
    NcMenu *tag_types;
    int32 choice;

    if (screen == NULL) {
        return false;
    }
    if (screen->active_focus == TAG_EDITOR_FOCUS_DIRECTORIES) {
        return !nc_menu_is_empty(
                   nc_editor_string_menu_base(&screen->tag_types))
               && !nc_menu_is_empty(nc_tag_row_menu_base(&screen->tags));
    }
    if (screen->active_focus == TAG_EDITOR_FOCUS_TAG_TYPES) {
        tag_types = nc_editor_string_menu_base(&screen->tag_types);
        choice = nc_menu_highlight(tag_types);
        return !nc_menu_is_empty(nc_tag_row_menu_base(&screen->tags))
               && (((choice >= 0) && (choice < 11)) || (choice == 12));
    }
    if (screen->active_focus == TAG_EDITOR_FOCUS_PARSER_ACTIONS) {
        return true;
    }
    return false;
}

static void
tag_editor_refresh_menu(NcWindow *window, NcMenu *menu) {
    ASSERT(window != NULL);
    ASSERT(menu != NULL);
    nc_menu_prepare_refresh(menu, nc_window_height(window), NULL, NULL);
    nc_window_display(window);
    nc_menu_refresh(menu, window, nc_window_width(window),
                    nc_window_height(window));
    return;
}

static void
tag_editor_finish_tag_type_change(TagEditorScreen *screen,
                                  bool refresh_tags) {
    NcMenu *menu;
    int32 highlight;

    ASSERT(screen != NULL);
    if (screen->active_focus != TAG_EDITOR_FOCUS_TAG_TYPES) {
        return;
    }
    menu = nc_editor_string_menu_base(&screen->tag_types);
    highlight = nc_menu_highlight(menu);
    if (screen->last_tag_type_highlight == highlight) {
        return;
    }
    screen->last_tag_type_highlight = highlight;
    if (refresh_tags) {
        tag_editor_refresh_menu(&screen->tags_window,
                                nc_tag_row_menu_base(&screen->tags));
    }
    return;
}

static bool
tag_editor_confirm(TagEditorScreen *screen, char *message,
                   int32 message_len) {
    if (screen->hooks.confirm == NULL) {
        return false;
    }
    return screen->hooks.confirm(screen->hooks.user, message, message_len);
}

void
tag_editor_screen_previous_column(TagEditorScreen *screen) {
    if (!tag_editor_screen_previous_column_available(screen)) {
        return;
    }
    if (screen->active_focus == TAG_EDITOR_FOCUS_TAGS) {
        tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_TAG_TYPES);
    } else if (screen->active_focus == TAG_EDITOR_FOCUS_TAG_TYPES) {
        NcMenu *menu = nc_tag_row_menu_base(&screen->tags);
        bool modified = false;

        for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
            NcmMutableSong *song = nc_menu_active_item_at(menu, i);

            ASSERT(song != NULL);
            if (ncm_mutable_song_is_modified(song)) {
                modified = true;
                break;
            }
        }
        if (modified
            && !tag_editor_confirm(
                screen, STRLIT("There are pending changes, are you sure?"))) {
            return;
        }
        tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_DIRECTORIES);
    } else if (tag_editor_focus_is_parser_helper(screen->active_focus)) {
        tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_PARSER_ACTIONS);
    }
    tag_editor_finish_tag_type_change(screen, false);
    return;
}

static enum TagEditorFocus
tag_editor_current_helper_focus(TagEditorScreen *screen) {
    ASSERT(screen != NULL);
    if (!screen->parser_preview_enabled) {
        return TAG_EDITOR_FOCUS_PARSER_LEGEND;
    }
    return TAG_EDITOR_FOCUS_PARSER_PREVIEW;
}

void
tag_editor_screen_next_column(TagEditorScreen *screen) {
    if (!tag_editor_screen_next_column_available(screen)) {
        return;
    }
    if (screen->active_focus == TAG_EDITOR_FOCUS_DIRECTORIES) {
        tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_TAG_TYPES);
    } else if (screen->active_focus == TAG_EDITOR_FOCUS_TAG_TYPES) {
        tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_TAGS);
    } else if (screen->active_focus
               == TAG_EDITOR_FOCUS_PARSER_ACTIONS) {
        tag_editor_set_focus(screen, tag_editor_current_helper_focus(screen));
    }
    tag_editor_finish_tag_type_change(screen, false);
    return;
}

static int32
tag_editor_for_each_target(TagEditorScreen *screen,
                           int32 (*cb)(NcmMutableSong *song, void *user),
                           void *user) {
    NcMenu *menu;
    bool has_selected;
    int32 count;
    int32 status;

    menu = nc_tag_row_menu_base(&screen->tags);
    has_selected = nc_menu_has_selected(menu);
    count = 0;
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmMutableSong *song;

        if (has_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        song = nc_menu_active_item_at(menu, i);
        ASSERT(song != NULL);
        status = cb(song, user);
        if (status < 0) {
            return status;
        }
        count += 1;
    }
    return count;
}

static int32
tag_editor_set_song_tag_callback(NcmMutableSong *song, void *user) {
    TagSetter *setter = user;

    ncm_mutable_song_set_tags(song, setter->field, setter->value,
                              setter->value_len, setter->separator,
                              setter->separator_len);
    return 0;
}

int32
tag_editor_screen_apply_tag_to_selection(TagEditorScreen *screen,
                                         enum NcmTagsField field,
                                         char *value, int32 value_len,
                                         char *separator,
                                         int32 separator_len) {
    TagSetter setter;

    if ((screen == NULL) || (value == NULL)) {
        return -EINVAL;
    }
    if (field >= NCM_TAGS_FIELD_COUNT) {
        return -EINVAL;
    }
    if ((value_len < 0) || (separator_len < 0)) {
        return -EINVAL;
    }
    setter.field = field;
    setter.value = value;
    setter.value_len = value_len;
    setter.separator = separator;
    setter.separator_len = separator_len;
    return tag_editor_for_each_target(screen, tag_editor_set_song_tag_callback,
                                      &setter);
}

static int32
tag_editor_number_song_callback(NcmMutableSong *song, void *user) {
    TrackNumberer *numberer = user;
    NcmStringView view;
    char buffer[64];
    int32 len;

    if (numberer->extended) {
        len = SNPRINTF(buffer, "%d/%d", numberer->current, numberer->total);
    } else {
        len = SNPRINTF(buffer, "%d", numberer->current);
    }

    numberer->current += 1;
    ncm_mutable_song_set_tag(song, NCM_TAGS_FIELD_TRACK, 0, buffer, len);
    for (int32 i = 1; ncm_mutable_song_has_tag_view(
        song, NCM_TAGS_FIELD_TRACK, i, &view); i += 1) {
        ncm_mutable_song_set_tag(song, NCM_TAGS_FIELD_TRACK, i, STRLIT(""));
    }
    return 0;
}

int32
tag_editor_screen_number_tracks(TagEditorScreen *screen,
                                bool extended) {
    TrackNumberer numberer;
    NcMenu *menu;

    menu = nc_tag_row_menu_base(&screen->tags);
    numberer.current = 1;
    if (nc_menu_has_selected(menu)) {
        numberer.total = nc_menu_selected_count(menu);
    } else {
        numberer.total = nc_menu_item_count(menu);
    }
    numberer.extended = extended;
    return tag_editor_for_each_target(screen, tag_editor_number_song_callback,
                                      &numberer);
}

static int32
tag_editor_capitalize_song_callback(NcmMutableSong *song, void *user) {
    (void)user;

    for (int32 fi = 0; ncm_song_info_tags[fi].name; fi += 1) {
        enum NcmTagsField field = ncm_song_info_tags[fi].field;

        for (int32 i = 0; ; i += 1) {
            NcmStringView view;
            StrBuilder converted = {0};
            int32 converted_len;

            if (!ncm_mutable_song_has_tag_view(song, field, i, &view)) {
                break;
            }

            converted_len = utf8_capitalize_first_letters(
                view.data, view.len, NULL, 0);
            sb_reserve(&converted, converted_len);
            converted.len = utf8_capitalize_first_letters(
                view.data, view.len, converted.data, converted_len);
            if (converted.data) {
                converted.data[converted.len] = '\0';
            }
            ncm_mutable_song_set_tag(song, field, i,
                                     converted.data, converted.len);
            sb_free(&converted);
        }
    }

    return 0;
}

void
tag_editor_screen_capitalize_first_letters(TagEditorScreen *screen) {
    tag_editor_for_each_target(screen,
                               tag_editor_capitalize_song_callback,
                               NULL);
    return;
}

static int32
tag_editor_lower_song_callback(NcmMutableSong *song, void *user) {
    (void)user;
    for (int32 field_idx = 0;
         ncm_song_info_tags[field_idx].name; field_idx += 1) {
        enum NcmTagsField field;

        field = ncm_song_info_tags[field_idx].field;
        for (int32 i = 0; ; i += 1) {
            NcmStringView view;
            StrBuilder buffer = {0};

            if (!ncm_mutable_song_has_tag_view(song, field, i, &view)) {
                break;
            }
            SB_APPEND(&buffer, view.data, view.len);
            if (buffer.data != NULL) {
                ncm_string_lowercase_ascii(buffer.data, buffer.len);
            }
            ncm_mutable_song_set_tag(song, field, i,
                                     buffer.data, buffer.len);
            sb_free(&buffer);
        }
    }
    return 0;
}

void
tag_editor_screen_lower_all_letters(TagEditorScreen *screen) {
    tag_editor_for_each_target(screen, tag_editor_lower_song_callback, NULL);
    return;
}

void
tag_editor_screen_clear_modifications(TagEditorScreen *screen) {
    NcMenu *menu;

    menu = nc_tag_row_menu_base(&screen->tags);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmMutableSong *song;

        song = nc_menu_active_item_at(menu, i);
        ASSERT(song != NULL);
        ncm_mutable_song_clear_modifications(song);
    }
    return;
}

static int32
tag_editor_save_song_callback(NcmMutableSong *song, void *user) {
    SaveContext *context = user;
    int32 status;
    int32 error_code;

    context->target_count += 1;
    {
        char *directory = song->directory;
        int32 directory_len = song->directory_len;

        if ((directory == NULL) && song->uri) {
            directory = song->uri;
            directory_len = ncm_string_parent_directory_len(
                song->uri, song->uri_len);
        }
        if (directory == NULL) {
            directory = "";
            directory_len = 0;
        }

        if (!context->shared_directory_valid) {
            sb_set(&context->shared_directory, directory, directory_len);
            context->shared_directory_valid = true;
        } else if (!STREQUAL(
                       context->shared_directory.data,
                       context->shared_directory.len,
                       directory, directory_len)) {
            StrBuilder shared = {0};

            shared = ncm_string_shared_directory(
                context->shared_directory.data,
                context->shared_directory.len, directory, directory_len);
            sb_free(&context->shared_directory);
            context->shared_directory = shared;
        }
    }
    if (!ncm_mutable_song_is_modified(song)) {
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
        tag_editor_status_message(context->screen,
                                  message.data, message.len);
        sb_free(&message);
    }

    status = ncm_mutable_song_write(song, context->music_dir);
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
        tag_editor_status_message(context->screen,
                                  message.data, message.len);
        sb_free(&message);
        return status;
    }

    context->write_count += 1;
    ncm_mutable_song_clear_modifications(song);
    return 0;
}

int32
tag_editor_screen_save_modified(TagEditorScreen *screen,
                                char *music_dir) {
    SaveContext context = {0};
    int32 status;

    if (screen == NULL) {
        return -EINVAL;
    }

    tag_editor_status_message(screen, STRLIT("Writing changes..."));

    context.screen = screen;
    context.music_dir = music_dir;

    status = tag_editor_for_each_target(
        screen, tag_editor_save_song_callback, &context);
    if (status < 0) {
        sb_free(&context.shared_directory);
        tag_editor_screen_clear_stale_tags(screen);
        return status;
    }

    tag_editor_status_message(screen, STRLIT("Tags updated"));
    nc_menu_reset(nc_editor_string_menu_base(&screen->tag_types));
    tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_DIRECTORIES);
    if (context.shared_directory_valid
        && (screen->hooks.update_directory != NULL)) {
        sb_reserve(&context.shared_directory, 1);
        context.shared_directory.data[context.shared_directory.len] = '\0';
        screen->hooks.update_directory(
            screen->hooks.user, context.shared_directory.data,
            context.shared_directory.len);
    }
    sb_free(&context.shared_directory);
    return context.target_count;
}

bool
tag_editor_screen_save_action_available(TagEditorScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->active_focus == TAG_EDITOR_FOCUS_TAG_TYPES;
}

static int32
tag_editor_compile_constraint(NcmRegex *regex, char *pattern,
                               int32 pattern_len, uint32 regex_flags,
                               NcmError *ncm_error) {
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
tag_editor_screen_apply_directory_filter(TagEditorScreen *screen,
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
        nc_menu_show_all_items(nc_editor_pair_menu_base(
            &screen->directories));
        tag_editor_update_titles(screen, true);
        return ncm_error_ok(ncm_error);
    }
    if ((status = tag_editor_compile_constraint(
        &screen->directory_filter_regex, pattern, pattern_len,
        regex_flags, ncm_error)) < 0) {
        return status;
    }
    sb_set(&screen->directory_filter_constraint, pattern, pattern_len);
    nc_menu_set_display_callbacks(
        nc_editor_pair_menu_base(&screen->directories),
        tag_editor_directory_display_callbacks(screen));
    screen->directory_filter_enabled = true;
    nc_menu_apply_filter(nc_editor_pair_menu_base(&screen->directories));
    tag_editor_update_titles(screen, true);
    return ncm_error_ok(ncm_error);
}

int32
tag_editor_screen_apply_tag_filter(TagEditorScreen *screen,
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
        tag_editor_update_titles(screen, true);
        return ncm_error_ok(ncm_error);
    }
    if ((status = tag_editor_compile_constraint(
        &screen->tag_filter_regex, pattern, pattern_len,
        regex_flags, ncm_error)) < 0) {
        return status;
    }
    sb_set(&screen->tag_filter_constraint, pattern, pattern_len);
    nc_menu_set_display_callbacks(nc_tag_row_menu_base(&screen->tags),
                                  tag_editor_tag_display_callbacks(screen));
    screen->tag_filter_enabled = true;
    nc_menu_apply_filter(nc_tag_row_menu_base(&screen->tags));
    tag_editor_update_titles(screen, true);
    return ncm_error_ok(ncm_error);
}

static bool
tag_editor_search_position(NcMenu *menu, int32 pos, void *user) {
    TagEditorSearchContext *context = user;
    TagEditorScreen *screen = context->screen;

    if (screen->active_focus == TAG_EDITOR_FOCUS_TAGS) {
        return tag_editor_tag_matches_regex(
            screen, nc_menu_active_item_at(menu, pos), context->regex);
    }
    if (screen->active_focus == TAG_EDITOR_FOCUS_DIRECTORIES) {
        return tag_editor_directory_matches_regex(
            nc_menu_active_item_at(menu, pos), context->regex, false);
    }
    return false;
}

int32
tag_editor_screen_search(TagEditorScreen *screen,
                         char *pattern, int32 pattern_len,
                         bool forward, bool wrap,
                         bool skip_current, NcmError *ncm_error) {
    TagEditorSearchContext context;
    NcmRegex *regex;
    StrBuilder *constraint;
    bool *enabled;
    NcMenu *menu;
    NcWindow *window;
    bool found;
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing tag editor"));
    }
    if ((pattern == NULL) || (pattern_len <= 0)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing search pattern"));
    }
    if ((screen->active_focus != TAG_EDITOR_FOCUS_DIRECTORIES)
        && (screen->active_focus != TAG_EDITOR_FOCUS_TAGS)) {
        return ncm_error_set_code(ncm_error, NCM_ERROR_UNAVAILABLE,
                                  STRLIT("tag editor cannot search"));
    }

    if (screen->active_focus == TAG_EDITOR_FOCUS_TAGS) {
        regex = &screen->tag_search_regex;
        constraint = &screen->tag_search_constraint;
        enabled = &screen->tag_search_enabled;
    } else {
        regex = &screen->directory_search_regex;
        constraint = &screen->directory_search_constraint;
        enabled = &screen->directory_search_enabled;
    }
    if ((status = tag_editor_compile_constraint(
        regex, pattern, pattern_len, Config.regular_expressions,
        ncm_error)) < 0) {
        return status;
    }
    sb_set(constraint, pattern, pattern_len);
    *enabled = true;

    menu = tag_editor_screen_active_menu(screen);
    window = tag_editor_screen_active_window(screen);
    context.screen = screen;
    context.regex = regex;
    found = nc_menu_search_selectable(menu, nc_window_height(window),
                                      forward, wrap, skip_current,
                                      tag_editor_search_position,
                                      &context, NULL) == 0;
    if (found) {
        tag_editor_screen_finish_directory_change(screen);
        return 1;
    }
    return 0;
}

static void
tag_editor_reset_parser_navigation(TagEditorScreen *screen) {
    ASSERT(screen != NULL);
    nc_menu_reset(nc_editor_string_menu_base(&screen->parser_dialog));
    nc_menu_reset(nc_editor_string_menu_base(&screen->parser_rows));
    nc_menu_reset(nc_editor_string_menu_base(&screen->parser_actions));
    return;
}

static void
tag_editor_append_parser_separator(TagEditorScreen *screen) {
    nc_editor_string_menu_add_separator(&screen->parser_rows);
    nc_editor_string_menu_add_separator(&screen->parser_actions);
    return;
}

static void
tag_editor_append_parser_row(NcEditorStringMenu *menu, char *data,
                             int32 data_len, uint32 flags) {
    StrBuilder string = {0};

    sb_set(&string, data, data_len);
    nc_editor_string_menu_add_with_flags(menu, &string, flags);
    sb_free(&string);
    return;
}

static void
tag_editor_append_parser_action_row(TagEditorScreen *screen,
                                    char *data, int32 data_len,
                                    uint32 flags) {
    tag_editor_append_parser_row(&screen->parser_rows, data, data_len, flags);
    tag_editor_append_parser_row(&screen->parser_actions, data, data_len,
                                 flags);
    return;
}

static void
tag_editor_append_parser_action_label(TagEditorScreen *screen,
                                      char *label, int32 label_len) {
    tag_editor_append_parser_action_row(screen, label, label_len,
                                        NC_MENU_ITEM_SELECTABLE);
    return;
}

static void
tag_editor_set_pattern(TagEditorScreen *screen,
                       char *pattern, int32 pattern_len) {
    ASSERT(screen != NULL);
    sb_set(&screen->pattern, pattern, pattern_len);
    return;
}

static void
tag_editor_build_parser_legend(TagEditorScreen *screen) {
    NcMenu *tags;
    int32 count;

    sb_clear(&screen->parser_legend);

    SB_APPEND(&screen->parser_legend, "%a - artist\n");
    SB_APPEND(&screen->parser_legend, "%A - album artist\n");
    SB_APPEND(&screen->parser_legend, "%t - title\n");
    SB_APPEND(&screen->parser_legend, "%b - album\n");
    SB_APPEND(&screen->parser_legend, "%y - date\n");
    SB_APPEND(&screen->parser_legend, "%n - track number\n");
    SB_APPEND(&screen->parser_legend, "%g - genre\n");
    SB_APPEND(&screen->parser_legend, "%c - composer\n");
    SB_APPEND(&screen->parser_legend, "%p - performer\n");
    SB_APPEND(&screen->parser_legend, "%d - disc\n");
    SB_APPEND(&screen->parser_legend, "%C - comment\n\nFiles:\n");

    tags = nc_tag_row_menu_base(&screen->tags);
    count = nc_menu_item_count(tags);
    for (int32 i = 0; i < count; i += 1) {
        NcmMutableSong *song;

        song = nc_menu_active_item_at(tags, i);
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

void
tag_editor_screen_prepare_parser_rows(TagEditorScreen *screen,
                                      enum TagEditorParserMode mode,
                                      char *pattern, int32 pattern_len) {
    screen->parser_mode = mode;
    if (pattern) {
        tag_editor_set_pattern(screen, pattern, pattern_len);
    } else if ((mode != TAG_EDITOR_PARSER_NONE)
               && (screen->pattern.len <= 0)
               && Config.default_tag_editor_pattern) {
        tag_editor_set_pattern(
            screen, Config.default_tag_editor_pattern,
            Config.default_tag_editor_pattern_len);
    }

    nc_menu_clear_items(nc_editor_string_menu_base(&screen->parser_dialog));
    nc_menu_clear_items(nc_editor_string_menu_base(&screen->parser_rows));
    nc_menu_clear_items(nc_editor_string_menu_base(&screen->parser_actions));
    tag_editor_append_parser_row(&screen->parser_dialog,
                                 STRLIT("Get tags from filename"),
                                 NC_MENU_ITEM_SELECTABLE);
    tag_editor_append_parser_row(&screen->parser_dialog,
                                 STRLIT("Rename files"),
                                 NC_MENU_ITEM_SELECTABLE);
    tag_editor_append_parser_row(&screen->parser_dialog, STRLIT("Cancel"),
                                 NC_MENU_ITEM_SELECTABLE);
    if (mode == TAG_EDITOR_PARSER_NONE) {
        tag_editor_reset_parser_navigation(screen);
        return;
    }
    tag_editor_append_parser_row(&screen->parser_rows,
                                 STRLIT("Get tags from filename"),
                                 NC_MENU_ITEM_SELECTABLE);
    tag_editor_append_parser_row(&screen->parser_rows,
                                 STRLIT("Rename files"),
                                 NC_MENU_ITEM_SELECTABLE);
    tag_editor_append_parser_row(&screen->parser_rows, STRLIT("Cancel"),
                                 NC_MENU_ITEM_SELECTABLE);
    {
        StrBuilder row = {0};

        SB_APPEND(&row, "Pattern: ");
        SB_APPEND(&row, screen->pattern.data, screen->pattern.len);
        tag_editor_append_parser_action_label(screen, row.data, row.len);
        sb_free(&row);
    }
    tag_editor_append_parser_action_label(screen, STRLIT("Preview"));
    tag_editor_append_parser_action_label(screen, STRLIT("Legend"));
    tag_editor_append_parser_separator(screen);
    tag_editor_append_parser_action_label(screen, STRLIT("Proceed"));
    tag_editor_append_parser_action_label(screen, STRLIT("Cancel"));
    if (screen->recent_patterns.len > 0) {
        tag_editor_append_parser_separator(screen);
        tag_editor_append_parser_action_row(
            screen, STRLIT("Recent patterns"), NC_MENU_ITEM_INACTIVE);
        tag_editor_append_parser_separator(screen);
        for (int32 i = 0; i < screen->recent_patterns.len; i += 1) {
            StrBuilder *recent_pattern;

            recent_pattern = &screen->recent_patterns.items[i];
            tag_editor_append_parser_action_label(
                screen, recent_pattern->data, recent_pattern->len);
        }
    }
    tag_editor_build_parser_legend(screen);
    tag_editor_reset_parser_navigation(screen);
    return;
}

void
tag_editor_screen_show_parser_dialog(TagEditorScreen *screen) {
    if (nc_menu_is_empty(nc_editor_string_menu_base(&screen->parser_dialog))) {
        tag_editor_screen_prepare_parser_rows(screen,
                                              TAG_EDITOR_PARSER_NONE,
                                              NULL, 0);
    }
    screen->parser_mode = TAG_EDITOR_PARSER_NONE;
    tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_PARSER_CHOICE);
    return;
}

static int32
tag_editor_find_recent_pattern(TagEditorScreen *screen,
                               char *pattern, int32 pattern_len) {
    if (pattern_len <= 0) {
        return -1;
    }
    for (int32 i = 0; i < screen->recent_patterns.len; i += 1) {
        StrBuilder *item;

        item = &screen->recent_patterns.items[i];
        if (STREQUAL(item->data, item->len, pattern, pattern_len)) {
            return i;
        }
    }
    return -1;
}

static void
tag_editor_history_path(StrBuilder *path) {
    ASSERT(path != NULL);
    if (Config.ncmpcpp_directory
        && (Config.ncmpcpp_directory_len > 0)) {
        ncm_fs_join(path, Config.ncmpcpp_directory,
                    Config.ncmpcpp_directory_len, STRLIT("patterns.list"));
        return;
    }
    sb_set(path, STRLIT("patterns.list"));
    return;
}

void
tag_editor_screen_show_parser_actions(TagEditorScreen *screen,
                                      enum TagEditorParserMode mode) {
    if (mode == TAG_EDITOR_PARSER_NONE) {
        return;
    }
    if (!screen->recent_patterns_loaded) {
        StrBuilder path = {0};
        StrBuilder line = {0};
        FILE *file;
        int32 status;

        screen->recent_patterns_loaded = true;
        tag_editor_history_path(&path);
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
                    && (tag_editor_find_recent_pattern(
                        screen, line.data, line.len) < 0)) {
                    StrBuilder *item;

                    item = str_builder_array_append(&screen->recent_patterns);
                    ASSERT(item != NULL);
                    sb_set(item, line.data, line.len);
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
        StrBuilder *pattern;

        pattern = &screen->recent_patterns.items[0];
        tag_editor_set_pattern(screen, pattern->data, pattern->len);
    }
    tag_editor_screen_prepare_parser_rows(screen, mode,
                                          screen->pattern.data,
                                          screen->pattern.len);
    tag_editor_build_parser_legend(screen);
    screen->parser_preview_enabled = false;
    tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_PARSER_ACTIONS);
    return;
}

void
tag_editor_screen_show_parser_legend(TagEditorScreen *screen) {
    if (screen->parser_mode == TAG_EDITOR_PARSER_NONE) {
        return;
    }
    tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_PARSER_LEGEND);
    return;
}

void
tag_editor_screen_show_parser_preview(TagEditorScreen *screen) {
    if (screen->parser_mode == TAG_EDITOR_PARSER_NONE) {
        return;
    }
    tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_PARSER_PREVIEW);
    return;
}

void
tag_editor_screen_close_parser(TagEditorScreen *screen) {
    screen->parser_mode = TAG_EDITOR_PARSER_NONE;
    tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_TAG_TYPES);
    return;
}

static bool
tag_editor_next_mask_tag(char *mask, int32 mask_len, int32 start,
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
tag_editor_parse_filename(NcmMutableSong *song, char *mask,
                          int32 mask_len, bool preview,
                          StrBuilder *preview_buffer) {
    StrBuilder file = {0};
    int32 mask_pos;
    int32 file_pos;
    int32 percent_pos;
    int32 name_len;
    char tag_char;

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
    while (tag_editor_next_mask_tag(mask, mask_len, mask_pos,
                                    &percent_pos, &tag_char)) {
        int32 next_mask_pos;
        int32 next_percent_pos;
        int32 value_end;
        int32 separator_len;
        char next_tag_char;
        enum NcmTagsField field;

        separator_len = percent_pos - mask_pos;
        if ((separator_len > 0)
            && (((file_pos + separator_len) > file.len)
                || !STREQUAL(file.data + file_pos, separator_len,
                             mask + mask_pos, separator_len))) {
            sb_free(&file);
            return -NCM_ERROR_PARSE;
        }
        file_pos += separator_len;
        next_mask_pos = percent_pos + 2;
        if (tag_editor_next_mask_tag(mask, mask_len, next_mask_pos,
                                     &next_percent_pos, &next_tag_char)) {
            int32 literal_len;
            int32 found;

            literal_len = next_percent_pos - next_mask_pos;
            found = -1;
            if (literal_len <= 0) {
                found = file_pos;
            } else {
                for (int32 i = file_pos; i + literal_len <= file.len;
                     i += 1) {
                    if (STREQUAL(file.data + i, literal_len,
                                 mask + next_mask_pos,
                                 literal_len)) {
                        found = i;
                        break;
                    }
                }
            }
            if (found < 0) {
                sb_free(&file);
                return -NCM_ERROR_PARSE;
            }
            value_end = found;
        } else {
            value_end = file.len;
        }

        field = ncm_tags_field_from_char(tag_char);
        if (field != NCM_TAGS_FIELD_COUNT) {
            for (int32 i = file_pos; i < value_end; i += 1) {
                if (file.data[i] == '_') {
                    file.data[i] = ' ';
                }
            }
            if (preview && preview_buffer) {
                sb_append_byte(preview_buffer, '%');
                sb_append_byte(preview_buffer, tag_char);
                SB_APPEND(preview_buffer, ": ");
                SB_APPEND(preview_buffer,
                          file.data + file_pos, value_end - file_pos);
                sb_append_byte(preview_buffer, '\n');
            } else {
                ncm_mutable_song_set_tags(
                    song, field, file.data + file_pos, value_end - file_pos,
                    NULL, 0);
            }
        }
        file_pos = value_end;
        mask_pos = percent_pos + 2;
    }
    sb_free(&file);
    return 0;
}

int32
tag_editor_generate_filename(NcmMutableSong *song, char *pattern,
                             int32 pattern_len,
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
            NcmMutableSongTag *tag = &song->tags[i];
            enum mpd_tag_type type = ncm_tags_field_to_tag_type(tag->field);
            char *value;
            int32 value_len;

            if (tag->modified) {
                value = tag->value;
                value_len = tag->value_len;
            } else {
                value = tag->original;
                value_len = tag->original_len;
            }
            if ((type == MPD_TAG_UNKNOWN) || (value == NULL)
                || (value_len <= 0)) {
                continue;
            }
            ncm_song_add_tag(&format_song, type, value, value_len);
        }
    }
    rendered = ncm_format_render_string(&ast, &format_song);
    SB_APPEND(filename, rendered.data, rendered.len);
    sb_free(&rendered);
    ncm_string_remove_invalid_filename_chars(
        filename->data, &filename->len,
        Config.generate_win32_compatible_filenames);
    if (filename->data) {
        filename->data[filename->len] = '\0';
    }
    ncm_format_ast_destroy(&ast);
    ncm_song_destroy(&format_song);
    return 0;
}

int32
tag_editor_song_display_value(NcmMutableSong *song,
                              enum NcmTagsField field,
                              StrBuilder *buffer) {
    StrBuilder tag = {0};

    if (field == NCM_TAGS_FIELD_COUNT) {
        SB_APPEND(buffer, song->name, song->name_len);
        if (song->new_name && (song->new_name_len > 0)) {
            SB_APPEND(buffer, " -> ");
            SB_APPEND(buffer, song->new_name, song->new_name_len);
        }
        return 0;
    }
    if (field >= NCM_TAGS_FIELD_COUNT) {
        return -EINVAL;
    }

    ncm_mutable_song_get_tag_buffer(song, field, 0, &tag);
    SB_APPEND(buffer, tag.data, tag.len);
    sb_free(&tag);
    return 0;
}

static TagEditorScreen *
tag_editor_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcWindow *
tag_editor_active_window(NcScreen *screen) {
    TagEditorScreen *editor = tag_editor_from_screen(screen);

    return tag_editor_screen_active_window(editor);
}

static void
tag_editor_refresh_active_helper(TagEditorScreen *screen) {
    StrBuilder *buffer;

    nc_window_display(&screen->parser_helper_window);
    if (screen->active_focus == TAG_EDITOR_FOCUS_PARSER_PREVIEW) {
        buffer = &screen->parser_preview;
    } else {
        buffer = &screen->parser_legend;
    }
    if (buffer->data && (buffer->len > 0)) {
        nc_window_print_data(&screen->parser_helper_window,
                             buffer->data, buffer->len);
    }
    return;
}

static int32
tag_editor_separator_width(TagEditorScreen *screen) {
    ASSERT(screen != NULL);
    if (screen->width >= 5) {
        return 1;
    }
    return 0;
}

static void
tag_editor_refresh(NcScreen *screen) {
    TagEditorScreen *editor = tag_editor_from_screen(screen);

    tag_editor_update_titles(editor, true);
    tag_editor_update_menu_highlights(editor);
    if (editor->active_focus == TAG_EDITOR_FOCUS_PARSER_CHOICE) {
        tag_editor_refresh_menu(&editor->parser_dialog_window,
                                nc_editor_string_menu_base(
                                    &editor->parser_dialog));
        return;
    }
    if ((editor->active_focus == TAG_EDITOR_FOCUS_PARSER_ACTIONS)
        || tag_editor_focus_is_parser_helper(editor->active_focus)) {
        tag_editor_refresh_menu(&editor->parser_window,
                                nc_editor_string_menu_base(
                                    &editor->parser_actions));
        tag_editor_refresh_active_helper(editor);
        return;
    }

    tag_editor_refresh_menu(&editor->directories_window,
                            nc_editor_pair_menu_base(
                                &editor->directories));
    if (tag_editor_separator_width(editor) > 0) {
        nc_screen_draw_vertical_separator(editor->middle_start_x - 1);
        nc_screen_draw_vertical_separator(editor->right_start_x - 1);
    }
    tag_editor_refresh_menu(&editor->tag_types_window,
                            nc_editor_string_menu_base(
                                &editor->tag_types));
    tag_editor_refresh_menu(&editor->tags_window,
                            nc_tag_row_menu_base(&editor->tags));
    return;
}

static void
tag_editor_refresh_window(NcScreen *screen) {
    TagEditorScreen *editor = tag_editor_from_screen(screen);

    tag_editor_update_titles(editor, true);
    tag_editor_update_menu_highlights(editor);
    if (tag_editor_focus_is_parser_helper(editor->active_focus)) {
        tag_editor_refresh_active_helper(editor);
        return;
    }

    {
        NcMenu *menu = tag_editor_screen_active_menu(editor);
        NcWindow *window = tag_editor_screen_active_window(editor);
        tag_editor_refresh_menu(window, menu);
    }
    return;
}

static void
tag_editor_scroll(NcScreen *screen, enum NcScroll where) {
    TagEditorScreen *editor = tag_editor_from_screen(screen);
    NcMenu *menu;
    NcWindow *window;

    menu = tag_editor_screen_active_menu(editor);
    window = tag_editor_screen_active_window(editor);
    if (menu) {
        nc_menu_scroll_selectable(menu, nc_window_height(window), where);
    } else if (window) {
        nc_window_scroll(window, where);
    }
    tag_editor_screen_finish_directory_change(editor);
    tag_editor_finish_tag_type_change(editor, true);
    tag_editor_update_menu_highlights(editor);
    return;
}

static enum TagEditorTagTypeAction
tag_editor_current_tag_type_action(TagEditorScreen *screen,
                                   enum NcmTagsField *field) {
    NcMenu *menu;
    StrBuilder *row;
    int32 choice;

    ASSERT(screen != NULL);
    ASSERT(field != NULL);

    *field = NCM_TAGS_FIELD_COUNT;
    menu = nc_editor_string_menu_base(&screen->tag_types);
    choice = nc_menu_highlight(menu);
    if (((row = nc_menu_current_item(menu)) == NULL)
        || !nc_menu_current_is_selectable(menu)) {
        return TAG_EDITOR_TAG_TYPE_ACTION_NONE;
    }

    if ((choice >= 0) && (choice < 11)) {
        *field = ncm_song_info_tags[choice].field;
        if ((ncm_song_info_tags[choice].field == NCM_TAGS_FIELD_TRACK)
            && (screen->active_focus
                == TAG_EDITOR_FOCUS_TAG_TYPES)) {
            return TAG_EDITOR_TAG_TYPE_ACTION_NUMBER_TRACKS;
        }
        return TAG_EDITOR_TAG_TYPE_ACTION_FIELD;
    }
    if (STREQUAL(row->data, row->len, "Filename")) {
        return TAG_EDITOR_TAG_TYPE_ACTION_FILENAME;
    }
    if (STREQUAL(row->data, row->len, "Capitalize First Letters")) {
        return TAG_EDITOR_TAG_TYPE_ACTION_CAPITALIZE;
    }
    if (STREQUAL(row->data, row->len, "lower all letters")) {
        return TAG_EDITOR_TAG_TYPE_ACTION_LOWER;
    }
    if (STREQUAL(row->data, row->len, "Reset")) {
        return TAG_EDITOR_TAG_TYPE_ACTION_RESET;
    }
    if (STREQUAL(row->data, row->len, "Save")) {
        return TAG_EDITOR_TAG_TYPE_ACTION_SAVE;
    }
    return TAG_EDITOR_TAG_TYPE_ACTION_NONE;
}

static bool
tag_editor_can_run_current(NcScreen *screen) {
    TagEditorScreen *editor = tag_editor_from_screen(screen);
    NcMenu *menu;
    enum NcmTagsField field;

    switch (editor->active_focus) {
    case TAG_EDITOR_FOCUS_DIRECTORIES:
    case TAG_EDITOR_FOCUS_PARSER_CHOICE:
        menu = tag_editor_screen_active_menu(editor);
        ASSERT(menu != NULL);
        return nc_menu_current_is_selectable(menu);
    case TAG_EDITOR_FOCUS_PARSER_ACTIONS:
        menu = tag_editor_screen_active_menu(editor);
        ASSERT(menu != NULL);
        if (!nc_menu_current_is_selectable(menu)) {
            return false;
        }
        switch (nc_menu_highlight(menu)) {
        case TAG_EDITOR_PARSER_ACTION_PATTERN:
        case TAG_EDITOR_PARSER_ACTION_PREVIEW:
        case TAG_EDITOR_PARSER_ACTION_LEGEND:
        case TAG_EDITOR_PARSER_ACTION_PROCEED:
        case TAG_EDITOR_PARSER_ACTION_CANCEL:
            return true;
        default:
            return nc_menu_highlight(menu)
                   >= (int32)TAG_EDITOR_PARSER_ACTION_RECENT_START;
        }
    case TAG_EDITOR_FOCUS_TAG_TYPES:
        if (nc_menu_is_empty(nc_tag_row_menu_base(&editor->tags))) {
            return false;
        }
        return tag_editor_current_tag_type_action(editor, &field)
               != TAG_EDITOR_TAG_TYPE_ACTION_NONE;
    case TAG_EDITOR_FOCUS_TAGS:
        if (nc_menu_is_empty(nc_tag_row_menu_base(&editor->tags))) {
            return false;
        }
        switch (tag_editor_current_tag_type_action(editor, &field)) {
        case TAG_EDITOR_TAG_TYPE_ACTION_FIELD:
        case TAG_EDITOR_TAG_TYPE_ACTION_FILENAME:
            return true;
        case TAG_EDITOR_TAG_TYPE_ACTION_NONE:
        case TAG_EDITOR_TAG_TYPE_ACTION_NUMBER_TRACKS:
        case TAG_EDITOR_TAG_TYPE_ACTION_CAPITALIZE:
        case TAG_EDITOR_TAG_TYPE_ACTION_LOWER:
        case TAG_EDITOR_TAG_TYPE_ACTION_RESET:
        case TAG_EDITOR_TAG_TYPE_ACTION_SAVE:
            return false;
        case TAG_EDITOR_TAG_TYPE_ACTION_COUNT:
        default:
            break;
        }
        return false;
    case TAG_EDITOR_FOCUS_PARSER_LEGEND:
    case TAG_EDITOR_FOCUS_PARSER_PREVIEW:
        return false;
    case TAG_EDITOR_FOCUS_COUNT:
    default:
        break;
    }
    return false;
}

static int32
tag_editor_save_recent_patterns(TagEditorScreen *screen) {
    StrBuilder path = {0};
    FILE *file;
    int32 limit;
    int32 status;

    tag_editor_history_path(&path);
    file = fopen(path.data, "w");
    if (file == NULL) {
        status = errno ? -errno : -EIO;
        sb_free(&path);
        return status;
    }
    status = 0;
    limit = screen->recent_patterns.len;
    if (limit > TAG_EDITOR_PATTERN_HISTORY_MAX) {
        limit = TAG_EDITOR_PATTERN_HISTORY_MAX;
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
tag_editor_prompt_tag_value(TagEditorScreen *screen,
                            enum NcmTagsField field, bool all_targets) {
    NcmMutableSong *song;
    StrBuilder initial;
    StrBuilder input = {0};
    char *label;
    int32 label_len;
    enum TagEditorPromptResult prompt_result;
    bool result;

    ASSERT(screen != NULL);
    if (field == NCM_TAGS_FIELD_COUNT) {
        return false;
    }
    song = nc_tag_row_menu_current(&screen->tags);
    ASSERT(song != NULL);

    label_len = NCM_TAGS_FIELD_alias_len(field, &label);
    initial = ncm_mutable_song_tags_buffer(
        song, field, Config.tags_separator, Config.tags_separator_len,
        Config.show_duplicate_tags);
    if (screen->hooks.prompt == NULL) {
        prompt_result = TAG_EDITOR_PROMPT_ERROR;
    } else {
        NcmStringView initial_view;

        ncm_string_view_set(&initial_view, initial.data, initial.len);
        prompt_result = screen->hooks.prompt(
            screen->hooks.user, label, label_len, initial_view, &input);
    }
    sb_free(&initial);

    if (prompt_result == TAG_EDITOR_PROMPT_ABORTED) {
        tag_editor_status_message(screen, STRLIT("Action aborted"));
        sb_free(&input);
        return false;
    }
    if (prompt_result != TAG_EDITOR_PROMPT_ACCEPTED) {
        sb_free(&input);
        return false;
    }

    if (all_targets) {
        tag_editor_screen_apply_tag_to_selection(
            screen, field, sb_opt_cstr(&input), input.len,
            Config.tags_separator, Config.tags_separator_len);
    } else {
        ncm_mutable_song_set_tags(
            song, field, sb_opt_cstr(&input), input.len,
            Config.tags_separator, Config.tags_separator_len);
    }
    result = true;
    sb_free(&input);
    return result;
}

static void
tag_editor_append_parser_filename(StrBuilder *buffer, char *name,
                                  int32 name_len) {
    if ((name == NULL) || (name_len <= 0)) {
        return;
    }
    SB_APPEND(buffer, name, name_len);
    return;
}

static int32
tag_editor_build_parser_preview(TagEditorScreen *screen,
                                bool apply, bool *success) {
    NcMenu *tags;
    int32 count;
    int32 status;

    ASSERT(screen != NULL);
    ASSERT(success != NULL);

    *success = true;
    tag_editor_status_message(screen, STRLIT("Parsing..."));
    sb_clear(&screen->parser_preview);
    tags = nc_tag_row_menu_base(&screen->tags);
    count = nc_menu_item_count(tags);
    for (int32 i = 0; i < count; i += 1) {
        NcmMutableSong *song;

        song = nc_menu_active_item_at(tags, i);
        ASSERT(song != NULL);
        if (screen->parser_mode == TAG_EDITOR_PARSER_TAGS_FROM_FILENAME) {
            if (!apply && song->name) {
                SB_APPEND(&screen->parser_preview, song->name, song->name_len);
                SB_APPEND(&screen->parser_preview, ":\n");
            }
            status = tag_editor_parse_filename(
                song, screen->pattern.data, screen->pattern.len, !apply,
                &screen->parser_preview);
            if ((status < 0) && !apply) {
                SB_APPEND(&screen->parser_preview,
                          "Error while parsing filename!\n");
            }
            if (!apply) {
                sb_append_byte(&screen->parser_preview, '\n');
            }
        } else if (screen->parser_mode == TAG_EDITOR_PARSER_RENAME_FILES) {
            StrBuilder stem = {0};
            StrBuilder new_name = {0};
            int32 extension_start;

            status = tag_editor_generate_filename(
                song, screen->pattern.data, screen->pattern.len, &stem);
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
                SB_APPEND(&new_name, song->name + extension_start,
                          song->name_len - extension_start);
            }
            if (apply && (stem.len <= 0)) {
                sb_clear(&screen->parser_preview);
                SB_APPEND(&screen->parser_preview, "File \"");
                tag_editor_append_parser_filename(
                    &screen->parser_preview, song->name, song->name_len);
                SB_APPEND(&screen->parser_preview,
                          STRLIT("\" would have an empty name"));
                tag_editor_status_message(
                    screen, screen->parser_preview.data,
                    screen->parser_preview.len);
                screen->parser_preview_enabled = true;
                *success = false;
                sb_free(&new_name);
                sb_free(&stem);
                return 0;
            }
            if (apply) {
                ncm_mutable_song_set_new_name(song, new_name.data,
                                              new_name.len);
            } else {
                tag_editor_append_parser_filename(
                    &screen->parser_preview, song->name, song->name_len);
                SB_APPEND(&screen->parser_preview,
                          " -> ");
                if (new_name.len > 0) {
                    SB_APPEND(&screen->parser_preview,
                              new_name.data, new_name.len);
                } else if (Config.empty_tag_marker) {
                    SB_APPEND(&screen->parser_preview,
                              Config.empty_tag_marker,
                              Config.empty_tag_marker_len);
                }
                SB_APPEND(&screen->parser_preview,
                          "\n\n");
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
tag_editor_run_current(NcScreen *screen) {
    TagEditorScreen *editor = tag_editor_from_screen(screen);

    switch (editor->active_focus) {
    case TAG_EDITOR_FOCUS_DIRECTORIES:
        if (tag_editor_screen_enter_directory(editor) == 0) {
            return 0;
        }
        return -NCM_ERROR_UNAVAILABLE;
    case TAG_EDITOR_FOCUS_TAG_TYPES: {
        enum TagEditorTagTypeAction action;
        enum NcmTagsField field;

        action = tag_editor_current_tag_type_action(editor, &field);
        switch (action) {
        case TAG_EDITOR_TAG_TYPE_ACTION_FIELD:
            if (tag_editor_prompt_tag_value(editor, field, true)) {
                return 0;
            }
            return -NCM_ERROR_UNAVAILABLE;
        case TAG_EDITOR_TAG_TYPE_ACTION_NUMBER_TRACKS:
            if (!tag_editor_confirm(editor, STRLIT("Number tracks?"))) {
                return -NCM_ERROR_UNAVAILABLE;
            }
            tag_editor_screen_number_tracks(
                editor, Config.tag_editor_extended_numeration);
            tag_editor_status_message(editor, STRLIT("Tracks numbered"));
            return 0;
        case TAG_EDITOR_TAG_TYPE_ACTION_FILENAME:
            tag_editor_screen_show_parser_dialog(editor);
            return 0;
        case TAG_EDITOR_TAG_TYPE_ACTION_CAPITALIZE:
            tag_editor_status_message(editor, STRLIT("Processing..."));
            tag_editor_screen_capitalize_first_letters(editor);
            tag_editor_status_message(editor, STRLIT("Done"));
            return 0;
        case TAG_EDITOR_TAG_TYPE_ACTION_LOWER:
            tag_editor_status_message(editor, STRLIT("Processing..."));
            tag_editor_screen_lower_all_letters(editor);
            tag_editor_status_message(editor, STRLIT("Done"));
            return 0;
        case TAG_EDITOR_TAG_TYPE_ACTION_RESET:
            tag_editor_screen_clear_modifications(editor);
            tag_editor_status_message(editor, STRLIT("Changes reset"));
            return 0;
        case TAG_EDITOR_TAG_TYPE_ACTION_SAVE:
            if (tag_editor_screen_save_modified(
                editor, Config.mpd_music_dir) > 0) {
                return 0;
            }
            return -NCM_ERROR_UNAVAILABLE;
        case TAG_EDITOR_TAG_TYPE_ACTION_NONE:
        case TAG_EDITOR_TAG_TYPE_ACTION_COUNT:
        default:
            return -NCM_ERROR_UNAVAILABLE;
        }
    }
    case TAG_EDITOR_FOCUS_TAGS: {
        enum TagEditorTagTypeAction action;
        enum NcmTagsField field;
        NcMenu *tags;
        bool result;

        action = tag_editor_current_tag_type_action(editor, &field);
        if (action == TAG_EDITOR_TAG_TYPE_ACTION_FIELD) {
            result = tag_editor_prompt_tag_value(editor, field, false);
        } else if (action == TAG_EDITOR_TAG_TYPE_ACTION_FILENAME) {
            NcmMutableSong *song;
            NcmStringView current_name;
            NcmStringView initial;
            StrBuilder input = {0};
            enum TagEditorPromptResult prompt_result;
            int32 dot = -1;

            ASSERT(editor != NULL);
            song = nc_tag_row_menu_current(&editor->tags);
            ASSERT(song != NULL);
            if (!ncm_mutable_song_has_new_name_view(song, &current_name)) {
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
                prompt_result = TAG_EDITOR_PROMPT_ERROR;
            } else {
                prompt_result = editor->hooks.prompt(
                    editor->hooks.user, STRLIT("New filename"), initial,
                    &input);
            }
            if (prompt_result == TAG_EDITOR_PROMPT_ABORTED) {
                tag_editor_status_message(editor, STRLIT("Action aborted"));
                result = false;
            } else if (prompt_result != TAG_EDITOR_PROMPT_ACCEPTED) {
                result = false;
            } else if (input.len <= 0) {
                result = true;
            } else {
                NcmStringView stem_name;
                StrBuilder new_name = {0};
                int32 stem_dot = -1;

                if (!ncm_mutable_song_has_new_name_view(song, &stem_name)) {
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
                    SB_APPEND(&new_name, stem_name.data + stem_dot,
                              stem_name.len - stem_dot);
                }
                ncm_mutable_song_set_new_name(song, new_name.data,
                                              new_name.len);
                sb_free(&new_name);
                result = true;
            }
            sb_free(&input);
        } else {
            return -NCM_ERROR_UNAVAILABLE;
        }

        if (result) {
            tags = nc_tag_row_menu_base(&editor->tags);
            nc_menu_scroll_selectable(
                tags, nc_window_height(&editor->tags_window), NC_SCROLL_DOWN);
            return 0;
        }
        return -NCM_ERROR_UNAVAILABLE;
    }
    case TAG_EDITOR_FOCUS_PARSER_CHOICE: {
        NcMenu *menu;
        int32 choice;

        menu = nc_editor_string_menu_base(&editor->parser_dialog);
        if (!nc_menu_current_is_selectable(menu)) {
            return -NCM_ERROR_UNAVAILABLE;
        }
        choice = nc_menu_highlight(menu);
        if (choice == 0) {
            tag_editor_screen_show_parser_actions(
                editor, TAG_EDITOR_PARSER_TAGS_FROM_FILENAME);
            return 0;
        }
        if (choice == 1) {
            tag_editor_screen_show_parser_actions(
                editor, TAG_EDITOR_PARSER_RENAME_FILES);
            return 0;
        }
        if (choice == 2) {
            tag_editor_screen_close_parser(editor);
            return 0;
        }
        return -NCM_ERROR_UNAVAILABLE;
    }
    case TAG_EDITOR_FOCUS_PARSER_ACTIONS: {
        NcMenu *menu;
        int32 choice;
        bool success;
        int32 status;

        menu = nc_editor_string_menu_base(&editor->parser_actions);
        if (!nc_menu_current_is_selectable(menu)) {
            return -NCM_ERROR_UNAVAILABLE;
        }
        choice = nc_menu_highlight(menu);
        if (choice == TAG_EDITOR_PARSER_ACTION_PATTERN) {
            bool result = false;

            if (editor->hooks.prompt != NULL) {
                StrBuilder input = {0};
                NcmStringView initial;
                enum TagEditorPromptResult prompt_result;

                initial.data = editor->pattern.data;
                initial.len = editor->pattern.len;
                prompt_result = editor->hooks.prompt(
                    editor->hooks.user, STRLIT("Pattern"), initial, &input);
                if (prompt_result == TAG_EDITOR_PROMPT_ABORTED) {
                    tag_editor_status_message(
                        editor, STRLIT("Action aborted"));
                } else if (prompt_result != TAG_EDITOR_PROMPT_ERROR) {
                    tag_editor_set_pattern(editor, input.data, input.len);
                    tag_editor_screen_prepare_parser_rows(
                        editor, editor->parser_mode, editor->pattern.data,
                        editor->pattern.len);
                    result = true;
                }
                sb_free(&input);
            }
            if (result) {
                tag_editor_set_focus(
                    editor, TAG_EDITOR_FOCUS_PARSER_ACTIONS);
                nc_menu_goto_selectable(
                    nc_editor_string_menu_base(&editor->parser_actions),
                    TAG_EDITOR_PARSER_ACTION_PATTERN);
                return 0;
            }
            return -NCM_ERROR_UNAVAILABLE;
        }
        if (choice == TAG_EDITOR_PARSER_ACTION_PREVIEW) {
            status = tag_editor_build_parser_preview(editor, false, &success);
            if (status < 0) {
                return -NCM_ERROR_UNAVAILABLE;
            }
            tag_editor_screen_show_parser_preview(editor);
            tag_editor_status_message(editor, STRLIT("Operation finished"));
            return 0;
        }
        if (choice == TAG_EDITOR_PARSER_ACTION_LEGEND) {
            tag_editor_build_parser_legend(editor);
            tag_editor_screen_show_parser_legend(editor);
            return 0;
        }
        if (choice == TAG_EDITOR_PARSER_ACTION_PROCEED) {
            status = tag_editor_build_parser_preview(editor, true, &success);
            if (status < 0) {
                return -NCM_ERROR_UNAVAILABLE;
            }
            if (success) {
                if (editor->pattern.len <= 0) {
                    return -NCM_ERROR_UNAVAILABLE;
                }
                {
                    StrBuilderArray replacement = {0};
                    StrBuilder first = {0};
                    int32 existing;

                    sb_set(&first, editor->pattern.data, editor->pattern.len);
                    str_builder_array_append_copy(&replacement, &first);
                    sb_free(&first);
                    existing = tag_editor_find_recent_pattern(
                        editor, editor->pattern.data, editor->pattern.len);
                    for (int32 i = 0; i < editor->recent_patterns.len;
                         i += 1) {
                        if (i == existing) {
                            continue;
                        }
                        str_builder_array_append_copy(
                            &replacement, &editor->recent_patterns.items[i]);
                    }
                    str_builder_array_move(&editor->recent_patterns,
                                           &replacement);
                    str_builder_array_destroy(&replacement);
                    tag_editor_screen_prepare_parser_rows(
                        editor, editor->parser_mode, editor->pattern.data,
                        editor->pattern.len);
                }
                tag_editor_save_recent_patterns(editor);
                tag_editor_status_message(editor,
                                          STRLIT("Operation finished"));
                tag_editor_screen_close_parser(editor);
                return 0;
            }
            return -NCM_ERROR_UNAVAILABLE;
        }
        if (choice == TAG_EDITOR_PARSER_ACTION_CANCEL) {
            tag_editor_save_recent_patterns(editor);
            tag_editor_screen_close_parser(editor);
            return 0;
        }
        if (choice >= (int32)TAG_EDITOR_PARSER_ACTION_RECENT_START) {
            StrBuilder *row;

            if ((row = nc_menu_active_item_at(menu, choice))) {
                tag_editor_set_pattern(editor, row->data, row->len);
                tag_editor_screen_prepare_parser_rows(
                    editor, editor->parser_mode, editor->pattern.data,
                    editor->pattern.len);
                tag_editor_set_focus(editor, TAG_EDITOR_FOCUS_PARSER_ACTIONS);
                nc_menu_goto_selectable(
                    nc_editor_string_menu_base(&editor->parser_actions),
                    TAG_EDITOR_PARSER_ACTION_PATTERN);
                return 0;
            }
        }
        return -NCM_ERROR_UNAVAILABLE;
    }
    case TAG_EDITOR_FOCUS_PARSER_LEGEND:
    case TAG_EDITOR_FOCUS_PARSER_PREVIEW:
    case TAG_EDITOR_FOCUS_COUNT:
    default:
        return -NCM_ERROR_UNAVAILABLE;
    }
}

static void
tag_editor_switch_to(NcScreen *screen) {
    nc_screen_switcher_finish_switch(screen);
    ncm_title_draw_header(STRLIT("Tag editor"));
    return;
}

static void
tag_editor_resize(NcScreen *screen) {
    TagEditorScreen *editor = tag_editor_from_screen(screen);
    int32 start_x;
    int32 width;

    nc_screen_switcher_get_resize_params(screen, &start_x, &width, true);
    tag_editor_screen_set_geometry(
        editor, start_x, width, ui_state_main_start_y(),
        ui_state_main_height());
    nc_screen_clear_resize_request(screen);
    return;
}

static char *
tag_editor_title(NcScreen *screen) {
    (void)screen;
    return "Tag editor";
}

static void
tag_editor_report_error(char *context, int32 context_len,
                        NcmError *ncm_error) {
    StrBuilder message = {0};

    SB_APPEND(&message, context, context_len);
    if (ncm_error && (ncm_error->message[0] != 0)) {
        SB_APPEND(&message, ": ");
        SB_APPEND(&message, ncm_error->message,
                  strlen32(ncm_error->message));
    }
    ncm_statusbar_print_cstring(Config.message_delay_time,
                                message.data);
    sb_free(&message);
    return;
}

static void
tag_editor_update(NcScreen *screen) {
    TagEditorScreen *editor = tag_editor_from_screen(screen);
    NcmError ncm_error;
    int32 status;
    bool changed = false;
    bool continue_update = true;

    tag_editor_screen_finish_directory_change(editor);
    ncm_error_clear(&ncm_error);
    if (editor->directories_update_requested
        || nc_menu_is_empty(
            nc_editor_pair_menu_base(&editor->directories))) {
        status = tag_editor_reload_directories_from_mpd(
            editor, &global_mpd, &ncm_error);
        if (status < 0) {
            editor->directories_update_requested = false;
            tag_editor_report_error(
                STRLIT("Could not fetch directories"), &ncm_error);
            ncm_error_clear(&ncm_error);
            tag_editor_update_titles(editor, true);
            continue_update = false;
        } else {
            changed = true;
        }
    }

    if (continue_update) {
        tag_editor_screen_finish_directory_change(editor);
        if (!editor->tags_update_requested
            && !nc_menu_is_empty(nc_tag_row_menu_base(&editor->tags))) {
            tag_editor_update_titles(editor, true);
            continue_update = false;
        }
    }

    if (continue_update) {
        ncm_error_clear(&ncm_error);
        status = tag_editor_reload_songs_from_mpd(
            editor, &global_mpd, &ncm_error);
        if (status < 0) {
            editor->tags_update_requested = false;
            tag_editor_report_error(STRLIT("Could not fetch songs"),
                                    &ncm_error);
            ncm_error_clear(&ncm_error);
            tag_editor_update_titles(editor, true);
        } else {
            changed = true;
            tag_editor_update_titles(editor, true);
        }
    }

    nc_screen_clear_update_request(screen);
    if (changed && app_controller_is_screen_visible(screen)) {
        nc_screen_refresh(screen);
    }
    return;
}

static bool
tag_editor_mouse_move_to_parser_focus(TagEditorScreen *screen,
                                      enum TagEditorFocus focus) {
    ASSERT(screen != NULL);
    if (focus == TAG_EDITOR_FOCUS_PARSER_CHOICE) {
        tag_editor_set_focus(screen, focus);
        return true;
    }
    if (screen->parser_mode == TAG_EDITOR_PARSER_NONE) {
        return false;
    }
    if ((focus == TAG_EDITOR_FOCUS_PARSER_ACTIONS)
        || tag_editor_focus_is_parser_helper(focus)) {
        tag_editor_set_focus(screen, focus);
        return true;
    }
    return false;
}

static bool
tag_editor_mouse_move_to_column(TagEditorScreen *screen,
                                enum TagEditorColumn column) {
    ASSERT(screen != NULL);
    if (!tag_editor_focus_is_main(screen->active_focus)) {
        return false;
    }
    if (((screen->active_focus == TAG_EDITOR_FOCUS_DIRECTORIES)
         && (column == TAG_EDITOR_COLUMN_DIRECTORIES))
        || ((screen->active_focus == TAG_EDITOR_FOCUS_TAG_TYPES)
            && (column == TAG_EDITOR_COLUMN_TAG_TYPES))
        || ((screen->active_focus == TAG_EDITOR_FOCUS_TAGS)
            && (column == TAG_EDITOR_COLUMN_TAGS))) {
        return true;
    }
    while (screen->active_column < column) {
        if (!tag_editor_screen_next_column_available(screen)) {
            return false;
        }
        tag_editor_screen_next_column(screen);
    }
    while (screen->active_column > column) {
        if (!tag_editor_screen_previous_column_available(screen)) {
            return false;
        }
        tag_editor_screen_previous_column(screen);
    }
    tag_editor_update_menu_highlights(screen);
    return true;
}

static void
tag_editor_mouse_scroll_menu(NcMenu *menu, NcWindow *window,
                             enum NcScroll where) {
    enum NcScroll effective;
    int32 count;

    ASSERT(menu != NULL);
    ASSERT(window != NULL);
    effective = where;
    count = Config.lines_scrolled;
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
        nc_menu_scroll_selectable(menu, nc_window_height(window),
                                  effective);
    }
    return;
}

static void
tag_editor_mouse_scroll(TagEditorScreen *screen,
                        enum NcScroll where) {
    NcMenu *menu;
    NcWindow *window;

    ASSERT(screen != NULL);

    menu = tag_editor_screen_active_menu(screen);
    window = tag_editor_screen_active_window(screen);
    tag_editor_mouse_scroll_menu(menu, window, where);
    tag_editor_screen_finish_directory_change(screen);
    tag_editor_finish_tag_type_change(screen, true);
    return;
}

static int32
tag_editor_run_current_action(TagEditorScreen *screen) {
    ASSERT(screen != NULL);
    return nc_screen_run_current(tag_editor_screen_base(screen));
}

static void
tag_editor_mouse_callback(NcScreen *screen, MEVENT event) {
    TagEditorScreen *editor = tag_editor_from_screen(screen);
    int32 x;
    int32 y;

    if (!tag_editor_focus_is_main(editor->active_focus)) {
        x = event.x;
        y = event.y;
        if (nc_window_has_coords(&editor->parser_dialog_window, &x, &y)) {
            if (!tag_editor_mouse_move_to_parser_focus(
                editor, TAG_EDITOR_FOCUS_PARSER_CHOICE)) {
                return;
            }
            if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
                NcMenu *menu = nc_editor_string_menu_base(
                    &editor->parser_dialog);

                if ((y >= 0) && (y < nc_menu_item_count(menu))
                    && (nc_menu_goto_selectable(menu, y) >= 0)
                    && (event.bstate & BUTTON3_PRESSED)) {
                    tag_editor_run_current_action(editor);
                }
            } else if (event.bstate & BUTTON5_PRESSED) {
                tag_editor_mouse_scroll_menu(
                    nc_editor_string_menu_base(&editor->parser_dialog),
                    &editor->parser_dialog_window, NC_SCROLL_DOWN);
            } else if (event.bstate & BUTTON4_PRESSED) {
                tag_editor_mouse_scroll_menu(
                    nc_editor_string_menu_base(&editor->parser_dialog),
                    &editor->parser_dialog_window, NC_SCROLL_UP);
            }
            nc_screen_refresh(screen);
            return;
        }

        x = event.x;
        y = event.y;
        if (nc_window_has_coords(&editor->parser_window, &x, &y)) {
            if (!tag_editor_mouse_move_to_parser_focus(
                editor, TAG_EDITOR_FOCUS_PARSER_ACTIONS)) {
                return;
            }
            if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
                NcMenu *menu = nc_editor_string_menu_base(
                    &editor->parser_actions);

                if ((y >= 0) && (y < nc_menu_item_count(menu))
                    && (nc_menu_goto_selectable(menu, y) >= 0)
                    && (event.bstate & BUTTON3_PRESSED)) {
                    tag_editor_run_current_action(editor);
                }
            } else if (event.bstate & BUTTON5_PRESSED) {
                tag_editor_mouse_scroll_menu(
                    nc_editor_string_menu_base(&editor->parser_actions),
                    &editor->parser_window, NC_SCROLL_DOWN);
            } else if (event.bstate & BUTTON4_PRESSED) {
                tag_editor_mouse_scroll_menu(
                    nc_editor_string_menu_base(&editor->parser_actions),
                    &editor->parser_window, NC_SCROLL_UP);
            }
            nc_screen_refresh(screen);
            return;
        }

        x = event.x;
        y = event.y;
        if (nc_window_has_coords(&editor->parser_helper_window, &x, &y)) {
            if (!tag_editor_mouse_move_to_parser_focus(
                editor, tag_editor_current_helper_focus(editor))) {
                return;
            }
            if (event.bstate & BUTTON5_PRESSED) {
                nc_window_scroll(&editor->parser_helper_window,
                                 NC_SCROLL_DOWN);
            } else if (event.bstate & BUTTON4_PRESSED) {
                nc_window_scroll(&editor->parser_helper_window,
                                 NC_SCROLL_UP);
            }
            return;
        }
        return;
    }

    x = event.x;
    y = event.y;
    if (nc_window_has_coords(&editor->directories_window, &x, &y)) {
        if (!tag_editor_mouse_move_to_column(
            editor, TAG_EDITOR_COLUMN_DIRECTORIES)) {
            return;
        }
        if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
            NcMenu *menu = nc_editor_pair_menu_base(&editor->directories);

            if ((y >= 0) && (y < nc_menu_item_count(menu))
                && (nc_menu_goto_selectable(menu, y) >= 0)) {
                tag_editor_screen_finish_directory_change(editor);
                if (event.bstate & BUTTON1_PRESSED) {
                    tag_editor_screen_enter_directory(editor);
                }
            }
        } else if (event.bstate & BUTTON5_PRESSED) {
            tag_editor_mouse_scroll(editor, NC_SCROLL_DOWN);
        } else if (event.bstate & BUTTON4_PRESSED) {
            tag_editor_mouse_scroll(editor, NC_SCROLL_UP);
        }
        tag_editor_screen_finish_directory_change(editor);
        nc_screen_refresh(screen);
        return;
    }

    x = event.x;
    y = event.y;
    if (nc_window_has_coords(&editor->tag_types_window, &x, &y)) {
        if (!tag_editor_mouse_move_to_column(
            editor, TAG_EDITOR_COLUMN_TAG_TYPES)) {
            return;
        }
        if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
            NcMenu *menu = nc_editor_string_menu_base(&editor->tag_types);

            if ((y >= 0) && (y < nc_menu_item_count(menu))
                && (nc_menu_goto_selectable(menu, y) >= 0)) {
                tag_editor_finish_tag_type_change(editor, true);
                if (event.bstate & BUTTON3_PRESSED) {
                    tag_editor_run_current_action(editor);
                }
            }
        } else if (event.bstate & BUTTON5_PRESSED) {
            tag_editor_mouse_scroll(editor, NC_SCROLL_DOWN);
        } else if (event.bstate & BUTTON4_PRESSED) {
            tag_editor_mouse_scroll(editor, NC_SCROLL_UP);
        }
        tag_editor_finish_tag_type_change(editor, true);
        nc_screen_refresh(screen);
        return;
    }

    x = event.x;
    y = event.y;
    if (nc_window_has_coords(&editor->tags_window, &x, &y)) {
        if (!tag_editor_mouse_move_to_column(
            editor, TAG_EDITOR_COLUMN_TAGS)) {
            return;
        }
        if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
            NcMenu *menu = nc_tag_row_menu_base(&editor->tags);

            if ((y >= 0) && (y < nc_menu_item_count(menu))
                && (nc_menu_goto_selectable(menu, y) >= 0)
                && (event.bstate & BUTTON3_PRESSED)) {
                tag_editor_run_current_action(editor);
            }
        } else if (event.bstate & BUTTON5_PRESSED) {
            tag_editor_mouse_scroll(editor, NC_SCROLL_DOWN);
        } else if (event.bstate & BUTTON4_PRESSED) {
            tag_editor_mouse_scroll(editor, NC_SCROLL_UP);
        }
        nc_screen_refresh(screen);
        return;
    }
    return;
}

static bool
tag_editor_focus_is_main(enum TagEditorFocus focus) {
    return (focus == TAG_EDITOR_FOCUS_DIRECTORIES)
           || (focus == TAG_EDITOR_FOCUS_TAG_TYPES)
           || (focus == TAG_EDITOR_FOCUS_TAGS);
}



static void
tag_editor_destroy_callback(NcScreen *screen) {
    tag_editor_screen_destroy(tag_editor_from_screen(screen));
    return;
}

static void
tag_editor_layout(TagEditorScreen *screen) {
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

    separator_width = tag_editor_separator_width(screen);
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

    nc_window_move_to(&screen->directories_window, screen->start_x,
                      screen->main_start_y);
    nc_window_resize(&screen->directories_window, screen->left_width,
                     screen->main_height);
    nc_window_move_to(&screen->tag_types_window, screen->middle_start_x,
                      screen->main_start_y);
    nc_window_resize(&screen->tag_types_window, screen->middle_width,
                     screen->main_height);
    nc_window_move_to(&screen->tags_window, screen->right_start_x,
                      screen->main_start_y);
    nc_window_resize(&screen->tags_window, screen->right_width,
                     screen->main_height);

    nc_window_move_to(&screen->parser_dialog_window,
                      screen->parser_dialog_start_x,
                      screen->parser_dialog_start_y);
    nc_window_resize(&screen->parser_dialog_window,
                     screen->parser_dialog_width,
                     screen->parser_dialog_height);
    nc_window_move_to(&screen->parser_window, screen->parser_start_x,
                      screen->parser_start_y);
    nc_window_resize(&screen->parser_window, screen->parser_width_one,
                     screen->parser_height);
    nc_window_move_to(&screen->parser_helper_window,
                      screen->parser_helper_start_x,
                      screen->parser_start_y);
    nc_window_resize(&screen->parser_helper_window,
                     screen->parser_width_two, screen->parser_height);
    return;
}

#endif /* NC_TAG_EDITOR_C */
