#include "screens/nc_tiny_tag_editor.h"

#include <stdio.h>
#include <string.h>

#include "app_controller.h"
#include "c/ncm_base.h"
#include "c/ncm_string.h"
#include "c/ncm_type_conversions.h"
#include "cbase/base_macros.h"

static NativeTinyTagEditorScreen *tiny_editor_from_screen(NcScreen *screen);
static NcWindow *tiny_editor_active_window(NcScreen *screen);
static void tiny_editor_refresh(NcScreen *screen);
static void tiny_editor_refresh_window(NcScreen *screen);
static void tiny_editor_scroll(NcScreen *screen, enum NcScroll where);
static bool tiny_editor_can_run_current(NcScreen *screen);
static bool tiny_editor_run_current(NcScreen *screen);
static void tiny_editor_switch_to(NcScreen *screen);
static void tiny_editor_resize(NcScreen *screen);
static char *tiny_editor_title(NcScreen *screen);
static void tiny_editor_update(NcScreen *screen);
static void tiny_editor_mouse_callback(NcScreen *screen, MEVENT event);
static bool tiny_editor_is_lockable(NcScreen *screen);
static bool tiny_editor_is_mergable(NcScreen *screen);
static void tiny_editor_destroy_callback(NcScreen *screen);
static bool tiny_editor_add_row(NativeTinyTagEditorScreen *screen,
                                NcBuffer *buffer, uint32 flags);
static void tiny_editor_buffer_key_value(NcBuffer *buffer, char *key,
                                         int32 key_len, char *value,
                                         int32 value_len);
static void tiny_editor_buffer_mutable_tag(NcBuffer *buffer,
                                           NcmMutableSong *song,
                                           enum NcmTagsField field);
static void tiny_editor_buffer_uint(NcBuffer *buffer, uint32 value,
                                    char *suffix, int32 suffix_len);
static int32 tiny_editor_channels_to_string(int32 channels,
                                            char *buffer,
                                            int32 buffer_cap);

static NcScreenCallbacks tiny_editor_callbacks = {
    .active_window = tiny_editor_active_window,
    .refresh = tiny_editor_refresh,
    .refresh_window = tiny_editor_refresh_window,
    .scroll = tiny_editor_scroll,
    .can_run_current = tiny_editor_can_run_current,
    .run_current = tiny_editor_run_current,
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
    NativeTinyTagEditorScreen *screen, int64 start_x, int64 width,
    int64 main_start_y, int64 main_height, NcColor color, NcBorder border) {
    nc_editor_buffer_menu_init(&screen->rows);
    nc_window_init(&screen->window, start_x, main_start_y, width,
                   main_height, NULL, 0, color, border);
    screen->bridge = (NativeTinyTagEditorBridge){0};
    ncm_mutable_song_init(&screen->edited);
    screen->previous_screen = NULL;
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    screen->has_edited = false;
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
    nc_window_destroy(&screen->window);
    nc_editor_buffer_menu_destroy(&screen->rows);
    screen->previous_screen = NULL;
    screen->has_edited = false;
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
native_tiny_tag_editor_screen_set_bridge(
    NativeTinyTagEditorScreen *screen, NativeTinyTagEditorBridge bridge) {
    if (screen == NULL) {
        return;
    }
    screen->bridge = bridge;
    return;
}

NcEditorBufferMenu *
native_tiny_tag_editor_screen_rows(NativeTinyTagEditorScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->rows;
}

NcmMutableSong *
native_tiny_tag_editor_screen_edited(NativeTinyTagEditorScreen *screen) {
    if (screen == NULL || !screen->has_edited) {
        return NULL;
    }
    return &screen->edited;
}

void
native_tiny_tag_editor_screen_set_geometry(
    NativeTinyTagEditorScreen *screen, int64 start_x, int64 width,
    int64 main_start_y, int64 main_height) {
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
    NativeTinyTagEditorScreen *screen, NcmSong *song) {
    if (screen == NULL || song == NULL) {
        return false;
    }
    if (!ncm_mutable_song_load_originals_from_song(&screen->edited, song)) {
        return false;
    }
    screen->has_edited = true;
    return true;
}

bool
native_tiny_tag_editor_screen_set_edited_mutable_song(
    NativeTinyTagEditorScreen *screen, NcmMutableSong *song) {
    if (screen == NULL || song == NULL) {
        return false;
    }
    if (!ncm_mutable_song_copy(&screen->edited, song)) {
        return false;
    }
    screen->has_edited = true;
    return true;
}

bool
native_tiny_tag_editor_screen_reload_rows(
    NativeTinyTagEditorScreen *screen,
    NcmTaglibAudioProperties *properties,
    bool extended_tags_supported) {
    NcBuffer row;
    char channel_buffer[32];
    int32 channel_len;

    if (screen == NULL || !screen->has_edited) {
        return false;
    }
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
    tiny_editor_buffer_key_value(&row, STRLIT_ARGS("URI"),
                                 screen->edited.uri,
                                 screen->edited.uri_len);
    if (!tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE)) {
        nc_buffer_destroy(&row);
        return false;
    }
    nc_buffer_clear(&row);
    tiny_editor_buffer_uint(&row, ncm_mutable_song_duration(&screen->edited),
                            STRLIT_ARGS(" s"));
    if (!tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE)) {
        nc_buffer_destroy(&row);
        return false;
    }
    nc_buffer_clear(&row);
    if (properties != NULL) {
        tiny_editor_buffer_uint(&row, (uint32)properties->bitrate,
                                STRLIT_ARGS(" kbps"));
    }
    if (!tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE)) {
        nc_buffer_destroy(&row);
        return false;
    }
    nc_buffer_clear(&row);
    if (properties != NULL) {
        tiny_editor_buffer_uint(&row, (uint32)properties->sample_rate,
                                STRLIT_ARGS(" Hz"));
    }
    if (!tiny_editor_add_row(screen, &row, NC_MENU_ITEM_INACTIVE)) {
        nc_buffer_destroy(&row);
        return false;
    }
    nc_buffer_clear(&row);
    channel_len = 0;
    if (properties != NULL) {
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
        tiny_editor_buffer_mutable_tag(&row, &screen->edited,
                                       (enum NcmTagsField)field);
        inactive = !extended_tags_supported
                   && ((field == NCM_TAGS_FIELD_ALBUM_ARTIST)
                       || (field == NCM_TAGS_FIELD_COMPOSER)
                       || (field == NCM_TAGS_FIELD_PERFORMER));
        if (!tiny_editor_add_row(&screen[0], &row,
                                 inactive ? NC_MENU_ITEM_INACTIVE
                                          : NC_MENU_ITEM_SELECTABLE)) {
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
                               8, screen->main_height);
    return true;
}

bool
native_tiny_tag_editor_screen_set_tag_value(
    NativeTinyTagEditorScreen *screen, enum NcmTagsField field,
    char *value, int32 value_len, char *separator, int32 separator_len) {
    if (screen == NULL || !screen->has_edited) {
        return false;
    }
    return ncm_mutable_song_set_tags(&screen->edited, field, value,
                                     value_len, separator, separator_len);
}

bool
native_tiny_tag_editor_screen_set_filename(
    NativeTinyTagEditorScreen *screen, char *name, int32 name_len) {
    if (screen == NULL || !screen->has_edited) {
        return false;
    }
    return ncm_mutable_song_set_new_name(&screen->edited, name, name_len);
}

bool
native_tiny_tag_editor_screen_save(NativeTinyTagEditorScreen *screen,
                                   char *music_dir) {
    if (screen == NULL || !screen->has_edited) {
        return false;
    }
    if (!ncm_mutable_song_write(&screen->edited, music_dir)) {
        return false;
    }
    ncm_mutable_song_clear_modifications(&screen->edited);
    return true;
}

bool
native_tiny_tag_editor_screen_action_runnable(
    NativeTinyTagEditorScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return !nc_menu_empty(nc_editor_buffer_menu_base(&screen->rows));
}

static NativeTinyTagEditorScreen *
tiny_editor_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcWindow *
tiny_editor_active_window(NcScreen *screen) {
    NativeTinyTagEditorScreen *editor;

    editor = tiny_editor_from_screen(screen);
    if (editor->bridge.active_window != NULL) {
        return editor->bridge.active_window(editor->bridge.user);
    }
    return &editor->window;
}

static void
tiny_editor_refresh(NcScreen *screen) {
    NativeTinyTagEditorScreen *editor;
    NcMenu *menu;

    editor = tiny_editor_from_screen(screen);
    if (editor->bridge.refresh != NULL) {
        editor->bridge.refresh(editor->bridge.user);
        return;
    }
    menu = nc_editor_buffer_menu_base(&editor->rows);
    nc_menu_prepare_refresh(menu, editor->main_height, NULL, NULL);
    nc_window_display(&editor->window);
    nc_menu_refresh(menu, &editor->window, editor->width,
                    editor->main_height);
    return;
}


static void
tiny_editor_refresh_window(NcScreen *screen) {
    NativeTinyTagEditorScreen *editor;

    editor = tiny_editor_from_screen(screen);
    if (editor->bridge.refresh_window != NULL) {
        editor->bridge.refresh_window(editor->bridge.user);
        return;
    }
    tiny_editor_refresh(screen);
    return;
}

static void
tiny_editor_scroll(NcScreen *screen, enum NcScroll where) {
    NativeTinyTagEditorScreen *editor;

    editor = tiny_editor_from_screen(screen);
    if (editor->bridge.scroll != NULL) {
        editor->bridge.scroll(editor->bridge.user, where);
        return;
    }
    nc_menu_scroll_selectable(nc_editor_buffer_menu_base(&editor->rows),
                              editor->main_height, where);
    return;
}

static bool
tiny_editor_can_run_current(NcScreen *screen) {
    NativeTinyTagEditorScreen *editor;

    editor = tiny_editor_from_screen(screen);
    if (editor == NULL || editor->bridge.run_action == NULL) {
        return false;
    }
    if (editor->bridge.action_runnable != NULL) {
        return editor->bridge.action_runnable(editor->bridge.user);
    }
    return native_tiny_tag_editor_screen_action_runnable(editor);
}

static bool
tiny_editor_run_current(NcScreen *screen) {
    NativeTinyTagEditorScreen *editor;

    if (!tiny_editor_can_run_current(screen)) {
        return false;
    }
    editor = tiny_editor_from_screen(screen);
    return editor->bridge.run_action(editor->bridge.user);
}

static void
tiny_editor_switch_to(NcScreen *screen) {
    NativeTinyTagEditorScreen *editor;

    editor = tiny_editor_from_screen(screen);
    editor->previous_screen = app_controller_previous_screen();
    if (editor->bridge.switch_to != NULL) {
        editor->bridge.switch_to(editor->bridge.user);
    }
    return;
}

static void
tiny_editor_resize(NcScreen *screen) {
    NativeTinyTagEditorScreen *editor;
    NcScreenResizeParams params;

    editor = tiny_editor_from_screen(screen);
    if (editor->bridge.resize != NULL) {
        editor->bridge.resize(editor->bridge.user);
        nc_screen_clear_resize_request(screen);
        return;
    }
    params = nc_screen_resize_params(screen);
    native_tiny_tag_editor_screen_set_geometry(editor, params.x_offset,
                                               params.width,
                                               editor->main_start_y,
                                               editor->main_height);
    nc_screen_clear_resize_request(screen);
    return;
}

static char *
tiny_editor_title(NcScreen *screen) {
    NativeTinyTagEditorScreen *editor;

    editor = tiny_editor_from_screen(screen);
    if (editor->bridge.title != NULL) {
        return editor->bridge.title(editor->bridge.user);
    }
    return (char *)"Tiny tag editor";
}

static void
tiny_editor_update(NcScreen *screen) {
    NativeTinyTagEditorScreen *editor;

    editor = tiny_editor_from_screen(screen);
    if (editor->bridge.update != NULL) {
        editor->bridge.update(editor->bridge.user);
    }
    nc_screen_clear_update_request(screen);
    return;
}


static void
tiny_editor_mouse_callback(NcScreen *screen, MEVENT event) {
    NativeTinyTagEditorScreen *editor;

    editor = tiny_editor_from_screen(screen);
    if (editor->bridge.mouse_button_pressed != NULL) {
        editor->bridge.mouse_button_pressed(editor->bridge.user, event);
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
    if (screen == NULL || buffer == NULL) {
        return false;
    }
    nc_editor_buffer_menu_add_with_flags(&screen->rows, buffer, flags);
    return true;
}

static void
tiny_editor_buffer_key_value(NcBuffer *buffer, char *key, int32 key_len,
                             char *value, int32 value_len) {
    nc_buffer_append_data(buffer, key, key_len);
    nc_buffer_append_data(buffer, STRLIT_ARGS(": "));
    if (value != NULL && value_len > 0) {
        nc_buffer_append_data(buffer, value, value_len);
    }
    return;
}

static void
tiny_editor_buffer_mutable_tag(NcBuffer *buffer, NcmMutableSong *song,
                               enum NcmTagsField field) {
    NcmBuffer value;
    char *name;

    name = ncm_tags_field_name(field);
    tiny_editor_buffer_key_value(buffer, name, (int32)strlen(name), NULL, 0);
    value = ncm_mutable_song_tags_buffer(song, field, STRLIT_ARGS(", "),
                                         false);
    nc_buffer_append_data(buffer, value.data, value.len);
    ncm_buffer_destroy(&value);
    return;
}

static void
tiny_editor_buffer_uint(NcBuffer *buffer, uint32 value, char *suffix,
                        int32 suffix_len) {
    char number[64];
    int32 len;

    len = snprintf(number, (size_t)SIZEOF(number), "%u", value);
    if (len < 0) {
        return;
    }
    if (len >= (int32)SIZEOF(number)) {
        len = (int32)SIZEOF(number) - 1;
    }
    nc_buffer_append_data(buffer, number, len);
    nc_buffer_append_data(buffer, suffix, suffix_len);
    return;
}

static int32
tiny_editor_channels_to_string(int32 channels, char *buffer,
                               int32 buffer_cap) {
    return ncm_channels_to_string(channels, buffer, buffer_cap);
}
