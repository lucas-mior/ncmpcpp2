#if !defined(NCMPCPP_NC_TINY_TAG_EDITOR_C)
#define NCMPCPP_NC_TINY_TAG_EDITOR_C

#include "screens/nc_tiny_tag_editor.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "app_controller.h"
#include "c/ncm_base.h"
#include "c/ncm_string.h"
#include "c/ncm_type_conversions.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"
#include "screens/screen_switcher.h"
#include "settings.h"
#include "title.h"
#include "ui_state.h"

static NcWindow *tiny_editor_active_window(NcScreen *screen);
static bool tiny_editor_can_run_current(NcScreen *screen);
static bool tiny_editor_run_current(NcScreen *screen);
static void tiny_editor_refresh(NcScreen *screen);
static void tiny_editor_refresh_window(NcScreen *screen);
static void tiny_editor_scroll(NcScreen *screen, enum NcScroll where);
static void tiny_editor_switch_to(NcScreen *screen);
static void tiny_editor_resize(NcScreen *screen);
static char *tiny_editor_title(NcScreen *screen);
static void tiny_editor_update(NcScreen *screen);
static void tiny_editor_mouse_callback(NcScreen *screen, MEVENT event);
static bool tiny_editor_is_lockable(NcScreen *screen);
static bool tiny_editor_is_mergable(NcScreen *screen);
static void tiny_editor_destroy_callback(NcScreen *screen);
static void tiny_editor_draw_row(NcMenu *menu, NcWindow *window,
                                 void *item, int32 pos, void *user);
static void tiny_editor_print_buffer(NcWindow *window, NcBuffer *buffer);
static bool tiny_editor_add_row(NativeTinyTagEditorScreen *screen,
                                NcBuffer *buffer, uint32 flags);
static int32 tiny_editor_current_row(NativeTinyTagEditorScreen *screen);
static void tiny_editor_status_message(
    NativeTinyTagEditorScreen *screen, char *message, int32 message_len);
static bool tiny_editor_replace_tag_row(NativeTinyTagEditorScreen *screen,
                                        enum NcmTagsField field);
static bool tiny_editor_replace_filename_row(NativeTinyTagEditorScreen *screen);
static bool tiny_editor_write_song(NativeTinyTagEditorScreen *screen,
                                   char *music_dir);
static void tiny_editor_complete_save(NativeTinyTagEditorScreen *screen);
static bool tiny_editor_finish(NativeTinyTagEditorScreen *screen);
static void tiny_editor_buffer_key_value(NcBuffer *buffer, char *key,
                                         int32 key_len, char *value,
                                         int32 value_len);
static void tiny_editor_buffer_mutable_tag(NcBuffer *buffer,
                                           NcmMutableSong *song,
                                           enum NcmTagsField field,
                                           char *tag_separator,
                                           int32 tag_separator_len,
                                           bool show_duplicate_tags);
static void tiny_editor_buffer_uint(NcBuffer *buffer, uint32 value,
                                    char *suffix, int32 suffix_len);
static void tiny_editor_buffer_key_uint(NcBuffer *buffer, char *key,
                                        int32 key_len, uint32 value,
                                        char *suffix, int32 suffix_len);
static int32 tiny_editor_channels_to_string(int32 channels,
                                            char *buffer,
                                            int32 buffer_cap);

static NcScreenCallbacks tiny_editor_callbacks = {
    .active_window = tiny_editor_active_window,
    .can_run_current = tiny_editor_can_run_current,
    .run_current = tiny_editor_run_current,
    .refresh = tiny_editor_refresh,
    .refresh_window = tiny_editor_refresh_window,
    .scroll = tiny_editor_scroll,
    .switch_to = tiny_editor_switch_to,
    .resize = tiny_editor_resize,
    .title = tiny_editor_title,
    .update = tiny_editor_update,
    .mouse_button_pressed = tiny_editor_mouse_callback,
    .is_lockable = tiny_editor_is_lockable,
    .is_mergable = tiny_editor_is_mergable,
    .destroy = tiny_editor_destroy_callback,
};

void
native_tiny_tag_editor_screen_init(
    NativeTinyTagEditorScreen *screen, int32 start_x, int32 width,
    int32 main_start_y, int32 main_height, NcColor color, NcBorder border
) {
    NcMenuDisplayCallbacks display_callbacks = {0};
    NcMenu *menu;

    nc_editor_buffer_menu_init(&screen->rows);
    menu = nc_editor_buffer_menu_base(&screen->rows);
    display_callbacks.draw = tiny_editor_draw_row;

    nc_menu_set_display_callbacks(menu, display_callbacks);
    nc_menu_set_highlight_prefix(menu, &Config.current_item_prefix);
    nc_menu_set_highlight_suffix(menu, &Config.current_item_suffix);
    nc_menu_set_cyclic_scrolling(menu, Config.use_cyclic_scrolling);
    nc_menu_set_centered_cursor(menu, Config.centered_cursor);

    nc_window_init(&screen->window, start_x, main_start_y, width,
                   main_height, NULL, 0, color, border);
    screen->hooks = (NativeTinyTagEditorHooks){0};
    ncm_mutable_song_init(&screen->edited);

    ncm_buffer_init(&screen->music_dir);
    ncm_buffer_init(&screen->tag_separator);

    screen->previous_screen = NULL;
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    screen->has_edited = false;
    screen->show_duplicate_tags = false;
    screen->registered = false;

    nc_screen_init(&screen->screen, tiny_editor_callbacks, screen,
                   NC_SCREEN_TYPE_TINY_TAG_EDITOR);
    return;
}

void
native_tiny_tag_editor_screen_destroy(NativeTinyTagEditorScreen *screen) {
    if (screen == NULL) {
        return;
    }

    (void)app_controller_unregister_screen(
        native_tiny_tag_editor_screen_base(screen));
    ncm_mutable_song_destroy(&screen->edited);
    ncm_buffer_destroy(&screen->music_dir);
    ncm_buffer_destroy(&screen->tag_separator);
    nc_window_destroy(&screen->window);

    nc_editor_buffer_menu_destroy(&screen->rows);

    screen->previous_screen = NULL;
    screen->has_edited = false;
    screen->show_duplicate_tags = false;
    screen->registered = false;

    return;
}

NcScreen *
native_tiny_tag_editor_screen_base(NativeTinyTagEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->screen;
}

void
native_tiny_tag_editor_screen_set_hooks(
    NativeTinyTagEditorScreen *screen, NativeTinyTagEditorHooks hooks
) {
    if (screen == NULL) {
        return;
    }
    screen->hooks = hooks;
    return;
}

NcEditorBufferMenu *
native_tiny_tag_editor_screen_rows(NativeTinyTagEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->rows;
}

void
native_tiny_tag_editor_screen_set_geometry(
    NativeTinyTagEditorScreen *screen, int32 start_x, int32 width,
    int32 main_start_y, int32 main_height
) {
    if (screen == NULL) {
        return;
    }

    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;

    nc_window_move_to(&screen->window, start_x, main_start_y);
    nc_window_resize(&screen->window, width, main_height);

    return;
}

bool
native_tiny_tag_editor_screen_set_edited_song(
    NativeTinyTagEditorScreen *screen, NcmSong *song
) {
    NcmMutableSong edited;

    if ((screen == NULL) || (song == NULL)) {
        return false;
    }
    if (ncm_song_is_stream(song)) {
        return false;
    }

    ncm_mutable_song_init(&edited);
    if (!ncm_mutable_song_load_originals_from_song(&edited, song)) {
        ncm_mutable_song_destroy(&edited);
        return false;
    }
    ncm_mutable_song_set_duration(&edited, ncm_song_duration(song));
    ncm_mutable_song_set_mtime(&edited, (int32)ncm_song_mtime(song));
    ncm_mutable_song_move(&screen->edited, &edited);
    screen->has_edited = true;
    return true;
}

enum NativeTinyTagEditorOpenResult
native_tiny_tag_editor_screen_open_song(
    NativeTinyTagEditorScreen *screen, NcmSong *song,
    char *music_dir, int32 music_dir_len, char *tag_separator,
    int32 tag_separator_len, bool show_duplicate_tags, NcmBuffer *path
) {
    NcmTaglibAudioProperties properties = {0};
    NcmTaglibFile file;
    bool extended_tags_supported;
    bool opened;
    bool rows_loaded;

    if (path) {
        ncm_buffer_clear(path);
    }
    if ((screen == NULL) || (song == NULL) || (path == NULL)
        || (music_dir_len < 0) || (tag_separator_len < 0)
        || ((music_dir_len > 0) && (music_dir == NULL))
        || ((tag_separator_len > 0) && (tag_separator == NULL))) {
        return NATIVE_TINY_TAG_EDITOR_OPEN_INVALID_ARGUMENT;
    }
    if (ncm_song_is_stream(song)) {
        return NATIVE_TINY_TAG_EDITOR_OPEN_STREAM;
    }
    if (ncm_song_is_from_database(song) && (music_dir_len <= 0)) {
        return NATIVE_TINY_TAG_EDITOR_OPEN_MISSING_MUSIC_DIRECTORY;
    }
    if (!ncm_buffer_set(&screen->music_dir, music_dir, music_dir_len)
        || !ncm_buffer_set(&screen->tag_separator, tag_separator,
                           tag_separator_len)) {
        return NATIVE_TINY_TAG_EDITOR_OPEN_PREPARE_FAILED;
    }
    screen->show_duplicate_tags = show_duplicate_tags;
    if (!native_tiny_tag_editor_screen_set_edited_song(screen, song)) {
        return NATIVE_TINY_TAG_EDITOR_OPEN_PREPARE_FAILED;
    }

    if (screen->edited.is_from_database) {
        ncm_buffer_append(path, music_dir, music_dir_len);
    }
    ncm_buffer_append(path, screen->edited.uri, screen->edited.uri_len);

    ncm_taglib_file_init(&file);
    if (screen->hooks.taglib_open) {
        opened = screen->hooks.taglib_open(
            screen->hooks.user, &file, path->data, path->len);
    } else {
        opened = ncm_taglib_file_open(&file, path->data);
    }
    if (!opened) {
        if (screen->hooks.taglib_close) {
            screen->hooks.taglib_close(screen->hooks.user, &file);
        } else {
            ncm_taglib_file_close(&file);
        }
        screen->has_edited = false;
        nc_menu_clear_items(nc_editor_buffer_menu_base(&screen->rows));
        return NATIVE_TINY_TAG_EDITOR_OPEN_UNREADABLE_FILE;
    }

    if (screen->hooks.taglib_audio_properties) {
        (void)screen->hooks.taglib_audio_properties(
            screen->hooks.user, &file, &properties);
    } else {
        (void)ncm_taglib_file_audio_properties(&file, &properties);
    }
    if (screen->hooks.taglib_extended_set_supported) {
        extended_tags_supported =
            screen->hooks.taglib_extended_set_supported(
                screen->hooks.user, &file);
    } else {
        extended_tags_supported = ncm_taglib_extended_set_supported(&file);
    }

    rows_loaded = native_tiny_tag_editor_screen_reload_rows(
        screen, &properties, extended_tags_supported, tag_separator,
        tag_separator_len, show_duplicate_tags);
    if (screen->hooks.taglib_close) {
        screen->hooks.taglib_close(screen->hooks.user, &file);
    } else {
        ncm_taglib_file_close(&file);
    }
    if (!rows_loaded) {
        screen->has_edited = false;
        nc_menu_clear_items(nc_editor_buffer_menu_base(&screen->rows));
        return NATIVE_TINY_TAG_EDITOR_OPEN_PREPARE_FAILED;
    }
    return NATIVE_TINY_TAG_EDITOR_OPEN_SUCCESS;
}

bool
native_tiny_tag_editor_screen_reload_rows(
    NativeTinyTagEditorScreen *screen,
    NcmTaglibAudioProperties *properties,
    bool extended_tags_supported, char *tag_separator,
    int32 tag_separator_len, bool show_duplicate_tags
) {
    NcBuffer row;
    char channel_buffer[32];
    char duration_buffer[32];
    int32 channel_len;
    int32 duration_len;

    if ((screen == NULL) || !screen->has_edited
        || (tag_separator_len < 0)
        || ((tag_separator == NULL) && (tag_separator_len > 0))) {
        return false;
    }
    if (!ncm_buffer_set(&screen->tag_separator, tag_separator,
                        tag_separator_len)) {
        return false;
    }
    screen->show_duplicate_tags = show_duplicate_tags;
    nc_menu_clear_items(nc_editor_buffer_menu_base(&screen->rows));
    nc_buffer_init(&row);
    tiny_editor_buffer_key_value(&row, STRLIT_ARGS("Filename"),
                                 screen->edited.name,
                                 screen->edited.name_len);
    if (!tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE)) {
        nc_buffer_destroy(&row);
        return false;
    }
    nc_buffer_clear(&row);
    tiny_editor_buffer_key_value(&row, STRLIT_ARGS("Directory"),
                                 screen->edited.directory,
                                 screen->edited.directory_len);
    if (!tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE)) {
        nc_buffer_destroy(&row);
        return false;
    }
    nc_buffer_clear(&row);
    if (!tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE)) {
        nc_buffer_destroy(&row);
        return false;
    }
    nc_buffer_clear(&row);
    if (ncm_mutable_song_duration(&screen->edited) > 0) {
        duration_len = ncm_song_show_time(
            ncm_mutable_song_duration(&screen->edited), duration_buffer,
            SIZEOF(duration_buffer));
    } else {
        duration_buffer[0] = '-';
        duration_buffer[1] = ':';
        duration_buffer[2] = '-';
        duration_buffer[3] = '-';
        duration_len = 4;
    }
    tiny_editor_buffer_key_value(&row, STRLIT_ARGS("Length"),
                                 duration_buffer, duration_len);
    if (!tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE)) {
        nc_buffer_destroy(&row);
        return false;
    }
    nc_buffer_clear(&row);
    if (properties) {
        tiny_editor_buffer_key_uint(
            &row, STRLIT_ARGS("Bitrate"), (uint32)properties->bitrate,
            STRLIT_ARGS(" kbps"));
    }
    if (!tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE)) {
        nc_buffer_destroy(&row);
        return false;
    }
    nc_buffer_clear(&row);
    if (properties) {
        tiny_editor_buffer_key_uint(
            &row, STRLIT_ARGS("Sample rate"),
            (uint32)properties->sample_rate, STRLIT_ARGS(" Hz"));
    }
    if (!tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE)) {
        nc_buffer_destroy(&row);
        return false;
    }
    nc_buffer_clear(&row);
    channel_len = 0;
    if (properties) {
        channel_len = tiny_editor_channels_to_string(properties->channels,
                                                     channel_buffer,
                                                     SIZEOF(channel_buffer));
    }
    tiny_editor_buffer_key_value(&row, STRLIT_ARGS("Channels"),
                                 channel_buffer, channel_len);
    if (!tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE)) {
        nc_buffer_destroy(&row);
        return false;
    }
    nc_buffer_destroy(&row);
    nc_editor_buffer_menu_add_separator(&screen->rows);

    for (uint32 field = 0; field < NCM_TAGS_FIELD_LAST; field += 1) {
        bool inactive;

        nc_buffer_init(&row);
        tiny_editor_buffer_mutable_tag(
            &row, &screen->edited, (enum NcmTagsField)field,
            tag_separator, tag_separator_len, show_duplicate_tags);
        inactive = !extended_tags_supported
                   && ((field == NCM_TAGS_FIELD_ALBUM_ARTIST)
                       || (field == NCM_TAGS_FIELD_COMPOSER)
                       || (field == NCM_TAGS_FIELD_PERFORMER)
                       || (field == NCM_TAGS_FIELD_DISC));
        if (inactive) {
            if (!tiny_editor_add_row(screen, &row,
                                     NC_MENU_ITEM_INACTIVE)) {
                nc_buffer_destroy(&row);
                return false;
            }
        } else if (!tiny_editor_add_row(screen, &row,
                                        NC_MENU_ITEM_SELECTABLE)) {
            nc_buffer_destroy(&row);
            return false;
        }
        nc_buffer_destroy(&row);
    }

    nc_editor_buffer_menu_add_separator(&screen->rows);
    nc_buffer_init(&row);
    tiny_editor_buffer_key_value(&row, STRLIT_ARGS("Filename"),
                                 screen->edited.name,
                                 screen->edited.name_len);
    if (!tiny_editor_add_row(screen, &row, NC_MENU_ITEM_SELECTABLE)) {
        nc_buffer_destroy(&row);
        return false;
    }
    nc_buffer_destroy(&row);
    nc_editor_buffer_menu_add_separator(&screen->rows);
    nc_buffer_init(&row);
    nc_buffer_append_data(&row, STRLIT_ARGS("Save"));
    if (!tiny_editor_add_row(screen, &row, NC_MENU_ITEM_SELECTABLE)) {
        nc_buffer_destroy(&row);
        return false;
    }
    nc_buffer_clear(&row);
    nc_buffer_append_data(&row, STRLIT_ARGS("Cancel"));
    if (!tiny_editor_add_row(screen, &row, NC_MENU_ITEM_SELECTABLE)) {
        nc_buffer_destroy(&row);
        return false;
    }
    nc_buffer_destroy(&row);
    nc_menu_highlight_position(nc_editor_buffer_menu_base(&screen->rows),
                               NATIVE_TINY_TAG_EDITOR_FIRST_TAG_ROW,
                               screen->main_height);
    return true;
}

bool
native_tiny_tag_editor_screen_set_tag_value(
    NativeTinyTagEditorScreen *screen, enum NcmTagsField field,
    char *value, int32 value_len, char *separator, int32 separator_len
) {
    if ((screen == NULL) || !screen->has_edited) {
        return false;
    }
    return ncm_mutable_song_set_tags(&screen->edited, field, value,
                                     value_len, separator, separator_len);
}

bool
native_tiny_tag_editor_screen_set_filename(
    NativeTinyTagEditorScreen *screen, char *name, int32 name_len
) {
    if ((screen == NULL) || !screen->has_edited) {
        return false;
    }
    return ncm_mutable_song_set_new_name(&screen->edited, name, name_len);
}

bool
native_tiny_tag_editor_screen_set_filename_stem(
    NativeTinyTagEditorScreen *screen, char *stem, int32 stem_len
) {
    NcmStringView current_name;
    StrBuilder new_name;
    int32 dot;
    bool result;

    if ((screen == NULL) || !screen->has_edited || (stem == NULL)
        || (stem_len <= 0)) {
        return false;
    }

    if (!ncm_mutable_song_get_new_name(&screen->edited, &current_name)) {
        current_name.data = screen->edited.name;
        current_name.len = screen->edited.name_len;
    }

    dot = -1;
    for (int32 i = 0; i < current_name.len; i += 1) {
        if (current_name.data[i] == '.') {
            dot = i;
        }
    }

    sb_init(&new_name);
    sb_append(&new_name, stem, stem_len);
    if (dot >= 0) {
        sb_append(&new_name, &current_name.data[dot],
                          current_name.len - dot);
    }
    result = native_tiny_tag_editor_screen_set_filename(
        screen, new_name.data, new_name.len);
    sb_free(&new_name);
    return result;
}

bool
native_tiny_tag_editor_screen_run_row(
    NativeTinyTagEditorScreen *screen, int32 row
) {
    enum NativeTinyTagEditorPromptResult prompt_result;
    enum NcmTagsField field;
    NcmStringView initial;
    char *field_name;
    NcmStringView current_name;
    StrBuilder input;
    StrBuilder tag_value;
    NcMenu *menu;
    char error_buffer[256];
    int32 error_len;
    int32 dot;
    bool result;

    if ((screen == NULL) || !screen->has_edited) {
        return false;
    }
    menu = nc_editor_buffer_menu_base(&screen->rows);
    if ((row < 0) || (row >= nc_menu_all_item_count(menu))
        || !nc_menu_position_is_selectable(menu, row)) {
        return false;
    }

    if ((row >= (int32)NATIVE_TINY_TAG_EDITOR_FIRST_TAG_ROW)
        && (row <= (int32)NATIVE_TINY_TAG_EDITOR_LAST_TAG_ROW)) {
        field = (enum NcmTagsField)(
            row - (int32)NATIVE_TINY_TAG_EDITOR_FIRST_TAG_ROW);
        tag_value = ncm_mutable_song_tags_buffer(
            &screen->edited, field, screen->tag_separator.data,
            screen->tag_separator.len, screen->show_duplicate_tags);
        initial.data = tag_value.data;
        initial.len = tag_value.len;
        field_name = ncm_tags_field_name(field);
        sb_init(&input);
        if (screen->hooks.prompt == NULL) {
            prompt_result = NATIVE_TINY_TAG_EDITOR_PROMPT_ERROR;
        } else {
            prompt_result = screen->hooks.prompt(
                screen->hooks.user, field_name,
                optional_strlen32(field_name), initial, &input);
        }
        sb_free(&tag_value);
        if (prompt_result == NATIVE_TINY_TAG_EDITOR_PROMPT_ABORTED) {
            tiny_editor_status_message(
                screen, STRLIT_ARGS("Action aborted"));
            sb_free(&input);
            return false;
        }
        if (prompt_result != NATIVE_TINY_TAG_EDITOR_PROMPT_ACCEPTED) {
            sb_free(&input);
            return false;
        }
        result = native_tiny_tag_editor_screen_set_tag_value(
            screen, field, input.data, input.len,
            screen->tag_separator.data, screen->tag_separator.len);
        sb_free(&input);
        if (!result || !tiny_editor_replace_tag_row(screen, field)) {
            return false;
        }
        return true;
    }

    if (row == NATIVE_TINY_TAG_EDITOR_FILE_NAME_EDIT_ROW) {
        if (!ncm_mutable_song_get_new_name(&screen->edited,
                                           &current_name)) {
            current_name.data = screen->edited.name;
            current_name.len = screen->edited.name_len;
        }
        dot = -1;
        for (int32 i = 0; i < current_name.len; i += 1) {
            if (current_name.data[i] == '.') {
                dot = i;
            }
        }
        initial = current_name;
        if (dot >= 0) {
            initial.len = dot;
        }

        sb_init(&input);
        if (screen->hooks.prompt == NULL) {
            prompt_result = NATIVE_TINY_TAG_EDITOR_PROMPT_ERROR;
        } else {
            prompt_result = screen->hooks.prompt(
                screen->hooks.user, STRLIT_ARGS("Filename"), initial,
                &input);
        }
        if (prompt_result == NATIVE_TINY_TAG_EDITOR_PROMPT_ABORTED) {
            tiny_editor_status_message(
                screen, STRLIT_ARGS("Action aborted"));
            sb_free(&input);
            return false;
        }
        if (prompt_result != NATIVE_TINY_TAG_EDITOR_PROMPT_ACCEPTED) {
            sb_free(&input);
            return false;
        }
        if (input.len <= 0) {
            sb_free(&input);
            return true;
        }
        result = native_tiny_tag_editor_screen_set_filename_stem(
            screen, input.data, input.len);
        sb_free(&input);
        if (!result || !tiny_editor_replace_filename_row(screen)) {
            return false;
        }
        return true;
    }

    if (row == NATIVE_TINY_TAG_EDITOR_SAVE_ROW) {
        tiny_editor_status_message(
            screen, STRLIT_ARGS("Updating tags..."));
        if (tiny_editor_write_song(screen, screen->music_dir.data)) {
            tiny_editor_status_message(
                screen, STRLIT_ARGS("Tags updated"));
            tiny_editor_complete_save(screen);
        } else {
            error_len = SNPRINTF(error_buffer, "Error while writing tags: %s",
                                 strerror(errno));
            if (error_len < 0) {
                error_len = 0;
            } else if (error_len >= SIZEOF(error_buffer)) {
                error_len = SIZEOF(error_buffer) - 1;
            }
            tiny_editor_status_message(screen, error_buffer, error_len);
        }
        (void)tiny_editor_finish(screen);
        return true;
    }

    if (row == NATIVE_TINY_TAG_EDITOR_CANCEL_ROW) {
        (void)tiny_editor_finish(screen);
        return true;
    }
    return false;
}

bool
native_tiny_tag_editor_screen_run_current(
    NativeTinyTagEditorScreen *screen
) {
    if (!native_tiny_tag_editor_screen_action_runnable(screen)) {
        return false;
    }
    return native_tiny_tag_editor_screen_run_row(
        screen, tiny_editor_current_row(screen));
}

bool
native_tiny_tag_editor_screen_action_runnable(
    NativeTinyTagEditorScreen *screen
) {
    NcMenu *menu;
    int32 row;

    if ((screen == NULL) || !screen->has_edited) {
        return false;
    }
    menu = nc_editor_buffer_menu_base(&screen->rows);
    if (nc_menu_empty(menu)) {
        return false;
    }
    row = tiny_editor_current_row(screen);
    return (row >= 0) && (row < nc_menu_all_item_count(menu))
           && nc_menu_position_is_selectable(menu, row);
}

static NativeTinyTagEditorScreen *
tiny_editor_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcWindow *
tiny_editor_active_window(NcScreen *screen) {
    NativeTinyTagEditorScreen *editor;

    editor = tiny_editor_from_screen(screen);
    return &editor->window;
}

static bool
tiny_editor_can_run_current(NcScreen *screen) {
    return native_tiny_tag_editor_screen_action_runnable(
        tiny_editor_from_screen(screen));
}

static bool
tiny_editor_run_current(NcScreen *screen) {
    return native_tiny_tag_editor_screen_run_current(
        tiny_editor_from_screen(screen));
}

static void
tiny_editor_refresh(NcScreen *screen) {
    NativeTinyTagEditorScreen *editor = tiny_editor_from_screen(screen);
    NcMenu *menu = nc_editor_buffer_menu_base(&editor->rows);

    nc_menu_prepare_refresh(menu, editor->main_height, NULL, NULL);
    nc_window_display(&editor->window);
    nc_menu_refresh(menu, &editor->window, editor->width,
                    editor->main_height);

    return;
}

static void
tiny_editor_refresh_window(NcScreen *screen) {
    tiny_editor_refresh(screen);
    return;
}

static void
tiny_editor_scroll(NcScreen *screen, enum NcScroll where) {
    NativeTinyTagEditorScreen *editor = tiny_editor_from_screen(screen);
    nc_menu_scroll_selectable(nc_editor_buffer_menu_base(&editor->rows),
                              editor->main_height, where);
    return;
}

static void
tiny_editor_switch_to(NcScreen *screen) {
    NativeTinyTagEditorScreen *editor;

    editor = tiny_editor_from_screen(screen);
    editor->previous_screen = app_controller_previous_screen();
    ncm_title_draw_header(STRLIT_ARGS("Tiny tag editor"));
    return;
}

static void
tiny_editor_resize(NcScreen *screen) {
    int32 start_x;
    int32 width;
    NativeTinyTagEditorScreen *editor = tiny_editor_from_screen(screen);

    nc_screen_switcher_get_resize_params(screen, &start_x, &width, true);
    native_tiny_tag_editor_screen_set_geometry(editor, start_x, width,
                                               ui_state_main_start_y(),
                                               ui_state_main_height());
    nc_screen_clear_resize_request(screen);

    return;
}

static char *
tiny_editor_title(NcScreen *screen) {
    (void)screen;
    return "Tiny tag editor";
}

static void
tiny_editor_update(NcScreen *screen) {
    nc_screen_clear_update_request(screen);
    return;
}

static void
tiny_editor_mouse_callback(NcScreen *screen, MEVENT event) {
    NativeTinyTagEditorScreen *editor;
    NcMenu *menu;
    enum NcScroll where;
    int32 count;
    int32 x;
    int32 y;

    editor = tiny_editor_from_screen(screen);
    x = event.x;
    y = event.y;
    if (!nc_window_has_coords(&editor->window, &x, &y)) {
        return;
    }

    menu = nc_editor_buffer_menu_base(&editor->rows);
    if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
        if (!nc_menu_goto_selectable(menu, y)) {
            return;
        }
        if (event.bstate & BUTTON3_PRESSED) {
            tiny_editor_refresh_window(screen);
            (void)native_tiny_tag_editor_screen_run_current(editor);
        }
        return;
    }

    count = Config.lines_scrolled;
    if (event.bstate & BUTTON5_PRESSED) {
        where = NC_SCROLL_DOWN;
    } else if (event.bstate & BUTTON4_PRESSED) {
        where = NC_SCROLL_UP;
    } else {
        return;
    }
    if (Config.mouse_list_scroll_whole_page) {
        count = 1;
        if (where == NC_SCROLL_DOWN) {
            where = NC_SCROLL_PAGE_DOWN;
        } else {
            where = NC_SCROLL_PAGE_UP;
        }
    }
    for (int32 i = 0; i < count; i += 1) {
        tiny_editor_scroll(screen, where);
    }
    return;
}

static void
tiny_editor_draw_row(NcMenu *menu, NcWindow *window, void *item,
                     int32 pos, void *user) {
    NcBuffer *buffer;

    (void)menu;
    (void)pos;
    (void)user;
    buffer = item;
    if ((window == NULL) || (buffer == NULL)) {
        return;
    }
    tiny_editor_print_buffer(window, buffer);
    return;
}

static void
tiny_editor_print_buffer(NcWindow *window, NcBuffer *buffer) {
    NcBufferProperty *properties;
    int32 property_count;
    int32 property_index;

    properties = nc_buffer_properties(buffer);
    property_count = nc_buffer_property_count(buffer);
    property_index = 0;
    for (int32 i = 0;; i += 1) {
        while ((property_index < property_count)
               && (properties[property_index].position == i)) {
            nc_buffer_apply_property(window, &properties[property_index]);
            property_index += 1;
        }
        if (i >= nc_buffer_len(buffer)) {
            break;
        }
        nc_window_print_char(window, nc_buffer_data(buffer)[i]);
    }
    return;
}

static bool
tiny_editor_is_lockable(NcScreen *screen) {
    (void)screen;
    return false;
}

static bool
tiny_editor_is_mergable(NcScreen *screen) {
    (void)screen;
    return true;
}

static void
tiny_editor_destroy_callback(NcScreen *screen) {
    native_tiny_tag_editor_screen_destroy(tiny_editor_from_screen(screen));
    return;
}

static bool
tiny_editor_add_row(NativeTinyTagEditorScreen *screen, NcBuffer *buffer,
                    uint32 flags) {
    if ((screen == NULL) || (buffer == NULL)) {
        return false;
    }
    nc_editor_buffer_menu_add_with_flags(&screen->rows, buffer, flags);
    return true;
}

static int32
tiny_editor_current_row(NativeTinyTagEditorScreen *screen) {
    return nc_menu_highlight(nc_editor_buffer_menu_base(&screen->rows));
}

static void
tiny_editor_status_message(
    NativeTinyTagEditorScreen *screen, char *message, int32 message_len
) {
    if (screen->hooks.status_message && message
        && (message_len >= 0)) {
        screen->hooks.status_message(screen->hooks.user, message,
                                     message_len);
    }
    return;
}

static bool
tiny_editor_replace_tag_row(
    NativeTinyTagEditorScreen *screen, enum NcmTagsField field
) {
    NcBuffer row;
    NcMenu *menu;
    int32 row_index;
    bool result;

    row_index = NATIVE_TINY_TAG_EDITOR_TAG_ROW(field);
    menu = nc_editor_buffer_menu_base(&screen->rows);
    nc_buffer_init(&row);
    tiny_editor_buffer_mutable_tag(
        &row, &screen->edited, field, screen->tag_separator.data,
        screen->tag_separator.len, screen->show_duplicate_tags);
    result = nc_menu_replace_item(menu, NC_MENU_ITEMS_ALL, row_index,
                                  &row);
    nc_buffer_destroy(&row);
    return result;
}

static bool
tiny_editor_replace_filename_row(
    NativeTinyTagEditorScreen *screen
) {
    NcmStringView name;
    NcBuffer row;
    NcMenu *menu;
    bool result;

    if (!ncm_mutable_song_get_new_name(&screen->edited, &name)) {
        name.data = screen->edited.name;
        name.len = screen->edited.name_len;
    }
    menu = nc_editor_buffer_menu_base(&screen->rows);
    nc_buffer_init(&row);
    tiny_editor_buffer_key_value(&row, STRLIT_ARGS("Filename"),
                                 name.data, name.len);
    result = nc_menu_replace_item(
        menu, NC_MENU_ITEMS_ALL,
        NATIVE_TINY_TAG_EDITOR_FILE_NAME_EDIT_ROW, &row);
    nc_buffer_destroy(&row);
    return result;
}

static bool
tiny_editor_write_song(NativeTinyTagEditorScreen *screen,
                       char *music_dir) {
    if ((screen == NULL) || !screen->has_edited) {
        return false;
    }
    if (screen->hooks.write_song) {
        return screen->hooks.write_song(
            screen->hooks.user, &screen->edited, music_dir);
    }
    return ncm_mutable_song_write(&screen->edited, music_dir);
}

static void
tiny_editor_complete_save(NativeTinyTagEditorScreen *screen) {
    int32 previous_type;

    if (screen->edited.is_from_database) {
        if (screen->hooks.update_directory) {
            screen->hooks.update_directory(
                screen->hooks.user, screen->edited.directory,
                screen->edited.directory_len);
        }
    } else if (screen->previous_screen) {
        previous_type = nc_screen_type(screen->previous_screen);
        if ((previous_type == NC_SCREEN_TYPE_PLAYLIST)
            && screen->hooks.update_playlist_song) {
            screen->hooks.update_playlist_song(
                screen->hooks.user, &screen->edited);
        } else if ((previous_type == NC_SCREEN_TYPE_BROWSER)
                   && screen->hooks.request_browser_update) {
            screen->hooks.request_browser_update(screen->hooks.user);
        }
    }

    ncm_mutable_song_clear_modifications(&screen->edited);
    return;
}

static bool
tiny_editor_finish(NativeTinyTagEditorScreen *screen) {
    NcScreen *previous;

    previous = screen->previous_screen;
    if (previous == NULL) {
        return true;
    }
    if (screen->hooks.switch_to_screen) {
        screen->hooks.switch_to_screen(screen->hooks.user, previous);
        return true;
    }
    return app_controller_switch_to_screen(previous);
}

static void
tiny_editor_buffer_key_value(NcBuffer *buffer, char *key, int32 key_len,
                             char *value, int32 value_len) {
    nc_buffer_append_data(buffer, key, key_len);
    nc_buffer_append_data(buffer, STRLIT_ARGS(": "));
    if (value && (value_len > 0)) {
        nc_buffer_append_data(buffer, value, value_len);
    }
    return;
}

static void
tiny_editor_buffer_mutable_tag(
    NcBuffer *buffer, NcmMutableSong *song, enum NcmTagsField field,
    char *tag_separator, int32 tag_separator_len,
    bool show_duplicate_tags
) {
    StrBuilder value;
    char *name;

    name = ncm_tags_field_name(field);
    tiny_editor_buffer_key_value(buffer,
                                 name, optional_strlen32(name), NULL, 0);
    value = ncm_mutable_song_tags_buffer(song, field,
                                         tag_separator, tag_separator_len,
                                         show_duplicate_tags);
    nc_buffer_append_data(buffer, value.data, value.len);
    sb_free(&value);
    return;
}

static void
tiny_editor_buffer_key_uint(NcBuffer *buffer, char *key, int32 key_len,
                            uint32 value, char *suffix, int32 suffix_len) {
    tiny_editor_buffer_key_value(buffer, key, key_len, NULL, 0);
    tiny_editor_buffer_uint(buffer, value, suffix, suffix_len);
    return;
}

static void
tiny_editor_buffer_uint(NcBuffer *buffer, uint32 value, char *suffix,
                        int32 suffix_len) {
    char number[64];
    int32 len = SNPRINTF(number, "%u", value);
    nc_buffer_append_data(buffer, number, len);
    nc_buffer_append_data(buffer, suffix, suffix_len);
    return;
}

static int32
tiny_editor_channels_to_string(int32 channels, char *buffer,
                               int32 buffer_cap) {
    return ncm_channels_to_string(channels, buffer, buffer_cap);
}

#endif /* NCMPCPP_NC_TINY_TAG_EDITOR_C */
