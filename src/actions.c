#if !defined(NCMPCPP_ACTIONS_C)
#define NCMPCPP_ACTIONS_C

#include "cbase.h"

#include "actions.h"
#include "app_controller.h"
#include "app_legacy_bridge.h"
#include "bindings.h"
#include "c/ncm_c.h"
#if defined(HAVE_TAGLIB_H)
#endif
#include "curses/nc_curses.h"
#include "global.h"
#include "helpers.h"
#include "screen_actions.h"
#include "screens/nc_screens.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "title.h"
#include "ui_state.h"

#define NCM_ACTION_TABLE_CALLBACKS(SUFFIX, TYPE)                           \
    static bool                                                            \
    ncm_action_can_run_##SUFFIX(void *user) {                              \
        (void)user;                                                        \
        return ncm_action_runtime_can_run(NULL, NCM_ACTION_##TYPE);        \
    }                                                                      \
                                                                           \
    static bool                                                            \
    ncm_action_run_##SUFFIX(void *user) {                                  \
        (void)user;                                                        \
        return ncm_action_runtime_run(NULL, NCM_ACTION_##TYPE);            \
    }

#define NCM_ACTION_TABLE_DEFS(XX)                                              \
XX(dummy, DUMMY)                                                    \
XX(update_environment, UPDATE_ENVIRONMENT)                          \
XX(mouse_event, MOUSE_EVENT)                                        \
XX(scroll_up, SCROLL_UP)                                            \
XX(scroll_down, SCROLL_DOWN)                                        \
XX(scroll_up_artist, SCROLL_UP_ARTIST)                              \
XX(scroll_up_album, SCROLL_UP_ALBUM)                                \
XX(scroll_down_artist, SCROLL_DOWN_ARTIST)                          \
XX(scroll_down_album, SCROLL_DOWN_ALBUM)                            \
XX(page_up, PAGE_UP)                                                \
XX(page_down, PAGE_DOWN)                                            \
XX(move_home, MOVE_HOME)                                            \
XX(move_end, MOVE_END)                                              \
XX(toggle_interface, TOGGLE_INTERFACE)                              \
XX(jump_to_parent_directory, JUMP_TO_PARENT_DIRECTORY)              \
XX(run_action, RUN_ACTION)                                          \
XX(previous_column, PREVIOUS_COLUMN)                                \
XX(next_column, NEXT_COLUMN)                                        \
XX(master_screen, MASTER_SCREEN)                                    \
XX(slave_screen, SLAVE_SCREEN)                                      \
XX(volume_up, VOLUME_UP)                                            \
XX(volume_down, VOLUME_DOWN)                                        \
XX(add_item_to_playlist, ADD_ITEM_TO_PLAYLIST)                      \
XX(play_item, PLAY_ITEM)                                            \
XX(delete_playlist_items, DELETE_PLAYLIST_ITEMS)                    \
XX(delete_stored_playlist, DELETE_STORED_PLAYLIST)                  \
XX(delete_browser_items, DELETE_BROWSER_ITEMS)                      \
XX(replay_song, REPLAY_SONG)                                        \
XX(previous, PREVIOUS)                                              \
XX(next, NEXT)                                                      \
XX(pause, PAUSE)                                                    \
XX(stop, STOP)                                                      \
XX(play, PLAY)                                                      \
XX(execute_command, EXECUTE_COMMAND)                                \
XX(save_playlist, SAVE_PLAYLIST)                                    \
XX(move_sort_order_up, MOVE_SORT_ORDER_UP)                          \
XX(move_sort_order_down, MOVE_SORT_ORDER_DOWN)                      \
XX(move_selected_items_up, MOVE_SELECTED_ITEMS_UP)                  \
XX(move_selected_items_down, MOVE_SELECTED_ITEMS_DOWN)              \
XX(move_selected_items_to, MOVE_SELECTED_ITEMS_TO)                  \
XX(add, ADD)                                                        \
XX(load, LOAD)                                                      \
XX(seek_forward, SEEK_FORWARD)                                      \
XX(seek_backward, SEEK_BACKWARD)                                    \
XX(toggle_display_mode, TOGGLE_DISPLAY_MODE)                        \
XX(toggle_separators_between_albums, TOGGLE_SEPARATORS_BETWEEN_ALBUMS)\
XX(toggle_lyrics_update_on_song_change, TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE)\
XX(toggle_lyrics_fetcher, TOGGLE_LYRICS_FETCHER)                    \
XX(toggle_fetching_lyrics_in_background, TOGGLE_FETCHING_LYRICS_IN_BACKGROUND)\
XX(toggle_playing_song_centering, TOGGLE_PLAYING_SONG_CENTERING)    \
XX(update_database, UPDATE_DATABASE)                                \
XX(jump_to_playing_song, JUMP_TO_PLAYING_SONG)                      \
XX(toggle_repeat, TOGGLE_REPEAT)                                    \
XX(shuffle, SHUFFLE)                                                \
XX(toggle_random, TOGGLE_RANDOM)                                    \
XX(start_searching, START_SEARCHING)                                \
XX(save_tag_changes, SAVE_TAG_CHANGES)                              \
XX(toggle_single, TOGGLE_SINGLE)                                    \
XX(toggle_consume, TOGGLE_CONSUME)                                  \
XX(toggle_crossfade, TOGGLE_CROSSFADE)                              \
XX(set_crossfade, SET_CROSSFADE)                                    \
XX(set_volume, SET_VOLUME)                                          \
XX(enter_directory, ENTER_DIRECTORY)                                \
XX(edit_song, EDIT_SONG)                                            \
XX(edit_library_tag, EDIT_LIBRARY_TAG)                              \
XX(edit_library_album, EDIT_LIBRARY_ALBUM)                          \
XX(edit_directory_name, EDIT_DIRECTORY_NAME)                        \
XX(edit_playlist_name, EDIT_PLAYLIST_NAME)                          \
XX(edit_lyrics, EDIT_LYRICS)                                        \
XX(jump_to_browser, JUMP_TO_BROWSER)                                \
XX(jump_to_media_library, JUMP_TO_MEDIA_LIBRARY)                    \
XX(jump_to_playlist_editor, JUMP_TO_PLAYLIST_EDITOR)                \
XX(toggle_screen_lock, TOGGLE_SCREEN_LOCK)                          \
XX(jump_to_tag_editor, JUMP_TO_TAG_EDITOR)                          \
XX(jump_to_position_in_song, JUMP_TO_POSITION_IN_SONG)              \
XX(select_item, SELECT_ITEM)                                        \
XX(select_range, SELECT_RANGE)                                      \
XX(reverse_selection, REVERSE_SELECTION)                            \
XX(remove_selection, REMOVE_SELECTION)                              \
XX(select_album, SELECT_ALBUM)                                      \
XX(select_found_items, SELECT_FOUND_ITEMS)                          \
XX(add_selected_items, ADD_SELECTED_ITEMS)                          \
XX(crop_main_playlist, CROP_MAIN_PLAYLIST)                          \
XX(crop_playlist, CROP_PLAYLIST)                                    \
XX(clear_main_playlist, CLEAR_MAIN_PLAYLIST)                        \
XX(clear_playlist, CLEAR_PLAYLIST)                                  \
XX(sort_playlist, SORT_PLAYLIST)                                    \
XX(reverse_playlist, REVERSE_PLAYLIST)                              \
XX(apply_filter, APPLY_FILTER)                                      \
XX(find, FIND)                                                      \
XX(find_item_forward, FIND_ITEM_FORWARD)                            \
XX(find_item_backward, FIND_ITEM_BACKWARD)                          \
XX(next_found_item, NEXT_FOUND_ITEM)                                \
XX(previous_found_item, PREVIOUS_FOUND_ITEM)                        \
XX(toggle_find_mode, TOGGLE_FIND_MODE)                              \
XX(toggle_replay_gain_mode, TOGGLE_REPLAY_GAIN_MODE)                \
XX(toggle_add_mode, TOGGLE_ADD_MODE)                                \
XX(toggle_mouse, TOGGLE_MOUSE)                                      \
XX(toggle_bitrate_visibility, TOGGLE_BITRATE_VISIBILITY)            \
XX(add_random_items, ADD_RANDOM_ITEMS)                              \
XX(toggle_browser_sort_mode, TOGGLE_BROWSER_SORT_MODE)              \
XX(toggle_library_tag_type, TOGGLE_LIBRARY_TAG_TYPE)                \
XX(toggle_media_library_sort_mode, TOGGLE_MEDIA_LIBRARY_SORT_MODE)  \
XX(fetch_lyrics_in_background, FETCH_LYRICS_IN_BACKGROUND)          \
XX(refetch_lyrics, REFETCH_LYRICS)                                  \
XX(set_selected_items_priority, SET_SELECTED_ITEMS_PRIORITY)        \
XX(toggle_output, TOGGLE_OUTPUT)                                    \
XX(toggle_visualization_type, TOGGLE_VISUALIZATION_TYPE)            \
XX(show_song_info, SHOW_SONG_INFO)                                  \
XX(show_artist_info, SHOW_ARTIST_INFO)                              \
XX(show_lyrics, SHOW_LYRICS)                                        \
XX(quit, QUIT)                                                      \
XX(next_screen, NEXT_SCREEN)                                        \
XX(previous_screen, PREVIOUS_SCREEN)                                \
XX(show_help, SHOW_HELP)                                            \
XX(show_playlist, SHOW_PLAYLIST)                                    \
XX(show_browser, SHOW_BROWSER)                                      \
XX(change_browse_mode, CHANGE_BROWSE_MODE)                          \
XX(show_search_engine, SHOW_SEARCH_ENGINE)                          \
XX(reset_search_engine, RESET_SEARCH_ENGINE)                        \
XX(show_media_library, SHOW_MEDIA_LIBRARY)                          \
XX(toggle_media_library_columns_mode, TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE)\
XX(show_playlist_editor, SHOW_PLAYLIST_EDITOR)                      \
XX(show_tag_editor, SHOW_TAG_EDITOR)                                \
XX(show_outputs, SHOW_OUTPUTS)                                      \
XX(show_visualizer, SHOW_VISUALIZER)                                \
XX(show_server_info, SHOW_SERVER_INFO)

#define XX(SUFFIX, TYPE) NCM_ACTION_TABLE_CALLBACKS(SUFFIX, TYPE)
NCM_ACTION_TABLE_DEFS(XX)
#undef XX

#define XX(SUFFIX, TYPE)                    \
    {                                       \
        #SUFFIX,                            \
        STRLIT_LEN(#SUFFIX),                \
        NCM_ACTION_##TYPE,                  \
        ncm_action_can_run_##SUFFIX,        \
        ncm_action_run_##SUFFIX,            \
    },
static NcmActionDef action_defs[] = {
    NCM_ACTION_TABLE_DEFS(XX)
};
#undef XX

NcmActionDef *
ncm_action_table_get(NcmActionDef *defs, int32 defs_len,
                     enum NcmActionType type) {
    if (defs == NULL) {
        return NULL;
    }

    for (int32 i = 0; i < defs_len; i += 1) {
        if (defs[i].type == type) {
            return defs + i;
        }
    }
    return NULL;
}

NcmActionDef *
ncm_action_table_find(NcmActionDef *defs, int32 defs_len,
                      char *name, int32 name_len) {
    if ((defs == NULL) || (name == NULL) || (name_len <= 0)) {
        return NULL;
    }

    for (int32 i = 0; i < defs_len; i += 1) {
        char *def_name = defs[i].name;
        int32 def_name_len = defs[i].name_len;
        if (def_name && STREQUAL(name, name_len, def_name, def_name_len)) {
            return defs + i;
        }
    }
    return NULL;
}

NcmActionDef *
ncm_action_get(enum NcmActionType type) {
    return ncm_action_table_get(action_defs, LENGTH(action_defs), type);
}

NcmActionDef *
ncm_action_find(char *name, int32 name_len) {
    return ncm_action_table_find(action_defs, LENGTH(action_defs),
                                 name, name_len);
}

bool
ncm_action_type_parse(char *name, int32 name_len, enum NcmActionType *type) {
    NcmActionDef *action;

    if (type == NULL) {
        return false;
    }

    if ((action = ncm_action_find(name, name_len)) == NULL) {
        return false;
    }
    *type = action->type;
    return true;
}

bool
ncm_action_def_can_run(NcmActionDef *action, void *user) {
    if ((action == NULL) || (action->can_run == NULL)) {
        return false;
    }
    return action->can_run(user);
}

bool
ncm_action_def_run(NcmActionDef *action, void *user) {
    if ((action == NULL) || (action->run == NULL)) {
        return false;
    }
    if (!ncm_action_def_can_run(action, user)) {
        return false;
    }
    return action->run(user);
}

bool
ncm_action_can_run(enum NcmActionType type, void *user) {
    return ncm_action_def_can_run(ncm_action_get(type), user);
}

bool
ncm_action_run(enum NcmActionType type, void *user) {
    return ncm_action_def_run(ncm_action_get(type), user);
}

static NcmActionRuntime action_global_runtime;
static bool action_global_runtime_initialized;

typedef struct ActionRuntimeCommandPrompt {
    StrBuilder previous;
} ActionRuntimeCommandPrompt;

typedef NcmSearchPromptState ActionRuntimeSearchPrompt;

static NcmActionRuntime *action_runtime_or_global(NcmActionRuntime *runtime);
static int32 action_runtime_call_hook(NcmActionRuntimeHook hook,
                                      enum NcmActionType type, void *user);
static bool action_runtime_hook_allowed(int32 result, bool *handled);
static bool action_runtime_hook_denied(int32 result, bool *handled);
static bool action_runtime_builtin_can_run(NcmActionRuntime *runtime,
                                           enum NcmActionType type);
static bool action_runtime_builtin_run(NcmActionRuntime *runtime,
                                       enum NcmActionType type);
static bool action_runtime_current_screen_is(enum ScreenType type);
static bool action_runtime_switch_to_screen(enum ScreenType type);
static bool action_runtime_switch_to_next_screen(bool reverse);
static bool action_runtime_mpd_error(NcmError *ncm_error);
static bool action_runtime_playlist_find_song(NcmSong *song,
                                              NcmSong **match);
static bool action_runtime_playlist_remove_song(NcmSong *song,
                                                NcmError *ncm_error);
static bool action_runtime_mpd_simple(bool (*func)(NcmMpdClient *client,
                                                   NcmError *ncm_error));
static bool action_runtime_mpd_toggle(bool (*func)(NcmMpdClient *client,
                                                   bool mode,
                                                   NcmError *ncm_error),
                                      bool current);
static bool action_runtime_volume(int32 change);
static bool action_runtime_update_database(void);
static bool action_runtime_replay_song(void);
static bool action_runtime_execute_command(void);
static bool action_runtime_apply_filter(void);
static bool action_runtime_find(void);
static bool action_runtime_find_item(enum SearchDirection direction);
static bool action_runtime_repeat_search(enum SearchDirection direction);
static bool action_runtime_command_prompt_hook(char *text, void *user);
static bool action_runtime_filter_prompt_hook(char *text, void *user);
static void action_runtime_search_prompt_init(ActionRuntimeSearchPrompt *state,
                                              enum SearchDirection direction);
static void action_runtime_search_prompt_destroy(
    ActionRuntimeSearchPrompt *state);
static bool action_runtime_search_from_prompt_start(
    ActionRuntimeSearchPrompt *state, char *text, int32 text_len, bool *found,
    NcmError *ncm_error);
static bool action_runtime_search_prompt_apply(ActionRuntimeSearchPrompt *state,
                                               char *text, int32 text_len,
                                               bool *found,
                                               NcmError *ncm_error);
static bool action_runtime_search_prompt_hook(char *text, void *user);
static bool action_runtime_prompt_result(StrBuilder *result, NcPrompt *prompt,
                                         NcWindow *window);
static bool action_runtime_prompt_string(char *prefix, int32 prefix_len,
                                         char *initial_text, bool remember,
                                         NcPromptHook hook, void *hook_user,
                                         StrBuilder *result);
static bool action_runtime_confirm(char *message, int32 message_len);
static bool action_runtime_parse_seek_position(char *text, int32 text_len,
                                               int32 total, int32 *position);
static void action_runtime_print_format_string(char *format, int32 format_len,
                                               char *text, int32 text_len);
static bool action_runtime_toggle_crossfade(void);
static bool action_runtime_set_crossfade(void);
static bool action_runtime_set_volume(void);
static bool action_runtime_add_prompt(void);
static bool action_runtime_load_prompt(void);
static bool action_runtime_add_random_items(void);
static NcMenu *action_runtime_current_menu(void);
static enum NcMenuItemSource action_runtime_menu_item_source(NcMenu *menu);
static bool action_runtime_menu_has_items(void);
static bool action_runtime_menu_has_selectable_item(void);
static bool action_runtime_menu_has_selection(void);
static bool action_runtime_scroll_by_tag(enum NcmSongGetter getter, bool down);
static bool action_runtime_tag_scroll_available(enum NcmSongGetter getter);
static bool action_runtime_add_item_to_playlist(bool play);
static bool action_runtime_selected_songs(NcmSongArray *songs);
static bool action_runtime_has_selected_songs(void);
static bool action_runtime_current_song(NcmSong *song);
static bool action_runtime_add_selected_songs(bool play);
static bool action_runtime_add_playlist_editor_item(bool play);
static bool action_runtime_delete_playlist_items(void);
static bool action_runtime_delete_browser_items(void);
static bool action_runtime_browser_item_name(NcmMpdItem *item,
                                             StrBuilder *name);
static void action_runtime_print_renamed(char *prefix, int32 prefix_len,
                                         StrBuilder *name);
static bool action_runtime_delete_playlist_editor_items(void);
static bool action_runtime_delete_stored_playlists(void);
static bool action_runtime_clear_playlist(bool main_playlist);
static bool action_runtime_crop_playlist(bool main_playlist);
static bool action_runtime_move_selected_items(bool down);
static bool action_runtime_move_selected_items_to(void);
static bool action_runtime_move_playlist_editor_items_to(void);
static bool action_runtime_edit_playlist_name(void);
static bool action_runtime_playlist_editor_content_active(void);
static bool action_runtime_playlist_editor_playlists_active(void);
static bool action_runtime_playlist_editor_has_playlists(void);
static bool action_runtime_playlist_editor_has_content(void);
static bool action_runtime_reverse_playlist(void);
static bool action_runtime_shuffle_playlist(void);
static bool action_runtime_save_playlist(void);
static bool action_runtime_set_selected_items_priority(void);
static bool action_runtime_jump_to_position_in_song(void);
static bool action_runtime_select_album(void);
static bool action_runtime_select_found_items(void);
static bool action_runtime_previous_column_available(void);
static bool action_runtime_next_column_available(void);
static bool action_runtime_previous_column(void);
static bool action_runtime_next_column(void);
static bool action_runtime_enter_directory(void);
static bool action_runtime_jump_to_parent_directory(void);
static bool action_runtime_seek_relative(bool forward);
static bool action_runtime_jump_to_playing_song(void);
static bool action_runtime_jump_to_browser(void);
static bool action_runtime_jump_to_playlist_editor(void);
static bool action_runtime_jump_to_media_library(void);
static bool action_runtime_jump_to_tag_editor(void);
static bool action_runtime_edit_directory_name(void);
static bool action_runtime_toggle_display_mode(void);
static bool action_runtime_change_browse_mode(void);
static bool action_runtime_toggle_browser_sort_mode(void);
static bool action_runtime_toggle_library_tag_type(void);
static bool action_runtime_toggle_media_library_sort_mode(void);
static bool action_runtime_toggle_media_library_columns(void);
static char *action_runtime_replay_gain_mode_name(
    enum NcmMpdReplayGainMode mode);
static bool action_runtime_toggle_replay_gain_mode(void);
static bool action_runtime_save_tag_changes(void);
static bool action_runtime_edit_current_song(void);
static bool action_runtime_fetch_lyrics_background(void);
static bool action_runtime_edit_lyrics(void);
static bool action_runtime_toggle_lyrics_fetcher(void);
static bool action_runtime_refetch_lyrics(void);
static bool action_runtime_show_lyrics(void);
static bool action_runtime_show_artist_info(void);
static bool action_runtime_mouse_event(void);
static bool action_runtime_media_library_current_artist_tag(char **artist,
                                                            int32 *artist_len);
static bool action_runtime_toggle_screen_lock(void);

NcmActionRuntime *
ncm_action_runtime_global(void) {
    if (!action_global_runtime_initialized) {
        action_global_runtime = (NcmActionRuntime){0};
        action_global_runtime_initialized = true;
    }
    return &action_global_runtime;
}

bool
ncm_action_runtime_exit_requested(NcmActionRuntime *runtime) {
    runtime = action_runtime_or_global(runtime);
    return runtime->exit_requested;
}

void
ncm_action_runtime_request_exit(NcmActionRuntime *runtime) {
    runtime = action_runtime_or_global(runtime);
    runtime->exit_requested = true;
    return;
}

bool
ncm_action_runtime_can_run(NcmActionRuntime *runtime, enum NcmActionType type) {
    int32 hook_result;
    bool handled;

    runtime = action_runtime_or_global(runtime);
    hook_result = action_runtime_call_hook(runtime->can_run_hook, type,
                                           runtime->user);
    if (action_runtime_hook_allowed(hook_result, &handled)) {
        return true;
    }
    if (action_runtime_hook_denied(hook_result, &handled)) {
        return false;
    }

    return action_runtime_builtin_can_run(runtime, type);
}

bool
ncm_action_runtime_run(NcmActionRuntime *runtime, enum NcmActionType type) {
    int32 hook_result;
    bool handled;

    runtime = action_runtime_or_global(runtime);
    if (!ncm_action_runtime_can_run(runtime, type)) {
        return false;
    }

    hook_result = action_runtime_call_hook(runtime->run_hook, type,
                                           runtime->user);
    if (action_runtime_hook_allowed(hook_result, &handled)) {
        return true;
    }
    if (action_runtime_hook_denied(hook_result, &handled)) {
        return false;
    }

    return action_runtime_builtin_run(runtime, type);
}

static NcmActionRuntime *
action_runtime_or_global(NcmActionRuntime *runtime) {
    if (runtime) {
        return runtime;
    }
    return ncm_action_runtime_global();
}

static int32
action_runtime_call_hook(NcmActionRuntimeHook hook, enum NcmActionType type,
                         void *user) {
    if (hook == NULL) {
        return NCM_ACTION_RUNTIME_DEFER;
    }
    return hook(type, user);
}

static bool
action_runtime_hook_allowed(int32 result, bool *handled) {
    if (handled) {
        *handled = result != NCM_ACTION_RUNTIME_DEFER;
    }
    return result == NCM_ACTION_RUNTIME_ALLOW;
}

static bool
action_runtime_hook_denied(int32 result, bool *handled) {
    if (handled) {
        *handled = result != NCM_ACTION_RUNTIME_DEFER;
    }
    return result == NCM_ACTION_RUNTIME_DENY;
}

static bool
action_runtime_current_screen_is(enum ScreenType type) {
    NcScreen *screen;
    int32 nc_type;

    if ((screen = app_controller_current_screen()) == NULL) {
        return false;
    }

    nc_type = screen_type_to_nc_type(type);
    return nc_screen_type(screen) == nc_type;
}

static bool
action_runtime_switch_to_screen(enum ScreenType type) {
    if ((type != NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER)
        && action_runtime_current_screen_is(
            NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER)
        && !selected_items_adder_screen_return_to_previous(
            app_screen_selected_items_adder())) {
        return false;
    }

    switch (type) {
    case NCM_SCREEN_TYPE_BROWSER:
        app_screen_browser_register();
        app_screen_browser_switch_to();
        return true;
    case NCM_SCREEN_TYPE_HELP:
        app_screen_help_register();
        app_screen_help_switch_to();
        return true;
    case NCM_SCREEN_TYPE_LASTFM:
        app_screen_lastfm_register();
        app_screen_lastfm_switch_to();
        return true;
    case NCM_SCREEN_TYPE_LYRICS:
        app_screen_lyrics_register();
        app_screen_lyrics_switch_to();
        return true;
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        app_screen_media_library_register();
        app_screen_media_library_switch_to();
        return true;
#if defined(ENABLE_OUTPUTS)
    case NCM_SCREEN_TYPE_OUTPUTS:
        app_screen_outputs_register();
        app_screen_outputs_switch_to();
        return true;
#endif
    case NCM_SCREEN_TYPE_PLAYLIST:
        app_screen_playlist_register();
        app_screen_playlist_switch_to();
        return true;
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        app_screen_playlist_editor_register();
        app_screen_playlist_editor_switch_to();
        return true;
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
        app_screen_search_engine_register();
        app_screen_search_engine_switch_to();
        return true;
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
        app_screen_selected_items_adder_register();
        app_screen_selected_items_adder_switch_to();
        return true;
    case NCM_SCREEN_TYPE_SERVER_INFO:
        app_screen_server_info_register();
        app_screen_server_info_switch_to();
        return true;
    case NCM_SCREEN_TYPE_SONG_INFO:
        app_screen_song_info_register();
        app_screen_song_info_switch_to();
        return true;
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
        app_screen_sort_playlist_dialog_register();
        return app_screen_sort_playlist_dialog_switch_to();
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        app_screen_tag_editor_register();
        app_screen_tag_editor_switch_to();
        return true;
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
        app_screen_tiny_tag_editor_register();
        app_screen_tiny_tag_editor_switch_to();
        return true;
#endif
#if defined(ENABLE_VISUALIZER)
    case NCM_SCREEN_TYPE_VISUALIZER:
        return ncm_action_show_visualizer();
#endif
    case NCM_SCREEN_TYPE_COUNT:
        break;
    default:
        break;
    }

    return false;
}

bool
ncm_action_show_visualizer(void) {
#if defined(ENABLE_VISUALIZER)
    app_screen_visualizer_register();
    return app_screens_switch_to_type(NCM_SCREEN_TYPE_VISUALIZER);
#else
    return false;
#endif
}

bool
ncm_action_toggle_visualization_type(void) {
#if defined(ENABLE_VISUALIZER)
    if (!app_screen_visualizer_is_current()) {
        return false;
    }
    visualizer_screen_toggle_type(app_screen_visualizer());
    return true;
#else
    return false;
#endif
}

static bool
action_runtime_switch_to_next_screen(bool reverse) {
    ScreenTypeArray *sequence = &Config.screen_sequence;
    NcScreen *current;
    bool selected_items_adder = action_runtime_current_screen_is(
        NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER);
    int32 current_index;
    int32 next_index;

    if (selected_items_adder && Config.screen_switcher_previous) {
        return selected_items_adder_screen_return_to_previous(
            app_screen_selected_items_adder());
    }
    if (Config.screen_switcher_previous) {
        current = app_controller_previous_screen();
        if (current == NULL) {
            return false;
        }
        return nc_screen_switcher_switch_to(
            current, nc_screen_has_to_be_resized(current));
    }

    if (sequence->len <= 0) {
        return false;
    }

    if ((current = app_controller_current_screen()) == NULL) {
        return action_runtime_switch_to_screen(sequence->items[0]);
    }

    current_index = -1;
    for (int32 i = 0; i < sequence->len; i += 1) {
        if (screen_type_to_nc_type(sequence->items[i])
            == nc_screen_type(current)) {
            current_index = i;
            break;
        }
    }
    if (current_index < 0) {
        if (reverse) {
            next_index = sequence->len - 1;
        } else {
            next_index = 0;
        }
    } else if (reverse) {
        next_index = current_index - 1;
        if (next_index < 0) {
            next_index = sequence->len - 1;
        }
    } else {
        next_index = current_index + 1;
        if (next_index >= sequence->len) {
            next_index = 0;
        }
    }

    return action_runtime_switch_to_screen(sequence->items[next_index]);
}

static bool
action_runtime_mpd_error(NcmError *ncm_error) {
    if (ncm_error && ncm_error_is_set(ncm_error)) {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    ncm_error->message);
    }
    return false;
}

static bool
action_runtime_playlist_find_song(NcmSong *song, NcmSong **match) {
    PlaylistScreen *screen = app_screen_playlist();
    NcSongMenu *song_menu;
    NcMenu *menu;
    NcmSong *item;
    int32 count;

    if (match) {
        *match = NULL;
    }
    if (song == NULL) {
        return false;
    }

    song_menu = playlist_screen_song_menu(screen);
    if (song_menu == NULL) {
        return false;
    }
    menu = nc_song_menu_base(song_menu);
    count = nc_menu_all_item_count(menu);
    for (int32 i = 0; i < count; i += 1) {
        item = nc_song_menu_item_at(song_menu, NC_MENU_ITEMS_ALL, i);
        if ((item == NULL) || !ncm_song_equal(item, song)) {
            continue;
        }
        if (match) {
            *match = item;
        }
        return true;
    }

    return false;
}

static bool
action_runtime_playlist_remove_song(NcmSong *song, NcmError *ncm_error) {
    PlaylistScreen *screen = app_screen_playlist();
    NcSongMenu *song_menu;
    NcMenu *menu;
    NcmSong *item;
    int32 position;
    int32 count;
    bool ok;

    if (song == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing MPD song"));
        return false;
    }

    song_menu = playlist_screen_song_menu(screen);
    if (song_menu == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing playlist screen"));
        return false;
    }
    menu = nc_song_menu_base(song_menu);
    count = nc_menu_all_item_count(menu);

    ok = ncm_mpd_client_start_command_list(&global_mpd, ncm_error);
    for (int32 i = count; ok && (i > 0); i -= 1) {
        item = nc_song_menu_item_at(song_menu, NC_MENU_ITEMS_ALL, i - 1);
        if ((item == NULL) || !ncm_song_equal(item, song)) {
            continue;
        }
        position = ncm_song_position(item);
        if (position < 0) {
            continue;
        }
        ok = ncm_mpd_client_delete(&global_mpd, position, ncm_error);
    }
    if (ok) {
        ok = ncm_mpd_client_commit_command_list(&global_mpd, ncm_error);
    }
    if (!ok && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }

    return ok;
}

static bool
action_runtime_mpd_simple(
    bool (*func)(NcmMpdClient *client, NcmError *ncm_error)
) {
    NcmError ncm_error;

    ncm_error_clear(&ncm_error);
    if (!func(&global_mpd, &ncm_error)) {
        return action_runtime_mpd_error(&ncm_error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return true;
}

bool
ncm_action_add_song_to_playlist_with_mode(NcmSong *song, bool play,
                                          int32 position,
                                          enum SpaceAddMode space_add_mode) {
    NcmSong *match;
    NcmError ncm_error;
    StrBuilder formatted;
    StrBuilder message = {0};
    int32 id;
    bool ok;

    if (song == NULL) {
        return false;
    }
    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    ncm_error_clear(&ncm_error);
    match = NULL;
    if ((space_add_mode == NCM_SPACE_ADD_MODE_ADD_REMOVE)
        && action_runtime_playlist_find_song(song, &match)) {
        if (play) {
            ok = ncm_mpd_client_play_id(&global_mpd,
                                        ncm_song_id(match), &ncm_error);
        } else {
            ok = action_runtime_playlist_remove_song(song, &ncm_error);
        }
        if (!ok) {
            return action_runtime_mpd_error(&ncm_error);
        }
        (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
        return true;
    }

    id = -1;
    if (!ncm_mpd_client_add_song_value(&global_mpd, song, position, &id,
                                       &ncm_error)) {
        return action_runtime_mpd_error(&ncm_error);
    }

    formatted = ncm_format_render_string(&Config.song_status_format, song);
    SB_APPEND(&message, STRLIT("Added to playlist: "));
    SB_APPEND(&message, formatted.data, formatted.len);
    ncm_statusbar_print(Config.message_delay_time, message.data,
                        message.len);
    sb_free(&message);
    sb_free(&formatted);

    if (play && (id >= 0)) {
        if (!ncm_mpd_client_play_id(&global_mpd, id, &ncm_error)) {
            return action_runtime_mpd_error(&ncm_error);
        }
    }

    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return true;
}

bool
ncm_action_add_song_to_playlist(NcmSong *song, bool play, int32 position) {
    return ncm_action_add_song_to_playlist_with_mode(song, play, position,
                                                     Config.space_add_mode);
}

static bool
action_runtime_mpd_toggle(bool (*func)(NcmMpdClient *client, bool mode,
                                       NcmError *ncm_error),
                          bool current) {
    NcmError ncm_error;

    ncm_error_clear(&ncm_error);
    if (!func(&global_mpd, !current, &ncm_error)) {
        return action_runtime_mpd_error(&ncm_error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return true;
}

static bool
action_runtime_volume(int32 change) {
    NcmError ncm_error;

    ncm_error_clear(&ncm_error);
    if (!ncm_mpd_client_change_volume(&global_mpd, change, &ncm_error)) {
        return action_runtime_mpd_error(&ncm_error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return true;
}

static bool
action_runtime_update_database(void) {
    NcmStringView view;
    NcmError ncm_error;
    char *path = "/";

    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        view = browser_screen_current_directory(app_screen_browser());
        if (view.data) {
            path = view.data;
        } else {
            path = "";
        }
    }

#if defined(HAVE_TAGLIB_H)
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TAG_EDITOR)) {
        if (tag_editor_screen_current_dir(app_screen_tag_editor(), &view)) {
            path = view.data;
        }
    }
#endif

    ncm_error_clear(&ncm_error);
    if (!ncm_mpd_client_update_directory(&global_mpd, path, NULL, &ncm_error)) {
        return action_runtime_mpd_error(&ncm_error);
    }
    return true;
}

static bool
action_runtime_replay_song(void) {
    NcmError ncm_error;
    int32 position = ncm_status_state_current_song_position();

    if (position < 0) {
        return false;
    }

    ncm_error_clear(&ncm_error);
    if (!ncm_mpd_client_play_pos(&global_mpd, position, &ncm_error)) {
        return action_runtime_mpd_error(&ncm_error);
    }
    return true;
}

static bool
action_runtime_toggle_crossfade(void) {
    NcmError ncm_error;
    int32 seconds = Config.crossfade_time;

    ncm_error_clear(&ncm_error);
    if (ncm_status_state_crossfade()) {
        seconds = 0;
    }
    if (!ncm_mpd_client_set_crossfade(&global_mpd, seconds, &ncm_error)) {
        return action_runtime_mpd_error(&ncm_error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return true;
}

static bool
action_runtime_set_crossfade(void) {
    StrBuilder input = {0};
    NcmError ncm_error;
    int32 seconds;
    bool prompted;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    prompted = action_runtime_prompt_string(STRLIT("Set crossfade to: "),
                                            "", false, NULL, NULL, &input);
    if (!prompted) {
        sb_free(&input);
        return true;
    }

    ncm_error_clear(&ncm_error);
    if (!ncm_parse_int32(input.data, input.len, &seconds, &ncm_error)) {
        sb_free(&input);
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Crossfade must be a non-negative number");
        return true;
    }
    sb_free(&input);

    Config.crossfade_time = seconds;
    ncm_error_clear(&ncm_error);
    if (!ncm_mpd_client_set_crossfade(&global_mpd, seconds, &ncm_error)) {
        return action_runtime_mpd_error(&ncm_error);
    }
    return true;
}

static bool
action_runtime_set_volume(void) {
    NcmStringFormatArg arg;
    StrBuilder input = {0};
    NcmError ncm_error;
    int32 volume;
    bool prompted;

    if (!ncm_mpd_client_connected(&global_mpd)
        || (ncm_status_state_volume() < 0)) {
        return false;
    }

    prompted = action_runtime_prompt_string(STRLIT("Set volume to: "), "",
                                            false, NULL, NULL, &input);
    if (!prompted) {
        sb_free(&input);
        return true;
    }

    ncm_error_clear(&ncm_error);
    if (!ncm_parse_int32(input.data, input.len, &volume, &ncm_error)
        || (volume > 100)) {
        sb_free(&input);
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Volume must be between 0 and 100");
        return true;
    }
    sb_free(&input);

    ncm_error_clear(&ncm_error);
    if (!ncm_mpd_client_set_volume(&global_mpd, volume, &ncm_error)) {
        return action_runtime_mpd_error(&ncm_error);
    }
    arg = ncm_string_format_arg_u64((uint64)volume);
    ncm_statusbar_format(Config.message_delay_time,
                         STRLIT("Volume set to %1%%%"), &arg, 1);
    return true;
}

static bool
action_runtime_add_random_items(void) {
    NcmStatusbarScopedLock scoped_lock;
    NcmStringFormatArg args[3];
    StrBuilder input = {0};
    StrBuilder prompt;
    NcmError ncm_error;
    NcWindow *window;
    char values[] = {
        's',
        'a',
        'A',
        'b',
    };
    char tag_name[32];
    char *plural;
    char *source_name;
    int32 count;
    int32 source_name_len;
    int32 number;
    enum mpd_tag_type tag_type = MPD_TAG_ARTIST;
    char random_type = 0;
    bool prompted = false;
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    ncm_statusbar_scoped_lock_init(&scoped_lock);
    if ((window = ncm_statusbar_put())) {
        nc_window_print_data(window,
                             STRLIT("Add random? [s]ongs/[a]rtists/"
                                         "album [A]rtists/al[b]ums "));
        prompted = ncm_statusbar_prompt_return_one_of(
            window, values, LENGTH(values), &random_type);
    }
    ncm_statusbar_scoped_lock_destroy(&scoped_lock);
    if (!prompted) {
        return true;
    }

    if (random_type == 's') {
        source_name = "song";
        source_name_len = STRLIT_LEN("song");
    } else {
        tag_type = ncm_char_to_tag_type(random_type);
        source_name = ncm_tag_type_name(tag_type);
        source_name_len = optional_strlen32(source_name);
        if (source_name_len >= SIZEOF(tag_name)) {
            return false;
        }
        memcpy64(tag_name, source_name, source_name_len);
        tag_name[source_name_len] = '\0';
        ncm_string_lowercase_ascii(tag_name, source_name_len);
        source_name = tag_name;
    }

    args[0] = ncm_string_format_arg_string(source_name, source_name_len);
    prompt = ncm_string_format_make(STRLIT("Number of random %1%s: "),
                                    args, 1);
    prompted = action_runtime_prompt_string(prompt.data, prompt.len, "", false,
                                            NULL, NULL, &input);
    sb_free(&prompt);
    if (!prompted) {
        sb_free(&input);
        return true;
    }

    ncm_error_clear(&ncm_error);
    if (!ncm_parse_int32(input.data, input.len, &number, &ncm_error)) {
        sb_free(&input);
        ncm_statusbar_print_cstring(
            Config.message_delay_time,
            "Random item count must be a non-negative number");
        return true;
    }
    sb_free(&input);
    count = number;
    if (count <= 0) {
        return true;
    }

    ncm_error_clear(&ncm_error);
    if (random_type == 's') {
        success = ncm_mpd_client_add_random_songs(
            &global_mpd, count, Config.random_exclude_pattern,
            Config.random_exclude_pattern_len, &global_random, &ncm_error);
    } else {
        success = ncm_mpd_client_add_random_tag(&global_mpd, tag_type, count,
                                                &global_random, &ncm_error);
    }
    if (!success) {
        return action_runtime_mpd_error(&ncm_error);
    }

    if (count == 1) {
        plural = "";
    } else {
        plural = "s";
    }
    args[0] = ncm_string_format_arg_i64(count);
    args[1] = ncm_string_format_arg_string(source_name, source_name_len);
    args[2] = ncm_string_format_arg_cstring(plural);
    ncm_statusbar_format(Config.message_delay_time,
                         STRLIT("%1% random %2%%3% added to playlist"),
                         args, LENGTH(args));
    return true;
}

static void
action_runtime_print_toggle(char *format, int32 format_len, char *value) {
    NcmStringFormatArg arg = ncm_string_format_arg_cstring(value);

    ncm_statusbar_format(Config.message_delay_time, format, format_len,
                         &arg, 1);
    return;
}

static bool
action_runtime_toggle_interface(void) {
    NcmStatusbarScopedLock scoped_lock;

    switch (Config.design) {
    case NCM_DESIGN_CLASSIC:
        Config.design = NCM_DESIGN_ALTERNATIVE;
        Config.statusbar_visibility = false;
        break;
    case NCM_DESIGN_ALTERNATIVE:
        Config.design = NCM_DESIGN_CLASSIC;
        Config.statusbar_visibility = ui_state_statusbar_visibility_baseline();
        break;
    case NCM_DESIGN_COUNT:
        return false;
    default:
        return false;
    }

    ncmpcpp_resize_screen(false);
    ncm_progressbar_scoped_lock_init(&scoped_lock);
    ncm_progressbar_scoped_lock_destroy(&scoped_lock);
    ncm_status_changes_mixer();
    ncm_status_changes_elapsed_time(false);
    action_runtime_print_toggle(STRLIT("User interface: %1%"),
                                ncm_design_str(Config.design));
    return true;
}

static bool
action_runtime_toggle_separators_between_albums(void) {
    Config.playlist_separate_albums = !Config.playlist_separate_albums;
    app_controller_request_current_screen_resize();
    if (Config.playlist_separate_albums) {
        action_runtime_print_toggle(
            STRLIT("Separators between albums: %1%"), "on");
    } else {
        action_runtime_print_toggle(
            STRLIT("Separators between albums: %1%"), "off");
    }
    return true;
}

static bool
action_runtime_toggle_lyrics_update_on_song_change(void) {
    if (!app_screen_lyrics_is_current()) {
        return false;
    }
    Config.now_playing_lyrics = !Config.now_playing_lyrics;
    if (Config.now_playing_lyrics) {
        action_runtime_print_toggle(
            STRLIT("Update lyrics if song changes: %1%"), "on");
    } else {
        action_runtime_print_toggle(
            STRLIT("Update lyrics if song changes: %1%"), "off");
    }
    return true;
}

static bool
action_runtime_toggle_fetching_lyrics_in_background(void) {
    Config.fetch_lyrics_in_background = !Config.fetch_lyrics_in_background;
    if (Config.fetch_lyrics_in_background) {
        action_runtime_print_toggle(
            STRLIT("Fetching lyrics for playing songs in background: %1%"),
            "on");
    } else {
        action_runtime_print_toggle(
            STRLIT("Fetching lyrics for playing songs in background: %1%"),
            "off");
    }
    return true;
}

static bool
action_runtime_toggle_add_mode(void) {
    char *mode_desc;

    switch (Config.space_add_mode) {
    case NCM_SPACE_ADD_MODE_ADD_REMOVE:
        Config.space_add_mode = NCM_SPACE_ADD_MODE_ALWAYS_ADD;
        mode_desc = "always add an item to playlist";
        break;
    case NCM_SPACE_ADD_MODE_ALWAYS_ADD:
        Config.space_add_mode = NCM_SPACE_ADD_MODE_ADD_REMOVE;
        mode_desc = "add an item to playlist or remove if already added";
        break;
    case NCM_SPACE_ADD_MODE_COUNT:
    default:
        return false;
    }
    action_runtime_print_toggle(STRLIT("Add mode: %1%"), mode_desc);
    return true;
}

static bool
action_runtime_toggle_mouse(void) {
    Config.mouse_support = !Config.mouse_support;
    if (Config.mouse_support) {
        nc_mouse_enable();
        action_runtime_print_toggle(STRLIT("Mouse support %1%"),
                                    "enabled");
    } else {
        nc_mouse_disable();
        action_runtime_print_toggle(STRLIT("Mouse support %1%"),
                                    "disabled");
    }
    return true;
}

static bool
action_runtime_toggle_bitrate_visibility(void) {
    Config.display_bitrate = !Config.display_bitrate;
    if (Config.display_bitrate) {
        action_runtime_print_toggle(STRLIT("Bitrate visibility %1%"),
                                    "enabled");
    } else {
        action_runtime_print_toggle(STRLIT("Bitrate visibility %1%"),
                                    "disabled");
    }
    return true;
}

static void
action_runtime_print_format_string(char *format, int32 format_len, char *text,
                                   int32 text_len) {
    NcmStringFormatArg arg = ncm_string_format_arg_string(text, text_len);

    ncm_statusbar_format(Config.message_delay_time, format, format_len,
                         &arg, 1);
    return;
}

bool
ncm_action_immediate_command_prompt_should_stop(StrBuilder *previous,
                                                char *text, int32 text_len) {
    NcmCommand *command;

    if (previous == NULL) {
        return false;
    }
    if (text == NULL) {
        text = "";
        text_len = 0;
    }

    if ((previous->len == text_len)
        && ((text_len == 0)
            || (memcmp64(previous->data, text, text_len) == 0))) {
        return false;
    }

    if (sb_set(previous, text, text_len) < 0) {
        return false;
    }

    command = ncm_bindings_configuration_find_command(&Bindings, text,
                                                      text_len);
    if (command && command->immediate) {
        return true;
    }
    return false;
}

static bool
action_runtime_command_prompt_hook(char *text, void *user) {
    ActionRuntimeCommandPrompt *state = user;
    int32 text_len = optional_strlen32(text);

    if (!ncm_statusbar_main_hook(text, text_len)) {
        return false;
    }
    if (ncm_action_immediate_command_prompt_should_stop(&state->previous, text,
                                                        text_len)) {
        return false;
    }
    return true;
}

static bool
action_runtime_filter_prompt_hook(char *text, void *user) {
    NcmError ncm_error;
    int32 text_len = optional_strlen32(text);

    (void)user;
    if (!ncm_statusbar_main_hook(text, text_len)) {
        return false;
    }

    ncm_error_clear(&ncm_error);
    (void)current_screen_apply_filter(text, text_len, &ncm_error);
    return true;
}

static void
action_runtime_search_prompt_init(ActionRuntimeSearchPrompt *state,
                                  enum SearchDirection direction) {
    NcMenu *menu;
    int32 count;
    int32 highlight;

    ncm_search_prompt_state_init(state, direction);

    if ((menu = action_runtime_current_menu()) == NULL) {
        return;
    }

    count = nc_menu_item_count(menu);
    highlight = nc_menu_highlight(menu);
    if ((highlight < 0) || (highlight >= count)) {
        return;
    }

    ncm_search_prompt_state_set_start_position(state, highlight);
    return;
}

static void
action_runtime_search_prompt_destroy(ActionRuntimeSearchPrompt *state) {
    ncm_search_prompt_state_destroy(state);
    return;
}

static bool
action_runtime_search_from_prompt_start(ActionRuntimeSearchPrompt *state,
                                        char *text, int32 text_len, bool *found,
                                        NcmError *ncm_error) {
    NcMenu *menu = action_runtime_current_menu();
    int32 old_beginning = 0;
    int32 old_highlight = 0;
    int32 count;
    bool restore = false;

    if (menu && state->has_start_position) {
        count = nc_menu_item_count(menu);
        if ((state->start_position >= 0) && (state->start_position < count)) {
            old_beginning = menu->beginning;
            old_highlight = menu->highlight;
            menu->highlight = state->start_position;
            restore = true;
        }
    }

    *found = current_screen_search(state->direction, text, text_len,
                                   Config.wrapped_search, false, ncm_error);
    if (restore && !*found && !ncm_error_is_set(ncm_error)) {
        NcScreen *screen;

        menu->beginning = old_beginning;
        menu->highlight = old_highlight;
        if ((screen = app_controller_current_screen())) {
            nc_screen_refresh_window(screen);
        }
    }
    return !ncm_error_is_set(ncm_error);
}

static bool
action_runtime_search_prompt_apply(ActionRuntimeSearchPrompt *state, char *text,
                                   int32 text_len, bool *found,
                                   NcmError *ncm_error) {
    bool last_found;
    bool ok;

    if (text == NULL) {
        text = "";
        text_len = 0;
    }
    if (ncm_search_prompt_state_cached_result(state, text, text_len,
                                              &last_found)) {
        if (found) {
            *found = last_found;
        }
        return true;
    }

    last_found = false;
    ncm_error_clear(ncm_error);
    ok = action_runtime_search_from_prompt_start(state, text, text_len,
                                                 &last_found, ncm_error);
    if (!ncm_search_prompt_state_finish_result(state, text, text_len, ok,
                                               last_found)) {
        return false;
    }
    if (found) {
        *found = last_found;
    }
    return ok;
}

static bool
action_runtime_search_prompt_hook(char *text, void *user) {
    ActionRuntimeSearchPrompt *state = user;
    NcmError ncm_error;
    int32 text_len = optional_strlen32(text);

    if (!ncm_statusbar_main_hook(text, text_len)) {
        return false;
    }

    ncm_error_clear(&ncm_error);
    (void)action_runtime_search_prompt_apply(state, text, text_len, NULL,
                                             &ncm_error);
    return true;
}

static bool
action_runtime_prompt_result(StrBuilder *result, NcPrompt *prompt,
                             NcWindow *window) {
    enum NcPromptStatus status;
    char *text = NULL;
    int32 text_len;
    bool ok;

    if (result == NULL) {
        return false;
    }

    if (window == NULL) {
        return false;
    }

    status = nc_window_prompt(window, prompt, &text);
    if ((status != NC_PROMPT_ACCEPTED) || (text == NULL)) {
        nc_window_prompt_result_destroy(text);
        return false;
    }

    text_len = optional_strlen32(text);
    ok = sb_set(result, text, text_len) >= 0;
    nc_window_prompt_result_destroy(text);
    return ok;
}

static bool
action_runtime_prompt_string(char *prefix, int32 prefix_len, char *initial_text,
                             bool remember, NcPromptHook hook, void *hook_user,
                             StrBuilder *result) {
    NcmStatusbarScopedLock scoped_lock;
    NcPrompt prompt;
    NcWindow *window;
    bool ok = false;

    if (initial_text == NULL) {
        initial_text = "";
    }

    ncm_statusbar_scoped_lock_init(&scoped_lock);
    if ((window = ncm_statusbar_put())) {
        nc_window_print_data(window, prefix, prefix_len);
        prompt = (NcPrompt){0};
        prompt.initial_text = initial_text;
        prompt.width = -1;
        prompt.hook = hook;
        prompt.hook_user_data = hook_user;
        prompt.encrypted = false;
        prompt.remember = remember;
        ok = action_runtime_prompt_result(result, &prompt, window);
    }
    ncm_statusbar_scoped_lock_destroy(&scoped_lock);
    return ok;
}

static bool
action_runtime_confirm(char *message, int32 message_len) {
    NcmStatusbarScopedLock scoped_lock;
    NcWindow *window;
    char values[] = {
        'y',
        'n',
    };
    char answer = 'n';
    bool prompted = false;

    ncm_statusbar_scoped_lock_init(&scoped_lock);
    if ((window = ncm_statusbar_put())) {
        nc_window_print_data(window, message, message_len);
        nc_window_print_data(window, STRLIT(" [y/n] "));
        prompted = ncm_statusbar_prompt_return_one_of(window, values,
                                                      LENGTH(values), &answer);
    }
    ncm_statusbar_scoped_lock_destroy(&scoped_lock);

    if (!prompted || (answer == 'n')) {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Action cancelled");
        return false;
    }
    return true;
}

static bool
action_runtime_parse_seek_position(char *text, int32 text_len, int32 total,
                                   int32 *position) {
    NcmError ncm_error;
    int32 first;
    int32 second;
    int32 third;
    int32 result;
    int32 first_colon = -1;
    int32 second_colon = -1;
    int32 number_len;

    if ((text == NULL) || (text_len <= 0) || (position == NULL)) {
        return false;
    }

    for (int32 i = 0; i < text_len; i += 1) {
        if (text[i] != ':') {
            continue;
        }
        if (first_colon < 0) {
            first_colon = i;
        } else if (second_colon < 0) {
            second_colon = i;
        } else {
            return false;
        }
    }

    ncm_error_clear(&ncm_error);
    if (first_colon >= 0) {
        if ((first_colon == 0) || (first_colon == text_len - 1)) {
            return false;
        }
        if (second_colon < 0) {
            if ((text_len - first_colon - 1) != 2) {
                return false;
            }
            if (!ncm_parse_int32(text, first_colon, &first, &ncm_error)
                || !ncm_parse_int32(text + first_colon + 1, 2, &second,
                                    &ncm_error)
                || (second > 60)) {
                return false;
            }
            result = first*60 + second;
        } else {
            if (((second_colon - first_colon - 1) != 2)
                || ((text_len - second_colon - 1) != 2)) {
                return false;
            }
            if (!ncm_parse_int32(text, first_colon, &first, &ncm_error)
                || !ncm_parse_int32(text + first_colon + 1, 2, &second,
                                    &ncm_error)
                || !ncm_parse_int32(text + second_colon + 1, 2, &third,
                                    &ncm_error)
                || (second > 60) || (third > 60)) {
                return false;
            }
            result = first*3600 + second*60 + third;
        }
        if (result > MAXOF(*position)) {
            return false;
        }
        *position = result;
        return true;
    }

    number_len = text_len;
    if (text[text_len - 1] == 's') {
        number_len -= 1;
        if (number_len <= 0) {
            return false;
        }
        if (!ncm_parse_int32(text, number_len, &first, &ncm_error)) {
            return false;
        }
        *position = first;
        return true;
    }
    if (text[text_len - 1] == '%') {
        number_len -= 1;
    }
    if (number_len <= 0) {
        return false;
    }
    if (!ncm_parse_int32(text, number_len, &first, &ncm_error)
        || (first > 100)) {
        return false;
    }
    *position = (first*total) / 100;
    return true;
}

static bool
action_runtime_execute_command(void) {
    ActionRuntimeCommandPrompt state;
    StrBuilder command_name = {0};
    NcmCommand *command;
    bool prompted;
    bool result;

    state.previous = (StrBuilder){0};
    prompted = action_runtime_prompt_string(STRLIT(":"), "", true,
                                            action_runtime_command_prompt_hook,
                                            &state, &command_name);
    if (!prompted && (state.previous.len > 0)) {
        sb_copy(&command_name, &state.previous);
        prompted = true;
    }
    sb_free(&state.previous);

    if (!prompted) {
        sb_free(&command_name);
        return true;
    }

    command = ncm_bindings_configuration_find_command(
        &Bindings, command_name.data, command_name.len);
    if (command == NULL) {
        action_runtime_print_format_string(
            STRLIT("No command named \"%1%\""), command_name.data,
            command_name.len);
        sb_free(&command_name);
        return true;
    }

    action_runtime_print_format_string(STRLIT("Executing %1%..."),
                                       command_name.data, command_name.len);
    if ((result = ncmpcpp_execute_binding(&command->binding))) {
        action_runtime_print_format_string(
            STRLIT("Execution of command \"%1%\" successful."),
            command_name.data, command_name.len);
    } else {
        action_runtime_print_format_string(
            STRLIT("Execution of command \"%1%\" unsuccessful."),
            command_name.data, command_name.len);
    }

    sb_free(&command_name);
    return result;
}

static bool
action_runtime_save_playlist(void) {
    StrBuilder name = {0};
    NcmError ncm_error;
    bool prompted;
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    prompted = action_runtime_prompt_string(STRLIT("Save playlist as: "),
                                            "", false, NULL, NULL, &name);
    if (!prompted) {
        sb_free(&name);
        return true;
    }

    ncm_error_clear(&ncm_error);
    success = ncm_mpd_client_save_playlist(&global_mpd, name.data, &ncm_error);
    if (!success
        && (ncm_mpd_client_server_error_code(&global_mpd)
            == MPD_SERVER_ERROR_EXIST)) {
        StrBuilder question = {0};

        SB_APPEND(&question, STRLIT("Playlist \""));
        SB_APPEND(&question, name.data, name.len);
        SB_APPEND(&question, STRLIT("\" already exists, overwrite?"));
        success = action_runtime_confirm(question.data, question.len);
        sb_free(&question);
        if (!success) {
            sb_free(&name);
            return true;
        }

        ncm_error_clear(&ncm_error);
        success = ncm_mpd_client_delete_playlist(&global_mpd, name.data,
                                                 &ncm_error);
        if (success) {
            success = ncm_mpd_client_save_playlist(&global_mpd, name.data,
                                                   &ncm_error);
        }
        if (success) {
            ncm_statusbar_print_cstring(Config.message_delay_time,
                                        "Playlist overwritten");
        }
    } else if (success) {
        action_runtime_print_format_string(
            STRLIT("Playlist saved as \"%1%\""), name.data, name.len);
    }

    sb_free(&name);
    if (!success) {
        return action_runtime_mpd_error(&ncm_error);
    }
    return true;
}

static bool
action_runtime_apply_filter(void) {
    NcmStringView current_filter;
    StrBuilder filter = {0};
    StrBuilder previous_filter = {0};
    NcmError ncm_error;
    bool old_autocenter_mode;
    bool prompted;

    if (!current_screen_allows_filter()) {
        return false;
    }

    current_filter = current_screen_current_filter();
    if (current_filter.data && (current_filter.len > 0)) {
        sb_set(&filter, current_filter.data, current_filter.len);
        sb_copy(&previous_filter, &filter);
        ncm_error_clear(&ncm_error);
        (void)current_screen_apply_filter(filter.data, filter.len, &ncm_error);
    }

    old_autocenter_mode = Config.autocenter_mode;
    Config.autocenter_mode = false;
    prompted = action_runtime_prompt_string(
        STRLIT("Apply filter: "), filter.data, false,
        action_runtime_filter_prompt_hook, NULL, &filter);
    Config.autocenter_mode = old_autocenter_mode;

    if (!prompted) {
        ncm_error_clear(&ncm_error);
        (void)current_screen_apply_filter(previous_filter.data,
                                          previous_filter.len, &ncm_error);
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Action cancelled");
        sb_free(&previous_filter);
        sb_free(&filter);
        return true;
    }

    ncm_error_clear(&ncm_error);
    (void)current_screen_apply_filter(filter.data, filter.len, &ncm_error);
    if (filter.len == 0) {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Filtering disabled");
    } else {
        action_runtime_print_format_string(STRLIT("Using filter \"%1%\""),
                                           filter.data, filter.len);
    }

    sb_free(&previous_filter);
    sb_free(&filter);
    return true;
}

static bool
action_runtime_find(void) {
    StrBuilder token = {0};
    NcmError ncm_error;
    bool found;
    bool prompted;

    if (!app_screen_help_is_current()
        && !app_screen_lastfm_is_current()
        && !app_screen_lyrics_is_current()) {
        return false;
    }

    prompted = action_runtime_prompt_string(STRLIT("Find: "), "", false,
                                            NULL, NULL, &token);
    if (!prompted) {
        sb_free(&token);
        return true;
    }

    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Searching...");
    ncm_error_clear(&ncm_error);
    found = current_screen_search(NCM_SEARCH_DIRECTION_FORWARD, token.data,
                                  token.len, false, false, &ncm_error);
    if (ncm_error_is_set(&ncm_error)) {
        sb_free(&token);
        return action_runtime_mpd_error(&ncm_error);
    }

    if ((token.len == 0) || found) {
        ncm_statusbar_print_cstring(Config.message_delay_time, "Done");
    } else {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "No matching patterns found");
    }

    sb_free(&token);
    return true;
}

static bool
action_runtime_find_item(enum SearchDirection direction) {
    ActionRuntimeSearchPrompt state;
    NcmStringView current_constraint;
    StrBuilder constraint = {0};
    StrBuilder previous_constraint = {0};
    NcmError ncm_error;
    bool old_autocenter_mode;
    bool prompted;
    char prompt[64];
    int32 prompt_len;

    if (!current_screen_allows_search()) {
        return false;
    }

    action_runtime_search_prompt_init(&state, direction);
    current_constraint = current_screen_current_search_constraint();
    if (current_constraint.data && (current_constraint.len > 0)) {
        sb_set(&previous_constraint, current_constraint.data,
               current_constraint.len);
    }

    prompt_len = SNPRINTF(prompt, "Find %s: ",
                          ncm_search_direction_str(direction));
    if (prompt_len < 0) {
        prompt_len = 0;
    }
    if (prompt_len >= SIZEOF(prompt)) {
        prompt_len = SIZEOF(prompt) - 1;
    }

    old_autocenter_mode = Config.autocenter_mode;
    Config.autocenter_mode = false;
    prompted = action_runtime_prompt_string(prompt, prompt_len, "", false,
                                            action_runtime_search_prompt_hook,
                                            &state, &constraint);
    Config.autocenter_mode = old_autocenter_mode;

    if (!prompted) {
        if (previous_constraint.len == 0) {
            current_screen_clear_search_constraint();
        } else {
            ncm_error_clear(&ncm_error);
            (void)current_screen_search(direction, previous_constraint.data,
                                        previous_constraint.len,
                                        Config.wrapped_search, false,
                                        &ncm_error);
        }
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Action cancelled");
        action_runtime_search_prompt_destroy(&state);
        sb_free(&previous_constraint);
        sb_free(&constraint);
        return true;
    }

    if (constraint.len == 0) {
        current_screen_clear_search_constraint();
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Constraint unset");
    } else {
        if (!ncm_search_prompt_state_cached_result(
            &state, constraint.data, constraint.len, NULL)) {
            ncm_error_clear(&ncm_error);
            if (!action_runtime_search_prompt_apply(
                &state, constraint.data, constraint.len, NULL,
                &ncm_error)) {
                action_runtime_search_prompt_destroy(&state);
                sb_free(&previous_constraint);
                sb_free(&constraint);
                if (ncm_error_is_set(&ncm_error)) {
                    return action_runtime_mpd_error(&ncm_error);
                }
                return false;
            }
        }
        action_runtime_print_format_string(
            STRLIT("Using constraint \"%1%\""), constraint.data,
            constraint.len);
    }

    action_runtime_search_prompt_destroy(&state);
    sb_free(&previous_constraint);
    sb_free(&constraint);
    return true;
}

static bool
action_runtime_repeat_search(enum SearchDirection direction) {
    NcmStringView constraint;
    NcmError ncm_error;

    if (!current_screen_allows_search()) {
        return false;
    }

    constraint = current_screen_current_search_constraint();
    if ((constraint.data == NULL) || (constraint.len <= 0)) {
        return true;
    }

    ncm_error_clear(&ncm_error);
    (void)current_screen_search(direction, constraint.data, constraint.len,
                                Config.wrapped_search, true, &ncm_error);
    if (ncm_error_is_set(&ncm_error)) {
        return action_runtime_mpd_error(&ncm_error);
    }
    return true;
}

static enum NcMenuItemSource
action_runtime_menu_item_source(NcMenu *menu) {
    if (menu && nc_menu_is_filtered(menu)) {
        return NC_MENU_ITEMS_FILTERED;
    }
    return NC_MENU_ITEMS_ALL;
}

static NcMenu *
action_runtime_current_menu(void) {
    switch (app_screens_current_type()) {
    case NCM_SCREEN_TYPE_BROWSER:
        return browser_screen_menu(app_screen_browser());
    case NCM_SCREEN_TYPE_PLAYLIST:
        return playlist_screen_menu(app_screen_playlist());
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        return playlist_editor_screen_active_menu(
            app_screen_playlist_editor());
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
        return search_engine_screen_menu(app_screen_search_engine());
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        return media_library_screen_active_menu(app_screen_media_library());
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
        return selected_items_adder_screen_active_menu(
            app_screen_selected_items_adder());
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
        return nc_editor_sort_menu_base(sort_playlist_dialog_menu(
            app_screen_sort_playlist_dialog()));
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        return tag_editor_screen_active_menu(app_screen_tag_editor());
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
        return nc_editor_buffer_menu_base(tiny_tag_editor_screen_rows(
            app_screen_tiny_tag_editor()));
#endif
    case NCM_SCREEN_TYPE_HELP:
    case NCM_SCREEN_TYPE_LASTFM:
    case NCM_SCREEN_TYPE_LYRICS:
#if defined(ENABLE_OUTPUTS)
    case NCM_SCREEN_TYPE_OUTPUTS:
#endif
    case NCM_SCREEN_TYPE_SERVER_INFO:
    case NCM_SCREEN_TYPE_SONG_INFO:
#if defined(ENABLE_VISUALIZER)
    case NCM_SCREEN_TYPE_VISUALIZER:
#endif
    case NCM_SCREEN_TYPE_COUNT:
        break;
    default:
        break;
    }
    return NULL;
}

static bool
action_runtime_menu_has_items(void) {
    NcMenu *menu;

    if ((menu = action_runtime_current_menu()) == NULL) {
        return false;
    }
    return !nc_menu_empty(menu);
}

static bool
action_runtime_menu_has_selectable_item(void) {
    NcMenu *menu;

    if ((menu = action_runtime_current_menu()) == NULL) {
        return false;
    }
    return nc_menu_current_is_selectable(menu);
}

static bool
action_runtime_menu_has_selection(void) {
    NcMenu *menu;

    if ((menu = action_runtime_current_menu()) == NULL) {
        return false;
    }
    return nc_menu_has_selected(menu);
}

static int32
action_runtime_current_menu_height(void) {
    NcWindow *window;

    switch (app_screens_current_type()) {
    case NCM_SCREEN_TYPE_BROWSER:
        window = browser_screen_window(app_screen_browser());
        break;
    case NCM_SCREEN_TYPE_PLAYLIST:
        window = playlist_screen_window(app_screen_playlist());
        break;
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        window = playlist_editor_screen_active_window(
            app_screen_playlist_editor());
        break;
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
        window = search_engine_screen_window(app_screen_search_engine());
        break;
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        window = media_library_screen_active_window(app_screen_media_library());
        break;
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        window = tag_editor_screen_active_window(app_screen_tag_editor());
        break;
#endif
    case NCM_SCREEN_TYPE_HELP:
    case NCM_SCREEN_TYPE_LASTFM:
    case NCM_SCREEN_TYPE_LYRICS:
#if defined(ENABLE_OUTPUTS)
    case NCM_SCREEN_TYPE_OUTPUTS:
#endif
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
    case NCM_SCREEN_TYPE_SERVER_INFO:
    case NCM_SCREEN_TYPE_SONG_INFO:
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
#endif
#if defined(ENABLE_VISUALIZER)
    case NCM_SCREEN_TYPE_VISUALIZER:
#endif
    case NCM_SCREEN_TYPE_COUNT:
    default:
        window = NULL;
        break;
    }

    if (window == NULL) {
        return ui_state_main_height();
    }
    return nc_window_height(window);
}

static NcMenu *
action_runtime_current_tag_scroll_menu(void) {
    switch (app_screens_current_type()) {
    case NCM_SCREEN_TYPE_BROWSER:
        return browser_screen_menu(app_screen_browser());
    case NCM_SCREEN_TYPE_PLAYLIST:
        return playlist_screen_menu(app_screen_playlist());
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR: {
        PlaylistEditorScreen *playlist_editor = app_screen_playlist_editor();

        if (!action_runtime_playlist_editor_content_active()) {
            return NULL;
        }
        return nc_song_menu_base(playlist_editor_screen_content(
            playlist_editor));
    }
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
        return search_engine_screen_menu(app_screen_search_engine());
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY: {
        MediaLibraryScreen *media_library = app_screen_media_library();

        if (media_library_screen_active_column(media_library)
            != MEDIA_LIBRARY_COLUMN_SONGS) {
            return NULL;
        }
        return media_library_screen_active_menu(media_library);
    }
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        if (app_screen_tag_editor()->active_focus
            != TAG_EDITOR_FOCUS_TAGS) {
            return NULL;
        }
        return tag_editor_screen_active_menu(app_screen_tag_editor());
#endif
    case NCM_SCREEN_TYPE_HELP:
    case NCM_SCREEN_TYPE_LASTFM:
    case NCM_SCREEN_TYPE_LYRICS:
#if defined(ENABLE_OUTPUTS)
    case NCM_SCREEN_TYPE_OUTPUTS:
#endif
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
    case NCM_SCREEN_TYPE_SERVER_INFO:
    case NCM_SCREEN_TYPE_SONG_INFO:
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
#endif
#if defined(ENABLE_VISUALIZER)
    case NCM_SCREEN_TYPE_VISUALIZER:
#endif
    case NCM_SCREEN_TYPE_COUNT:
    default:
        break;
    }
    return NULL;
}

static StrBuilder
action_runtime_song_tag_buffer(NcmSong *song, enum NcmSongGetter getter) {
    if (song == NULL) {
        StrBuilder empty = {0};

        return empty;
    }
    return ncm_song_tags_buffer(song, getter, Config.tags_separator,
                                Config.tags_separator_len,
                                Config.show_duplicate_tags);
}

static bool
action_runtime_song_tag_at(int32 pos, enum NcmSongGetter getter,
                           StrBuilder *tag) {
    NcmMpdItem *item;
    NcSearchRow *row;
    NcMenu *menu;
    NcmSong *song;

    if (tag == NULL) {
        return false;
    }

    switch (app_screens_current_type()) {
    case NCM_SCREEN_TYPE_BROWSER:
        menu = browser_screen_menu(app_screen_browser());
        if (((item = nc_menu_active_item_at(menu, pos)) == NULL)
            || (ncm_mpd_item_kind(item) != NCM_MPD_ITEM_SONG)) {
            return false;
        }
        *tag = action_runtime_song_tag_buffer(ncm_mpd_item_song(item), getter);
        return true;
    case NCM_SCREEN_TYPE_PLAYLIST:
        menu = playlist_screen_menu(app_screen_playlist());
        if ((song = nc_menu_active_item_at(menu, pos)) == NULL) {
            return false;
        }
        *tag = action_runtime_song_tag_buffer(song, getter);
        return true;
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        if (!action_runtime_playlist_editor_content_active()) {
            return false;
        }
        menu = nc_song_menu_base(playlist_editor_screen_content(
            app_screen_playlist_editor()));
        if ((song = nc_menu_active_item_at(menu, pos)) == NULL) {
            return false;
        }
        *tag = action_runtime_song_tag_buffer(song, getter);
        return true;
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
        menu = search_engine_screen_menu(app_screen_search_engine());
        if (((row = nc_menu_active_item_at(menu, pos)) == NULL)
            || !row->is_song) {
            return false;
        }
        *tag = action_runtime_song_tag_buffer(&row->song, getter);
        return true;
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        if (media_library_screen_active_column(app_screen_media_library())
            != MEDIA_LIBRARY_COLUMN_SONGS) {
            return false;
        }
        menu = media_library_screen_active_menu(app_screen_media_library());
        if ((song = nc_menu_active_item_at(menu, pos)) == NULL) {
            return false;
        }
        *tag = action_runtime_song_tag_buffer(song, getter);
        return true;
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR: {
        NcmMutableSong *mutable_song;
        enum NcmTagsField field;

        if (app_screen_tag_editor()->active_focus
            != TAG_EDITOR_FOCUS_TAGS) {
            return false;
        }
        field = ncm_song_getter_to_tags_field(getter);
        if (field == NCM_TAGS_FIELD_COUNT) {
            return false;
        }
        menu = tag_editor_screen_active_menu(app_screen_tag_editor());
        if ((mutable_song = nc_menu_active_item_at(menu, pos)) == NULL) {
            return false;
        }
        *tag = ncm_mutable_song_tags_buffer(
            mutable_song, field, Config.tags_separator,
            Config.tags_separator_len, Config.show_duplicate_tags);
        return true;
    }
#endif
    case NCM_SCREEN_TYPE_HELP:
    case NCM_SCREEN_TYPE_LASTFM:
    case NCM_SCREEN_TYPE_LYRICS:
#if defined(ENABLE_OUTPUTS)
    case NCM_SCREEN_TYPE_OUTPUTS:
#endif
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
    case NCM_SCREEN_TYPE_SERVER_INFO:
    case NCM_SCREEN_TYPE_SONG_INFO:
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
#endif
#if defined(ENABLE_VISUALIZER)
    case NCM_SCREEN_TYPE_VISUALIZER:
#endif
    case NCM_SCREEN_TYPE_COUNT:
    default:
        break;
    }
    return false;
}

static bool
action_runtime_tag_scroll_available(enum NcmSongGetter getter) {
    StrBuilder tag = {0};
    NcMenu *menu;
    bool available;

    if (((menu = action_runtime_current_tag_scroll_menu()) == NULL)
        || nc_menu_empty(menu)) {
        return false;
    }

    available = action_runtime_song_tag_at(nc_menu_highlight(menu), getter,
                                           &tag);
    sb_free(&tag);
    return available;
}

static bool
action_runtime_scroll_by_tag(enum NcmSongGetter getter, bool down) {
    StrBuilder current_tag;
    StrBuilder other_tag;
    NcMenu *menu;
    int32 current;
    int32 target;
    int32 count;
    int32 step;
    bool same;

    if (((menu = action_runtime_current_tag_scroll_menu()) == NULL)
        || nc_menu_empty(menu)) {
        return false;
    }

    current = nc_menu_highlight(menu);
    if (!action_runtime_song_tag_at(current, getter, &current_tag)) {
        return false;
    }

    target = current;
    count = nc_menu_item_count(menu);
    if (down) {
        step = 1;
    } else {
        step = -1;
    }

    while (true) {
        int32 next = target + step;

        if ((next < 0) || (next >= count)) {
            break;
        }
        if (!action_runtime_song_tag_at(next, getter, &other_tag)) {
            target = next;
            break;
        }
        same = STREQUAL(current_tag.data, current_tag.len, other_tag.data,
                        other_tag.len);
        sb_free(&other_tag);
        target = next;
        if (!same) {
            break;
        }
    }

    sb_free(&current_tag);
    nc_menu_highlight_position(menu, target,
                               action_runtime_current_menu_height());
    nc_screen_finish_list_change(app_controller_current_screen());
    return true;
}

static bool
action_runtime_selected_songs(NcmSongArray *songs) {
    if (songs == NULL) {
        return false;
    }

    switch (app_screens_current_type()) {
    case NCM_SCREEN_TYPE_BROWSER:
        return browser_screen_selected_songs(app_screen_browser(),
                                             songs);
    case NCM_SCREEN_TYPE_PLAYLIST:
        return playlist_screen_selected_songs(app_screen_playlist(),
                                              songs);
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        return playlist_editor_screen_selected_songs(
            app_screen_playlist_editor(), songs);
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
        return search_engine_screen_selected_songs(
            app_screen_search_engine(), songs);
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        return media_library_screen_selected_songs(
            app_screen_media_library(), songs);
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        return tag_editor_screen_selected_songs(
            app_screen_tag_editor(), songs);
#endif
    case NCM_SCREEN_TYPE_HELP:
    case NCM_SCREEN_TYPE_LASTFM:
    case NCM_SCREEN_TYPE_LYRICS:
#if defined(ENABLE_OUTPUTS)
    case NCM_SCREEN_TYPE_OUTPUTS:
#endif
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
    case NCM_SCREEN_TYPE_SERVER_INFO:
    case NCM_SCREEN_TYPE_SONG_INFO:
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
#endif
#if defined(ENABLE_VISUALIZER)
    case NCM_SCREEN_TYPE_VISUALIZER:
#endif
    case NCM_SCREEN_TYPE_COUNT:
        break;
    default:
        break;
    }
    return false;
}

static bool
action_runtime_has_selected_songs(void) {
    NcmSongArray songs;
    bool result;

    songs = (NcmSongArray){0};
    result = action_runtime_selected_songs(&songs) && (songs.len > 0);
    ncm_song_array_destroy(&songs);
    return result;
}

static bool
action_runtime_has_current_song(void) {
    NcmSong song;
    bool result;

    song = (NcmSong){0};
    result = action_runtime_current_song(&song);
    ncm_song_destroy(&song);
    return result;
}

static bool
action_runtime_current_song(NcmSong *song) {
    NcmSong *lyrics_song;

    if (song == NULL) {
        return false;
    }

    switch (app_screens_current_type()) {
    case NCM_SCREEN_TYPE_BROWSER:
        return browser_screen_current_song(app_screen_browser(), song);
    case NCM_SCREEN_TYPE_PLAYLIST:
        return playlist_screen_current_song(app_screen_playlist(), song);
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        return playlist_editor_screen_current_song(
            app_screen_playlist_editor(), song);
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
        return search_engine_screen_current_song(
            app_screen_search_engine(), song);
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        return media_library_screen_current_song(
            app_screen_media_library(), song);
    case NCM_SCREEN_TYPE_LYRICS:
        lyrics_song = lyrics_screen_song(app_screen_lyrics());
        if (lyrics_song == NULL) {
            return false;
        }
        return ncm_song_copy(song, lyrics_song);
    case NCM_SCREEN_TYPE_HELP:
    case NCM_SCREEN_TYPE_LASTFM:
#if defined(ENABLE_OUTPUTS)
    case NCM_SCREEN_TYPE_OUTPUTS:
#endif
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
    case NCM_SCREEN_TYPE_SERVER_INFO:
    case NCM_SCREEN_TYPE_SONG_INFO:
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
#endif
#if defined(ENABLE_VISUALIZER)
    case NCM_SCREEN_TYPE_VISUALIZER:
#endif
    case NCM_SCREEN_TYPE_COUNT:
        break;
    default:
        break;
    }
    return false;
}

static void
action_runtime_sort_positions(int32 *positions, int32 count, bool descending) {
    int32 value;

    if (positions == NULL) {
        return;
    }

    for (int32 i = 0; i < count; i += 1) {
        for (int32 j = i + 1; j < count; j += 1) {
            if (descending) {
                if (positions[j] <= positions[i]) {
                    continue;
                }
            } else {
                if (positions[j] >= positions[i]) {
                    continue;
                }
            }
            value = positions[i];
            positions[i] = positions[j];
            positions[j] = value;
        }
    }
    return;
}

static bool
action_runtime_song_positions(NcmSongArray *songs,
                              int32 **positions, int32 *count) {
    int32 *result;

    if ((songs == NULL) || (positions == NULL) || (count == NULL)) {
        return false;
    }
    if (songs->len <= 0) {
        return false;
    }

    result = malloc2(songs->len*SIZEOF(*result));
    for (int32 i = 0; i < songs->len; i += 1) {
        result[i] = ncm_song_position(&songs->items[i]);
    }

    *positions = result;
    *count = songs->len;
    return true;
}

static bool
action_runtime_add_prompt(void) {
    StrBuilder path = {0};
    StrBuilder message = {0};
    NcmError ncm_error;
    enum mpd_server_error server_error;
    bool prompted;
    bool success;

    prompted = action_runtime_prompt_string(STRLIT("Add: "), "", false,
                                            NULL, NULL, &path);
    if (!prompted) {
        sb_free(&path);
        return true;
    }

    if ((path.len <= 0)
        && !action_runtime_confirm(
            STRLIT("Are you sure you want to add the whole database?"))) {
        sb_free(&path);
        return true;
    }

    {
        char *path_text = path.data;
        bool added = false;

        if (path_text == NULL) {
            path_text = "";
        }

        ncm_statusbar_print_cstring(0, "Adding...");
        ncm_error_clear(&ncm_error);
        success = ncm_mpd_client_add(&global_mpd, path_text, &added,
                                     &ncm_error);
        server_error = ncm_mpd_client_server_error_code(&global_mpd);
        if (!success && (server_error == MPD_SERVER_ERROR_NO_EXIST)) {
            bool loaded = false;

            ncm_error_clear(&ncm_error);
            success = ncm_mpd_client_load_playlist(
                &global_mpd, path_text, &loaded, &ncm_error);
            sb_free(&path);
            if (!success) {
                return action_runtime_mpd_error(&ncm_error);
            }
            return true;
        }
    }
    sb_free(&path);

    if (!success && (server_error != (enum mpd_server_error)0)) {
        SB_APPEND(&message, STRLIT("Error while adding item: "));
        if (ncm_error_is_set(&ncm_error)) {
            SB_APPEND(&message, ncm_error.message,
                      optional_strlen32(ncm_error.message));
        }
        ncm_statusbar_print(Config.message_delay_time, message.data,
                            message.len);
        sb_free(&message);
        return false;
    }
    if (!success) {
        return action_runtime_mpd_error(&ncm_error);
    }
    return true;
}

static bool
action_runtime_load_prompt(void) {
    StrBuilder name = {0};
    NcmError ncm_error;
    bool prompted = action_runtime_prompt_string(STRLIT("Load playlist: "), "",
                                                 false, NULL, NULL, &name);

    if (!prompted) {
        sb_free(&name);
        return true;
    }

    {
        char *name_text = name.data;
        bool loaded = false;

        if (name_text == NULL) {
            name_text = "";
        }

        ncm_statusbar_print_cstring(0, "Loading...");
        ncm_error_clear(&ncm_error);
        if (!ncm_mpd_client_load_playlist(&global_mpd, name_text, &loaded,
                                          &ncm_error)) {
            sb_free(&name);
            return action_runtime_mpd_error(&ncm_error);
        }
    }

    sb_free(&name);
    return true;
}

static bool
action_runtime_add_selected_songs(bool play) {
    NcmSongArray songs;
    bool success;
    bool first = true;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    songs = (NcmSongArray){0};
    if (!(success = action_runtime_selected_songs(&songs) && (songs.len > 0))) {
        ncm_song_array_destroy(&songs);
        return false;
    }

    for (int32 i = 0; success && (i < songs.len); i += 1) {
        success = ncm_action_add_song_to_playlist(&songs.items[i],
                                                  play && first, -1);
        first = false;
    }

    ncm_song_array_destroy(&songs);
    return success;
}

static bool
action_runtime_add_item_to_playlist(bool play) {
    NcmError ncm_error;
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY)) {
        ncm_error_clear(&ncm_error);
        success = media_library_screen_add_item_to_playlist(
            app_screen_media_library(), play, &ncm_error);
        if (!success && ncm_error_is_set(&ncm_error)) {
            ncm_statusbar_print_cstring(Config.message_delay_time,
                                        ncm_error.message);
            return false;
        }
        return success;
    }

    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return action_runtime_add_playlist_editor_item(play);
    }

    return action_runtime_add_selected_songs(play);
}

static bool
action_runtime_playlist_editor_playlists_active(void) {
    PlaylistEditorScreen *screen = app_screen_playlist_editor();

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return false;
    }
    return playlist_editor_screen_active_menu(screen)
           == nc_playlist_entry_menu_base(
               playlist_editor_screen_playlists(screen));
}

static bool
action_runtime_playlist_editor_content_active(void) {
    PlaylistEditorScreen *screen = app_screen_playlist_editor();

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return false;
    }
    return playlist_editor_screen_active_menu(screen)
           == nc_song_menu_base(playlist_editor_screen_content(screen));
}

static bool
action_runtime_playlist_editor_has_playlists(void) {
    PlaylistEditorScreen *screen = app_screen_playlist_editor();

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return false;
    }
    return nc_menu_all_item_count(nc_playlist_entry_menu_base(
        playlist_editor_screen_playlists(screen)))
           > 0;
}

static bool
action_runtime_playlist_editor_has_content(void) {
    PlaylistEditorScreen *screen = app_screen_playlist_editor();

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return false;
    }
    return nc_menu_all_item_count(
        nc_song_menu_base(playlist_editor_screen_content(screen)))
           > 0;
}

static bool
action_runtime_add_playlist_editor_item(bool play) {
    PlaylistEditorScreen *screen = app_screen_playlist_editor();
    NcmPlaylist playlist;
    NcmError ncm_error;
    int32 play_position;
    bool loaded;
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return false;
    }
    if (action_runtime_playlist_editor_content_active()) {
        return action_runtime_add_selected_songs(play);
    }
    if (!action_runtime_playlist_editor_playlists_active()) {
        return false;
    }

    playlist = (NcmPlaylist){0};
    success = playlist_editor_screen_current_playlist(screen, &playlist);
    if (!success) {
        ncm_playlist_destroy(&playlist);
        return false;
    }

    loaded = false;
    play_position = ncm_status_state_playlist_length();
    ncm_error_clear(&ncm_error);
    success = ncm_mpd_client_load_playlist(&global_mpd, playlist.path, &loaded,
                                           &ncm_error);
    if (success && play && loaded) {
        success = ncm_mpd_client_play_pos(&global_mpd, play_position,
                                          &ncm_error);
    }
    ncm_playlist_destroy(&playlist);
    if (!success) {
        return action_runtime_mpd_error(&ncm_error);
    }

    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return true;
}

static bool
action_runtime_delete_browser_items(void) {
    BrowserScreen *screen = app_screen_browser();
    NcMenu *menu;
    NcmMpdItem *item;
    StrBuilder question = {0};
    StrBuilder name = {0};
    NcmError ncm_error;
    bool success;
    bool has_selected;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        return false;
    }
    if (((menu = browser_screen_menu(screen)) == NULL)
        || nc_menu_empty(menu)) {
        return false;
    }
    if (!Config.allow_for_physical_item_deletion) {
        ncm_statusbar_print_cstring(
            Config.message_delay_time,
            "Flag \"allow_for_physical_item_deletion\" needs to be "
            "enabled in configuration file");
        return false;
    }
    if (!browser_screen_is_local(screen)
        && (Config.mpd_music_dir_len <= 0)) {
        ncm_statusbar_print_cstring(
            Config.message_delay_time,
            "Proper mpd_music_dir variable has to be set in "
            "configuration file");
        return false;
    }

    has_selected = nc_menu_has_selected(menu);
    if (has_selected) {
        SB_APPEND(&question, STRLIT("Delete selected items?"));
    } else {
        item = nc_menu_current_item(menu);
        if (browser_screen_item_is_parent(item)) {
            sb_free(&name);
            sb_free(&question);
            return true;
        }
        if (!action_runtime_browser_item_name(item, &name)) {
            sb_free(&name);
            sb_free(&question);
            return false;
        }
        SB_APPEND(&question, STRLIT("Delete \""));
        SB_APPEND(&question, name.data, name.len);
        SB_APPEND(&question, STRLIT("\"?"));
    }

    success = action_runtime_confirm(question.data, question.len);
    sb_free(&name);
    sb_free(&question);
    if (!success) {
        return true;
    }

    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Deleting items...");
    ncm_error_clear(&ncm_error);
    if (!browser_screen_delete_items(screen, &global_mpd, &ncm_error)) {
        return action_runtime_mpd_error(&ncm_error);
    }
    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Item(s) deleted");
    return true;
}

static bool
action_runtime_browser_item_name(NcmMpdItem *item, StrBuilder *name) {
    NcmStringView view;
    int32 basename;

    if ((item == NULL) || (name == NULL)) {
        return false;
    }
    sb_clear(name);
    ncm_string_view_clear(&view);

    switch (ncm_mpd_item_kind(item)) {
    case NCM_MPD_ITEM_DIRECTORY:
        if (!ncm_directory_path_view(ncm_mpd_item_directory(item), &view)) {
            return false;
        }
        break;
    case NCM_MPD_ITEM_SONG:
        if (!ncm_song_name_view(ncm_mpd_item_song(item), 0, &view)
            && !ncm_song_uri_view(ncm_mpd_item_song(item), 0, &view)) {
            return false;
        }
        break;
    case NCM_MPD_ITEM_PLAYLIST:
        if (!ncm_playlist_path_view(ncm_mpd_item_playlist(item), &view)) {
            return false;
        }
        break;
    case NCM_MPD_ITEM_COUNT:
    default:
        return false;
    }

    basename = ncm_path_basename_start(view.data, view.len);
    SB_APPEND(name, view.data + basename, view.len - basename);
    return true;
}

static void
action_runtime_print_renamed(char *prefix, int32 prefix_len, StrBuilder *name) {
    StrBuilder message = {0};

    if (name == NULL) {
        return;
    }

    SB_APPEND(&message, prefix, prefix_len);
    SB_APPEND(&message, name->data, name->len);
    SB_APPEND(&message, STRLIT("\""));
    ncm_statusbar_print(Config.message_delay_time, message.data,
                        message.len);
    sb_free(&message);
    return;
}

static bool
action_runtime_delete_main_playlist_items(void) {
    NcmSongArray songs;
    NcmError ncm_error;
    int32 *positions;
    int32 count;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
        return false;
    }

    songs = (NcmSongArray){0};
    if (!playlist_screen_selected_songs(app_screen_playlist(), &songs)) {
        ncm_song_array_destroy(&songs);
        return false;
    }
    if (!action_runtime_song_positions(&songs, &positions, &count)) {
        ncm_song_array_destroy(&songs);
        return false;
    }

    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Deleting items...");
    action_runtime_sort_positions(positions, count, true);
    ncm_error_clear(&ncm_error);
    for (int32 i = 0; i < count; i += 1) {
        if (!ncm_mpd_client_delete(&global_mpd, positions[i], &ncm_error)) {
            free2(positions, count*SIZEOF(*positions));
            ncm_song_array_destroy(&songs);
            return action_runtime_mpd_error(&ncm_error);
        }
    }

    free2(positions, count*SIZEOF(*positions));
    ncm_song_array_destroy(&songs);
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Item(s) deleted");
    return true;
}

static bool
action_runtime_delete_playlist_editor_items(void) {
    PlaylistEditorScreen *screen = app_screen_playlist_editor();
    NcmPlaylist playlist;
    NcmSongArray songs;
    NcmError ncm_error;
    int32 *positions;
    int32 count;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (!action_runtime_playlist_editor_content_active()) {
        return false;
    }

    playlist = (NcmPlaylist){0};
    songs = (NcmSongArray){0};
    if (!playlist_editor_screen_current_playlist(screen, &playlist)) {
        ncm_playlist_destroy(&playlist);
        ncm_song_array_destroy(&songs);
        return false;
    }
    if (!playlist_editor_screen_selected_songs(screen, &songs)) {
        ncm_playlist_destroy(&playlist);
        ncm_song_array_destroy(&songs);
        return false;
    }
    if (!action_runtime_song_positions(&songs, &positions, &count)) {
        ncm_playlist_destroy(&playlist);
        ncm_song_array_destroy(&songs);
        return false;
    }

    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Deleting items...");
    action_runtime_sort_positions(positions, count, true);
    ncm_error_clear(&ncm_error);
    for (int32 i = 0; i < count; i += 1) {
        if (!ncm_mpd_client_playlist_delete(&global_mpd, playlist.path,
                                            positions[i], &ncm_error)) {
            free2(positions, count*SIZEOF(*positions));
            ncm_playlist_destroy(&playlist);
            ncm_song_array_destroy(&songs);
            return action_runtime_mpd_error(&ncm_error);
        }
    }

    free2(positions, count*SIZEOF(*positions));
    ncm_playlist_destroy(&playlist);
    ncm_song_array_destroy(&songs);
    playlist_editor_screen_request_content_update(screen);
    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Item(s) deleted");
    return true;
}

static bool
action_runtime_delete_playlist_items(void) {
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
        return action_runtime_delete_main_playlist_items();
    }
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return action_runtime_delete_playlist_editor_items();
    }
    return false;
}

static bool
action_runtime_delete_stored_playlists(void) {
    PlaylistEditorScreen *screen = app_screen_playlist_editor();
    NcMenu *menu;
    NcmPlaylist *playlist;
    StrBuilder question = {0};
    NcmError ncm_error;
    enum NcMenuItemSource source;
    int32 count;
    bool has_selected;
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (!action_runtime_playlist_editor_playlists_active()) {
        return false;
    }
    if (!action_runtime_playlist_editor_has_playlists()) {
        return false;
    }

    menu = nc_playlist_entry_menu_base(
        playlist_editor_screen_playlists(screen));
    source = action_runtime_menu_item_source(menu);
    has_selected = nc_menu_has_selected(menu);

    if (has_selected) {
        SB_APPEND(&question, STRLIT("Delete selected playlists?"));
    } else {
        if (((playlist = nc_menu_current_item(menu)) == NULL)
            || (playlist->path == NULL)) {
            sb_free(&question);
            return false;
        }
        SB_APPEND(&question, STRLIT("Delete playlist \""));
        SB_APPEND(&question, playlist->path, playlist->path_len);
        SB_APPEND(&question, STRLIT("\"?"));
    }
    success = action_runtime_confirm(question.data, question.len);
    sb_free(&question);
    if (!success) {
        return true;
    }

    ncm_error_clear(&ncm_error);
    success = true;
    count = nc_menu_item_count(menu);
    for (int32 i = 0; success && (i < count); i += 1) {
        if (has_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!has_selected && (i != nc_menu_highlight(menu))) {
            continue;
        }
        if (((playlist = nc_menu_item_at(menu, source, i)) == NULL)
            || (playlist->path == NULL)) {
            success = false;
            break;
        }
        success = ncm_mpd_client_delete_playlist(&global_mpd, playlist->path,
                                                 &ncm_error);
    }
    if (!success) {
        return action_runtime_mpd_error(&ncm_error);
    }

    playlist_editor_screen_request_playlists_update(screen);
    if (has_selected) {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Playlists deleted");
    } else {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Playlist deleted");
    }
    return true;
}

static bool
action_runtime_clear_playlist(bool main_playlist) {
    PlaylistEditorScreen *screen = app_screen_playlist_editor();
    NcmPlaylist playlist;
    StrBuilder question = {0};
    StrBuilder message = {0};
    NcmError ncm_error;
    bool success = false;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    ncm_error_clear(&ncm_error);
    if (main_playlist) {
        if (!playlist_screen_empty(app_screen_playlist())
            && Config.ask_before_clearing_playlists
            && !action_runtime_confirm(
                STRLIT("Do you really want to clear main playlist?"))) {
            return true;
        }
        if (!ncm_mpd_client_clear_queue(&global_mpd, &ncm_error)) {
            return action_runtime_mpd_error(&ncm_error);
        }
        playlist_screen_clear(app_screen_playlist());
        (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Playlist cleared");
        return true;
    }

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return false;
    }
    if (!action_runtime_playlist_editor_has_playlists()) {
        return false;
    }

    playlist = (NcmPlaylist){0};
    success = playlist_editor_screen_current_playlist(screen, &playlist);
    if (!success) {
        ncm_playlist_destroy(&playlist);
        return false;
    }

    if (Config.ask_before_clearing_playlists) {

        SB_APPEND(&question,
                  STRLIT("Do you really want to clear playlist \""));
        SB_APPEND(&question, playlist.path, playlist.path_len);
        SB_APPEND(&question, STRLIT("\"?"));

        success = action_runtime_confirm(question.data, question.len);
        sb_free(&question);
        if (!success) {
            ncm_playlist_destroy(&playlist);
            return true;
        }
    }

    success = ncm_mpd_client_clear_playlist(&global_mpd, playlist.path,
                                            &ncm_error);
    if (success) {
        SB_APPEND(&message, STRLIT("Playlist \""));
        SB_APPEND(&message, playlist.path, playlist.path_len);
        SB_APPEND(&message, STRLIT("\" cleared"));
        ncm_statusbar_print(Config.message_delay_time, message.data,
                            message.len);
        sb_free(&message);
    }
    ncm_playlist_destroy(&playlist);
    if (!success) {
        return action_runtime_mpd_error(&ncm_error);
    }
    playlist_editor_screen_request_content_update(screen);
    return true;
}

static bool
action_runtime_crop_playlist(bool main_playlist) {
    PlaylistEditorScreen *editor = app_screen_playlist_editor();
    NcmPlaylist playlist;
    NcmSongArray songs;
    StrBuilder question = {0};
    StrBuilder message = {0};
    NcmError ncm_error;
    bool success = false;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    songs = (NcmSongArray){0};
    if (main_playlist) {
        if (playlist_screen_song_count(app_screen_playlist())
            <= 1) {
            ncm_song_array_destroy(&songs);
            return true;
        }
        if (Config.ask_before_clearing_playlists
            && !action_runtime_confirm(
                STRLIT("Do you really want to crop main playlist?"))) {
            ncm_song_array_destroy(&songs);
            return true;
        }
        success = playlist_screen_selected_songs(
            app_screen_playlist(), &songs);
    } else if (action_runtime_current_screen_is(
        NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        if (!action_runtime_playlist_editor_has_playlists()) {
            ncm_song_array_destroy(&songs);
            return false;
        }
        if (action_runtime_playlist_editor_has_content()
            && (nc_menu_all_item_count(
                nc_song_menu_base(playlist_editor_screen_content(
                    app_screen_playlist_editor())))
                <= 1)) {
            ncm_song_array_destroy(&songs);
            return true;
        }
        success = playlist_editor_screen_selected_songs(
            app_screen_playlist_editor(), &songs);
    }
    if (!success || (songs.len <= 0)) {
        ncm_song_array_destroy(&songs);
        return false;
    }

    ncm_error_clear(&ncm_error);
    if (main_playlist) {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Cropping playlist...");
        if (!ncm_mpd_client_clear_queue(&global_mpd, &ncm_error)) {
            ncm_song_array_destroy(&songs);
            return action_runtime_mpd_error(&ncm_error);
        }
        for (int32 i = 0; i < songs.len; i += 1) {
            if (!ncm_mpd_client_add_song_value(&global_mpd, &songs.items[i], -1,
                                               NULL, &ncm_error)) {
                ncm_song_array_destroy(&songs);
                return action_runtime_mpd_error(&ncm_error);
            }
        }
        (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
        ncm_song_array_destroy(&songs);
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Playlist cropped");
        return true;
    }

    playlist = (NcmPlaylist){0};
    success = playlist_editor_screen_current_playlist(editor, &playlist);
    if (success && Config.ask_before_clearing_playlists) {
        SB_APPEND(
            &question, STRLIT("Do you really want to crop playlist \""));
        SB_APPEND(&question, playlist.path, playlist.path_len);
        SB_APPEND(&question, STRLIT("\"?"));
        success = action_runtime_confirm(question.data, question.len);
        sb_free(&question);
        if (!success) {
            ncm_playlist_destroy(&playlist);
            ncm_song_array_destroy(&songs);
            return true;
        }
    }
    if (success) {
        SB_APPEND(&message, STRLIT("Cropping playlist \""));
        SB_APPEND(&message, playlist.path, playlist.path_len);
        SB_APPEND(&message, STRLIT("\"..."));
        ncm_statusbar_print(Config.message_delay_time, message.data,
                            message.len);
        sb_free(&message);
        success = ncm_mpd_client_clear_playlist(&global_mpd, playlist.path,
                                                &ncm_error);
    }
    for (int32 i = 0; success && (i < songs.len); i += 1) {
        success = ncm_mpd_client_add_song_to_playlist(
            &global_mpd, playlist.path, &songs.items[i], &ncm_error);
    }
    if (success) {
        SB_APPEND(&message, STRLIT("Playlist \""));
        SB_APPEND(&message, playlist.path, playlist.path_len);
        SB_APPEND(&message, STRLIT("\" cropped"));
        ncm_statusbar_print(Config.message_delay_time, message.data,
                            message.len);
        sb_free(&message);
    }
    ncm_playlist_destroy(&playlist);
    ncm_song_array_destroy(&songs);
    if (!success) {
        return action_runtime_mpd_error(&ncm_error);
    }
    playlist_editor_screen_request_content_update(editor);
    return true;
}

static bool
action_runtime_move_main_playlist_items(NcmSongArray *songs, bool down) {
    NcmError ncm_error;
    int32 *positions;
    int32 count;
    bool success;

    if (!action_runtime_song_positions(songs, &positions, &count)) {
        return false;
    }

    action_runtime_sort_positions(positions, count, down);
    ncm_error_clear(&ncm_error);
    success = ncm_mpd_client_start_command_list(&global_mpd, &ncm_error);
    for (int32 i = 0; success && (i < count); i += 1) {
        if (down) {
            if (positions[i] + 1 >= ncm_status_state_playlist_length()) {
                continue;
            }
            success = ncm_mpd_client_swap(&global_mpd, positions[i],
                                          positions[i] + 1, &ncm_error);
        } else {
            if (positions[i] == 0) {
                continue;
            }
            success = ncm_mpd_client_swap(&global_mpd, positions[i],
                                          positions[i] - 1, &ncm_error);
        }
    }
    if (success) {
        success = ncm_mpd_client_commit_command_list(&global_mpd, &ncm_error);
    }
    if (!success && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }

    free2(positions, count*SIZEOF(*positions));
    if (!success) {
        return action_runtime_mpd_error(&ncm_error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return true;
}

static bool
action_runtime_move_stored_playlist_items(NcmSongArray *songs, bool down) {
    PlaylistEditorScreen *screen = app_screen_playlist_editor();
    NcmPlaylist playlist;
    NcmError ncm_error;
    int32 *positions;
    int32 item_count;
    int32 count;
    bool success;

    if (!action_runtime_song_positions(songs, &positions, &count)) {
        return false;
    }

    playlist = (NcmPlaylist){0};
    success = playlist_editor_screen_current_playlist(screen, &playlist);
    if (!success) {
        free2(positions, count*SIZEOF(*positions));
        ncm_playlist_destroy(&playlist);
        return false;
    }

    action_runtime_sort_positions(positions, count, down);
    item_count = nc_menu_all_item_count(
        playlist_editor_screen_active_menu(screen));
    ncm_error_clear(&ncm_error);
    success = ncm_mpd_client_start_command_list(&global_mpd, &ncm_error);
    for (int32 i = 0; success && (i < count); i += 1) {
        if (down) {
            if (positions[i] + 1 >= item_count) {
                continue;
            }
            success = ncm_mpd_client_playlist_move(&global_mpd, playlist.path,
                                                   positions[i],
                                                   positions[i] + 1,
                                                   &ncm_error);
        } else if (positions[i] > 0) {
            success = ncm_mpd_client_playlist_move(&global_mpd, playlist.path,
                                                   positions[i],
                                                   positions[i] - 1,
                                                   &ncm_error);
        }
    }
    if (success) {
        success = ncm_mpd_client_commit_command_list(&global_mpd, &ncm_error);
    }
    if (!success && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }

    free2(positions, count*SIZEOF(*positions));
    ncm_playlist_destroy(&playlist);
    if (!success) {
        return action_runtime_mpd_error(&ncm_error);
    }
    playlist_editor_screen_request_content_update(screen);
    return true;
}

static bool
action_runtime_move_selected_items(bool down) {
    NcmSongArray songs;
    NcMenu *menu;
    enum ScreenType screen_type = app_screens_current_type();
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    if ((screen_type != NCM_SCREEN_TYPE_PLAYLIST)
        && (screen_type != NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return false;
    }
    if ((menu = action_runtime_current_menu()) && nc_menu_is_filtered(menu)) {
        ncm_statusbar_print_cstring(
            Config.message_delay_time,
            "Moving items is disabled in filtered playlist");
        return true;
    }

    songs = (NcmSongArray){0};
    if (!action_runtime_selected_songs(&songs) || (songs.len <= 0)) {
        ncm_song_array_destroy(&songs);
        return false;
    }

    if (screen_type == NCM_SCREEN_TYPE_PLAYLIST) {
        success = action_runtime_move_main_playlist_items(&songs, down);
    } else if (screen_type == NCM_SCREEN_TYPE_PLAYLIST_EDITOR) {
        success = action_runtime_move_stored_playlist_items(&songs, down);
    } else {
        success = false;
    }

    ncm_song_array_destroy(&songs);
    return success;
}

static bool
action_runtime_move_main_playlist_items_to(void) {
    PlaylistScreen *screen = app_screen_playlist();
    NcMenu *menu;
    NcmSong *song;
    NcmError ncm_error;
    int32 *positions;
    int32 target;
    int32 destination;
    int32 item_count;
    int32 count;
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
        return false;
    }

    if (((menu = playlist_screen_menu(screen)) == NULL)
        || !nc_menu_has_selected(menu)) {
        return false;
    }
    song = nc_menu_active_item_at(menu, nc_menu_highlight(menu));
    if (song == NULL) {
        return false;
    }
    target = ncm_song_position(song);

    item_count = nc_menu_all_item_count(menu);
    positions = malloc2(item_count*SIZEOF(*positions));
    count = 0;
    for (int32 i = 0; i < item_count; i += 1) {
        uint32 flags = nc_menu_item_flags_at(menu, NC_MENU_ITEMS_ALL, i);

        if (!(flags & NC_MENU_ITEM_SELECTED)) {
            continue;
        }
        if ((song = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, i)) == NULL) {
            free2(positions, item_count*SIZEOF(*positions));
            return false;
        }
        positions[count] = ncm_song_position(song);
        count += 1;
    }
    if (count <= 0) {
        free2(positions, item_count*SIZEOF(*positions));
        return false;
    }

    action_runtime_sort_positions(positions, count, false);
    if ((target >= positions[0]) && (target <= positions[count - 1])) {
        free2(positions, item_count*SIZEOF(*positions));
        return true;
    }

    ncm_error_clear(&ncm_error);
    if ((success = ncm_mpd_client_start_command_list(&global_mpd, &ncm_error))
        && (target > positions[0])) {
        destination = target - count;
        for (int32 i = count; success && (i > 0); i -= 1) {
            success = ncm_mpd_client_move(&global_mpd, positions[i - 1],
                                          destination + i - 1, &ncm_error);
        }
    } else if (success) {
        destination = target;
        for (int32 i = 0; success && (i < count); i += 1) {
            success = ncm_mpd_client_move(&global_mpd, positions[i],
                                          destination + i, &ncm_error);
        }
    }
    if (success) {
        success = ncm_mpd_client_commit_command_list(&global_mpd, &ncm_error);
    }
    if (!success && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }
    free2(positions, item_count*SIZEOF(*positions));
    if (!success) {
        return action_runtime_mpd_error(&ncm_error);
    }

    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return true;
}

static bool
action_runtime_move_playlist_editor_items_to(void) {
    PlaylistEditorScreen *screen = app_screen_playlist_editor();
    NcmPlaylist playlist;
    NcMenu *menu;
    NcmSong *song;
    NcmError ncm_error;
    int32 *positions;
    int32 target;
    int32 destination;
    int32 item_count;
    int32 count;
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (!action_runtime_playlist_editor_content_active()) {
        return false;
    }

    menu = nc_song_menu_base(playlist_editor_screen_content(screen));
    if ((menu == NULL) || !nc_menu_has_selected(menu)) {
        return false;
    }
    if (nc_menu_is_filtered(menu)) {
        ncm_statusbar_print_cstring(
            Config.message_delay_time,
            "Moving items is disabled in filtered playlist");
        return true;
    }

    song = nc_menu_active_item_at(menu, nc_menu_highlight(menu));
    if (song == NULL) {
        return false;
    }
    target = ncm_song_position(song);

    item_count = nc_menu_all_item_count(menu);
    positions = malloc2(item_count*SIZEOF(*positions));
    count = 0;
    for (int32 i = 0; i < item_count; i += 1) {
        uint32 flags = nc_menu_item_flags_at(menu, NC_MENU_ITEMS_ALL, i);

        if (!(flags & NC_MENU_ITEM_SELECTED)) {
            continue;
        }
        if ((song = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, i)) == NULL) {
            free2(positions, item_count*SIZEOF(*positions));
            return false;
        }
        positions[count] = ncm_song_position(song);
        count += 1;
    }
    if (count <= 0) {
        free2(positions, item_count*SIZEOF(*positions));
        return false;
    }

    playlist = (NcmPlaylist){0};
    success = playlist_editor_screen_current_playlist(screen, &playlist);
    if (!success) {
        ncm_playlist_destroy(&playlist);
        free2(positions, item_count*SIZEOF(*positions));
        return false;
    }

    action_runtime_sort_positions(positions, count, false);
    if ((target >= positions[0]) && (target <= positions[count - 1])) {
        ncm_playlist_destroy(&playlist);
        free2(positions, item_count*SIZEOF(*positions));
        return true;
    }

    ncm_error_clear(&ncm_error);
    if ((success = ncm_mpd_client_start_command_list(&global_mpd, &ncm_error))
        && (target > positions[0])) {
        destination = target - count;
        for (int32 i = count; success && (i > 0); i -= 1) {
            success = ncm_mpd_client_playlist_move(
                &global_mpd, playlist.path, positions[i - 1],
                destination + i - 1, &ncm_error);
        }
    } else if (success) {
        destination = target;
        for (int32 i = 0; success && (i < count); i += 1) {
            success = ncm_mpd_client_playlist_move(
                &global_mpd, playlist.path, positions[i],
                destination + i, &ncm_error);
        }
    }
    if (success) {
        success = ncm_mpd_client_commit_command_list(&global_mpd, &ncm_error);
    }
    if (!success && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }

    ncm_playlist_destroy(&playlist);
    free2(positions, item_count*SIZEOF(*positions));
    if (!success) {
        return action_runtime_mpd_error(&ncm_error);
    }
    playlist_editor_screen_request_content_update(screen);
    return true;
}

static bool
action_runtime_move_selected_items_to(void) {
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
        return action_runtime_move_main_playlist_items_to();
    }
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return action_runtime_move_playlist_editor_items_to();
    }
    return false;
}

static bool
action_runtime_playlist_range(NcMenu *menu, int32 *first, int32 *last) {
    enum NcMenuItemSource source;
    int32 range_first;
    int32 range_last;
    NcmSong *song;

    if ((menu == NULL) || (first == NULL) || (last == NULL)) {
        return false;
    }
    source = action_runtime_menu_item_source(menu);
    if (!ncm_menu_find_full_selected_range(menu, source, &range_first,
                                           &range_last)) {
        return false;
    }
    if (range_first >= range_last) {
        return false;
    }

    if ((song = nc_menu_active_item_at(menu, range_first)) == NULL) {
        return false;
    }
    *first = ncm_song_position(song);
    if ((song = nc_menu_active_item_at(menu, range_last - 1)) == NULL) {
        return false;
    }
    *last = ncm_song_position(song) + 1;
    return true;
}

static bool
action_runtime_reverse_playlist(void) {
    enum NcMenuItemSource source;
    NcMenu *menu;
    NcmSong *left;
    NcmSong *right;
    NcmError ncm_error;
    int32 first;
    int32 last;
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
        return false;
    }

    menu = action_runtime_current_menu();
    source = action_runtime_menu_item_source(menu);
    if (!ncm_menu_find_full_selected_range(menu, source, &first, &last)) {
        return false;
    }
    if (first >= last) {
        return false;
    }

    last -= 1;
    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Reversing range...");
    ncm_error_clear(&ncm_error);
    success = ncm_mpd_client_start_command_list(&global_mpd, &ncm_error);
    while (success && (first < last)) {
        left = nc_menu_active_item_at(menu, first);
        if ((left == NULL)
            || ((right = nc_menu_active_item_at(menu, last)) == NULL)) {
            success = false;
            break;
        }
        success = ncm_mpd_client_swap(&global_mpd, ncm_song_position(left),
                                      ncm_song_position(right), &ncm_error);
        first += 1;
        last -= 1;
    }
    if (success) {
        success = ncm_mpd_client_commit_command_list(&global_mpd, &ncm_error);
    }
    if (!success && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }
    if (!success) {
        return action_runtime_mpd_error(&ncm_error);
    }

    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Range reversed");
    return true;
}

static bool
action_runtime_shuffle_playlist(void) {
    NcMenu *menu;
    NcmError ncm_error;
    int32 first;
    int32 last;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
        return false;
    }

    menu = action_runtime_current_menu();
    if (!action_runtime_playlist_range(menu, &first, &last)) {
        return false;
    }
    if (Config.ask_before_shuffling_playlists
        && !action_runtime_confirm(
            STRLIT("Do you really want to shuffle selected range?"))) {
        return true;
    }

    ncm_error_clear(&ncm_error);
    if (!ncm_mpd_client_shuffle_range(&global_mpd, first, last, &ncm_error)) {
        return action_runtime_mpd_error(&ncm_error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Range shuffled");
    return true;
}

static bool
action_runtime_set_selected_items_priority(void) {
    StrBuilder input = {0};
    NcmError ncm_error;
    int32 priority;
    bool prompted;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
        return false;
    }
    if (ncm_mpd_client_version(&global_mpd) < 17) {
        ncm_statusbar_print_cstring(
            Config.message_delay_time,
            "Priorities are supported in MPD >= 0.17.0");
        return false;
    }

    prompted = action_runtime_prompt_string(
        STRLIT("Set priority [0-255]: "), "", false, NULL, NULL, &input);
    if (!prompted) {
        sb_free(&input);
        return true;
    }

    ncm_error_clear(&ncm_error);
    if (!ncm_parse_int32(input.data, input.len, &priority, &ncm_error)
        || (priority > 255)) {
        sb_free(&input);
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Priority must be between 0 and 255");
        return true;
    }
    sb_free(&input);

    ncm_error_clear(&ncm_error);
    if (!playlist_screen_set_selected_priority(
        app_screen_playlist(), &global_mpd, priority, &ncm_error)) {
        return action_runtime_mpd_error(&ncm_error);
    }
    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Priority set");
    return true;
}

static bool
action_runtime_jump_to_position_in_song(void) {
    StrBuilder input = {0};
    NcmError ncm_error;
    int32 song_position;
    int32 total;
    int32 target;
    bool prompted;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (ncm_status_state_player() == NCM_STATUS_PLAYER_STOP) {
        return false;
    }
    total = ncm_status_state_total_time();
    song_position = ncm_status_state_current_song_position();
    if ((total == 0) || (song_position < 0)) {
        return false;
    }

    prompted = action_runtime_prompt_string(
        STRLIT("Position to go (in %/h:m:ss/m:ss/seconds(s)): "), "",
        false, NULL, NULL, &input);
    if (!prompted) {
        sb_free(&input);
        return true;
    }
    if (!action_runtime_parse_seek_position(input.data, input.len, total,
                                            &target)) {
        sb_free(&input);
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Invalid format ([h]:[mm]:[ss], [m]:[ss], "
                                    "[s]s, [%]%, [%] accepted)");
        return true;
    }
    sb_free(&input);

    ncm_error_clear(&ncm_error);
    if (!ncm_mpd_client_seek_pos(&global_mpd, song_position, target,
                                 &ncm_error)) {
        return action_runtime_mpd_error(&ncm_error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return true;
}

static bool
action_runtime_select_album(void) {
    StrBuilder album;
    StrBuilder candidate;
    NcMenu *menu;
    int32 current;
    int32 count;
    bool equal;

    if (((menu = action_runtime_current_tag_scroll_menu()) == NULL)
        || nc_menu_empty(menu)) {
        return false;
    }

    current = nc_menu_highlight(menu);
    if (!action_runtime_song_tag_at(current, NCM_SONG_GETTER_ALBUM, &album)) {
        return false;
    }

    for (int32 position = current; position >= 0; position -= 1) {
        if (!action_runtime_song_tag_at(position, NCM_SONG_GETTER_ALBUM,
                                        &candidate)) {
            break;
        }
        equal = STREQUAL(album.data, album.len, candidate.data, candidate.len);
        sb_free(&candidate);
        if (!equal) {
            break;
        }
        (void)nc_menu_set_position_selected(menu, position, true);
    }

    count = nc_menu_item_count(menu);
    for (int32 position = current + 1; position < count; position += 1) {
        if (!action_runtime_song_tag_at(position, NCM_SONG_GETTER_ALBUM,
                                        &candidate)) {
            break;
        }
        equal = STREQUAL(album.data, album.len, candidate.data, candidate.len);
        sb_free(&candidate);
        if (!equal) {
            break;
        }
        (void)nc_menu_set_position_selected(menu, position, true);
    }
    sb_free(&album);

    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Album around cursor position selected");
    return true;
}

static bool
action_runtime_select_found_items(void) {
    NcmStringView constraint;
    NcMenu *menu;
    NcmError ncm_error;
    int32 original;
    int32 height;
    bool found;

    if (!current_screen_allows_search()) {
        return false;
    }
    constraint = current_screen_current_search_constraint();
    if ((constraint.data == NULL) || (constraint.len <= 0)) {
        return false;
    }

    if (((menu = action_runtime_current_menu()) == NULL)
        || nc_menu_empty(menu)) {
        return false;
    }

    original = nc_menu_highlight(menu);
    height = action_runtime_current_menu_height();
    nc_menu_highlight_position(menu, 0, height);
    ncm_error_clear(&ncm_error);
    found = current_screen_search(NCM_SEARCH_DIRECTION_FORWARD, constraint.data,
                                  constraint.len, false, false, &ncm_error);
    if (ncm_error_is_set(&ncm_error)) {
        nc_menu_highlight_position(menu, original, height);
        return action_runtime_mpd_error(&ncm_error);
    }

    if (found) {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Searching for items...");
        (void)nc_menu_set_current_selected(menu, true);
        while (true) {
            found = current_screen_search(NCM_SEARCH_DIRECTION_FORWARD,
                                          constraint.data, constraint.len,
                                          false, true, &ncm_error);
            if (!found) {
                break;
            }
            (void)nc_menu_set_current_selected(menu, true);
        }
        if (ncm_error_is_set(&ncm_error)) {
            nc_menu_highlight_position(menu, original, height);
            return action_runtime_mpd_error(&ncm_error);
        }
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Found items selected");
    }
    nc_menu_highlight_position(menu, original, height);
    nc_screen_finish_list_change(app_controller_current_screen());
    return true;
}

static bool
action_runtime_previous_column_available(void) {
    switch (app_screens_current_type()) {
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        return media_library_screen_previous_column_available(
            app_screen_media_library());
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        return playlist_editor_screen_previous_column_available(
            app_screen_playlist_editor());
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        return tag_editor_screen_previous_column_available(
            app_screen_tag_editor());
#endif
    case NCM_SCREEN_TYPE_BROWSER:
    case NCM_SCREEN_TYPE_HELP:
    case NCM_SCREEN_TYPE_LASTFM:
    case NCM_SCREEN_TYPE_LYRICS:
#if defined(ENABLE_OUTPUTS)
    case NCM_SCREEN_TYPE_OUTPUTS:
#endif
    case NCM_SCREEN_TYPE_PLAYLIST:
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
    case NCM_SCREEN_TYPE_SERVER_INFO:
    case NCM_SCREEN_TYPE_SONG_INFO:
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
#endif
#if defined(ENABLE_VISUALIZER)
    case NCM_SCREEN_TYPE_VISUALIZER:
#endif
    case NCM_SCREEN_TYPE_COUNT:
    default:
        break;
    }
    return false;
}

static bool
action_runtime_next_column_available(void) {
    switch (app_screens_current_type()) {
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        return media_library_screen_next_column_available(
            app_screen_media_library());
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        return playlist_editor_screen_next_column_available(
            app_screen_playlist_editor());
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        return tag_editor_screen_next_column_available(
            app_screen_tag_editor());
#endif
    case NCM_SCREEN_TYPE_BROWSER:
    case NCM_SCREEN_TYPE_HELP:
    case NCM_SCREEN_TYPE_LASTFM:
    case NCM_SCREEN_TYPE_LYRICS:
#if defined(ENABLE_OUTPUTS)
    case NCM_SCREEN_TYPE_OUTPUTS:
#endif
    case NCM_SCREEN_TYPE_PLAYLIST:
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
    case NCM_SCREEN_TYPE_SERVER_INFO:
    case NCM_SCREEN_TYPE_SONG_INFO:
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
#endif
#if defined(ENABLE_VISUALIZER)
    case NCM_SCREEN_TYPE_VISUALIZER:
#endif
    case NCM_SCREEN_TYPE_COUNT:
    default:
        break;
    }
    return false;
}

static bool
action_runtime_previous_column(void) {
    if (!action_runtime_previous_column_available()) {
        return false;
    }
    switch (app_screens_current_type()) {
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        media_library_screen_previous_column(app_screen_media_library());
        return true;
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        playlist_editor_screen_previous_column(app_screen_playlist_editor());
        return true;
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        tag_editor_screen_previous_column(app_screen_tag_editor());
        return true;
#endif
    case NCM_SCREEN_TYPE_BROWSER:
    case NCM_SCREEN_TYPE_HELP:
    case NCM_SCREEN_TYPE_LASTFM:
    case NCM_SCREEN_TYPE_LYRICS:
#if defined(ENABLE_OUTPUTS)
    case NCM_SCREEN_TYPE_OUTPUTS:
#endif
    case NCM_SCREEN_TYPE_PLAYLIST:
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
    case NCM_SCREEN_TYPE_SERVER_INFO:
    case NCM_SCREEN_TYPE_SONG_INFO:
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
#endif
#if defined(ENABLE_VISUALIZER)
    case NCM_SCREEN_TYPE_VISUALIZER:
#endif
    case NCM_SCREEN_TYPE_COUNT:
    default:
        break;
    }
    return false;
}

static bool
action_runtime_next_column(void) {
    if (!action_runtime_next_column_available()) {
        return false;
    }
    switch (app_screens_current_type()) {
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        media_library_screen_next_column(app_screen_media_library());
        return true;
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        playlist_editor_screen_next_column(app_screen_playlist_editor());
        return true;
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        tag_editor_screen_next_column(app_screen_tag_editor());
        return true;
#endif
    case NCM_SCREEN_TYPE_BROWSER:
    case NCM_SCREEN_TYPE_HELP:
    case NCM_SCREEN_TYPE_LASTFM:
    case NCM_SCREEN_TYPE_LYRICS:
#if defined(ENABLE_OUTPUTS)
    case NCM_SCREEN_TYPE_OUTPUTS:
#endif
    case NCM_SCREEN_TYPE_PLAYLIST:
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
    case NCM_SCREEN_TYPE_SERVER_INFO:
    case NCM_SCREEN_TYPE_SONG_INFO:
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
#endif
#if defined(ENABLE_VISUALIZER)
    case NCM_SCREEN_TYPE_VISUALIZER:
#endif
    case NCM_SCREEN_TYPE_COUNT:
    default:
        break;
    }
    return false;
}

static bool
action_runtime_enter_directory(void) {
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        return browser_screen_enter_directory(app_screen_browser());
    }
#if defined(HAVE_TAGLIB_H)
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TAG_EDITOR)) {
        return tag_editor_screen_enter_directory(app_screen_tag_editor());
    }
#endif
    return false;
}

static bool
action_runtime_jump_to_parent_directory(void) {
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        return browser_screen_go_to_parent(app_screen_browser());
    }
#if defined(HAVE_TAGLIB_H)
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TAG_EDITOR)) {
        return tag_editor_screen_go_to_parent(app_screen_tag_editor());
    }
#endif
    return false;
}

static bool
action_runtime_seek_relative(bool forward) {
    NcmError ncm_error;
    int32 position;
    int32 elapsed;
    int32 total;
    int32 target;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (ncm_status_state_player() == NCM_STATUS_PLAYER_STOP) {
        return false;
    }

    position = ncm_status_state_current_song_position();
    if (position < 0) {
        return false;
    }

    elapsed = ncm_status_state_elapsed_time();
    total = ncm_status_state_total_time();
    target = elapsed;
    if (forward) {
        target += Config.seek_time;
        if ((total > 0) && (target > total)) {
            target = total;
        }
    } else if (target > Config.seek_time) {
        target -= Config.seek_time;
    } else {
        target = 0;
    }

    ncm_error_clear(&ncm_error);
    if (!ncm_mpd_client_seek_pos(&global_mpd, position, target, &ncm_error)) {
        return action_runtime_mpd_error(&ncm_error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return true;
}

static bool
action_runtime_jump_to_playing_song(void) {
    NcmSong song;
    NcmError ncm_error;
    int32 position = ncm_status_state_current_song_position();
    bool success;

    if (position < 0) {
        return false;
    }

    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
        success = playlist_screen_locate_position(
            app_screen_playlist(), position);
        if (!success) {
            ncm_statusbar_print_cstring(Config.message_delay_time,
                                        "Song is filtered out");
        }
        return true;
    }
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY)) {
        song = (NcmSong){0};
        ncm_error_clear(&ncm_error);
        success = ncm_mpd_client_get_current_song(&global_mpd, &song,
                                                  &ncm_error);
        if (success) {
            success = media_library_screen_locate_song(
                app_screen_media_library(), &song, &ncm_error);
        }
        ncm_song_destroy(&song);
        if (!success) {
            return action_runtime_mpd_error(&ncm_error);
        }
        return true;
    }
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        return action_runtime_jump_to_browser();
    }
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        song = (NcmSong){0};
        ncm_error_clear(&ncm_error);
        success = ncm_mpd_client_get_current_song(&global_mpd, &song,
                                                  &ncm_error);
        if (success) {
            success = playlist_editor_screen_locate_song(
                app_screen_playlist_editor(), &global_mpd, &song, &ncm_error);
        }
        ncm_song_destroy(&song);
        if (!success && ncm_error_is_set(&ncm_error)) {
            return action_runtime_mpd_error(&ncm_error);
        }
        return true;
    }
    return false;
}

static bool
action_runtime_jump_to_browser(void) {
    NcmSong song;
    NcmError ncm_error;
    bool success;

    song = (NcmSong){0};
    if (!(success = action_runtime_current_song(&song))) {
        ncm_song_destroy(&song);
        return false;
    }

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        success = action_runtime_switch_to_screen(NCM_SCREEN_TYPE_BROWSER);
    }
    if (success) {
        ncm_error_clear(&ncm_error);
        success = browser_screen_locate_song(app_screen_browser(), &song,
                                             &global_mpd, &ncm_error);
        if (!success) {
            (void)action_runtime_mpd_error(&ncm_error);
        }
    }

    ncm_song_destroy(&song);
    return success;
}

static bool
action_runtime_jump_to_playlist_editor(void) {
    BrowserScreen *browser = app_screen_browser();
    NcmStringView path;
    NcmError ncm_error;
    bool success;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_PLAYLIST_EDITOR);
    }

    if (!browser_screen_current_playlist_path(browser, &path)) {
        return false;
    }

    success = action_runtime_switch_to_screen(NCM_SCREEN_TYPE_PLAYLIST_EDITOR);
    if (!success) {
        return false;
    }

    ncm_error_clear(&ncm_error);
    success = playlist_editor_screen_locate_playlist(
        app_screen_playlist_editor(), &global_mpd, path.data, path.len,
        &ncm_error);
    if (!success) {
        return action_runtime_mpd_error(&ncm_error);
    }
    return true;
}

static bool
action_runtime_jump_to_media_library(void) {
    NcmSong song;
    NcmError ncm_error;
    bool success;

    song = (NcmSong){0};
    if (!action_runtime_current_song(&song)) {
        ncm_song_destroy(&song);
        return false;
    }

    success = action_runtime_switch_to_screen(NCM_SCREEN_TYPE_MEDIA_LIBRARY);
    if (success) {
        ncm_statusbar_print_cstring(0, "Jumping to song...");
        ncm_error_clear(&ncm_error);
        success = media_library_screen_locate_song(
            app_screen_media_library(), &song, &ncm_error);
        if (!success) {
            (void)action_runtime_mpd_error(&ncm_error);
        }
    }
    ncm_song_destroy(&song);
    return success;
}

static bool
action_runtime_jump_to_tag_editor(void) {
#if defined(HAVE_TAGLIB_H)
    NcmStringView directory;
    NcmSong song;
    bool success;

    if (Config.mpd_music_dir_len <= 0) {
        ncm_statusbar_print_cstring(
            Config.message_delay_time,
            "Proper mpd_music_dir variable has to be set in "
            "configuration file");
        return false;
    }

    song = (NcmSong){0};
    if ((success = action_runtime_current_song(&song))) {
        success = ncm_song_directory_view(&song, 0, &directory)
                  && (directory.len > 0);
    }
    if (success) {
        success = action_runtime_switch_to_screen(NCM_SCREEN_TYPE_TAG_EDITOR);
    }
    if (success) {
        success = tag_editor_screen_locate_song(
            app_screen_tag_editor(), &song);
    }
    ncm_song_destroy(&song);
    return success;
#else
    return false;
#endif
}

static bool
action_runtime_edit_directory_name(void) {
    NcmStringView path;
    StrBuilder name = {0};
    NcmError ncm_error;
    bool prompted;
    bool success;

    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        BrowserScreen *browser = app_screen_browser();

        if (!browser_screen_current_directory_path(browser, &path)) {
            return false;
        }

        prompted = action_runtime_prompt_string(
            STRLIT("Directory: "), path.data, false, NULL, NULL, &name);
        if (!prompted) {
            sb_free(&name);
            return true;
        }
        if ((name.len <= 0)
            || STREQUAL(name.data, name.len, path.data, path.len)) {
            sb_free(&name);
            return true;
        }

        ncm_error_clear(&ncm_error);
        success = browser_screen_rename_current_directory(
            browser, name.data, name.len, &global_mpd, &ncm_error);
        if (success) {
            action_runtime_print_renamed(STRLIT("Directory renamed to \""),
                                         &name);
        }
        sb_free(&name);
        if (!success) {
            return action_runtime_mpd_error(&ncm_error);
        }
        return true;
    }

#if defined(HAVE_TAGLIB_H)
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TAG_EDITOR)) {
        return tag_editor_screen_rename_current_directory(
            app_screen_tag_editor(), Config.mpd_music_dir,
            Config.mpd_music_dir_len);
    }
#endif
    return false;
}

static bool
action_runtime_edit_playlist_name(void) {
    BrowserScreen *browser = app_screen_browser();
    PlaylistEditorScreen *screen = app_screen_playlist_editor();
    NcmPlaylist playlist;
    NcmStringView path;
    StrBuilder name = {0};
    NcmError ncm_error;
    bool prompted;
    bool success;

    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        if (!ncm_mpd_client_connected(&global_mpd)) {
            return false;
        }
        if (!browser_screen_current_playlist_path(browser, &path)) {
            return false;
        }

        prompted = action_runtime_prompt_string(
            STRLIT("Playlist: "), path.data, false, NULL, NULL, &name);
        if (!prompted) {
            sb_free(&name);
            return true;
        }
        if ((name.len <= 0)
            || STREQUAL(name.data, name.len, path.data, path.len)) {
            sb_free(&name);
            return true;
        }

        ncm_error_clear(&ncm_error);
        success = browser_screen_rename_current_playlist(
            browser, name.data, name.len, &global_mpd, &ncm_error);
        if (success) {
            action_runtime_print_renamed(STRLIT("Playlist renamed to \""),
                                         &name);
        }
        sb_free(&name);
        if (!success) {
            return action_runtime_mpd_error(&ncm_error);
        }
        return true;
    }

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (!action_runtime_playlist_editor_playlists_active()) {
        return false;
    }
    if (!action_runtime_playlist_editor_has_playlists()) {
        return false;
    }

    playlist = (NcmPlaylist){0};
    success = playlist_editor_screen_current_playlist(screen, &playlist);
    if (!success) {
        ncm_playlist_destroy(&playlist);
        return false;
    }

    prompted = action_runtime_prompt_string(
        STRLIT("Playlist: "), playlist.path, false, NULL, NULL, &name);
    if (!prompted) {
        sb_free(&name);
        ncm_playlist_destroy(&playlist);
        return true;
    }
    if ((name.len <= 0)
        || STREQUAL(name.data, name.len, playlist.path, playlist.path_len)) {
        sb_free(&name);
        ncm_playlist_destroy(&playlist);
        return true;
    }

    ncm_error_clear(&ncm_error);
    success = ncm_mpd_client_rename_playlist(&global_mpd, playlist.path,
                                             name.data, &ncm_error);
    if (success) {
        action_runtime_print_renamed(STRLIT("Playlist renamed to \""),
                                     &name);
        playlist_editor_screen_request_playlists_update(screen);
    }
    sb_free(&name);
    ncm_playlist_destroy(&playlist);
    if (!success) {
        return action_runtime_mpd_error(&ncm_error);
    }
    return true;
}

static bool
action_runtime_toggle_display_mode(void) {
    enum ScreenType screen_type = app_screens_current_type();

    if (screen_type == NCM_SCREEN_TYPE_SEARCH_ENGINE) {
        SearchEngineScreen *screen = app_screen_search_engine();
        NcmStringFormatArg arg;
        enum DisplayMode search_mode;

        search_mode = search_engine_screen_toggle_display_mode(screen);
        arg = ncm_string_format_arg_cstring(ncm_display_mode_str(search_mode));
        ncm_statusbar_format(Config.message_delay_time,
                             STRLIT("Search engine display mode: %1%"),
                             &arg, 1);
        app_controller_request_current_screen_resize();
        app_controller_refresh_current_screen();
        return true;
    }

    switch (screen_type) {
    case NCM_SCREEN_TYPE_BROWSER:
        if (Config.browser_display_mode == NCM_DISPLAY_MODE_CLASSIC) {
            Config.browser_display_mode = NCM_DISPLAY_MODE_COLUMNS;
        } else {
            Config.browser_display_mode = NCM_DISPLAY_MODE_CLASSIC;
        }
        browser_screen_set_display_mode(app_screen_browser(),
                                        Config.browser_display_mode);
        app_controller_request_current_screen_resize();
        return true;
    case NCM_SCREEN_TYPE_PLAYLIST: {
        NcmStringFormatArg arg;

        if (Config.playlist_display_mode == NCM_DISPLAY_MODE_CLASSIC) {
            Config.playlist_display_mode = NCM_DISPLAY_MODE_COLUMNS;
        } else {
            Config.playlist_display_mode = NCM_DISPLAY_MODE_CLASSIC;
        }
        playlist_screen_update_column_title(app_screen_playlist());
        app_controller_request_current_screen_resize();
        app_controller_refresh_current_screen();
        arg = ncm_string_format_arg_cstring(ncm_display_mode_str(
            Config.playlist_display_mode));
        ncm_statusbar_format(Config.message_delay_time,
                             STRLIT("Playlist display mode: %1%"), &arg,
                             1);
        return true;
    }
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        if (Config.playlist_editor_display_mode == NCM_DISPLAY_MODE_CLASSIC) {
            Config.playlist_editor_display_mode = NCM_DISPLAY_MODE_COLUMNS;
        } else {
            Config.playlist_editor_display_mode = NCM_DISPLAY_MODE_CLASSIC;
        }
        app_controller_request_current_screen_resize();
        return true;
    case NCM_SCREEN_TYPE_HELP:
    case NCM_SCREEN_TYPE_LASTFM:
    case NCM_SCREEN_TYPE_LYRICS:
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
#if defined(ENABLE_OUTPUTS)
    case NCM_SCREEN_TYPE_OUTPUTS:
#endif
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
    case NCM_SCREEN_TYPE_SERVER_INFO:
    case NCM_SCREEN_TYPE_SONG_INFO:
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
#endif
#if defined(ENABLE_VISUALIZER)
    case NCM_SCREEN_TYPE_VISUALIZER:
#endif
    case NCM_SCREEN_TYPE_COUNT:
    default:
        break;
    }
    return false;
}

static bool
action_runtime_change_browse_mode(void) {
    BrowserScreen *browser = app_screen_browser();
    NcmError ncm_error;
    char *message;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        return false;
    }

    ncm_error_clear(&ncm_error);
    if (!browser_screen_change_browse_mode(browser, &global_mpd,
                                           &ncm_error)) {
        if (ncm_error.code == EINVAL) {
            ncm_statusbar_print_cstring(
                Config.message_delay_time,
                "For browsing local filesystem connection to MPD "
                "via UNIX Socket is required");
        } else if (ncm_error_is_set(&ncm_error)) {
            ncm_statusbar_print_cstring(Config.message_delay_time,
                                        ncm_error.message);
        }
        return false;
    }

    if (browser_screen_is_local(browser)) {
        message = "Browse mode: local filesystem";
    } else {
        message = "Browse mode: MPD database";
    }
    ncm_statusbar_print_cstring(Config.message_delay_time, message);
    return true;
}

static bool
action_runtime_toggle_browser_sort_mode(void) {
    char *message;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        return false;
    }
    Config.browser_sort_mode += 1;
    if (Config.browser_sort_mode >= NCM_SORT_MODE_COUNT) {
        Config.browser_sort_mode = NCM_SORT_MODE_TYPE;
    }

    switch (Config.browser_sort_mode) {
    case NCM_SORT_MODE_TYPE:
        message = "Sort songs by: type";
        break;
    case NCM_SORT_MODE_NAME:
        message = "Sort songs by: name";
        break;
    case NCM_SORT_MODE_MODIFICATION_TIME:
        message = "Sort songs by: modification time";
        break;
    case NCM_SORT_MODE_CUSTOM_FORMAT:
        message = "Sort songs by: custom format";
        break;
    case NCM_SORT_MODE_NONE:
        message = "Do not sort songs";
        break;
    case NCM_SORT_MODE_COUNT:
        message = "Sort songs by: type";
        break;
    default:
        Config.browser_sort_mode = NCM_SORT_MODE_TYPE;
        message = "Sort songs by: type";
        break;
    }
    ncm_statusbar_print_cstring(Config.message_delay_time, message);
    (void)browser_screen_sort(app_screen_browser());
    app_controller_request_current_screen_update();
    return true;
}

static bool
action_runtime_toggle_library_tag_type(void) {
    MediaLibraryScreen *screen = app_screen_media_library();
    enum mpd_tag_type tag_type = MPD_TAG_ARTIST;
    enum MediaLibraryColumn column;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY)) {
        return false;
    }
    column = media_library_screen_active_column(screen);
    if ((column != MEDIA_LIBRARY_COLUMN_TAGS)
        && ((media_library_screen_column_count(screen) != 2)
            || (column != MEDIA_LIBRARY_COLUMN_ALBUMS))) {
        return false;
    }

    if (Config.media_lib_primary_tag == MPD_TAG_ARTIST) {
        tag_type = MPD_TAG_ALBUM_ARTIST;
    } else if (Config.media_lib_primary_tag == MPD_TAG_ALBUM_ARTIST) {
        tag_type = MPD_TAG_DATE;
    } else if (Config.media_lib_primary_tag == MPD_TAG_DATE) {
        tag_type = MPD_TAG_GENRE;
    } else if (Config.media_lib_primary_tag == MPD_TAG_GENRE) {
        tag_type = MPD_TAG_COMPOSER;
    } else if (Config.media_lib_primary_tag == MPD_TAG_COMPOSER) {
        tag_type = MPD_TAG_PERFORMER;
    }

    return media_library_screen_set_primary_tag_type(screen, tag_type);
}

static bool
action_runtime_toggle_media_library_sort_mode(void) {
    bool sort_by_mtime;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY)) {
        return false;
    }
    sort_by_mtime = media_library_screen_toggle_sort_mode(
        app_screen_media_library());
    if (sort_by_mtime) {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Sorting library by: modification time");
    } else {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Sorting library by: name");
    }
    return true;
}

static bool
action_runtime_toggle_media_library_columns(void) {
    MediaLibraryScreen *screen = app_screen_media_library();

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY)) {
        return false;
    }
    (void)media_library_screen_toggle_mode(screen);
    app_controller_request_current_screen_resize();
    return true;
}

static char *
action_runtime_replay_gain_mode_name(enum NcmMpdReplayGainMode mode) {
    switch (mode) {
    case NCM_MPD_REPLAY_GAIN_OFF:
        return "off";
    case NCM_MPD_REPLAY_GAIN_TRACK:
        return "track";
    case NCM_MPD_REPLAY_GAIN_ALBUM:
        return "album";
    case NCM_MPD_REPLAY_GAIN_COUNT:
    default:
        break;
    }
    return "unknown";
}

static bool
action_runtime_toggle_replay_gain_mode(void) {
    NcmError ncm_error;
    NcWindow *window;
    char choice;
    enum NcmMpdReplayGainMode mode;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    if ((window = app_controller_active_window()) == NULL) {
        return false;
    }

    ncm_statusbar_print_cstring(0,
                                "Replay gain: off [o], track [t], album [a]");
    choice = 'o';
    if (!ncm_statusbar_prompt_return_one_of(window, "ota", 3, &choice)) {
        return false;
    }

    mode = NCM_MPD_REPLAY_GAIN_OFF;
    if (choice == 't') {
        mode = NCM_MPD_REPLAY_GAIN_TRACK;
    } else if (choice == 'a') {
        mode = NCM_MPD_REPLAY_GAIN_ALBUM;
    }

    ncm_error_clear(&ncm_error);
    if (!ncm_mpd_client_set_replay_gain_mode(&global_mpd, mode, &ncm_error)) {
        return action_runtime_mpd_error(&ncm_error);
    }
    ncm_error_clear(&ncm_error);
    if (!ncm_mpd_client_get_replay_gain_mode(&global_mpd, &mode, &ncm_error)) {
        return action_runtime_mpd_error(&ncm_error);
    }
    action_runtime_print_toggle(STRLIT("Replay gain mode: %1%"),
                                action_runtime_replay_gain_mode_name(mode));
    return true;
}

static bool
action_runtime_save_tag_changes(void) {
#if defined(HAVE_TAGLIB_H)
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TAG_EDITOR)) {
        if (!tag_editor_screen_save_action_available(
            app_screen_tag_editor())) {
            return false;
        }
        return tag_editor_screen_save_modified(
            app_screen_tag_editor(), Config.mpd_music_dir);
    }
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TINY_TAG_EDITOR)) {
        return tiny_tag_editor_screen_run_row(
            app_screen_tiny_tag_editor(), TINY_TAG_EDITOR_SAVE_ROW);
    }
#endif
    return false;
}

bool
ncm_action_edit_song(NcmSong *song) {
#if defined(HAVE_TAGLIB_H)
    enum TinyTagEditorOpenResult open_result;
    NcmStringFormatArg arg;
    StrBuilder path = {0};
    int32 path_len;
    int32 path_width;
    bool success = false;

    if (song == NULL) {
        return false;
    }
    if (Config.mpd_music_dir_len <= 0) {
        ncm_statusbar_print_cstring(
            Config.message_delay_time,
            "Proper mpd_music_dir variable has to be set in "
            "configuration file");
        return false;
    }

    open_result = tiny_tag_editor_screen_open_song(
        app_screen_tiny_tag_editor(), song, Config.mpd_music_dir,
        Config.mpd_music_dir_len, Config.tags_separator,
        Config.tags_separator_len, Config.show_duplicate_tags, &path);
    switch (open_result) {
    case TINY_TAG_EDITOR_OPEN_SUCCESS:
        success = action_runtime_switch_to_screen(
            NCM_SCREEN_TYPE_TINY_TAG_EDITOR);
        break;
    case TINY_TAG_EDITOR_OPEN_STREAM:
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Streams can't be edited");
        break;
    case TINY_TAG_EDITOR_OPEN_MISSING_MUSIC_DIRECTORY:
        ncm_statusbar_print_cstring(
            Config.message_delay_time,
            "Proper mpd_music_dir variable has to be set in "
            "configuration file");
        break;
    case TINY_TAG_EDITOR_OPEN_UNREADABLE_FILE:
        path_width = COLS - STRLIT_LEN("Couldn't read file \"%1%\"");
        if (path_width < 0) {
            path_width = 0;
        }
        path_len = utf8_cut_width(path.data, path.len, path_width);
        arg = ncm_string_format_arg_string(path.data, path_len);
        ncm_statusbar_format(Config.message_delay_time,
                             STRLIT("Couldn't read file \"%1%\""), &arg,
                             1);
        break;
    case TINY_TAG_EDITOR_OPEN_INVALID_ARGUMENT:
    case TINY_TAG_EDITOR_OPEN_PREPARE_FAILED:
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Couldn't prepare tiny tag editor");
        break;
    case TINY_TAG_EDITOR_OPEN_COUNT:
    default:
        break;
    }
    sb_free(&path);
    return success;
#else
    (void)song;
    return false;
#endif
}

static bool
action_runtime_media_library_current_artist_tag(char **artist,
                                                int32 *artist_len) {
    MediaLibraryScreen *library = app_screen_media_library();
    char *value;
    int32 value_len;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY)) {
        return false;
    }
    if (Config.media_lib_primary_tag != MPD_TAG_ARTIST) {
        return false;
    }

    if (media_library_screen_active_column(library)
        != MEDIA_LIBRARY_COLUMN_TAGS) {
        return false;
    }
    if (!media_library_screen_current_primary_tag_value(library, &value,
                                                        &value_len)) {
        return false;
    }

    if (artist) {
        *artist = value;
    }
    if (artist_len) {
        *artist_len = value_len;
    }
    return true;
}

static bool
action_runtime_toggle_screen_lock(void) {
    NcmStringFormatArg args[3];
    StrBuilder input = {0};
    NcmError ncm_error;
    NcScreen *current;
    char initial[16];
    int32 part = (int32)Config.locked_screen_width_part*100;
    bool prompted;

    if (app_controller_locked_screen()) {
        app_controller_unlock_screen();
        app_screens_request_registered_resize();
        app_screen_lyrics_set_resize();
        app_controller_resize_current_screen();
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Screen unlocked");
        return true;
    }

    if (((current = app_controller_current_screen()) == NULL)
        || !nc_screen_is_lockable(current)) {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Current screen can't be locked");
        return true;
    }

    if (Config.ask_for_locked_screen_width_part) {
        SNPRINTF(initial, "%d", part);
        prompted = action_runtime_prompt_string(
            STRLIT("% of the locked screen's width to be reserved "
                        "(20-80): "),
            initial, true, NULL, NULL, &input);
        if (!prompted) {
            sb_free(&input);
            ncm_statusbar_print_cstring(Config.message_delay_time,
                                        "Action aborted");
            return true;
        }

        ncm_error_clear(&ncm_error);
        if (!ncm_parse_int32(input.data, input.len, &part, &ncm_error)) {
            args[0] = ncm_string_format_arg_string(input.data, input.len);
            ncm_statusbar_format(Config.message_delay_time,
                                 STRLIT("Invalid value: %1%"), args, 1);
            sb_free(&input);
            return true;
        }
        sb_free(&input);
    }

    if ((part < 20) || (part > 80)) {
        args[0] = ncm_string_format_arg_u64(20);
        args[1] = ncm_string_format_arg_u64(80);
        args[2] = ncm_string_format_arg_u64((uint64)part);
        ncm_statusbar_format(Config.message_delay_time,
                             STRLIT("Error: value is out of bounds "
                                         "([%1%, %2%] expected, %3% given)"),
                             args, LENGTH(args));
        return true;
    }

    Config.locked_screen_width_part = part / 100.0;
    if (app_controller_lock_current_screen()) {
        args[0] = ncm_string_format_arg_u64((uint32)part);
        ncm_statusbar_format(Config.message_delay_time,
                             STRLIT("Screen locked (with %1%%% width)"),
                             args, 1);
    } else {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Current screen can't be locked");
    }
    return true;
}

#if defined(HAVE_TAGLIB_H)
static bool
action_runtime_mpd_music_dir_is_set(void) {
    if (Config.mpd_music_dir_len > 0) {
        return true;
    }

    ncm_statusbar_print_cstring(
        Config.message_delay_time,
        "Proper mpd_music_dir variable has to be set in "
        "configuration file");
    return false;
}

static bool
action_runtime_media_library_current_tag(char **tag, int32 *tag_len) {
    MediaLibraryScreen *library = app_screen_media_library();

    if ((tag == NULL) || (tag_len == NULL)) {
        return false;
    }
    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY)) {
        return false;
    }

    if (media_library_screen_active_column(library)
        != MEDIA_LIBRARY_COLUMN_TAGS) {
        return false;
    }
    return media_library_screen_current_primary_tag_value(library, tag,
                                                          tag_len);
}

static bool
action_runtime_media_library_current_album(char **album, int32 *album_len) {
    MediaLibraryScreen *library = app_screen_media_library();

    if ((album == NULL) || (album_len == NULL)) {
        return false;
    }
    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY)) {
        return false;
    }

    if (media_library_screen_active_column(library)
        != MEDIA_LIBRARY_COLUMN_ALBUMS) {
        return false;
    }
    return media_library_screen_current_album_value(library, album,
                                                    album_len);
}

static bool
action_runtime_can_edit_library_tag(void) {
    char *tag;
    int32 tag_len;

    return (Config.mpd_music_dir_len > 0)
           && action_runtime_media_library_current_tag(&tag, &tag_len);
}

static bool
action_runtime_can_edit_library_album(void) {
    char *album;
    int32 album_len;

    return (Config.mpd_music_dir_len > 0)
           && action_runtime_media_library_current_album(&album, &album_len);
}

static bool
action_runtime_song_uri_view(NcmSong *song, NcmStringView *uri) {
    if (uri) {
        *uri = (NcmStringView){0};
    }
    return ncm_song_uri_view(song, 0, uri);
}

static bool
action_runtime_song_name_or_uri_view(NcmSong *song, NcmStringView *view) {
    if (view) {
        *view = (NcmStringView){0};
    }
    if (ncm_song_name_view(song, 0, view)) {
        return true;
    }
    return action_runtime_song_uri_view(song, view);
}

static bool
action_runtime_shared_directory_update(StrBuilder *shared_directory,
                                       bool *valid, char *directory,
                                       int32 directory_len) {
    StrBuilder shared;

    if ((shared_directory == NULL) || (valid == NULL)) {
        return false;
    }
    if (directory == NULL) {
        directory = "";
        directory_len = 0;
    }

    if (!*valid) {
        *valid = true;
        return sb_set(shared_directory, directory, directory_len) >= 0;
    }

    shared = ncm_string_shared_directory(shared_directory->data,
                                         shared_directory->len, directory,
                                         directory_len);
    sb_free(shared_directory);
    *shared_directory = shared;
    return true;
}

static void
action_runtime_print_updating_song(NcmSong *song) {
    NcmStringView name;
    NcmStringFormatArg arg;

    if (!action_runtime_song_name_or_uri_view(song, &name)) {
        return;
    }

    arg = ncm_string_format_arg_string(name.data, name.len);
    ncm_statusbar_format(0, STRLIT("Updating tags in \"%1%\"..."), &arg,
                         1);
    return;
}

static void
action_runtime_print_album_file_error(char *format, int32 format_len,
                                      NcmSong *song) {
    NcmStringView uri;
    NcmStringFormatArg arg;
    int32 width;
    int32 uri_len;

    if (!action_runtime_song_uri_view(song, &uri)) {
        return;
    }

    width = COLS - format_len;
    if (width < 0) {
        width = 0;
    }
    uri_len = utf8_cut_width(uri.data, uri.len, width);
    arg = ncm_string_format_arg_string(uri.data, uri_len);
    ncm_statusbar_format(Config.message_delay_time, format, format_len,
                         &arg, 1);
    return;
}

static bool
action_runtime_update_tag_directory(StrBuilder *shared_directory, bool valid) {
    NcmError ncm_error;

    if (!valid) {
        return true;
    }

    {
        char *directory = shared_directory->data;

        if (directory == NULL) {
            directory = "";
        }

        ncm_error_clear(&ncm_error);
        if (!ncm_mpd_client_update_directory(&global_mpd, directory, NULL,
                                             &ncm_error)) {
            return action_runtime_mpd_error(&ncm_error);
        }
    }
    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Tags updated successfully");
    return true;
}

static bool
action_runtime_edit_library_tag(void) {
    enum NcmTagsField field;
    NcmMpdSongList songs;
    NcmMutableSong mutable_song;
    NcmStringView uri;
    StrBuilder current_tag = {0};
    StrBuilder prompt = {0};
    StrBuilder new_tag = {0};
    StrBuilder shared_directory = {0};
    NcmError ncm_error;
    char *tag;
    int32 tag_len;
    bool shared_directory_valid = false;
    bool prompted;
    bool success = false;

    if (!action_runtime_mpd_music_dir_is_set()) {
        return false;
    }
    if (!action_runtime_media_library_current_tag(&tag, &tag_len)) {
        return false;
    }

    songs = (NcmMpdSongList){0};

    if (sb_set(&current_tag, tag, tag_len) < 0) {
        goto cleanup;
    }
    SB_APPEND(
        &prompt, ncm_tag_type_name(Config.media_lib_primary_tag),
        optional_strlen32(ncm_tag_type_name(Config.media_lib_primary_tag)));
    SB_APPEND(&prompt, STRLIT(": "));
    prompted = action_runtime_prompt_string(
        prompt.data, prompt.len, current_tag.data, false, NULL, NULL, &new_tag);
    if (!prompted) {
        success = true;
        goto cleanup;
    }
    if ((new_tag.len <= 0)
        || STREQUAL(new_tag.data, new_tag.len, current_tag.data,
                    current_tag.len)) {
        success = true;
        goto cleanup;
    }

    field = ncm_tags_field_from_tag_type(Config.media_lib_primary_tag);
    if (field == NCM_TAGS_FIELD_COUNT) {
        goto cleanup;
    }

    ncm_statusbar_print_cstring(0, "Updating tags...");
    ncm_error_clear(&ncm_error);
    if (!ncm_mpd_client_start_search(&global_mpd, true, &ncm_error)
        || !ncm_mpd_client_add_search_tag(
            &global_mpd, Config.media_lib_primary_tag, current_tag.data,
            &ncm_error)
        || !ncm_mpd_client_commit_search_songs(&global_mpd, &songs,
                                               &ncm_error)) {
        success = action_runtime_mpd_error(&ncm_error);
        goto cleanup;
    }

    success = true;
    for (int32 i = 0; i < ncm_mpd_song_list_count(&songs); i += 1) {
        NcmSong *song = ncm_mpd_song_list_at(&songs, i);

        mutable_song = (NcmMutableSong){0};
        if (!ncm_mutable_song_load_originals_from_song(&mutable_song, song)
            || !ncm_mutable_song_set_tags(&mutable_song, field, new_tag.data,
                                          new_tag.len, Config.tags_separator,
                                          Config.tags_separator_len)) {
            ncm_mutable_song_destroy(&mutable_song);
            success = false;
            break;
        }

        action_runtime_print_updating_song(song);
        if (!ncm_mutable_song_write(&mutable_song, Config.mpd_music_dir)) {
            NcmStringView name;
            NcmStringFormatArg args[2];

            if (action_runtime_song_name_or_uri_view(song, &name)) {
                args[0] = ncm_string_format_arg_string(name.data, name.len);
                args[1] = ncm_string_format_arg_cstring(strerror(errno));
                ncm_statusbar_format(
                    Config.message_delay_time,
                    STRLIT("Error while writing tags to \"%1%\": %2%"),
                    args, LENGTH(args));
            }
            ncm_mutable_song_destroy(&mutable_song);
            success = false;
            break;
        }

        if (action_runtime_song_uri_view(song, &uri)) {
            success = action_runtime_shared_directory_update(
                &shared_directory, &shared_directory_valid, uri.data, uri.len);
        }
        ncm_mutable_song_destroy(&mutable_song);
        if (!success) {
            break;
        }
    }

    if (success) {
        success = action_runtime_update_tag_directory(&shared_directory,
                                                      shared_directory_valid);
    }

cleanup:
    ncm_mpd_song_list_destroy(&songs);
    sb_free(&shared_directory);
    sb_free(&new_tag);
    sb_free(&prompt);
    sb_free(&current_tag);
    return success;
}

static bool
action_runtime_edit_library_album(void) {
    NcmSongArray songs;
    StrBuilder current_album = {0};
    StrBuilder new_album = {0};
    StrBuilder path = {0};
    StrBuilder shared_directory = {0};
    NcmError ncm_error;
    char *album;
    int32 album_len;
    bool shared_directory_valid = false;
    bool prompted;
    bool success = false;

    if (!action_runtime_mpd_music_dir_is_set()) {
        return false;
    }
    if (!action_runtime_media_library_current_album(&album, &album_len)) {
        return false;
    }

    songs = (NcmSongArray){0};
    ncm_error_clear(&ncm_error);

    if (sb_set(&current_album, album, album_len) < 0) {
        goto cleanup;
    }
    prompted = action_runtime_prompt_string(STRLIT("Album: "),
                                            current_album.data, false, NULL,
                                            NULL, &new_album);
    if (!prompted) {
        success = true;
        goto cleanup;
    }
    if ((new_album.len <= 0)
        || STREQUAL(new_album.data, new_album.len, current_album.data,
                    current_album.len)) {
        success = true;
        goto cleanup;
    }

    if (!media_library_screen_copy_visible_songs(
        app_screen_media_library(), &songs, &ncm_error)) {
        if (ncm_error_is_set(&ncm_error)) {
            ncm_statusbar_print_cstring(Config.message_delay_time,
                                        ncm_error.message);
        }
        goto cleanup;
    }

    ncm_statusbar_print_cstring(0, "Updating tags...");
    success = true;
    for (int32 i = 0; i < songs.len; i += 1) {
        NcmSong *song = &songs.items[i];
        NcmStringView directory;
        NcmStringView uri;
        NcmTaglibFile file;

        action_runtime_print_updating_song(song);
        sb_clear(&path);
        if (!action_runtime_song_uri_view(song, &uri)) {
            success = false;
            break;
        }
        SB_APPEND(&path, Config.mpd_music_dir, Config.mpd_music_dir_len);
        SB_APPEND(&path, uri.data, uri.len);
        if (ncm_song_directory_view(song, 0, &directory)) {
            success = action_runtime_shared_directory_update(
                &shared_directory, &shared_directory_valid, directory.data,
                directory.len);
            if (!success) {
                break;
            }
        }

        file = (NcmTaglibFile){0};
        if (!ncm_taglib_file_open(&file, path.data)) {
            action_runtime_print_album_file_error(
                STRLIT("Error while opening file \"%1%\""), song);
            success = false;
            break;
        }
        ncm_taglib_clear_property(&file, "ALBUM");
        ncm_taglib_append_property(&file, "ALBUM", new_album.data);
        if (!ncm_taglib_file_save(&file)) {
            action_runtime_print_album_file_error(
                STRLIT("Error while writing tags in \"%1%\""), song);
            ncm_taglib_file_close(&file);
            success = false;
            break;
        }
        ncm_taglib_file_close(&file);
    }

    if (success) {
        success = action_runtime_update_tag_directory(&shared_directory,
                                                      shared_directory_valid);
    }

cleanup:
    sb_free(&shared_directory);
    sb_free(&path);
    sb_free(&new_album);
    sb_free(&current_album);
    ncm_song_array_destroy(&songs);
    return success;
}
#else
static bool
action_runtime_can_edit_library_tag(void) {
    return false;
}

static bool
action_runtime_can_edit_library_album(void) {
    return false;
}

static bool
action_runtime_edit_library_tag(void) {
    return false;
}

static bool
action_runtime_edit_library_album(void) {
    return false;
}
#endif

static bool
action_runtime_edit_current_song(void) {
#if defined(HAVE_TAGLIB_H)
    NcmSong song;
    bool success;

    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_LYRICS)) {
        return false;
    }
    if (!action_runtime_has_current_song()) {
        return false;
    }
    song = (NcmSong){0};
    if ((success = action_runtime_current_song(&song))) {
        success = ncm_action_edit_song(&song);
    }
    ncm_song_destroy(&song);
    return success;
#else
    return false;
#endif
}

static bool
action_runtime_toggle_lyrics_fetcher(void) {
    NcmLyricsFetcherDef *fetcher;
    NcmStringFormatArg arg;

    fetcher = lyrics_screen_toggle_fetcher(app_screen_lyrics(),
                                           &Config.lyrics_fetchers);
    if (fetcher == NULL) {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Using all lyrics fetchers");
        return true;
    }

    arg = ncm_string_format_arg_string(ncm_lyrics_fetcher_name(fetcher),
                                       ncm_lyrics_fetcher_name_len(fetcher));
    ncm_statusbar_format(Config.message_delay_time,
                         STRLIT("Using lyrics fetcher: %1%"), &arg, 1);
    return true;
}

static bool
action_runtime_edit_lyrics(void) {
    LyricsScreen *lyrics = app_screen_lyrics();
    NcmSong *song;
    StrBuilder *filename;
    StrBuilder escaped = {0};
    StrBuilder command = {0};
    NcmError ncm_error;
    bool success;

    if (Config.external_editor_len <= 0) {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "external_editor variable has to be set in "
                                    "configuration file");
        return false;
    }

    if ((song = lyrics_screen_song(lyrics)) == NULL) {
        return false;
    }

    if (!lyrics_screen_build_filename(
        lyrics, song, Config.mpd_music_dir, Config.mpd_music_dir_len,
        Config.lyrics_directory, Config.lyrics_directory_len,
        Config.store_lyrics_in_song_dir,
        Config.generate_win32_compatible_filenames)) {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "failed to build lyrics "
                                    "filename");
        return false;
    }

    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Opening lyrics in external "
                                "editor...");
    filename = lyrics_screen_filename(lyrics);
    ncm_string_append_shell_escaped_single_quotes(&escaped, filename->data,
                                                  filename->len);
    SB_APPEND(&command, Config.external_editor, Config.external_editor_len);
    SB_APPEND(&command, STRLIT(" '"));
    SB_APPEND(&command, escaped.data, escaped.len);
    sb_append_byte(&command, '\'');

    ncm_error_clear(&ncm_error);
    if (Config.use_console_editor) {
        nc_pause_screen();
        success = ncm_macro_run_external_console_command(command.data,
                                                         command.len,
                                                         &ncm_error);
        nc_unpause_screen();
        if (!success) {
            sb_free(&command);
            sb_free(&escaped);
            return action_runtime_mpd_error(&ncm_error);
        }

        if (!lyrics_screen_load_file(lyrics, filename->data,
                                     filename->len, &ncm_error)) {
            lyrics->has_song = false;
            success = lyrics_screen_fetch(lyrics, song, NULL, &ncm_error);
        }
    } else {
        success = ncm_macro_run_external_command(command.data, command.len,
                                                 false, &ncm_error);
    }

    sb_free(&command);
    sb_free(&escaped);
    if (!success) {
        return action_runtime_mpd_error(&ncm_error);
    }
    ncm_error_clear(&ncm_error);
    return true;
}

static bool
action_runtime_fetch_lyrics_background(void) {
    NcmSongArray songs;
    NcmError ncm_error;

    songs = (NcmSongArray){0};
    if (!action_runtime_selected_songs(&songs) || (songs.len <= 0)) {
        ncm_song_array_destroy(&songs);
        return false;
    }

    ncm_error_clear(&ncm_error);
    for (int32 i = 0; i < songs.len; i += 1) {
        if (!lyrics_screen_fetch_in_background(
            app_screen_lyrics(), &songs.items[i], true, &ncm_error)) {
            ncm_song_array_destroy(&songs);
            return action_runtime_mpd_error(&ncm_error);
        }
    }

    ncm_song_array_destroy(&songs);
    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Selected songs queued for lyrics fetching");
    return true;
}

static bool
action_runtime_refetch_lyrics(void) {
    NcmError ncm_error;

    ncm_error_clear(&ncm_error);
    lyrics_screen_refetch_current(app_screen_lyrics(), &ncm_error);
    return !ncm_error_is_set(&ncm_error);
}

static bool
action_runtime_show_lyrics(void) {
    NcmSong song;
    NcmSong *lyrics_song;
    NcmError ncm_error;
    bool success;

    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_LYRICS)) {
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_LYRICS);
    }

    song = (NcmSong){0};
    if ((success = action_runtime_current_song(&song))) {
        lyrics_song = lyrics_screen_song(app_screen_lyrics());
        if ((lyrics_song == NULL) || !ncm_song_equal(lyrics_song, &song)) {
            ncm_error_clear(&ncm_error);
            success = lyrics_screen_fetch(app_screen_lyrics(), &song, NULL,
                                          &ncm_error);
        }
    }
    ncm_song_destroy(&song);
    if (!success) {
        return false;
    }
    return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_LYRICS);
}

static bool
action_runtime_show_artist_info(void) {
    NcmSong song;
    NcmStringView artist;
    NcmError ncm_error;
    char *media_library_artist = NULL;
    int32 media_library_artist_len = 0;
    bool has_artist = false;
    bool success;

    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_LASTFM)) {
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_LASTFM);
    }

    artist = (NcmStringView){0};
    if (action_runtime_media_library_current_artist_tag(
        &media_library_artist, &media_library_artist_len)) {
        artist.data = media_library_artist;
        artist.len = media_library_artist_len;
        has_artist = true;
    }

    song = (NcmSong){0};
    if (!has_artist) {
        if (!action_runtime_current_song(&song)) {
            ncm_song_destroy(&song);
            return false;
        }
        has_artist = ncm_song_tag_view(&song, MPD_TAG_ARTIST, 0, &artist);
    }

    if (has_artist && (artist.len > 0)) {
        ncm_error_clear(&ncm_error);
        success = lastfm_screen_queue_artist_info(
            app_screen_lastfm(), artist.data, artist.len,
            Config.lastfm_preferred_language,
            Config.lastfm_preferred_language_len, &ncm_error);
        ncm_song_destroy(&song);
        if (!success) {
            return false;
        }
        if (!app_controller_is_screen_visible(app_screen_lastfm_base())) {
            return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_LASTFM);
        }
        return true;
    }

    ncm_song_destroy(&song);
    return true;
}

static bool
action_runtime_mouse_event(void) {
    NcmError ncm_error;
    NcWindow *window;
    MEVENT *event;
    int32 position;
    int32 progressbar_y;
    int32 player_state_y;
    int32 seconds;

    if (!Config.mouse_support) {
        return false;
    }

    if ((window = ui_state_footer_window()) == NULL) {
        return false;
    }
    if ((event = nc_window_mouse_event(window)) == NULL) {
        return false;
    }

    progressbar_y = LINES - 1;
    if (Config.statusbar_visibility) {
        progressbar_y -= 1;
    }
    player_state_y = LINES - 1;
    if (Config.design == NCM_DESIGN_ALTERNATIVE) {
        player_state_y = 1;
    }

    if ((event->bstate & BUTTON1_PRESSED)
        && (event->y == progressbar_y)) {
        if (ncm_status_state_player() == NCM_STATUS_PLAYER_STOP) {
            return true;
        }
        position = ncm_status_state_current_song_position();
        seconds = (ncm_status_state_total_time()*event->x / COLS);
        ncm_error_clear(&ncm_error);
        if (!ncm_mpd_client_seek_pos(&global_mpd, position, seconds,
                                     &ncm_error)) {
            return action_runtime_mpd_error(&ncm_error);
        }
    } else if ((event->bstate & BUTTON1_PRESSED)
               && (Config.statusbar_visibility
                   || (Config.design == NCM_DESIGN_ALTERNATIVE))
               && (ncm_status_state_player() != NCM_STATUS_PLAYER_STOP)
               && (event->y == player_state_y)
               && (event->x < 9)) {
        ncm_error_clear(&ncm_error);
        if (!ncm_mpd_client_toggle_pause(&global_mpd, &ncm_error)) {
            return action_runtime_mpd_error(&ncm_error);
        }
    } else if (((event->bstate & BUTTON5_PRESSED)
                || (event->bstate & BUTTON4_PRESSED))
               && (Config.header_visibility
                   || (Config.design == NCM_DESIGN_ALTERNATIVE))
               && (event->y == 0)
               && (event->x > COLS - global_volume_state_len())) {
        if (event->bstate & BUTTON5_PRESSED) {
            return ncm_action_runtime_run(NULL, NCM_ACTION_VOLUME_DOWN);
        }
        return ncm_action_runtime_run(NULL, NCM_ACTION_VOLUME_UP);
    } else if (event->bstate
               & (BUTTON1_PRESSED | BUTTON3_PRESSED | BUTTON4_PRESSED
                  | BUTTON5_PRESSED)) {
        app_controller_mouse_button_pressed_current(*event);
    }
    return true;
}

static bool
action_runtime_builtin_can_run(NcmActionRuntime *runtime,
                               enum NcmActionType type) {
    (void)runtime;

    switch (type) {
    case NCM_ACTION_DUMMY:
    case NCM_ACTION_UPDATE_ENVIRONMENT:
    case NCM_ACTION_SCROLL_UP:
    case NCM_ACTION_SCROLL_DOWN:
    case NCM_ACTION_PAGE_UP:
    case NCM_ACTION_PAGE_DOWN:
    case NCM_ACTION_MOVE_HOME:
    case NCM_ACTION_MOVE_END:
    case NCM_ACTION_TOGGLE_INTERFACE:
    case NCM_ACTION_QUIT:
    case NCM_ACTION_NEXT_SCREEN:
    case NCM_ACTION_PREVIOUS_SCREEN:
    case NCM_ACTION_SHOW_SEARCH_ENGINE:
    case NCM_ACTION_SHOW_SONG_INFO:
    case NCM_ACTION_TOGGLE_SCREEN_LOCK:
    case NCM_ACTION_TOGGLE_PLAYING_SONG_CENTERING:
    case NCM_ACTION_TOGGLE_MOUSE:
    case NCM_ACTION_TOGGLE_BITRATE_VISIBILITY:
    case NCM_ACTION_TOGGLE_ADD_MODE:
    case NCM_ACTION_TOGGLE_LYRICS_FETCHER:
    case NCM_ACTION_TOGGLE_FETCHING_LYRICS_IN_BACKGROUND:
    case NCM_ACTION_TOGGLE_SEPARATORS_BETWEEN_ALBUMS:
        return true;
    case NCM_ACTION_SCROLL_UP_ARTIST:
    case NCM_ACTION_SCROLL_DOWN_ARTIST:
        return action_runtime_tag_scroll_available(NCM_SONG_GETTER_ARTIST);
    case NCM_ACTION_SCROLL_UP_ALBUM:
    case NCM_ACTION_SCROLL_DOWN_ALBUM:
        return action_runtime_tag_scroll_available(NCM_SONG_GETTER_ALBUM);
    case NCM_ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE:
        return app_screen_lyrics_is_current();
    case NCM_ACTION_SHOW_HELP:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_HELP)) {
            return false;
        }
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TINY_TAG_EDITOR)) {
            return false;
        }
#endif
        return true;
    case NCM_ACTION_SHOW_PLAYLIST:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
            return false;
        }
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TINY_TAG_EDITOR)) {
            return false;
        }
#endif
        return true;
    case NCM_ACTION_SHOW_PLAYLIST_EDITOR:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
            return false;
        }
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TINY_TAG_EDITOR)) {
            return false;
        }
#endif
        return true;
    case NCM_ACTION_SHOW_SERVER_INFO:
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TINY_TAG_EDITOR)) {
            return false;
        }
#endif
        return true;
    case NCM_ACTION_MOUSE_EVENT:
        return Config.mouse_support;
    case NCM_ACTION_SHOW_MEDIA_LIBRARY:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY)) {
            return false;
        }
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TINY_TAG_EDITOR)) {
            return false;
        }
#endif
        return true;
    case NCM_ACTION_JUMP_TO_PARENT_DIRECTORY:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
            return true;
        }
#if defined(HAVE_TAGLIB_H)
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_TAG_EDITOR);
#else
        return false;
#endif
    case NCM_ACTION_PREVIOUS_COLUMN:
        return action_runtime_previous_column_available();
    case NCM_ACTION_NEXT_COLUMN:
        return action_runtime_next_column_available();
    case NCM_ACTION_MASTER_SCREEN:
        return app_controller_can_show_locked_screen();
    case NCM_ACTION_SLAVE_SCREEN:
        return app_controller_can_show_inactive_screen();
    case NCM_ACTION_PAUSE:
        return ncm_status_state_player() != NCM_STATUS_PLAYER_STOP;
    case NCM_ACTION_PLAY:
    case NCM_ACTION_STOP:
    case NCM_ACTION_NEXT:
    case NCM_ACTION_PREVIOUS:
        return ncm_mpd_client_connected(&global_mpd);
    case NCM_ACTION_VOLUME_UP:
    case NCM_ACTION_VOLUME_DOWN:
        return ncm_mpd_client_connected(&global_mpd)
               && (ncm_status_state_volume() >= 0);
    case NCM_ACTION_ADD_ITEM_TO_PLAYLIST:
        if (!ncm_mpd_client_connected(&global_mpd)) {
            return false;
        }
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY)) {
            return media_library_screen_item_available(
                app_screen_media_library());
        }
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
            return action_runtime_playlist_editor_has_playlists();
        }
        return action_runtime_has_selected_songs();
    case NCM_ACTION_PLAY_ITEM:
        if (!ncm_mpd_client_connected(&global_mpd)) {
            return false;
        }
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
            return action_runtime_has_current_song();
        }
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY)) {
            return media_library_screen_item_available(
                app_screen_media_library());
        }
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
            return action_runtime_playlist_editor_has_playlists();
        }
        return action_runtime_has_selected_songs();
    case NCM_ACTION_DELETE_PLAYLIST_ITEMS:
        if (!ncm_mpd_client_connected(&global_mpd)) {
            return false;
        }
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
            return action_runtime_has_selected_songs();
        }
        return action_runtime_playlist_editor_content_active()
               && action_runtime_playlist_editor_has_content();
    case NCM_ACTION_DELETE_STORED_PLAYLIST:
        return ncm_mpd_client_connected(&global_mpd)
               && action_runtime_playlist_editor_playlists_active()
               && action_runtime_playlist_editor_has_playlists();
    case NCM_ACTION_REPLAY_SONG:
        return ncm_mpd_client_connected(&global_mpd)
               && (ncm_status_state_current_song_position() >= 0);
    case NCM_ACTION_RUN_ACTION:
        return nc_screen_can_run_current(app_controller_current_screen());
    case NCM_ACTION_MOVE_SORT_ORDER_UP:
    case NCM_ACTION_MOVE_SORT_ORDER_DOWN:
        return action_runtime_current_screen_is(
            NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG);
    case NCM_ACTION_MOVE_SELECTED_ITEMS_UP:
    case NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN:
        if (!ncm_mpd_client_connected(&global_mpd)) {
            return false;
        }
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
            return action_runtime_has_selected_songs();
        }
        return action_runtime_playlist_editor_content_active()
               && action_runtime_playlist_editor_has_content();
    case NCM_ACTION_MOVE_SELECTED_ITEMS_TO:
        if (!ncm_mpd_client_connected(&global_mpd)) {
            return false;
        }
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
            return nc_menu_has_selected(
                playlist_screen_menu(app_screen_playlist()));
        }
        return action_runtime_playlist_editor_content_active()
               && nc_menu_has_selected(
                   nc_song_menu_base(playlist_editor_screen_content(
                       app_screen_playlist_editor())));
    case NCM_ACTION_ADD:
    case NCM_ACTION_LOAD:
        return true;
    case NCM_ACTION_SEEK_FORWARD:
    case NCM_ACTION_SEEK_BACKWARD:
        return ncm_mpd_client_connected(&global_mpd)
               && (ncm_status_state_player() != NCM_STATUS_PLAYER_STOP)
               && (ncm_status_state_total_time() > 0);
    case NCM_ACTION_TOGGLE_DISPLAY_MODE:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)
               || action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)
               || action_runtime_current_screen_is(
                   NCM_SCREEN_TYPE_PLAYLIST_EDITOR)
               || action_runtime_current_screen_is(
                   NCM_SCREEN_TYPE_SEARCH_ENGINE);
    case NCM_ACTION_TOGGLE_REPEAT:
    case NCM_ACTION_TOGGLE_RANDOM:
    case NCM_ACTION_TOGGLE_SINGLE:
    case NCM_ACTION_TOGGLE_CONSUME:
    case NCM_ACTION_TOGGLE_CROSSFADE:
    case NCM_ACTION_UPDATE_DATABASE:
    case NCM_ACTION_TOGGLE_REPLAY_GAIN_MODE:
    case NCM_ACTION_CLEAR_MAIN_PLAYLIST:
    case NCM_ACTION_SET_CROSSFADE:
    case NCM_ACTION_ADD_RANDOM_ITEMS:
        return ncm_mpd_client_connected(&global_mpd);
    case NCM_ACTION_SET_VOLUME:
        return ncm_mpd_client_connected(&global_mpd)
               && (ncm_status_state_volume() >= 0);
    case NCM_ACTION_SHUFFLE: {
        int32 first;
        int32 last;

        if (!ncm_mpd_client_connected(&global_mpd)
            || !action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
            return false;
        }
        return action_runtime_playlist_range(action_runtime_current_menu(),
                                             &first, &last);
    }
    case NCM_ACTION_JUMP_TO_PLAYING_SONG:
        return ncm_mpd_client_connected(&global_mpd)
               && (ncm_status_state_current_song_position() >= 0);
    case NCM_ACTION_SAVE_TAG_CHANGES:
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TAG_EDITOR)) {
            return tag_editor_screen_save_action_available(
                app_screen_tag_editor());
        }
        return action_runtime_current_screen_is(
            NCM_SCREEN_TYPE_TINY_TAG_EDITOR);
#else
        return false;
#endif
    case NCM_ACTION_ENTER_DIRECTORY:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)
#if defined(HAVE_TAGLIB_H)
               || action_runtime_current_screen_is(NCM_SCREEN_TYPE_TAG_EDITOR)
#endif
            ;
    case NCM_ACTION_EDIT_SONG:
#if defined(HAVE_TAGLIB_H)
        return !action_runtime_current_screen_is(NCM_SCREEN_TYPE_LYRICS)
               && (Config.mpd_music_dir_len > 0)
               && action_runtime_has_current_song();
#else
        return false;
#endif
    case NCM_ACTION_JUMP_TO_BROWSER:
        return action_runtime_has_current_song();
    case NCM_ACTION_JUMP_TO_PLAYLIST_EDITOR:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
            NcmMpdItem *item;

            item = browser_screen_current_item(app_screen_browser());
            return item
                   && (ncm_mpd_item_kind(item) == NCM_MPD_ITEM_PLAYLIST);
        }
        return true;
    case NCM_ACTION_JUMP_TO_MEDIA_LIBRARY:
        return action_runtime_has_current_song();
    case NCM_ACTION_JUMP_TO_TAG_EDITOR:
#if defined(HAVE_TAGLIB_H)
        return (Config.mpd_music_dir_len > 0)
               && action_runtime_has_current_song();
#else
        return false;
#endif
    case NCM_ACTION_SELECT_ITEM:
        return action_runtime_menu_has_selectable_item();
    case NCM_ACTION_SELECT_RANGE:
        return action_runtime_menu_has_selection();
    case NCM_ACTION_REVERSE_SELECTION:
    case NCM_ACTION_REMOVE_SELECTION:
        if (action_runtime_current_menu() == NULL) {
            return false;
        }
        return true;
    case NCM_ACTION_ADD_SELECTED_ITEMS:
        return action_runtime_has_selected_songs();
    case NCM_ACTION_CROP_MAIN_PLAYLIST:
        if (!ncm_mpd_client_connected(&global_mpd)
            || !action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
            return false;
        }
        return (playlist_screen_song_count(app_screen_playlist())
                > 1)
               && action_runtime_has_selected_songs();
    case NCM_ACTION_CROP_PLAYLIST:
    case NCM_ACTION_CLEAR_PLAYLIST:
        return ncm_mpd_client_connected(&global_mpd)
               && action_runtime_current_screen_is(
                   NCM_SCREEN_TYPE_PLAYLIST_EDITOR)
               && action_runtime_playlist_editor_has_playlists();
    case NCM_ACTION_SORT_PLAYLIST:
        return ncm_mpd_client_connected(&global_mpd)
               && action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)
               && playlist_screen_has_sortable_range(
                   app_screen_playlist());
    case NCM_ACTION_REVERSE_PLAYLIST: {
        NcMenu *menu;
        int32 first;
        int32 last;

        if (!ncm_mpd_client_connected(&global_mpd)
            || !action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
            return false;
        }
        if (((menu = action_runtime_current_menu()) == NULL)
            || nc_menu_empty(menu)) {
            return false;
        }
        return ncm_menu_find_full_selected_range(
            menu, action_runtime_menu_item_source(menu), &first, &last);
    }
    case NCM_ACTION_TOGGLE_BROWSER_SORT_MODE:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER);
    case NCM_ACTION_TOGGLE_LIBRARY_TAG_TYPE:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY)) {
            MediaLibraryScreen *library = app_screen_media_library();
            enum MediaLibraryColumn column;

            column = media_library_screen_active_column(library);
            return (column == MEDIA_LIBRARY_COLUMN_TAGS)
                   || ((media_library_screen_column_count(library) == 2)
                       && (column == MEDIA_LIBRARY_COLUMN_ALBUMS));
        }
        return false;
    case NCM_ACTION_TOGGLE_MEDIA_LIBRARY_SORT_MODE:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY);
    case NCM_ACTION_FETCH_LYRICS_IN_BACKGROUND:
        return action_runtime_has_selected_songs();
    case NCM_ACTION_EDIT_LYRICS:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_LYRICS);
    case NCM_ACTION_REFETCH_LYRICS:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_LYRICS);
    case NCM_ACTION_SHOW_ARTIST_INFO:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_LASTFM)) {
            return true;
        }
        if (action_runtime_media_library_current_artist_tag(NULL, NULL)) {
            return true;
        }
        return action_runtime_has_current_song();
    case NCM_ACTION_SHOW_LYRICS:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_LYRICS)
               || action_runtime_has_current_song();
    case NCM_ACTION_SHOW_OUTPUTS:
#if defined(ENABLE_OUTPUTS)
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_OUTPUTS)) {
            return false;
        }
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TINY_TAG_EDITOR)) {
            return false;
        }
#endif
        return true;
#else
        return false;
#endif
    case NCM_ACTION_TOGGLE_OUTPUT:
#if defined(ENABLE_OUTPUTS)
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_OUTPUTS);
#else
        return false;
#endif
    case NCM_ACTION_SHOW_VISUALIZER:
#if defined(ENABLE_VISUALIZER)
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_VISUALIZER)) {
            return false;
        }
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TINY_TAG_EDITOR)) {
            return false;
        }
#endif
        return true;
#else
        return false;
#endif
    case NCM_ACTION_TOGGLE_VISUALIZATION_TYPE:
#if defined(ENABLE_VISUALIZER)
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_VISUALIZER);
#else
        return false;
#endif
    case NCM_ACTION_SHOW_TAG_EDITOR:
#if defined(HAVE_TAGLIB_H)
        return true;
#else
        return false;
#endif
    case NCM_ACTION_SHOW_BROWSER:
        return !action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER);
    case NCM_ACTION_CHANGE_BROWSE_MODE:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER);
    case NCM_ACTION_RESET_SEARCH_ENGINE:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_SEARCH_ENGINE);
    case NCM_ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY);
    case NCM_ACTION_EXECUTE_COMMAND:
        return true;
    case NCM_ACTION_APPLY_FILTER:
        return current_screen_allows_filter();
    case NCM_ACTION_FIND:
        return app_screen_help_is_current()
               || app_screen_lastfm_is_current()
               || app_screen_lyrics_is_current();
    case NCM_ACTION_FIND_ITEM_FORWARD:
    case NCM_ACTION_FIND_ITEM_BACKWARD:
    case NCM_ACTION_NEXT_FOUND_ITEM:
    case NCM_ACTION_PREVIOUS_FOUND_ITEM:
        return current_screen_allows_search();
    case NCM_ACTION_TOGGLE_FIND_MODE:
        return true;
    case NCM_ACTION_START_SEARCHING:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_SEARCH_ENGINE)
               && !search_engine_screen_constraints_locked(
                   app_screen_search_engine());
    case NCM_ACTION_SAVE_PLAYLIST:
        return ncm_mpd_client_connected(&global_mpd);
    case NCM_ACTION_JUMP_TO_POSITION_IN_SONG:
        return ncm_mpd_client_connected(&global_mpd)
               && (ncm_status_state_player() != NCM_STATUS_PLAYER_STOP)
               && (ncm_status_state_total_time() > 0)
               && (ncm_status_state_current_song_position() >= 0);
    case NCM_ACTION_SELECT_FOUND_ITEMS: {
        NcmStringView constraint;

        if (!current_screen_allows_search()) {
            return false;
        }
        constraint = current_screen_current_search_constraint();
        return action_runtime_menu_has_items() && constraint.data
               && (constraint.len > 0);
    }
    case NCM_ACTION_SELECT_ALBUM:
        return action_runtime_tag_scroll_available(NCM_SONG_GETTER_ALBUM);
    case NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY:
        if (!ncm_mpd_client_connected(&global_mpd)
            || !action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)
            || !action_runtime_has_selected_songs()) {
            return false;
        }
        if (ncm_mpd_client_version(&global_mpd) < 17) {
            ncm_statusbar_print_cstring(
                Config.message_delay_time,
                "Priorities are supported in MPD >= 0.17.0");
            return false;
        }
        return true;
    case NCM_ACTION_EDIT_PLAYLIST_NAME:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
            return ncm_mpd_client_connected(&global_mpd)
                   && browser_screen_rename_playlist_available(
                       app_screen_browser());
        }
        return ncm_mpd_client_connected(&global_mpd)
               && action_runtime_playlist_editor_playlists_active()
               && action_runtime_playlist_editor_has_playlists();
    case NCM_ACTION_EDIT_DIRECTORY_NAME:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
            return browser_screen_rename_directory_available(
                app_screen_browser());
        }
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TAG_EDITOR)) {
            return tag_editor_screen_rename_directory_available(
                app_screen_tag_editor(), Config.mpd_music_dir,
                Config.mpd_music_dir_len);
        }
#endif
        return false;
    case NCM_ACTION_DELETE_BROWSER_ITEMS:
        if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
            return false;
        }
        if (!Config.allow_for_physical_item_deletion) {
            return false;
        }
        if (!browser_screen_is_local(app_screen_browser())
            && (Config.mpd_music_dir_len <= 0)) {
            return false;
        }
        return action_runtime_menu_has_items();
    case NCM_ACTION_EDIT_LIBRARY_TAG:
#if defined(HAVE_TAGLIB_H)
        return action_runtime_can_edit_library_tag();
#else
        return false;
#endif
    case NCM_ACTION_EDIT_LIBRARY_ALBUM:
#if defined(HAVE_TAGLIB_H)
        return action_runtime_can_edit_library_album();
#else
        return false;
#endif
    case NCM_ACTION_MACRO_UTILITY:
    case NCM_ACTION_LAST:
    default:
        return false;
    }
}

static bool
action_runtime_builtin_run(NcmActionRuntime *runtime, enum NcmActionType type) {
    switch (type) {
    case NCM_ACTION_DUMMY:
        return true;
    case NCM_ACTION_UPDATE_ENVIRONMENT:
        return ncmpcpp_update_environment(true, true, true);
    case NCM_ACTION_MOUSE_EVENT:
        return action_runtime_mouse_event();
    case NCM_ACTION_SCROLL_UP:
        app_controller_scroll_current_screen(NC_SCROLL_UP);
        return true;
    case NCM_ACTION_SCROLL_UP_ARTIST:
        return action_runtime_scroll_by_tag(NCM_SONG_GETTER_ARTIST, false);
    case NCM_ACTION_SCROLL_UP_ALBUM:
        return action_runtime_scroll_by_tag(NCM_SONG_GETTER_ALBUM, false);
    case NCM_ACTION_SCROLL_DOWN:
        app_controller_scroll_current_screen(NC_SCROLL_DOWN);
        return true;
    case NCM_ACTION_SCROLL_DOWN_ARTIST:
        return action_runtime_scroll_by_tag(NCM_SONG_GETTER_ARTIST, true);
    case NCM_ACTION_SCROLL_DOWN_ALBUM:
        return action_runtime_scroll_by_tag(NCM_SONG_GETTER_ALBUM, true);
    case NCM_ACTION_PAGE_UP:
        app_controller_scroll_current_screen(NC_SCROLL_PAGE_UP);
        return true;
    case NCM_ACTION_PAGE_DOWN:
        app_controller_scroll_current_screen(NC_SCROLL_PAGE_DOWN);
        return true;
    case NCM_ACTION_MOVE_HOME:
        app_controller_scroll_current_screen(NC_SCROLL_HOME);
        return true;
    case NCM_ACTION_MOVE_END:
        app_controller_scroll_current_screen(NC_SCROLL_END);
        return true;
    case NCM_ACTION_TOGGLE_INTERFACE:
        return action_runtime_toggle_interface();
    case NCM_ACTION_JUMP_TO_PARENT_DIRECTORY:
        return action_runtime_jump_to_parent_directory();
    case NCM_ACTION_PREVIOUS_COLUMN:
        return action_runtime_previous_column();
    case NCM_ACTION_NEXT_COLUMN:
        return action_runtime_next_column();
    case NCM_ACTION_MASTER_SCREEN:
        if (!app_controller_show_locked_screen()) {
            return false;
        }
        ncm_title_draw_current_header();
        return true;
    case NCM_ACTION_SLAVE_SCREEN:
        if (!app_controller_show_inactive_screen()) {
            return false;
        }
        ncm_title_draw_current_header();
        return true;
    case NCM_ACTION_PLAY:
        return action_runtime_mpd_simple(ncm_mpd_client_play);
    case NCM_ACTION_PAUSE:
        return action_runtime_mpd_simple(ncm_mpd_client_toggle_pause);
    case NCM_ACTION_STOP:
        return action_runtime_mpd_simple(ncm_mpd_client_stop);
    case NCM_ACTION_NEXT:
        return action_runtime_mpd_simple(ncm_mpd_client_next);
    case NCM_ACTION_PREVIOUS:
        return action_runtime_mpd_simple(ncm_mpd_client_previous);
    case NCM_ACTION_REPLAY_SONG:
        return action_runtime_replay_song();
    case NCM_ACTION_VOLUME_UP:
        return action_runtime_volume(Config.volume_change_step);
    case NCM_ACTION_VOLUME_DOWN:
        return action_runtime_volume(-Config.volume_change_step);
    case NCM_ACTION_ADD_ITEM_TO_PLAYLIST:
        if (!action_runtime_add_item_to_playlist(false)) {
            return false;
        }
        app_controller_scroll_current_screen(NC_SCROLL_DOWN);
        nc_screen_finish_list_change(app_controller_current_screen());
        return true;
    case NCM_ACTION_PLAY_ITEM:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
            return nc_playlist_screen_activate_current(
                playlist_screen_playlist(app_screen_playlist()));
        }
        if (!action_runtime_add_item_to_playlist(true)) {
            return false;
        }
        nc_screen_finish_list_change(app_controller_current_screen());
        return true;
    case NCM_ACTION_DELETE_PLAYLIST_ITEMS:
        return action_runtime_delete_playlist_items();
    case NCM_ACTION_DELETE_STORED_PLAYLIST:
        return action_runtime_delete_stored_playlists();
    case NCM_ACTION_RUN_ACTION:
        return nc_screen_run_current(app_controller_current_screen());
    case NCM_ACTION_MOVE_SORT_ORDER_UP:
        return sort_playlist_dialog_move_current_up(
            app_screen_sort_playlist_dialog());
    case NCM_ACTION_MOVE_SORT_ORDER_DOWN:
        return sort_playlist_dialog_move_current_down(
            app_screen_sort_playlist_dialog());
    case NCM_ACTION_MOVE_SELECTED_ITEMS_UP:
        return action_runtime_move_selected_items(false);
    case NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN:
        return action_runtime_move_selected_items(true);
    case NCM_ACTION_MOVE_SELECTED_ITEMS_TO:
        return action_runtime_move_selected_items_to();
    case NCM_ACTION_ADD:
        if (action_runtime_current_screen_is(
            NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER)) {
            return selected_items_adder_screen_run_current(
                app_screen_selected_items_adder());
        }
        return action_runtime_add_prompt();
    case NCM_ACTION_LOAD:
        return action_runtime_load_prompt();
    case NCM_ACTION_SEEK_FORWARD:
        return action_runtime_seek_relative(true);
    case NCM_ACTION_SEEK_BACKWARD:
        return action_runtime_seek_relative(false);
    case NCM_ACTION_TOGGLE_DISPLAY_MODE:
        return action_runtime_toggle_display_mode();
    case NCM_ACTION_TOGGLE_SEPARATORS_BETWEEN_ALBUMS:
        return action_runtime_toggle_separators_between_albums();
    case NCM_ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE:
        return action_runtime_toggle_lyrics_update_on_song_change();
    case NCM_ACTION_TOGGLE_LYRICS_FETCHER:
        return action_runtime_toggle_lyrics_fetcher();
    case NCM_ACTION_TOGGLE_FETCHING_LYRICS_IN_BACKGROUND:
        return action_runtime_toggle_fetching_lyrics_in_background();
    case NCM_ACTION_TOGGLE_PLAYING_SONG_CENTERING:
        Config.autocenter_mode = !Config.autocenter_mode;
        if (Config.autocenter_mode) {
            int32 position;

            position = ncm_status_state_current_song_position();
            if (position >= 0) {
                (void)playlist_screen_locate_position(
                    app_screen_playlist(), position);
            }
        }
        if (Config.autocenter_mode) {
            ncm_statusbar_print_cstring(Config.message_delay_time,
                                        "Centering playing song: on");
        } else {
            ncm_statusbar_print_cstring(Config.message_delay_time,
                                        "Centering playing song: off");
        }
        return true;
    case NCM_ACTION_UPDATE_DATABASE:
        return action_runtime_update_database();
    case NCM_ACTION_JUMP_TO_PLAYING_SONG:
        return action_runtime_jump_to_playing_song();
    case NCM_ACTION_TOGGLE_REPEAT:
        return action_runtime_mpd_toggle(ncm_mpd_client_set_repeat,
                                         ncm_status_state_repeat());
    case NCM_ACTION_SHUFFLE:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
            return action_runtime_shuffle_playlist();
        }
        return action_runtime_mpd_simple(ncm_mpd_client_shuffle);
    case NCM_ACTION_TOGGLE_RANDOM:
        return action_runtime_mpd_toggle(ncm_mpd_client_set_random,
                                         ncm_status_state_random());
    case NCM_ACTION_SAVE_TAG_CHANGES:
        return action_runtime_save_tag_changes();
    case NCM_ACTION_TOGGLE_SINGLE:
        return action_runtime_mpd_toggle(ncm_mpd_client_set_single,
                                         ncm_status_state_single());
    case NCM_ACTION_TOGGLE_CONSUME:
        return action_runtime_mpd_toggle(ncm_mpd_client_set_consume,
                                         ncm_status_state_consume());
    case NCM_ACTION_TOGGLE_CROSSFADE:
        return action_runtime_toggle_crossfade();
    case NCM_ACTION_ENTER_DIRECTORY:
        return action_runtime_enter_directory();
    case NCM_ACTION_EDIT_SONG:
        return action_runtime_edit_current_song();
    case NCM_ACTION_JUMP_TO_BROWSER:
        return action_runtime_jump_to_browser();
    case NCM_ACTION_JUMP_TO_MEDIA_LIBRARY:
        return action_runtime_jump_to_media_library();
    case NCM_ACTION_JUMP_TO_PLAYLIST_EDITOR:
        return action_runtime_jump_to_playlist_editor();
    case NCM_ACTION_TOGGLE_SCREEN_LOCK:
        return action_runtime_toggle_screen_lock();
    case NCM_ACTION_JUMP_TO_TAG_EDITOR:
        return action_runtime_jump_to_tag_editor();
    case NCM_ACTION_JUMP_TO_POSITION_IN_SONG:
        return action_runtime_jump_to_position_in_song();
    case NCM_ACTION_SELECT_ITEM:
        return nc_menu_toggle_current_selected(action_runtime_current_menu());
    case NCM_ACTION_SELECT_RANGE: {
        enum NcMenuItemSource source;
        NcMenu *menu;
        int32 first;
        int32 last;

        if ((menu = action_runtime_current_menu()) == NULL) {
            return false;
        }
        source = action_runtime_menu_item_source(menu);
        if (!ncm_menu_find_selected_range(menu, source, &first, &last)) {
            return false;
        }
        for (int32 i = first; i < last; i += 1) {
            (void)nc_menu_set_position_selected(menu, i, true);
        }
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Range selected");
        return true;
    }
    case NCM_ACTION_REVERSE_SELECTION: {
        NcMenu *menu;

        if ((menu = action_runtime_current_menu()) == NULL) {
            return false;
        }
        ncm_menu_reverse_selection(menu, action_runtime_menu_item_source(menu));
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Selection reversed");
        return true;
    }
    case NCM_ACTION_REMOVE_SELECTION:
        nc_menu_clear_selection(action_runtime_current_menu());
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Selection removed");
        return true;
    case NCM_ACTION_ADD_SELECTED_ITEMS: {
        NcmSongArray songs;
        NcmError ncm_error;
        bool success;

        songs = (NcmSongArray){0};
        if (!action_runtime_selected_songs(&songs)
            || (songs.len <= 0)) {
            ncm_song_array_destroy(&songs);
            return false;
        }

        ncm_error_clear(&ncm_error);
        success = app_screen_selected_items_adder_open(&songs, &ncm_error);
        ncm_song_array_destroy(&songs);
        if (!success) {
            return action_runtime_mpd_error(&ncm_error);
        }
        return true;
    }
    case NCM_ACTION_SELECT_FOUND_ITEMS:
        return action_runtime_select_found_items();
    case NCM_ACTION_SELECT_ALBUM:
        return action_runtime_select_album();
    case NCM_ACTION_CROP_MAIN_PLAYLIST:
        return action_runtime_crop_playlist(true);
    case NCM_ACTION_CROP_PLAYLIST:
        return action_runtime_crop_playlist(false);
    case NCM_ACTION_CLEAR_MAIN_PLAYLIST:
        return action_runtime_clear_playlist(true);
    case NCM_ACTION_CLEAR_PLAYLIST:
        return action_runtime_clear_playlist(false);
    case NCM_ACTION_SORT_PLAYLIST:
        return action_runtime_switch_to_screen(
            NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG);
    case NCM_ACTION_REVERSE_PLAYLIST:
        return action_runtime_reverse_playlist();
    case NCM_ACTION_TOGGLE_REPLAY_GAIN_MODE:
        return action_runtime_toggle_replay_gain_mode();
    case NCM_ACTION_TOGGLE_ADD_MODE:
        return action_runtime_toggle_add_mode();
    case NCM_ACTION_TOGGLE_MOUSE:
        return action_runtime_toggle_mouse();
    case NCM_ACTION_TOGGLE_BITRATE_VISIBILITY:
        return action_runtime_toggle_bitrate_visibility();
    case NCM_ACTION_TOGGLE_BROWSER_SORT_MODE:
        return action_runtime_toggle_browser_sort_mode();
    case NCM_ACTION_TOGGLE_LIBRARY_TAG_TYPE:
        return action_runtime_toggle_library_tag_type();
    case NCM_ACTION_TOGGLE_MEDIA_LIBRARY_SORT_MODE:
        return action_runtime_toggle_media_library_sort_mode();
    case NCM_ACTION_FETCH_LYRICS_IN_BACKGROUND:
        return action_runtime_fetch_lyrics_background();
    case NCM_ACTION_EDIT_LYRICS:
        return action_runtime_edit_lyrics();
    case NCM_ACTION_REFETCH_LYRICS:
        return action_runtime_refetch_lyrics();
    case NCM_ACTION_SHOW_ARTIST_INFO:
        return action_runtime_show_artist_info();
    case NCM_ACTION_SHOW_LYRICS:
        return action_runtime_show_lyrics();
    case NCM_ACTION_QUIT:
        runtime->exit_requested = true;
        return true;
    case NCM_ACTION_NEXT_SCREEN:
        return action_runtime_switch_to_next_screen(false);
    case NCM_ACTION_PREVIOUS_SCREEN:
        return action_runtime_switch_to_next_screen(true);
    case NCM_ACTION_SHOW_HELP:
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_HELP);
    case NCM_ACTION_SHOW_PLAYLIST:
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_PLAYLIST);
    case NCM_ACTION_SHOW_BROWSER:
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_BROWSER);
    case NCM_ACTION_CHANGE_BROWSE_MODE:
        return action_runtime_change_browse_mode();
    case NCM_ACTION_SHOW_SEARCH_ENGINE:
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_SEARCH_ENGINE);
    case NCM_ACTION_RESET_SEARCH_ENGINE:
        search_engine_screen_reset(app_screen_search_engine());
        return true;
    case NCM_ACTION_SHOW_MEDIA_LIBRARY:
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_MEDIA_LIBRARY);
    case NCM_ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE:
        return action_runtime_toggle_media_library_columns();
    case NCM_ACTION_SHOW_PLAYLIST_EDITOR:
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_PLAYLIST_EDITOR);
    case NCM_ACTION_SHOW_SERVER_INFO:
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_SERVER_INFO);
    case NCM_ACTION_SHOW_SONG_INFO:
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_SONG_INFO);
    case NCM_ACTION_SHOW_OUTPUTS:
#if defined(ENABLE_OUTPUTS)
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_OUTPUTS);
#else
        return false;
#endif
    case NCM_ACTION_TOGGLE_OUTPUT:
#if defined(ENABLE_OUTPUTS)
        app_screen_outputs_toggle();
        return true;
#else
        return false;
#endif
    case NCM_ACTION_SHOW_VISUALIZER:
#if defined(ENABLE_VISUALIZER)
        return ncm_action_show_visualizer();
#else
        return false;
#endif
    case NCM_ACTION_TOGGLE_VISUALIZATION_TYPE:
#if defined(ENABLE_VISUALIZER)
        return ncm_action_toggle_visualization_type();
#else
        return false;
#endif
    case NCM_ACTION_SHOW_TAG_EDITOR:
#if defined(HAVE_TAGLIB_H)
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_TAG_EDITOR);
#else
        return false;
#endif
    case NCM_ACTION_EXECUTE_COMMAND:
        return action_runtime_execute_command();
    case NCM_ACTION_SAVE_PLAYLIST:
        return action_runtime_save_playlist();
    case NCM_ACTION_APPLY_FILTER:
        return action_runtime_apply_filter();
    case NCM_ACTION_FIND:
        return action_runtime_find();
    case NCM_ACTION_FIND_ITEM_FORWARD:
        return action_runtime_find_item(NCM_SEARCH_DIRECTION_FORWARD);
    case NCM_ACTION_FIND_ITEM_BACKWARD:
        return action_runtime_find_item(NCM_SEARCH_DIRECTION_BACKWARD);
    case NCM_ACTION_NEXT_FOUND_ITEM:
        return action_runtime_repeat_search(NCM_SEARCH_DIRECTION_FORWARD);
    case NCM_ACTION_PREVIOUS_FOUND_ITEM:
        return action_runtime_repeat_search(NCM_SEARCH_DIRECTION_BACKWARD);
    case NCM_ACTION_TOGGLE_FIND_MODE:
        Config.wrapped_search = !Config.wrapped_search;
        if (Config.wrapped_search) {
            ncm_statusbar_print_cstring(Config.message_delay_time,
                                        "Search mode: Wrapped");
        } else {
            ncm_statusbar_print_cstring(Config.message_delay_time,
                                        "Search mode: Normal");
        }
        return true;
    case NCM_ACTION_START_SEARCHING: {
        NcmError ncm_error;

        ncm_error_clear(&ncm_error);
        return search_engine_screen_start_searching(
            app_screen_search_engine(), &global_mpd, &ncm_error);
    }
    case NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY:
        return action_runtime_set_selected_items_priority();
    case NCM_ACTION_SET_CROSSFADE:
        return action_runtime_set_crossfade();
    case NCM_ACTION_SET_VOLUME:
        return action_runtime_set_volume();
    case NCM_ACTION_ADD_RANDOM_ITEMS:
        return action_runtime_add_random_items();
    case NCM_ACTION_EDIT_PLAYLIST_NAME:
        return action_runtime_edit_playlist_name();
    case NCM_ACTION_EDIT_DIRECTORY_NAME:
        return action_runtime_edit_directory_name();
    case NCM_ACTION_DELETE_BROWSER_ITEMS:
        return action_runtime_delete_browser_items();
    case NCM_ACTION_EDIT_LIBRARY_TAG:
        return action_runtime_edit_library_tag();
    case NCM_ACTION_EDIT_LIBRARY_ALBUM:
        return action_runtime_edit_library_album();
    case NCM_ACTION_MACRO_UTILITY:
    case NCM_ACTION_LAST:
    default:
        return false;
    }
}

#endif /* NCMPCPP_ACTIONS_C */
