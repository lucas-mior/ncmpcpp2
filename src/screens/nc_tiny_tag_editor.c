#if !defined(NCMPCPP_NC_TINY_TAG_EDITOR_C)
#define NCMPCPP_NC_TINY_TAG_EDITOR_C

#include "cbase.h"

#include "app_controller.h"
#include "c/ncm_c.h"
#include "screens/nc_screens.h"
#include "settings.h"
#include "title.h"
#include "ui_state.h"

static bool tiny_editor_can_run_current(NcScreen *screen);
static int32 tiny_editor_run_current(NcScreen *screen);
static void tiny_editor_display(TinyTagEditorScreen *screen);
static void tiny_editor_switch_to(NcScreen *screen);
static void tiny_editor_resize(NcScreen *screen);
static char *tiny_editor_title(NcScreen *screen);
static void tiny_editor_update(NcScreen *screen);
static void tiny_editor_mouse_callback(NcScreen *screen, MEVENT event);

#define NC_SCREEN_IMPL_TYPE TinyTagEditorScreen
#define NC_SCREEN_IMPL_PREFIX tiny_editor
#define NC_SCREEN_IMPL_PUBLIC_PREFIX tiny_tag_editor_screen
#define NC_SCREEN_IMPL_BASE_FIELD screen
#define NC_SCREEN_IMPL_WINDOW_FIELD window
#define NC_SCREEN_IMPL_MENU(screen) nc_editor_buffer_menu_base(&(screen)->rows)
#define NC_SCREEN_IMPL_SCROLL_HEIGHT(screen) ((screen)->main_height)
#define NC_SCREEN_IMPL_REFRESH_CALLBACK tiny_editor_display
#define NC_SCREEN_IMPL_CAN_RUN_CURRENT_CALLBACK tiny_editor_can_run_current
#define NC_SCREEN_IMPL_RUN_CURRENT_CALLBACK tiny_editor_run_current
#define NC_SCREEN_IMPL_SWITCH_TO_CALLBACK tiny_editor_switch_to
#define NC_SCREEN_IMPL_RESIZE_CALLBACK tiny_editor_resize
#define NC_SCREEN_IMPL_TITLE_CALLBACK tiny_editor_title
#define NC_SCREEN_IMPL_UPDATE_CALLBACK tiny_editor_update
#define NC_SCREEN_IMPL_MOUSE_CALLBACK tiny_editor_mouse_callback
#define NC_SCREEN_IMPL_DESTROY_TYPED_CALLBACK \
    tiny_tag_editor_screen_destroy
#define NC_SCREEN_IMPL_MERGABLE true
#include "screens/nc_screen_impl_template.h"

static void
tiny_editor_draw_row(NcMenu *menu, NcWindow *window, void *item,
                     int32 pos, void *user) {
    NcBuffer *buffer;
    NcBufferProperty *properties;
    int32 property_count;
    int32 property_index;

    (void)menu;
    (void)pos;
    (void)user;
    buffer = item;

    properties = nc_buffer_properties(buffer);
    property_count = ARRAY_LEN(buffer->properties);
    property_index = 0;
    for (int32 i = 0;; i += 1) {
        while ((property_index < property_count)
               && (properties[property_index].position == i)) {
            nc_buffer_apply_property(window, &properties[property_index]);
            property_index += 1;
        }
        if (i >= buffer->len) {
            break;
        }
        nc_window_print_char(window, nc_buffer_data(buffer)[i]);
    }
    return;
}

void
tiny_tag_editor_screen_init(
    TinyTagEditorScreen *screen, int32 start_x, int32 width,
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
    nc_menu_set_cyclic_scrolling(menu, Config.cyclic_scrolling);
    nc_menu_set_centered_cursor(menu, Config.centered_cursor);

    nc_window_init(&screen->window, start_x, main_start_y, width,
                   main_height, NULL, 0, color, border);
    screen->hooks = (TinyTagEditorHooks){0};
    screen->edited = (NcmMutableSong){0};

    screen->music_dir = (StrBuilder){0};
    screen->tag_separator = (StrBuilder){0};

    screen->previous_screen = NULL;
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    screen->has_edited = false;
    screen->show_duplicate_tags = false;
    screen->registered = false;

    nc_screen_init_ops(&screen->screen, tiny_editor_ops, screen,
                       NC_SCREEN_TYPE_TINY_TAG_EDITOR);
    return;
}

void
tiny_tag_editor_screen_destroy(TinyTagEditorScreen *screen) {
    app_controller_unregister_screen(tiny_tag_editor_screen_base(screen));
    ncm_mutable_song_destroy(&screen->edited);
    sb_free(&screen->music_dir);
    sb_free(&screen->tag_separator);
    nc_window_destroy(&screen->window);

    nc_editor_buffer_menu_destroy(&screen->rows);

    screen->previous_screen = NULL;
    screen->has_edited = false;
    screen->show_duplicate_tags = false;
    screen->registered = false;

    return;
}

void
tiny_tag_editor_screen_set_hooks(
    TinyTagEditorScreen *screen, TinyTagEditorHooks hooks
) {
    if (screen == NULL) {
        return;
    }
    screen->hooks = hooks;
    return;
}

NcEditorBufferMenu *
tiny_tag_editor_screen_rows(TinyTagEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->rows;
}

static void
tiny_editor_buffer_key_value(NcBuffer *buffer, char *key, int32 key_len,
                             char *value, int32 value_len) {
    nc_buffer_append_data(buffer, key, key_len);
    nc_buffer_append_data(buffer, STRLIT(": "));
    if (value_len > 0) {
        nc_buffer_append_data(buffer, value, value_len);
    }
    return;
}

static void
tiny_editor_add_row(TinyTagEditorScreen *screen, NcBuffer *buffer,
                    uint32 flags) {
    nc_editor_buffer_menu_add_with_flags(&screen->rows, buffer, flags);
    return;
}

static void
tiny_editor_buffer_key_uint(NcBuffer *buffer, char *key, int32 key_len,
                            uint32 value, char *suffix, int32 suffix_len) {
    char number[64];
    int32 len;

    tiny_editor_buffer_key_value(buffer, key, key_len, NULL, 0);
    len = SNPRINTF(number, "%u", value);
    nc_buffer_append_data(buffer, number, len);
    nc_buffer_append_data(buffer, suffix, suffix_len);
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
    int32 name_len;

    name_len = NCM_TAGS_FIELD_alias_len(field, &name);
    tiny_editor_buffer_key_value(buffer, name, name_len, NULL, 0);
    value = ncm_mutable_song_tags_buffer(song, field,
                                         tag_separator, tag_separator_len,
                                         show_duplicate_tags);
    nc_buffer_append_data(buffer, value.data, value.len);
    sb_free(&value);
    return;
}

enum TinyTagEditorOpenResult
tiny_tag_editor_screen_open_song(
    TinyTagEditorScreen *screen, NcmSong *song,
    char *music_dir, int32 music_dir_len, char *tag_separator,
    int32 tag_separator_len, bool show_duplicate_tags, StrBuilder *path
) {
    NcmMutableSong edited = {0};
    NcmTaglibAudioProperties properties = {0};
    NcmTaglibFile file;
    NcBuffer row;
    char channel_buffer[32];
    char duration_buffer[32];
    bool extended_tags_supported;
    int32 channel_len;
    int32 duration_len;
    int32 status;

    if (path) {
        sb_clear(path);
    }
    if ((screen == NULL) || (song == NULL) || (path == NULL)
        || (music_dir_len < 0) || (tag_separator_len < 0)
        || ((music_dir_len > 0) && (music_dir == NULL))
        || ((tag_separator_len > 0) && (tag_separator == NULL))) {
        return TINY_TAG_EDITOR_OPEN_INVALID_ARGUMENT;
    }
    if (ncm_song_is_stream(song)) {
        return TINY_TAG_EDITOR_OPEN_STREAM;
    }
    if (ncm_song_is_from_database(song) && (music_dir_len <= 0)) {
        return TINY_TAG_EDITOR_OPEN_MISSING_MUSIC_DIRECTORY;
    }
    sb_set(&screen->music_dir, music_dir, music_dir_len);
    sb_set(&screen->tag_separator, tag_separator, tag_separator_len);
    screen->show_duplicate_tags = show_duplicate_tags;

    status = ncm_mutable_song_load_originals_from_song(&edited, song);
    if (status < 0) {
        ncm_mutable_song_destroy(&edited);
        return TINY_TAG_EDITOR_OPEN_PREPARE_FAILED;
    }
    edited.duration = ncm_song_duration(song);
    edited.mtime = (int32)ncm_song_mtime(song);
    ncm_mutable_song_move(&screen->edited, &edited);
    screen->has_edited = true;

    if (screen->edited.is_from_database) {
        SB_APPEND(path, music_dir, music_dir_len);
    }
    SB_APPEND(path, screen->edited.uri, screen->edited.uri_len);

    file = (NcmTaglibFile){0};
    if (screen->hooks.taglib_open) {
        status = screen->hooks.taglib_open(
            screen->hooks.user, &file, path->data, path->len);
    } else {
        status = ncm_taglib_file_open(&file, path->data);
    }
    if (status < 0) {
        if (screen->hooks.taglib_close) {
            screen->hooks.taglib_close(screen->hooks.user, &file);
        } else {
            ncm_taglib_file_close(&file);
        }
        screen->has_edited = false;
        nc_menu_clear_items(nc_editor_buffer_menu_base(&screen->rows));
        return TINY_TAG_EDITOR_OPEN_UNREADABLE_FILE;
    }

    if (screen->hooks.taglib_audio_properties) {
        screen->hooks.taglib_audio_properties(
            screen->hooks.user, &file, &properties);
    } else {
        ncm_taglib_file_audio_properties(&file, &properties);
    }
    if (screen->hooks.taglib_file_can_set_extended_tags) {
        extended_tags_supported =
            screen->hooks.taglib_file_can_set_extended_tags(
                screen->hooks.user, &file);
    } else {
        extended_tags_supported = ncm_taglib_file_can_set_extended_tags(&file);
    }

    screen->show_duplicate_tags = show_duplicate_tags;
    nc_menu_clear_items(nc_editor_buffer_menu_base(&screen->rows));

    row = (NcBuffer){0};
    tiny_editor_buffer_key_value(&row, STRLIT("Filename"),
                                 screen->edited.name,
                                 screen->edited.name_len);
    tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE);
    nc_buffer_clear(&row);

    tiny_editor_buffer_key_value(&row, STRLIT("Directory"),
                                 screen->edited.directory,
                                 screen->edited.directory_len);
    tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE);
    nc_buffer_clear(&row);

    tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE);
    nc_buffer_clear(&row);

    if (screen->edited.duration > 0) {
        duration_len = ncm_song_show_time(
            screen->edited.duration, duration_buffer, SIZEOF(duration_buffer));
    } else {
        duration_buffer[0] = '-';
        duration_buffer[1] = ':';
        duration_buffer[2] = '-';
        duration_buffer[3] = '-';
        duration_len = 4;
    }
    tiny_editor_buffer_key_value(&row, STRLIT("Length"),
                                 duration_buffer, duration_len);
    tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE);
    nc_buffer_clear(&row);

    tiny_editor_buffer_key_uint(
        &row, STRLIT("Bitrate"), (uint32)properties.bitrate,
        STRLIT(" kbps"));
    tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE);
    nc_buffer_clear(&row);

    tiny_editor_buffer_key_uint(
        &row, STRLIT("Sample rate"), (uint32)properties.sample_rate,
        STRLIT(" Hz"));
    tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE);
    nc_buffer_clear(&row);

    channel_len = ncm_channels_to_string(
        properties.channels, channel_buffer, SIZEOF(channel_buffer));
    tiny_editor_buffer_key_value(&row, STRLIT("Channels"),
                                 channel_buffer, channel_len);
    tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE);
    nc_buffer_destroy(&row);
    nc_editor_buffer_menu_add_separator(&screen->rows);

    for (uint32 field = 0; field < NCM_TAGS_FIELD_COUNT; field += 1) {
        bool inactive;

        row = (NcBuffer){0};
        tiny_editor_buffer_mutable_tag(
            &row, &screen->edited, (enum NcmTagsField)field,
            tag_separator, tag_separator_len, show_duplicate_tags);
        inactive = !extended_tags_supported
                   && ((field == NCM_TAGS_FIELD_ALBUM_ARTIST)
                       || (field == NCM_TAGS_FIELD_COMPOSER)
                       || (field == NCM_TAGS_FIELD_PERFORMER)
                       || (field == NCM_TAGS_FIELD_DISC));
        if (inactive) {
            tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE);
        } else {
            tiny_editor_add_row(screen, &row, NC_MENU_ITEM_SELECTABLE);
        }
        nc_buffer_destroy(&row);
    }

    nc_editor_buffer_menu_add_separator(&screen->rows);
    row = (NcBuffer){0};
    tiny_editor_buffer_key_value(&row, STRLIT("Filename"),
                                 screen->edited.name,
                                 screen->edited.name_len);
    tiny_editor_add_row(screen, &row, NC_MENU_ITEM_SELECTABLE);
    nc_buffer_destroy(&row);

    nc_editor_buffer_menu_add_separator(&screen->rows);
    row = (NcBuffer){0};
    nc_buffer_append_data(&row, STRLIT("Save"));
    tiny_editor_add_row(screen, &row, NC_MENU_ITEM_SELECTABLE);
    nc_buffer_clear(&row);

    nc_buffer_append_data(&row, STRLIT("Cancel"));
    tiny_editor_add_row(screen, &row, NC_MENU_ITEM_SELECTABLE);
    nc_buffer_destroy(&row);

    nc_menu_highlight_position(nc_editor_buffer_menu_base(&screen->rows),
                               TINY_TAG_EDITOR_FIRST_TAG_ROW,
                               screen->main_height);
    if (screen->hooks.taglib_close) {
        screen->hooks.taglib_close(screen->hooks.user, &file);
    } else {
        ncm_taglib_file_close(&file);
    }
    return TINY_TAG_EDITOR_OPEN_SUCCESS;
}

static void
tiny_editor_status_message(
    TinyTagEditorScreen *screen, char *message, int32 message_len
) {
    if (screen->hooks.status_message) {
        screen->hooks.status_message(screen->hooks.user, message,
                                     message_len);
    }
    return;
}

static int32
tiny_editor_finish(TinyTagEditorScreen *screen) {
    NcScreen *previous;

    previous = screen->previous_screen;
    if (previous == NULL) {
        return 0;
    }
    if (screen->hooks.switch_to_screen) {
        screen->hooks.switch_to_screen(screen->hooks.user, previous);
        return 0;
    }
    return nc_screen_switcher_switch_to(
        previous, previous->has_to_be_resized);
}

static int32
tiny_editor_run_row(TinyTagEditorScreen *screen, int32 row) {
    enum TinyTagEditorPromptResult prompt_result;
    enum NcmTagsField field;
    NcmStringView initial;
    char *field_name;
    int32 field_name_len;
    NcmStringView current_name;
    StrBuilder input = {0};
    StrBuilder tag_value;
    NcMenu *menu;
    char error_buffer[256];
    int32 error_len;
    int32 status;
    int32 dot;

    menu = nc_editor_buffer_menu_base(&screen->rows);

    if ((row >= (int32)TINY_TAG_EDITOR_FIRST_TAG_ROW)
        && (row <= (int32)TINY_TAG_EDITOR_LAST_TAG_ROW)) {
        NcBuffer row_buffer = {0};

        field = (enum NcmTagsField)(
            row - (int32)TINY_TAG_EDITOR_FIRST_TAG_ROW);
        tag_value = ncm_mutable_song_tags_buffer(
            &screen->edited, field, screen->tag_separator.data,
            screen->tag_separator.len, screen->show_duplicate_tags);
        initial.data = tag_value.data;
        initial.len = tag_value.len;
        field_name_len = NCM_TAGS_FIELD_alias_len(field, &field_name);
        if (screen->hooks.prompt == NULL) {
            prompt_result = TINY_TAG_EDITOR_PROMPT_ERROR;
        } else {
            prompt_result = screen->hooks.prompt(
                screen->hooks.user, field_name, field_name_len, initial,
                &input);
        }
        sb_free(&tag_value);
        if (prompt_result == TINY_TAG_EDITOR_PROMPT_ABORTED) {
            tiny_editor_status_message(
                screen, STRLIT("Action aborted"));
            sb_free(&input);
            return -NCM_ERROR_CANCELLED;
        }
        if (prompt_result != TINY_TAG_EDITOR_PROMPT_ACCEPTED) {
            sb_free(&input);
            return -NCM_ERROR_UNAVAILABLE;
        }

        ncm_mutable_song_set_tags(
            &screen->edited, field, sb_opt_cstr(&input), input.len,
            screen->tag_separator.data, screen->tag_separator.len);
        sb_free(&input);

        tiny_editor_buffer_mutable_tag(
            &row_buffer, &screen->edited, field,
            screen->tag_separator.data, screen->tag_separator.len,
            screen->show_duplicate_tags);
        nc_menu_replace_item(menu, NC_MENU_ITEMS_ALL,
                             TINY_TAG_EDITOR_TAG_ROW(field), &row_buffer);
        nc_buffer_destroy(&row_buffer);
        return 0;
    }

    if (row == TINY_TAG_EDITOR_FILE_NAME_EDIT_ROW) {
        NcmStringView name;
        NcBuffer row_buffer = {0};
        StrBuilder new_name = {0};

        if (!ncm_mutable_song_has_new_name_view(&screen->edited,
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

        if (screen->hooks.prompt == NULL) {
            prompt_result = TINY_TAG_EDITOR_PROMPT_ERROR;
        } else {
            prompt_result = screen->hooks.prompt(
                screen->hooks.user, STRLIT("Filename"), initial,
                &input);
        }
        if (prompt_result == TINY_TAG_EDITOR_PROMPT_ABORTED) {
            tiny_editor_status_message(
                screen, STRLIT("Action aborted"));
            sb_free(&input);
            return -NCM_ERROR_CANCELLED;
        }
        if (prompt_result != TINY_TAG_EDITOR_PROMPT_ACCEPTED) {
            sb_free(&input);
            return -NCM_ERROR_UNAVAILABLE;
        }
        if (input.len <= 0) {
            sb_free(&input);
            return 0;
        }

        if (!ncm_mutable_song_has_new_name_view(&screen->edited,
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

        SB_APPEND(&new_name, input.data, input.len);
        if (dot >= 0) {
            SB_APPEND(&new_name, &current_name.data[dot],
                      current_name.len - dot);
        }
        ncm_mutable_song_set_new_name(&screen->edited,
                                      new_name.data, new_name.len);
        sb_free(&new_name);
        sb_free(&input);

        if (!ncm_mutable_song_has_new_name_view(&screen->edited, &name)) {
            name.data = screen->edited.name;
            name.len = screen->edited.name_len;
        }
        tiny_editor_buffer_key_value(
            &row_buffer, STRLIT("Filename"), name.data, name.len);
        nc_menu_replace_item(menu, NC_MENU_ITEMS_ALL,
                             TINY_TAG_EDITOR_FILE_NAME_EDIT_ROW,
                             &row_buffer);
        nc_buffer_destroy(&row_buffer);
        return 0;
    }

    if (row == TINY_TAG_EDITOR_SAVE_ROW) {
        int32 previous_type;

        tiny_editor_status_message(
            screen, STRLIT("Updating tags..."));
        if (screen->hooks.write_song) {
            status = screen->hooks.write_song(
                screen->hooks.user, &screen->edited,
                screen->music_dir.data);
        } else {
            status = ncm_mutable_song_write(
                &screen->edited, screen->music_dir.data);
        }
        if (status < 0) {
            error_len = SNPRINTF(error_buffer, "Error while writing tags: %s",
                                 strerror(errno));
            tiny_editor_status_message(screen, error_buffer, error_len);
            tiny_editor_finish(screen);
            return status;
        }

        tiny_editor_status_message(screen, STRLIT("Tags updated"));
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
        return tiny_editor_finish(screen);
    }

    if (row == TINY_TAG_EDITOR_CANCEL_ROW) {
        return tiny_editor_finish(screen);
    }
    return -NCM_ERROR_UNAVAILABLE;
}

static int32
tiny_editor_current_row(TinyTagEditorScreen *screen) {
    return nc_menu_highlight(nc_editor_buffer_menu_base(&screen->rows));
}

static bool
tiny_editor_action_runnable(TinyTagEditorScreen *screen) {
    NcMenu *menu;
    int32 row;

    if (!screen->has_edited) {
        return false;
    }
    menu = nc_editor_buffer_menu_base(&screen->rows);
    if (nc_menu_is_empty(menu)) {
        return false;
    }
    row = tiny_editor_current_row(screen);
    return (row >= 0) && (row < nc_menu_all_item_count(menu))
           && nc_menu_position_is_selectable(menu, row);
}

int32
tiny_tag_editor_screen_run_row(TinyTagEditorScreen *screen, int32 row) {
    NcMenu *menu;

    if (screen == NULL) {
        return -EINVAL;
    }
    if (!screen->has_edited) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    menu = nc_editor_buffer_menu_base(&screen->rows);
    if ((row < 0) || (row >= nc_menu_all_item_count(menu))
        || !nc_menu_position_is_selectable(menu, row)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    return tiny_editor_run_row(screen, row);
}

int32
tiny_tag_editor_screen_run_current(TinyTagEditorScreen *screen) {
    if (screen == NULL) {
        return -EINVAL;
    }
    if (!tiny_editor_action_runnable(screen)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    return tiny_editor_run_row(screen, tiny_editor_current_row(screen));
}

bool
tiny_tag_editor_screen_action_runnable(TinyTagEditorScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return tiny_editor_action_runnable(screen);
}

static bool
tiny_editor_can_run_current(NcScreen *screen) {
    return tiny_editor_action_runnable(tiny_editor_from_screen(screen));
}

static int32
tiny_editor_run_current(NcScreen *screen) {
    TinyTagEditorScreen *editor;

    editor = tiny_editor_from_screen(screen);
    if (!tiny_editor_action_runnable(editor)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    return tiny_editor_run_row(editor, tiny_editor_current_row(editor));
}

static void
tiny_editor_display(TinyTagEditorScreen *editor) {
    NcMenu *menu;

    menu = nc_editor_buffer_menu_base(&editor->rows);
    nc_menu_prepare_refresh(menu, editor->main_height, NULL, NULL);
    nc_window_display(&editor->window);
    nc_menu_refresh(menu, &editor->window, editor->width,
                    editor->main_height);

    return;
}

static void
tiny_editor_switch_to(NcScreen *screen) {
    TinyTagEditorScreen *editor;

    editor = tiny_editor_from_screen(screen);
    editor->previous_screen = app_controller_previous_screen();
    ncm_title_draw_header(STRLIT("Tiny tag editor"));
    return;
}

static void
tiny_editor_resize(NcScreen *screen) {
    int32 start_x;
    int32 width;
    TinyTagEditorScreen *editor = tiny_editor_from_screen(screen);

    nc_screen_switcher_get_resize_params(screen, &start_x, &width, true);
    editor->start_x = start_x;
    editor->width = width;
    editor->main_start_y = ui_state_main_start_y();
    editor->main_height = ui_state_main_height();

    nc_window_move_to(&editor->window, editor->start_x, editor->main_start_y);
    nc_window_resize(&editor->window, editor->width, editor->main_height);
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
    TinyTagEditorScreen *editor;
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
        if (nc_menu_goto_selectable(menu, y) < 0) {
            return;
        }
        if (event.bstate & BUTTON3_PRESSED) {
            tiny_editor_refresh_window(screen);
            tiny_editor_run_current(screen);
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

#endif /* NCMPCPP_NC_TINY_TAG_EDITOR_C */
