#if !defined(NCMPCPP_NC_TAG_EDITOR_C)
#define NCMPCPP_NC_TAG_EDITOR_C

#include "screens/nc_tag_editor.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "app_controller.h"
#include "global.h"
#include "settings.h"
#include "statusbar.h"
#include "ui_state.h"
#include "c/ncm_base.h"
#include "c/ncm_charset.h"
#include "c/ncm_comparators.h"
#include "c/ncm_format.h"
#include "c/ncm_fs.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_string.h"
#include "cbase/utf8.c"
#include "c/ncm_type_conversions.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"
#include "screens/screen_switcher.h"
#include "screens/song_info.h"
#include "title.h"

#define ENUM_NAME TagEditorParserActionRow
#define ENUM_PREFIX_ TAG_EDITOR_PARSER_ACTION_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(TAG_EDITOR_PARSER_ACTION_PATTERN, 0) \
    X(TAG_EDITOR_PARSER_ACTION_PREVIEW, 1) \
    X(TAG_EDITOR_PARSER_ACTION_LEGEND, 2) \
    X(TAG_EDITOR_PARSER_ACTION_PROCEED, 4) \
    X(TAG_EDITOR_PARSER_ACTION_CANCEL, 5) \
    X(TAG_EDITOR_PARSER_ACTION_RECENT_START, 9)
#include "cbase/xenums.c"

#define TAG_EDITOR_PATTERN_HISTORY_MAX 30

#define ENUM_NAME TagEditorTagTypeAction
#define ENUM_PREFIX_ TAG_EDITOR_TAG_TYPE_ACTION_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(TAG_EDITOR_TAG_TYPE_ACTION_NONE) \
    X(TAG_EDITOR_TAG_TYPE_ACTION_FIELD) \
    X(TAG_EDITOR_TAG_TYPE_ACTION_NUMBER_TRACKS) \
    X(TAG_EDITOR_TAG_TYPE_ACTION_FILENAME) \
    X(TAG_EDITOR_TAG_TYPE_ACTION_CAPITALIZE) \
    X(TAG_EDITOR_TAG_TYPE_ACTION_LOWER) \
    X(TAG_EDITOR_TAG_TYPE_ACTION_RESET) \
    X(TAG_EDITOR_TAG_TYPE_ACTION_SAVE)
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
static bool tag_editor_update_from_mpd(NativeTagEditorScreen *screen,
                                       NcmMpdClient *client);
static bool tag_editor_reload_directories_from_mpd(
    NativeTagEditorScreen *screen, NcmMpdClient *client, NcmError *error);
static bool tag_editor_reload_songs_from_mpd(NativeTagEditorScreen *screen,
                                             NcmMpdClient *client,
                                             NcmError *error);
static void tag_editor_mouse_callback(NcScreen *screen, MEVENT event);
static bool tag_editor_is_lockable(NcScreen *screen);
static bool tag_editor_is_mergable(NcScreen *screen);
static void tag_editor_destroy_callback(NcScreen *screen);
static void tag_editor_mouse_scroll(NativeTagEditorScreen *screen,
                                    enum NcScroll where);
static void tag_editor_mouse_scroll_menu(NcMenu *menu, NcWindow *window,
                                         enum NcScroll where);
static bool tag_editor_mouse_select_directory(
    NativeTagEditorScreen *screen, int32 y, bool enter);
static bool tag_editor_mouse_select_tag_type(
    NativeTagEditorScreen *screen, int32 y, bool run);
static bool tag_editor_mouse_select_tag(
    NativeTagEditorScreen *screen, int32 y, bool run);
static bool tag_editor_mouse_select_parser_dialog(
    NativeTagEditorScreen *screen, int32 y, bool run);
static bool tag_editor_mouse_select_parser_row(
    NativeTagEditorScreen *screen, int32 y, bool run);
static bool tag_editor_run_current_action(NativeTagEditorScreen *screen);
static bool tag_editor_mouse_move_to_column(
    NativeTagEditorScreen *screen, enum NativeTagEditorColumn column);
static bool tag_editor_mouse_move_to_parser_focus(
    NativeTagEditorScreen *screen, enum NativeTagEditorFocus focus);
static void tag_editor_finish_tag_type_change(
    NativeTagEditorScreen *screen, bool refresh_tags);
static void tag_editor_layout(NativeTagEditorScreen *screen);
static int64 tag_editor_min_int64(int64 left, int64 right);
static int64 tag_editor_separator_width(NativeTagEditorScreen *screen);
static void tag_editor_configure_menus(NativeTagEditorScreen *screen);
static bool tag_editor_initialize_tag_types(NativeTagEditorScreen *screen);
static bool tag_editor_append_string_row(NcEditorStringMenu *menu,
                                         char *data, int32 data_len,
                                         uint32 flags);
static void tag_editor_update_menu_highlights(
    NativeTagEditorScreen *screen);
static void tag_editor_update_parser_borders(NativeTagEditorScreen *screen);
static void tag_editor_refresh_active_helper(NativeTagEditorScreen *screen);
static void tag_editor_refresh_menu(NcWindow *window, NcMenu *menu);
static void tag_editor_draw_separators(NativeTagEditorScreen *screen);
static NcMenuDisplayCallbacks tag_editor_directory_display_callbacks(
    NativeTagEditorScreen *screen);
static NcMenuDisplayCallbacks tag_editor_tag_type_display_callbacks(
    NativeTagEditorScreen *screen);
static NcMenuDisplayCallbacks tag_editor_tag_display_callbacks(
    NativeTagEditorScreen *screen);
static void tag_editor_draw_directory(NcMenu *menu, NcWindow *window,
                                      void *item, int64 pos,
                                      void *user);
static void tag_editor_draw_string(NcMenu *menu, NcWindow *window,
                                   void *item, int64 pos, void *user);
static void tag_editor_draw_tag(NcMenu *menu, NcWindow *window,
                                void *item, int64 pos, void *user);
static void tag_editor_append_tag_display_value(
    NativeTagEditorScreen *screen, NcmMutableSong *song, NcBuffer *buffer);
static void tag_editor_append_empty_tag(NcBuffer *buffer);
static void tag_editor_append_formatted_color(NcBuffer *buffer,
                                              NcFormattedColor *color);
static void tag_editor_append_formatted_color_end(NcBuffer *buffer,
                                                  NcFormattedColor *color);
static void tag_editor_append_locale(NcBuffer *buffer, char *data,
                                     int32 data_len);
static void tag_editor_print_buffer(NcWindow *window, NcBuffer *buffer);
static void tag_editor_initialize_buffers(NativeTagEditorScreen *screen);
static void tag_editor_destroy_buffers(NativeTagEditorScreen *screen);
static void tag_editor_initialize_regexes(NativeTagEditorScreen *screen);
static void tag_editor_destroy_regexes(NativeTagEditorScreen *screen);
static bool tag_editor_set_buffer(NcmBuffer *buffer, char *data,
                                  int32 data_len);
static bool tag_editor_compile_constraint(NcmRegex *regex, char *pattern,
                                          int32 pattern_len,
                                          uint32 regex_flags,
                                          NcmError *error);
static void tag_editor_update_titles(NativeTagEditorScreen *screen,
                                     bool update_windows);
static void tag_editor_update_visible_counts(NativeTagEditorScreen *screen);
static void tag_editor_observe_current_directory(
    NativeTagEditorScreen *screen);
static bool tag_editor_directory_row_changed(NativeTagEditorScreen *screen);
static bool tag_editor_focus_is_main(enum NativeTagEditorFocus focus);
static bool tag_editor_focus_is_main_column(
    enum NativeTagEditorFocus focus, enum NativeTagEditorColumn column);
static bool tag_editor_focus_is_parser_helper(
    enum NativeTagEditorFocus focus);
static enum NativeTagEditorFocus tag_editor_column_focus(
    enum NativeTagEditorColumn column);
static void tag_editor_set_focus(NativeTagEditorScreen *screen,
                                 enum NativeTagEditorFocus focus);
static enum NativeTagEditorFocus tag_editor_current_helper_focus(
    NativeTagEditorScreen *screen);
static bool tag_editor_current_directory_path(NativeTagEditorScreen *screen,
                                              char **path,
                                              int32 *path_len);
static bool tag_editor_directory_has_subdirectories(
    NativeTagEditorScreen *screen, char *path, int32 path_len);
static bool tag_editor_directory_is_control(char *label, int32 label_len);
static bool tag_editor_highlight_directory_path(
    NativeTagEditorScreen *screen, char *path, int32 path_len);
static bool tag_editor_highlight_song_uri(
    NativeTagEditorScreen *screen, char *uri, int32 uri_len);
static bool tag_editor_current_directory_pair(
    NativeTagEditorScreen *screen, NcMenuStringPair **pair);
static bool tag_editor_build_renamed_directory(
    NativeTagEditorScreen *screen, char *name, int32 name_len,
    NcmBuffer *result);
static void tag_editor_status_directory_renamed(
    NativeTagEditorScreen *screen, char *name, int32 name_len);
static void tag_editor_status_directory_rename_error(
    NativeTagEditorScreen *screen, char *name, int32 name_len,
    NcmError *error);
static bool tag_editor_has_modified_songs(NativeTagEditorScreen *screen);
static int32 tag_editor_compare_directories(NcmDirectory *left,
                                            NcmDirectory *right);
static int32 tag_editor_compare_songs(NcmSong *left, NcmSong *right);
static bool tag_editor_directory_filter(NcMenu *menu, void *item,
                                        void *user);
static bool tag_editor_tag_filter(NcMenu *menu, void *item, void *user);
static bool tag_editor_copy_selected_song_at(
    NativeTagEditorScreen *screen, NcmSongArray *songs, int64 pos);
static bool tag_editor_for_each_target(NativeTagEditorScreen *screen,
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
static void tag_editor_save_status_with_name(
    NativeTagEditorScreen *screen, char *prefix, int32 prefix_len,
    NcmMutableSong *song, char *suffix, int32 suffix_len);
static void tag_editor_save_status_error(NativeTagEditorScreen *screen,
                                         NcmMutableSong *song, int32 error);
static void tag_editor_update_modified_directory(
    NativeTagEditorScreen *screen, NcmBuffer *directory);
static void tag_editor_save_context_add_directory(
    SaveContext *context, NcmMutableSong *song);
static bool tag_editor_tag_matches(NativeTagEditorScreen *screen,
                                   NcmMutableSong *song);
static bool tag_editor_directory_matches(NativeTagEditorScreen *screen,
                                         NcMenuStringPair *pair);
static bool tag_editor_tag_matches_regex(NativeTagEditorScreen *screen,
                                         NcmMutableSong *song,
                                         NcmRegex *regex);
static bool tag_editor_tag_search_text(NativeTagEditorScreen *screen,
                                       NcmMutableSong *song,
                                       NcmBuffer *buffer);
static bool tag_editor_tag_search_field(NativeTagEditorScreen *screen,
                                        enum NcmTagsField *field);
static bool tag_editor_tag_type_choice_is_editable(int64 choice);
static bool tag_editor_mutable_song_to_song(NcmMutableSong *source,
                                            NcmSong *dest);
static bool tag_editor_directory_matches_regex(NcMenuStringPair *pair,
                                               NcmRegex *regex,
                                               bool filter);
static bool tag_editor_active_item_matches(NativeTagEditorScreen *screen,
                                           NcMenu *menu, int64 pos,
                                           NcmRegex *regex);
static bool tag_editor_search_position(NcMenu *menu, int64 pos,
                                       void *user);
static bool tag_editor_append_parser_row(NcEditorStringMenu *menu,
                                         char *data, int32 data_len,
                                         uint32 flags);
static bool tag_editor_build_parser_menus(NativeTagEditorScreen *screen);
static bool tag_editor_build_parser_legend(NativeTagEditorScreen *screen);
static bool tag_editor_build_parser_preview(NativeTagEditorScreen *screen,
                                            bool apply, bool *success);
static bool tag_editor_load_recent_patterns(NativeTagEditorScreen *screen);
static bool tag_editor_save_recent_patterns(NativeTagEditorScreen *screen);
static bool tag_editor_history_path(NcmBuffer *path);
static bool tag_editor_read_pattern_line(FILE *file, NcmBuffer *line,
                                         bool *read_line);
static bool tag_editor_add_recent_pattern(
    NativeTagEditorScreen *screen, char *pattern, int32 pattern_len);
static bool tag_editor_move_pattern_to_front(
    NativeTagEditorScreen *screen, char *pattern, int32 pattern_len);
static bool tag_editor_set_pattern(NativeTagEditorScreen *screen,
                                   char *pattern, int32 pattern_len);
static bool tag_editor_prompt_pattern(NativeTagEditorScreen *screen);
static bool tag_editor_apply_recent_pattern(NativeTagEditorScreen *screen,
                                            int64 choice);
static bool tag_editor_mutable_song_to_format_song(NcmMutableSong *source,
                                                   NcmSong *dest);
static int32 tag_editor_filename_extension_start(char *name,
                                                int32 name_len);
static void tag_editor_append_parser_filename(
    NcmBuffer *buffer, char *name, int32 name_len);
static bool tag_editor_mutable_song_get_field(NcmMutableSong *song,
                                              enum NcmTagsField field,
                                              NcmBuffer *buffer);
static void tag_editor_lower_ascii_buffer(NcmBuffer *buffer);
static bool tag_editor_next_mask_tag(char *mask, int32 mask_len,
                                     int32 start, int32 *percent_pos,
                                     char *tag_char);
static enum TagEditorTagTypeAction tag_editor_current_tag_type_action(
    NativeTagEditorScreen *screen, enum NcmTagsField *field);
static bool tag_editor_run_directory_current(NativeTagEditorScreen *screen);
static bool tag_editor_run_tag_type_current(NativeTagEditorScreen *screen);
static bool tag_editor_run_tag_current(NativeTagEditorScreen *screen);
static bool tag_editor_run_parser_choice_current(
    NativeTagEditorScreen *screen);
static bool tag_editor_run_parser_action_current(
    NativeTagEditorScreen *screen);
static bool tag_editor_prompt_tag_value(NativeTagEditorScreen *screen,
                                        enum NcmTagsField field,
                                        bool all_targets);
static bool tag_editor_prompt_current_filename(
    NativeTagEditorScreen *screen);
static bool tag_editor_set_song_filename_stem(NcmMutableSong *song,
                                              char *stem,
                                              int32 stem_len);
static void tag_editor_status_message(NativeTagEditorScreen *screen,
                                      char *message, int32 message_len);
static bool tag_editor_confirm(NativeTagEditorScreen *screen,
                               char *message, int32 message_len);
static bool tag_editor_strings_equal(char *left, int32 left_len,
                                     char *right, int32 right_len);

static NcScreenCallbacks tag_editor_callbacks = {
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
    .is_lockable = tag_editor_is_lockable,
    .is_mergable = tag_editor_is_mergable,
    .destroy = tag_editor_destroy_callback,
};

typedef struct TagEditorSearchContext {
    NativeTagEditorScreen *screen;
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
    NativeTagEditorScreen *screen;
    NcmBuffer shared_directory;
    char *music_dir;
    int32 target_count;
    int32 modified_count;
    int32 write_count;
    bool shared_directory_valid;
    bool ok;
};

void
native_tag_editor_screen_init(NativeTagEditorScreen *screen,
                              int64 start_x, int64 width,
                              int64 main_start_y, int64 main_height,
                              NcColor color, NcBorder border) {
    nc_editor_pair_menu_init(&screen->directories);
    nc_editor_string_menu_init(&screen->tag_types);
    nc_tag_row_menu_init(&screen->tags);
    nc_editor_string_menu_init(&screen->parser_dialog);
    nc_editor_string_menu_init(&screen->parser_rows);
    nc_editor_string_menu_init(&screen->parser_actions);
    screen->hooks = (NativeTagEditorHooks){0};
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
    screen->active_column = NATIVE_TAG_EDITOR_COLUMN_DIRECTORIES;
    screen->active_focus = NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES;
    screen->last_directory_highlight = -1;
    screen->last_tag_type_highlight = -1;
    screen->last_known_directory_count = 0;
    screen->last_known_tag_count = 0;
    screen->window_timeout_ms = -1;
    screen->parser_mode = NATIVE_TAG_EDITOR_PARSER_NONE;
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

    (void)native_tag_editor_screen_set_current_dir(screen,
                                                   STRLIT_ARGS("/"));
    (void)tag_editor_initialize_tag_types(screen);
    tag_editor_layout(screen);
    tag_editor_configure_menus(screen);
    tag_editor_observe_current_directory(screen);
    nc_screen_init(&screen->screen, tag_editor_callbacks, screen,
                   NC_SCREEN_TYPE_TAG_EDITOR);
    (void)native_tag_editor_screen_prepare_parser_rows(
        screen, NATIVE_TAG_EDITOR_PARSER_NONE, NULL, 0);
    return;
}

void
native_tag_editor_screen_destroy(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    (void)app_controller_unregister_screen(native_tag_editor_screen_base(
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
native_tag_editor_screen_base(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->screen;
}

void
native_tag_editor_screen_set_hooks(NativeTagEditorScreen *screen,
                                   NativeTagEditorHooks hooks) {
    if (screen == NULL) {
        return;
    }
    screen->hooks = hooks;
    return;
}

NcMenu *
native_tag_editor_screen_active_menu(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    switch (screen->active_focus) {
    case NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES:
        return nc_editor_pair_menu_base(&screen->directories);
    case NATIVE_TAG_EDITOR_FOCUS_TAG_TYPES:
        return nc_editor_string_menu_base(&screen->tag_types);
    case NATIVE_TAG_EDITOR_FOCUS_TAGS:
        return nc_tag_row_menu_base(&screen->tags);
    case NATIVE_TAG_EDITOR_FOCUS_PARSER_CHOICE:
        return nc_editor_string_menu_base(&screen->parser_dialog);
    case NATIVE_TAG_EDITOR_FOCUS_PARSER_ACTIONS:
        return nc_editor_string_menu_base(&screen->parser_actions);
    case NATIVE_TAG_EDITOR_FOCUS_PARSER_LEGEND:
    case NATIVE_TAG_EDITOR_FOCUS_PARSER_PREVIEW:
        return NULL;
    case NATIVE_TAG_EDITOR_FOCUS_LAST:
    default:
        break;
    }
    return NULL;
}

NcWindow *
native_tag_editor_screen_active_window(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    switch (screen->active_focus) {
    case NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES:
        return &screen->directories_window;
    case NATIVE_TAG_EDITOR_FOCUS_TAG_TYPES:
        return &screen->tag_types_window;
    case NATIVE_TAG_EDITOR_FOCUS_TAGS:
        return &screen->tags_window;
    case NATIVE_TAG_EDITOR_FOCUS_PARSER_CHOICE:
        return &screen->parser_dialog_window;
    case NATIVE_TAG_EDITOR_FOCUS_PARSER_ACTIONS:
        return &screen->parser_window;
    case NATIVE_TAG_EDITOR_FOCUS_PARSER_LEGEND:
    case NATIVE_TAG_EDITOR_FOCUS_PARSER_PREVIEW:
        return &screen->parser_helper_window;
    case NATIVE_TAG_EDITOR_FOCUS_LAST:
    default:
        break;
    }
    return NULL;
}

void
native_tag_editor_screen_set_geometry(NativeTagEditorScreen *screen,
                                      int64 start_x, int64 width,
                                      int64 main_start_y,
                                      int64 main_height) {
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
native_tag_editor_screen_clear_directories(
    NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
    return;
}

void
native_tag_editor_screen_clear_stale_tags(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_clear_items(nc_tag_row_menu_base(&screen->tags));
    ncm_buffer_clear(&screen->displayed_dir);
    screen->displayed_dir_valid = false;
    screen->tags_update_requested = true;
    screen->last_known_tag_count = 0;
    tag_editor_update_titles(screen, true);
    return;
}

void
native_tag_editor_screen_finish_directory_change(
    NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    if (screen->active_focus != NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES) {
        return;
    }
    if (tag_editor_directory_row_changed(screen)) {
        native_tag_editor_screen_clear_stale_tags(screen);
    }
    return;
}

bool
native_tag_editor_screen_set_current_dir(NativeTagEditorScreen *screen,
                                         char *dir, int32 dir_len) {
    bool changed;

    if (screen == NULL) {
        return false;
    }
    changed = screen->current_dir.data
              && !STREQUAL(screen->current_dir.data,
                                   screen->current_dir.len, dir,
                                   dir_len);
    if (!tag_editor_set_buffer(&screen->current_dir, dir, dir_len)) {
        return false;
    }
    screen->directories_update_requested = true;
    if (changed) {
        native_tag_editor_screen_clear_stale_tags(screen);
    }
    return true;
}

bool
native_tag_editor_screen_current_dir(NativeTagEditorScreen *screen,
                                     NcmStringView *view) {
    ncm_string_view_init(view);
    if (screen == NULL) {
        return false;
    }
    ncm_string_view_set(view, screen->current_dir.data,
                        screen->current_dir.len);
    return screen->current_dir.data;
}

bool
native_tag_editor_screen_current_directory_path(
    NativeTagEditorScreen *screen, NcmStringView *view) {
    char *path;
    int32 path_len;

    ncm_string_view_init(view);
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
native_tag_editor_screen_enter_directory(NativeTagEditorScreen *screen) {
    NcmStringView path;

    ncm_string_view_init(&path);
    if (screen == NULL) {
        return false;
    }
    if (screen->active_focus != NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES) {
        return false;
    }
    if (!native_tag_editor_screen_current_directory_path(screen, &path)) {
        return false;
    }
    if (!tag_editor_directory_has_subdirectories(screen, path.data,
                                                path.len)) {
        tag_editor_status_message(screen,
                                  STRLIT_ARGS("No subdirectories found"));
        return false;
    }
    ncm_buffer_clear(&screen->highlighted_dir);
    if (!native_tag_editor_screen_set_current_dir(screen, path.data,
                                                 path.len)) {
        return false;
    }
    nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
    native_tag_editor_screen_clear_stale_tags(screen);
    screen->directories_update_requested = true;
    screen->observed_dir_valid = false;
    tag_editor_update_titles(screen, true);
    return true;
}

bool
native_tag_editor_screen_go_to_parent(NativeTagEditorScreen *screen) {
    NcmBuffer parent;
    int32 parent_len;
    bool ok;

    if (screen == NULL) {
        return false;
    }
    if (screen->active_focus != NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES) {
        return false;
    }
    if ((screen->current_dir.data == NULL)
        || (screen->current_dir.len <= 0)
        || STREQUAL(screen->current_dir.data,
                            screen->current_dir.len, STRLIT_ARGS("/"))) {
        return false;
    }

    ncm_buffer_init(&parent);
    if (!tag_editor_set_buffer(&screen->highlighted_dir,
                               screen->current_dir.data,
                               screen->current_dir.len)) {
        ncm_buffer_destroy(&parent);
        return false;
    }
    parent_len = ncm_string_parent_directory_len(screen->current_dir.data,
                                                screen->current_dir.len);
    if (parent_len <= 0) {
        ok = tag_editor_set_buffer(&parent, STRLIT_ARGS("/"));
    } else {
        ok = tag_editor_set_buffer(&parent, screen->current_dir.data,
                                   parent_len);
    }
    if (!ok || !native_tag_editor_screen_set_current_dir(
            screen, parent.data, parent.len)) {
        ncm_buffer_destroy(&parent);
        return false;
    }
    ncm_buffer_destroy(&parent);

    nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
    native_tag_editor_screen_clear_stale_tags(screen);
    screen->directories_update_requested = true;
    screen->observed_dir_valid = false;
    tag_editor_update_titles(screen, true);
    return true;
}

bool
native_tag_editor_screen_locate_song(NativeTagEditorScreen *screen,
                                     NcmSong *song) {
    NcmStringView directory;
    NcmStringView uri;
    NcmBuffer parent;
    NcmError error;
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

    ncm_buffer_init(&parent);
    parent_len = ncm_string_parent_directory_len(directory.data,
                                                directory.len);
    if (parent_len <= 0) {
        ok = ncm_buffer_set(&parent, STRLIT_ARGS("/"));
    } else {
        ok = ncm_buffer_set(&parent, directory.data, parent_len);
    }
    if (!ok) {
        ncm_buffer_destroy(&parent);
        return false;
    }

    ok = native_tag_editor_screen_set_current_dir(screen, parent.data,
                                                 parent.len)
         && ncm_buffer_set(&screen->highlighted_dir, directory.data,
                           directory.len);
    if (ok) {
        nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
        ncm_error_clear(&error);
        ok = tag_editor_reload_directories_from_mpd(screen, &global_mpd,
                                                    &error);
    }
    if (ok) {
        ok = tag_editor_highlight_directory_path(screen, directory.data,
                                                directory.len);
    }
    if (ok) {
        native_tag_editor_screen_clear_stale_tags(screen);
        ncm_error_clear(&error);
        ok = tag_editor_reload_songs_from_mpd(screen, &global_mpd, &error);
    }
    if (ok) {
        nc_menu_reset(nc_editor_string_menu_base(&screen->tag_types));
        tag_editor_set_focus(screen, NATIVE_TAG_EDITOR_FOCUS_TAGS);
        ok = tag_editor_highlight_song_uri(screen, uri.data, uri.len);
    }

    ncm_buffer_destroy(&parent);
    tag_editor_update_titles(screen, true);
    return ok;
}

bool
native_tag_editor_screen_rename_directory_available(
    NativeTagEditorScreen *screen, char *music_dir, int32 music_dir_len) {
    NcMenuStringPair *pair;

    if ((screen == NULL) || (music_dir == NULL) || (music_dir_len <= 0)) {
        return false;
    }
    if (screen->active_focus != NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES) {
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
native_tag_editor_screen_rename_current_directory(
    NativeTagEditorScreen *screen, char *music_dir, int32 music_dir_len) {
    NcMenuStringPair *pair;
    NcmStringView initial;
    NcmBuffer name;
    NcmBuffer old_path;
    NcmBuffer new_path;
    NcmBuffer new_relative;
    NcmError error;
    enum NativeTagEditorPromptResult result;
    bool ok;

    if (!native_tag_editor_screen_rename_directory_available(
            screen, music_dir, music_dir_len)) {
        return false;
    }
    if ((screen->hooks.prompt == NULL) || !tag_editor_current_directory_pair(
            screen, &pair)) {
        return false;
    }

    ncm_string_view_set(&initial, pair->first, pair->first_len);
    ncm_buffer_init(&name);
    result = screen->hooks.prompt(
        screen->hooks.user, STRLIT_ARGS("Directory: "), initial, &name);
    if (result == NATIVE_TAG_EDITOR_PROMPT_ABORTED) {
        ncm_buffer_destroy(&name);
        return true;
    }
    if (result != NATIVE_TAG_EDITOR_PROMPT_ACCEPTED) {
        ncm_buffer_destroy(&name);
        return false;
    }
    if ((name.len <= 0)
        || STREQUAL(name.data, name.len, pair->first,
                            pair->first_len)) {
        ncm_buffer_destroy(&name);
        return true;
    }

    ncm_buffer_init(&old_path);
    ncm_buffer_init(&new_path);
    ncm_buffer_init(&new_relative);
    ok = ncm_fs_join(&old_path, music_dir, music_dir_len,
                     pair->second, pair->second_len)
         && tag_editor_build_renamed_directory(screen, name.data,
                                               name.len, &new_relative)
         && ncm_fs_join(&new_path, music_dir, music_dir_len,
                        new_relative.data, new_relative.len);
    if (ok) {
        ncm_error_clear(&error);
        ok = ncm_fs_rename(old_path.data, old_path.len,
                           new_path.data, new_path.len, &error);
        if (!ok) {
            tag_editor_status_directory_rename_error(
                screen, pair->first, pair->first_len, &error);
        }
    }
    if (ok) {
        tag_editor_status_directory_renamed(screen, name.data, name.len);
        if (screen->hooks.update_directory) {
            screen->hooks.update_directory(
                screen->hooks.user, screen->current_dir.data,
                screen->current_dir.len);
        }
        (void)ncm_buffer_set(&screen->highlighted_dir,
                             new_relative.data, new_relative.len);
        screen->directories_update_requested = true;
        tag_editor_update_titles(screen, true);
    }

    ncm_buffer_destroy(&new_relative);
    ncm_buffer_destroy(&new_path);
    ncm_buffer_destroy(&old_path);
    ncm_buffer_destroy(&name);
    return ok;
}

bool
native_tag_editor_screen_add_directory(NativeTagEditorScreen *screen,
                                       char *label, int32 label_len,
                                       char *path, int32 path_len) {
    NcMenuStringPair pair;
    NcmBuffer first;
    NcmBuffer second;
    bool ok;

    if (screen == NULL) {
        return false;
    }
    nc_menu_string_pair_init(&pair);
    ncm_buffer_init(&first);
    ncm_buffer_init(&second);
    ok = ncm_buffer_set(&first, label, label_len)
         && ncm_buffer_set(&second, path, path_len);
    if (ok) {
        pair.first = ncm_buffer_steal(&first, &pair.first_len,
                                      &pair.first_cap);
        pair.second = ncm_buffer_steal(&second, &pair.second_len,
                                       &pair.second_cap);
        nc_editor_pair_menu_add(&screen->directories, &pair);
        screen->last_known_directory_count = nc_menu_item_count(
            nc_editor_pair_menu_base(&screen->directories));
        tag_editor_update_titles(screen, true);
    }
    ncm_buffer_destroy(&second);
    ncm_buffer_destroy(&first);
    nc_menu_string_pair_destroy(&pair);
    return ok;
}

bool
native_tag_editor_screen_load_songs(NativeTagEditorScreen *screen,
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

        ncm_mutable_song_init(&mutable_song);
        ok = ncm_mutable_song_load_originals_from_song(&mutable_song,
                                                       &songs->items[i]);
        if (ok) {
            ok = native_tag_editor_screen_add_mutable_song(screen,
                                                           &mutable_song);
        }
        ncm_mutable_song_destroy(&mutable_song);
        if (!ok) {
            return false;
        }
    }
    if (tag_editor_current_directory_path(screen, &path, &path_len)) {
        ncm_buffer_set(&screen->displayed_dir, path, path_len);
        screen->displayed_dir_valid = true;
    } else {
        ncm_buffer_clear(&screen->displayed_dir);
        screen->displayed_dir_valid = false;
    }
    screen->tags_update_requested = false;
    screen->last_known_tag_count = nc_menu_item_count(
        nc_tag_row_menu_base(&screen->tags));
    tag_editor_update_titles(screen, true);
    return true;
}

bool
native_tag_editor_screen_add_mutable_song(NativeTagEditorScreen *screen,
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
native_tag_editor_screen_selected_songs(NativeTagEditorScreen *screen,
                                        NcmSongArray *songs) {
    NcMenu *menu;
    bool has_selected;

    if ((screen == NULL) || (songs == NULL)) {
        return false;
    }
    ncm_song_array_clear(songs);
    if (screen->active_focus != NATIVE_TAG_EDITOR_FOCUS_TAGS) {
        return false;
    }

    menu = nc_tag_row_menu_base(&screen->tags);
    has_selected = nc_menu_has_selected(menu);
    if (!has_selected) {
        if (nc_menu_empty(menu)) {
            return true;
        }
        return tag_editor_copy_selected_song_at(
            screen, songs, nc_menu_highlight(menu));
    }

    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
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
native_tag_editor_screen_previous_column_available(
    NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (screen->active_focus == NATIVE_TAG_EDITOR_FOCUS_TAGS) {
        return !nc_menu_empty(nc_editor_string_menu_base(&screen->tag_types));
    }
    if (screen->active_focus == NATIVE_TAG_EDITOR_FOCUS_TAG_TYPES) {
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
native_tag_editor_screen_next_column_available(NativeTagEditorScreen *screen) {
    NcMenu *tag_types;

    if (screen == NULL) {
        return false;
    }
    if (screen->active_focus == NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES) {
        return !nc_menu_empty(nc_editor_string_menu_base(&screen->tag_types))
               && !nc_menu_empty(nc_tag_row_menu_base(&screen->tags));
    }
    if (screen->active_focus == NATIVE_TAG_EDITOR_FOCUS_TAG_TYPES) {
        tag_types = nc_editor_string_menu_base(&screen->tag_types);
        return !nc_menu_empty(nc_tag_row_menu_base(&screen->tags))
               && tag_editor_tag_type_choice_is_editable(
                   nc_menu_highlight(tag_types));
    }
    if (screen->active_focus == NATIVE_TAG_EDITOR_FOCUS_PARSER_ACTIONS) {
        return true;
    }
    return false;
}

void
native_tag_editor_screen_previous_column(NativeTagEditorScreen *screen) {
    if (!native_tag_editor_screen_previous_column_available(screen)) {
        return;
    }
    if (screen->active_focus == NATIVE_TAG_EDITOR_FOCUS_TAGS) {
        tag_editor_set_focus(screen, NATIVE_TAG_EDITOR_FOCUS_TAG_TYPES);
    } else if (screen->active_focus == NATIVE_TAG_EDITOR_FOCUS_TAG_TYPES) {
        if (tag_editor_has_modified_songs(screen)
            && !tag_editor_confirm(
                screen, STRLIT_ARGS("There are pending changes, "
                                    "are you sure?"))) {
            return;
        }
        tag_editor_set_focus(screen, NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES);
    } else if (tag_editor_focus_is_parser_helper(screen->active_focus)) {
        tag_editor_set_focus(screen, NATIVE_TAG_EDITOR_FOCUS_PARSER_ACTIONS);
    }
    tag_editor_finish_tag_type_change(screen, false);
    return;
}

void
native_tag_editor_screen_next_column(NativeTagEditorScreen *screen) {
    if (!native_tag_editor_screen_next_column_available(screen)) {
        return;
    }
    if (screen->active_focus == NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES) {
        tag_editor_set_focus(screen, NATIVE_TAG_EDITOR_FOCUS_TAG_TYPES);
    } else if (screen->active_focus == NATIVE_TAG_EDITOR_FOCUS_TAG_TYPES) {
        tag_editor_set_focus(screen, NATIVE_TAG_EDITOR_FOCUS_TAGS);
    } else if (screen->active_focus
               == NATIVE_TAG_EDITOR_FOCUS_PARSER_ACTIONS) {
        tag_editor_set_focus(screen, tag_editor_current_helper_focus(screen));
    }
    tag_editor_finish_tag_type_change(screen, false);
    return;
}

bool
native_tag_editor_screen_apply_tag_to_selection(
    NativeTagEditorScreen *screen, enum NcmTagsField field, char *value,
    int32 value_len, char *separator, int32 separator_len) {
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
native_tag_editor_screen_number_tracks(NativeTagEditorScreen *screen,
                                       bool extended) {
    TrackNumberer numberer;
    NcMenu *menu;

    if (screen == NULL) {
        return false;
    }
    menu = nc_tag_row_menu_base(&screen->tags);
    numberer.current = 1;
    if (nc_menu_has_selected(menu)) {
        numberer.total = (int32)nc_menu_selected_count(menu);
    } else {
        numberer.total = (int32)nc_menu_item_count(menu);
    }
    numberer.extended = extended;
    return tag_editor_for_each_target(screen, tag_editor_number_song_callback,
                                      &numberer);
}

void
native_tag_editor_screen_capitalize_first_letters(
    NativeTagEditorScreen *screen) {
    (void)tag_editor_for_each_target(screen,
                                     tag_editor_capitalize_song_callback,
                                     NULL);
    return;
}

void
native_tag_editor_screen_lower_all_letters(NativeTagEditorScreen *screen) {
    (void)tag_editor_for_each_target(screen, tag_editor_lower_song_callback,
                                     NULL);
    return;
}

void
native_tag_editor_screen_clear_modifications(NativeTagEditorScreen *screen) {
    NcMenu *menu;

    if (screen == NULL) {
        return;
    }
    menu = nc_tag_row_menu_base(&screen->tags);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmMutableSong *song;

        song = nc_menu_active_item_at(menu, i);
        if (song) {
            ncm_mutable_song_clear_modifications(song);
        }
    }
    return;
}

bool
native_tag_editor_screen_save_modified(NativeTagEditorScreen *screen,
                                       char *music_dir) {
    SaveContext context;
    bool iterated;

    if (screen == NULL) {
        return false;
    }

    tag_editor_status_message(screen, STRLIT_ARGS("Writing changes..."));

    context = (SaveContext){0};
    context.screen = screen;
    context.music_dir = music_dir;
    context.ok = true;
    ncm_buffer_init(&context.shared_directory);

    iterated = tag_editor_for_each_target(
        screen, tag_editor_save_song_callback, &context);
    if (!iterated || !context.ok) {
        ncm_buffer_destroy(&context.shared_directory);
        native_tag_editor_screen_clear_stale_tags(screen);
        return false;
    }

    tag_editor_status_message(screen, STRLIT_ARGS("Tags updated"));
    nc_menu_reset(nc_editor_string_menu_base(&screen->tag_types));
    tag_editor_set_focus(screen, NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES);
    if (context.shared_directory_valid) {
        tag_editor_update_modified_directory(
            screen, &context.shared_directory);
    }
    ncm_buffer_destroy(&context.shared_directory);
    return context.target_count > 0;
}

bool
native_tag_editor_screen_save_action_available(
    NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->active_focus == NATIVE_TAG_EDITOR_FOCUS_TAG_TYPES;
}

bool
native_tag_editor_screen_apply_directory_filter(
    NativeTagEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, NcmError *error) {
    if (screen == NULL) {
        return false;
    }
    if (!tag_editor_compile_constraint(&screen->directory_filter_regex,
                                       pattern, pattern_len, regex_flags,
                                       error)) {
        return false;
    }
    ncm_buffer_set(&screen->directory_filter_constraint, pattern,
                   pattern_len);
    nc_menu_set_display_callbacks(
        nc_editor_pair_menu_base(&screen->directories),
        tag_editor_directory_display_callbacks(screen));
    screen->directory_filter_enabled = true;
    nc_menu_apply_filter(nc_editor_pair_menu_base(&screen->directories));
    tag_editor_update_titles(screen, true);
    return true;
}

bool
native_tag_editor_screen_apply_tag_filter(
    NativeTagEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, NcmError *error) {
    if (screen == NULL) {
        return false;
    }
    if (!tag_editor_compile_constraint(&screen->tag_filter_regex, pattern,
                                       pattern_len, regex_flags, error)) {
        return false;
    }
    ncm_buffer_set(&screen->tag_filter_constraint, pattern, pattern_len);
    nc_menu_set_display_callbacks(nc_tag_row_menu_base(&screen->tags),
                                  tag_editor_tag_display_callbacks(screen));
    screen->tag_filter_enabled = true;
    nc_menu_apply_filter(nc_tag_row_menu_base(&screen->tags));
    tag_editor_update_titles(screen, true);
    return true;
}

bool
native_tag_editor_screen_search(NativeTagEditorScreen *screen,
                                char *pattern, int32 pattern_len,
                                bool forward, bool wrap,
                                bool skip_current, NcmError *error) {
    TagEditorSearchContext context;
    NcmRegex *regex;
    NcmBuffer *constraint;
    bool *enabled;
    NcMenu *menu;
    NcWindow *window;
    bool result;

    if ((screen == NULL) || (pattern == NULL) || (pattern_len <= 0)) {
        return false;
    }
    if ((screen->active_focus != NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES)
        && (screen->active_focus != NATIVE_TAG_EDITOR_FOCUS_TAGS)) {
        return false;
    }

    if (screen->active_focus == NATIVE_TAG_EDITOR_FOCUS_TAGS) {
        regex = &screen->tag_search_regex;
        constraint = &screen->tag_search_constraint;
        enabled = &screen->tag_search_enabled;
    } else {
        regex = &screen->directory_search_regex;
        constraint = &screen->directory_search_constraint;
        enabled = &screen->directory_search_enabled;
    }
    if (!tag_editor_compile_constraint(regex, pattern, pattern_len,
                                       Config.regex_type, error)) {
        return false;
    }
    ncm_buffer_set(constraint, pattern, pattern_len);
    *enabled = true;

    menu = native_tag_editor_screen_active_menu(screen);
    window = native_tag_editor_screen_active_window(screen);
    context.screen = screen;
    context.regex = regex;
    result = nc_menu_search_selectable(menu, nc_window_height(window),
                                       forward, wrap, skip_current,
                                       tag_editor_search_position,
                                       &context, NULL);
    if (result) {
        native_tag_editor_screen_finish_directory_change(screen);
    }
    return result;
}

static bool
tag_editor_search_position(NcMenu *menu, int64 pos, void *user) {
    TagEditorSearchContext *context;

    context = user;
    return tag_editor_active_item_matches(context->screen, menu, pos,
                                          context->regex);
}

static void
tag_editor_reset_parser_navigation(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_reset(nc_editor_string_menu_base(&screen->parser_dialog));
    nc_menu_reset(nc_editor_string_menu_base(&screen->parser_rows));
    nc_menu_reset(nc_editor_string_menu_base(&screen->parser_actions));
    return;
}

bool
native_tag_editor_screen_prepare_parser_rows(
    NativeTagEditorScreen *screen, enum NativeTagEditorParserMode mode,
    char *pattern, int32 pattern_len) {
    if (screen == NULL) {
        return false;
    }
    screen->parser_mode = mode;
    if (pattern) {
        if (!tag_editor_set_pattern(screen, pattern, pattern_len)) {
            return false;
        }
    } else if ((mode != NATIVE_TAG_EDITOR_PARSER_NONE)
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
            &screen->parser_dialog, STRLIT_ARGS("Get tags from filename"),
            NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    if (!tag_editor_append_parser_row(
            &screen->parser_dialog, STRLIT_ARGS("Rename files"),
            NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    if (!tag_editor_append_parser_row(
            &screen->parser_dialog, STRLIT_ARGS("Cancel"),
            NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    if (mode == NATIVE_TAG_EDITOR_PARSER_NONE) {
        tag_editor_reset_parser_navigation(screen);
        return true;
    }
    if (!tag_editor_append_parser_row(
            &screen->parser_rows, STRLIT_ARGS("Get tags from filename"),
            NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    if (!tag_editor_append_parser_row(
            &screen->parser_rows, STRLIT_ARGS("Rename files"),
            NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    if (!tag_editor_append_parser_row(
            &screen->parser_rows, STRLIT_ARGS("Cancel"),
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
native_tag_editor_screen_show_parser_dialog(
    NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    if (nc_menu_empty(nc_editor_string_menu_base(&screen->parser_dialog))) {
        (void)native_tag_editor_screen_prepare_parser_rows(
            screen, NATIVE_TAG_EDITOR_PARSER_NONE, NULL, 0);
    }
    screen->parser_mode = NATIVE_TAG_EDITOR_PARSER_NONE;
    tag_editor_set_focus(screen, NATIVE_TAG_EDITOR_FOCUS_PARSER_CHOICE);
    return;
}

void
native_tag_editor_screen_show_parser_actions(
    NativeTagEditorScreen *screen, enum NativeTagEditorParserMode mode) {
    if ((screen == NULL) || (mode == NATIVE_TAG_EDITOR_PARSER_NONE)) {
        return;
    }
    if (!tag_editor_load_recent_patterns(screen)) {
        return;
    }
    if ((screen->pattern.len <= 0) && (screen->recent_patterns.len > 0)) {
        NcmBuffer *pattern;

        pattern = &screen->recent_patterns.items[0];
        (void)tag_editor_set_pattern(screen, pattern->data, pattern->len);
    }
    (void)native_tag_editor_screen_prepare_parser_rows(
        screen, mode, screen->pattern.data, screen->pattern.len);
    (void)tag_editor_build_parser_legend(screen);
    screen->parser_preview_enabled = false;
    tag_editor_set_focus(screen, NATIVE_TAG_EDITOR_FOCUS_PARSER_ACTIONS);
    return;
}

void
native_tag_editor_screen_show_parser_legend(
    NativeTagEditorScreen *screen) {
    if ((screen == NULL)
        || (screen->parser_mode == NATIVE_TAG_EDITOR_PARSER_NONE)) {
        return;
    }
    tag_editor_set_focus(screen, NATIVE_TAG_EDITOR_FOCUS_PARSER_LEGEND);
    return;
}

void
native_tag_editor_screen_show_parser_preview(
    NativeTagEditorScreen *screen) {
    if ((screen == NULL)
        || (screen->parser_mode == NATIVE_TAG_EDITOR_PARSER_NONE)) {
        return;
    }
    tag_editor_set_focus(screen, NATIVE_TAG_EDITOR_FOCUS_PARSER_PREVIEW);
    return;
}

void
native_tag_editor_screen_close_parser(
    NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->parser_mode = NATIVE_TAG_EDITOR_PARSER_NONE;
    tag_editor_set_focus(screen, NATIVE_TAG_EDITOR_FOCUS_TAG_TYPES);
    return;
}

bool
native_tag_editor_parse_filename(NcmMutableSong *song, char *mask,
                                 int32 mask_len, bool preview,
                                 NcmBuffer *preview_buffer) {
    NcmBuffer file;
    int32 mask_pos;
    int32 file_pos;
    int32 percent_pos;
    int32 name_len;
    char tag_char;

    if ((song == NULL) || (mask == NULL) || (mask_len < 0)) {
        return false;
    }
    ncm_buffer_init(&file);
    if (song->name == NULL) {
        ncm_buffer_destroy(&file);
        return false;
    }
    name_len = song->name_len;
    for (int32 i = song->name_len - 1; i >= 0; i -= 1) {
        if (song->name[i] == '.') {
            name_len = i;
            break;
        }
    }
    ncm_buffer_append(&file, song->name, name_len);
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
            ncm_buffer_destroy(&file);
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
                ncm_buffer_destroy(&file);
                return false;
            }
            value_end = found;
        } else {
            value_end = file.len;
        }

        field = ncm_tags_field_from_char(tag_char);
        if (field != NCM_TAGS_FIELD_LAST) {
            for (int32 i = file_pos; i < value_end; i += 1) {
                if (file.data[i] == '_') {
                    file.data[i] = ' ';
                }
            }
            if (preview && preview_buffer) {
                ncm_buffer_append_byte(preview_buffer, '%');
                ncm_buffer_append_byte(preview_buffer, tag_char);
                ncm_buffer_append(preview_buffer, STRLIT_ARGS(": "));
                ncm_buffer_append(preview_buffer, file.data + file_pos,
                                  value_end - file_pos);
                ncm_buffer_append_byte(preview_buffer, '\n');
            } else if (!ncm_mutable_song_set_tags(song, field,
                                                  file.data + file_pos,
                                                  value_end - file_pos,
                                                  NULL, 0)) {
                ncm_buffer_destroy(&file);
                return false;
            }
        }
        file_pos = value_end;
        mask_pos = percent_pos + 2;
    }
    ncm_buffer_destroy(&file);
    return true;
}

bool
native_tag_editor_generate_filename(NcmMutableSong *song, char *pattern,
                                    int32 pattern_len,
                                    NcmBuffer *filename) {
    NcmFormatAst ast;
    NcmSong format_song;
    NcmBuffer rendered;
    NcmError error;

    if ((song == NULL) || (filename == NULL) || (pattern_len < 0)) {
        return false;
    }
    if ((pattern == NULL) && (pattern_len > 0)) {
        return false;
    }

    ncm_format_ast_init(&ast);
    ncm_song_init(&format_song);
    ncm_error_clear(&error);
    if (!ncm_format_parse(&ast, pattern, pattern_len,
                          NCM_FORMAT_FLAG_TAG, &error)) {
        ncm_error_clear(&error);
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
    ncm_buffer_append(filename, rendered.data, rendered.len);
    ncm_buffer_destroy(&rendered);
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
native_tag_editor_song_display_value(NcmMutableSong *song,
                                     enum NcmTagsField field,
                                     NcmBuffer *buffer) {
    if ((song == NULL) || (buffer == NULL)) {
        return false;
    }
    if (field == NCM_TAGS_FIELD_LAST) {
        ncm_buffer_append(buffer, song->name, song->name_len);
        if (song->new_name && (song->new_name_len > 0)) {
            ncm_buffer_append(buffer, STRLIT_ARGS(" -> "));
            ncm_buffer_append(buffer, song->new_name, song->new_name_len);
        }
        return true;
    }
    return tag_editor_mutable_song_get_field(song, field, buffer);
}

static NativeTagEditorScreen *
tag_editor_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcWindow *
tag_editor_active_window(NcScreen *screen) {
    NativeTagEditorScreen *editor;

    editor = tag_editor_from_screen(screen);
    return native_tag_editor_screen_active_window(editor);
}

static void
tag_editor_refresh(NcScreen *screen) {
    NativeTagEditorScreen *editor;

    editor = tag_editor_from_screen(screen);
    if (editor == NULL) {
        return;
    }
    tag_editor_update_titles(editor, true);
    tag_editor_update_menu_highlights(editor);
    if (editor->active_focus == NATIVE_TAG_EDITOR_FOCUS_PARSER_CHOICE) {
        tag_editor_refresh_menu(&editor->parser_dialog_window,
                                nc_editor_string_menu_base(
                                    &editor->parser_dialog));
        return;
    }
    if ((editor->active_focus == NATIVE_TAG_EDITOR_FOCUS_PARSER_ACTIONS)
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
    NativeTagEditorScreen *editor;
    NcMenu *menu;
    NcWindow *window;

    editor = tag_editor_from_screen(screen);
    if (editor == NULL) {
        return;
    }
    tag_editor_update_titles(editor, true);
    tag_editor_update_menu_highlights(editor);
    if (tag_editor_focus_is_parser_helper(editor->active_focus)) {
        tag_editor_refresh_active_helper(editor);
        return;
    }
    menu = native_tag_editor_screen_active_menu(editor);
    window = native_tag_editor_screen_active_window(editor);
    tag_editor_refresh_menu(window, menu);
    return;
}

static void
tag_editor_scroll(NcScreen *screen, enum NcScroll where) {
    NativeTagEditorScreen *editor;
    NcMenu *menu;
    NcWindow *window;

    editor = tag_editor_from_screen(screen);
    if (editor == NULL) {
        return;
    }
    menu = native_tag_editor_screen_active_menu(editor);
    window = native_tag_editor_screen_active_window(editor);
    if (menu) {
        nc_menu_scroll_selectable(menu, nc_window_height(window), where);
    } else if (window) {
        nc_window_scroll(window, where);
    }
    native_tag_editor_screen_finish_directory_change(editor);
    tag_editor_finish_tag_type_change(editor, true);
    tag_editor_update_menu_highlights(editor);
    return;
}

static bool
tag_editor_can_run_current(NcScreen *screen) {
    NativeTagEditorScreen *editor;
    NcMenu *menu;
    enum NcmTagsField field;

    editor = tag_editor_from_screen(screen);
    if (editor == NULL) {
        return false;
    }

    switch (editor->active_focus) {
    case NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES:
    case NATIVE_TAG_EDITOR_FOCUS_PARSER_CHOICE:
        menu = native_tag_editor_screen_active_menu(editor);
        return menu && nc_menu_current_is_selectable(menu);
    case NATIVE_TAG_EDITOR_FOCUS_PARSER_ACTIONS:
        menu = native_tag_editor_screen_active_menu(editor);
        if ((menu == NULL) || !nc_menu_current_is_selectable(menu)) {
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
                   >= TAG_EDITOR_PARSER_ACTION_RECENT_START;
        }
    case NATIVE_TAG_EDITOR_FOCUS_TAG_TYPES:
        if (nc_menu_empty(nc_tag_row_menu_base(&editor->tags))) {
            return false;
        }
        return tag_editor_current_tag_type_action(editor, &field)
               != TAG_EDITOR_TAG_TYPE_ACTION_NONE;
    case NATIVE_TAG_EDITOR_FOCUS_TAGS:
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
        case TAG_EDITOR_TAG_TYPE_ACTION_LAST:
        default:
            break;
        }
        return false;
    case NATIVE_TAG_EDITOR_FOCUS_PARSER_LEGEND:
    case NATIVE_TAG_EDITOR_FOCUS_PARSER_PREVIEW:
        return false;
    case NATIVE_TAG_EDITOR_FOCUS_LAST:
    default:
        break;
    }
    return false;
}

static bool
tag_editor_run_current(NcScreen *screen) {
    NativeTagEditorScreen *editor;

    editor = tag_editor_from_screen(screen);
    if (editor == NULL) {
        return false;
    }

    switch (editor->active_focus) {
    case NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES:
        return tag_editor_run_directory_current(editor);
    case NATIVE_TAG_EDITOR_FOCUS_TAG_TYPES:
        return tag_editor_run_tag_type_current(editor);
    case NATIVE_TAG_EDITOR_FOCUS_TAGS:
        return tag_editor_run_tag_current(editor);
    case NATIVE_TAG_EDITOR_FOCUS_PARSER_CHOICE:
        return tag_editor_run_parser_choice_current(editor);
    case NATIVE_TAG_EDITOR_FOCUS_PARSER_ACTIONS:
        return tag_editor_run_parser_action_current(editor);
    case NATIVE_TAG_EDITOR_FOCUS_PARSER_LEGEND:
    case NATIVE_TAG_EDITOR_FOCUS_PARSER_PREVIEW:
        return false;
    case NATIVE_TAG_EDITOR_FOCUS_LAST:
    default:
        break;
    }
    return false;
}

static void
tag_editor_switch_to(NcScreen *screen) {
    (void)nc_screen_switcher_finish_switch(screen);
    ncm_title_draw_header(STRLIT_ARGS("Tag editor"));
    return;
}

static void
tag_editor_resize(NcScreen *screen) {
    NativeTagEditorScreen *editor;
    int64 start_x;
    int64 width;

    editor = tag_editor_from_screen(screen);
    nc_screen_switcher_get_resize_params(screen, &start_x, &width, true);
    native_tag_editor_screen_set_geometry(
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
    NativeTagEditorScreen *editor;
    bool changed;

    editor = tag_editor_from_screen(screen);
    native_tag_editor_screen_finish_directory_change(editor);
    changed = tag_editor_update_from_mpd(editor, &global_mpd);
    nc_screen_clear_update_request(screen);
    if (changed && app_controller_is_screen_visible(screen)) {
        nc_screen_refresh(screen);
    }
    return;
}

static void
tag_editor_mouse_callback(NcScreen *screen, MEVENT event) {
    NativeTagEditorScreen *editor;
    int32 x;
    int32 y;

    editor = tag_editor_from_screen(screen);
    if (editor == NULL) {
        return;
    }

    if (!tag_editor_focus_is_main(editor->active_focus)) {
        x = event.x;
        y = event.y;
        if (nc_window_has_coords(&editor->parser_dialog_window, &x, &y)) {
            if (!tag_editor_mouse_move_to_parser_focus(
                    editor, NATIVE_TAG_EDITOR_FOCUS_PARSER_CHOICE)) {
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
                    editor, NATIVE_TAG_EDITOR_FOCUS_PARSER_ACTIONS)) {
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
                editor, NATIVE_TAG_EDITOR_COLUMN_DIRECTORIES)) {
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
        native_tag_editor_screen_finish_directory_change(editor);
        nc_screen_refresh(screen);
        return;
    }

    x = event.x;
    y = event.y;
    if (nc_window_has_coords(&editor->tag_types_window, &x, &y)) {
        if (!tag_editor_mouse_move_to_column(
                editor, NATIVE_TAG_EDITOR_COLUMN_TAG_TYPES)) {
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
                editor, NATIVE_TAG_EDITOR_COLUMN_TAGS)) {
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
tag_editor_mouse_scroll(NativeTagEditorScreen *screen,
                        enum NcScroll where) {
    NcMenu *menu;
    NcWindow *window;

    if (screen == NULL) {
        return;
    }
    menu = native_tag_editor_screen_active_menu(screen);
    window = native_tag_editor_screen_active_window(screen);
    tag_editor_mouse_scroll_menu(menu, window, where);
    native_tag_editor_screen_finish_directory_change(screen);
    tag_editor_finish_tag_type_change(screen, true);
    return;
}

static void
tag_editor_mouse_scroll_menu(NcMenu *menu, NcWindow *window,
                             enum NcScroll where) {
    enum NcScroll effective;
    int64 count;

    if ((menu == NULL) || (window == NULL)) {
        return;
    }
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
    for (int64 i = 0; i < count; i += 1) {
        nc_menu_scroll_selectable(menu, nc_window_height(window),
                                  effective);
    }
    return;
}

static bool
tag_editor_mouse_select_directory(NativeTagEditorScreen *screen,
                                  int32 y, bool enter) {
    NcMenu *menu;

    if (screen == NULL) {
        return false;
    }
    menu = nc_editor_pair_menu_base(&screen->directories);
    if ((y < 0) || (y >= nc_menu_item_count(menu))) {
        return false;
    }
    if (!nc_menu_goto_selectable(menu, y)) {
        return false;
    }
    native_tag_editor_screen_finish_directory_change(screen);
    if (enter) {
        return native_tag_editor_screen_enter_directory(screen);
    }
    return true;
}

static bool
tag_editor_mouse_select_tag_type(NativeTagEditorScreen *screen,
                                 int32 y, bool run) {
    NcMenu *menu;

    if (screen == NULL) {
        return false;
    }
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
tag_editor_mouse_select_tag(NativeTagEditorScreen *screen,
                            int32 y, bool run) {
    NcMenu *menu;

    if (screen == NULL) {
        return false;
    }
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
tag_editor_mouse_select_parser_dialog(NativeTagEditorScreen *screen,
                                      int32 y, bool run) {
    NcMenu *menu;

    if (screen == NULL) {
        return false;
    }
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
tag_editor_mouse_select_parser_row(NativeTagEditorScreen *screen,
                                   int32 y, bool run) {
    NcMenu *menu;

    if (screen == NULL) {
        return false;
    }
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
tag_editor_run_current_action(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return nc_screen_run_current(native_tag_editor_screen_base(screen));
}

static bool
tag_editor_mouse_move_to_column(NativeTagEditorScreen *screen,
                                enum NativeTagEditorColumn column) {
    if ((screen == NULL) || !tag_editor_focus_is_main(screen->active_focus)) {
        return false;
    }
    if (tag_editor_focus_is_main_column(screen->active_focus, column)) {
        return true;
    }
    while (screen->active_column < column) {
        if (!native_tag_editor_screen_next_column_available(screen)) {
            return false;
        }
        native_tag_editor_screen_next_column(screen);
    }
    while (screen->active_column > column) {
        if (!native_tag_editor_screen_previous_column_available(screen)) {
            return false;
        }
        native_tag_editor_screen_previous_column(screen);
    }
    tag_editor_update_menu_highlights(screen);
    return true;
}

static bool
tag_editor_mouse_move_to_parser_focus(NativeTagEditorScreen *screen,
                                      enum NativeTagEditorFocus focus) {
    if (screen == NULL) {
        return false;
    }
    if (focus == NATIVE_TAG_EDITOR_FOCUS_PARSER_CHOICE) {
        tag_editor_set_focus(screen, focus);
        return true;
    }
    if (screen->parser_mode == NATIVE_TAG_EDITOR_PARSER_NONE) {
        return false;
    }
    if ((focus == NATIVE_TAG_EDITOR_FOCUS_PARSER_ACTIONS)
        || tag_editor_focus_is_parser_helper(focus)) {
        tag_editor_set_focus(screen, focus);
        return true;
    }
    return false;
}

static bool
tag_editor_run_directory_current(NativeTagEditorScreen *screen) {
    return native_tag_editor_screen_enter_directory(screen);
}

static bool
tag_editor_run_tag_type_current(NativeTagEditorScreen *screen) {
    enum TagEditorTagTypeAction action;
    enum NcmTagsField field;

    action = tag_editor_current_tag_type_action(screen, &field);
    switch (action) {
    case TAG_EDITOR_TAG_TYPE_ACTION_FIELD:
        return tag_editor_prompt_tag_value(screen, field, true);
    case TAG_EDITOR_TAG_TYPE_ACTION_NUMBER_TRACKS:
        if (!tag_editor_confirm(screen, STRLIT_ARGS("Number tracks?"))) {
            return false;
        }
        if (!native_tag_editor_screen_number_tracks(
                screen, Config.tag_editor_extended_numeration)) {
            return false;
        }
        tag_editor_status_message(screen, STRLIT_ARGS("Tracks numbered"));
        return true;
    case TAG_EDITOR_TAG_TYPE_ACTION_FILENAME:
        native_tag_editor_screen_show_parser_dialog(screen);
        return true;
    case TAG_EDITOR_TAG_TYPE_ACTION_CAPITALIZE:
        tag_editor_status_message(screen, STRLIT_ARGS("Processing..."));
        native_tag_editor_screen_capitalize_first_letters(screen);
        tag_editor_status_message(screen, STRLIT_ARGS("Done"));
        return true;
    case TAG_EDITOR_TAG_TYPE_ACTION_LOWER:
        tag_editor_status_message(screen, STRLIT_ARGS("Processing..."));
        native_tag_editor_screen_lower_all_letters(screen);
        tag_editor_status_message(screen, STRLIT_ARGS("Done"));
        return true;
    case TAG_EDITOR_TAG_TYPE_ACTION_RESET:
        native_tag_editor_screen_clear_modifications(screen);
        tag_editor_status_message(screen, STRLIT_ARGS("Changes reset"));
        return true;
    case TAG_EDITOR_TAG_TYPE_ACTION_SAVE:
        return native_tag_editor_screen_save_modified(
            screen, Config.mpd_music_dir);
    case TAG_EDITOR_TAG_TYPE_ACTION_NONE:
        return false;
    case TAG_EDITOR_TAG_TYPE_ACTION_LAST:
    default:
        break;
    }
    return false;
}

static bool
tag_editor_run_tag_current(NativeTagEditorScreen *screen) {
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
tag_editor_run_parser_choice_current(NativeTagEditorScreen *screen) {
    NcMenu *menu;
    int64 choice;

    menu = nc_editor_string_menu_base(&screen->parser_dialog);
    if (!nc_menu_current_is_selectable(menu)) {
        return false;
    }
    choice = nc_menu_highlight(menu);
    if (choice == 0) {
        native_tag_editor_screen_show_parser_actions(
            screen, NATIVE_TAG_EDITOR_PARSER_TAGS_FROM_FILENAME);
        return true;
    }
    if (choice == 1) {
        native_tag_editor_screen_show_parser_actions(
            screen, NATIVE_TAG_EDITOR_PARSER_RENAME_FILES);
        return true;
    }
    if (choice == 2) {
        native_tag_editor_screen_close_parser(screen);
        return true;
    }
    return false;
}

static bool
tag_editor_run_parser_action_current(NativeTagEditorScreen *screen) {
    NcMenu *menu;
    int64 choice;
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
        native_tag_editor_screen_show_parser_preview(screen);
        tag_editor_status_message(screen, STRLIT_ARGS("Operation finished"));
        return true;
    }
    if (choice == TAG_EDITOR_PARSER_ACTION_LEGEND) {
        if (!tag_editor_build_parser_legend(screen)) {
            return false;
        }
        native_tag_editor_screen_show_parser_legend(screen);
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
                                      STRLIT_ARGS("Operation finished"));
            native_tag_editor_screen_close_parser(screen);
        }
        return success;
    }
    if (choice == TAG_EDITOR_PARSER_ACTION_CANCEL) {
        (void)tag_editor_save_recent_patterns(screen);
        native_tag_editor_screen_close_parser(screen);
        return true;
    }
    if (choice >= TAG_EDITOR_PARSER_ACTION_RECENT_START) {
        return tag_editor_apply_recent_pattern(screen, choice);
    }
    return false;
}

static bool
tag_editor_prompt_tag_value(NativeTagEditorScreen *screen,
                            enum NcmTagsField field, bool all_targets) {
    NcmMutableSong *song;
    NcmBuffer initial;
    NcmBuffer input;
    char *label;
    int32 label_len;
    enum NativeTagEditorPromptResult prompt_result;
    bool result;

    if ((screen == NULL) || (field == NCM_TAGS_FIELD_LAST)) {
        return false;
    }
    song = nc_tag_row_menu_current(&screen->tags);
    if (song == NULL) {
        return false;
    }

    label = ncm_tags_field_name(field);
    label_len = strlen32(label);
    initial = ncm_mutable_song_tags_buffer(
        song, field, Config.tags_separator, Config.tags_separator_len,
        Config.show_duplicate_tags);
    ncm_buffer_init(&input);
    if (screen->hooks.prompt == NULL) {
        prompt_result = NATIVE_TAG_EDITOR_PROMPT_ERROR;
    } else {
        NcmStringView initial_view;

        ncm_string_view_set(&initial_view, initial.data, initial.len);
        prompt_result = screen->hooks.prompt(
            screen->hooks.user, label, label_len, initial_view, &input);
    }
    ncm_buffer_destroy(&initial);

    if (prompt_result == NATIVE_TAG_EDITOR_PROMPT_ABORTED) {
        tag_editor_status_message(screen, STRLIT_ARGS("Action aborted"));
        ncm_buffer_destroy(&input);
        return false;
    }
    if (prompt_result != NATIVE_TAG_EDITOR_PROMPT_ACCEPTED) {
        ncm_buffer_destroy(&input);
        return false;
    }

    if (all_targets) {
        result = native_tag_editor_screen_apply_tag_to_selection(
            screen, field, input.data, input.len, Config.tags_separator,
            Config.tags_separator_len);
    } else {
        result = ncm_mutable_song_set_tags(
            song, field, input.data, input.len, Config.tags_separator,
            Config.tags_separator_len);
    }
    ncm_buffer_destroy(&input);
    return result;
}

static bool
tag_editor_prompt_current_filename(NativeTagEditorScreen *screen) {
    NcmMutableSong *song;
    NcmStringView current_name;
    NcmStringView initial;
    NcmBuffer input;
    enum NativeTagEditorPromptResult prompt_result;
    int32 dot;
    bool result;

    if (screen == NULL) {
        return false;
    }
    song = nc_tag_row_menu_current(&screen->tags);
    if (song == NULL) {
        return false;
    }

    if (!ncm_mutable_song_get_new_name(song, &current_name)) {
        current_name.data = song->name;
        current_name.len = song->name_len;
    }
    initial = current_name;
    dot = -1;
    for (int32 i = 0; i < current_name.len; i += 1) {
        if (current_name.data[i] == '.') {
            dot = i;
        }
    }
    if (dot >= 0) {
        initial.len = dot;
    }

    ncm_buffer_init(&input);
    if (screen->hooks.prompt == NULL) {
        prompt_result = NATIVE_TAG_EDITOR_PROMPT_ERROR;
    } else {
        prompt_result = screen->hooks.prompt(
            screen->hooks.user, STRLIT_ARGS("New filename"), initial,
            &input);
    }
    if (prompt_result == NATIVE_TAG_EDITOR_PROMPT_ABORTED) {
        tag_editor_status_message(screen, STRLIT_ARGS("Action aborted"));
        ncm_buffer_destroy(&input);
        return false;
    }
    if (prompt_result != NATIVE_TAG_EDITOR_PROMPT_ACCEPTED) {
        ncm_buffer_destroy(&input);
        return false;
    }
    if (input.len <= 0) {
        ncm_buffer_destroy(&input);
        return true;
    }
    result = tag_editor_set_song_filename_stem(song, input.data, input.len);
    ncm_buffer_destroy(&input);
    return result;
}

static bool
tag_editor_set_song_filename_stem(NcmMutableSong *song, char *stem,
                                  int32 stem_len) {
    NcmStringView current_name;
    NcmBuffer new_name;
    int32 dot;
    bool result;

    if ((song == NULL) || (stem == NULL) || (stem_len <= 0)) {
        return false;
    }
    if (!ncm_mutable_song_get_new_name(song, &current_name)) {
        current_name.data = song->name;
        current_name.len = song->name_len;
    }

    dot = -1;
    for (int32 i = 0; i < current_name.len; i += 1) {
        if (current_name.data[i] == '.') {
            dot = i;
        }
    }

    ncm_buffer_init(&new_name);
    ncm_buffer_append(&new_name, stem, stem_len);
    if (dot >= 0) {
        ncm_buffer_append(&new_name, current_name.data + dot,
                          current_name.len - dot);
    }
    result = ncm_mutable_song_set_new_name(song, new_name.data,
                                           new_name.len);
    ncm_buffer_destroy(&new_name);
    return result;
}

static bool
tag_editor_focus_is_main(enum NativeTagEditorFocus focus) {
    return (focus == NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES)
           || (focus == NATIVE_TAG_EDITOR_FOCUS_TAG_TYPES)
           || (focus == NATIVE_TAG_EDITOR_FOCUS_TAGS);
}

static bool
tag_editor_focus_is_main_column(enum NativeTagEditorFocus focus,
                                enum NativeTagEditorColumn column) {
    return focus == tag_editor_column_focus(column);
}

static bool
tag_editor_focus_is_parser_helper(enum NativeTagEditorFocus focus) {
    return (focus == NATIVE_TAG_EDITOR_FOCUS_PARSER_LEGEND)
           || (focus == NATIVE_TAG_EDITOR_FOCUS_PARSER_PREVIEW);
}

static enum NativeTagEditorFocus
tag_editor_column_focus(enum NativeTagEditorColumn column) {
    switch (column) {
    case NATIVE_TAG_EDITOR_COLUMN_DIRECTORIES:
        return NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES;
    case NATIVE_TAG_EDITOR_COLUMN_TAG_TYPES:
        return NATIVE_TAG_EDITOR_FOCUS_TAG_TYPES;
    case NATIVE_TAG_EDITOR_COLUMN_TAGS:
        return NATIVE_TAG_EDITOR_FOCUS_TAGS;
    case NATIVE_TAG_EDITOR_COLUMN_LAST:
    default:
        break;
    }
    return NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES;
}

static void
tag_editor_set_focus(NativeTagEditorScreen *screen,
                     enum NativeTagEditorFocus focus) {
    if (screen == NULL) {
        return;
    }
    screen->active_focus = focus;
    if (focus == NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES) {
        screen->active_column = NATIVE_TAG_EDITOR_COLUMN_DIRECTORIES;
    } else if (focus == NATIVE_TAG_EDITOR_FOCUS_TAG_TYPES) {
        screen->active_column = NATIVE_TAG_EDITOR_COLUMN_TAG_TYPES;
    } else if (focus == NATIVE_TAG_EDITOR_FOCUS_TAGS) {
        screen->active_column = NATIVE_TAG_EDITOR_COLUMN_TAGS;
    } else if (focus == NATIVE_TAG_EDITOR_FOCUS_PARSER_LEGEND) {
        screen->parser_preview_enabled = false;
    } else if (focus == NATIVE_TAG_EDITOR_FOCUS_PARSER_PREVIEW) {
        screen->parser_preview_enabled = true;
    }
    tag_editor_update_menu_highlights(screen);
    return;
}

static enum NativeTagEditorFocus
tag_editor_current_helper_focus(NativeTagEditorScreen *screen) {
    if (screen && !screen->parser_preview_enabled) {
        return NATIVE_TAG_EDITOR_FOCUS_PARSER_LEGEND;
    }
    return NATIVE_TAG_EDITOR_FOCUS_PARSER_PREVIEW;
}

static bool
tag_editor_tag_type_row_changed(NativeTagEditorScreen *screen) {
    NcMenu *menu;
    int64 highlight;
    bool changed;

    if (screen == NULL) {
        return false;
    }
    menu = nc_editor_string_menu_base(&screen->tag_types);
    highlight = nc_menu_highlight(menu);
    changed = screen->last_tag_type_highlight != highlight;
    screen->last_tag_type_highlight = highlight;
    return changed;
}

static void
tag_editor_finish_tag_type_change(NativeTagEditorScreen *screen,
                                  bool refresh_tags) {
    if (screen == NULL) {
        return;
    }
    if (screen->active_focus != NATIVE_TAG_EDITOR_FOCUS_TAG_TYPES) {
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

static bool
tag_editor_is_lockable(NcScreen *screen) {
    (void)screen;
    return true;
}

static bool
tag_editor_is_mergable(NcScreen *screen) {
    (void)screen;
    return true;
}

static void
tag_editor_destroy_callback(NcScreen *screen) {
    native_tag_editor_screen_destroy(tag_editor_from_screen(screen));
    return;
}

static void
tag_editor_initialize_buffers(NativeTagEditorScreen *screen) {
    ncm_buffer_init(&screen->current_dir);
    ncm_buffer_init(&screen->displayed_dir);
    ncm_buffer_init(&screen->observed_dir);
    ncm_buffer_init(&screen->highlighted_dir);
    ncm_buffer_init(&screen->directories_title);
    ncm_buffer_init(&screen->tag_types_title);
    ncm_buffer_init(&screen->tags_title);
    ncm_buffer_init(&screen->parser_dialog_title);
    ncm_buffer_init(&screen->parser_title);
    ncm_buffer_init(&screen->parser_helper_title);
    ncm_buffer_init(&screen->parser_legend);
    ncm_buffer_init(&screen->parser_preview);
    ncm_buffer_array_init(&screen->recent_patterns);
    ncm_buffer_init(&screen->directory_filter_constraint);
    ncm_buffer_init(&screen->tag_filter_constraint);
    ncm_buffer_init(&screen->directory_search_constraint);
    ncm_buffer_init(&screen->tag_search_constraint);
    ncm_buffer_init(&screen->pattern);
    return;
}

static void
tag_editor_destroy_buffers(NativeTagEditorScreen *screen) {
    ncm_buffer_destroy(&screen->pattern);
    ncm_buffer_destroy(&screen->tag_search_constraint);
    ncm_buffer_destroy(&screen->directory_search_constraint);
    ncm_buffer_destroy(&screen->tag_filter_constraint);
    ncm_buffer_destroy(&screen->directory_filter_constraint);
    ncm_buffer_array_destroy(&screen->recent_patterns);
    ncm_buffer_destroy(&screen->parser_preview);
    ncm_buffer_destroy(&screen->parser_legend);
    ncm_buffer_destroy(&screen->parser_helper_title);
    ncm_buffer_destroy(&screen->parser_title);
    ncm_buffer_destroy(&screen->parser_dialog_title);
    ncm_buffer_destroy(&screen->tags_title);
    ncm_buffer_destroy(&screen->tag_types_title);
    ncm_buffer_destroy(&screen->directories_title);
    ncm_buffer_destroy(&screen->highlighted_dir);
    ncm_buffer_destroy(&screen->observed_dir);
    ncm_buffer_destroy(&screen->displayed_dir);
    ncm_buffer_destroy(&screen->current_dir);
    return;
}

static void
tag_editor_initialize_regexes(NativeTagEditorScreen *screen) {
    ncm_regex_init(&screen->directory_filter_regex);
    ncm_regex_init(&screen->tag_filter_regex);
    ncm_regex_init(&screen->directory_search_regex);
    ncm_regex_init(&screen->tag_search_regex);
    return;
}

static void
tag_editor_destroy_regexes(NativeTagEditorScreen *screen) {
    ncm_regex_destroy(&screen->tag_search_regex);
    ncm_regex_destroy(&screen->directory_search_regex);
    ncm_regex_destroy(&screen->tag_filter_regex);
    ncm_regex_destroy(&screen->directory_filter_regex);
    return;
}

static bool
tag_editor_compile_constraint(NcmRegex *regex, char *pattern,
                              int32 pattern_len, uint32 regex_flags,
                              NcmError *error) {
    NcmRegex compiled;

    if (regex == NULL) {
        return false;
    }
    ncm_regex_init(&compiled);
    if (!ncm_regex_compile(&compiled, pattern, pattern_len, regex_flags,
                           error)) {
        ncm_regex_destroy(&compiled);
        return false;
    }
    ncm_regex_destroy(regex);
    *regex = compiled;
    return true;
}

static bool
tag_editor_set_buffer(NcmBuffer *buffer, char *data, int32 data_len) {
    if (!ncm_buffer_set(buffer, data, data_len)) {
        return false;
    }
    if (buffer->data) {
        buffer->data[buffer->len] = '\0';
    }
    return true;
}

static void
tag_editor_update_titles(NativeTagEditorScreen *screen,
                         bool update_windows) {
    if (screen == NULL) {
        return;
    }
    tag_editor_update_visible_counts(screen);
    ncm_buffer_clear(&screen->directories_title);
    ncm_buffer_clear(&screen->tag_types_title);
    ncm_buffer_clear(&screen->tags_title);
    ncm_buffer_clear(&screen->parser_dialog_title);
    ncm_buffer_clear(&screen->parser_title);
    ncm_buffer_clear(&screen->parser_helper_title);
    if (Config.titles_visibility) {
        ncm_buffer_append(&screen->directories_title,
                          STRLIT_ARGS("Directories"));
        ncm_buffer_append(&screen->tag_types_title,
                          STRLIT_ARGS("Tag types"));
        ncm_buffer_append(&screen->tags_title, STRLIT_ARGS("Tags"));
        if (screen->parser_mode
            == NATIVE_TAG_EDITOR_PARSER_TAGS_FROM_FILENAME) {
            ncm_buffer_append(&screen->parser_title,
                              STRLIT_ARGS("Get tags from filename"));
        } else if (screen->parser_mode
                   == NATIVE_TAG_EDITOR_PARSER_RENAME_FILES) {
            ncm_buffer_append(&screen->parser_title,
                              STRLIT_ARGS("Rename files"));
        } else {
            ncm_buffer_append(&screen->parser_title,
                              STRLIT_ARGS("Pattern"));
        }
        if ((screen->active_focus == NATIVE_TAG_EDITOR_FOCUS_PARSER_LEGEND)
            || !screen->parser_preview_enabled) {
            ncm_buffer_append(&screen->parser_helper_title,
                              STRLIT_ARGS("Legend"));
        } else {
            ncm_buffer_append(&screen->parser_helper_title,
                              STRLIT_ARGS("Preview"));
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
tag_editor_update_visible_counts(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->last_known_directory_count = nc_menu_item_count(
        nc_editor_pair_menu_base(&screen->directories));
    screen->last_known_tag_count = nc_menu_item_count(
        nc_tag_row_menu_base(&screen->tags));
    return;
}

static void
tag_editor_observe_current_directory(NativeTagEditorScreen *screen) {
    char *path;
    int32 path_len;
    NcMenu *menu;

    if (screen == NULL) {
        return;
    }
    menu = nc_editor_pair_menu_base(&screen->directories);
    screen->last_directory_highlight = nc_menu_highlight(menu);
    if (!tag_editor_current_directory_path(screen, &path, &path_len)) {
        ncm_buffer_clear(&screen->observed_dir);
        screen->observed_dir_valid = false;
        return;
    }
    ncm_buffer_set(&screen->observed_dir, path, path_len);
    screen->observed_dir_valid = true;
    return;
}

static bool
tag_editor_directory_row_changed(NativeTagEditorScreen *screen) {
    char *path;
    int32 path_len;
    NcMenu *menu;
    bool changed;

    if (screen == NULL) {
        return false;
    }
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
tag_editor_current_directory_path(NativeTagEditorScreen *screen,
                                  char **path, int32 *path_len) {
    NcMenuStringPair *pair;

    if (path) {
        *path = NULL;
    }
    if (path_len) {
        *path_len = 0;
    }
    if (screen == NULL) {
        return false;
    }
    pair = nc_editor_pair_menu_current(&screen->directories);
    if ((pair == NULL) || (pair->second == NULL)) {
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
tag_editor_directory_has_subdirectories(
    NativeTagEditorScreen *screen, char *path, int32 path_len) {
    NcmDirectoryArray directories;
    NcmError error;
    bool result;

    (void)screen;
    if ((path == NULL) || (path_len <= 0)) {
        return false;
    }

    ncm_error_clear(&error);
    ncm_directory_array_init(&directories);
    result = ncm_mpd_client_get_directory_list(
        &global_mpd, path, &directories, &error)
        && (directories.len > 0);
    ncm_error_clear(&error);
    ncm_directory_array_destroy(&directories);
    return result;
}

static bool
tag_editor_directory_is_control(char *label, int32 label_len) {
    return STREQUAL(label, label_len, STRLIT_ARGS("."))
           || STREQUAL(label, label_len, STRLIT_ARGS(".."));
}

static bool
tag_editor_highlight_directory_path(NativeTagEditorScreen *screen,
                                    char *path, int32 path_len) {
    NcMenu *menu;

    if ((screen == NULL) || (path == NULL) || (path_len <= 0)) {
        return false;
    }
    menu = nc_editor_pair_menu_base(&screen->directories);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcMenuStringPair *pair;

        pair = nc_menu_active_item_at(menu, i);
        if ((pair == NULL) || (pair->second == NULL)) {
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
tag_editor_highlight_song_uri(NativeTagEditorScreen *screen, char *uri,
                              int32 uri_len) {
    NcMenu *menu;

    if ((screen == NULL) || (uri == NULL) || (uri_len <= 0)) {
        return false;
    }
    menu = nc_tag_row_menu_base(&screen->tags);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmMutableSong *song;

        song = nc_menu_active_item_at(menu, i);
        if ((song == NULL) || (song->uri == NULL)) {
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
tag_editor_current_directory_pair(NativeTagEditorScreen *screen,
                                  NcMenuStringPair **pair) {
    NcMenuStringPair *current;

    if (pair) {
        *pair = NULL;
    }
    if (screen == NULL) {
        return false;
    }
    current = nc_editor_pair_menu_current(&screen->directories);
    if ((current == NULL) || (current->first == NULL)
        || (current->second == NULL)) {
        return false;
    }
    if (pair) {
        *pair = current;
    }
    return true;
}

static bool
tag_editor_build_renamed_directory(NativeTagEditorScreen *screen,
                                   char *name, int32 name_len,
                                   NcmBuffer *result) {
    if ((screen == NULL) || (result == NULL) || (name == NULL)
        || (name_len <= 0)) {
        return false;
    }
    return ncm_fs_join(result, screen->current_dir.data,
                       screen->current_dir.len, name, name_len);
}

static void
tag_editor_status_directory_renamed(NativeTagEditorScreen *screen,
                                    char *name, int32 name_len) {
    NcmBuffer message;

    ncm_buffer_init(&message);
    ncm_buffer_append(&message, STRLIT_ARGS("Directory renamed to \""));
    ncm_buffer_append(&message, name, name_len);
    ncm_buffer_append(&message, STRLIT_ARGS("\""));
    tag_editor_status_message(screen, message.data, message.len);
    ncm_buffer_destroy(&message);
    return;
}

static void
tag_editor_status_directory_rename_error(NativeTagEditorScreen *screen,
                                         char *name, int32 name_len,
                                         NcmError *error) {
    NcmBuffer message;
    int32 error_len;

    ncm_buffer_init(&message);
    ncm_buffer_append(&message, STRLIT_ARGS("Couldn't rename \""));
    ncm_buffer_append(&message, name, name_len);
    ncm_buffer_append(&message, STRLIT_ARGS("\": "));
    if (error && ncm_error_is_set(error)) {
        error_len = strlen32(error->message);
        ncm_buffer_append(&message, error->message, error_len);
    } else {
        ncm_buffer_append(&message, STRLIT_ARGS("unknown error"));
    }
    tag_editor_status_message(screen, message.data, message.len);
    ncm_buffer_destroy(&message);
    return;
}

static bool
tag_editor_has_modified_songs(NativeTagEditorScreen *screen) {
    NcMenu *menu;

    if (screen == NULL) {
        return false;
    }
    menu = nc_tag_row_menu_base(&screen->tags);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmMutableSong *song;

        song = nc_menu_active_item_at(menu, i);
        if (song && ncm_mutable_song_is_modified(song)) {
            return true;
        }
    }
    return false;
}

static void
tag_editor_preserve_current_directory(NativeTagEditorScreen *screen,
                                      NcmBuffer *path) {
    char *data;
    int32 data_len;

    if (path == NULL) {
        return;
    }
    ncm_buffer_clear(path);
    if (tag_editor_current_directory_path(screen, &data, &data_len)) {
        ncm_buffer_set(path, data, data_len);
    }
    return;
}

static void
tag_editor_restore_current_directory(NativeTagEditorScreen *screen,
                                     NcmBuffer *path) {
    NcMenu *menu;

    if ((screen == NULL) || (path == NULL) || (path->len <= 0)) {
        return;
    }
    menu = nc_editor_pair_menu_base(&screen->directories);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcMenuStringPair *pair;

        pair = nc_menu_active_item_at(menu, i);
        if ((pair == NULL) || (pair->second == NULL)) {
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
tag_editor_preserve_current_song(NativeTagEditorScreen *screen,
                                 NcmBuffer *uri) {
    NcmMutableSong *current;

    if (uri == NULL) {
        return;
    }
    ncm_buffer_clear(uri);
    if (screen == NULL) {
        return;
    }
    current = nc_tag_row_menu_current(&screen->tags);
    if ((current == NULL) || (current->uri == NULL)
        || (current->uri_len <= 0)) {
        return;
    }
    ncm_buffer_set(uri, current->uri, current->uri_len);
    return;
}

static void
tag_editor_restore_current_song(NativeTagEditorScreen *screen,
                               NcmBuffer *uri) {
    NcMenu *menu;

    if ((screen == NULL) || (uri == NULL) || (uri->len <= 0)) {
        return;
    }
    menu = nc_tag_row_menu_base(&screen->tags);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmMutableSong *item;

        item = nc_menu_active_item_at(menu, i);
        if ((item == NULL) || (item->uri == NULL)) {
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
tag_editor_add_control_directory(NativeTagEditorScreen *screen) {
    char *dir;
    int32 dir_len;
    int32 parent_len;

    if (screen == NULL) {
        return false;
    }
    dir = screen->current_dir.data;
    dir_len = screen->current_dir.len;
    if ((dir == NULL) || (dir_len <= 0)
        || STREQUAL(dir, dir_len, STRLIT_ARGS("/"))) {
        return native_tag_editor_screen_add_directory(
            screen, STRLIT_ARGS("."), STRLIT_ARGS("/"));
    }

    parent_len = ncm_string_parent_directory_len(dir, dir_len);
    if (parent_len <= 0) {
        return native_tag_editor_screen_add_directory(
            screen, STRLIT_ARGS(".."), STRLIT_ARGS("/"));
    }
    return native_tag_editor_screen_add_directory(
        screen, STRLIT_ARGS(".."), dir, parent_len);
}

static void
tag_editor_sort_directories(NcmDirectoryArray *directories) {
    if (directories == NULL) {
        return;
    }
    for (int32 i = 1; i < directories->len; i += 1) {
        NcmDirectory current;
        int32 j;

        ncm_directory_init(&current);
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
    if (songs == NULL) {
        return;
    }
    for (int32 i = 1; i < songs->len; i += 1) {
        NcmSong current;
        int32 j;

        ncm_song_init(&current);
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
                        NcmError *error) {
    NcmBuffer message;

    ncm_buffer_init(&message);
    ncm_buffer_append(&message, context, context_len);
    if (error && (error->message[0] != 0)) {
        ncm_buffer_append(&message, STRLIT_ARGS(": "));
        ncm_buffer_append(&message, error->message,
                          strlen32(error->message));
    }
    ncm_buffer_append_byte(&message, '\0');
    ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                message.data);
    ncm_buffer_destroy(&message);
    return;
}

static bool
tag_editor_update_from_mpd(NativeTagEditorScreen *screen,
                           NcmMpdClient *client) {
    NcmError error;
    bool changed;
    bool ok;

    if (screen == NULL) {
        return false;
    }

    changed = false;
    ncm_error_clear(&error);
    if (screen->directories_update_requested
        || nc_menu_empty(nc_editor_pair_menu_base(&screen->directories))) {
        ok = tag_editor_reload_directories_from_mpd(screen, client,
                                                    &error);
        if (!ok) {
            screen->directories_update_requested = false;
            tag_editor_report_error(
                STRLIT_ARGS("Could not fetch directories"), &error);
            ncm_error_clear(&error);
            tag_editor_update_titles(screen, true);
            return changed;
        }
        changed = true;
    }

    native_tag_editor_screen_finish_directory_change(screen);
    if (!screen->tags_update_requested
        && !nc_menu_empty(nc_tag_row_menu_base(&screen->tags))) {
        tag_editor_update_titles(screen, true);
        return changed;
    }

    ncm_error_clear(&error);
    ok = tag_editor_reload_songs_from_mpd(screen, client, &error);
    if (!ok) {
        screen->tags_update_requested = false;
        tag_editor_report_error(STRLIT_ARGS("Could not fetch songs"),
                                &error);
        ncm_error_clear(&error);
        tag_editor_update_titles(screen, true);
        return changed;
    }
    changed = true;
    tag_editor_update_titles(screen, true);
    return changed;
}

static bool
tag_editor_reload_directories_from_mpd(NativeTagEditorScreen *screen,
                                       NcmMpdClient *client,
                                       NcmError *error) {
    NcmDirectoryArray directories;
    NcmBuffer preserved;
    char *dir;
    bool ok;

    if (screen == NULL) {
        return false;
    }

    ncm_directory_array_init(&directories);
    ncm_buffer_init(&preserved);
    tag_editor_preserve_current_directory(screen, &preserved);
    if ((preserved.len <= 0) && (screen->highlighted_dir.len > 0)) {
        ncm_buffer_set(&preserved, screen->highlighted_dir.data,
                       screen->highlighted_dir.len);
    }
    dir = screen->current_dir.data;
    if (dir == NULL) {
        dir = "/";
    }

    ok = ncm_mpd_client_get_directory_list(client, dir, &directories,
                                           error);
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
            ok = native_tag_editor_screen_add_directory(
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
        ncm_buffer_clear(&screen->highlighted_dir);
        screen->directories_update_requested = false;
    }

    ncm_buffer_destroy(&preserved);
    ncm_directory_array_destroy(&directories);
    return ok;
}

static bool
tag_editor_reload_songs_from_mpd(NativeTagEditorScreen *screen,
                                 NcmMpdClient *client, NcmError *error) {
    NcmMpdSongList list;
    NcmSongArray songs;
    NcmBuffer preserved_uri;
    char *path;
    int32 path_len;
    bool ok;

    if (screen == NULL) {
        return false;
    }
    if (!tag_editor_current_directory_path(screen, &path, &path_len)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing directory"));
        return false;
    }

    ncm_mpd_song_list_init(&list);
    ncm_song_array_init(&songs);
    ncm_buffer_init(&preserved_uri);
    tag_editor_preserve_current_song(screen, &preserved_uri);

    ok = ncm_mpd_client_get_songs(client, path, &list, error);
    if (ok) {
        ok = ncm_mpd_song_list_to_song_array(&list, &songs);
    }
    if (ok) {
        tag_editor_sort_songs(&songs);
        ok = native_tag_editor_screen_load_songs(screen, &songs);
    }
    if (ok) {
        if (screen->tag_filter_enabled) {
            nc_menu_apply_filter(nc_tag_row_menu_base(&screen->tags));
        }
        tag_editor_restore_current_song(screen, &preserved_uri);
        screen->tags_update_requested = false;
        tag_editor_update_titles(screen, true);
    }

    ncm_buffer_destroy(&preserved_uri);
    ncm_song_array_destroy(&songs);
    ncm_mpd_song_list_destroy(&list);
    return ok;
}

static void
tag_editor_layout(NativeTagEditorScreen *screen) {
    int64 separator_width;
    int64 parser_dialog_x_space;
    int64 parser_dialog_y_space;
    int64 parser_x_space;
    int64 parser_y_space;
    int64 screen_height;

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

static int64
tag_editor_min_int64(int64 left, int64 right) {
    if (left < right) {
        return left;
    }
    return right;
}

static int64
tag_editor_separator_width(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return 0;
    }
    if (screen->width >= 5) {
        return 1;
    }
    return 0;
}

static bool
tag_editor_initialize_tag_types(NativeTagEditorScreen *screen) {
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
    if (!tag_editor_append_string_row(menu, STRLIT_ARGS("Filename"),
                                      NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    nc_editor_string_menu_add_separator(menu);
    if (Config.titles_visibility) {
        if (!tag_editor_append_string_row(
                menu, STRLIT_ARGS("Options"), NC_MENU_ITEM_INACTIVE)) {
            return false;
        }
        nc_editor_string_menu_add_separator(menu);
    }
    if (!tag_editor_append_string_row(
            menu, STRLIT_ARGS("Capitalize First Letters"),
            NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    if (!tag_editor_append_string_row(menu, STRLIT_ARGS("lower all letters"),
                                      NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    nc_editor_string_menu_add_separator(menu);
    if (!tag_editor_append_string_row(menu, STRLIT_ARGS("Reset"),
                                      NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    return tag_editor_append_string_row(menu, STRLIT_ARGS("Save"),
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
    nc_menu_string_init(&string);
    ok = nc_menu_string_set(&string, data, data_len);
    if (ok) {
        nc_editor_string_menu_add_with_flags(menu, &string, flags);
    }
    nc_menu_string_destroy(&string);
    return ok;
}

static void
tag_editor_configure_menu(NcMenu *menu) {
    if (menu == NULL) {
        return;
    }
    nc_menu_set_selected_prefix(menu, &Config.selected_item_prefix);
    nc_menu_set_selected_suffix(menu, &Config.selected_item_suffix);
    nc_menu_set_cyclic_scrolling(menu, Config.use_cyclic_scrolling);
    nc_menu_set_centered_cursor(menu, Config.centered_cursor);
    return;
}

static void
tag_editor_configure_menus(NativeTagEditorScreen *screen) {
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
tag_editor_update_menu_highlights(
    NativeTagEditorScreen *screen) {
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

    active = native_tag_editor_screen_active_menu(screen);
    if (active) {
        nc_menu_set_highlight_prefix(active, &Config.current_item_prefix);
        nc_menu_set_highlight_suffix(active, &Config.current_item_suffix);
    }
    tag_editor_update_parser_borders(screen);
    return;
}

static void
tag_editor_update_parser_borders(NativeTagEditorScreen *screen) {
    NcBorder dialog_border;
    NcBorder parser_border;
    NcBorder helper_border;

    if (screen == NULL) {
        return;
    }
    dialog_border = Config.window_border;
    parser_border = Config.window_border;
    helper_border = Config.window_border;
    if (screen->active_focus == NATIVE_TAG_EDITOR_FOCUS_PARSER_CHOICE) {
        dialog_border = Config.active_window_border;
    } else if (screen->active_focus
               == NATIVE_TAG_EDITOR_FOCUS_PARSER_ACTIONS) {
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
tag_editor_refresh_active_helper(NativeTagEditorScreen *screen) {
    NcmBuffer *buffer;

    if (screen == NULL) {
        return;
    }
    nc_window_display(&screen->parser_helper_window);
    if (screen->active_focus == NATIVE_TAG_EDITOR_FOCUS_PARSER_PREVIEW) {
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
    if ((window == NULL) || (menu == NULL)) {
        return;
    }
    nc_menu_prepare_refresh(menu, nc_window_height(window), NULL, NULL);
    nc_window_display(window);
    nc_menu_refresh(menu, window, nc_window_width(window),
                    nc_window_height(window));
    return;
}

static void
tag_editor_draw_separators(NativeTagEditorScreen *screen) {
    if (tag_editor_separator_width(screen) <= 0) {
        return;
    }
    nc_screen_draw_vertical_separator(screen->middle_start_x - 1);
    nc_screen_draw_vertical_separator(screen->right_start_x - 1);
    return;
}

static NcMenuDisplayCallbacks
tag_editor_directory_display_callbacks(NativeTagEditorScreen *screen) {
    NcMenuDisplayCallbacks callbacks;

    callbacks = (NcMenuDisplayCallbacks){0};
    callbacks.draw = tag_editor_draw_directory;
    callbacks.filter = tag_editor_directory_filter;
    callbacks.user = screen;
    return callbacks;
}

static NcMenuDisplayCallbacks
tag_editor_tag_type_display_callbacks(NativeTagEditorScreen *screen) {
    NcMenuDisplayCallbacks callbacks;

    callbacks = (NcMenuDisplayCallbacks){0};
    callbacks.draw = tag_editor_draw_string;
    callbacks.user = screen;
    return callbacks;
}

static NcMenuDisplayCallbacks
tag_editor_tag_display_callbacks(NativeTagEditorScreen *screen) {
    NcMenuDisplayCallbacks callbacks;

    callbacks = (NcMenuDisplayCallbacks){0};
    callbacks.draw = tag_editor_draw_tag;
    callbacks.filter = tag_editor_tag_filter;
    callbacks.user = screen;
    return callbacks;
}

static void
tag_editor_draw_directory(NcMenu *menu, NcWindow *window, void *item,
                          int64 pos, void *user) {
    NcMenuStringPair *pair;
    NcmBuffer converted;

    (void)menu;
    (void)pos;
    (void)user;
    pair = item;
    if ((window == NULL) || (pair == NULL) || (pair->first == NULL)) {
        return;
    }
    converted = ncm_charset_utf8_to_locale(pair->first, pair->first_len);
    nc_window_print_data(window, converted.data, converted.len);
    ncm_buffer_destroy(&converted);
    return;
}

static void
tag_editor_draw_string(NcMenu *menu, NcWindow *window, void *item,
                       int64 pos, void *user) {
    NcMenuString *string;
    NcmBuffer converted;

    (void)menu;
    (void)pos;
    (void)user;
    string = item;
    if ((window == NULL) || (string == NULL) || (string->data == NULL)) {
        return;
    }
    converted = ncm_charset_utf8_to_locale(string->data, string->len);
    nc_window_print_data(window, converted.data, converted.len);
    ncm_buffer_destroy(&converted);
    return;
}

static void
tag_editor_draw_tag(NcMenu *menu, NcWindow *window, void *item,
                    int64 pos, void *user) {
    NativeTagEditorScreen *screen;
    NcBuffer buffer;

    (void)menu;
    (void)pos;
    screen = user;
    if ((screen == NULL) || (window == NULL) || (item == NULL)) {
        return;
    }
    nc_buffer_init(&buffer);
    tag_editor_append_tag_display_value(screen, item, &buffer);
    tag_editor_print_buffer(window, &buffer);
    nc_buffer_destroy(&buffer);
    return;
}

static void
tag_editor_append_tag_display_value(NativeTagEditorScreen *screen,
                                    NcmMutableSong *song,
                                    NcBuffer *buffer) {
    NcMenu *tag_types;
    int64 choice;

    if ((screen == NULL) || (song == NULL) || (buffer == NULL)) {
        return;
    }
    if (ncm_mutable_song_is_modified(song)) {
        nc_buffer_append_data(buffer, Config.modified_item_prefix.data,
                              Config.modified_item_prefix.len);
    }

    tag_types = nc_editor_string_menu_base(&screen->tag_types);
    choice = nc_menu_highlight(tag_types);
    if ((choice >= 0) && (choice < 11)) {
        NcmBuffer tag;
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
        ncm_buffer_destroy(&tag);
        return;
    }

    if (choice == 12) {
        tag_editor_append_locale(buffer, song->name, song->name_len);
        if (song->new_name && (song->new_name_len > 0)) {
            tag_editor_append_formatted_color(buffer, &Config.color2);
            nc_buffer_append_data(buffer, STRLIT_ARGS(" -> "));
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
    NcmBuffer converted;

    if ((buffer == NULL) || (data == NULL) || (data_len <= 0)) {
        return;
    }
    converted = ncm_charset_utf8_to_locale(data, data_len);
    nc_buffer_append_data(buffer, converted.data, converted.len);
    ncm_buffer_destroy(&converted);
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
    NativeTagEditorScreen *screen;
    NcMenuStringPair *pair;

    (void)menu;
    screen = user;
    pair = item;
    return tag_editor_directory_matches(screen, pair);
}

static bool
tag_editor_tag_filter(NcMenu *menu, void *item, void *user) {
    NativeTagEditorScreen *screen;
    NcmMutableSong *song;

    (void)menu;
    screen = user;
    song = item;
    return tag_editor_tag_matches(screen, song);
}

static bool
tag_editor_copy_selected_song_at(NativeTagEditorScreen *screen,
                                 NcmSongArray *songs, int64 pos) {
    NcmMutableSong *source;
    NcmSong song;

    source = nc_menu_active_item_at(
        nc_tag_row_menu_base(&screen->tags), pos);
    if (source == NULL) {
        return false;
    }

    ncm_song_init(&song);
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
tag_editor_for_each_target(NativeTagEditorScreen *screen,
                            bool (*cb)(NcmMutableSong *song, void *user),
                            void *user) {
    NcMenu *menu;
    bool has_selected;

    if ((screen == NULL) || (cb == NULL)) {
        return false;
    }
    menu = nc_tag_row_menu_base(&screen->tags);
    has_selected = nc_menu_has_selected(menu);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmMutableSong *song;

        if (has_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        song = nc_menu_active_item_at(menu, i);
        if (song && !cb(song, user)) {
            return false;
        }
    }
    return true;
}

static bool
tag_editor_set_song_tag_callback(NcmMutableSong *song, void *user) {
    TagSetter *setter;

    setter = user;
    return ncm_mutable_song_set_tags(song, setter->field, setter->value,
                                     setter->value_len, setter->separator,
                                     setter->separator_len);
}

static bool
tag_editor_number_song_callback(NcmMutableSong *song, void *user) {
    TrackNumberer *numberer;
    NcmStringView view;
    char buffer[64];
    int32 len;

    numberer = user;
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
    if (len >= (int32)SIZEOF(buffer)) {
        len = (int32)SIZEOF(buffer) - 1;
    }
    numberer->current += 1;
    if (!ncm_mutable_song_set_tag(song, NCM_TAGS_FIELD_TRACK, 0,
                                  buffer, len)) {
        return false;
    }
    for (int32 i = 1; ncm_mutable_song_get_tag(
             song, NCM_TAGS_FIELD_TRACK, i, &view); i += 1) {
        if (!ncm_mutable_song_set_tag(
                song, NCM_TAGS_FIELD_TRACK, i, STRLIT_ARGS(""))) {
            return false;
        }
    }
    return true;
}

static bool
tag_editor_capitalize_song_callback(NcmMutableSong *song, void *user) {
    (void)user;
    for (int32 field_idx = 0;
         ncm_song_info_tags[field_idx].name; field_idx += 1) {
        enum NcmTagsField field;

        field = ncm_song_info_tags[field_idx].field;
        for (int32 i = 0; ; i += 1) {
            NcmStringView view;
            NcmBuffer converted;
            int32 converted_len;

            if (!ncm_mutable_song_get_tag(song, field, i, &view)) {
                break;
            }

            ncm_buffer_init(&converted);
            converted_len = utf8_capitalize_first_letters(
                view.data, view.len, NULL, 0);
            ncm_buffer_reserve(&converted, converted_len);
            converted.len = utf8_capitalize_first_letters(
                view.data, view.len, converted.data, converted_len);
            if (converted.data) {
                converted.data[converted.len] = '\0';
            }
            if (!ncm_mutable_song_set_tag(song, field, i, converted.data,
                                          converted.len)) {
                ncm_buffer_destroy(&converted);
                return false;
            }
            ncm_buffer_destroy(&converted);
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
            NcmBuffer buffer;

            if (!ncm_mutable_song_get_tag(song, field, i, &view)) {
                break;
            }
            ncm_buffer_init(&buffer);
            ncm_buffer_append(&buffer, view.data, view.len);
            tag_editor_lower_ascii_buffer(&buffer);
            if (!ncm_mutable_song_set_tag(song, field, i, buffer.data,
                                          buffer.len)) {
                ncm_buffer_destroy(&buffer);
                return false;
            }
            ncm_buffer_destroy(&buffer);
        }
    }
    return true;
}

static bool
tag_editor_save_song_callback(NcmMutableSong *song, void *user) {
    SaveContext *context;
    int32 error;

    context = user;
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
        context->screen, STRLIT_ARGS("Writing tags in \""), song,
        STRLIT_ARGS("\"..."));
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
tag_editor_save_status_with_name(
    NativeTagEditorScreen *screen, char *prefix, int32 prefix_len,
    NcmMutableSong *song, char *suffix, int32 suffix_len) {
    NcmBuffer message;

    if ((screen == NULL) || (song == NULL)) {
        return;
    }

    ncm_buffer_init(&message);
    ncm_buffer_append(&message, prefix, prefix_len);
    if (song->name) {
        ncm_buffer_append(&message, song->name, song->name_len);
    }
    ncm_buffer_append(&message, suffix, suffix_len);
    tag_editor_status_message(screen, message.data, message.len);
    ncm_buffer_destroy(&message);
    return;
}

static void
tag_editor_save_status_error(NativeTagEditorScreen *screen,
                             NcmMutableSong *song, int32 error) {
    NcmBuffer message;
    char *system_error;

    if ((screen == NULL) || (song == NULL)) {
        return;
    }

    system_error = strerror(error);
    ncm_buffer_init(&message);
    ncm_buffer_append(&message, STRLIT_ARGS(
                          "Error while writing tags to \""));
    if (song->name) {
        ncm_buffer_append(&message, song->name, song->name_len);
    }
    ncm_buffer_append(&message, STRLIT_ARGS("\": "));
    ncm_buffer_append(&message, system_error, strlen32(system_error));
    tag_editor_status_message(screen, message.data, message.len);
    ncm_buffer_destroy(&message);
    return;
}

static void
tag_editor_update_modified_directory(
    NativeTagEditorScreen *screen, NcmBuffer *directory) {
    if ((screen == NULL) || (directory == NULL)) {
        return;
    }
    if (screen->hooks.update_directory == NULL) {
        return;
    }
    ncm_buffer_reserve(directory, 1);
    directory->data[directory->len] = '\0';
    screen->hooks.update_directory(screen->hooks.user, directory->data,
                                   directory->len);
    return;
}

static void
tag_editor_save_context_add_directory(
    SaveContext *context, NcmMutableSong *song) {
    NcmBuffer shared;
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
        ncm_buffer_set(&context->shared_directory,
                       directory, directory_len);
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
    ncm_buffer_destroy(&context->shared_directory);
    context->shared_directory = shared;
    return;
}

static bool
tag_editor_tag_matches(NativeTagEditorScreen *screen, NcmMutableSong *song) {
    if (!screen->tag_filter_enabled) {
        return true;
    }
    return tag_editor_tag_matches_regex(screen, song,
                                        &screen->tag_filter_regex);
}

static bool
tag_editor_directory_matches(NativeTagEditorScreen *screen,
                             NcMenuStringPair *pair) {
    if (!screen->directory_filter_enabled) {
        return true;
    }
    return tag_editor_directory_matches_regex(
        pair, &screen->directory_filter_regex, true);
}

static bool
tag_editor_tag_matches_regex(NativeTagEditorScreen *screen,
                             NcmMutableSong *song, NcmRegex *regex) {
    NcmBuffer buffer;
    bool found;

    if ((screen == NULL) || (song == NULL) || (regex == NULL)) {
        return false;
    }

    ncm_buffer_init(&buffer);
    if (!tag_editor_tag_search_text(screen, song, &buffer)) {
        ncm_buffer_destroy(&buffer);
        return false;
    }
    found = ncm_regex_search(regex, buffer.data, buffer.len);
    ncm_buffer_destroy(&buffer);
    return found;
}

static bool
tag_editor_tag_search_text(NativeTagEditorScreen *screen,
                           NcmMutableSong *song, NcmBuffer *buffer) {
    enum NcmTagsField field;

    if ((screen == NULL) || (song == NULL) || (buffer == NULL)) {
        return false;
    }
    if (!tag_editor_tag_search_field(screen, &field)) {
        return false;
    }
    if (!native_tag_editor_song_display_value(song, field, buffer)) {
        return false;
    }
    if (buffer->len <= 0) {
        ncm_buffer_append(buffer, Config.empty_tag, Config.empty_tag_len);
    }
    return true;
}

static bool
tag_editor_tag_search_field(NativeTagEditorScreen *screen,
                            enum NcmTagsField *field) {
    NcMenu *tag_types;
    int64 choice;

    if ((screen == NULL) || (field == NULL)) {
        return false;
    }
    tag_types = nc_editor_string_menu_base(&screen->tag_types);
    choice = nc_menu_highlight(tag_types);
    if ((choice >= 0) && (choice < 11)) {
        *field = ncm_song_info_tags[choice].field;
        return true;
    }
    if (choice == 12) {
        *field = NCM_TAGS_FIELD_LAST;
        return true;
    }
    return false;
}

static bool
tag_editor_tag_type_choice_is_editable(int64 choice) {
    return ((choice >= 0) && (choice < 11)) || (choice == 12);
}

static enum TagEditorTagTypeAction
tag_editor_current_tag_type_action(NativeTagEditorScreen *screen,
                                   enum NcmTagsField *field) {
    NcMenu *menu;
    NcMenuString *row;
    int64 choice;

    if (field) {
        *field = NCM_TAGS_FIELD_LAST;
    }
    if (screen == NULL) {
        return TAG_EDITOR_TAG_TYPE_ACTION_NONE;
    }

    menu = nc_editor_string_menu_base(&screen->tag_types);
    choice = nc_menu_highlight(menu);
    row = nc_menu_current_item(menu);
    if ((row == NULL) || !nc_menu_current_is_selectable(menu)) {
        return TAG_EDITOR_TAG_TYPE_ACTION_NONE;
    }

    if ((choice >= 0) && (choice < 11)) {
        if (field) {
            *field = ncm_song_info_tags[choice].field;
        }
        if ((ncm_song_info_tags[choice].field == NCM_TAGS_FIELD_TRACK)
            && (screen->active_focus
                == NATIVE_TAG_EDITOR_FOCUS_TAG_TYPES)) {
            return TAG_EDITOR_TAG_TYPE_ACTION_NUMBER_TRACKS;
        }
        return TAG_EDITOR_TAG_TYPE_ACTION_FIELD;
    }
    if (tag_editor_strings_equal(row->data, row->len,
                                 STRLIT_ARGS("Filename"))) {
        return TAG_EDITOR_TAG_TYPE_ACTION_FILENAME;
    }
    if (tag_editor_strings_equal(row->data, row->len,
                                 STRLIT_ARGS("Capitalize First Letters"))) {
        return TAG_EDITOR_TAG_TYPE_ACTION_CAPITALIZE;
    }
    if (tag_editor_strings_equal(row->data, row->len,
                                 STRLIT_ARGS("lower all letters"))) {
        return TAG_EDITOR_TAG_TYPE_ACTION_LOWER;
    }
    if (tag_editor_strings_equal(row->data, row->len,
                                 STRLIT_ARGS("Reset"))) {
        return TAG_EDITOR_TAG_TYPE_ACTION_RESET;
    }
    if (tag_editor_strings_equal(row->data, row->len,
                                 STRLIT_ARGS("Save"))) {
        return TAG_EDITOR_TAG_TYPE_ACTION_SAVE;
    }
    return TAG_EDITOR_TAG_TYPE_ACTION_NONE;
}

static void
tag_editor_status_message(NativeTagEditorScreen *screen,
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
tag_editor_confirm(NativeTagEditorScreen *screen, char *message,
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
    if (STREQUAL(pair->first, pair->first_len, STRLIT_ARGS("."))) {
        return filter;
    }
    if (STREQUAL(pair->first, pair->first_len, STRLIT_ARGS(".."))) {
        return filter;
    }
    return ncm_regex_search(regex, pair->first, pair->first_len);
}

static bool
tag_editor_active_item_matches(NativeTagEditorScreen *screen,
                               NcMenu *menu, int64 pos, NcmRegex *regex) {
    if (screen->active_focus == NATIVE_TAG_EDITOR_FOCUS_TAGS) {
        return tag_editor_tag_matches_regex(screen,
                                            nc_menu_active_item_at(menu,
                                                                   pos),
                                            regex);
    }
    if (screen->active_focus == NATIVE_TAG_EDITOR_FOCUS_DIRECTORIES) {
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
    nc_menu_string_init(&string);
    ok = nc_menu_string_set(&string, data, data_len);
    if (ok) {
        nc_editor_string_menu_add_with_flags(menu, &string, flags);
    }
    nc_menu_string_destroy(&string);
    return ok;
}

static bool
tag_editor_append_parser_separator(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    nc_editor_string_menu_add_separator(&screen->parser_rows);
    nc_editor_string_menu_add_separator(&screen->parser_actions);
    return true;
}

static bool
tag_editor_append_parser_action_row(NativeTagEditorScreen *screen,
                                    char *data, int32 data_len,
                                    uint32 flags) {
    if (screen == NULL) {
        return false;
    }
    return tag_editor_append_parser_row(&screen->parser_rows, data,
                                        data_len, flags)
           && tag_editor_append_parser_row(&screen->parser_actions, data,
                                           data_len, flags);
}

static bool
tag_editor_append_parser_action_label(NativeTagEditorScreen *screen,
                                      char *label, int32 label_len) {
    return tag_editor_append_parser_action_row(screen, label, label_len,
                                               NC_MENU_ITEM_SELECTABLE);
}

static bool
tag_editor_append_pattern_row(NativeTagEditorScreen *screen) {
    NcmBuffer row;
    bool result;

    ncm_buffer_init(&row);
    ncm_buffer_append(&row, STRLIT_ARGS("Pattern: "));
    ncm_buffer_append(&row, screen->pattern.data, screen->pattern.len);
    result = tag_editor_append_parser_action_label(screen, row.data,
                                                   row.len);
    ncm_buffer_destroy(&row);
    return result;
}

static bool
tag_editor_build_parser_menus(NativeTagEditorScreen *screen) {
    if (!tag_editor_append_pattern_row(screen)) {
        return false;
    }
    if (!tag_editor_append_parser_action_label(screen,
                                               STRLIT_ARGS("Preview"))) {
        return false;
    }
    if (!tag_editor_append_parser_action_label(screen,
                                               STRLIT_ARGS("Legend"))) {
        return false;
    }
    if (!tag_editor_append_parser_separator(screen)) {
        return false;
    }
    if (!tag_editor_append_parser_action_label(screen,
                                               STRLIT_ARGS("Proceed"))) {
        return false;
    }
    if (!tag_editor_append_parser_action_label(screen,
                                               STRLIT_ARGS("Cancel"))) {
        return false;
    }
    if (screen->recent_patterns.len <= 0) {
        return true;
    }
    if (!tag_editor_append_parser_separator(screen)) {
        return false;
    }
    if (!tag_editor_append_parser_action_row(screen,
                                             STRLIT_ARGS("Recent patterns"),
                                             NC_MENU_ITEM_INACTIVE)) {
        return false;
    }
    if (!tag_editor_append_parser_separator(screen)) {
        return false;
    }
    for (int32 i = 0; i < screen->recent_patterns.len; i += 1) {
        NcmBuffer *pattern;

        pattern = &screen->recent_patterns.items[i];
        if (!tag_editor_append_parser_action_label(screen, pattern->data,
                                                   pattern->len)) {
            return false;
        }
    }
    return true;
}

static bool
tag_editor_build_parser_legend(NativeTagEditorScreen *screen) {
    NcMenu *tags;
    int64 count;

    if (screen == NULL) {
        return false;
    }
    ncm_buffer_clear(&screen->parser_legend);
    ncm_buffer_append(&screen->parser_legend,
                      STRLIT_ARGS("%a - artist\n"));
    ncm_buffer_append(&screen->parser_legend,
                      STRLIT_ARGS("%A - album artist\n"));
    ncm_buffer_append(&screen->parser_legend,
                      STRLIT_ARGS("%t - title\n"));
    ncm_buffer_append(&screen->parser_legend,
                      STRLIT_ARGS("%b - album\n"));
    ncm_buffer_append(&screen->parser_legend,
                      STRLIT_ARGS("%y - date\n"));
    ncm_buffer_append(&screen->parser_legend,
                      STRLIT_ARGS("%n - track number\n"));
    ncm_buffer_append(&screen->parser_legend,
                      STRLIT_ARGS("%g - genre\n"));
    ncm_buffer_append(&screen->parser_legend,
                      STRLIT_ARGS("%c - composer\n"));
    ncm_buffer_append(&screen->parser_legend,
                      STRLIT_ARGS("%p - performer\n"));
    ncm_buffer_append(&screen->parser_legend,
                      STRLIT_ARGS("%d - disc\n"));
    ncm_buffer_append(&screen->parser_legend,
                      STRLIT_ARGS("%C - comment\n\nFiles:\n"));

    tags = nc_tag_row_menu_base(&screen->tags);
    count = nc_menu_item_count(tags);
    for (int64 i = 0; i < count; i += 1) {
        NcmMutableSong *song;

        song = nc_menu_active_item_at(tags, i);
        if ((song == NULL) || (song->name == NULL)) {
            continue;
        }
        ncm_buffer_append(&screen->parser_legend, STRLIT_ARGS(" * "));
        ncm_buffer_append(&screen->parser_legend, song->name,
                          song->name_len);
        ncm_buffer_append_byte(&screen->parser_legend, '\n');
    }
    return true;
}

static bool
tag_editor_build_parser_preview(NativeTagEditorScreen *screen,
                                bool apply, bool *success) {
    NcMenu *tags;
    int64 count;

    if (success) {
        *success = true;
    }
    if (screen == NULL) {
        return false;
    }
    tag_editor_status_message(screen, STRLIT_ARGS("Parsing..."));
    ncm_buffer_clear(&screen->parser_preview);
    tags = nc_tag_row_menu_base(&screen->tags);
    count = nc_menu_item_count(tags);
    for (int64 i = 0; i < count; i += 1) {
        NcmMutableSong *song;

        song = nc_menu_active_item_at(tags, i);
        if (song == NULL) {
            continue;
        }
        if (screen->parser_mode
            == NATIVE_TAG_EDITOR_PARSER_TAGS_FROM_FILENAME) {
            bool parsed;

            if (!apply && song->name) {
                ncm_buffer_append(&screen->parser_preview, song->name,
                                  song->name_len);
                ncm_buffer_append(&screen->parser_preview,
                                  STRLIT_ARGS(":\n"));
            }
            parsed = native_tag_editor_parse_filename(
                song, screen->pattern.data, screen->pattern.len, !apply,
                &screen->parser_preview);
            if (!parsed && !apply) {
                ncm_buffer_append(&screen->parser_preview,
                                  STRLIT_ARGS(
                                      "Error while parsing filename!\n"));
            }
            if (!apply) {
                ncm_buffer_append_byte(&screen->parser_preview, '\n');
            }
        } else if (screen->parser_mode
                   == NATIVE_TAG_EDITOR_PARSER_RENAME_FILES) {
            NcmBuffer stem;
            NcmBuffer new_name;
            int32 extension_start;

            ncm_buffer_init(&stem);
            ncm_buffer_init(&new_name);
            if (!native_tag_editor_generate_filename(
                    song, screen->pattern.data, screen->pattern.len,
                    &stem)) {
                ncm_buffer_destroy(&new_name);
                ncm_buffer_destroy(&stem);
                return false;
            }
            extension_start = tag_editor_filename_extension_start(
                song->name, song->name_len);
            ncm_buffer_append(&new_name, stem.data, stem.len);
            if ((extension_start >= 0) && song->name) {
                ncm_buffer_append(&new_name, song->name + extension_start,
                                  song->name_len - extension_start);
            }
            if (apply && (stem.len <= 0)) {
                ncm_buffer_clear(&screen->parser_preview);
                ncm_buffer_append(&screen->parser_preview,
                                  STRLIT_ARGS("File \""));
                tag_editor_append_parser_filename(
                    &screen->parser_preview, song->name, song->name_len);
                ncm_buffer_append(&screen->parser_preview,
                                  STRLIT_ARGS(
                                      "\" would have an empty name"));
                tag_editor_status_message(
                    screen, screen->parser_preview.data,
                    screen->parser_preview.len);
                screen->parser_preview_enabled = true;
                if (success) {
                    *success = false;
                }
                ncm_buffer_destroy(&new_name);
                ncm_buffer_destroy(&stem);
                return true;
            }
            if (apply) {
                if (!ncm_mutable_song_set_new_name(song, new_name.data,
                                                   new_name.len)) {
                    ncm_buffer_destroy(&new_name);
                    ncm_buffer_destroy(&stem);
                    return false;
                }
            } else {
                tag_editor_append_parser_filename(
                    &screen->parser_preview, song->name, song->name_len);
                ncm_buffer_append(&screen->parser_preview,
                                  STRLIT_ARGS(" -> "));
                if (new_name.len > 0) {
                    ncm_buffer_append(&screen->parser_preview,
                                      new_name.data, new_name.len);
                } else if (Config.empty_tag) {
                    ncm_buffer_append(&screen->parser_preview,
                                      Config.empty_tag,
                                      Config.empty_tag_len);
                }
                ncm_buffer_append(&screen->parser_preview,
                                  STRLIT_ARGS("\n\n"));
            }
            ncm_buffer_destroy(&new_name);
            ncm_buffer_destroy(&stem);
        }
    }
    if (!apply) {
        screen->parser_preview_enabled = true;
    }
    return true;
}

static bool
tag_editor_load_recent_patterns(NativeTagEditorScreen *screen) {
    NcmBuffer path;
    NcmBuffer line;
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
    ncm_buffer_init(&path);
    ncm_buffer_init(&line);
    if (!tag_editor_history_path(&path)) {
        ncm_buffer_destroy(&line);
        ncm_buffer_destroy(&path);
        return false;
    }
    file = fopen(path.data, "r");
    if (file == NULL) {
        ncm_buffer_destroy(&line);
        ncm_buffer_destroy(&path);
        return true;
    }
    ok = true;
    read_line = false;
    while (ok) {
        ok = tag_editor_read_pattern_line(file, &line, &read_line);
        if (!ok || !read_line) {
            break;
        }
        if (line.len > 0) {
            ok = tag_editor_add_recent_pattern(screen, line.data,
                                               line.len);
        }
    }
    fclose(file);
    ncm_buffer_destroy(&line);
    ncm_buffer_destroy(&path);
    return ok;
}

static bool
tag_editor_save_recent_patterns(NativeTagEditorScreen *screen) {
    NcmBuffer path;
    FILE *file;
    int32 limit;

    if (screen == NULL) {
        return false;
    }
    ncm_buffer_init(&path);
    if (!tag_editor_history_path(&path)) {
        ncm_buffer_destroy(&path);
        return false;
    }
    file = fopen(path.data, "w");
    if (file == NULL) {
        ncm_buffer_destroy(&path);
        return false;
    }
    limit = screen->recent_patterns.len;
    if (limit > TAG_EDITOR_PATTERN_HISTORY_MAX) {
        limit = TAG_EDITOR_PATTERN_HISTORY_MAX;
    }
    for (int32 i = 0; i < limit; i += 1) {
        NcmBuffer *pattern;

        pattern = &screen->recent_patterns.items[i];
        if ((pattern->len > 0)
            && (fwrite64(pattern->data, 1, pattern->len, file)
                != pattern->len)) {
            fclose(file);
            ncm_buffer_destroy(&path);
            return false;
        }
        if (fputc('\n', file) == EOF) {
            fclose(file);
            ncm_buffer_destroy(&path);
            return false;
        }
    }
    fclose(file);
    ncm_buffer_destroy(&path);
    return true;
}

static bool
tag_editor_history_path(NcmBuffer *path) {
    if (path == NULL) {
        return false;
    }
    if (Config.ncmpcpp_directory
        && (Config.ncmpcpp_directory_len > 0)) {
        return ncm_fs_join(path, Config.ncmpcpp_directory,
                           Config.ncmpcpp_directory_len,
                           STRLIT_ARGS("patterns.list"));
    }
    return tag_editor_set_buffer(path, STRLIT_ARGS("patterns.list"));
}

static bool
tag_editor_read_pattern_line(FILE *file, NcmBuffer *line,
                             bool *read_line) {
    int32 ch;

    if ((file == NULL) || (line == NULL) || (read_line == NULL)) {
        return false;
    }
    ncm_buffer_clear(line);
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
        ncm_buffer_append_byte(line, (char)ch);
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
tag_editor_find_recent_pattern(NativeTagEditorScreen *screen,
                               char *pattern, int32 pattern_len) {
    if ((screen == NULL) || (pattern == NULL) || (pattern_len <= 0)) {
        return -1;
    }
    for (int32 i = 0; i < screen->recent_patterns.len; i += 1) {
        NcmBuffer *item;

        item = &screen->recent_patterns.items[i];
        if (STREQUAL(item->data, item->len, pattern,
                             pattern_len)) {
            return i;
        }
    }
    return -1;
}

static bool
tag_editor_add_recent_pattern(NativeTagEditorScreen *screen,
                              char *pattern, int32 pattern_len) {
    NcmBuffer *item;

    if ((screen == NULL) || (pattern == NULL) || (pattern_len <= 0)) {
        return false;
    }
    if (tag_editor_find_recent_pattern(screen, pattern, pattern_len) >= 0) {
        return true;
    }
    item = ncm_buffer_array_append(&screen->recent_patterns);
    if (item == NULL) {
        return false;
    }
    return tag_editor_set_buffer(item, pattern, pattern_len);
}

static bool
tag_editor_move_pattern_to_front(NativeTagEditorScreen *screen,
                                 char *pattern, int32 pattern_len) {
    NcmBufferArray replacement;
    NcmBuffer first;
    int32 existing;
    bool ok;

    if ((screen == NULL) || (pattern == NULL) || (pattern_len <= 0)) {
        return false;
    }
    ncm_buffer_array_init(&replacement);
    ncm_buffer_init(&first);
    ok = tag_editor_set_buffer(&first, pattern, pattern_len)
         && ncm_buffer_array_append_copy(&replacement, &first);
    ncm_buffer_destroy(&first);
    if (!ok) {
        ncm_buffer_array_destroy(&replacement);
        return false;
    }
    existing = tag_editor_find_recent_pattern(screen, pattern, pattern_len);
    for (int32 i = 0; i < screen->recent_patterns.len; i += 1) {
        if (i == existing) {
            continue;
        }
        if (!ncm_buffer_array_append_copy(&replacement,
                                          &screen->recent_patterns.items[i])) {
            ncm_buffer_array_destroy(&replacement);
            return false;
        }
    }
    ncm_buffer_array_move(&screen->recent_patterns, &replacement);
    ncm_buffer_array_destroy(&replacement);
    return native_tag_editor_screen_prepare_parser_rows(
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
tag_editor_set_pattern(NativeTagEditorScreen *screen,
                       char *pattern, int32 pattern_len) {
    if (screen == NULL) {
        return false;
    }
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
tag_editor_prompt_pattern(NativeTagEditorScreen *screen) {
    NcmBuffer input;
    NcmStringView initial;
    enum NativeTagEditorPromptResult prompt_result;
    bool result;

    if ((screen == NULL) || (screen->hooks.prompt == NULL)) {
        return false;
    }
    ncm_buffer_init(&input);
    initial.data = screen->pattern.data;
    initial.len = screen->pattern.len;
    prompt_result = screen->hooks.prompt(screen->hooks.user,
                                         STRLIT_ARGS("Pattern"),
                                         initial, &input);
    if (prompt_result == NATIVE_TAG_EDITOR_PROMPT_ABORTED) {
        tag_editor_status_message(screen, STRLIT_ARGS("Action aborted"));
        ncm_buffer_destroy(&input);
        return false;
    }
    if (prompt_result == NATIVE_TAG_EDITOR_PROMPT_ERROR) {
        ncm_buffer_destroy(&input);
        return false;
    }
    result = tag_editor_set_pattern(screen, input.data, input.len)
             && native_tag_editor_screen_prepare_parser_rows(
                 screen, screen->parser_mode, screen->pattern.data,
                 screen->pattern.len);
    ncm_buffer_destroy(&input);
    if (result) {
        tag_editor_set_focus(screen, NATIVE_TAG_EDITOR_FOCUS_PARSER_ACTIONS);
        (void)nc_menu_goto_selectable(
            nc_editor_string_menu_base(&screen->parser_actions),
            TAG_EDITOR_PARSER_ACTION_PATTERN);
    }
    return result;
}

static bool
tag_editor_apply_recent_pattern(NativeTagEditorScreen *screen,
                                int64 choice) {
    NcMenu *menu;
    NcMenuString *row;

    if (screen == NULL) {
        return false;
    }
    menu = nc_editor_string_menu_base(&screen->parser_actions);
    row = nc_menu_active_item_at(menu, choice);
    if (row == NULL) {
        return false;
    }
    if (!tag_editor_set_pattern(screen, row->data, row->len)) {
        return false;
    }
    if (!native_tag_editor_screen_prepare_parser_rows(
            screen, screen->parser_mode, screen->pattern.data,
            screen->pattern.len)) {
        return false;
    }
    tag_editor_set_focus(screen, NATIVE_TAG_EDITOR_FOCUS_PARSER_ACTIONS);
    return nc_menu_goto_selectable(
        nc_editor_string_menu_base(&screen->parser_actions),
        TAG_EDITOR_PARSER_ACTION_PATTERN);
}

static bool
tag_editor_mutable_song_to_format_song(NcmMutableSong *source,
                                       NcmSong *dest) {
    NcmBuffer uri;
    bool ok;

    if ((source == NULL) || (dest == NULL)) {
        return false;
    }

    ncm_buffer_init(&uri);
    if (source->uri && (source->uri_len >= 0)) {
        ncm_buffer_append(&uri, source->uri, source->uri_len);
    } else if (source->directory && (source->directory_len > 0)
               && source->name && (source->name_len >= 0)) {
        if (!ncm_fs_join(&uri, source->directory, source->directory_len,
                         source->name, source->name_len)) {
            ncm_buffer_destroy(&uri);
            return false;
        }
    } else if (source->name && (source->name_len >= 0)) {
        ncm_buffer_append(&uri, source->name, source->name_len);
    }
    if (uri.data == NULL) {
        ok = ncm_song_set_uri(dest, STRLIT_ARGS(""));
    } else {
        ok = ncm_song_set_uri(dest, uri.data, uri.len);
    }
    ncm_buffer_destroy(&uri);
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
tag_editor_append_parser_filename(NcmBuffer *buffer, char *name,
                                  int32 name_len) {
    if ((name == NULL) || (name_len <= 0)) {
        return;
    }
    ncm_buffer_append(buffer, name, name_len);
    return;
}

static bool
tag_editor_mutable_song_get_field(NcmMutableSong *song,
                                  enum NcmTagsField field,
                                  NcmBuffer *buffer) {
    NcmBuffer tag;

    tag = ncm_mutable_song_get_tag_buffer(song, field, 0);
    ncm_buffer_append(buffer, tag.data, tag.len);
    ncm_buffer_destroy(&tag);
    return true;
}

static void
tag_editor_lower_ascii_buffer(NcmBuffer *buffer) {
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
