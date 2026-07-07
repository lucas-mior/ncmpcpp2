#include "screens/nc_tag_editor.h"

#include <stdio.h>

#include "app_controller.h"
#include "c/ncm_base.h"
#include "c/ncm_string.h"
#include "c/ncm_type_conversions.h"
#include "cbase/base_macros.h"
#include "screens/screen_switcher.h"

static NativeTagEditorScreen *tag_editor_from_screen(NcScreen *screen);
static NcWindow *tag_editor_active_window(NcScreen *screen);
static void tag_editor_refresh(NcScreen *screen);
static void tag_editor_refresh_window(NcScreen *screen);
static void tag_editor_scroll(NcScreen *screen, enum NcScroll where);
static void tag_editor_switch_to(NcScreen *screen);
static void tag_editor_resize(NcScreen *screen);
static char *tag_editor_title(NcScreen *screen);
static void tag_editor_update(NcScreen *screen);
static bool tag_editor_is_lockable(NcScreen *screen);
static bool tag_editor_is_mergable(NcScreen *screen);
static void tag_editor_destroy_callback(NcScreen *screen);
static void tag_editor_layout(NativeTagEditorScreen *screen);
static bool tag_editor_directory_filter(NcMenu *menu, void *item,
                                        void *user);
static bool tag_editor_tag_filter(NcMenu *menu, void *item, void *user);
static bool tag_editor_copy_selected_song_at(NativeTagEditorScreen *screen,
                                             NcmMutableSongArray *songs,
                                             int64 pos);
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
    .switch_to = tag_editor_switch_to,
    .resize = tag_editor_resize,
    .title = tag_editor_title,
    .update = tag_editor_update,
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
    nc_window_init(&screen->directories_window, start_x, main_start_y,
                   width, main_height, STRLIT_ARGS("Directories"),
                   color, border);
    nc_window_init(&screen->tag_types_window, start_x, main_start_y,
                   width, main_height, STRLIT_ARGS("Tag types"),
                   color, border);
    nc_window_init(&screen->tags_window, start_x, main_start_y,
                   width, main_height, STRLIT_ARGS("Tags"), color,
                   border);
    nc_window_init(&screen->parser_dialog_window, start_x, main_start_y,
                   width, main_height, NULL, 0, color, border);
    nc_window_init(&screen->parser_window, start_x, main_start_y,
                   width, main_height, STRLIT_ARGS("Pattern"), color,
                   border);
    nc_window_init(&screen->parser_helper_window, start_x, main_start_y,
                   width, main_height, STRLIT_ARGS("Preview"), color,
                   border);
    ncm_buffer_init(&screen->current_dir);
    ncm_buffer_init(&screen->highlighted_dir);
    ncm_buffer_init(&screen->directory_search_constraint);
    ncm_buffer_init(&screen->tag_search_constraint);
    ncm_buffer_init(&screen->pattern);
    ncm_regex_init(&screen->directory_filter_regex);
    ncm_regex_init(&screen->tag_filter_regex);

    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    screen->active_column = NATIVE_TAG_EDITOR_COLUMN_DIRECTORIES;
    screen->parser_mode = NATIVE_TAG_EDITOR_PARSER_NONE;
    screen->directory_filter_enabled = false;
    screen->tag_filter_enabled = false;
    screen->parser_preview_enabled = true;
    screen->registered = false;

    (void)native_tag_editor_screen_set_current_dir(screen,
                                                   STRLIT_ARGS("/"));
    tag_editor_layout(screen);
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
    ncm_regex_destroy(&screen->tag_filter_regex);
    ncm_regex_destroy(&screen->directory_filter_regex);
    ncm_buffer_destroy(&screen->pattern);
    ncm_buffer_destroy(&screen->tag_search_constraint);
    ncm_buffer_destroy(&screen->directory_search_constraint);
    ncm_buffer_destroy(&screen->highlighted_dir);
    ncm_buffer_destroy(&screen->current_dir);
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
    return;
}

void
native_tag_editor_screen_clear(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_clear_items(nc_editor_pair_menu_base(&screen->directories));
    nc_menu_clear_items(nc_tag_row_menu_base(&screen->tags));
    return;
}

bool
native_tag_editor_screen_set_current_dir(NativeTagEditorScreen *screen,
                                         char *dir, int32 dir_len) {
    if (screen == NULL) {
        return false;
    }
    return ncm_buffer_set(&screen->current_dir, dir, dir_len);
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
    }
    ncm_buffer_destroy(&second);
    ncm_buffer_destroy(&first);
    nc_menu_string_pair_destroy(&pair);
    return ok;
}

bool
native_tag_editor_screen_load_directories(NativeTagEditorScreen *screen,
                                          NcmDirectoryArray *directories) {
    if (screen == NULL || directories == NULL) {
        return false;
    }
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
            return false;
        }
    }
    return true;
}

bool
native_tag_editor_screen_load_songs(NativeTagEditorScreen *screen,
                                    NcmSongArray *songs) {
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
    return true;
}

bool
native_tag_editor_screen_add_mutable_song(NativeTagEditorScreen *screen,
                                          NcmMutableSong *song) {
    if (screen == NULL || song == NULL) {
        return false;
    }
    nc_tag_row_menu_add(&screen->tags, song);
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
                                        NcmMutableSongArray *songs) {
    NcMenu *menu;
    bool has_selected;

    if (screen == NULL || songs == NULL) {
        return false;
    }
    menu = nc_tag_row_menu_base(&screen->tags);
    has_selected = nc_menu_has_selected(menu);
    ncm_mutable_song_array_clear(songs);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (has_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!tag_editor_copy_selected_song_at(screen, songs, i)) {
            return false;
        }
    }
    if ((songs->len == 0) && !nc_menu_empty(menu)) {
        if (!tag_editor_copy_selected_song_at(screen, songs,
                                             nc_menu_highlight(menu))) {
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
    return screen->active_column > NATIVE_TAG_EDITOR_COLUMN_DIRECTORIES;
}

bool
native_tag_editor_screen_next_column_available(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->active_column < NATIVE_TAG_EDITOR_COLUMN_TAGS;
}

void
native_tag_editor_screen_previous_column(NativeTagEditorScreen *screen) {
    if (!native_tag_editor_screen_previous_column_available(screen)) {
        return;
    }
    screen->active_column -= 1;
    return;
}

void
native_tag_editor_screen_next_column(NativeTagEditorScreen *screen) {
    if (!native_tag_editor_screen_next_column_available(screen)) {
        return;
    }
    screen->active_column += 1;
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

        song = nc_tag_row_menu_item_at(&screen->tags, NC_MENU_ITEMS_FILTERED,
                                       i);
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
    NcMenuDisplayCallbacks callbacks = {0};

    if (screen == NULL) {
        return false;
    }
    if (!ncm_regex_compile(&screen->directory_filter_regex, pattern,
                           pattern_len, regex_flags, error)) {
        return false;
    }
    callbacks.filter = tag_editor_directory_filter;
    callbacks.user = screen;
    ncm_buffer_set(&screen->directory_search_constraint, pattern,
                   pattern_len);
    nc_menu_set_display_callbacks(nc_editor_pair_menu_base(
                                      &screen->directories), callbacks);
    screen->directory_filter_enabled = true;
    nc_menu_apply_filter(nc_editor_pair_menu_base(&screen->directories));
    return true;
}

bool
native_tag_editor_screen_apply_tag_filter(
    NativeTagEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, NcmError *error) {
    NcMenuDisplayCallbacks callbacks = {0};

    if (screen == NULL) {
        return false;
    }
    if (!ncm_regex_compile(&screen->tag_filter_regex, pattern,
                           pattern_len, regex_flags, error)) {
        return false;
    }
    callbacks.filter = tag_editor_tag_filter;
    callbacks.user = screen;
    ncm_buffer_set(&screen->tag_search_constraint, pattern, pattern_len);
    nc_menu_set_display_callbacks(nc_tag_row_menu_base(&screen->tags),
                                  callbacks);
    screen->tag_filter_enabled = true;
    nc_menu_apply_filter(nc_tag_row_menu_base(&screen->tags));
    return true;
}

void
native_tag_editor_screen_clear_filters(NativeTagEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->directory_filter_enabled = false;
    screen->tag_filter_enabled = false;
    ncm_buffer_clear(&screen->directory_search_constraint);
    ncm_buffer_clear(&screen->tag_search_constraint);
    nc_menu_show_all_items(nc_editor_pair_menu_base(&screen->directories));
    nc_menu_show_all_items(nc_tag_row_menu_base(&screen->tags));
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
        if (song->new_name != NULL) {
            ncm_buffer_append(buffer, song->name, song->name_len);
            ncm_buffer_append(buffer, STRLIT_ARGS(" -> "));
            ncm_buffer_append(buffer, song->new_name, song->new_name_len);
        } else {
            ncm_buffer_append(buffer, song->name, song->name_len);
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
    return native_tag_editor_screen_active_window(tag_editor_from_screen(
                                                     screen));
}

static void
tag_editor_refresh(NcScreen *screen) {
    NativeTagEditorScreen *editor;

    editor = tag_editor_from_screen(screen);
    tag_editor_refresh_window(screen);
    nc_window_display(&editor->tag_types_window);
    nc_window_display(&editor->tags_window);
    return;
}

static void
tag_editor_refresh_window(NcScreen *screen) {
    NativeTagEditorScreen *editor;
    NcMenu *menu;
    NcWindow *window;

    editor = tag_editor_from_screen(screen);
    menu = native_tag_editor_screen_active_menu(editor);
    window = native_tag_editor_screen_active_window(editor);
    nc_menu_prepare_refresh(menu, editor->main_height, NULL, NULL);
    nc_window_display(window);
    nc_menu_refresh(menu, window, nc_window_width(window),
                    nc_window_height(window));
    return;
}

static void
tag_editor_scroll(NcScreen *screen, enum NcScroll where) {
    NativeTagEditorScreen *editor;
    NcMenu *menu;

    editor = tag_editor_from_screen(screen);
    menu = native_tag_editor_screen_active_menu(editor);
    nc_menu_scroll_selectable(menu, editor->main_height, where);
    return;
}

static void
tag_editor_switch_to(NcScreen *screen) {
    (void)nc_screen_switcher_switch_to(screen, nc_screen_has_to_be_resized(
                                           screen));
    return;
}

static void
tag_editor_resize(NcScreen *screen) {
    NativeTagEditorScreen *editor;
    NcScreenResizeParams params;

    editor = tag_editor_from_screen(screen);
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
    (void)screen;
    return (char *)"Tag editor";
}

static void
tag_editor_update(NcScreen *screen) {
    nc_screen_clear_update_request(screen);
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
tag_editor_layout(NativeTagEditorScreen *screen) {
    int64 middle_width;

    middle_width = 26;
    if (screen->width < middle_width + 2) {
        middle_width = screen->width;
    }
    screen->left_width = (screen->width - middle_width)/2;
    screen->middle_start_x = screen->start_x + screen->left_width + 1;
    screen->middle_width = middle_width;
    screen->right_start_x = screen->middle_start_x + middle_width + 1;
    screen->right_width = screen->width - screen->left_width
                          - middle_width - 2;
    if (screen->right_width < 0) {
        screen->right_width = 0;
    }

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
                                  NcmMutableSongArray *songs, int64 pos) {
    NcmMutableSong *song;

    song = nc_tag_row_menu_item_at(&screen->tags, NC_MENU_ITEMS_FILTERED,
                                   pos);
    if (song == NULL) {
        return false;
    }
    return ncm_mutable_song_array_append_copy(songs, song);
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
        song = nc_tag_row_menu_item_at(&screen->tags,
                                       NC_MENU_ITEMS_FILTERED, i);
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
    for (uint32 field = 0; field < NCM_TAGS_FIELD_LAST; field += 1) {
        NcmBuffer buffer;
        bool found;

        buffer = ncm_mutable_song_tags_buffer(song, (enum NcmTagsField)field,
                                              STRLIT_ARGS(" "), false);
        found = ncm_regex_search(&screen->tag_filter_regex, buffer.data,
                                 buffer.len);
        ncm_buffer_destroy(&buffer);
        if (found) {
            return true;
        }
    }
    return false;
}

static bool
tag_editor_directory_matches(NativeTagEditorScreen *screen,
                             NcMenuStringPair *pair) {
    if (!screen->directory_filter_enabled) {
        return true;
    }
    if (pair == NULL || pair->first == NULL) {
        return false;
    }
    if (ncm_string_equal(pair->first, pair->first_len, STRLIT_ARGS("."))) {
        return false;
    }
    if (ncm_string_equal(pair->first, pair->first_len, STRLIT_ARGS(".."))) {
        return false;
    }
    return ncm_regex_search(&screen->directory_filter_regex, pair->first,
                            pair->first_len);
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

    tag = ncm_mutable_song_tags_buffer(song, field, STRLIT_ARGS(", "),
                                       false);
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
