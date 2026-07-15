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
#include "c/ncm_mpd_client.h"
#include "c/ncm_string.h"
#include "c/ncm_type_conversions.h"
#include "cbase/base_macros.h"
#include "screens/song_info.h"

static NativeTagEditorScreen *tag_editor_from_screen(NcScreen *screen);
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
static bool tag_editor_current_directory_path(NativeTagEditorScreen *screen,
                                              char **path,
                                              int32 *path_len);
static void tag_editor_preserve_current_directory(
    NativeTagEditorScreen *screen, NcmBuffer *path);
static void tag_editor_restore_current_directory(
    NativeTagEditorScreen *screen, NcmBuffer *path);
static void tag_editor_preserve_current_song(NativeTagEditorScreen *screen,
                                             NcmBuffer *uri);
static void tag_editor_restore_current_song(NativeTagEditorScreen *screen,
                                            NcmBuffer *uri);
static bool tag_editor_add_control_directory(NativeTagEditorScreen *screen);
static void tag_editor_sort_directories(NcmDirectoryArray *directories);
static int32 tag_editor_compare_directories(NcmDirectory *left,
                                            NcmDirectory *right);
static void tag_editor_sort_songs(NcmSongArray *songs);
static int32 tag_editor_compare_songs(NcmSong *left, NcmSong *right);
static void tag_editor_report_error(char *context, int32 context_len,
                                    NcmError *error);
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
static bool tag_editor_tag_type_choice_is_actionable(int64 choice);
static bool tag_editor_mutable_song_to_song(NcmMutableSong *source,
                                            NcmSong *dest);
static bool tag_editor_directory_matches_regex(NcMenuStringPair *pair,
                                               NcmRegex *regex,
                                               bool filter);
static bool tag_editor_active_item_matches(NativeTagEditorScreen *screen,
                                           NcMenu *menu, int64 pos,
                                           NcmRegex *regex);
static bool tag_editor_append_parser_row(NativeTagEditorScreen *screen,
                                         char *data, int32 data_len,
                                         uint32 flags);
static bool tag_editor_mutable_song_get_field(NcmMutableSong *song,
                                              enum NcmTagsField field,
                                              NcmBuffer *buffer);
static bool tag_editor_append_generated_field(NcmMutableSong *song,
                                              char marker,
                                              NcmBuffer *filename);
static void tag_editor_lower_ascii_buffer(NcmBuffer *buffer);
static bool tag_editor_next_mask_tag(char *mask, int32 mask_len,
                                     int32 start, int32 *percent_pos,
                                     char *tag_char);

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

typedef struct SaveContext {
    char *music_dir;
    bool ok;
} SaveContext;

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
    screen->bridge = (NativeTagEditorBridge){0};
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
                   screen->parser_dialog_title.len, color, border);
    nc_window_init(&screen->parser_window, start_x, main_start_y,
                   width, main_height, screen->parser_title.data,
                   screen->parser_title.len, color, border);
    nc_window_init(&screen->parser_helper_window, start_x, main_start_y,
                   width, main_height, screen->parser_helper_title.data,
                   screen->parser_helper_title.len, color, border);

    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    screen->active_column = NATIVE_TAG_EDITOR_COLUMN_DIRECTORIES;
    screen->last_directory_highlight = -1;
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
native_tag_editor_screen_set_bridge(NativeTagEditorScreen *screen,
                                    NativeTagEditorBridge bridge) {
    if (screen == NULL) {
        return;
    }
    screen->bridge = bridge;
    return;
}

NcEditorPairMenu *
native_tag_editor_screen_directories(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->directories;
}

NcEditorStringMenu *
native_tag_editor_screen_tag_types(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->tag_types;
}

NcTagRowMenu *
native_tag_editor_screen_tags(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->tags;
}

NcEditorStringMenu *
native_tag_editor_screen_parser_rows(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->parser_rows;
}

NcMenu *
native_tag_editor_screen_active_menu(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    if (screen->active_column == NATIVE_TAG_EDITOR_COLUMN_DIRECTORIES) {
        return nc_editor_pair_menu_base(&screen->directories);
    }
    if (screen->active_column == NATIVE_TAG_EDITOR_COLUMN_TAG_TYPES) {
        return nc_editor_string_menu_base(&screen->tag_types);
    }
    return nc_tag_row_menu_base(&screen->tags);
}

NcWindow *
native_tag_editor_screen_active_window(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    if (screen->active_column == NATIVE_TAG_EDITOR_COLUMN_DIRECTORIES) {
        return &screen->directories_window;
    }
    if (screen->active_column == NATIVE_TAG_EDITOR_COLUMN_TAG_TYPES) {
        return &screen->tag_types_window;
    }
    return &screen->tags_window;
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
native_tag_editor_screen_clear(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
    native_tag_editor_screen_clear_stale_tags(screen);
    screen->directories_update_requested = true;
    screen->last_known_directory_count = 0;
    tag_editor_update_titles(screen, true);
    return;
}

void
native_tag_editor_screen_clear_directories(
    NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    if (screen->bridge.clear_directories != NULL) {
        screen->bridge.clear_directories(screen->bridge.user);
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
    if (screen->active_column != NATIVE_TAG_EDITOR_COLUMN_DIRECTORIES) {
        return;
    }
    if (tag_editor_directory_row_changed(screen)) {
        native_tag_editor_screen_clear_stale_tags(screen);
    }
    return;
}

bool
native_tag_editor_screen_displayed_dir(NativeTagEditorScreen *screen,
                                       NcmStringView *view) {
    ncm_string_view_init(view);
    if ((screen == NULL) || !screen->displayed_dir_valid) {
        return false;
    }
    ncm_string_view_set(view, screen->displayed_dir.data,
                        screen->displayed_dir.len);
    return true;
}

bool
native_tag_editor_screen_observed_dir(NativeTagEditorScreen *screen,
                                      NcmStringView *view) {
    ncm_string_view_init(view);
    if ((screen == NULL) || !screen->observed_dir_valid) {
        return false;
    }
    ncm_string_view_set(view, screen->observed_dir.data,
                        screen->observed_dir.len);
    return true;
}

bool
native_tag_editor_screen_set_current_dir(NativeTagEditorScreen *screen,
                                         char *dir, int32 dir_len) {
    bool changed;

    if (screen == NULL) {
        return false;
    }
    changed = screen->current_dir.data != NULL
              && !ncm_string_equal(screen->current_dir.data,
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
    return screen->current_dir.data != NULL;
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
native_tag_editor_screen_current_tag_field(
    NativeTagEditorScreen *screen, enum NcmTagsField *field) {
    return tag_editor_tag_search_field(screen, field);
}

bool
native_tag_editor_screen_current_tag_song(
    NativeTagEditorScreen *screen, NcmMutableSong *song) {
    return native_tag_editor_screen_current_song(screen, song);
}

int64
native_tag_editor_screen_selected_tag_song_count(
    NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return 0;
    }
    return nc_menu_selected_count(nc_tag_row_menu_base(&screen->tags));
}

bool
native_tag_editor_screen_active_menu_empty(
    NativeTagEditorScreen *screen) {
    NcMenu *menu;

    if (screen == NULL) {
        return true;
    }
    menu = native_tag_editor_screen_active_menu(screen);
    return (menu == NULL) || nc_menu_empty(menu);
}

bool
native_tag_editor_screen_current_tag_type_editable(
    NativeTagEditorScreen *screen) {
    NcMenu *menu;

    if (screen == NULL) {
        return false;
    }
    menu = nc_editor_string_menu_base(&screen->tag_types);
    return tag_editor_tag_type_choice_is_editable(nc_menu_highlight(menu));
}

bool
native_tag_editor_screen_current_tag_type_actionable(
    NativeTagEditorScreen *screen) {
    NcMenu *menu;
    int64 choice;

    if (screen == NULL) {
        return false;
    }
    menu = nc_editor_string_menu_base(&screen->tag_types);
    choice = nc_menu_highlight(menu);
    if (!tag_editor_tag_type_choice_is_actionable(choice)) {
        return false;
    }
    return nc_menu_position_is_selectable(menu, choice);
}

bool
native_tag_editor_screen_enter_directory(NativeTagEditorScreen *screen) {
    NcmStringView path;

    ncm_string_view_init(&path);
    if (screen == NULL) {
        return false;
    }
    if (screen->active_column != NATIVE_TAG_EDITOR_COLUMN_DIRECTORIES) {
        return false;
    }
    if (!native_tag_editor_screen_current_directory_path(screen, &path)) {
        return false;
    }
    if (!tag_editor_set_buffer(&screen->highlighted_dir,
                               screen->current_dir.data,
                               screen->current_dir.len)) {
        return false;
    }
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
native_tag_editor_screen_load_directories(NativeTagEditorScreen *screen,
                                          NcmDirectoryArray *directories) {
    NcmBuffer preserved;

    if (screen == NULL || directories == NULL) {
        return false;
    }
    ncm_buffer_init(&preserved);
    tag_editor_preserve_current_directory(screen, &preserved);
    nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
    for (int32 i = 0; i < directories->len; i += 1) {
        NcmDirectory *directory;
        NcmStringView path;
        int32 basename_start;

        directory = &directories->items[i];
        if (!ncm_directory_path_view(directory, &path)) {
            continue;
        }
        basename_start = ncm_string_basename_start(path.data, path.len);
        if (!native_tag_editor_screen_add_directory(
                screen, path.data + basename_start,
                path.len - basename_start, path.data, path.len)) {
            ncm_buffer_destroy(&preserved);
            return false;
        }
    }
    tag_editor_restore_current_directory(screen, &preserved);
    tag_editor_observe_current_directory(screen);
    screen->directories_update_requested = false;
    screen->last_known_directory_count = nc_menu_item_count(
        nc_editor_pair_menu_base(&screen->directories));
    tag_editor_update_titles(screen, true);
    ncm_buffer_destroy(&preserved);
    return true;
}

bool
native_tag_editor_screen_load_songs(NativeTagEditorScreen *screen,
                                    NcmSongArray *songs) {
    char *path;
    int32 path_len;

    if (screen == NULL || songs == NULL) {
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
    if (screen == NULL || song == NULL) {
        return false;
    }
    nc_tag_row_menu_add(&screen->tags, song);
    screen->last_known_tag_count = nc_menu_item_count(
        nc_tag_row_menu_base(&screen->tags));
    tag_editor_update_titles(screen, true);
    return true;
}

bool
native_tag_editor_screen_current_song(NativeTagEditorScreen *screen,
                                      NcmMutableSong *song) {
    NcmMutableSong *current;

    if (screen == NULL || song == NULL) {
        return false;
    }
    current = nc_tag_row_menu_current(&screen->tags);
    if (current == NULL) {
        return false;
    }
    return ncm_mutable_song_copy(song, current);
}

bool
native_tag_editor_screen_selected_songs(NativeTagEditorScreen *screen,
                                        NcmSongArray *songs) {
    NcMenu *menu;
    bool has_selected;

    if (screen == NULL || songs == NULL) {
        return false;
    }
    ncm_song_array_clear(songs);
    if (screen->active_column != NATIVE_TAG_EDITOR_COLUMN_TAGS) {
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
    if (screen->active_column == NATIVE_TAG_EDITOR_COLUMN_TAGS) {
        return !nc_menu_empty(nc_editor_string_menu_base(&screen->tag_types))
               && !nc_menu_empty(nc_editor_pair_menu_base(
                       &screen->directories));
    }
    if (screen->active_column == NATIVE_TAG_EDITOR_COLUMN_TAG_TYPES) {
        if (nc_menu_empty(nc_editor_pair_menu_base(&screen->directories))) {
            return false;
        }
        return true;
    }
    return false;
}

bool
native_tag_editor_screen_next_column_available(NativeTagEditorScreen *screen) {
    NcMenu *tag_types;

    if (screen == NULL) {
        return false;
    }
    if (screen->active_column == NATIVE_TAG_EDITOR_COLUMN_DIRECTORIES) {
        return !nc_menu_empty(nc_editor_string_menu_base(&screen->tag_types))
               && !nc_menu_empty(nc_tag_row_menu_base(&screen->tags));
    }
    if (screen->active_column != NATIVE_TAG_EDITOR_COLUMN_TAG_TYPES) {
        return false;
    }
    tag_types = nc_editor_string_menu_base(&screen->tag_types);
    return !nc_menu_empty(nc_tag_row_menu_base(&screen->tags))
           && tag_editor_tag_type_choice_is_editable(
               nc_menu_highlight(tag_types));
}

void
native_tag_editor_screen_previous_column(NativeTagEditorScreen *screen) {
    if (!native_tag_editor_screen_previous_column_available(screen)) {
        return;
    }
    screen->active_column -= 1;
    tag_editor_update_menu_highlights(screen);
    return;
}

void
native_tag_editor_screen_next_column(NativeTagEditorScreen *screen) {
    if (!native_tag_editor_screen_next_column_available(screen)) {
        return;
    }
    screen->active_column += 1;
    tag_editor_update_menu_highlights(screen);
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
    numberer.total = (int32)nc_menu_item_count(menu);
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
        if (song != NULL) {
            ncm_mutable_song_clear_modifications(song);
        }
    }
    return;
}

bool
native_tag_editor_screen_save_modified(NativeTagEditorScreen *screen,
                                       char *music_dir) {
    SaveContext context;

    if (screen == NULL) {
        return false;
    }
    context.music_dir = music_dir;
    context.ok = true;
    if (!tag_editor_for_each_target(screen, tag_editor_save_song_callback,
                                    &context)) {
        return false;
    }
    return context.ok;
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

void
native_tag_editor_screen_clear_filters(NativeTagEditorScreen *screen) {
    NcmBuffer directory;
    NcmBuffer song;

    if (screen == NULL) {
        return;
    }

    ncm_buffer_init(&directory);
    ncm_buffer_init(&song);
    tag_editor_preserve_current_directory(screen, &directory);
    tag_editor_preserve_current_song(screen, &song);

    screen->directory_filter_enabled = false;
    screen->tag_filter_enabled = false;
    ncm_buffer_clear(&screen->directory_filter_constraint);
    ncm_buffer_clear(&screen->tag_filter_constraint);
    nc_menu_show_all_items(nc_editor_pair_menu_base(&screen->directories));
    nc_menu_show_all_items(nc_tag_row_menu_base(&screen->tags));
    tag_editor_restore_current_directory(screen, &directory);
    tag_editor_restore_current_song(screen, &song);
    ncm_buffer_destroy(&song);
    ncm_buffer_destroy(&directory);
    tag_editor_update_titles(screen, true);
    return;
}

bool
native_tag_editor_screen_search(NativeTagEditorScreen *screen,
                                char *pattern, int32 pattern_len,
                                bool forward, bool wrap,
                                bool skip_current, NcmError *error) {
    NcmRegex *regex;
    NcmBuffer *constraint;
    bool *enabled;
    NcMenu *menu;
    int64 count;
    int64 start;

    if (screen == NULL || pattern == NULL || pattern_len <= 0) {
        return false;
    }
    if (screen->active_column == NATIVE_TAG_EDITOR_COLUMN_TAG_TYPES) {
        return false;
    }

    if (screen->active_column == NATIVE_TAG_EDITOR_COLUMN_TAGS) {
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
    count = nc_menu_item_count(menu);
    start = nc_menu_highlight(menu);
    if (skip_current) {
        if (forward) {
            start += 1;
        } else {
            start -= 1;
        }
    }

    for (int64 i = 0; i < count; i += 1) {
        int64 pos;

        if (forward) {
            pos = start + i;
        } else {
            pos = start - i;
        }
        if (wrap) {
            while (pos < 0) {
                pos += count;
            }
            while (pos >= count) {
                pos -= count;
            }
        }
        if (pos < 0 || pos >= count) {
            continue;
        }
        if (tag_editor_active_item_matches(screen, menu, pos, regex)) {
            return nc_menu_goto_selectable(menu, pos);
        }
    }

    return false;
}

bool
native_tag_editor_screen_prepare_parser_rows(
    NativeTagEditorScreen *screen, enum NativeTagEditorParserMode mode,
    char *pattern, int32 pattern_len) {
    if (screen == NULL) {
        return false;
    }
    screen->parser_mode = mode;
    if (pattern != NULL) {
        if (!ncm_buffer_set(&screen->pattern, pattern, pattern_len)) {
            return false;
        }
    }
    nc_menu_clear_items(nc_editor_string_menu_base(&screen->parser_dialog));
    nc_menu_clear_items(nc_editor_string_menu_base(&screen->parser_rows));
    if (!tag_editor_append_parser_row(screen,
                                      STRLIT_ARGS("Get tags from filename"),
                                      NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    if (!tag_editor_append_parser_row(screen, STRLIT_ARGS("Rename files"),
                                      NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    if (!tag_editor_append_parser_row(screen, STRLIT_ARGS("Cancel"),
                                      NC_MENU_ITEM_SELECTABLE)) {
        return false;
    }
    if (mode == NATIVE_TAG_EDITOR_PARSER_NONE) {
        return true;
    }
    return tag_editor_append_parser_row(screen, STRLIT_ARGS("Pattern"),
                                        NC_MENU_ITEM_SELECTABLE)
           && tag_editor_append_parser_row(screen, STRLIT_ARGS("Preview"),
                                           NC_MENU_ITEM_SELECTABLE)
           && tag_editor_append_parser_row(screen, STRLIT_ARGS("Legend"),
                                           NC_MENU_ITEM_SELECTABLE)
           && tag_editor_append_parser_row(screen, STRLIT_ARGS("Proceed"),
                                           NC_MENU_ITEM_SELECTABLE)
           && tag_editor_append_parser_row(screen, STRLIT_ARGS("Cancel"),
                                           NC_MENU_ITEM_SELECTABLE);
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

    if (song == NULL || mask == NULL || mask_len < 0) {
        return false;
    }
    ncm_buffer_init(&file);
    if (song->name == NULL) {
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
                || !ncm_string_equal(file.data + file_pos, separator_len,
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
            if (literal_len == 0) {
                found = file_pos;
            } else {
                for (int32 i = file_pos; i + literal_len <= file.len;
                     i += 1) {
                    if (ncm_string_equal(file.data + i, literal_len,
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
            if (preview && preview_buffer != NULL) {
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
    if (song == NULL || pattern == NULL || filename == NULL) {
        return false;
    }
    for (int32 i = 0; i < pattern_len; i += 1) {
        if ((pattern[i] == '%') && (i + 1 < pattern_len)) {
            if (!tag_editor_append_generated_field(song, pattern[i + 1],
                                                   filename)) {
                return false;
            }
            i += 1;
        } else {
            ncm_buffer_append_byte(filename, pattern[i]);
        }
    }
    ncm_string_remove_invalid_filename_chars(filename->data, &filename->len,
                                            true);
    if (filename->data != NULL) {
        filename->data[filename->len] = '\0';
    }
    return true;
}

bool
native_tag_editor_song_display_value(NcmMutableSong *song,
                                     enum NcmTagsField field,
                                     NcmBuffer *buffer) {
    if (song == NULL || buffer == NULL) {
        return false;
    }
    if (field == NCM_TAGS_FIELD_LAST) {
        ncm_buffer_append(buffer, song->name, song->name_len);
        if ((song->new_name != NULL) && (song->new_name_len > 0)) {
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
    tag_editor_update_titles(editor, true);
    tag_editor_update_menu_highlights(editor);
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
    tag_editor_update_titles(editor, true);
    tag_editor_update_menu_highlights(editor);
    menu = native_tag_editor_screen_active_menu(editor);
    window = native_tag_editor_screen_active_window(editor);
    tag_editor_refresh_menu(window, menu);
    return;
}

static void
tag_editor_scroll(NcScreen *screen, enum NcScroll where) {
    NativeTagEditorScreen *editor;
    NcMenu *menu;

    editor = tag_editor_from_screen(screen);
    menu = native_tag_editor_screen_active_menu(editor);
    nc_menu_scroll_selectable(menu, nc_window_height(
                                  native_tag_editor_screen_active_window(
                                      editor)), where);
    native_tag_editor_screen_finish_directory_change(editor);
    tag_editor_update_menu_highlights(editor);
    return;
}

static bool
tag_editor_can_run_current(NcScreen *screen) {
    NativeTagEditorScreen *editor;

    editor = tag_editor_from_screen(screen);
    if ((editor == NULL) || (editor->bridge.run_action == NULL)) {
        return false;
    }
    if (editor->bridge.action_runnable == NULL) {
        return true;
    }
    return editor->bridge.action_runnable(editor->bridge.user);
}

static bool
tag_editor_run_current(NcScreen *screen) {
    NativeTagEditorScreen *editor;

    if (!tag_editor_can_run_current(screen)) {
        return false;
    }
    editor = tag_editor_from_screen(screen);
    return editor->bridge.run_action(editor->bridge.user);
}

static void
tag_editor_switch_to(NcScreen *screen) {
    NativeTagEditorScreen *editor;

    editor = tag_editor_from_screen(screen);
    if (editor->bridge.switch_to != NULL) {
        editor->bridge.switch_to(editor->bridge.user);
    }
    return;
}

static void
tag_editor_resize(NcScreen *screen) {
    NativeTagEditorScreen *editor;
    NcScreenResizeParams params;

    editor = tag_editor_from_screen(screen);
    if (editor->bridge.resize != NULL) {
        editor->bridge.resize(editor->bridge.user);
        nc_screen_clear_resize_request(screen);
        return;
    }
    params = nc_screen_resize_params(screen);
    native_tag_editor_screen_set_geometry(editor, params.x_offset,
                                          params.width,
                                          editor->main_start_y,
                                          editor->main_height);
    nc_screen_clear_resize_request(screen);
    return;
}

static char *
tag_editor_title(NcScreen *screen) {
    NativeTagEditorScreen *editor;

    editor = tag_editor_from_screen(screen);
    if (editor->bridge.title != NULL) {
        return editor->bridge.title(editor->bridge.user);
    }
    return (char *)"Tag editor";
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

    editor = tag_editor_from_screen(screen);
    if (editor->bridge.mouse_button_pressed != NULL) {
        editor->bridge.mouse_button_pressed(editor->bridge.user, event);
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
    if (buffer->data != NULL) {
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
        ncm_buffer_append(&screen->parser_title, STRLIT_ARGS("Pattern"));
        ncm_buffer_append(&screen->parser_helper_title,
                          STRLIT_ARGS("Preview"));
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
              || !ncm_string_equal(screen->observed_dir.data,
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

    if (path != NULL) {
        *path = NULL;
    }
    if (path_len != NULL) {
        *path_len = 0;
    }
    if (screen == NULL) {
        return false;
    }
    pair = nc_editor_pair_menu_current(&screen->directories);
    if (pair == NULL || pair->second == NULL) {
        return false;
    }
    if (path != NULL) {
        *path = pair->second;
    }
    if (path_len != NULL) {
        *path_len = pair->second_len;
    }
    return true;
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

    if (screen == NULL || path == NULL || path->len <= 0) {
        return;
    }
    menu = nc_editor_pair_menu_base(&screen->directories);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcMenuStringPair *pair;

        pair = nc_menu_active_item_at(menu, i);
        if (pair == NULL || pair->second == NULL) {
            continue;
        }
        if (ncm_string_equal(pair->second, pair->second_len,
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

    if (screen == NULL || uri == NULL || uri->len <= 0) {
        return;
    }
    menu = nc_tag_row_menu_base(&screen->tags);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmMutableSong *item;

        item = nc_menu_active_item_at(menu, i);
        if ((item == NULL) || (item->uri == NULL)) {
            continue;
        }
        if (ncm_string_equal(item->uri, item->uri_len,
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
        || ncm_string_equal(dir, dir_len, STRLIT_ARGS("/"))) {
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
    if ((error != NULL) && (error->message[0] != 0)) {
        ncm_buffer_append(&message, STRLIT_ARGS(": "));
        ncm_buffer_append(&message, error->message,
                          (int32)strlen(error->message));
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
    dir = screen->current_dir.data;
    if (dir == NULL) {
        dir = (char *)"/";
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
    for (int32 i = 0; ncm_song_info_tags[i].name != NULL; i += 1) {
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

    if (screen == NULL) {
        return;
    }
    directories = nc_editor_pair_menu_base(&screen->directories);
    tag_types = nc_editor_string_menu_base(&screen->tag_types);
    tags = nc_tag_row_menu_base(&screen->tags);
    parser_dialog = nc_editor_string_menu_base(&screen->parser_dialog);
    parser_rows = nc_editor_string_menu_base(&screen->parser_rows);

    tag_editor_configure_menu(directories);
    tag_editor_configure_menu(tag_types);
    tag_editor_configure_menu(tags);
    tag_editor_configure_menu(parser_dialog);
    tag_editor_configure_menu(parser_rows);

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

    tag_editor_update_menu_highlights(screen);
    return;
}

static void
tag_editor_update_menu_highlights(
    NativeTagEditorScreen *screen) {
    NcMenu *directories;
    NcMenu *tag_types;
    NcMenu *tags;
    NcMenu *active;

    if (screen == NULL) {
        return;
    }
    directories = nc_editor_pair_menu_base(&screen->directories);
    tag_types = nc_editor_string_menu_base(&screen->tag_types);
    tags = nc_tag_row_menu_base(&screen->tags);

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

    active = native_tag_editor_screen_active_menu(screen);
    nc_menu_set_highlight_prefix(active, &Config.current_item_prefix);
    nc_menu_set_highlight_suffix(active, &Config.current_item_suffix);
    return;
}

static void
tag_editor_refresh_menu(NcWindow *window, NcMenu *menu) {
    if (window == NULL || menu == NULL) {
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
        if (tag.len == 0) {
            tag_editor_append_empty_tag(buffer);
        } else {
            tag_editor_append_locale(buffer, tag.data, tag.len);
        }
        ncm_buffer_destroy(&tag);
        return;
    }

    if (choice == 12) {
        tag_editor_append_locale(buffer, song->name, song->name_len);
        if ((song->new_name != NULL) && (song->new_name_len > 0)) {
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

    if (screen == NULL || cb == NULL) {
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
        if (song != NULL && !cb(song, user)) {
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
    char buffer[64];
    int32 len;

    numberer = user;
    if (numberer->extended) {
        len = snprintf(buffer, (size_t)SIZEOF(buffer), "%d/%d",
                       numberer->current, numberer->total);
    } else {
        len = snprintf(buffer, (size_t)SIZEOF(buffer), "%d",
                       numberer->current);
    }
    if (len < 0) {
        return false;
    }
    if (len >= (int32)SIZEOF(buffer)) {
        len = (int32)SIZEOF(buffer) - 1;
    }
    numberer->current += 1;
    return ncm_mutable_song_set_tags(song, NCM_TAGS_FIELD_TRACK,
                                     buffer, len, NULL, 0);
}

static bool
tag_editor_capitalize_song_callback(NcmMutableSong *song, void *user) {
    (void)user;
    for (uint32 field = 0; field < NCM_TAGS_FIELD_LAST; field += 1) {
        for (int32 i = 0; ; i += 1) {
            NcmBuffer buffer;

            buffer = ncm_mutable_song_get_tag_buffer(
                song, (enum NcmTagsField)field, i);
            if (buffer.len == 0) {
                ncm_buffer_destroy(&buffer);
                break;
            }
            if (buffer.data[0] >= 'a' && buffer.data[0] <= 'z') {
                buffer.data[0] = (char)(buffer.data[0] - ('a' - 'A'));
            }
            if (!ncm_mutable_song_set_tag(song, (enum NcmTagsField)field,
                                          i, buffer.data, buffer.len)) {
                ncm_buffer_destroy(&buffer);
                return false;
            }
            ncm_buffer_destroy(&buffer);
        }
    }
    return true;
}

static bool
tag_editor_lower_song_callback(NcmMutableSong *song, void *user) {
    (void)user;
    for (uint32 field = 0; field < NCM_TAGS_FIELD_LAST; field += 1) {
        for (int32 i = 0; ; i += 1) {
            NcmBuffer buffer;

            buffer = ncm_mutable_song_get_tag_buffer(
                song, (enum NcmTagsField)field, i);
            if (buffer.len == 0) {
                ncm_buffer_destroy(&buffer);
                break;
            }
            tag_editor_lower_ascii_buffer(&buffer);
            if (!ncm_mutable_song_set_tag(song, (enum NcmTagsField)field,
                                          i, buffer.data, buffer.len)) {
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

    context = user;
    if (!ncm_mutable_song_is_modified(song)) {
        return true;
    }
    if (!ncm_mutable_song_write(song, context->music_dir)) {
        context->ok = false;
        return false;
    }
    ncm_mutable_song_clear_modifications(song);
    return true;
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

    if (screen == NULL || song == NULL || regex == NULL) {
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

    if (screen == NULL || song == NULL || buffer == NULL) {
        return false;
    }
    if (!tag_editor_tag_search_field(screen, &field)) {
        return false;
    }
    if (!native_tag_editor_song_display_value(song, field, buffer)) {
        return false;
    }
    if (buffer->len == 0) {
        ncm_buffer_append(buffer, Config.empty_tag, Config.empty_tag_len);
    }
    return true;
}

static bool
tag_editor_tag_search_field(NativeTagEditorScreen *screen,
                            enum NcmTagsField *field) {
    NcMenu *tag_types;
    int64 choice;

    if (screen == NULL || field == NULL) {
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

static bool
tag_editor_tag_type_choice_is_actionable(int64 choice) {
    if (tag_editor_tag_type_choice_is_editable(choice)) {
        return true;
    }
    return (choice == 16) || (choice == 17) || (choice == 19)
           || (choice == 20);
}

static bool
tag_editor_directory_matches_regex(NcMenuStringPair *pair,
                                   NcmRegex *regex, bool filter) {
    if (pair == NULL || pair->first == NULL) {
        return false;
    }
    if (ncm_string_equal(pair->first, pair->first_len, STRLIT_ARGS("."))) {
        return filter;
    }
    if (ncm_string_equal(pair->first, pair->first_len, STRLIT_ARGS(".."))) {
        return filter;
    }
    return ncm_regex_search(regex, pair->first, pair->first_len);
}

static bool
tag_editor_active_item_matches(NativeTagEditorScreen *screen,
                               NcMenu *menu, int64 pos, NcmRegex *regex) {
    if (screen->active_column == NATIVE_TAG_EDITOR_COLUMN_TAGS) {
        return tag_editor_tag_matches_regex(screen,
                                            nc_menu_active_item_at(menu,
                                                                   pos),
                                            regex);
    }
    if (screen->active_column == NATIVE_TAG_EDITOR_COLUMN_DIRECTORIES) {
        return tag_editor_directory_matches_regex(
            nc_menu_active_item_at(menu, pos), regex, false);
    }
    return false;
}

static bool
tag_editor_append_parser_row(NativeTagEditorScreen *screen, char *data,
                             int32 data_len, uint32 flags) {
    NcMenuString string;
    bool ok;

    nc_menu_string_init(&string);
    ok = nc_menu_string_set(&string, data, data_len);
    if (ok) {
        nc_editor_string_menu_add_with_flags(&screen->parser_rows,
                                             &string, flags);
    }
    nc_menu_string_destroy(&string);
    return ok;
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

static bool
tag_editor_append_generated_field(NcmMutableSong *song, char marker,
                                  NcmBuffer *filename) {
    enum NcmTagsField field;
    NcmBuffer value;

    field = ncm_tags_field_from_char(marker);
    if (field == NCM_TAGS_FIELD_LAST) {
        ncm_buffer_append_byte(filename, '%');
        ncm_buffer_append_byte(filename, marker);
        return true;
    }
    value = ncm_mutable_song_tags_buffer(song, field, STRLIT_ARGS(" "),
                                         false);
    ncm_buffer_append(filename, value.data, value.len);
    ncm_buffer_destroy(&value);
    return true;
}

static void
tag_editor_lower_ascii_buffer(NcmBuffer *buffer) {
    if (buffer == NULL || buffer->data == NULL) {
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
