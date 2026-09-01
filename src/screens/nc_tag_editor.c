#if !defined(NCMPCPP_NC_TAG_EDITOR_C)
#define NCMPCPP_NC_TAG_EDITOR_C

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
#define ENUM_FIELDS \
    XX(TAG_EDITOR_TAG_TYPE_ACTION_NONE) \
    XX(TAG_EDITOR_TAG_TYPE_ACTION_FIELD) \
    XX(TAG_EDITOR_TAG_TYPE_ACTION_NUMBER_TRACKS) \
    XX(TAG_EDITOR_TAG_TYPE_ACTION_FILENAME) \
    XX(TAG_EDITOR_TAG_TYPE_ACTION_CAPITALIZE) \
    XX(TAG_EDITOR_TAG_TYPE_ACTION_LOWER) \
    XX(TAG_EDITOR_TAG_TYPE_ACTION_RESET) \
    XX(TAG_EDITOR_TAG_TYPE_ACTION_SAVE)
#include "cbase/xenums.c"

typedef struct SaveContext SaveContext;

static NcWindow *tag_editor_active_window(NcScreen *screen);
static void tag_editor_refresh(NcScreen *screen);
static void tag_editor_refresh_window(NcScreen *screen);
static void tag_editor_scroll(NcScreen *screen, enum NcScroll where);
static bool tag_editor_can_run_current(NcScreen *screen);
static bool tag_editor_run_current(NcScreen *screen);
static void tag_editor_switch_to(NcScreen *screen);
static void tag_editor_resize(NcScreen *screen);
static char *tag_editor_title(NcScreen *screen);
static void tag_editor_update(NcScreen *screen);
static bool tag_editor_update_from_mpd(TagEditorScreen *screen,
                                       NcmMpdClient *client);
static bool tag_editor_reload_directories_from_mpd(TagEditorScreen *screen,
                                                   NcmMpdClient *client,
                                                   NcmError *ncm_error);
static bool tag_editor_reload_songs_from_mpd(TagEditorScreen *screen,
                                             NcmMpdClient *client,
                                             NcmError *ncm_error);
static void tag_editor_mouse_callback(NcScreen *screen, MEVENT event);
static void tag_editor_destroy_callback(NcScreen *screen);
static void tag_editor_mouse_scroll(TagEditorScreen *screen,
                                    enum NcScroll where);
static void tag_editor_mouse_scroll_menu(NcMenu *menu, NcWindow *window,
                                         enum NcScroll where);
static bool tag_editor_mouse_select_directory(TagEditorScreen *screen,
                                              int32 y, bool enter);
static bool tag_editor_mouse_select_tag_type(TagEditorScreen *screen,
                                             int32 y, bool run);
static bool tag_editor_mouse_select_tag(TagEditorScreen *screen,
                                        int32 y, bool run);
static bool tag_editor_mouse_select_parser_dialog(TagEditorScreen *screen,
                                                  int32 y, bool run);
static bool tag_editor_mouse_select_parser_row(TagEditorScreen *screen,
                                               int32 y, bool run);
static bool tag_editor_run_current_action(TagEditorScreen *screen);
static bool tag_editor_mouse_move_to_column(TagEditorScreen *screen,
                                            enum TagEditorColumn column);
static bool tag_editor_mouse_move_to_parser_focus(TagEditorScreen *screen,
                                                  enum TagEditorFocus focus);
static void tag_editor_finish_tag_type_change(TagEditorScreen *screen,
                                              bool refresh_tags);
static void tag_editor_layout(TagEditorScreen *screen);
static int32 tag_editor_min_int64(int32 left, int32 right);
static int32 tag_editor_separator_width(TagEditorScreen *screen);
static void tag_editor_configure_menus(TagEditorScreen *screen);
static bool tag_editor_initialize_tag_types(TagEditorScreen *screen);
static bool tag_editor_append_string_row(NcEditorStringMenu *menu,
                                         char *data, int32 data_len,
                                         uint32 flags);
static void tag_editor_update_menu_highlights(TagEditorScreen *screen);
static void tag_editor_update_parser_borders(TagEditorScreen *screen);
static void tag_editor_refresh_active_helper(TagEditorScreen *screen);
static void tag_editor_refresh_menu(NcWindow *window, NcMenu *menu);
static void tag_editor_draw_separators(TagEditorScreen *screen);
static NcMenuDisplayCallbacks tag_editor_directory_display_callbacks(
    TagEditorScreen *screen);
static NcMenuDisplayCallbacks tag_editor_tag_type_display_callbacks(
    TagEditorScreen *screen);
static NcMenuDisplayCallbacks tag_editor_tag_display_callbacks(
    TagEditorScreen *screen);
static void tag_editor_draw_directory(NcMenu *menu, NcWindow *window,
                                      void *item, int32 pos,
                                      void *user);
static void tag_editor_draw_string(NcMenu *menu, NcWindow *window,
                                   void *item, int32 pos, void *user);
static void tag_editor_draw_tag(NcMenu *menu, NcWindow *window,
                                void *item, int32 pos, void *user);
static void tag_editor_append_tag_display_value(
    TagEditorScreen *screen, NcmMutableSong *song, NcBuffer *buffer);
static void tag_editor_append_empty_tag(NcBuffer *buffer);
static void tag_editor_append_formatted_color(NcBuffer *buffer,
                                              NcFormattedColor *color);
static void tag_editor_append_formatted_color_end(NcBuffer *buffer,
                                                  NcFormattedColor *color);
static void tag_editor_append_locale(NcBuffer *buffer, char *data,
                                     int32 data_len);
static void tag_editor_print_buffer(NcWindow *window, NcBuffer *buffer);
static void tag_editor_initialize_buffers(TagEditorScreen *screen);
static void tag_editor_destroy_buffers(TagEditorScreen *screen);
static void tag_editor_initialize_regexes(TagEditorScreen *screen);
static void tag_editor_destroy_regexes(TagEditorScreen *screen);
static bool tag_editor_set_buffer(StrBuilder *buffer, char *data,
                                  int32 data_len);
static bool tag_editor_compile_constraint(NcmRegex *regex, char *pattern,
                                          int32 pattern_len,
                                          uint32 regex_flags,
                                          NcmError *ncm_error);
static void tag_editor_update_titles(TagEditorScreen *screen,
                                     bool update_windows);
static void tag_editor_update_visible_counts(TagEditorScreen *screen);
static void tag_editor_observe_current_directory(TagEditorScreen *screen);
static bool tag_editor_directory_row_changed(TagEditorScreen *screen);
static bool tag_editor_focus_is_main(enum TagEditorFocus focus);
static bool tag_editor_focus_is_main_column(enum TagEditorFocus focus,
                                            enum TagEditorColumn column);
static bool tag_editor_focus_is_parser_helper(enum TagEditorFocus focus);
static enum TagEditorFocus tag_editor_column_focus(enum TagEditorColumn column);
static void tag_editor_set_focus(TagEditorScreen *screen,
                                 enum TagEditorFocus focus);
static enum TagEditorFocus tag_editor_current_helper_focus(
    TagEditorScreen *screen);
static bool tag_editor_current_directory_path(TagEditorScreen *screen,
                                              char **path,
                                              int32 *path_len);
static bool tag_editor_directory_has_subdirectories(TagEditorScreen *screen,
                                                    char *path,
                                                    int32 path_len);
static bool tag_editor_directory_is_control(char *label, int32 label_len);
static bool tag_editor_highlight_directory_path(TagEditorScreen *screen,
                                                char *path, int32 path_len);
static bool tag_editor_highlight_song_uri(TagEditorScreen *screen,
                                          char *uri, int32 uri_len);
static bool tag_editor_current_directory_pair(TagEditorScreen *screen,
                                              NcMenuStringPair **pair);
static bool tag_editor_build_renamed_directory(TagEditorScreen *screen,
                                               char *name, int32 name_len,
                                               StrBuilder *result);
static void tag_editor_status_directory_renamed(TagEditorScreen *screen,
                                                char *name, int32 name_len);
static void tag_editor_status_directory_rename_error(TagEditorScreen *screen,
                                                     char *name,
                                                     int32 name_len,
                                                     NcmError *ncm_error);
static bool tag_editor_has_modified_songs(TagEditorScreen *screen);
static int32 tag_editor_compare_directories(NcmDirectory *left,
                                            NcmDirectory *right);
static int32 tag_editor_compare_songs(NcmSong *left, NcmSong *right);
static bool tag_editor_directory_filter(NcMenu *menu, void *item,
                                        void *user);
static bool tag_editor_tag_filter(NcMenu *menu, void *item, void *user);
static bool tag_editor_copy_selected_song_at(
    TagEditorScreen *screen, NcmSongArray *songs, int32 pos);
static bool tag_editor_for_each_target(TagEditorScreen *screen,
                                       bool (*cb)(NcmMutableSong *song,
                                                  void *user),
                                       void *user);
static bool tag_editor_set_song_tag_callback(NcmMutableSong *song,
                                             void *user);
static bool tag_editor_number_song_callback(NcmMutableSong *song,
                                            void *user);
static bool tag_editor_capitalize_song_callback(NcmMutableSong *song,
                                                void *user);
static bool tag_editor_lower_song_callback(NcmMutableSong *song,
                                           void *user);
static bool tag_editor_save_song_callback(NcmMutableSong *song, void *user);
static void tag_editor_save_status_with_name(TagEditorScreen *screen,
                                             char *prefix, int32 prefix_len,
                                             NcmMutableSong *song,
                                             char *suffix, int32 suffix_len);
static void tag_editor_save_status_error(TagEditorScreen *screen,
                                         NcmMutableSong *song, int32 error);
static void tag_editor_update_modified_directory(TagEditorScreen *screen,
                                                 StrBuilder *directory);
static void tag_editor_save_context_add_directory(SaveContext *context,
                                                  NcmMutableSong *song);
static bool tag_editor_tag_matches(TagEditorScreen *screen,
                                   NcmMutableSong *song);
static bool tag_editor_directory_matches(TagEditorScreen *screen,
                                         NcMenuStringPair *pair);
static bool tag_editor_tag_matches_regex(TagEditorScreen *screen,
                                         NcmMutableSong *song,
                                         NcmRegex *regex);
static bool tag_editor_tag_search_text(TagEditorScreen *screen,
                                       NcmMutableSong *song,
                                       StrBuilder *buffer);
static bool tag_editor_tag_search_field(TagEditorScreen *screen,
                                        enum NcmTagsField *field);
static bool tag_editor_tag_type_choice_is_editable(int32 choice);
static bool tag_editor_mutable_song_to_song(NcmMutableSong *source,
                                            NcmSong *dest);
static bool tag_editor_directory_matches_regex(NcMenuStringPair *pair,
                                               NcmRegex *regex,
                                               bool filter);
static bool tag_editor_active_item_matches(TagEditorScreen *screen,
                                           NcMenu *menu, int32 pos,
                                           NcmRegex *regex);
static bool tag_editor_search_position(NcMenu *menu, int32 pos,
                                       void *user);
static bool tag_editor_append_parser_row(NcEditorStringMenu *menu,
                                         char *data, int32 data_len,
                                         uint32 flags);
static bool tag_editor_build_parser_menus(TagEditorScreen *screen);
static bool tag_editor_build_parser_legend(TagEditorScreen *screen);
static bool tag_editor_build_parser_preview(TagEditorScreen *screen,
                                            bool apply, bool *success);
static bool tag_editor_load_recent_patterns(TagEditorScreen *screen);
static bool tag_editor_save_recent_patterns(TagEditorScreen *screen);
static bool tag_editor_history_path(StrBuilder *path);
static bool tag_editor_read_pattern_line(FILE *file, StrBuilder *line,
                                         bool *read_line);
static bool tag_editor_add_recent_pattern(TagEditorScreen *screen,
                                          char *pattern, int32 pattern_len);
static bool tag_editor_move_pattern_to_front(TagEditorScreen *screen,
                                             char *pattern, int32 pattern_len);
static bool tag_editor_set_pattern(TagEditorScreen *screen,
                                   char *pattern, int32 pattern_len);
static bool tag_editor_prompt_pattern(TagEditorScreen *screen);
static bool tag_editor_apply_recent_pattern(TagEditorScreen *screen,
                                            int32 choice);
static bool tag_editor_mutable_song_to_format_song(NcmMutableSong *source,
                                                   NcmSong *dest);
static int32 tag_editor_filename_extension_start(char *name,
                                                 int32 name_len);
static void tag_editor_append_parser_filename(StrBuilder *buffer,
                                              char *name, int32 name_len);
static bool tag_editor_mutable_song_get_field(NcmMutableSong *song,
                                              enum NcmTagsField field,
                                              StrBuilder *buffer);
static void tag_editor_lower_ascii_buffer(StrBuilder *buffer);
static bool tag_editor_next_mask_tag(char *mask, int32 mask_len,
                                     int32 start, int32 *percent_pos,
                                     char *tag_char);
static enum TagEditorTagTypeAction tag_editor_current_tag_type_action(
    TagEditorScreen *screen, enum NcmTagsField *field);
static bool tag_editor_run_directory_current(TagEditorScreen *screen);
static bool tag_editor_run_tag_type_current(TagEditorScreen *screen);
static bool tag_editor_run_tag_current(TagEditorScreen *screen);
static bool tag_editor_run_parser_choice_current(TagEditorScreen *screen);
static bool tag_editor_run_parser_action_current(TagEditorScreen *screen);
static bool tag_editor_prompt_tag_value(TagEditorScreen *screen,
                                        enum NcmTagsField field,
                                        bool all_targets);
static bool tag_editor_prompt_current_filename(TagEditorScreen *screen);
static bool tag_editor_set_song_filename_stem(NcmMutableSong *song,
                                              char *stem,
                                              int32 stem_len);
static void tag_editor_status_message(TagEditorScreen *screen,
                                      char *message, int32 message_len);
static bool tag_editor_confirm(TagEditorScreen *screen,
                               char *message, int32 message_len);
static bool tag_editor_strings_equal(char *left, int32 left_len,
                                     char *right, int32 right_len);

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
    bool ok;
};

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
    tag_editor_initialize_buffers(screen);
    tag_editor_initialize_regexes(screen);
    tag_editor_update_titles(screen, false);
    nc_window_init(&screen->directories_window, start_x, main_start_y,
                   width, main_height, screen->directories_title.data,
                   screen->directories_title.len, color, border);
    nc_window_init(&screen->tag_types_window, start_x, main_start_y,
                   width, main_height, screen->tag_types_title.data,
                   screen->tag_types_title.len, color, border);
    nc_window_init(&screen->tags_window, start_x, main_start_y,
                   width, main_height, screen->tags_title.data,
                   screen->tags_title.len, color, border);
    nc_window_init(&screen->parser_dialog_window, start_x, main_start_y,
                   width, main_height, screen->parser_dialog_title.data,
                   screen->parser_dialog_title.len, color,
                   Config.window_border);
    nc_window_init(&screen->parser_window, start_x, main_start_y,
                   width, main_height, screen->parser_title.data,
                   screen->parser_title.len, color, Config.window_border);
    nc_window_init(&screen->parser_helper_window, start_x, main_start_y,
                   width, main_height, screen->parser_helper_title.data,
                   screen->parser_helper_title.len, color,
                   Config.window_border);

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

    (void)tag_editor_screen_set_current_dir(screen,
                                            STRLIT("/"));
    (void)tag_editor_initialize_tag_types(screen);
    tag_editor_layout(screen);
    tag_editor_configure_menus(screen);
    tag_editor_observe_current_directory(screen);
    nc_screen_init_ops(&screen->screen, tag_editor_callbacks, screen,
                       NC_SCREEN_TYPE_TAG_EDITOR);
    (void)tag_editor_screen_prepare_parser_rows(
        screen, TAG_EDITOR_PARSER_NONE, NULL, 0);
    return;
}

void
tag_editor_screen_destroy(TagEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    (void)app_controller_unregister_screen(tag_editor_screen_base(
        screen));
    tag_editor_destroy_regexes(screen);
    tag_editor_destroy_buffers(screen);
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
    if (screen == NULL) {
        return;
    }
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
    if (screen == NULL) {
        return;
    }
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
    if (screen == NULL) {
        return;
    }
    if (screen->active_focus != TAG_EDITOR_FOCUS_DIRECTORIES) {
        return;
    }
    if (tag_editor_directory_row_changed(screen)) {
        tag_editor_screen_clear_stale_tags(screen);
    }
    return;
}

bool
tag_editor_screen_set_current_dir(TagEditorScreen *screen,
                                  char *dir, int32 dir_len) {
    bool changed;

    if (screen == NULL) {
        return false;
    }
    changed = screen->current_dir.data
              && !STREQUAL(screen->current_dir.data,
                           screen->current_dir.len, dir, dir_len);
    if (!tag_editor_set_buffer(&screen->current_dir, dir, dir_len)) {
        return false;
    }
    screen->directories_update_requested = true;
    if (changed) {
        tag_editor_screen_clear_stale_tags(screen);
    }
    return true;
}

bool
tag_editor_screen_current_dir(TagEditorScreen *screen,
                              NcmStringView *view) {
    if (view) {
        *view = (NcmStringView){0};
    }
    if (screen == NULL) {
        return false;
    }
    ncm_string_view_set(view, screen->current_dir.data,
                        screen->current_dir.len);
    return screen->current_dir.data;
}

bool
tag_editor_screen_current_directory_path(TagEditorScreen *screen,
                                         NcmStringView *view) {
    char *path;
    int32 path_len;

    if (view) {
        *view = (NcmStringView){0};
    }
    if (screen == NULL) {
        return false;
    }
    if (!tag_editor_current_directory_path(screen, &path, &path_len)) {
        return false;
    }
    ncm_string_view_set(view, path, path_len);
    return true;
}

bool
tag_editor_screen_enter_directory(TagEditorScreen *screen) {
    NcmStringView path;

    path = (NcmStringView){0};
    if (screen == NULL) {
        return false;
    }
    if (screen->active_focus != TAG_EDITOR_FOCUS_DIRECTORIES) {
        return false;
    }
    if (!tag_editor_screen_current_directory_path(screen, &path)) {
        return false;
    }
    if (!tag_editor_directory_has_subdirectories(screen, path.data,
                                                 path.len)) {
        tag_editor_status_message(screen, STRLIT("No subdirectories found"));
        return false;
    }
    sb_clear(&screen->highlighted_dir);
    if (!tag_editor_screen_set_current_dir(screen, path.data,
                                           path.len)) {
        return false;
    }
    nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
    tag_editor_screen_clear_stale_tags(screen);
    screen->directories_update_requested = true;
    screen->observed_dir_valid = false;
    tag_editor_update_titles(screen, true);
    return true;
}

bool
tag_editor_screen_go_to_parent(TagEditorScreen *screen) {
    StrBuilder parent = {0};
    int32 parent_len;
    bool ok;

    if (screen == NULL) {
        return false;
    }
    if (screen->active_focus != TAG_EDITOR_FOCUS_DIRECTORIES) {
        return false;
    }
    if ((screen->current_dir.data == NULL)
        || (screen->current_dir.len <= 0)
        || STREQUAL(screen->current_dir.data,
                    screen->current_dir.len, STRLIT("/"))) {
        return false;
    }

    if (!tag_editor_set_buffer(&screen->highlighted_dir,
                               screen->current_dir.data,
                               screen->current_dir.len)) {
        sb_free(&parent);
        return false;
    }
    parent_len = ncm_string_parent_directory_len(screen->current_dir.data,
                                                 screen->current_dir.len);
    if (parent_len <= 0) {
        ok = tag_editor_set_buffer(&parent, STRLIT("/"));
    } else {
        ok = tag_editor_set_buffer(&parent, screen->current_dir.data,
                                   parent_len);
    }
    if (!ok || !tag_editor_screen_set_current_dir(
        screen, parent.data, parent.len)) {
        sb_free(&parent);
        return false;
    }
    sb_free(&parent);

    nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
    tag_editor_screen_clear_stale_tags(screen);
    screen->directories_update_requested = true;
    screen->observed_dir_valid = false;
    tag_editor_update_titles(screen, true);
    return true;
}

bool
tag_editor_screen_locate_song(TagEditorScreen *screen,
                              NcmSong *song) {
    NcmStringView directory;
    NcmStringView uri;
    StrBuilder parent = {0};
    NcmError ncm_error;
    int32 parent_len;
    bool ok;

    if ((screen == NULL) || (song == NULL)) {
        return false;
    }
    if (!ncm_song_uri_view(song, 0, &uri) || (uri.len <= 0)) {
        return false;
    }
    if (!ncm_string_contains_char(uri.data, uri.len, '/')) {
        return false;
    }
    if (!ncm_song_directory_view(song, 0, &directory)
        || (directory.len <= 0)) {
        return false;
    }

    parent_len = ncm_string_parent_directory_len(directory.data,
                                                 directory.len);
    if (parent_len <= 0) {
        ok = sb_set(&parent, STRLIT("/")) >= 0;
    } else {
        ok = sb_set(&parent, directory.data, parent_len) >= 0;
    }
    if (!ok) {
        sb_free(&parent);
        return false;
    }

    ok = tag_editor_screen_set_current_dir(screen, parent.data,
                                           parent.len)
         && (sb_set(&screen->highlighted_dir, directory.data,
                    directory.len) >= 0);
    if (ok) {
        nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
        ncm_error_clear(&ncm_error);
        ok = tag_editor_reload_directories_from_mpd(screen, &global_mpd,
                                                    &ncm_error);
    }
    if (ok) {
        ok = tag_editor_highlight_directory_path(screen, directory.data,
                                                 directory.len);
    }
    if (ok) {
        tag_editor_screen_clear_stale_tags(screen);
        ncm_error_clear(&ncm_error);
        ok = tag_editor_reload_songs_from_mpd(screen, &global_mpd, &ncm_error);
    }
    if (ok) {
        nc_menu_reset(nc_editor_string_menu_base(&screen->tag_types));
        tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_TAGS);
        ok = tag_editor_highlight_song_uri(screen, uri.data, uri.len);
    }

    sb_free(&parent);
    tag_editor_update_titles(screen, true);
    return ok;
}

bool
tag_editor_screen_rename_directory_available(TagEditorScreen *screen,
                                             char *music_dir,
                                             int32 music_dir_len) {
    NcMenuStringPair *pair;

    if ((screen == NULL) || (music_dir == NULL) || (music_dir_len <= 0)) {
        return false;
    }
    if (screen->active_focus != TAG_EDITOR_FOCUS_DIRECTORIES) {
        return false;
    }
    if (!tag_editor_current_directory_pair(screen, &pair)) {
        return false;
    }
    if (tag_editor_directory_is_control(pair->first, pair->first_len)) {
        return false;
    }
    return true;
}

bool
tag_editor_screen_rename_current_directory(TagEditorScreen *screen,
                                           char *music_dir,
                                           int32 music_dir_len) {
    NcMenuStringPair *pair;
    NcmStringView initial;
    StrBuilder name = {0};
    StrBuilder old_path = {0};
    StrBuilder new_path = {0};
    StrBuilder new_relative = {0};
    NcmError ncm_error;
    enum TagEditorPromptResult result;
    bool ok;

    if (!tag_editor_screen_rename_directory_available(
        screen, music_dir, music_dir_len)) {
        return false;
    }
    if ((screen->hooks.prompt == NULL) || !tag_editor_current_directory_pair(
        screen, &pair)) {
        return false;
    }

    ncm_string_view_set(&initial, pair->first, pair->first_len);
    result = screen->hooks.prompt(
        screen->hooks.user, STRLIT("Directory: "), initial, &name);
    if (result == TAG_EDITOR_PROMPT_ABORTED) {
        sb_free(&name);
        return true;
    }
    if (result != TAG_EDITOR_PROMPT_ACCEPTED) {
        sb_free(&name);
        return false;
    }
    if ((name.len <= 0)
        || STREQUAL(name.data, name.len, pair->first,
                    pair->first_len)) {
        sb_free(&name);
        return true;
    }

    ok = ncm_fs_join(&old_path, music_dir, music_dir_len,
                     pair->second, pair->second_len)
         && tag_editor_build_renamed_directory(screen, name.data,
                                               name.len, &new_relative)
         && ncm_fs_join(&new_path, music_dir, music_dir_len,
                        new_relative.data, new_relative.len);
    if (ok) {
        ncm_error_clear(&ncm_error);
        ok = ncm_fs_rename(old_path.data, old_path.len,
                           new_path.data, new_path.len, &ncm_error);
        if (!ok) {
            tag_editor_status_directory_rename_error(
                screen, pair->first, pair->first_len, &ncm_error);
        }
    }
    if (ok) {
        tag_editor_status_directory_renamed(screen, name.data, name.len);
        if (screen->hooks.update_directory) {
            screen->hooks.update_directory(
                screen->hooks.user, screen->current_dir.data,
                screen->current_dir.len);
        }
        (void)sb_set(&screen->highlighted_dir,
                     new_relative.data, new_relative.len);
        screen->directories_update_requested = true;
        tag_editor_update_titles(screen, true);
    }

    sb_free(&new_relative);
    sb_free(&new_path);
    sb_free(&old_path);
    sb_free(&name);
    return ok;
}

bool
tag_editor_screen_add_directory(TagEditorScreen *screen,
                                char *label, int32 label_len,
                                char *path, int32 path_len) {
    NcMenuStringPair pair;
    StrBuilder first = {0};
    StrBuilder second = {0};
    bool ok;

    if (screen == NULL) {
        return false;
    }
    pair = (NcMenuStringPair){0};
    ok = (sb_set(&first, label, label_len) >= 0)
         && (sb_set(&second, path, path_len) >= 0);
    if (ok) {
        pair.first = sb_steal(&first, &pair.first_len, &pair.first_cap);
        pair.second = sb_steal(&second, &pair.second_len, &pair.second_cap);
        nc_editor_pair_menu_add(&screen->directories, &pair);
        screen->last_known_directory_count = nc_menu_item_count(
            nc_editor_pair_menu_base(&screen->directories));
        tag_editor_update_titles(screen, true);
    }
    sb_free(&second);
    sb_free(&first);
    nc_menu_string_pair_destroy(&pair);
    return ok;
}

bool
tag_editor_screen_load_songs(TagEditorScreen *screen,
                             NcmSongArray *songs) {
    char *path;
    int32 path_len;

    if ((screen == NULL) || (songs == NULL)) {
        return false;
    }
    nc_menu_clear_items(nc_tag_row_menu_base(&screen->tags));
    for (int32 i = 0; i < songs->len; i += 1) {
        NcmMutableSong mutable_song;
        bool ok;

        mutable_song = (NcmMutableSong){0};
        ok = ncm_mutable_song_load_originals_from_song(&mutable_song,
                                                       &songs->items[i]);
        if (ok) {
            ok = tag_editor_screen_add_mutable_song(screen, &mutable_song);
        }
        ncm_mutable_song_destroy(&mutable_song);
        if (!ok) {
            return false;
        }
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
    return true;
}

bool
tag_editor_screen_add_mutable_song(TagEditorScreen *screen,
                                   NcmMutableSong *song) {
    if ((screen == NULL) || (song == NULL)) {
        return false;
    }
    nc_tag_row_menu_add(&screen->tags, song);
    screen->last_known_tag_count = nc_menu_item_count(
        nc_tag_row_menu_base(&screen->tags));
    tag_editor_update_titles(screen, true);
    return true;
}

bool
tag_editor_screen_selected_songs(TagEditorScreen *screen,
                                 NcmSongArray *songs) {
    NcMenu *menu;

    if ((screen == NULL) || (songs == NULL)) {
        return false;
    }
    ncm_song_array_clear(songs);
    if (screen->active_focus != TAG_EDITOR_FOCUS_TAGS) {
        return false;
    }

    menu = nc_tag_row_menu_base(&screen->tags);
    if (!nc_menu_has_selected(menu)) {
        if (nc_menu_empty(menu)) {
            return true;
        }
        return tag_editor_copy_selected_song_at(
            screen, songs, nc_menu_highlight(menu));
    }

    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (!nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!tag_editor_copy_selected_song_at(screen, songs, i)) {
            ncm_song_array_clear(songs);
            return false;
        }
    }
    return true;
}

bool
tag_editor_screen_previous_column_available(TagEditorScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (screen->active_focus == TAG_EDITOR_FOCUS_TAGS) {
        return !nc_menu_empty(nc_editor_string_menu_base(&screen->tag_types));
    }
    if (screen->active_focus == TAG_EDITOR_FOCUS_TAG_TYPES) {
        if (nc_menu_empty(nc_editor_pair_menu_base(&screen->directories))) {
            return false;
        }
        return true;
    }
    if (tag_editor_focus_is_parser_helper(screen->active_focus)) {
        return !nc_menu_empty(nc_editor_string_menu_base(
            &screen->parser_actions));
    }
    return false;
}

bool
tag_editor_screen_next_column_available(TagEditorScreen *screen) {
    NcMenu *tag_types;

    if (screen == NULL) {
        return false;
    }
    if (screen->active_focus == TAG_EDITOR_FOCUS_DIRECTORIES) {
        return !nc_menu_empty(nc_editor_string_menu_base(&screen->tag_types))
               && !nc_menu_empty(nc_tag_row_menu_base(&screen->tags));
    }
    if (screen->active_focus == TAG_EDITOR_FOCUS_TAG_TYPES) {
        tag_types = nc_editor_string_menu_base(&screen->tag_types);
        return !nc_menu_empty(nc_tag_row_menu_base(&screen->tags))
               && tag_editor_tag_type_choice_is_editable(
                   nc_menu_highlight(tag_types));
    }
    if (screen->active_focus == TAG_EDITOR_FOCUS_PARSER_ACTIONS) {
        return true;
    }
    return false;
}

void
tag_editor_screen_previous_column(TagEditorScreen *screen) {
    if (!tag_editor_screen_previous_column_available(screen)) {
        return;
    }
    if (screen->active_focus == TAG_EDITOR_FOCUS_TAGS) {
        tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_TAG_TYPES);
    } else if (screen->active_focus == TAG_EDITOR_FOCUS_TAG_TYPES) {
        if (tag_editor_has_modified_songs(screen)
            && !tag_editor_confirm(
                screen, STRLIT("There are pending changes, "
                                    "are you sure?"))) {
            return;
        }
        tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_DIRECTORIES);
    } else if (tag_editor_focus_is_parser_helper(screen->active_focus)) {
        tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_PARSER_ACTIONS);
    }
    tag_editor_finish_tag_type_change(screen, false);
    return;
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

bool
tag_editor_screen_apply_tag_to_selection(TagEditorScreen *screen,
                                         enum NcmTagsField field,
                                         char *value, int32 value_len,
                                         char *separator,
                                         int32 separator_len) {
    TagSetter setter;

    if (screen == NULL) {
        return false;
    }
    setter.field = field;
    setter.value = value;
    setter.value_len = value_len;
    setter.separator = separator;
    setter.separator_len = separator_len;
    return tag_editor_for_each_target(screen, tag_editor_set_song_tag_callback,
                                      &setter);
}

bool
tag_editor_screen_number_tracks(TagEditorScreen *screen,
                                bool extended) {
    TrackNumberer numberer;
    NcMenu *menu;

    if (screen == NULL) {
        return false;
    }
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

void
tag_editor_screen_capitalize_first_letters(TagEditorScreen *screen) {
    (void)tag_editor_for_each_target(screen,
                                     tag_editor_capitalize_song_callback,
                                     NULL);
    return;
}

void
tag_editor_screen_lower_all_letters(TagEditorScreen *screen) {
    (void)tag_editor_for_each_target(screen, tag_editor_lower_song_callback,
                                     NULL);
    return;
}

void
tag_editor_screen_clear_modifications(TagEditorScreen *screen) {
    NcMenu *menu;

    if (screen == NULL) {
        return;
    }
    menu = nc_tag_row_menu_base(&screen->tags);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmMutableSong *song;

        if ((song = nc_menu_active_item_at(menu, i))) {
            ncm_mutable_song_clear_modifications(song);
        }
    }
    return;
}

bool
tag_editor_screen_save_modified(TagEditorScreen *screen,
                                char *music_dir) {
    SaveContext context = {0};
    bool iterated;

    if (screen == NULL) {
        return false;
    }

    tag_editor_status_message(screen, STRLIT("Writing changes..."));

    context.screen = screen;
    context.music_dir = music_dir;
    context.ok = true;
    context.shared_directory = (StrBuilder){0};

    iterated = tag_editor_for_each_target(
        screen, tag_editor_save_song_callback, &context);
    if (!iterated || !context.ok) {
        sb_free(&context.shared_directory);
        tag_editor_screen_clear_stale_tags(screen);
        return false;
    }

    tag_editor_status_message(screen, STRLIT("Tags updated"));
    nc_menu_reset(nc_editor_string_menu_base(&screen->tag_types));
    tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_DIRECTORIES);
    if (context.shared_directory_valid) {
        tag_editor_update_modified_directory(
            screen, &context.shared_directory);
    }
    sb_free(&context.shared_directory);
    return context.target_count > 0;
}

bool
tag_editor_screen_save_action_available(TagEditorScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->active_focus == TAG_EDITOR_FOCUS_TAG_TYPES;
}

bool
tag_editor_screen_apply_directory_filter(TagEditorScreen *screen,
                                         char *pattern, int32 pattern_len,
                                         uint32 regex_flags,
                                         NcmError *ncm_error) {
    if (screen == NULL) {
        return false;
    }
    if (!tag_editor_compile_constraint(&screen->directory_filter_regex,
                                       pattern, pattern_len, regex_flags,
                                       ncm_error)) {
        return false;
    }
    sb_set(&screen->directory_filter_constraint, pattern, pattern_len);
    nc_menu_set_display_callbacks(
        nc_editor_pair_menu_base(&screen->directories),
        tag_editor_directory_display_callbacks(screen));
    screen->directory_filter_enabled = true;
    nc_menu_apply_filter(nc_editor_pair_menu_base(&screen->directories));
    tag_editor_update_titles(screen, true);
    return true;
}

bool
tag_editor_screen_apply_tag_filter(TagEditorScreen *screen,
                                   char *pattern, int32 pattern_len,
                                   uint32 regex_flags, NcmError *ncm_error) {
    if (screen == NULL) {
        return false;
    }
    if (!tag_editor_compile_constraint(&screen->tag_filter_regex, pattern,
                                       pattern_len, regex_flags, ncm_error)) {
        return false;
    }
    sb_set(&screen->tag_filter_constraint, pattern, pattern_len);
    nc_menu_set_display_callbacks(nc_tag_row_menu_base(&screen->tags),
                                  tag_editor_tag_display_callbacks(screen));
    screen->tag_filter_enabled = true;
    nc_menu_apply_filter(nc_tag_row_menu_base(&screen->tags));
    tag_editor_update_titles(screen, true);
    return true;
}

bool
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
    bool result;

    if ((screen == NULL) || (pattern == NULL) || (pattern_len <= 0)) {
        return false;
    }
    if ((screen->active_focus != TAG_EDITOR_FOCUS_DIRECTORIES)
        && (screen->active_focus != TAG_EDITOR_FOCUS_TAGS)) {
        return false;
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
    if (!tag_editor_compile_constraint(regex, pattern, pattern_len,
                                       Config.regex_flags, ncm_error)) {
        return false;
    }
    sb_set(constraint, pattern, pattern_len);
    *enabled = true;

    menu = tag_editor_screen_active_menu(screen);
    window = tag_editor_screen_active_window(screen);
    context.screen = screen;
    context.regex = regex;
    result = nc_menu_search_selectable(menu, nc_window_height(window),
                                       forward, wrap, skip_current,
                                       tag_editor_search_position,
                                       &context, NULL);
    if (result) {
        tag_editor_screen_finish_directory_change(screen);
    }
    return result;
}

static bool
tag_editor_search_position(NcMenu *menu, int32 pos, void *user) {
    TagEditorSearchContext *context = user;

    return tag_editor_active_item_matches(context->screen, menu, pos,
                                          context->regex);
}

static void
tag_editor_reset_parser_navigation(TagEditorScreen *screen) {
    ASSERT(screen != NULL);
    nc_menu_reset(nc_editor_string_menu_base(&screen->parser_dialog));
    nc_menu_reset(nc_editor_string_menu_base(&screen->parser_rows));
    nc_menu_reset(nc_editor_string_menu_base(&screen->parser_actions));
    return;
}

bool
tag_editor_screen_prepare_parser_rows(TagEditorScreen *screen,
                                      enum TagEditorParserMode mode,
                                      char *pattern, int32 pattern_len) {
    if (screen == NULL) {
        return false;
    }
    screen->parser_mode = mode;
    if (pattern) {
        if (!tag_editor_set_pattern(screen, pattern, pattern_len)) {
            return false;
        }
    } else if ((mode != TAG_EDITOR_PARSER_NONE)
               && (screen->pattern.len <= 0)
               && Config.pattern) {
        if (!tag_editor_set_pattern(screen, Config.pattern,
                                    Config.pattern_len)) {
            return false;
        }
    }

    nc_menu_clear_items(nc_editor_string_menu_base(&screen->parser_dialog));
    nc_menu_clear_items(nc_editor_string_menu_base(&screen->parser_rows));
    nc_menu_clear_items(nc_editor_string_menu_base(&screen->parser_actions));
    if (!tag_editor_append_parser_row(
        &screen->parser_dialog, STRLIT("Get tags from filename"),
        NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    if (!tag_editor_append_parser_row(
        &screen->parser_dialog, STRLIT("Rename files"),
        NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    if (!tag_editor_append_parser_row(
        &screen->parser_dialog, STRLIT("Cancel"),
        NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    if (mode == TAG_EDITOR_PARSER_NONE) {
        tag_editor_reset_parser_navigation(screen);
        return true;
    }
    if (!tag_editor_append_parser_row(
        &screen->parser_rows, STRLIT("Get tags from filename"),
        NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    if (!tag_editor_append_parser_row(
        &screen->parser_rows, STRLIT("Rename files"),
        NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    if (!tag_editor_append_parser_row(
        &screen->parser_rows, STRLIT("Cancel"),
        NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    if (!tag_editor_build_parser_menus(screen)
        || !tag_editor_build_parser_legend(screen)) {
        return false;
    }
    tag_editor_reset_parser_navigation(screen);
    return true;
}

void
tag_editor_screen_show_parser_dialog(TagEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    if (nc_menu_empty(nc_editor_string_menu_base(&screen->parser_dialog))) {
        (void)tag_editor_screen_prepare_parser_rows(
            screen, TAG_EDITOR_PARSER_NONE, NULL, 0);
    }
    screen->parser_mode = TAG_EDITOR_PARSER_NONE;
    tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_PARSER_CHOICE);
    return;
}

void
tag_editor_screen_show_parser_actions(TagEditorScreen *screen,
                                      enum TagEditorParserMode mode) {
    if ((screen == NULL) || (mode == TAG_EDITOR_PARSER_NONE)) {
        return;
    }
    if (!tag_editor_load_recent_patterns(screen)) {
        return;
    }
    if ((screen->pattern.len <= 0) && (screen->recent_patterns.len > 0)) {
        StrBuilder *pattern;

        pattern = &screen->recent_patterns.items[0];
        (void)tag_editor_set_pattern(screen, pattern->data, pattern->len);
    }
    (void)tag_editor_screen_prepare_parser_rows(
        screen, mode, screen->pattern.data, screen->pattern.len);
    (void)tag_editor_build_parser_legend(screen);
    screen->parser_preview_enabled = false;
    tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_PARSER_ACTIONS);
    return;
}

void
tag_editor_screen_show_parser_legend(TagEditorScreen *screen) {
    if ((screen == NULL)
        || (screen->parser_mode == TAG_EDITOR_PARSER_NONE)) {
        return;
    }
    tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_PARSER_LEGEND);
    return;
}

void
tag_editor_screen_show_parser_preview(TagEditorScreen *screen) {
    if ((screen == NULL)
        || (screen->parser_mode == TAG_EDITOR_PARSER_NONE)) {
        return;
    }
    tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_PARSER_PREVIEW);
    return;
}

void
tag_editor_screen_close_parser(TagEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->parser_mode = TAG_EDITOR_PARSER_NONE;
    tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_TAG_TYPES);
    return;
}

bool
tag_editor_parse_filename(NcmMutableSong *song, char *mask,
                          int32 mask_len, bool preview,
                          StrBuilder *preview_buffer) {
    StrBuilder file = {0};
    int32 mask_pos;
    int32 file_pos;
    int32 percent_pos;
    int32 name_len;
    char tag_char;

    if ((song == NULL) || (mask == NULL) || (mask_len < 0)) {
        return false;
    }
    if (song->name == NULL) {
        sb_free(&file);
        return false;
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
            return false;
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
                return false;
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
                SB_APPEND(preview_buffer, STRLIT(": "));
                SB_APPEND(preview_buffer, file.data + file_pos,
                          value_end - file_pos);
                sb_append_byte(preview_buffer, '\n');
            } else if (!ncm_mutable_song_set_tags(song, field,
                                                  file.data + file_pos,
                                                  value_end - file_pos,
                                                  NULL, 0)) {
                sb_free(&file);
                return false;
            }
        }
        file_pos = value_end;
        mask_pos = percent_pos + 2;
    }
    sb_free(&file);
    return true;
}

bool
tag_editor_generate_filename(NcmMutableSong *song, char *pattern,
                             int32 pattern_len,
                             StrBuilder *filename) {
    NcmFormatAst ast;
    NcmSong format_song;
    StrBuilder rendered;
    NcmError ncm_error;

    if ((song == NULL) || (filename == NULL) || (pattern_len < 0)) {
        return false;
    }
    if ((pattern == NULL) && (pattern_len > 0)) {
        return false;
    }

    ast = (NcmFormatAst){0};
    format_song = (NcmSong){0};
    ncm_error_clear(&ncm_error);
    if (!ncm_format_parse(&ast, pattern, pattern_len,
                          NCM_FORMAT_FLAG_TAG, &ncm_error)) {
        ncm_error_clear(&ncm_error);
        ncm_format_ast_destroy(&ast);
        ncm_song_destroy(&format_song);
        return false;
    }
    if (!tag_editor_mutable_song_to_format_song(song, &format_song)) {
        ncm_format_ast_destroy(&ast);
        ncm_song_destroy(&format_song);
        return false;
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
    return true;
}

bool
tag_editor_song_display_value(NcmMutableSong *song,
                              enum NcmTagsField field,
                              StrBuilder *buffer) {
    if ((song == NULL) || (buffer == NULL)) {
        return false;
    }
    if (field == NCM_TAGS_FIELD_COUNT) {
        SB_APPEND(buffer, song->name, song->name_len);
        if (song->new_name && (song->new_name_len > 0)) {
            SB_APPEND(buffer, STRLIT(" -> "));
            SB_APPEND(buffer, song->new_name, song->new_name_len);
        }
        return true;
    }
    return tag_editor_mutable_song_get_field(song, field, buffer);
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
tag_editor_refresh(NcScreen *screen) {
    TagEditorScreen *editor;

    if ((editor = tag_editor_from_screen(screen)) == NULL) {
        return;
    }
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
    tag_editor_draw_separators(editor);
    tag_editor_refresh_menu(&editor->tag_types_window,
                            nc_editor_string_menu_base(
                                &editor->tag_types));
    tag_editor_refresh_menu(&editor->tags_window,
                            nc_tag_row_menu_base(&editor->tags));
    return;
}

static void
tag_editor_refresh_window(NcScreen *screen) {
    TagEditorScreen *editor;
    NcMenu *menu;
    NcWindow *window;

    if ((editor = tag_editor_from_screen(screen)) == NULL) {
        return;
    }
    tag_editor_update_titles(editor, true);
    tag_editor_update_menu_highlights(editor);
    if (tag_editor_focus_is_parser_helper(editor->active_focus)) {
        tag_editor_refresh_active_helper(editor);
        return;
    }
    menu = tag_editor_screen_active_menu(editor);
    window = tag_editor_screen_active_window(editor);
    tag_editor_refresh_menu(window, menu);
    return;
}

static void
tag_editor_scroll(NcScreen *screen, enum NcScroll where) {
    TagEditorScreen *editor;
    NcMenu *menu;
    NcWindow *window;

    if ((editor = tag_editor_from_screen(screen)) == NULL) {
        return;
    }
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

static bool
tag_editor_can_run_current(NcScreen *screen) {
    TagEditorScreen *editor;
    NcMenu *menu;
    enum NcmTagsField field;

    if ((editor = tag_editor_from_screen(screen)) == NULL) {
        return false;
    }

    switch (editor->active_focus) {
    case TAG_EDITOR_FOCUS_DIRECTORIES:
    case TAG_EDITOR_FOCUS_PARSER_CHOICE:
        menu = tag_editor_screen_active_menu(editor);
        return menu && nc_menu_current_is_selectable(menu);
    case TAG_EDITOR_FOCUS_PARSER_ACTIONS:
        if (((menu = tag_editor_screen_active_menu(editor)) == NULL)
            || !nc_menu_current_is_selectable(menu)) {
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
        if (nc_menu_empty(nc_tag_row_menu_base(&editor->tags))) {
            return false;
        }
        return tag_editor_current_tag_type_action(editor, &field)
               != TAG_EDITOR_TAG_TYPE_ACTION_NONE;
    case TAG_EDITOR_FOCUS_TAGS:
        if (nc_menu_empty(nc_tag_row_menu_base(&editor->tags))) {
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

static bool
tag_editor_run_current(NcScreen *screen) {
    TagEditorScreen *editor;

    if ((editor = tag_editor_from_screen(screen)) == NULL) {
        return false;
    }

    switch (editor->active_focus) {
    case TAG_EDITOR_FOCUS_DIRECTORIES:
        return tag_editor_run_directory_current(editor);
    case TAG_EDITOR_FOCUS_TAG_TYPES:
        return tag_editor_run_tag_type_current(editor);
    case TAG_EDITOR_FOCUS_TAGS:
        return tag_editor_run_tag_current(editor);
    case TAG_EDITOR_FOCUS_PARSER_CHOICE:
        return tag_editor_run_parser_choice_current(editor);
    case TAG_EDITOR_FOCUS_PARSER_ACTIONS:
        return tag_editor_run_parser_action_current(editor);
    case TAG_EDITOR_FOCUS_PARSER_LEGEND:
    case TAG_EDITOR_FOCUS_PARSER_PREVIEW:
        return false;
    case TAG_EDITOR_FOCUS_COUNT:
    default:
        break;
    }
    return false;
}

static void
tag_editor_switch_to(NcScreen *screen) {
    (void)nc_screen_switcher_finish_switch(screen);
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
tag_editor_update(NcScreen *screen) {
    TagEditorScreen *editor = tag_editor_from_screen(screen);
    bool changed;

    tag_editor_screen_finish_directory_change(editor);
    changed = tag_editor_update_from_mpd(editor, &global_mpd);
    nc_screen_clear_update_request(screen);
    if (changed && app_controller_is_screen_visible(screen)) {
        nc_screen_refresh(screen);
    }
    return;
}

static void
tag_editor_mouse_callback(NcScreen *screen, MEVENT event) {
    TagEditorScreen *editor;
    int32 x;
    int32 y;

    if ((editor = tag_editor_from_screen(screen)) == NULL) {
        return;
    }

    if (!tag_editor_focus_is_main(editor->active_focus)) {
        x = event.x;
        y = event.y;
        if (nc_window_has_coords(&editor->parser_dialog_window, &x, &y)) {
            if (!tag_editor_mouse_move_to_parser_focus(
                editor, TAG_EDITOR_FOCUS_PARSER_CHOICE)) {
                return;
            }
            if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
                (void)tag_editor_mouse_select_parser_dialog(
                    editor, y, (event.bstate & BUTTON3_PRESSED) != 0);
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
                (void)tag_editor_mouse_select_parser_row(
                    editor, y, (event.bstate & BUTTON3_PRESSED) != 0);
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
            (void)tag_editor_mouse_select_directory(
                editor, y, (event.bstate & BUTTON1_PRESSED) != 0);
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
            (void)tag_editor_mouse_select_tag_type(
                editor, y, (event.bstate & BUTTON3_PRESSED) != 0);
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
            (void)tag_editor_mouse_select_tag(
                editor, y, (event.bstate & BUTTON3_PRESSED) != 0);
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

static bool
tag_editor_mouse_select_directory(TagEditorScreen *screen,
                                  int32 y, bool enter) {
    NcMenu *menu;

    ASSERT(screen != NULL);
    menu = nc_editor_pair_menu_base(&screen->directories);
    if ((y < 0) || (y >= nc_menu_item_count(menu))) {
        return false;
    }
    if (!nc_menu_goto_selectable(menu, y)) {
        return false;
    }
    tag_editor_screen_finish_directory_change(screen);
    if (enter) {
        return tag_editor_screen_enter_directory(screen);
    }
    return true;
}

static bool
tag_editor_mouse_select_tag_type(TagEditorScreen *screen,
                                 int32 y, bool run) {
    NcMenu *menu;

    ASSERT(screen != NULL);
    menu = nc_editor_string_menu_base(&screen->tag_types);
    if ((y < 0) || (y >= nc_menu_item_count(menu))) {
        return false;
    }
    if (!nc_menu_goto_selectable(menu, y)) {
        return false;
    }
    tag_editor_finish_tag_type_change(screen, true);
    if (run) {
        return tag_editor_run_current_action(screen);
    }
    return true;
}

static bool
tag_editor_mouse_select_tag(TagEditorScreen *screen,
                            int32 y, bool run) {
    NcMenu *menu;

    ASSERT(screen != NULL);
    menu = nc_tag_row_menu_base(&screen->tags);
    if ((y < 0) || (y >= nc_menu_item_count(menu))) {
        return false;
    }
    if (!nc_menu_goto_selectable(menu, y)) {
        return false;
    }
    if (run) {
        return tag_editor_run_current_action(screen);
    }
    return true;
}

static bool
tag_editor_mouse_select_parser_dialog(TagEditorScreen *screen,
                                      int32 y, bool run) {
    NcMenu *menu;

    ASSERT(screen != NULL);
    menu = nc_editor_string_menu_base(&screen->parser_dialog);
    if ((y < 0) || (y >= nc_menu_item_count(menu))) {
        return false;
    }
    if (!nc_menu_goto_selectable(menu, y)) {
        return false;
    }
    if (run) {
        return tag_editor_run_current_action(screen);
    }
    return true;
}

static bool
tag_editor_mouse_select_parser_row(TagEditorScreen *screen,
                                   int32 y, bool run) {
    NcMenu *menu;

    ASSERT(screen != NULL);
    menu = nc_editor_string_menu_base(&screen->parser_actions);
    if ((y < 0) || (y >= nc_menu_item_count(menu))) {
        return false;
    }
    if (!nc_menu_goto_selectable(menu, y)) {
        return false;
    }
    if (run) {
        return tag_editor_run_current_action(screen);
    }
    return true;
}

static bool
tag_editor_run_current_action(TagEditorScreen *screen) {
    ASSERT(screen != NULL);
    return nc_screen_run_current(tag_editor_screen_base(screen));
}

static bool
tag_editor_mouse_move_to_column(TagEditorScreen *screen,
                                enum TagEditorColumn column) {
    ASSERT(screen != NULL);
    if (!tag_editor_focus_is_main(screen->active_focus)) {
        return false;
    }
    if (tag_editor_focus_is_main_column(screen->active_focus, column)) {
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
tag_editor_run_directory_current(TagEditorScreen *screen) {
    return tag_editor_screen_enter_directory(screen);
}

static bool
tag_editor_run_tag_type_current(TagEditorScreen *screen) {
    enum TagEditorTagTypeAction action;
    enum NcmTagsField field;

    action = tag_editor_current_tag_type_action(screen, &field);
    switch (action) {
    case TAG_EDITOR_TAG_TYPE_ACTION_FIELD:
        return tag_editor_prompt_tag_value(screen, field, true);
    case TAG_EDITOR_TAG_TYPE_ACTION_NUMBER_TRACKS:
        if (!tag_editor_confirm(screen, STRLIT("Number tracks?"))) {
            return false;
        }
        if (!tag_editor_screen_number_tracks(
            screen, Config.tag_editor_extended_numeration)) {
            return false;
        }
        tag_editor_status_message(screen, STRLIT("Tracks numbered"));
        return true;
    case TAG_EDITOR_TAG_TYPE_ACTION_FILENAME:
        tag_editor_screen_show_parser_dialog(screen);
        return true;
    case TAG_EDITOR_TAG_TYPE_ACTION_CAPITALIZE:
        tag_editor_status_message(screen, STRLIT("Processing..."));
        tag_editor_screen_capitalize_first_letters(screen);
        tag_editor_status_message(screen, STRLIT("Done"));
        return true;
    case TAG_EDITOR_TAG_TYPE_ACTION_LOWER:
        tag_editor_status_message(screen, STRLIT("Processing..."));
        tag_editor_screen_lower_all_letters(screen);
        tag_editor_status_message(screen, STRLIT("Done"));
        return true;
    case TAG_EDITOR_TAG_TYPE_ACTION_RESET:
        tag_editor_screen_clear_modifications(screen);
        tag_editor_status_message(screen, STRLIT("Changes reset"));
        return true;
    case TAG_EDITOR_TAG_TYPE_ACTION_SAVE:
        return tag_editor_screen_save_modified(
            screen, Config.mpd_music_dir);
    case TAG_EDITOR_TAG_TYPE_ACTION_NONE:
        return false;
    case TAG_EDITOR_TAG_TYPE_ACTION_COUNT:
    default:
        break;
    }
    return false;
}

static bool
tag_editor_run_tag_current(TagEditorScreen *screen) {
    enum TagEditorTagTypeAction action;
    enum NcmTagsField field;
    NcMenu *tags;
    bool result;

    action = tag_editor_current_tag_type_action(screen, &field);
    if (action == TAG_EDITOR_TAG_TYPE_ACTION_FIELD) {
        result = tag_editor_prompt_tag_value(screen, field, false);
    } else if (action == TAG_EDITOR_TAG_TYPE_ACTION_FILENAME) {
        result = tag_editor_prompt_current_filename(screen);
    } else {
        return false;
    }

    if (result) {
        tags = nc_tag_row_menu_base(&screen->tags);
        nc_menu_scroll_selectable(tags, nc_window_height(&screen->tags_window),
                                  NC_SCROLL_DOWN);
    }
    return result;
}

static bool
tag_editor_run_parser_choice_current(TagEditorScreen *screen) {
    NcMenu *menu;
    int32 choice;

    menu = nc_editor_string_menu_base(&screen->parser_dialog);
    if (!nc_menu_current_is_selectable(menu)) {
        return false;
    }
    choice = nc_menu_highlight(menu);
    if (choice == 0) {
        tag_editor_screen_show_parser_actions(
            screen, TAG_EDITOR_PARSER_TAGS_FROM_FILENAME);
        return true;
    }
    if (choice == 1) {
        tag_editor_screen_show_parser_actions(
            screen, TAG_EDITOR_PARSER_RENAME_FILES);
        return true;
    }
    if (choice == 2) {
        tag_editor_screen_close_parser(screen);
        return true;
    }
    return false;
}

static bool
tag_editor_run_parser_action_current(TagEditorScreen *screen) {
    NcMenu *menu;
    int32 choice;
    bool success;

    menu = nc_editor_string_menu_base(&screen->parser_actions);
    if (!nc_menu_current_is_selectable(menu)) {
        return false;
    }
    choice = nc_menu_highlight(menu);
    if (choice == TAG_EDITOR_PARSER_ACTION_PATTERN) {
        return tag_editor_prompt_pattern(screen);
    }
    if (choice == TAG_EDITOR_PARSER_ACTION_PREVIEW) {
        if (!tag_editor_build_parser_preview(screen, false, &success)) {
            return false;
        }
        tag_editor_screen_show_parser_preview(screen);
        tag_editor_status_message(screen, STRLIT("Operation finished"));
        return true;
    }
    if (choice == TAG_EDITOR_PARSER_ACTION_LEGEND) {
        if (!tag_editor_build_parser_legend(screen)) {
            return false;
        }
        tag_editor_screen_show_parser_legend(screen);
        return true;
    }
    if (choice == TAG_EDITOR_PARSER_ACTION_PROCEED) {
        if (!tag_editor_build_parser_preview(screen, true, &success)) {
            return false;
        }
        if (success) {
            if (!tag_editor_move_pattern_to_front(
                screen, screen->pattern.data, screen->pattern.len)) {
                return false;
            }
            (void)tag_editor_save_recent_patterns(screen);
            tag_editor_status_message(screen,
                                      STRLIT("Operation finished"));
            tag_editor_screen_close_parser(screen);
        }
        return success;
    }
    if (choice == TAG_EDITOR_PARSER_ACTION_CANCEL) {
        (void)tag_editor_save_recent_patterns(screen);
        tag_editor_screen_close_parser(screen);
        return true;
    }
    if (choice >= (int32)TAG_EDITOR_PARSER_ACTION_RECENT_START) {
        return tag_editor_apply_recent_pattern(screen, choice);
    }
    return false;
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
    if ((song = nc_tag_row_menu_current(&screen->tags)) == NULL) {
        return false;
    }

    label = ncm_tags_field_name(field);
    label_len = strlen32(label);
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
        result = tag_editor_screen_apply_tag_to_selection(
            screen, field, input.data, input.len, Config.tags_separator,
            Config.tags_separator_len);
    } else {
        result = ncm_mutable_song_set_tags(
            song, field, input.data, input.len, Config.tags_separator,
            Config.tags_separator_len);
    }
    sb_free(&input);
    return result;
}

static bool
tag_editor_prompt_current_filename(TagEditorScreen *screen) {
    NcmMutableSong *song;
    NcmStringView current_name;
    NcmStringView initial;
    StrBuilder input = {0};
    enum TagEditorPromptResult prompt_result;
    int32 dot = -1;
    bool result;

    ASSERT(screen != NULL);
    if ((song = nc_tag_row_menu_current(&screen->tags)) == NULL) {
        return false;
    }

    if (!ncm_mutable_song_get_new_name(song, &current_name)) {
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

    if (screen->hooks.prompt == NULL) {
        prompt_result = TAG_EDITOR_PROMPT_ERROR;
    } else {
        prompt_result = screen->hooks.prompt(
            screen->hooks.user, STRLIT("New filename"), initial,
            &input);
    }
    if (prompt_result == TAG_EDITOR_PROMPT_ABORTED) {
        tag_editor_status_message(screen, STRLIT("Action aborted"));
        sb_free(&input);
        return false;
    }
    if (prompt_result != TAG_EDITOR_PROMPT_ACCEPTED) {
        sb_free(&input);
        return false;
    }
    if (input.len <= 0) {
        sb_free(&input);
        return true;
    }
    result = tag_editor_set_song_filename_stem(song, input.data, input.len);
    sb_free(&input);
    return result;
}

static bool
tag_editor_set_song_filename_stem(NcmMutableSong *song, char *stem,
                                  int32 stem_len) {
    NcmStringView current_name;
    StrBuilder new_name = {0};
    int32 dot = -1;
    bool result;

    ASSERT(song != NULL);
    ASSERT(stem != NULL);
    ASSERT(stem_len > 0);
    if (!ncm_mutable_song_get_new_name(song, &current_name)) {
        current_name.data = song->name;
        current_name.len = song->name_len;
    }

    for (int32 i = 0; i < current_name.len; i += 1) {
        if (current_name.data[i] == '.') {
            dot = i;
        }
    }

    SB_APPEND(&new_name, stem, stem_len);
    if (dot >= 0) {
        SB_APPEND(&new_name, current_name.data + dot,
                  current_name.len - dot);
    }
    result = ncm_mutable_song_set_new_name(song, new_name.data,
                                           new_name.len);
    sb_free(&new_name);
    return result;
}

static bool
tag_editor_focus_is_main(enum TagEditorFocus focus) {
    return (focus == TAG_EDITOR_FOCUS_DIRECTORIES)
           || (focus == TAG_EDITOR_FOCUS_TAG_TYPES)
           || (focus == TAG_EDITOR_FOCUS_TAGS);
}

static bool
tag_editor_focus_is_main_column(enum TagEditorFocus focus,
                                enum TagEditorColumn column) {
    return focus == tag_editor_column_focus(column);
}

static bool
tag_editor_focus_is_parser_helper(enum TagEditorFocus focus) {
    return (focus == TAG_EDITOR_FOCUS_PARSER_LEGEND)
           || (focus == TAG_EDITOR_FOCUS_PARSER_PREVIEW);
}

static enum TagEditorFocus
tag_editor_column_focus(enum TagEditorColumn column) {
    switch (column) {
    case TAG_EDITOR_COLUMN_DIRECTORIES:
        return TAG_EDITOR_FOCUS_DIRECTORIES;
    case TAG_EDITOR_COLUMN_TAG_TYPES:
        return TAG_EDITOR_FOCUS_TAG_TYPES;
    case TAG_EDITOR_COLUMN_TAGS:
        return TAG_EDITOR_FOCUS_TAGS;
    case TAG_EDITOR_COLUMN_COUNT:
    default:
        break;
    }
    return TAG_EDITOR_FOCUS_DIRECTORIES;
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

static enum TagEditorFocus
tag_editor_current_helper_focus(TagEditorScreen *screen) {
    ASSERT(screen != NULL);
    if (!screen->parser_preview_enabled) {
        return TAG_EDITOR_FOCUS_PARSER_LEGEND;
    }
    return TAG_EDITOR_FOCUS_PARSER_PREVIEW;
}

static bool
tag_editor_tag_type_row_changed(TagEditorScreen *screen) {
    NcMenu *menu;
    int32 highlight;
    bool changed;

    ASSERT(screen != NULL);

    menu = nc_editor_string_menu_base(&screen->tag_types);
    highlight = nc_menu_highlight(menu);
    changed = screen->last_tag_type_highlight != highlight;
    screen->last_tag_type_highlight = highlight;

    return changed;
}

static void
tag_editor_finish_tag_type_change(TagEditorScreen *screen,
                                  bool refresh_tags) {
    ASSERT(screen != NULL);
    if (screen->active_focus != TAG_EDITOR_FOCUS_TAG_TYPES) {
        return;
    }
    if (!tag_editor_tag_type_row_changed(screen)) {
        return;
    }
    if (refresh_tags) {
        tag_editor_refresh_menu(&screen->tags_window,
                                nc_tag_row_menu_base(&screen->tags));
    }
    return;
}



static void
tag_editor_destroy_callback(NcScreen *screen) {
    tag_editor_screen_destroy(tag_editor_from_screen(screen));
    return;
}

static void
tag_editor_initialize_buffers(TagEditorScreen *screen) {
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

    str_builder_array_init(&screen->recent_patterns);

    screen->directory_filter_constraint = (StrBuilder){0};
    screen->tag_filter_constraint = (StrBuilder){0};
    screen->directory_search_constraint = (StrBuilder){0};
    screen->tag_search_constraint = (StrBuilder){0};
    screen->pattern = (StrBuilder){0};
    return;
}

static void
tag_editor_destroy_buffers(TagEditorScreen *screen) {
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
    return;
}

static void
tag_editor_initialize_regexes(TagEditorScreen *screen) {
    screen->directory_filter_regex = (NcmRegex){0};
    screen->tag_filter_regex = (NcmRegex){0};
    screen->directory_search_regex = (NcmRegex){0};
    screen->tag_search_regex = (NcmRegex){0};
    return;
}

static void
tag_editor_destroy_regexes(TagEditorScreen *screen) {
    ncm_regex_destroy(&screen->tag_search_regex);
    ncm_regex_destroy(&screen->directory_search_regex);
    ncm_regex_destroy(&screen->tag_filter_regex);
    ncm_regex_destroy(&screen->directory_filter_regex);
    return;
}

static bool
tag_editor_compile_constraint(NcmRegex *regex, char *pattern,
                              int32 pattern_len, uint32 regex_flags,
                              NcmError *ncm_error) {
    NcmRegex compiled;

    ASSERT(regex != NULL);
    compiled = (NcmRegex){0};
    if (!ncm_regex_compile(&compiled, pattern, pattern_len, regex_flags,
                           ncm_error)) {
        ncm_regex_destroy(&compiled);
        return false;
    }
    ncm_regex_destroy(regex);
    *regex = compiled;
    return true;
}

static bool
tag_editor_set_buffer(StrBuilder *buffer, char *data, int32 data_len) {
    if (sb_set(buffer, data, data_len) < 0) {
        return false;
    }
    if (buffer->data) {
        buffer->data[buffer->len] = '\0';
    }
    return true;
}

static void
tag_editor_update_titles(TagEditorScreen *screen,
                         bool update_windows) {
    ASSERT(screen != NULL);

    tag_editor_update_visible_counts(screen);
    sb_clear(&screen->directories_title);
    sb_clear(&screen->tag_types_title);
    sb_clear(&screen->tags_title);
    sb_clear(&screen->parser_dialog_title);
    sb_clear(&screen->parser_title);
    sb_clear(&screen->parser_helper_title);

    if (Config.titles_visibility) {
        SB_APPEND(&screen->directories_title,
                  STRLIT("Directories"));
        SB_APPEND(&screen->tag_types_title,
                  STRLIT("Tag types"));
        SB_APPEND(&screen->tags_title, STRLIT("Tags"));
        if (screen->parser_mode
            == TAG_EDITOR_PARSER_TAGS_FROM_FILENAME) {
            SB_APPEND(&screen->parser_title,
                      STRLIT("Get tags from filename"));
        } else if (screen->parser_mode
                   == TAG_EDITOR_PARSER_RENAME_FILES) {
            SB_APPEND(&screen->parser_title,
                      STRLIT("Rename files"));
        } else {
            SB_APPEND(&screen->parser_title,
                      STRLIT("Pattern"));
        }
        if ((screen->active_focus == TAG_EDITOR_FOCUS_PARSER_LEGEND)
            || !screen->parser_preview_enabled) {
            SB_APPEND(&screen->parser_helper_title,
                      STRLIT("Legend"));
        } else {
            SB_APPEND(&screen->parser_helper_title,
                      STRLIT("Preview"));
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

static void
tag_editor_update_visible_counts(TagEditorScreen *screen) {
    ASSERT(screen != NULL);
    screen->last_known_directory_count = nc_menu_item_count(
        nc_editor_pair_menu_base(&screen->directories));
    screen->last_known_tag_count = nc_menu_item_count(
        nc_tag_row_menu_base(&screen->tags));
    return;
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

static bool
tag_editor_directory_row_changed(TagEditorScreen *screen) {
    char *path;
    int32 path_len;
    NcMenu *menu;
    bool changed;

    ASSERT(screen != NULL);
    menu = nc_editor_pair_menu_base(&screen->directories);
    if (!tag_editor_current_directory_path(screen, &path, &path_len)) {
        changed = screen->observed_dir_valid;
        tag_editor_observe_current_directory(screen);
        return changed;
    }
    changed = !screen->observed_dir_valid
              || !STREQUAL(screen->observed_dir.data,
                           screen->observed_dir.len, path,
                           path_len)
              || (screen->last_directory_highlight
                  != nc_menu_highlight(menu));
    if (changed) {
        tag_editor_observe_current_directory(screen);
    }
    return changed;
}

static bool
tag_editor_current_directory_path(TagEditorScreen *screen,
                                  char **path, int32 *path_len) {
    NcMenuStringPair *pair;

    if (path) {
        *path = NULL;
    }
    if (path_len) {
        *path_len = 0;
    }
    ASSERT(screen != NULL);
    if (((pair = nc_editor_pair_menu_current(&screen->directories)) == NULL)
        || (pair->second == NULL)) {
        return false;
    }
    if (path) {
        *path = pair->second;
    }
    if (path_len) {
        *path_len = pair->second_len;
    }
    return true;
}

static bool
tag_editor_directory_has_subdirectories(TagEditorScreen *screen,
                                        char *path, int32 path_len) {
    NcmDirectoryArray directories;
    NcmError ncm_error;
    bool result;

    (void)screen;
    if ((path == NULL) || (path_len <= 0)) {
        return false;
    }

    ncm_error_clear(&ncm_error);
    directories = (NcmDirectoryArray){0};
    result = ncm_mpd_client_get_directory_list(
        &global_mpd, path, &directories, &ncm_error)
        && (directories.len > 0);
    ncm_error_clear(&ncm_error);
    ncm_directory_array_destroy(&directories);
    return result;
}

static bool
tag_editor_directory_is_control(char *label, int32 label_len) {
    return STREQUAL(label, label_len, STRLIT("."))
           || STREQUAL(label, label_len, STRLIT(".."));
}

static bool
tag_editor_highlight_directory_path(TagEditorScreen *screen,
                                    char *path, int32 path_len) {
    NcMenu *menu;

    ASSERT(screen != NULL);
    if ((path == NULL) || (path_len <= 0)) {
        return false;
    }
    menu = nc_editor_pair_menu_base(&screen->directories);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcMenuStringPair *pair;

        if (((pair = nc_menu_active_item_at(menu, i)) == NULL)
            || (pair->second == NULL)) {
            continue;
        }
        if (STREQUAL(pair->second, pair->second_len,
                     path, path_len)) {
            (void)nc_menu_goto_selectable(menu, i);
            tag_editor_observe_current_directory(screen);
            return true;
        }
    }
    return false;
}

static bool
tag_editor_highlight_song_uri(TagEditorScreen *screen, char *uri,
                              int32 uri_len) {
    NcMenu *menu;

    ASSERT(screen != NULL);
    if ((uri == NULL) || (uri_len <= 0)) {
        return false;
    }
    menu = nc_tag_row_menu_base(&screen->tags);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmMutableSong *song;

        if (((song = nc_menu_active_item_at(menu, i)) == NULL)
            || (song->uri == NULL)) {
            continue;
        }
        if (STREQUAL(song->uri, song->uri_len, uri, uri_len)) {
            (void)nc_menu_goto_selectable(menu, i);
            return true;
        }
    }
    return false;
}

static bool
tag_editor_current_directory_pair(TagEditorScreen *screen,
                                  NcMenuStringPair **pair) {
    NcMenuStringPair *current;

    if (pair) {
        *pair = NULL;
    }
    ASSERT(screen != NULL);
    if (((current = nc_editor_pair_menu_current(&screen->directories)) == NULL)
        || (current->first == NULL)
        || (current->second == NULL)) {
        return false;
    }
    if (pair) {
        *pair = current;
    }
    return true;
}

static bool
tag_editor_build_renamed_directory(TagEditorScreen *screen,
                                   char *name, int32 name_len,
                                   StrBuilder *result) {
    ASSERT(screen != NULL);
    ASSERT(result != NULL);
    if ((name == NULL) || (name_len <= 0)) {
        return false;
    }
    return ncm_fs_join(result, screen->current_dir.data,
                       screen->current_dir.len, name, name_len);
}

static void
tag_editor_status_directory_renamed(TagEditorScreen *screen,
                                    char *name, int32 name_len) {
    StrBuilder message = {0};

    SB_APPEND(&message, STRLIT("Directory renamed to \""));
    SB_APPEND(&message, name, name_len);
    SB_APPEND(&message, STRLIT("\""));
    tag_editor_status_message(screen, message.data, message.len);
    sb_free(&message);
    return;
}

static void
tag_editor_status_directory_rename_error(TagEditorScreen *screen,
                                         char *name, int32 name_len,
                                         NcmError *ncm_error) {
    StrBuilder message;
    int32 error_len;

    SB_APPEND(&message, STRLIT("Couldn't rename \""));
    SB_APPEND(&message, name, name_len);
    SB_APPEND(&message, STRLIT("\": "));
    if (ncm_error && ncm_error_is_set(ncm_error)) {
        error_len = strlen32(ncm_error->message);
        SB_APPEND(&message, ncm_error->message, error_len);
    } else {
        SB_APPEND(&message, STRLIT("unknown error"));
    }
    tag_editor_status_message(screen, message.data, message.len);
    sb_free(&message);
    return;
}

static bool
tag_editor_has_modified_songs(TagEditorScreen *screen) {
    NcMenu *menu;

    ASSERT(screen != NULL);
    menu = nc_tag_row_menu_base(&screen->tags);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmMutableSong *song;

        if ((song = nc_menu_active_item_at(menu, i))
            && ncm_mutable_song_is_modified(song)) {
            return true;
        }
    }
    return false;
}

static void
tag_editor_preserve_current_directory(TagEditorScreen *screen,
                                      StrBuilder *path) {
    char *data;
    int32 data_len;

    ASSERT(screen != NULL);
    ASSERT(path != NULL);
    sb_clear(path);
    if (tag_editor_current_directory_path(screen, &data, &data_len)) {
        sb_set(path, data, data_len);
    }
    return;
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
        NcMenuStringPair *pair;

        if (((pair = nc_menu_active_item_at(menu, i)) == NULL)
            || (pair->second == NULL)) {
            continue;
        }
        if (STREQUAL(pair->second, pair->second_len,
                     path->data, path->len)) {
            (void)nc_menu_goto_selectable(menu, i);
            return;
        }
    }
    return;
}

static void
tag_editor_preserve_current_song(TagEditorScreen *screen,
                                 StrBuilder *uri) {
    NcmMutableSong *current;

    ASSERT(screen != NULL);
    ASSERT(uri != NULL);
    sb_clear(uri);
    if (((current = nc_tag_row_menu_current(&screen->tags)) == NULL)
        || (current->uri == NULL)
        || (current->uri_len <= 0)) {
        return;
    }
    sb_set(uri, current->uri, current->uri_len);
    return;
}

static void
tag_editor_restore_current_song(TagEditorScreen *screen,
                                StrBuilder *uri) {
    NcMenu *menu;

    ASSERT(screen != NULL);
    ASSERT(uri != NULL);
    if (uri->len <= 0) {
        return;
    }
    menu = nc_tag_row_menu_base(&screen->tags);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmMutableSong *item;

        if (((item = nc_menu_active_item_at(menu, i)) == NULL)
            || (item->uri == NULL)) {
            continue;
        }
        if (STREQUAL(item->uri, item->uri_len,
                     uri->data, uri->len)) {
            (void)nc_menu_goto_selectable(menu, i);
            return;
        }
    }
    return;
}

static bool
tag_editor_add_control_directory(TagEditorScreen *screen) {
    char *dir;
    int32 dir_len;
    int32 parent_len;

    ASSERT(screen != NULL);
    dir = screen->current_dir.data;
    dir_len = screen->current_dir.len;
    if ((dir == NULL) || (dir_len <= 0)
        || STREQUAL(dir, dir_len, STRLIT("/"))) {
        return tag_editor_screen_add_directory(
            screen, STRLIT("."), STRLIT("/"));
    }

    parent_len = ncm_string_parent_directory_len(dir, dir_len);
    if (parent_len <= 0) {
        return tag_editor_screen_add_directory(
            screen, STRLIT(".."), STRLIT("/"));
    }
    return tag_editor_screen_add_directory(
        screen, STRLIT(".."), dir, parent_len);
}

static void
tag_editor_sort_directories(NcmDirectoryArray *directories) {
    ASSERT(directories != NULL);
    for (int32 i = 1; i < directories->len; i += 1) {
        NcmDirectory current;
        int32 j;

        current = (NcmDirectory){0};
        ncm_directory_move(&current, &directories->items[i]);
        j = i;
        while ((j > 0)
               && (tag_editor_compare_directories(
                   &directories->items[j - 1], &current) > 0)) {
            ncm_directory_move(&directories->items[j],
                               &directories->items[j - 1]);
            j -= 1;
        }
        ncm_directory_move(&directories->items[j], &current);
        ncm_directory_destroy(&current);
    }
    return;
}

static int32
tag_editor_compare_directories(NcmDirectory *left,
                               NcmDirectory *right) {
    int32 left_start;
    int32 right_start;

    if ((left == NULL) || (left->path == NULL)) {
        if ((right == NULL) || (right->path == NULL)) {
            return 0;
        }
        return -1;
    }
    if ((right == NULL) || (right->path == NULL)) {
        return 1;
    }

    left_start = ncm_string_basename_start(left->path, left->path_len);
    right_start = ncm_string_basename_start(right->path, right->path_len);
    return ncm_compare_locale_strings(
        left->path + left_start, left->path_len - left_start,
        right->path + right_start, right->path_len - right_start,
        Config.ignore_leading_the);
}

static void
tag_editor_sort_songs(NcmSongArray *songs) {
    ASSERT(songs != NULL);
    for (int32 i = 1; i < songs->len; i += 1) {
        NcmSong current;
        int32 j;

        current = (NcmSong){0};
        ncm_song_move(&current, &songs->items[i]);
        j = i;
        while ((j > 0)
               && (tag_editor_compare_songs(&songs->items[j - 1],
                                            &current) > 0)) {
            ncm_song_move(&songs->items[j], &songs->items[j - 1]);
            j -= 1;
        }
        ncm_song_move(&songs->items[j], &current);
        ncm_song_destroy(&current);
    }
    return;
}

static int32
tag_editor_compare_songs(NcmSong *left, NcmSong *right) {
    NcmStringView left_uri;
    NcmStringView right_uri;

    if (!ncm_song_uri_view(left, 0, &left_uri)) {
        if (!ncm_song_uri_view(right, 0, &right_uri)) {
            return 0;
        }
        return -1;
    }
    if (!ncm_song_uri_view(right, 0, &right_uri)) {
        return 1;
    }
    return ncm_compare_locale_strings(left_uri.data, left_uri.len,
                                      right_uri.data, right_uri.len,
                                      Config.ignore_leading_the);
}

static void
tag_editor_report_error(char *context, int32 context_len,
                        NcmError *ncm_error) {
    StrBuilder message = {0};

    SB_APPEND(&message, context, context_len);
    if (ncm_error && (ncm_error->message[0] != 0)) {
        SB_APPEND(&message, STRLIT(": "));
        SB_APPEND(&message, ncm_error->message,
                  strlen32(ncm_error->message));
    }
    ncm_statusbar_print_cstring(Config.message_delay_time,
                                message.data);
    sb_free(&message);
    return;
}

static bool
tag_editor_update_from_mpd(TagEditorScreen *screen,
                           NcmMpdClient *client) {
    NcmError ncm_error;
    bool changed = false;
    bool ok;

    if (screen == NULL) {
        return false;
    }

    ncm_error_clear(&ncm_error);
    if (screen->directories_update_requested
        || nc_menu_empty(nc_editor_pair_menu_base(&screen->directories))) {
        ok = tag_editor_reload_directories_from_mpd(screen, client,
                                                    &ncm_error);
        if (!ok) {
            screen->directories_update_requested = false;
            tag_editor_report_error(
                STRLIT("Could not fetch directories"), &ncm_error);
            ncm_error_clear(&ncm_error);
            tag_editor_update_titles(screen, true);
            return changed;
        }
        changed = true;
    }

    tag_editor_screen_finish_directory_change(screen);
    if (!screen->tags_update_requested
        && !nc_menu_empty(nc_tag_row_menu_base(&screen->tags))) {
        tag_editor_update_titles(screen, true);
        return changed;
    }

    ncm_error_clear(&ncm_error);
    if (!tag_editor_reload_songs_from_mpd(screen, client, &ncm_error)) {
        screen->tags_update_requested = false;
        tag_editor_report_error(STRLIT("Could not fetch songs"),
                                &ncm_error);
        ncm_error_clear(&ncm_error);
        tag_editor_update_titles(screen, true);
        return changed;
    }
    changed = true;
    tag_editor_update_titles(screen, true);
    return changed;
}

static bool
tag_editor_reload_directories_from_mpd(TagEditorScreen *screen,
                                       NcmMpdClient *client,
                                       NcmError *ncm_error) {
    NcmDirectoryArray directories;
    StrBuilder preserved = {0};
    char *dir;
    bool ok;

    if (screen == NULL) {
        return false;
    }

    directories = (NcmDirectoryArray){0};
    tag_editor_preserve_current_directory(screen, &preserved);
    if ((preserved.len <= 0) && (screen->highlighted_dir.len > 0)) {
        sb_set(&preserved, screen->highlighted_dir.data,
               screen->highlighted_dir.len);
    }
    dir = screen->current_dir.data;
    if (dir == NULL) {
        dir = "/";
    }

    ok = ncm_mpd_client_get_directory_list(client, dir, &directories,
                                           ncm_error);
    if (ok) {
        tag_editor_sort_directories(&directories);
        nc_menu_show_all_items(nc_editor_pair_menu_base(
            &screen->directories));
        nc_menu_clear_items(nc_editor_pair_menu_base(
            &screen->directories));
        ok = tag_editor_add_control_directory(screen);
        for (int32 i = 0; ok && (i < directories.len); i += 1) {
            NcmDirectory *directory;
            NcmStringView path;
            int32 basename_start;

            directory = &directories.items[i];
            if (!ncm_directory_path_view(directory, &path)) {
                continue;
            }
            basename_start = ncm_string_basename_start(path.data,
                                                       path.len);
            ok = tag_editor_screen_add_directory(
                screen, path.data + basename_start,
                path.len - basename_start, path.data, path.len);
        }
    }

    if (ok) {
        tag_editor_restore_current_directory(screen, &preserved);
        if (screen->directory_filter_enabled) {
            nc_menu_apply_filter(nc_editor_pair_menu_base(
                &screen->directories));
            tag_editor_restore_current_directory(screen, &preserved);
        }
        tag_editor_observe_current_directory(screen);
        sb_clear(&screen->highlighted_dir);
        screen->directories_update_requested = false;
    }

    sb_free(&preserved);
    ncm_directory_array_destroy(&directories);
    return ok;
}

static bool
tag_editor_reload_songs_from_mpd(TagEditorScreen *screen,
                                 NcmMpdClient *client, NcmError *ncm_error) {
    NcmMpdSongList list;
    NcmSongArray songs;
    StrBuilder preserved_uri = {0};
    char *path;
    int32 path_len;
    bool ok;

    if (screen == NULL) {
        return false;
    }
    if (!tag_editor_current_directory_path(screen, &path, &path_len)) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing directory"));
        return false;
    }

    list = (NcmMpdSongList){0};
    songs = (NcmSongArray){0};
    tag_editor_preserve_current_song(screen, &preserved_uri);

    if ((ok = ncm_mpd_client_get_songs(client, path, &list, ncm_error))) {
        ok = ncm_mpd_song_list_to_song_array(&list, &songs);
    }
    if (ok) {
        tag_editor_sort_songs(&songs);
        ok = tag_editor_screen_load_songs(screen, &songs);
    }
    if (ok) {
        if (screen->tag_filter_enabled) {
            nc_menu_apply_filter(nc_tag_row_menu_base(&screen->tags));
        }
        tag_editor_restore_current_song(screen, &preserved_uri);
        screen->tags_update_requested = false;
        tag_editor_update_titles(screen, true);
    }

    sb_free(&preserved_uri);
    ncm_song_array_destroy(&songs);
    ncm_mpd_song_list_destroy(&list);
    return ok;
}

static void
tag_editor_layout(TagEditorScreen *screen) {
    int32 separator_width;
    int32 parser_dialog_x_space;
    int32 parser_dialog_y_space;
    int32 parser_x_space;
    int32 parser_y_space;
    int32 screen_height;

    if (screen == NULL) {
        return;
    }
    if (screen->width < 1) {
        screen->width = 1;
    }
    if (screen->main_height < 1) {
        screen->main_height = 1;
    }

    separator_width = tag_editor_separator_width(screen);
    screen->middle_width = tag_editor_min_int64(26,
                                                screen->width
                                                - 2*separator_width);
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

    screen->parser_dialog_width = tag_editor_min_int64(30, screen->width);
    screen->parser_dialog_height = tag_editor_min_int64(5,
                                                        screen->main_height);
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
    screen->parser_height = tag_editor_min_int64(screen_height*8/10,
                                                 screen->main_height);
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

static int32
tag_editor_min_int64(int32 left, int32 right) {
    if (left < right) {
        return left;
    }
    return right;
}

static int32
tag_editor_separator_width(TagEditorScreen *screen) {
    ASSERT(screen != NULL);
    if (screen->width >= 5) {
        return 1;
    }
    return 0;
}

static bool
tag_editor_initialize_tag_types(TagEditorScreen *screen) {
    NcEditorStringMenu *menu;

    if (screen == NULL) {
        return false;
    }
    menu = &screen->tag_types;
    nc_menu_clear_items(nc_editor_string_menu_base(menu));
    for (int32 i = 0; ncm_song_info_tags[i].name; i += 1) {
        int32 name_len;

        name_len = 0;
        while (ncm_song_info_tags[i].name[name_len] != '\0') {
            name_len += 1;
        }
        if (!tag_editor_append_string_row(menu, ncm_song_info_tags[i].name,
                                          name_len,
                                          NC_MENU_ITEM_SELECTABLE)) {
            return false;
        }
    }
    nc_editor_string_menu_add_separator(menu);
    if (!tag_editor_append_string_row(menu, STRLIT("Filename"),
                                      NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    nc_editor_string_menu_add_separator(menu);
    if (Config.titles_visibility) {
        if (!tag_editor_append_string_row(
            menu, STRLIT("Options"), NC_MENU_ITEM_INACTIVE)) {
            return false;
        }
        nc_editor_string_menu_add_separator(menu);
    }
    if (!tag_editor_append_string_row(
        menu, STRLIT("Capitalize First Letters"),
        NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    if (!tag_editor_append_string_row(menu, STRLIT("lower all letters"),
                                      NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    nc_editor_string_menu_add_separator(menu);
    if (!tag_editor_append_string_row(menu, STRLIT("Reset"),
                                      NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    return tag_editor_append_string_row(menu, STRLIT("Save"),
                                        NC_MENU_ITEM_SELECTABLE);
}

static bool
tag_editor_append_string_row(NcEditorStringMenu *menu, char *data,
                             int32 data_len, uint32 flags) {
    NcMenuString string;
    bool ok;

    if (menu == NULL) {
        return false;
    }
    string = (NcMenuString){0};
    if ((ok = nc_menu_string_set(&string, data, data_len))) {
        nc_editor_string_menu_add_with_flags(menu, &string, flags);
    }
    nc_menu_string_destroy(&string);
    return ok;
}

static void
tag_editor_configure_menu(NcMenu *menu) {
    ASSERT(menu != NULL);
    nc_menu_set_selected_prefix(menu, &Config.selected_item_prefix);
    nc_menu_set_selected_suffix(menu, &Config.selected_item_suffix);
    nc_menu_set_cyclic_scrolling(menu, Config.use_cyclic_scrolling);
    nc_menu_set_centered_cursor(menu, Config.centered_cursor);
    return;
}

static void
tag_editor_configure_menus(TagEditorScreen *screen) {
    NcMenu *directories;
    NcMenu *tag_types;
    NcMenu *tags;
    NcMenu *parser_dialog;
    NcMenu *parser_rows;
    NcMenu *parser_actions;

    if (screen == NULL) {
        return;
    }
    directories = nc_editor_pair_menu_base(&screen->directories);
    tag_types = nc_editor_string_menu_base(&screen->tag_types);
    tags = nc_tag_row_menu_base(&screen->tags);
    parser_dialog = nc_editor_string_menu_base(&screen->parser_dialog);
    parser_rows = nc_editor_string_menu_base(&screen->parser_rows);
    parser_actions = nc_editor_string_menu_base(&screen->parser_actions);

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
tag_editor_update_menu_highlights(TagEditorScreen *screen) {
    NcMenu *directories;
    NcMenu *tag_types;
    NcMenu *tags;
    NcMenu *parser_dialog;
    NcMenu *parser_rows;
    NcMenu *parser_actions;
    NcMenu *active;

    if (screen == NULL) {
        return;
    }
    directories = nc_editor_pair_menu_base(&screen->directories);
    tag_types = nc_editor_string_menu_base(&screen->tag_types);
    tags = nc_tag_row_menu_base(&screen->tags);
    parser_dialog = nc_editor_string_menu_base(&screen->parser_dialog);
    parser_rows = nc_editor_string_menu_base(&screen->parser_rows);
    parser_actions = nc_editor_string_menu_base(&screen->parser_actions);

    nc_menu_set_highlight_prefix(
        directories, &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(
        directories, &Config.current_item_inactive_column_suffix);
    nc_menu_set_highlight_prefix(
        tag_types, &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(
        tag_types, &Config.current_item_inactive_column_suffix);
    nc_menu_set_highlight_prefix(
        tags, &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(
        tags, &Config.current_item_inactive_column_suffix);
    nc_menu_set_highlight_prefix(
        parser_dialog, &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(
        parser_dialog, &Config.current_item_inactive_column_suffix);
    nc_menu_set_highlight_prefix(
        parser_rows, &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(
        parser_rows, &Config.current_item_inactive_column_suffix);
    nc_menu_set_highlight_prefix(
        parser_actions, &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(
        parser_actions, &Config.current_item_inactive_column_suffix);

    if ((active = tag_editor_screen_active_menu(screen))) {
        nc_menu_set_highlight_prefix(active, &Config.current_item_prefix);
        nc_menu_set_highlight_suffix(active, &Config.current_item_suffix);
    }
    tag_editor_update_parser_borders(screen);
    return;
}

static void
tag_editor_update_parser_borders(TagEditorScreen *screen) {
    NcBorder dialog_border;
    NcBorder parser_border;
    NcBorder helper_border;

    if (screen == NULL) {
        return;
    }
    dialog_border = Config.window_border;
    parser_border = Config.window_border;
    helper_border = Config.window_border;
    if (screen->active_focus == TAG_EDITOR_FOCUS_PARSER_CHOICE) {
        dialog_border = Config.active_window_border;
    } else if (screen->active_focus
               == TAG_EDITOR_FOCUS_PARSER_ACTIONS) {
        parser_border = Config.active_window_border;
    } else if (tag_editor_focus_is_parser_helper(screen->active_focus)) {
        helper_border = Config.active_window_border;
    }
    nc_window_set_border(&screen->parser_dialog_window, dialog_border);
    nc_window_set_border(&screen->parser_window, parser_border);
    nc_window_set_border(&screen->parser_helper_window, helper_border);
    return;
}

static void
tag_editor_refresh_active_helper(TagEditorScreen *screen) {
    StrBuilder *buffer;

    if (screen == NULL) {
        return;
    }
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
tag_editor_draw_separators(TagEditorScreen *screen) {
    if (tag_editor_separator_width(screen) <= 0) {
        return;
    }
    nc_screen_draw_vertical_separator(screen->middle_start_x - 1);
    nc_screen_draw_vertical_separator(screen->right_start_x - 1);
    return;
}

static NcMenuDisplayCallbacks
tag_editor_directory_display_callbacks(TagEditorScreen *screen) {
    NcMenuDisplayCallbacks callbacks = {0};

    callbacks.draw = tag_editor_draw_directory;
    callbacks.filter = tag_editor_directory_filter;
    callbacks.user = screen;
    return callbacks;
}

static NcMenuDisplayCallbacks
tag_editor_tag_type_display_callbacks(TagEditorScreen *screen) {
    NcMenuDisplayCallbacks callbacks = {0};

    callbacks.draw = tag_editor_draw_string;
    callbacks.user = screen;
    return callbacks;
}

static NcMenuDisplayCallbacks
tag_editor_tag_display_callbacks(TagEditorScreen *screen) {
    NcMenuDisplayCallbacks callbacks = {0};

    callbacks.draw = tag_editor_draw_tag;
    callbacks.filter = tag_editor_tag_filter;
    callbacks.user = screen;
    return callbacks;
}

static void
tag_editor_draw_directory(NcMenu *menu, NcWindow *window, void *item,
                          int32 pos, void *user) {
    NcMenuStringPair *pair = item;
    StrBuilder converted;

    (void)menu;
    (void)pos;
    (void)user;
    ASSERT(window != NULL);
    ASSERT(pair != NULL);
    ASSERT(pair->first != NULL);
    converted = ncm_charset_utf8_to_locale(pair->first, pair->first_len);
    nc_window_print_data(window, converted.data, converted.len);
    sb_free(&converted);
    return;
}

static void
tag_editor_draw_string(NcMenu *menu, NcWindow *window, void *item,
                       int32 pos, void *user) {
    NcMenuString *string = item;
    StrBuilder converted;

    (void)menu;
    (void)pos;
    (void)user;
    ASSERT(window != NULL);
    ASSERT(string != NULL);
    ASSERT(string->data != NULL);
    converted = ncm_charset_utf8_to_locale(string->data, string->len);
    nc_window_print_data(window, converted.data, converted.len);
    sb_free(&converted);
    return;
}

static void
tag_editor_draw_tag(NcMenu *menu, NcWindow *window, void *item,
                    int32 pos, void *user) {
    TagEditorScreen *screen = user;
    NcBuffer buffer;

    (void)menu;
    (void)pos;
    ASSERT(screen != NULL);
    ASSERT(window != NULL);
    ASSERT(item != NULL);
    buffer = (NcBuffer){0};
    tag_editor_append_tag_display_value(screen, item, &buffer);
    tag_editor_print_buffer(window, &buffer);
    nc_buffer_destroy(&buffer);
    return;
}

static void
tag_editor_append_tag_display_value(TagEditorScreen *screen,
                                    NcmMutableSong *song,
                                    NcBuffer *buffer) {
    NcMenu *tag_types;
    int32 choice;

    ASSERT(screen != NULL);
    ASSERT(song != NULL);
    ASSERT(buffer != NULL);
    if (ncm_mutable_song_is_modified(song)) {
        nc_buffer_append_data(buffer, Config.modified_item_prefix.data,
                              Config.modified_item_prefix.len);
    }

    tag_types = nc_editor_string_menu_base(&screen->tag_types);
    choice = nc_menu_highlight(tag_types);
    if ((choice >= 0) && (choice < 11)) {
        StrBuilder tag;
        enum NcmTagsField field;

        field = ncm_song_info_tags[choice].field;
        tag = ncm_mutable_song_tags_buffer(
            song, field, Config.tags_separator, Config.tags_separator_len,
            Config.show_duplicate_tags);
        if (tag.len <= 0) {
            tag_editor_append_empty_tag(buffer);
        } else {
            tag_editor_append_locale(buffer, tag.data, tag.len);
        }
        sb_free(&tag);
        return;
    }

    if (choice == 12) {
        tag_editor_append_locale(buffer, song->name, song->name_len);
        if (song->new_name && (song->new_name_len > 0)) {
            tag_editor_append_formatted_color(buffer, &Config.color2);
            nc_buffer_append_data(buffer, STRLIT(" -> "));
            tag_editor_append_formatted_color_end(buffer, &Config.color2);
            tag_editor_append_locale(buffer, song->new_name,
                                     song->new_name_len);
        }
    }
    return;
}

static void
tag_editor_append_empty_tag(NcBuffer *buffer) {
    tag_editor_append_formatted_color(buffer, &Config.empty_tags_color);
    nc_buffer_append_data(buffer, Config.empty_tag, Config.empty_tag_len);
    tag_editor_append_formatted_color_end(buffer, &Config.empty_tags_color);
    return;
}

static void
tag_editor_append_formatted_color(NcBuffer *buffer,
                                  NcFormattedColor *color) {
    nc_buffer_add_formatted_color(buffer, nc_buffer_len(buffer), color, 0);
    return;
}

static void
tag_editor_append_formatted_color_end(NcBuffer *buffer,
                                      NcFormattedColor *color) {
    nc_buffer_add_formatted_color_end(buffer, nc_buffer_len(buffer), color,
                                      0);
    return;
}

static void
tag_editor_append_locale(NcBuffer *buffer, char *data, int32 data_len) {
    StrBuilder converted;

    if ((buffer == NULL) || (data == NULL) || (data_len <= 0)) {
        return;
    }
    converted = ncm_charset_utf8_to_locale(data, data_len);
    nc_buffer_append_data(buffer, converted.data, converted.len);
    sb_free(&converted);
    return;
}

static void
tag_editor_print_buffer(NcWindow *window, NcBuffer *buffer) {
    NcBufferProperty *properties;
    char *data;
    int32 property_count;
    int32 property_index;
    int32 len;

    if ((window == NULL) || (buffer == NULL)) {
        return;
    }

    data = nc_buffer_data(buffer);
    len = nc_buffer_len(buffer);
    properties = nc_buffer_properties(buffer);
    property_count = nc_buffer_property_count(buffer);
    property_index = 0;

    for (int32 i = 0;; i += 1) {
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
    return;
}

static bool
tag_editor_directory_filter(NcMenu *menu, void *item, void *user) {
    TagEditorScreen *screen = user;
    NcMenuStringPair *pair = item;

    (void)menu;
    return tag_editor_directory_matches(screen, pair);
}

static bool
tag_editor_tag_filter(NcMenu *menu, void *item, void *user) {
    TagEditorScreen *screen = user;
    NcmMutableSong *song = item;

    (void)menu;
    return tag_editor_tag_matches(screen, song);
}

static bool
tag_editor_copy_selected_song_at(TagEditorScreen *screen,
                                 NcmSongArray *songs, int32 pos) {
    NcmMutableSong *source;
    NcmSong song;

    source = nc_menu_active_item_at(
        nc_tag_row_menu_base(&screen->tags), pos);
    if (source == NULL) {
        return false;
    }

    song = (NcmSong){0};
    if (!tag_editor_mutable_song_to_song(source, &song)) {
        ncm_song_destroy(&song);
        return false;
    }
    ncm_song_array_append_move(songs, &song);
    ncm_song_destroy(&song);
    return true;
}

static bool
tag_editor_mutable_song_to_song(NcmMutableSong *source,
                                NcmSong *dest) {
    if ((source == NULL) || (dest == NULL) || (source->uri == NULL)
        || (source->uri_len <= 0)) {
        return false;
    }

    if (!ncm_song_set_uri(dest, source->uri, source->uri_len)) {
        return false;
    }
    ncm_song_set_duration(dest, source->duration);
    ncm_song_set_mtime(dest, source->mtime);
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
        if (!ncm_song_add_tag(dest, type, value, value_len)) {
            return false;
        }
    }
    return true;
}

static bool
tag_editor_for_each_target(TagEditorScreen *screen,
                           bool (*cb)(NcmMutableSong *song, void *user),
                           void *user) {
    NcMenu *menu;
    bool has_selected;

    if ((screen == NULL) || (cb == NULL)) {
        return false;
    }
    menu = nc_tag_row_menu_base(&screen->tags);
    has_selected = nc_menu_has_selected(menu);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmMutableSong *song;

        if (has_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if ((song = nc_menu_active_item_at(menu, i)) && !cb(song, user)) {
            return false;
        }
    }
    return true;
}

static bool
tag_editor_set_song_tag_callback(NcmMutableSong *song, void *user) {
    TagSetter *setter = user;

    return ncm_mutable_song_set_tags(song, setter->field, setter->value,
                                     setter->value_len, setter->separator,
                                     setter->separator_len);
}

static bool
tag_editor_number_song_callback(NcmMutableSong *song, void *user) {
    TrackNumberer *numberer = user;
    NcmStringView view;
    char buffer[64];
    int32 len;

    if (numberer->extended) {
        len = SNPRINTF(buffer, "%d/%d",
                       numberer->current, numberer->total);
    } else {
        len = SNPRINTF(buffer, "%d",
                       numberer->current);
    }
    if (len < 0) {
        return false;
    }
    if (len >= SIZEOF(buffer)) {
        len = SIZEOF(buffer) - 1;
    }
    numberer->current += 1;
    if (!ncm_mutable_song_set_tag(song, NCM_TAGS_FIELD_TRACK, 0,
                                  buffer, len)) {
        return false;
    }
    for (int32 i = 1; ncm_mutable_song_get_tag(
        song, NCM_TAGS_FIELD_TRACK, i, &view); i += 1) {
        if (!ncm_mutable_song_set_tag(
            song, NCM_TAGS_FIELD_TRACK, i, STRLIT(""))) {
            return false;
        }
    }
    return true;
}

static bool
tag_editor_capitalize_song_callback(NcmMutableSong *song, void *user) {
    (void)user;

    for (int32 field_idx = 0;
         ncm_song_info_tags[field_idx].name;
         field_idx += 1) {
        enum NcmTagsField field = ncm_song_info_tags[field_idx].field;

        for (int32 i = 0; ; i += 1) {
            NcmStringView view;
            StrBuilder converted = {0};
            int32 converted_len;

            if (!ncm_mutable_song_get_tag(song, field, i, &view)) {
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
            if (!ncm_mutable_song_set_tag(song, field, i, converted.data,
                                          converted.len)) {
                sb_free(&converted);
                return false;
            }
            sb_free(&converted);
        }
    }

    return true;
}

static bool
tag_editor_lower_song_callback(NcmMutableSong *song, void *user) {
    (void)user;
    for (int32 field_idx = 0;
         ncm_song_info_tags[field_idx].name; field_idx += 1) {
        enum NcmTagsField field;

        field = ncm_song_info_tags[field_idx].field;
        for (int32 i = 0; ; i += 1) {
            NcmStringView view;
            StrBuilder buffer = {0};

            if (!ncm_mutable_song_get_tag(song, field, i, &view)) {
                break;
            }
            SB_APPEND(&buffer, view.data, view.len);
            tag_editor_lower_ascii_buffer(&buffer);
            if (!ncm_mutable_song_set_tag(song, field, i, buffer.data,
                                          buffer.len)) {
                sb_free(&buffer);
                return false;
            }
            sb_free(&buffer);
        }
    }
    return true;
}

static bool
tag_editor_save_song_callback(NcmMutableSong *song, void *user) {
    SaveContext *context = user;
    int32 error;

    if ((context == NULL) || (song == NULL)) {
        return false;
    }

    context->target_count += 1;
    tag_editor_save_context_add_directory(context, song);
    if (!ncm_mutable_song_is_modified(song)) {
        return true;
    }

    context->modified_count += 1;
    tag_editor_save_status_with_name(
        context->screen, STRLIT("Writing tags in \""), song,
        STRLIT("\"..."));
    errno = 0;
    if (!ncm_mutable_song_write(song, context->music_dir)) {
        error = errno;
        if (error == 0) {
            error = EIO;
        }
        context->ok = false;
        tag_editor_save_status_error(context->screen, song, error);
        return false;
    }

    context->write_count += 1;
    ncm_mutable_song_clear_modifications(song);
    return true;
}

static void
tag_editor_save_status_with_name(TagEditorScreen *screen,
                                 char *prefix, int32 prefix_len,
                                 NcmMutableSong *song,
                                 char *suffix, int32 suffix_len) {
    StrBuilder message = {0};

    if ((screen == NULL) || (song == NULL)) {
        return;
    }

    SB_APPEND(&message, prefix, prefix_len);
    if (song->name) {
        SB_APPEND(&message, song->name, song->name_len);
    }
    SB_APPEND(&message, suffix, suffix_len);
    tag_editor_status_message(screen, message.data, message.len);
    sb_free(&message);
    return;
}

static void
tag_editor_save_status_error(TagEditorScreen *screen,
                             NcmMutableSong *song, int32 error) {
    StrBuilder message = {0};
    char *system_error;

    if ((screen == NULL) || (song == NULL)) {
        return;
    }

    system_error = strerror(error);
    SB_APPEND(&message, STRLIT("Error while writing tags to \""));
    if (song->name) {
        SB_APPEND(&message, song->name, song->name_len);
    }
    SB_APPEND(&message, STRLIT("\": "));
    SB_APPEND(&message, system_error, strlen32(system_error));
    tag_editor_status_message(screen, message.data, message.len);
    sb_free(&message);
    return;
}

static void
tag_editor_update_modified_directory(TagEditorScreen *screen,
                                     StrBuilder *directory) {
    if ((screen == NULL) || (directory == NULL)) {
        return;
    }
    if (screen->hooks.update_directory == NULL) {
        return;
    }
    sb_reserve(directory, 1);
    directory->data[directory->len] = '\0';
    screen->hooks.update_directory(screen->hooks.user, directory->data,
                                   directory->len);
    return;
}

static void
tag_editor_save_context_add_directory(SaveContext *context,
                                      NcmMutableSong *song) {
    StrBuilder shared;
    char *directory;
    int32 directory_len;

    if ((context == NULL) || (song == NULL)) {
        return;
    }

    directory = song->directory;
    directory_len = song->directory_len;
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
        return;
    }

    if (STREQUAL(context->shared_directory.data,
                 context->shared_directory.len,
                 directory, directory_len)) {
        return;
    }

    shared = ncm_string_shared_directory(
        context->shared_directory.data, context->shared_directory.len,
        directory, directory_len);
    sb_free(&context->shared_directory);
    context->shared_directory = shared;
    return;
}

static bool
tag_editor_tag_matches(TagEditorScreen *screen, NcmMutableSong *song) {
    if (!screen->tag_filter_enabled) {
        return true;
    }
    return tag_editor_tag_matches_regex(screen, song,
                                        &screen->tag_filter_regex);
}

static bool
tag_editor_directory_matches(TagEditorScreen *screen,
                             NcMenuStringPair *pair) {
    if (!screen->directory_filter_enabled) {
        return true;
    }
    return tag_editor_directory_matches_regex(
        pair, &screen->directory_filter_regex, true);
}

static bool
tag_editor_tag_matches_regex(TagEditorScreen *screen,
                             NcmMutableSong *song, NcmRegex *regex) {
    StrBuilder buffer = {0};
    bool found;

    ASSERT(screen != NULL);
    ASSERT(song != NULL);
    ASSERT(regex != NULL);

    if (!tag_editor_tag_search_text(screen, song, &buffer)) {
        sb_free(&buffer);
        return false;
    }
    found = ncm_regex_search(regex, buffer.data, buffer.len);
    sb_free(&buffer);
    return found;
}

static bool
tag_editor_tag_search_text(TagEditorScreen *screen,
                           NcmMutableSong *song, StrBuilder *buffer) {
    enum NcmTagsField field;

    ASSERT(screen != NULL);
    ASSERT(song != NULL);
    ASSERT(buffer != NULL);
    if (!tag_editor_tag_search_field(screen, &field)) {
        return false;
    }
    if (!tag_editor_song_display_value(song, field, buffer)) {
        return false;
    }
    if (buffer->len <= 0) {
        SB_APPEND(buffer, Config.empty_tag, Config.empty_tag_len);
    }
    return true;
}

static bool
tag_editor_tag_search_field(TagEditorScreen *screen,
                            enum NcmTagsField *field) {
    NcMenu *tag_types;
    int32 choice;

    ASSERT(screen != NULL);
    ASSERT(field != NULL);
    tag_types = nc_editor_string_menu_base(&screen->tag_types);
    choice = nc_menu_highlight(tag_types);
    if ((choice >= 0) && (choice < 11)) {
        *field = ncm_song_info_tags[choice].field;
        return true;
    }
    if (choice == 12) {
        *field = NCM_TAGS_FIELD_COUNT;
        return true;
    }
    return false;
}

static bool
tag_editor_tag_type_choice_is_editable(int32 choice) {
    return ((choice >= 0) && (choice < 11)) || (choice == 12);
}

static enum TagEditorTagTypeAction
tag_editor_current_tag_type_action(TagEditorScreen *screen,
                                   enum NcmTagsField *field) {
    NcMenu *menu;
    NcMenuString *row;
    int32 choice;

    if (field) {
        *field = NCM_TAGS_FIELD_COUNT;
    }
    ASSERT(screen != NULL);

    menu = nc_editor_string_menu_base(&screen->tag_types);
    choice = nc_menu_highlight(menu);
    if (((row = nc_menu_current_item(menu)) == NULL)
        || !nc_menu_current_is_selectable(menu)) {
        return TAG_EDITOR_TAG_TYPE_ACTION_NONE;
    }

    if ((choice >= 0) && (choice < 11)) {
        if (field) {
            *field = ncm_song_info_tags[choice].field;
        }
        if ((ncm_song_info_tags[choice].field == NCM_TAGS_FIELD_TRACK)
            && (screen->active_focus
                == TAG_EDITOR_FOCUS_TAG_TYPES)) {
            return TAG_EDITOR_TAG_TYPE_ACTION_NUMBER_TRACKS;
        }
        return TAG_EDITOR_TAG_TYPE_ACTION_FIELD;
    }
    if (tag_editor_strings_equal(row->data, row->len,
                                 STRLIT("Filename"))) {
        return TAG_EDITOR_TAG_TYPE_ACTION_FILENAME;
    }
    if (tag_editor_strings_equal(row->data, row->len,
                                 STRLIT("Capitalize First Letters"))) {
        return TAG_EDITOR_TAG_TYPE_ACTION_CAPITALIZE;
    }
    if (tag_editor_strings_equal(row->data, row->len,
                                 STRLIT("lower all letters"))) {
        return TAG_EDITOR_TAG_TYPE_ACTION_LOWER;
    }
    if (tag_editor_strings_equal(row->data, row->len,
                                 STRLIT("Reset"))) {
        return TAG_EDITOR_TAG_TYPE_ACTION_RESET;
    }
    if (tag_editor_strings_equal(row->data, row->len,
                                 STRLIT("Save"))) {
        return TAG_EDITOR_TAG_TYPE_ACTION_SAVE;
    }
    return TAG_EDITOR_TAG_TYPE_ACTION_NONE;
}

static void
tag_editor_status_message(TagEditorScreen *screen,
                          char *message, int32 message_len) {
    if ((screen == NULL) || (message == NULL) || (message_len < 0)) {
        return;
    }
    if (screen->hooks.status_message) {
        screen->hooks.status_message(screen->hooks.user, message,
                                     message_len);
    }
    return;
}

static bool
tag_editor_confirm(TagEditorScreen *screen, char *message,
                   int32 message_len) {
    if ((screen == NULL) || (message == NULL) || (message_len < 0)
        || (screen->hooks.confirm == NULL)) {
        return false;
    }
    return screen->hooks.confirm(screen->hooks.user, message, message_len);
}

static bool
tag_editor_strings_equal(char *left, int32 left_len,
                         char *right, int32 right_len) {
    if ((left == NULL) || (right == NULL) || (left_len != right_len)) {
        return false;
    }
    return STREQUAL(left, left_len, right, right_len);
}

static bool
tag_editor_directory_matches_regex(NcMenuStringPair *pair,
                                   NcmRegex *regex, bool filter) {
    if ((pair == NULL) || (pair->first == NULL)) {
        return false;
    }
    if (STREQUAL(pair->first, pair->first_len, STRLIT("."))) {
        return filter;
    }
    if (STREQUAL(pair->first, pair->first_len, STRLIT(".."))) {
        return filter;
    }
    return ncm_regex_search(regex, pair->first, pair->first_len);
}

static bool
tag_editor_active_item_matches(TagEditorScreen *screen,
                               NcMenu *menu, int32 pos, NcmRegex *regex) {
    if (screen->active_focus == TAG_EDITOR_FOCUS_TAGS) {
        return tag_editor_tag_matches_regex(screen,
                                            nc_menu_active_item_at(menu,
                                                                   pos),
                                            regex);
    }
    if (screen->active_focus == TAG_EDITOR_FOCUS_DIRECTORIES) {
        return tag_editor_directory_matches_regex(
            nc_menu_active_item_at(menu, pos), regex, false);
    }
    return false;
}

static bool
tag_editor_append_parser_row(NcEditorStringMenu *menu, char *data,
                             int32 data_len, uint32 flags) {
    NcMenuString string;
    bool ok;

    if (menu == NULL) {
        return false;
    }
    string = (NcMenuString){0};
    if ((ok = nc_menu_string_set(&string, data, data_len))) {
        nc_editor_string_menu_add_with_flags(menu, &string, flags);
    }
    nc_menu_string_destroy(&string);
    return ok;
}

static bool
tag_editor_append_parser_separator(TagEditorScreen *screen) {
    ASSERT(screen != NULL);
    nc_editor_string_menu_add_separator(&screen->parser_rows);
    nc_editor_string_menu_add_separator(&screen->parser_actions);
    return true;
}

static bool
tag_editor_append_parser_action_row(TagEditorScreen *screen,
                                    char *data, int32 data_len,
                                    uint32 flags) {
    ASSERT(screen != NULL);
    return tag_editor_append_parser_row(&screen->parser_rows, data,
                                        data_len, flags)
           && tag_editor_append_parser_row(&screen->parser_actions, data,
                                           data_len, flags);
}

static bool
tag_editor_append_parser_action_label(TagEditorScreen *screen,
                                      char *label, int32 label_len) {
    return tag_editor_append_parser_action_row(screen, label, label_len,
                                               NC_MENU_ITEM_SELECTABLE);
}

static bool
tag_editor_append_pattern_row(TagEditorScreen *screen) {
    StrBuilder row = {0};
    bool result;

    SB_APPEND(&row, STRLIT("Pattern: "));
    SB_APPEND(&row, screen->pattern.data, screen->pattern.len);
    result = tag_editor_append_parser_action_label(screen, row.data,
                                                   row.len);
    sb_free(&row);
    return result;
}

static bool
tag_editor_build_parser_menus(TagEditorScreen *screen) {
    if (!tag_editor_append_pattern_row(screen)) {
        return false;
    }
    if (!tag_editor_append_parser_action_label(screen,
                                               STRLIT("Preview"))) {
        return false;
    }
    if (!tag_editor_append_parser_action_label(screen,
                                               STRLIT("Legend"))) {
        return false;
    }
    if (!tag_editor_append_parser_separator(screen)) {
        return false;
    }
    if (!tag_editor_append_parser_action_label(screen,
                                               STRLIT("Proceed"))) {
        return false;
    }
    if (!tag_editor_append_parser_action_label(screen,
                                               STRLIT("Cancel"))) {
        return false;
    }
    if (screen->recent_patterns.len <= 0) {
        return true;
    }
    if (!tag_editor_append_parser_separator(screen)) {
        return false;
    }
    if (!tag_editor_append_parser_action_row(screen,
                                             STRLIT("Recent patterns"),
                                             NC_MENU_ITEM_INACTIVE)) {
        return false;
    }
    if (!tag_editor_append_parser_separator(screen)) {
        return false;
    }
    for (int32 i = 0; i < screen->recent_patterns.len; i += 1) {
        StrBuilder *pattern;

        pattern = &screen->recent_patterns.items[i];
        if (!tag_editor_append_parser_action_label(screen, pattern->data,
                                                   pattern->len)) {
            return false;
        }
    }
    return true;
}

static bool
tag_editor_build_parser_legend(TagEditorScreen *screen) {
    NcMenu *tags;
    int32 count;

    if (screen == NULL) {
        return false;
    }
    sb_clear(&screen->parser_legend);
    SB_APPEND(&screen->parser_legend,
              STRLIT("%a - artist\n"));
    SB_APPEND(&screen->parser_legend,
              STRLIT("%A - album artist\n"));
    SB_APPEND(&screen->parser_legend,
              STRLIT("%t - title\n"));
    SB_APPEND(&screen->parser_legend,
              STRLIT("%b - album\n"));
    SB_APPEND(&screen->parser_legend,
              STRLIT("%y - date\n"));
    SB_APPEND(&screen->parser_legend,
              STRLIT("%n - track number\n"));
    SB_APPEND(&screen->parser_legend,
              STRLIT("%g - genre\n"));
    SB_APPEND(&screen->parser_legend,
              STRLIT("%c - composer\n"));
    SB_APPEND(&screen->parser_legend,
              STRLIT("%p - performer\n"));
    SB_APPEND(&screen->parser_legend,
              STRLIT("%d - disc\n"));
    SB_APPEND(&screen->parser_legend,
              STRLIT("%C - comment\n\nFiles:\n"));

    tags = nc_tag_row_menu_base(&screen->tags);
    count = nc_menu_item_count(tags);
    for (int32 i = 0; i < count; i += 1) {
        NcmMutableSong *song;

        if (((song = nc_menu_active_item_at(tags, i)) == NULL)
            || (song->name == NULL)) {
            continue;
        }
        SB_APPEND(&screen->parser_legend, STRLIT(" * "));
        SB_APPEND(&screen->parser_legend, song->name,
                  song->name_len);
        sb_append_byte(&screen->parser_legend, '\n');
    }
    return true;
}

static bool
tag_editor_build_parser_preview(TagEditorScreen *screen,
                                bool apply, bool *success) {
    NcMenu *tags;
    int32 count;

    if (success) {
        *success = true;
    }
    ASSERT(screen != NULL);
    tag_editor_status_message(screen, STRLIT("Parsing..."));
    sb_clear(&screen->parser_preview);
    tags = nc_tag_row_menu_base(&screen->tags);
    count = nc_menu_item_count(tags);
    for (int32 i = 0; i < count; i += 1) {
        NcmMutableSong *song;

        if ((song = nc_menu_active_item_at(tags, i)) == NULL) {
            continue;
        }
        if (screen->parser_mode
            == TAG_EDITOR_PARSER_TAGS_FROM_FILENAME) {
            bool parsed;

            if (!apply && song->name) {
                SB_APPEND(&screen->parser_preview, song->name,
                          song->name_len);
                SB_APPEND(&screen->parser_preview,
                          STRLIT(":\n"));
            }
            parsed = tag_editor_parse_filename(
                song, screen->pattern.data, screen->pattern.len, !apply,
                &screen->parser_preview);
            if (!parsed && !apply) {
                SB_APPEND(&screen->parser_preview,
                          "Error while parsing filename!\n");
            }
            if (!apply) {
                sb_append_byte(&screen->parser_preview, '\n');
            }
        } else if (screen->parser_mode
                   == TAG_EDITOR_PARSER_RENAME_FILES) {
            StrBuilder stem = {0};
            StrBuilder new_name = {0};
            int32 extension_start;

            if (!tag_editor_generate_filename(
                song, screen->pattern.data, screen->pattern.len,
                &stem)) {
                sb_free(&new_name);
                sb_free(&stem);
                return false;
            }
            extension_start = tag_editor_filename_extension_start(
                song->name, song->name_len);
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
                          STRLIT(
                                      "\" would have an empty name"));
                tag_editor_status_message(
                    screen, screen->parser_preview.data,
                    screen->parser_preview.len);
                screen->parser_preview_enabled = true;
                if (success) {
                    *success = false;
                }
                sb_free(&new_name);
                sb_free(&stem);
                return true;
            }
            if (apply) {
                if (!ncm_mutable_song_set_new_name(song, new_name.data,
                                                   new_name.len)) {
                    sb_free(&new_name);
                    sb_free(&stem);
                    return false;
                }
            } else {
                tag_editor_append_parser_filename(
                    &screen->parser_preview, song->name, song->name_len);
                SB_APPEND(&screen->parser_preview,
                          STRLIT(" -> "));
                if (new_name.len > 0) {
                    SB_APPEND(&screen->parser_preview,
                              new_name.data, new_name.len);
                } else if (Config.empty_tag) {
                    SB_APPEND(&screen->parser_preview,
                              Config.empty_tag,
                              Config.empty_tag_len);
                }
                SB_APPEND(&screen->parser_preview,
                          STRLIT("\n\n"));
            }
            sb_free(&new_name);
            sb_free(&stem);
        }
    }
    if (!apply) {
        screen->parser_preview_enabled = true;
    }
    return true;
}

static bool
tag_editor_load_recent_patterns(TagEditorScreen *screen) {
    StrBuilder path = {0};
    StrBuilder line = {0};
    FILE *file;
    bool ok;
    bool read_line;

    if (screen == NULL) {
        return false;
    }
    if (screen->recent_patterns_loaded) {
        return true;
    }
    screen->recent_patterns_loaded = true;
    if (!tag_editor_history_path(&path)) {
        sb_free(&line);
        sb_free(&path);
        return false;
    }
    if ((file = fopen(path.data, "r")) == NULL) {
        sb_free(&line);
        sb_free(&path);
        return true;
    }
    ok = true;
    read_line = false;
    while (ok) {
        if (!(ok = tag_editor_read_pattern_line(file, &line, &read_line))
            || !read_line) {
            break;
        }
        if (line.len > 0) {
            ok = tag_editor_add_recent_pattern(screen, line.data,
                                               line.len);
        }
    }
    fclose(file);
    sb_free(&line);
    sb_free(&path);
    return ok;
}

static bool
tag_editor_save_recent_patterns(TagEditorScreen *screen) {
    StrBuilder path = {0};
    FILE *file;
    int32 limit;

    if (screen == NULL) {
        return false;
    }
    if (!tag_editor_history_path(&path)) {
        sb_free(&path);
        return false;
    }
    if ((file = fopen(path.data, "w")) == NULL) {
        sb_free(&path);
        return false;
    }
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
            fclose(file);
            sb_free(&path);
            return false;
        }
        if (fputc('\n', file) == EOF) {
            fclose(file);
            sb_free(&path);
            return false;
        }
    }
    fclose(file);
    sb_free(&path);
    return true;
}

static bool
tag_editor_history_path(StrBuilder *path) {
    ASSERT(path != NULL);
    if (Config.ncmpcpp_directory
        && (Config.ncmpcpp_directory_len > 0)) {
        return ncm_fs_join(path, Config.ncmpcpp_directory,
                           Config.ncmpcpp_directory_len,
                           STRLIT("patterns.list"));
    }
    return tag_editor_set_buffer(path, STRLIT("patterns.list"));
}

static bool
tag_editor_read_pattern_line(FILE *file, StrBuilder *line,
                             bool *read_line) {
    int32 ch;

    if ((file == NULL) || (line == NULL) || (read_line == NULL)) {
        return false;
    }
    sb_clear(line);
    *read_line = false;
    while (true) {
        ch = fgetc(file);
        if (ch == EOF) {
            break;
        }
        *read_line = true;
        if (ch == '\n') {
            break;
        }
        sb_append_byte(line, (char)ch);
    }
    while ((line->len > 0)
           && ((line->data[line->len - 1] == '\n')
               || (line->data[line->len - 1] == '\r'))) {
        line->len -= 1;
        line->data[line->len] = '\0';
    }
    return true;
}

static int32
tag_editor_find_recent_pattern(TagEditorScreen *screen,
                               char *pattern, int32 pattern_len) {
    if ((screen == NULL) || (pattern == NULL) || (pattern_len <= 0)) {
        return -1;
    }
    for (int32 i = 0; i < screen->recent_patterns.len; i += 1) {
        StrBuilder *item;

        item = &screen->recent_patterns.items[i];
        if (STREQUAL(item->data, item->len, pattern,
                     pattern_len)) {
            return i;
        }
    }
    return -1;
}

static bool
tag_editor_add_recent_pattern(TagEditorScreen *screen,
                              char *pattern, int32 pattern_len) {
    StrBuilder *item;

    if ((screen == NULL) || (pattern == NULL) || (pattern_len <= 0)) {
        return false;
    }
    if (tag_editor_find_recent_pattern(screen, pattern, pattern_len) >= 0) {
        return true;
    }
    if ((item = str_builder_array_append(&screen->recent_patterns)) == NULL) {
        return false;
    }
    return tag_editor_set_buffer(item, pattern, pattern_len);
}

static bool
tag_editor_move_pattern_to_front(TagEditorScreen *screen,
                                 char *pattern, int32 pattern_len) {
    StrBuilderArray replacement;
    StrBuilder first = {0};
    int32 existing;
    bool ok;

    if ((screen == NULL) || (pattern == NULL) || (pattern_len <= 0)) {
        return false;
    }
    str_builder_array_init(&replacement);
    ok = tag_editor_set_buffer(&first, pattern, pattern_len)
         && (str_builder_array_append_copy(&replacement, &first) >= 0);
    sb_free(&first);
    if (!ok) {
        str_builder_array_destroy(&replacement);
        return false;
    }
    existing = tag_editor_find_recent_pattern(screen, pattern, pattern_len);
    for (int32 i = 0; i < screen->recent_patterns.len; i += 1) {
        if (i == existing) {
            continue;
        }
        if (str_builder_array_append_copy(
                &replacement, &screen->recent_patterns.items[i]) < 0) {
            str_builder_array_destroy(&replacement);
            return false;
        }
    }
    str_builder_array_move(&screen->recent_patterns, &replacement);
    str_builder_array_destroy(&replacement);
    return tag_editor_screen_prepare_parser_rows(
        screen, screen->parser_mode, screen->pattern.data,
        screen->pattern.len);
}

static bool
tag_editor_set_config_pattern(char *pattern, int32 pattern_len) {
    char *copy;
    int32 cap;

    if (pattern_len < 0) {
        return false;
    }
    if ((pattern == NULL) && (pattern_len > 0)) {
        return false;
    }
    cap = pattern_len + 1;
    copy = malloc2(cap);
    if (pattern && (pattern_len > 0)) {
        memcpy64(copy, pattern, pattern_len);
    }
    copy[pattern_len] = '\0';
    if (Config.pattern && (Config.pattern_cap > 0)) {
        free2(Config.pattern, Config.pattern_cap);
    }
    Config.pattern = copy;
    Config.pattern_len = pattern_len;
    Config.pattern_cap = cap;
    return true;
}

static bool
tag_editor_set_pattern(TagEditorScreen *screen,
                       char *pattern, int32 pattern_len) {
    ASSERT(screen != NULL);
    if (pattern == screen->pattern.data) {
        return tag_editor_set_config_pattern(screen->pattern.data,
                                             screen->pattern.len);
    }
    if (!tag_editor_set_buffer(&screen->pattern, pattern, pattern_len)) {
        return false;
    }
    return tag_editor_set_config_pattern(screen->pattern.data,
                                         screen->pattern.len);
}

static bool
tag_editor_prompt_pattern(TagEditorScreen *screen) {
    StrBuilder input = {0};
    NcmStringView initial;
    enum TagEditorPromptResult prompt_result;
    bool result;

    if ((screen == NULL) || (screen->hooks.prompt == NULL)) {
        return false;
    }
    initial.data = screen->pattern.data;
    initial.len = screen->pattern.len;
    prompt_result = screen->hooks.prompt(screen->hooks.user,
                                         STRLIT("Pattern"),
                                         initial, &input);
    if (prompt_result == TAG_EDITOR_PROMPT_ABORTED) {
        tag_editor_status_message(screen, STRLIT("Action aborted"));
        sb_free(&input);
        return false;
    }
    if (prompt_result == TAG_EDITOR_PROMPT_ERROR) {
        sb_free(&input);
        return false;
    }
    result = tag_editor_set_pattern(screen, input.data, input.len)
             && tag_editor_screen_prepare_parser_rows(
                 screen, screen->parser_mode, screen->pattern.data,
                 screen->pattern.len);
    sb_free(&input);
    if (result) {
        tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_PARSER_ACTIONS);
        (void)nc_menu_goto_selectable(
            nc_editor_string_menu_base(&screen->parser_actions),
            TAG_EDITOR_PARSER_ACTION_PATTERN);
    }
    return result;
}

static bool
tag_editor_apply_recent_pattern(TagEditorScreen *screen,
                                int32 choice) {
    NcMenu *menu;
    NcMenuString *row;

    if (screen == NULL) {
        return false;
    }
    menu = nc_editor_string_menu_base(&screen->parser_actions);
    if ((row = nc_menu_active_item_at(menu, choice)) == NULL) {
        return false;
    }
    if (!tag_editor_set_pattern(screen, row->data, row->len)) {
        return false;
    }
    if (!tag_editor_screen_prepare_parser_rows(
        screen, screen->parser_mode, screen->pattern.data,
        screen->pattern.len)) {
        return false;
    }
    tag_editor_set_focus(screen, TAG_EDITOR_FOCUS_PARSER_ACTIONS);
    return nc_menu_goto_selectable(
        nc_editor_string_menu_base(&screen->parser_actions),
        TAG_EDITOR_PARSER_ACTION_PATTERN);
}

static bool
tag_editor_mutable_song_to_format_song(NcmMutableSong *source,
                                       NcmSong *dest) {
    StrBuilder uri = {0};
    bool ok;

    if ((source == NULL) || (dest == NULL)) {
        return false;
    }

    if (source->uri && (source->uri_len >= 0)) {
        SB_APPEND(&uri, source->uri, source->uri_len);
    } else if (source->directory && (source->directory_len > 0)
               && source->name && (source->name_len >= 0)) {
        if (!ncm_fs_join(&uri, source->directory, source->directory_len,
                         source->name, source->name_len)) {
            sb_free(&uri);
            return false;
        }
    } else if (source->name && (source->name_len >= 0)) {
        SB_APPEND(&uri, source->name, source->name_len);
    }
    if (uri.data == NULL) {
        ok = ncm_song_set_uri(dest, STRLIT(""));
    } else {
        ok = ncm_song_set_uri(dest, uri.data, uri.len);
    }
    sb_free(&uri);
    if (!ok) {
        return false;
    }

    ncm_song_set_duration(dest, source->duration);
    ncm_song_set_mtime(dest, source->mtime);
    for (int32 i = 0; i < source->tags_len; i += 1) {
        NcmMutableSongTag *tag;
        enum mpd_tag_type type;
        char *value;
        int32 value_len;

        tag = &source->tags[i];
        type = ncm_tags_field_to_tag_type(tag->field);
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
        if (!ncm_song_add_tag(dest, type, value, value_len)) {
            return false;
        }
    }
    return true;
}

static int32
tag_editor_filename_extension_start(char *name, int32 name_len) {
    if ((name == NULL) || (name_len <= 0)) {
        return -1;
    }
    for (int32 i = name_len - 1; i > 0; i -= 1) {
        if (name[i] == '.') {
            return i;
        }
    }
    return -1;
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

static bool
tag_editor_mutable_song_get_field(NcmMutableSong *song,
                                  enum NcmTagsField field,
                                  StrBuilder *buffer) {
    StrBuilder tag = {0};

    ncm_mutable_song_get_tag_buffer(song, field, 0, &tag);
    SB_APPEND(buffer, tag.data, tag.len);
    sb_free(&tag);
    return true;
}

static void
tag_editor_lower_ascii_buffer(StrBuilder *buffer) {
    if ((buffer == NULL) || (buffer->data == NULL)) {
        return;
    }
    ncm_string_lowercase_ascii(buffer->data, buffer->len);
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

#endif /* NCMPCPP_NC_TAG_EDITOR_C */
