#include "screens/native_c_screens.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>

#include "actions.h"
#include "app_controller.h"
#include "bindings.h"
#include "c/ncm_base.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_string.h"
#include "global.h"
#include "settings.h"
#include "screens/screen_switcher.h"
#include "screens/song_info.h"
#include "statusbar.h"
#include "title.h"
#include "ui_state.h"

#if defined(HAVE_TAGLIB_H)
# include "c/ncm_taglib.h"
# include "c/ncm_tags.h"
#endif

struct NativeHelpScreen {
    NcHelpScreen screen;
    bool initialized;
};

struct NativeOutputsScreen {
    NcOutputsScreen screen;
    bool initialized;
};


struct NativeServerInfoScreen {
    NcServerInfoScreen screen;
    NcmMpdStringList url_handlers;
    NcmMpdStringList tag_types;
    NcmTimePoint timer;
    bool initialized;
};

struct NativeSongInfoScreen {
    NcSongInfoScreen screen;
    NcmSong song;
    bool has_song;
    bool initialized;
};

NcmSongInfoMetadata ncm_song_info_tags[] = {
    {(char *)"Title", NCM_SONG_GETTER_TITLE, NCM_TAGS_FIELD_TITLE},
    {(char *)"Artist", NCM_SONG_GETTER_ARTIST, NCM_TAGS_FIELD_ARTIST},
    {(char *)"Album Artist",
     NCM_SONG_GETTER_ALBUM_ARTIST,
     NCM_TAGS_FIELD_ALBUM_ARTIST},
    {(char *)"Album", NCM_SONG_GETTER_ALBUM, NCM_TAGS_FIELD_ALBUM},
    {(char *)"Date", NCM_SONG_GETTER_DATE, NCM_TAGS_FIELD_DATE},
    {(char *)"Track", NCM_SONG_GETTER_TRACK, NCM_TAGS_FIELD_TRACK},
    {(char *)"Genre", NCM_SONG_GETTER_GENRE, NCM_TAGS_FIELD_GENRE},
    {(char *)"Composer", NCM_SONG_GETTER_COMPOSER,
     NCM_TAGS_FIELD_COMPOSER},
    {(char *)"Performer", NCM_SONG_GETTER_PERFORMER,
     NCM_TAGS_FIELD_PERFORMER},
    {(char *)"Disc", NCM_SONG_GETTER_DISC, NCM_TAGS_FIELD_DISC},
    {(char *)"Comment", NCM_SONG_GETTER_COMMENT,
     NCM_TAGS_FIELD_COMMENT},
    {NULL, NCM_SONG_GETTER_NONE, NCM_TAGS_FIELD_LAST},
};

static struct NativeHelpScreen help_screen;
static struct NativeOutputsScreen outputs_screen;
static NativePlaylistScreen playlist_screen;
static bool playlist_screen_initialized;
static struct NativeServerInfoScreen server_info_screen;
static struct NativeSongInfoScreen song_info_screen;

static NcBorder native_no_border(void);
static int32 native_cstring_len(char *string);
static void native_draw_screen_header(NcScreen *screen);
static bool native_register_screen(NcScreen *screen);
static void native_resize_main_area(NcScreen *base, int64 *x, int64 *width);
static void native_append_cstring(NcBuffer *buffer, char *string);
static void native_append_data(NcBuffer *buffer, char *string, int32 len);
static void native_append_format(NcBuffer *buffer, enum NcFormat format);
static void native_append_formatted_color(NcBuffer *buffer,
                                          NcFormattedColor *color);
static void native_append_formatted_color_end(NcBuffer *buffer,
                                              NcFormattedColor *color);
static void native_append_bold_label(NcBuffer *buffer, char *label);
static void native_append_song_tag(NcBuffer *buffer, NcmBuffer *tag);
static void native_append_song_key_value(NcBuffer *buffer, char *key,
                                         NcmBuffer *value,
                                         bool empty_as_missing);
static int32 native_key_name(NcKey key, char *buffer, int32 buffer_cap);
static void native_append_action_keys(NcBuffer *buffer,
                                      enum NcmActionType type);
static void native_append_help_line(NcBuffer *buffer,
                                    enum NcmActionType type,
                                    char *description);
static bool native_help_render(void *user, NcBuffer *buffer);
static void native_help_switch_to(void *user);
static void native_help_resize(void *user, NcHelpScreen *screen);
static void native_help_destroy(void *user);
static NcHelpHooks native_help_hooks(void);
static void native_outputs_fetch(void *user, NcOutputsScreen *screen);
static bool native_outputs_toggle(void *user, uint32 id, bool enabled,
                                  char *name, int32 name_len);
static void native_outputs_switch_to(void *user);
static void native_outputs_resize(void *user, NcOutputsScreen *screen);
static void native_outputs_destroy(void *user);
static NcOutputsHooks native_outputs_hooks(void);
static void native_server_info_load_lists(void *user);
static bool native_server_info_render(void *user, NcBuffer *buffer);
static void native_server_info_switch_to(void *user);
static void native_server_info_resize(void *user);
static char *native_server_info_title(void *user);
static void native_server_info_destroy(void *user);
static NcServerInfoHooks native_server_info_hooks(void);
static bool native_song_info_render(void *user, NcSongInfoScreen *screen,
                                    NcBuffer *buffer);
static void native_song_info_switch_to(void *user, NcSongInfoScreen *screen);
static void native_song_info_resize(void *user, NcSongInfoScreen *screen);
static void native_song_info_destroy(void *user);
static NcSongInfoHooks native_song_info_hooks(void);
static void native_show_long_time(NcBuffer *buffer, uint64 seconds);

void
native_c_screen_help_init(void) {
    if (help_screen.initialized) {
        return;
    }

    nc_help_screen_init(&help_screen.screen,
                        native_help_hooks(),
                        0,
                        ui_state_screen_width(),
                        ui_state_main_start_y(),
                        ui_state_main_height(),
                        Config.main_color,
                        native_no_border(),
                        Config.lines_scrolled);
    help_screen.initialized = true;
    (void)nc_help_screen_reload(&help_screen.screen);
    return;
}

void
native_c_screen_help_register(void) {
    native_c_screen_help_init();
    assert(native_register_screen(native_c_screen_help_native()));
    return;
}

void
native_c_screen_help_set_resize(void) {
    nc_screen_request_resize(native_c_screen_help_native());
    return;
}

void
native_c_screen_help_switch_to(void) {
    (void)nc_screen_switcher_switch_to(native_c_screen_help_native(),
                                       nc_screen_has_to_be_resized(
                                           native_c_screen_help_native()));
    return;
}

bool
native_c_screen_help_is_current(void) {
    return nc_screen_switcher_is_current(native_c_screen_help_native());
}

NativeHelpScreen *
native_c_screen_help(void) {
    native_c_screen_help_init();
    return &help_screen;
}

NcHelpScreen *
native_c_screen_help_typed(void) {
    native_c_screen_help_init();
    return &help_screen.screen;
}

NcScreen *
native_c_screen_help_native(void) {
    native_c_screen_help_init();
    return nc_help_screen_base(&help_screen.screen);
}

void
native_c_screen_playlist_init(void) {
    if (playlist_screen_initialized) {
        return;
    }

    native_playlist_screen_init(&playlist_screen,
                                0,
                                ui_state_screen_width(),
                                ui_state_main_start_y(),
                                ui_state_main_height(),
                                Config.main_color,
                                native_no_border());
    native_playlist_screen_set_mouse_config(
        &playlist_screen, Config.lines_scrolled,
        Config.mouse_list_scroll_whole_page);
    playlist_screen_initialized = true;
    return;
}

void
native_c_screen_playlist_register(void) {
    native_c_screen_playlist_init();
    assert(native_register_screen(native_c_screen_playlist_native()));
    return;
}

void
native_c_screen_playlist_set_resize(void) {
    nc_screen_request_resize(native_c_screen_playlist_native());
    return;
}

void
native_c_screen_playlist_switch_to(void) {
    (void)nc_screen_switcher_switch_to(native_c_screen_playlist_native(),
                                       nc_screen_has_to_be_resized(
                                           native_c_screen_playlist_native()));
    return;
}

bool
native_c_screen_playlist_is_current(void) {
    return nc_screen_switcher_is_current(native_c_screen_playlist_native());
}

NativePlaylistScreen *
native_c_screen_playlist(void) {
    native_c_screen_playlist_init();
    return &playlist_screen;
}

NcPlaylistScreen *
native_c_screen_playlist_typed(void) {
    native_c_screen_playlist_init();
    return native_playlist_screen_playlist(&playlist_screen);
}

NcScreen *
native_c_screen_playlist_native(void) {
    native_c_screen_playlist_init();
    return native_playlist_screen_base(&playlist_screen);
}

void
native_c_screen_song_info_init(void) {
    if (song_info_screen.initialized) {
        return;
    }

    ncm_song_init(&song_info_screen.song);
    nc_song_info_screen_init(&song_info_screen.screen,
                             native_song_info_hooks(),
                             0,
                             ui_state_screen_width(),
                             ui_state_main_start_y(),
                             ui_state_main_height(),
                             Config.main_color,
                             native_no_border(),
                             Config.lines_scrolled);
    song_info_screen.initialized = true;
    return;
}

void
native_c_screen_song_info_register(void) {
    native_c_screen_song_info_init();
    assert(native_register_screen(native_c_screen_song_info_native()));
    return;
}

void
native_c_screen_song_info_set_resize(void) {
    nc_screen_request_resize(native_c_screen_song_info_native());
    return;
}

void
native_c_screen_song_info_switch_to(void) {
    (void)nc_screen_switcher_switch_to(native_c_screen_song_info_native(),
                                       nc_screen_has_to_be_resized(
                                           native_c_screen_song_info_native()));
    return;
}

NativeSongInfoScreen *
native_c_screen_song_info(void) {
    native_c_screen_song_info_init();
    return &song_info_screen;
}

NcSongInfoScreen *
native_c_screen_song_info_typed(void) {
    native_c_screen_song_info_init();
    return &song_info_screen.screen;
}

NcScreen *
native_c_screen_song_info_native(void) {
    native_c_screen_song_info_init();
    return nc_song_info_screen_base(&song_info_screen.screen);
}

void
native_c_screen_server_info_init(void) {
    if (server_info_screen.initialized) {
        return;
    }

    ncm_mpd_string_list_init(&server_info_screen.url_handlers);
    ncm_mpd_string_list_init(&server_info_screen.tag_types);
    nc_server_info_screen_init(&server_info_screen.screen,
                               native_server_info_hooks(),
                               ui_state_screen_width(),
                               ui_state_screen_height(),
                               ui_state_main_start_y(),
                               ui_state_main_height(),
                               Config.main_color,
                               Config.window_border);
    server_info_screen.initialized = true;
    return;
}

void
native_c_screen_server_info_register(void) {
    native_c_screen_server_info_init();
    assert(native_register_screen(native_c_screen_server_info_native()));
    return;
}

void
native_c_screen_server_info_set_resize(void) {
    nc_screen_request_resize(native_c_screen_server_info_native());
    return;
}

void
native_c_screen_server_info_switch_to(void) {
    NcScreen *screen;

    screen = native_c_screen_server_info_native();
    (void)nc_screen_switcher_switch_to(screen,
                                       nc_screen_has_to_be_resized(screen));
    return;
}

NativeServerInfoScreen *
native_c_screen_server_info(void) {
    native_c_screen_server_info_init();
    return &server_info_screen;
}

NcServerInfoScreen *
native_c_screen_server_info_typed(void) {
    native_c_screen_server_info_init();
    return &server_info_screen.screen;
}

NcScreen *
native_c_screen_server_info_native(void) {
    native_c_screen_server_info_init();
    return nc_server_info_screen_base(&server_info_screen.screen);
}

void
native_c_screen_outputs_init(void) {
#if defined(ENABLE_OUTPUTS)
    NcBuffer prefix;
    NcBuffer suffix;

    if (outputs_screen.initialized) {
        return;
    }

    nc_outputs_screen_init(&outputs_screen.screen,
                           native_outputs_hooks(),
                           0,
                           ui_state_screen_width(),
                           ui_state_main_start_y(),
                           ui_state_main_height(),
                           Config.main_color,
                           Config.window_border,
                           Config.lines_scrolled,
                           Config.mouse_list_scroll_whole_page);
    nc_buffer_init(&prefix);
    nc_buffer_init(&suffix);
    nc_buffer_copy(&prefix, &Config.current_item_prefix);
    nc_buffer_copy(&suffix, &Config.current_item_suffix);
    nc_outputs_screen_set_highlight_prefix(&outputs_screen.screen, &prefix);
    nc_outputs_screen_set_highlight_suffix(&outputs_screen.screen, &suffix);
    nc_buffer_destroy(&prefix);
    nc_buffer_destroy(&suffix);
    outputs_screen.initialized = true;
#endif
    return;
}

void
native_c_screen_outputs_register(void) {
#if defined(ENABLE_OUTPUTS)
    native_c_screen_outputs_init();
    assert(native_register_screen(native_c_screen_outputs_native()));
#endif
    return;
}

void
native_c_screen_outputs_set_resize(void) {
#if defined(ENABLE_OUTPUTS)
    nc_screen_request_resize(native_c_screen_outputs_native());
#endif
    return;
}

void
native_c_screen_outputs_switch_to(void) {
#if defined(ENABLE_OUTPUTS)
    (void)nc_screen_switcher_switch_to(native_c_screen_outputs_native(),
                                       nc_screen_has_to_be_resized(
                                           native_c_screen_outputs_native()));
#endif
    return;
}

bool
native_c_screen_outputs_is_current(void) {
#if defined(ENABLE_OUTPUTS)
    return nc_screen_switcher_is_current(native_c_screen_outputs_native());
#else
    return false;
#endif
}

void
native_c_screen_outputs_toggle(void) {
#if defined(ENABLE_OUTPUTS)
    (void)nc_outputs_screen_toggle_current(&outputs_screen.screen);
#endif
    return;
}

void
native_c_screen_outputs_fetch_list(void) {
#if defined(ENABLE_OUTPUTS)
    native_c_screen_outputs_init();
    nc_outputs_screen_fetch_list(&outputs_screen.screen);
#endif
    return;
}

void
native_c_screen_outputs_refresh_if_visible(void) {
#if defined(ENABLE_OUTPUTS)
    if (nc_screen_switcher_is_visible(native_c_screen_outputs_native())) {
        nc_screen_refresh_window(native_c_screen_outputs_native());
    }
#endif
    return;
}

NativeOutputsScreen *
native_c_screen_outputs(void) {
#if defined(ENABLE_OUTPUTS)
    native_c_screen_outputs_init();
    return &outputs_screen;
#else
    return NULL;
#endif
}

NcOutputsScreen *
native_c_screen_outputs_typed(void) {
#if defined(ENABLE_OUTPUTS)
    native_c_screen_outputs_init();
    return &outputs_screen.screen;
#else
    return NULL;
#endif
}

NcScreen *
native_c_screen_outputs_native(void) {
#if defined(ENABLE_OUTPUTS)
    native_c_screen_outputs_init();
    return nc_outputs_screen_base(&outputs_screen.screen);
#else
    return NULL;
#endif
}

static NcBorder
native_no_border(void) {
    NcBorder border = {0};

    return border;
}

static int32
native_cstring_len(char *string) {
    int32 len;

    if (string == NULL) {
        return 0;
    }

    len = 0;
    while (string[len] != '\0') {
        len += 1;
    }
    return len;
}

static void
native_draw_screen_header(NcScreen *screen) {
    char *title;

    title = nc_screen_title(screen);
    ncm_title_draw_header(title, native_cstring_len(title));
    return;
}

static bool
native_register_screen(NcScreen *screen) {
    if (app_controller_is_screen_registered(screen)) {
        return true;
    }
    return app_controller_register_screen(screen);
}

static void
native_resize_main_area(NcScreen *base, int64 *x, int64 *width) {
    nc_screen_switcher_get_resize_params(base, x, width, true);
    return;
}

static void
native_append_cstring(NcBuffer *buffer, char *string) {
    nc_buffer_append_cstring(buffer, string);
    return;
}

static void
native_append_data(NcBuffer *buffer, char *string, int32 len) {
    if ((string != NULL) && (len > 0)) {
        nc_buffer_append_data(buffer, string, len);
    }
    return;
}

static void
native_append_format(NcBuffer *buffer, enum NcFormat format) {
    nc_buffer_add_format(buffer, nc_buffer_len(buffer), format, 0);
    return;
}

static void
native_append_formatted_color(NcBuffer *buffer,
                              NcFormattedColor *color) {
    nc_buffer_add_formatted_color(buffer, nc_buffer_len(buffer), color, 0);
    return;
}

static void
native_append_formatted_color_end(NcBuffer *buffer,
                                  NcFormattedColor *color) {
    nc_buffer_add_formatted_color_end(buffer, nc_buffer_len(buffer), color, 0);
    return;
}

static void
native_append_bold_label(NcBuffer *buffer, char *label) {
    native_append_format(buffer, NC_FORMAT_BOLD);
    native_append_cstring(buffer, label);
    native_append_format(buffer, NC_FORMAT_NO_BOLD);
    return;
}

static void
native_append_song_tag(NcBuffer *buffer, NcmBuffer *tag) {
    if ((tag == NULL) || (tag->len <= 0)) {
        native_append_formatted_color(buffer, &Config.empty_tags_color);
        native_append_data(buffer, Config.empty_tag, Config.empty_tag_len);
        native_append_formatted_color_end(buffer, &Config.empty_tags_color);
        return;
    }
    native_append_data(buffer, tag->data, tag->len);
    return;
}

static void
native_append_song_key_value(NcBuffer *buffer, char *key,
                             NcmBuffer *value,
                             bool empty_as_missing) {
    native_append_format(buffer, NC_FORMAT_BOLD);
    native_append_formatted_color(buffer, &Config.color1);
    native_append_cstring(buffer, key);
    native_append_cstring(buffer, ":");
    native_append_formatted_color_end(buffer, &Config.color1);
    native_append_format(buffer, NC_FORMAT_NO_BOLD);
    native_append_cstring(buffer, " ");
    native_append_formatted_color(buffer, &Config.color2);
    if (empty_as_missing) {
        native_append_song_tag(buffer, value);
    } else if (value != NULL) {
        native_append_data(buffer, value->data, value->len);
    }
    native_append_formatted_color_end(buffer, &Config.color2);
    native_append_cstring(buffer, "\n");
    return;
}

static int32
native_key_name(NcKey key, char *buffer, int32 buffer_cap) {
    int32 len;

    len = ncm_bindings_key_name(key, buffer, buffer_cap);
    if (len < 0) {
        return 0;
    }
    return len;
}

static void
native_append_action_keys(NcBuffer *buffer, enum NcmActionType type) {
    int32 column_start;
    int32 width;

    column_start = nc_buffer_len(buffer);
    width = 0;
    for (int32 i = 0; i < Bindings.keys_len; i += 1) {
        NcmKeyBindings *key_bindings;
        char key_name[64];
        int32 key_len;

        key_bindings = Bindings.keys + i;
        for (int32 j = 0; j < key_bindings->bindings_len; j += 1) {
            NcmBinding *binding;

            binding = key_bindings->bindings + j;
            if (!ncm_binding_is_single_action_type(binding, type)) {
                continue;
            }
            key_len = native_key_name(key_bindings->key, key_name,
                                      (int32)sizeof(key_name));
            if (key_len <= 0) {
                continue;
            }
            if (width > 0) {
                native_append_cstring(buffer, " ");
                width += 1;
            }
            native_append_data(buffer, key_name, key_len);
            width += key_len;
        }
    }
    while ((nc_buffer_len(buffer) - column_start) < 20) {
        nc_buffer_append_char(buffer, ' ');
    }
    return;
}

static void
native_append_help_line(NcBuffer *buffer, enum NcmActionType type,
                        char *description) {
    native_append_cstring(buffer, "    ");
    native_append_action_keys(buffer, type);
    native_append_cstring(buffer, " : ");
    native_append_cstring(buffer, description);
    native_append_cstring(buffer, "\n");
    return;
}

static bool
native_help_render(void *user, NcBuffer *buffer) {
    (void)user;

    native_append_format(buffer, NC_FORMAT_BOLD);
    native_append_cstring(buffer, "\n  Keys - Movement\n\n");
    native_append_format(buffer, NC_FORMAT_NO_BOLD);
    native_append_help_line(buffer, NCM_ACTION_SCROLL_UP, "Move cursor up");
    native_append_help_line(buffer, NCM_ACTION_SCROLL_DOWN,
                            "Move cursor down");
    native_append_help_line(buffer, NCM_ACTION_PAGE_UP, "Page up");
    native_append_help_line(buffer, NCM_ACTION_PAGE_DOWN, "Page down");
    native_append_help_line(buffer, NCM_ACTION_MOVE_HOME, "Home");
    native_append_help_line(buffer, NCM_ACTION_MOVE_END, "End");
    native_append_help_line(buffer, NCM_ACTION_NEXT_SCREEN, "Next screen");
    native_append_help_line(buffer, NCM_ACTION_PREVIOUS_SCREEN,
                            "Previous screen");
    native_append_help_line(buffer, NCM_ACTION_SHOW_HELP, "Show help");
    native_append_help_line(buffer, NCM_ACTION_SHOW_PLAYLIST,
                            "Show playlist");
    native_append_help_line(buffer, NCM_ACTION_SHOW_BROWSER, "Show browser");
    native_append_help_line(buffer, NCM_ACTION_SHOW_SEARCH_ENGINE,
                            "Show search engine");
    native_append_help_line(buffer, NCM_ACTION_SHOW_MEDIA_LIBRARY,
                            "Show media library");
    native_append_help_line(buffer, NCM_ACTION_SHOW_PLAYLIST_EDITOR,
                            "Show playlist editor");
    native_append_help_line(buffer, NCM_ACTION_SHOW_SERVER_INFO,
                            "Show server info");
#if defined(ENABLE_OUTPUTS)
    native_append_help_line(buffer, NCM_ACTION_SHOW_OUTPUTS,
                            "Show outputs");
#endif
#if defined(ENABLE_VISUALIZER)
    native_append_help_line(buffer, NCM_ACTION_SHOW_VISUALIZER,
                            "Show music visualizer");
#endif
#if defined(HAVE_TAGLIB_H)
    native_append_help_line(buffer, NCM_ACTION_SHOW_TAG_EDITOR,
                            "Show tag editor");
#endif

    native_append_format(buffer, NC_FORMAT_BOLD);
    native_append_cstring(buffer, "\n  Keys - Global\n\n");
    native_append_format(buffer, NC_FORMAT_NO_BOLD);
    native_append_help_line(buffer, NCM_ACTION_PLAY, "Play");
    native_append_help_line(buffer, NCM_ACTION_STOP, "Stop");
    native_append_help_line(buffer, NCM_ACTION_PAUSE, "Pause");
    native_append_help_line(buffer, NCM_ACTION_NEXT, "Next track");
    native_append_help_line(buffer, NCM_ACTION_PREVIOUS, "Previous track");
    native_append_help_line(buffer, NCM_ACTION_VOLUME_DOWN,
                            "Decrease volume");
    native_append_help_line(buffer, NCM_ACTION_VOLUME_UP,
                            "Increase volume");
    native_append_help_line(buffer, NCM_ACTION_TOGGLE_REPEAT,
                            "Toggle repeat mode");
    native_append_help_line(buffer, NCM_ACTION_TOGGLE_RANDOM,
                            "Toggle random mode");
    native_append_help_line(buffer, NCM_ACTION_TOGGLE_SINGLE,
                            "Toggle single mode");
    native_append_help_line(buffer, NCM_ACTION_TOGGLE_CONSUME,
                            "Toggle consume mode");
    native_append_help_line(buffer, NCM_ACTION_UPDATE_DATABASE,
                            "Start music database update");
    native_append_help_line(buffer, NCM_ACTION_EXECUTE_COMMAND,
                            "Execute command");
    native_append_help_line(buffer, NCM_ACTION_QUIT, "Quit");
    return true;
}

static void
native_help_switch_to(void *user) {
    (void)user;
    (void)nc_screen_switcher_finish_switch(native_c_screen_help_native());
    native_draw_screen_header(native_c_screen_help_native());
    return;
}

static void
native_help_resize(void *user, NcHelpScreen *screen) {
    int64 x;
    int64 width;

    (void)user;
    native_resize_main_area(nc_help_screen_base(screen), &x, &width);
    nc_help_screen_set_geometry(screen,
                                x,
                                width,
                                ui_state_main_start_y(),
                                ui_state_main_height());
    return;
}

static void
native_help_destroy(void *user) {
    NativeHelpScreen *owner;

    owner = user;
    owner->initialized = false;
    return;
}

static NcHelpHooks
native_help_hooks(void) {
    NcHelpHooks hooks = {0};

    hooks.render = native_help_render;
    hooks.switch_to = native_help_switch_to;
    hooks.resize_layout = native_help_resize;
    hooks.destroy = native_help_destroy;
    hooks.user = &help_screen;
    return hooks;
}

static void
native_outputs_fetch(void *user, NcOutputsScreen *screen) {
#if defined(ENABLE_OUTPUTS)
    NcmMpdOutputList outputs;
    NcmError error;

    (void)user;
    ncm_error_clear(&error);
    ncm_mpd_output_list_init(&outputs);
    if (!ncm_mpd_client_get_outputs(&global_mpd, &outputs, &error)) {
        NcmStringFormatArg arg;

        arg = ncm_string_format_arg_cstring(error.message);
        ncm_statusbar_format(5,
                             STRLIT_ARGS("Could not fetch outputs: %1"),
                             &arg,
                             1);
        ncm_mpd_output_list_destroy(&outputs);
        return;
    }

    for (int32 i = 0; i < outputs.count; i += 1) {
        NcmMpdOutput *output;

        output = outputs.items + i;
        nc_outputs_screen_add_output(screen,
                                     output->id,
                                     output->name,
                                     output->name_len,
                                     output->enabled);
    }
    ncm_mpd_output_list_destroy(&outputs);
#else
    (void)user;
    (void)screen;
#endif
    return;
}

static bool
native_outputs_toggle(void *user, uint32 id, bool enabled,
                      char *name, int32 name_len) {
#if defined(ENABLE_OUTPUTS)
    NcmError error;
    bool ok;

    (void)user;
    ncm_error_clear(&error);
    if (enabled) {
        ok = ncm_mpd_client_disable_output(&global_mpd, id, &error);
    } else {
        ok = ncm_mpd_client_enable_output(&global_mpd, id, &error);
    }
    if (!ok) {
        NcmStringFormatArg args[2];

        args[0] = ncm_string_format_arg_string(name, name_len);
        args[1] = ncm_string_format_arg_cstring(error.message);
        ncm_statusbar_format(5,
                             STRLIT_ARGS("Could not toggle output %1: %2"),
                             args,
                             2);
        return false;
    }

    if (enabled) {
        NcmStringFormatArg arg;

        arg = ncm_string_format_arg_string(name, name_len);
        ncm_statusbar_format(3,
                             STRLIT_ARGS("Output %1 disabled"),
                             &arg,
                             1);
    } else {
        NcmStringFormatArg arg;

        arg = ncm_string_format_arg_string(name, name_len);
        ncm_statusbar_format(3,
                             STRLIT_ARGS("Output %1 enabled"),
                             &arg,
                             1);
    }
    return true;
#else
    (void)user;
    (void)id;
    (void)enabled;
    (void)name;
    (void)name_len;
    return false;
#endif
}

static void
native_outputs_switch_to(void *user) {
    (void)user;
    (void)nc_screen_switcher_finish_switch(native_c_screen_outputs_native());
    native_draw_screen_header(native_c_screen_outputs_native());
    return;
}

static void
native_outputs_resize(void *user, NcOutputsScreen *screen) {
    int64 x;
    int64 width;

    (void)user;
    native_resize_main_area(nc_outputs_screen_base(screen), &x, &width);
    nc_outputs_screen_set_geometry(screen,
                                   x,
                                   width,
                                   ui_state_main_start_y(),
                                   ui_state_main_height());
    return;
}

static void
native_outputs_destroy(void *user) {
#if defined(ENABLE_OUTPUTS)
    NativeOutputsScreen *owner;

    owner = user;
    owner->initialized = false;
#else
    (void)user;
#endif
    return;
}

static NcOutputsHooks
native_outputs_hooks(void) {
    NcOutputsHooks hooks = {0};

    hooks.fetch_outputs = native_outputs_fetch;
    hooks.toggle_output = native_outputs_toggle;
    hooks.switch_to = native_outputs_switch_to;
    hooks.resize_layout = native_outputs_resize;
    hooks.destroy = native_outputs_destroy;
    hooks.user = &outputs_screen;
    return hooks;
}

static void
native_server_info_load_lists(void *user) {
    NativeServerInfoScreen *owner;
    NcmError error;

    owner = user;
    ncm_error_clear(&error);
    (void)ncm_mpd_client_get_url_handlers(&global_mpd,
                                           &owner->url_handlers,
                                           &error);
    ncm_error_clear(&error);
    (void)ncm_mpd_client_get_tag_types(&global_mpd,
                                        &owner->tag_types,
                                        &error);
    return;
}

static bool
native_server_info_render(void *user, NcBuffer *buffer) {
    NativeServerInfoScreen *owner;
    NcmMpdStats stats;
    NcmError error;
    char time_buffer[64];

    owner = user;
    if (global_timer_elapsed_ms(owner->timer) < 1000) {
        return false;
    }
    owner->timer = global_timer;

    ncm_error_clear(&error);
    if (!ncm_mpd_client_get_stats(&global_mpd, &stats, &error)) {
        return false;
    }

    native_append_bold_label(buffer, "Version: ");
    native_append_cstring(buffer, "0.");
    nc_buffer_append_uint32(buffer, ncm_mpd_client_version(&global_mpd));
    native_append_cstring(buffer, ".*\n");

    native_append_bold_label(buffer, "Uptime: ");
    native_show_long_time(buffer, stats.uptime);
    native_append_cstring(buffer, "\n");

    native_append_bold_label(buffer, "Time playing: ");
    ncm_song_show_time((uint32)stats.play_time,
                       time_buffer,
                       (int32)sizeof(time_buffer));
    native_append_cstring(buffer, time_buffer);
    native_append_cstring(buffer, "\n\n");

    native_append_bold_label(buffer, "Total playtime: ");
    native_show_long_time(buffer, stats.db_play_time);
    native_append_cstring(buffer, "\n");

    native_append_bold_label(buffer, "Artist names: ");
    nc_buffer_append_uint32(buffer, stats.artists);
    native_append_cstring(buffer, "\n");

    native_append_bold_label(buffer, "Album names: ");
    nc_buffer_append_uint32(buffer, stats.albums);
    native_append_cstring(buffer, "\n");

    native_append_bold_label(buffer, "Songs in database: ");
    nc_buffer_append_uint32(buffer, stats.songs);
    native_append_cstring(buffer, "\n\n");

    native_append_bold_label(buffer, "URL Handlers:");
    for (int32 i = 0; i < owner->url_handlers.count; i += 1) {
        NcmMpdString *handler;

        handler = owner->url_handlers.items + i;
        native_append_cstring(buffer, i == 0 ? " " : ", ");
        native_append_data(buffer, handler->value, handler->value_len);
    }
    native_append_cstring(buffer, "\n\n");

    native_append_bold_label(buffer, "Tag Types:");
    for (int32 i = 0; i < owner->tag_types.count; i += 1) {
        NcmMpdString *tag;

        tag = owner->tag_types.items + i;
        native_append_cstring(buffer, i == 0 ? " " : ", ");
        native_append_data(buffer, tag->value, tag->value_len);
    }
    return true;
}

static void
native_server_info_switch_to(void *user) {
    (void)user;
    (void)nc_screen_switcher_finish_switch(
        native_c_screen_server_info_native());
    native_draw_screen_header(native_c_screen_server_info_native());
    return;
}

static void
native_server_info_resize(void *user) {
    (void)user;
    nc_server_info_screen_set_dimensions(&server_info_screen.screen,
                                         ui_state_screen_width(),
                                         ui_state_screen_height(),
                                         ui_state_main_start_y(),
                                         ui_state_main_height());
    return;
}

static char *
native_server_info_title(void *user) {
    (void)user;
    return (char *)"Server info";
}

static void
native_server_info_destroy(void *user) {
    NativeServerInfoScreen *owner;

    owner = user;
    ncm_mpd_string_list_destroy(&owner->url_handlers);
    ncm_mpd_string_list_destroy(&owner->tag_types);
    owner->initialized = false;
    return;
}

static NcServerInfoHooks
native_server_info_hooks(void) {
    NcServerInfoHooks hooks = {0};

    hooks.load_lists = native_server_info_load_lists;
    hooks.render = native_server_info_render;
    hooks.switch_to = native_server_info_switch_to;
    hooks.resize_layout = native_server_info_resize;
    hooks.title = native_server_info_title;
    hooks.destroy = native_server_info_destroy;
    hooks.user = &server_info_screen;
    return hooks;
}

static bool
native_song_info_render(void *user, NcSongInfoScreen *screen,
                        NcBuffer *buffer) {
    NativeSongInfoScreen *owner;
    NcmBuffer value;

    (void)screen;
    owner = user;
    if (!owner->has_song) {
        return false;
    }

    value = ncm_song_getter_buffer(&owner->song,
                                   NCM_SONG_GETTER_NAME,
                                   0);
    native_append_song_key_value(buffer, "Filename", &value, false);
    ncm_buffer_destroy(&value);

    value = ncm_song_getter_buffer(&owner->song,
                                   NCM_SONG_GETTER_DIRECTORY,
                                   0);
    native_append_song_key_value(buffer, "Directory", &value, true);
    ncm_buffer_destroy(&value);
    native_append_cstring(buffer, "\n");

    value = ncm_song_getter_buffer(&owner->song,
                                   NCM_SONG_GETTER_LENGTH,
                                   0);
    native_append_song_key_value(buffer, "Length", &value, false);
    ncm_buffer_destroy(&value);

    for (int32 i = 0; ncm_song_info_tags[i].name != NULL; i += 1) {
        native_append_format(buffer, NC_FORMAT_BOLD);
        native_append_cstring(buffer, "\n");
        native_append_cstring(buffer, ncm_song_info_tags[i].name);
        native_append_cstring(buffer, ":");
        native_append_format(buffer, NC_FORMAT_NO_BOLD);
        native_append_cstring(buffer, " ");
        value = ncm_song_tags_buffer(&owner->song,
                                     ncm_song_info_tags[i].get,
                                     Config.tags_separator,
                                     Config.tags_separator_len,
                                     Config.show_duplicate_tags);
        native_append_song_tag(buffer, &value);
        ncm_buffer_destroy(&value);
    }
    return true;
}

static void
native_song_info_switch_to(void *user, NcSongInfoScreen *screen) {
    NativeSongInfoScreen *owner;
    NcmError error;

    owner = user;
    ncm_error_clear(&error);
    ncm_song_destroy(&owner->song);
    ncm_song_init(&owner->song);
    owner->has_song = ncm_mpd_client_get_current_song(&global_mpd,
                                                      &owner->song,
                                                      &error);
    if (!owner->has_song) {
        NcmStringFormatArg arg;

        arg = ncm_string_format_arg_cstring(error.message);
        ncm_statusbar_format(5,
                             STRLIT_ARGS("Could not fetch current song: %1"),
                             &arg,
                             1);
        return;
    }

    (void)nc_screen_switcher_finish_switch(native_c_screen_song_info_native());
    (void)nc_song_info_screen_prepare_current(screen);
    native_draw_screen_header(native_c_screen_song_info_native());
    return;
}

static void
native_song_info_resize(void *user, NcSongInfoScreen *screen) {
    int64 x;
    int64 width;

    (void)user;
    native_resize_main_area(nc_song_info_screen_base(screen), &x, &width);
    nc_song_info_screen_set_geometry(screen,
                                     x,
                                     width,
                                     ui_state_main_start_y(),
                                     ui_state_main_height());
    return;
}

static void
native_song_info_destroy(void *user) {
    NativeSongInfoScreen *owner;

    owner = user;
    ncm_song_destroy(&owner->song);
    owner->has_song = false;
    owner->initialized = false;
    return;
}

static NcSongInfoHooks
native_song_info_hooks(void) {
    NcSongInfoHooks hooks = {0};

    hooks.render = native_song_info_render;
    hooks.switch_to = native_song_info_switch_to;
    hooks.resize_layout = native_song_info_resize;
    hooks.destroy = native_song_info_destroy;
    hooks.user = &song_info_screen;
    return hooks;
}

static void
native_show_long_time(NcBuffer *buffer, uint64 seconds) {
    uint64 days;
    uint64 hours;
    uint64 minutes;

    days = seconds / 86400;
    seconds -= days*86400;
    hours = seconds / 3600;
    seconds -= hours*3600;
    minutes = seconds / 60;
    seconds -= minutes*60;

    if (days > 0) {
        nc_buffer_append_uint64(buffer, days);
        native_append_cstring(buffer, "d ");
    }
    if ((days > 0) || (hours > 0)) {
        nc_buffer_append_uint64(buffer, hours);
        native_append_cstring(buffer, "h ");
    }
    if ((days > 0) || (hours > 0) || (minutes > 0)) {
        nc_buffer_append_uint64(buffer, minutes);
        native_append_cstring(buffer, "m ");
    }
    nc_buffer_append_uint64(buffer, seconds);
    native_append_cstring(buffer, "s");
    return;
}
