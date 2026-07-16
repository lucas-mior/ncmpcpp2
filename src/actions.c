#include "actions.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "app_controller.h"
#include "app_legacy_bridge.h"
#include "bindings.h"
#include "curses/nc_window.h"
#include "global.h"
#include "helpers.h"
#include "settings.h"
#include "screens/native_c_screens.h"
#include "screens/nc_search_engine.h"
#include "screens/screen_type.h"
#include "screen_actions.h"
#include "status.h"
#include "statusbar.h"
#include "title.h"
#include "ui_state.h"

#include "c/ncm_base.h"
#include "c/ncm_conversion.h"
#include "c/ncm_macro_utilities.h"
#include "c/ncm_path.h"
#include "c/ncm_string.h"
#include "c/ncm_type_conversions.h"
#include "c/ncm_utf8.h"
#include "cbase/base_macros.h"
#include "cbase/cbase.h"

#if defined(__GNUC__)
extern bool ncmpcpp_legacy_execute_binding(NcmBinding *binding)
    __attribute__((weak));
#endif

static void action_runtime_mpd_noidle_callback(int32 flags, void *user);
static int32 ncm_action_name_len(char *name);
static bool ncm_action_name_equals(char *left, int32 left_len,
                                   char *right);

#define NCM_ACTION_TABLE_CALLBACKS(SUFFIX, TYPE)                         \
    static bool                                                          \
    ncm_action_can_run_##SUFFIX(void *user) {                            \
        (void)user;                                                       \
        return ncm_action_runtime_can_run(NULL, TYPE);                    \
    }                                                                     \
                                                                          \
    static bool                                                          \
    ncm_action_run_##SUFFIX(void *user) {                                \
        (void)user;                                                       \
        return ncm_action_runtime_run(NULL, TYPE);                        \
    }

NCM_ACTION_TABLE_CALLBACKS(dummy, NCM_ACTION_DUMMY)
NCM_ACTION_TABLE_CALLBACKS(update_environment, NCM_ACTION_UPDATE_ENVIRONMENT)
NCM_ACTION_TABLE_CALLBACKS(mouse_event, NCM_ACTION_MOUSE_EVENT)
NCM_ACTION_TABLE_CALLBACKS(scroll_up, NCM_ACTION_SCROLL_UP)
NCM_ACTION_TABLE_CALLBACKS(scroll_down, NCM_ACTION_SCROLL_DOWN)
NCM_ACTION_TABLE_CALLBACKS(scroll_up_artist, NCM_ACTION_SCROLL_UP_ARTIST)
NCM_ACTION_TABLE_CALLBACKS(scroll_up_album, NCM_ACTION_SCROLL_UP_ALBUM)
NCM_ACTION_TABLE_CALLBACKS(scroll_down_artist, NCM_ACTION_SCROLL_DOWN_ARTIST)
NCM_ACTION_TABLE_CALLBACKS(scroll_down_album, NCM_ACTION_SCROLL_DOWN_ALBUM)
NCM_ACTION_TABLE_CALLBACKS(page_up, NCM_ACTION_PAGE_UP)
NCM_ACTION_TABLE_CALLBACKS(page_down, NCM_ACTION_PAGE_DOWN)
NCM_ACTION_TABLE_CALLBACKS(move_home, NCM_ACTION_MOVE_HOME)
NCM_ACTION_TABLE_CALLBACKS(move_end, NCM_ACTION_MOVE_END)
NCM_ACTION_TABLE_CALLBACKS(toggle_interface, NCM_ACTION_TOGGLE_INTERFACE)
NCM_ACTION_TABLE_CALLBACKS(
    jump_to_parent_directory,
    NCM_ACTION_JUMP_TO_PARENT_DIRECTORY)
NCM_ACTION_TABLE_CALLBACKS(run_action, NCM_ACTION_RUN_ACTION)
NCM_ACTION_TABLE_CALLBACKS(previous_column, NCM_ACTION_PREVIOUS_COLUMN)
NCM_ACTION_TABLE_CALLBACKS(next_column, NCM_ACTION_NEXT_COLUMN)
NCM_ACTION_TABLE_CALLBACKS(master_screen, NCM_ACTION_MASTER_SCREEN)
NCM_ACTION_TABLE_CALLBACKS(slave_screen, NCM_ACTION_SLAVE_SCREEN)
NCM_ACTION_TABLE_CALLBACKS(volume_up, NCM_ACTION_VOLUME_UP)
NCM_ACTION_TABLE_CALLBACKS(volume_down, NCM_ACTION_VOLUME_DOWN)
NCM_ACTION_TABLE_CALLBACKS(
    add_item_to_playlist,
    NCM_ACTION_ADD_ITEM_TO_PLAYLIST)
NCM_ACTION_TABLE_CALLBACKS(play_item, NCM_ACTION_PLAY_ITEM)
NCM_ACTION_TABLE_CALLBACKS(
    delete_playlist_items,
    NCM_ACTION_DELETE_PLAYLIST_ITEMS)
NCM_ACTION_TABLE_CALLBACKS(
    delete_stored_playlist,
    NCM_ACTION_DELETE_STORED_PLAYLIST)
NCM_ACTION_TABLE_CALLBACKS(
    delete_browser_items,
    NCM_ACTION_DELETE_BROWSER_ITEMS)
NCM_ACTION_TABLE_CALLBACKS(replay_song, NCM_ACTION_REPLAY_SONG)
NCM_ACTION_TABLE_CALLBACKS(previous, NCM_ACTION_PREVIOUS)
NCM_ACTION_TABLE_CALLBACKS(next, NCM_ACTION_NEXT)
NCM_ACTION_TABLE_CALLBACKS(pause, NCM_ACTION_PAUSE)
NCM_ACTION_TABLE_CALLBACKS(stop, NCM_ACTION_STOP)
NCM_ACTION_TABLE_CALLBACKS(play, NCM_ACTION_PLAY)
NCM_ACTION_TABLE_CALLBACKS(execute_command, NCM_ACTION_EXECUTE_COMMAND)
NCM_ACTION_TABLE_CALLBACKS(save_playlist, NCM_ACTION_SAVE_PLAYLIST)
NCM_ACTION_TABLE_CALLBACKS(move_sort_order_up, NCM_ACTION_MOVE_SORT_ORDER_UP)
NCM_ACTION_TABLE_CALLBACKS(
    move_sort_order_down,
    NCM_ACTION_MOVE_SORT_ORDER_DOWN)
NCM_ACTION_TABLE_CALLBACKS(
    move_selected_items_up,
    NCM_ACTION_MOVE_SELECTED_ITEMS_UP)
NCM_ACTION_TABLE_CALLBACKS(
    move_selected_items_down,
    NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN)
NCM_ACTION_TABLE_CALLBACKS(
    move_selected_items_to,
    NCM_ACTION_MOVE_SELECTED_ITEMS_TO)
NCM_ACTION_TABLE_CALLBACKS(add, NCM_ACTION_ADD)
NCM_ACTION_TABLE_CALLBACKS(load, NCM_ACTION_LOAD)
NCM_ACTION_TABLE_CALLBACKS(seek_forward, NCM_ACTION_SEEK_FORWARD)
NCM_ACTION_TABLE_CALLBACKS(seek_backward, NCM_ACTION_SEEK_BACKWARD)
NCM_ACTION_TABLE_CALLBACKS(toggle_display_mode, NCM_ACTION_TOGGLE_DISPLAY_MODE)
NCM_ACTION_TABLE_CALLBACKS(
    toggle_separators_between_albums,
    NCM_ACTION_TOGGLE_SEPARATORS_BETWEEN_ALBUMS)
NCM_ACTION_TABLE_CALLBACKS(
    toggle_lyrics_update_on_song_change,
    NCM_ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE)
NCM_ACTION_TABLE_CALLBACKS(
    toggle_lyrics_fetcher,
    NCM_ACTION_TOGGLE_LYRICS_FETCHER)
NCM_ACTION_TABLE_CALLBACKS(
    toggle_fetching_lyrics_in_background,
    NCM_ACTION_TOGGLE_FETCHING_LYRICS_IN_BACKGROUND)
NCM_ACTION_TABLE_CALLBACKS(
    toggle_playing_song_centering,
    NCM_ACTION_TOGGLE_PLAYING_SONG_CENTERING)
NCM_ACTION_TABLE_CALLBACKS(update_database, NCM_ACTION_UPDATE_DATABASE)
NCM_ACTION_TABLE_CALLBACKS(
    jump_to_playing_song,
    NCM_ACTION_JUMP_TO_PLAYING_SONG)
NCM_ACTION_TABLE_CALLBACKS(toggle_repeat, NCM_ACTION_TOGGLE_REPEAT)
NCM_ACTION_TABLE_CALLBACKS(shuffle, NCM_ACTION_SHUFFLE)
NCM_ACTION_TABLE_CALLBACKS(toggle_random, NCM_ACTION_TOGGLE_RANDOM)
NCM_ACTION_TABLE_CALLBACKS(start_searching, NCM_ACTION_START_SEARCHING)
NCM_ACTION_TABLE_CALLBACKS(save_tag_changes, NCM_ACTION_SAVE_TAG_CHANGES)
NCM_ACTION_TABLE_CALLBACKS(toggle_single, NCM_ACTION_TOGGLE_SINGLE)
NCM_ACTION_TABLE_CALLBACKS(toggle_consume, NCM_ACTION_TOGGLE_CONSUME)
NCM_ACTION_TABLE_CALLBACKS(toggle_crossfade, NCM_ACTION_TOGGLE_CROSSFADE)
NCM_ACTION_TABLE_CALLBACKS(set_crossfade, NCM_ACTION_SET_CROSSFADE)
NCM_ACTION_TABLE_CALLBACKS(set_volume, NCM_ACTION_SET_VOLUME)
NCM_ACTION_TABLE_CALLBACKS(enter_directory, NCM_ACTION_ENTER_DIRECTORY)
NCM_ACTION_TABLE_CALLBACKS(edit_song, NCM_ACTION_EDIT_SONG)
NCM_ACTION_TABLE_CALLBACKS(edit_library_tag, NCM_ACTION_EDIT_LIBRARY_TAG)
NCM_ACTION_TABLE_CALLBACKS(edit_library_album, NCM_ACTION_EDIT_LIBRARY_ALBUM)
NCM_ACTION_TABLE_CALLBACKS(edit_directory_name, NCM_ACTION_EDIT_DIRECTORY_NAME)
NCM_ACTION_TABLE_CALLBACKS(edit_playlist_name, NCM_ACTION_EDIT_PLAYLIST_NAME)
NCM_ACTION_TABLE_CALLBACKS(edit_lyrics, NCM_ACTION_EDIT_LYRICS)
NCM_ACTION_TABLE_CALLBACKS(jump_to_browser, NCM_ACTION_JUMP_TO_BROWSER)
NCM_ACTION_TABLE_CALLBACKS(
    jump_to_media_library,
    NCM_ACTION_JUMP_TO_MEDIA_LIBRARY)
NCM_ACTION_TABLE_CALLBACKS(
    jump_to_playlist_editor,
    NCM_ACTION_JUMP_TO_PLAYLIST_EDITOR)
NCM_ACTION_TABLE_CALLBACKS(toggle_screen_lock, NCM_ACTION_TOGGLE_SCREEN_LOCK)
NCM_ACTION_TABLE_CALLBACKS(jump_to_tag_editor, NCM_ACTION_JUMP_TO_TAG_EDITOR)
NCM_ACTION_TABLE_CALLBACKS(
    jump_to_position_in_song,
    NCM_ACTION_JUMP_TO_POSITION_IN_SONG)
NCM_ACTION_TABLE_CALLBACKS(select_item, NCM_ACTION_SELECT_ITEM)
NCM_ACTION_TABLE_CALLBACKS(select_range, NCM_ACTION_SELECT_RANGE)
NCM_ACTION_TABLE_CALLBACKS(reverse_selection, NCM_ACTION_REVERSE_SELECTION)
NCM_ACTION_TABLE_CALLBACKS(remove_selection, NCM_ACTION_REMOVE_SELECTION)
NCM_ACTION_TABLE_CALLBACKS(select_album, NCM_ACTION_SELECT_ALBUM)
NCM_ACTION_TABLE_CALLBACKS(select_found_items, NCM_ACTION_SELECT_FOUND_ITEMS)
NCM_ACTION_TABLE_CALLBACKS(add_selected_items, NCM_ACTION_ADD_SELECTED_ITEMS)
NCM_ACTION_TABLE_CALLBACKS(crop_main_playlist, NCM_ACTION_CROP_MAIN_PLAYLIST)
NCM_ACTION_TABLE_CALLBACKS(crop_playlist, NCM_ACTION_CROP_PLAYLIST)
NCM_ACTION_TABLE_CALLBACKS(clear_main_playlist, NCM_ACTION_CLEAR_MAIN_PLAYLIST)
NCM_ACTION_TABLE_CALLBACKS(clear_playlist, NCM_ACTION_CLEAR_PLAYLIST)
NCM_ACTION_TABLE_CALLBACKS(sort_playlist, NCM_ACTION_SORT_PLAYLIST)
NCM_ACTION_TABLE_CALLBACKS(reverse_playlist, NCM_ACTION_REVERSE_PLAYLIST)
NCM_ACTION_TABLE_CALLBACKS(apply_filter, NCM_ACTION_APPLY_FILTER)
NCM_ACTION_TABLE_CALLBACKS(find, NCM_ACTION_FIND)
NCM_ACTION_TABLE_CALLBACKS(find_item_forward, NCM_ACTION_FIND_ITEM_FORWARD)
NCM_ACTION_TABLE_CALLBACKS(find_item_backward, NCM_ACTION_FIND_ITEM_BACKWARD)
NCM_ACTION_TABLE_CALLBACKS(next_found_item, NCM_ACTION_NEXT_FOUND_ITEM)
NCM_ACTION_TABLE_CALLBACKS(previous_found_item, NCM_ACTION_PREVIOUS_FOUND_ITEM)
NCM_ACTION_TABLE_CALLBACKS(toggle_find_mode, NCM_ACTION_TOGGLE_FIND_MODE)
NCM_ACTION_TABLE_CALLBACKS(
    toggle_replay_gain_mode,
    NCM_ACTION_TOGGLE_REPLAY_GAIN_MODE)
NCM_ACTION_TABLE_CALLBACKS(toggle_add_mode, NCM_ACTION_TOGGLE_ADD_MODE)
NCM_ACTION_TABLE_CALLBACKS(toggle_mouse, NCM_ACTION_TOGGLE_MOUSE)
NCM_ACTION_TABLE_CALLBACKS(
    toggle_bitrate_visibility,
    NCM_ACTION_TOGGLE_BITRATE_VISIBILITY)
NCM_ACTION_TABLE_CALLBACKS(add_random_items, NCM_ACTION_ADD_RANDOM_ITEMS)
NCM_ACTION_TABLE_CALLBACKS(
    toggle_browser_sort_mode,
    NCM_ACTION_TOGGLE_BROWSER_SORT_MODE)
NCM_ACTION_TABLE_CALLBACKS(
    toggle_library_tag_type,
    NCM_ACTION_TOGGLE_LIBRARY_TAG_TYPE)
NCM_ACTION_TABLE_CALLBACKS(
    toggle_media_library_sort_mode,
    NCM_ACTION_TOGGLE_MEDIA_LIBRARY_SORT_MODE)
NCM_ACTION_TABLE_CALLBACKS(
    fetch_lyrics_in_background,
    NCM_ACTION_FETCH_LYRICS_IN_BACKGROUND)
NCM_ACTION_TABLE_CALLBACKS(refetch_lyrics, NCM_ACTION_REFETCH_LYRICS)
NCM_ACTION_TABLE_CALLBACKS(
    set_selected_items_priority,
    NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY)
NCM_ACTION_TABLE_CALLBACKS(toggle_output, NCM_ACTION_TOGGLE_OUTPUT)
NCM_ACTION_TABLE_CALLBACKS(
    toggle_visualization_type,
    NCM_ACTION_TOGGLE_VISUALIZATION_TYPE)
NCM_ACTION_TABLE_CALLBACKS(show_song_info, NCM_ACTION_SHOW_SONG_INFO)
NCM_ACTION_TABLE_CALLBACKS(show_artist_info, NCM_ACTION_SHOW_ARTIST_INFO)
NCM_ACTION_TABLE_CALLBACKS(show_lyrics, NCM_ACTION_SHOW_LYRICS)
NCM_ACTION_TABLE_CALLBACKS(quit, NCM_ACTION_QUIT)
NCM_ACTION_TABLE_CALLBACKS(next_screen, NCM_ACTION_NEXT_SCREEN)
NCM_ACTION_TABLE_CALLBACKS(previous_screen, NCM_ACTION_PREVIOUS_SCREEN)
NCM_ACTION_TABLE_CALLBACKS(show_help, NCM_ACTION_SHOW_HELP)
NCM_ACTION_TABLE_CALLBACKS(show_playlist, NCM_ACTION_SHOW_PLAYLIST)
NCM_ACTION_TABLE_CALLBACKS(show_browser, NCM_ACTION_SHOW_BROWSER)
NCM_ACTION_TABLE_CALLBACKS(change_browse_mode, NCM_ACTION_CHANGE_BROWSE_MODE)
NCM_ACTION_TABLE_CALLBACKS(show_search_engine, NCM_ACTION_SHOW_SEARCH_ENGINE)
NCM_ACTION_TABLE_CALLBACKS(reset_search_engine, NCM_ACTION_RESET_SEARCH_ENGINE)
NCM_ACTION_TABLE_CALLBACKS(show_media_library, NCM_ACTION_SHOW_MEDIA_LIBRARY)
NCM_ACTION_TABLE_CALLBACKS(
    toggle_media_library_columns_mode,
    NCM_ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE)
NCM_ACTION_TABLE_CALLBACKS(
    show_playlist_editor,
    NCM_ACTION_SHOW_PLAYLIST_EDITOR)
NCM_ACTION_TABLE_CALLBACKS(show_tag_editor, NCM_ACTION_SHOW_TAG_EDITOR)
NCM_ACTION_TABLE_CALLBACKS(show_outputs, NCM_ACTION_SHOW_OUTPUTS)
NCM_ACTION_TABLE_CALLBACKS(show_visualizer, NCM_ACTION_SHOW_VISUALIZER)
NCM_ACTION_TABLE_CALLBACKS(show_server_info, NCM_ACTION_SHOW_SERVER_INFO)

static NcmActionDef action_defs[] = {
    {
        (char *)"dummy",
        NCM_ACTION_DUMMY,
        ncm_action_can_run_dummy,
        ncm_action_run_dummy,
    },
    {
        (char *)"update_environment",
        NCM_ACTION_UPDATE_ENVIRONMENT,
        ncm_action_can_run_update_environment,
        ncm_action_run_update_environment,
    },
    {
        (char *)"mouse_event",
        NCM_ACTION_MOUSE_EVENT,
        ncm_action_can_run_mouse_event,
        ncm_action_run_mouse_event,
    },
    {
        (char *)"scroll_up",
        NCM_ACTION_SCROLL_UP,
        ncm_action_can_run_scroll_up,
        ncm_action_run_scroll_up,
    },
    {
        (char *)"scroll_down",
        NCM_ACTION_SCROLL_DOWN,
        ncm_action_can_run_scroll_down,
        ncm_action_run_scroll_down,
    },
    {
        (char *)"scroll_up_artist",
        NCM_ACTION_SCROLL_UP_ARTIST,
        ncm_action_can_run_scroll_up_artist,
        ncm_action_run_scroll_up_artist,
    },
    {
        (char *)"scroll_up_album",
        NCM_ACTION_SCROLL_UP_ALBUM,
        ncm_action_can_run_scroll_up_album,
        ncm_action_run_scroll_up_album,
    },
    {
        (char *)"scroll_down_artist",
        NCM_ACTION_SCROLL_DOWN_ARTIST,
        ncm_action_can_run_scroll_down_artist,
        ncm_action_run_scroll_down_artist,
    },
    {
        (char *)"scroll_down_album",
        NCM_ACTION_SCROLL_DOWN_ALBUM,
        ncm_action_can_run_scroll_down_album,
        ncm_action_run_scroll_down_album,
    },
    {
        (char *)"page_up",
        NCM_ACTION_PAGE_UP,
        ncm_action_can_run_page_up,
        ncm_action_run_page_up,
    },
    {
        (char *)"page_down",
        NCM_ACTION_PAGE_DOWN,
        ncm_action_can_run_page_down,
        ncm_action_run_page_down,
    },
    {
        (char *)"move_home",
        NCM_ACTION_MOVE_HOME,
        ncm_action_can_run_move_home,
        ncm_action_run_move_home,
    },
    {
        (char *)"move_end",
        NCM_ACTION_MOVE_END,
        ncm_action_can_run_move_end,
        ncm_action_run_move_end,
    },
    {
        (char *)"toggle_interface",
        NCM_ACTION_TOGGLE_INTERFACE,
        ncm_action_can_run_toggle_interface,
        ncm_action_run_toggle_interface,
    },
    {
        (char *)"jump_to_parent_directory",
        NCM_ACTION_JUMP_TO_PARENT_DIRECTORY,
        ncm_action_can_run_jump_to_parent_directory,
        ncm_action_run_jump_to_parent_directory,
    },
    {
        (char *)"run_action",
        NCM_ACTION_RUN_ACTION,
        ncm_action_can_run_run_action,
        ncm_action_run_run_action,
    },
    {
        (char *)"previous_column",
        NCM_ACTION_PREVIOUS_COLUMN,
        ncm_action_can_run_previous_column,
        ncm_action_run_previous_column,
    },
    {
        (char *)"next_column",
        NCM_ACTION_NEXT_COLUMN,
        ncm_action_can_run_next_column,
        ncm_action_run_next_column,
    },
    {
        (char *)"master_screen",
        NCM_ACTION_MASTER_SCREEN,
        ncm_action_can_run_master_screen,
        ncm_action_run_master_screen,
    },
    {
        (char *)"slave_screen",
        NCM_ACTION_SLAVE_SCREEN,
        ncm_action_can_run_slave_screen,
        ncm_action_run_slave_screen,
    },
    {
        (char *)"volume_up",
        NCM_ACTION_VOLUME_UP,
        ncm_action_can_run_volume_up,
        ncm_action_run_volume_up,
    },
    {
        (char *)"volume_down",
        NCM_ACTION_VOLUME_DOWN,
        ncm_action_can_run_volume_down,
        ncm_action_run_volume_down,
    },
    {
        (char *)"add_item_to_playlist",
        NCM_ACTION_ADD_ITEM_TO_PLAYLIST,
        ncm_action_can_run_add_item_to_playlist,
        ncm_action_run_add_item_to_playlist,
    },
    {
        (char *)"play_item",
        NCM_ACTION_PLAY_ITEM,
        ncm_action_can_run_play_item,
        ncm_action_run_play_item,
    },
    {
        (char *)"delete_playlist_items",
        NCM_ACTION_DELETE_PLAYLIST_ITEMS,
        ncm_action_can_run_delete_playlist_items,
        ncm_action_run_delete_playlist_items,
    },
    {
        (char *)"delete_stored_playlist",
        NCM_ACTION_DELETE_STORED_PLAYLIST,
        ncm_action_can_run_delete_stored_playlist,
        ncm_action_run_delete_stored_playlist,
    },
    {
        (char *)"delete_browser_items",
        NCM_ACTION_DELETE_BROWSER_ITEMS,
        ncm_action_can_run_delete_browser_items,
        ncm_action_run_delete_browser_items,
    },
    {
        (char *)"replay_song",
        NCM_ACTION_REPLAY_SONG,
        ncm_action_can_run_replay_song,
        ncm_action_run_replay_song,
    },
    {
        (char *)"previous",
        NCM_ACTION_PREVIOUS,
        ncm_action_can_run_previous,
        ncm_action_run_previous,
    },
    {
        (char *)"next",
        NCM_ACTION_NEXT,
        ncm_action_can_run_next,
        ncm_action_run_next,
    },
    {
        (char *)"pause",
        NCM_ACTION_PAUSE,
        ncm_action_can_run_pause,
        ncm_action_run_pause,
    },
    {
        (char *)"stop",
        NCM_ACTION_STOP,
        ncm_action_can_run_stop,
        ncm_action_run_stop,
    },
    {
        (char *)"play",
        NCM_ACTION_PLAY,
        ncm_action_can_run_play,
        ncm_action_run_play,
    },
    {
        (char *)"execute_command",
        NCM_ACTION_EXECUTE_COMMAND,
        ncm_action_can_run_execute_command,
        ncm_action_run_execute_command,
    },
    {
        (char *)"save_playlist",
        NCM_ACTION_SAVE_PLAYLIST,
        ncm_action_can_run_save_playlist,
        ncm_action_run_save_playlist,
    },
    {
        (char *)"move_sort_order_up",
        NCM_ACTION_MOVE_SORT_ORDER_UP,
        ncm_action_can_run_move_sort_order_up,
        ncm_action_run_move_sort_order_up,
    },
    {
        (char *)"move_sort_order_down",
        NCM_ACTION_MOVE_SORT_ORDER_DOWN,
        ncm_action_can_run_move_sort_order_down,
        ncm_action_run_move_sort_order_down,
    },
    {
        (char *)"move_selected_items_up",
        NCM_ACTION_MOVE_SELECTED_ITEMS_UP,
        ncm_action_can_run_move_selected_items_up,
        ncm_action_run_move_selected_items_up,
    },
    {
        (char *)"move_selected_items_down",
        NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN,
        ncm_action_can_run_move_selected_items_down,
        ncm_action_run_move_selected_items_down,
    },
    {
        (char *)"move_selected_items_to",
        NCM_ACTION_MOVE_SELECTED_ITEMS_TO,
        ncm_action_can_run_move_selected_items_to,
        ncm_action_run_move_selected_items_to,
    },
    {
        (char *)"add",
        NCM_ACTION_ADD,
        ncm_action_can_run_add,
        ncm_action_run_add,
    },
    {
        (char *)"load",
        NCM_ACTION_LOAD,
        ncm_action_can_run_load,
        ncm_action_run_load,
    },
    {
        (char *)"seek_forward",
        NCM_ACTION_SEEK_FORWARD,
        ncm_action_can_run_seek_forward,
        ncm_action_run_seek_forward,
    },
    {
        (char *)"seek_backward",
        NCM_ACTION_SEEK_BACKWARD,
        ncm_action_can_run_seek_backward,
        ncm_action_run_seek_backward,
    },
    {
        (char *)"toggle_display_mode",
        NCM_ACTION_TOGGLE_DISPLAY_MODE,
        ncm_action_can_run_toggle_display_mode,
        ncm_action_run_toggle_display_mode,
    },
    {
        (char *)"toggle_separators_between_albums",
        NCM_ACTION_TOGGLE_SEPARATORS_BETWEEN_ALBUMS,
        ncm_action_can_run_toggle_separators_between_albums,
        ncm_action_run_toggle_separators_between_albums,
    },
    {
        (char *)"toggle_lyrics_update_on_song_change",
        NCM_ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE,
        ncm_action_can_run_toggle_lyrics_update_on_song_change,
        ncm_action_run_toggle_lyrics_update_on_song_change,
    },
    {
        (char *)"toggle_lyrics_fetcher",
        NCM_ACTION_TOGGLE_LYRICS_FETCHER,
        ncm_action_can_run_toggle_lyrics_fetcher,
        ncm_action_run_toggle_lyrics_fetcher,
    },
    {
        (char *)"toggle_fetching_lyrics_in_background",
        NCM_ACTION_TOGGLE_FETCHING_LYRICS_IN_BACKGROUND,
        ncm_action_can_run_toggle_fetching_lyrics_in_background,
        ncm_action_run_toggle_fetching_lyrics_in_background,
    },
    {
        (char *)"toggle_playing_song_centering",
        NCM_ACTION_TOGGLE_PLAYING_SONG_CENTERING,
        ncm_action_can_run_toggle_playing_song_centering,
        ncm_action_run_toggle_playing_song_centering,
    },
    {
        (char *)"update_database",
        NCM_ACTION_UPDATE_DATABASE,
        ncm_action_can_run_update_database,
        ncm_action_run_update_database,
    },
    {
        (char *)"jump_to_playing_song",
        NCM_ACTION_JUMP_TO_PLAYING_SONG,
        ncm_action_can_run_jump_to_playing_song,
        ncm_action_run_jump_to_playing_song,
    },
    {
        (char *)"toggle_repeat",
        NCM_ACTION_TOGGLE_REPEAT,
        ncm_action_can_run_toggle_repeat,
        ncm_action_run_toggle_repeat,
    },
    {
        (char *)"shuffle",
        NCM_ACTION_SHUFFLE,
        ncm_action_can_run_shuffle,
        ncm_action_run_shuffle,
    },
    {
        (char *)"toggle_random",
        NCM_ACTION_TOGGLE_RANDOM,
        ncm_action_can_run_toggle_random,
        ncm_action_run_toggle_random,
    },
    {
        (char *)"start_searching",
        NCM_ACTION_START_SEARCHING,
        ncm_action_can_run_start_searching,
        ncm_action_run_start_searching,
    },
    {
        (char *)"save_tag_changes",
        NCM_ACTION_SAVE_TAG_CHANGES,
        ncm_action_can_run_save_tag_changes,
        ncm_action_run_save_tag_changes,
    },
    {
        (char *)"toggle_single",
        NCM_ACTION_TOGGLE_SINGLE,
        ncm_action_can_run_toggle_single,
        ncm_action_run_toggle_single,
    },
    {
        (char *)"toggle_consume",
        NCM_ACTION_TOGGLE_CONSUME,
        ncm_action_can_run_toggle_consume,
        ncm_action_run_toggle_consume,
    },
    {
        (char *)"toggle_crossfade",
        NCM_ACTION_TOGGLE_CROSSFADE,
        ncm_action_can_run_toggle_crossfade,
        ncm_action_run_toggle_crossfade,
    },
    {
        (char *)"set_crossfade",
        NCM_ACTION_SET_CROSSFADE,
        ncm_action_can_run_set_crossfade,
        ncm_action_run_set_crossfade,
    },
    {
        (char *)"set_volume",
        NCM_ACTION_SET_VOLUME,
        ncm_action_can_run_set_volume,
        ncm_action_run_set_volume,
    },
    {
        (char *)"enter_directory",
        NCM_ACTION_ENTER_DIRECTORY,
        ncm_action_can_run_enter_directory,
        ncm_action_run_enter_directory,
    },
    {
        (char *)"edit_song",
        NCM_ACTION_EDIT_SONG,
        ncm_action_can_run_edit_song,
        ncm_action_run_edit_song,
    },
    {
        (char *)"edit_library_tag",
        NCM_ACTION_EDIT_LIBRARY_TAG,
        ncm_action_can_run_edit_library_tag,
        ncm_action_run_edit_library_tag,
    },
    {
        (char *)"edit_library_album",
        NCM_ACTION_EDIT_LIBRARY_ALBUM,
        ncm_action_can_run_edit_library_album,
        ncm_action_run_edit_library_album,
    },
    {
        (char *)"edit_directory_name",
        NCM_ACTION_EDIT_DIRECTORY_NAME,
        ncm_action_can_run_edit_directory_name,
        ncm_action_run_edit_directory_name,
    },
    {
        (char *)"edit_playlist_name",
        NCM_ACTION_EDIT_PLAYLIST_NAME,
        ncm_action_can_run_edit_playlist_name,
        ncm_action_run_edit_playlist_name,
    },
    {
        (char *)"edit_lyrics",
        NCM_ACTION_EDIT_LYRICS,
        ncm_action_can_run_edit_lyrics,
        ncm_action_run_edit_lyrics,
    },
    {
        (char *)"jump_to_browser",
        NCM_ACTION_JUMP_TO_BROWSER,
        ncm_action_can_run_jump_to_browser,
        ncm_action_run_jump_to_browser,
    },
    {
        (char *)"jump_to_media_library",
        NCM_ACTION_JUMP_TO_MEDIA_LIBRARY,
        ncm_action_can_run_jump_to_media_library,
        ncm_action_run_jump_to_media_library,
    },
    {
        (char *)"jump_to_playlist_editor",
        NCM_ACTION_JUMP_TO_PLAYLIST_EDITOR,
        ncm_action_can_run_jump_to_playlist_editor,
        ncm_action_run_jump_to_playlist_editor,
    },
    {
        (char *)"toggle_screen_lock",
        NCM_ACTION_TOGGLE_SCREEN_LOCK,
        ncm_action_can_run_toggle_screen_lock,
        ncm_action_run_toggle_screen_lock,
    },
    {
        (char *)"jump_to_tag_editor",
        NCM_ACTION_JUMP_TO_TAG_EDITOR,
        ncm_action_can_run_jump_to_tag_editor,
        ncm_action_run_jump_to_tag_editor,
    },
    {
        (char *)"jump_to_position_in_song",
        NCM_ACTION_JUMP_TO_POSITION_IN_SONG,
        ncm_action_can_run_jump_to_position_in_song,
        ncm_action_run_jump_to_position_in_song,
    },
    {
        (char *)"select_item",
        NCM_ACTION_SELECT_ITEM,
        ncm_action_can_run_select_item,
        ncm_action_run_select_item,
    },
    {
        (char *)"select_range",
        NCM_ACTION_SELECT_RANGE,
        ncm_action_can_run_select_range,
        ncm_action_run_select_range,
    },
    {
        (char *)"reverse_selection",
        NCM_ACTION_REVERSE_SELECTION,
        ncm_action_can_run_reverse_selection,
        ncm_action_run_reverse_selection,
    },
    {
        (char *)"remove_selection",
        NCM_ACTION_REMOVE_SELECTION,
        ncm_action_can_run_remove_selection,
        ncm_action_run_remove_selection,
    },
    {
        (char *)"select_album",
        NCM_ACTION_SELECT_ALBUM,
        ncm_action_can_run_select_album,
        ncm_action_run_select_album,
    },
    {
        (char *)"select_found_items",
        NCM_ACTION_SELECT_FOUND_ITEMS,
        ncm_action_can_run_select_found_items,
        ncm_action_run_select_found_items,
    },
    {
        (char *)"add_selected_items",
        NCM_ACTION_ADD_SELECTED_ITEMS,
        ncm_action_can_run_add_selected_items,
        ncm_action_run_add_selected_items,
    },
    {
        (char *)"crop_main_playlist",
        NCM_ACTION_CROP_MAIN_PLAYLIST,
        ncm_action_can_run_crop_main_playlist,
        ncm_action_run_crop_main_playlist,
    },
    {
        (char *)"crop_playlist",
        NCM_ACTION_CROP_PLAYLIST,
        ncm_action_can_run_crop_playlist,
        ncm_action_run_crop_playlist,
    },
    {
        (char *)"clear_main_playlist",
        NCM_ACTION_CLEAR_MAIN_PLAYLIST,
        ncm_action_can_run_clear_main_playlist,
        ncm_action_run_clear_main_playlist,
    },
    {
        (char *)"clear_playlist",
        NCM_ACTION_CLEAR_PLAYLIST,
        ncm_action_can_run_clear_playlist,
        ncm_action_run_clear_playlist,
    },
    {
        (char *)"sort_playlist",
        NCM_ACTION_SORT_PLAYLIST,
        ncm_action_can_run_sort_playlist,
        ncm_action_run_sort_playlist,
    },
    {
        (char *)"reverse_playlist",
        NCM_ACTION_REVERSE_PLAYLIST,
        ncm_action_can_run_reverse_playlist,
        ncm_action_run_reverse_playlist,
    },
    {
        (char *)"apply_filter",
        NCM_ACTION_APPLY_FILTER,
        ncm_action_can_run_apply_filter,
        ncm_action_run_apply_filter,
    },
    {
        (char *)"find",
        NCM_ACTION_FIND,
        ncm_action_can_run_find,
        ncm_action_run_find,
    },
    {
        (char *)"find_item_forward",
        NCM_ACTION_FIND_ITEM_FORWARD,
        ncm_action_can_run_find_item_forward,
        ncm_action_run_find_item_forward,
    },
    {
        (char *)"find_item_backward",
        NCM_ACTION_FIND_ITEM_BACKWARD,
        ncm_action_can_run_find_item_backward,
        ncm_action_run_find_item_backward,
    },
    {
        (char *)"next_found_item",
        NCM_ACTION_NEXT_FOUND_ITEM,
        ncm_action_can_run_next_found_item,
        ncm_action_run_next_found_item,
    },
    {
        (char *)"previous_found_item",
        NCM_ACTION_PREVIOUS_FOUND_ITEM,
        ncm_action_can_run_previous_found_item,
        ncm_action_run_previous_found_item,
    },
    {
        (char *)"toggle_find_mode",
        NCM_ACTION_TOGGLE_FIND_MODE,
        ncm_action_can_run_toggle_find_mode,
        ncm_action_run_toggle_find_mode,
    },
    {
        (char *)"toggle_replay_gain_mode",
        NCM_ACTION_TOGGLE_REPLAY_GAIN_MODE,
        ncm_action_can_run_toggle_replay_gain_mode,
        ncm_action_run_toggle_replay_gain_mode,
    },
    {
        (char *)"toggle_add_mode",
        NCM_ACTION_TOGGLE_ADD_MODE,
        ncm_action_can_run_toggle_add_mode,
        ncm_action_run_toggle_add_mode,
    },
    {
        (char *)"toggle_mouse",
        NCM_ACTION_TOGGLE_MOUSE,
        ncm_action_can_run_toggle_mouse,
        ncm_action_run_toggle_mouse,
    },
    {
        (char *)"toggle_bitrate_visibility",
        NCM_ACTION_TOGGLE_BITRATE_VISIBILITY,
        ncm_action_can_run_toggle_bitrate_visibility,
        ncm_action_run_toggle_bitrate_visibility,
    },
    {
        (char *)"add_random_items",
        NCM_ACTION_ADD_RANDOM_ITEMS,
        ncm_action_can_run_add_random_items,
        ncm_action_run_add_random_items,
    },
    {
        (char *)"toggle_browser_sort_mode",
        NCM_ACTION_TOGGLE_BROWSER_SORT_MODE,
        ncm_action_can_run_toggle_browser_sort_mode,
        ncm_action_run_toggle_browser_sort_mode,
    },
    {
        (char *)"toggle_library_tag_type",
        NCM_ACTION_TOGGLE_LIBRARY_TAG_TYPE,
        ncm_action_can_run_toggle_library_tag_type,
        ncm_action_run_toggle_library_tag_type,
    },
    {
        (char *)"toggle_media_library_sort_mode",
        NCM_ACTION_TOGGLE_MEDIA_LIBRARY_SORT_MODE,
        ncm_action_can_run_toggle_media_library_sort_mode,
        ncm_action_run_toggle_media_library_sort_mode,
    },
    {
        (char *)"fetch_lyrics_in_background",
        NCM_ACTION_FETCH_LYRICS_IN_BACKGROUND,
        ncm_action_can_run_fetch_lyrics_in_background,
        ncm_action_run_fetch_lyrics_in_background,
    },
    {
        (char *)"refetch_lyrics",
        NCM_ACTION_REFETCH_LYRICS,
        ncm_action_can_run_refetch_lyrics,
        ncm_action_run_refetch_lyrics,
    },
    {
        (char *)"set_selected_items_priority",
        NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY,
        ncm_action_can_run_set_selected_items_priority,
        ncm_action_run_set_selected_items_priority,
    },
    {
        (char *)"toggle_output",
        NCM_ACTION_TOGGLE_OUTPUT,
        ncm_action_can_run_toggle_output,
        ncm_action_run_toggle_output,
    },
    {
        (char *)"toggle_visualization_type",
        NCM_ACTION_TOGGLE_VISUALIZATION_TYPE,
        ncm_action_can_run_toggle_visualization_type,
        ncm_action_run_toggle_visualization_type,
    },
    {
        (char *)"show_song_info",
        NCM_ACTION_SHOW_SONG_INFO,
        ncm_action_can_run_show_song_info,
        ncm_action_run_show_song_info,
    },
    {
        (char *)"show_artist_info",
        NCM_ACTION_SHOW_ARTIST_INFO,
        ncm_action_can_run_show_artist_info,
        ncm_action_run_show_artist_info,
    },
    {
        (char *)"show_lyrics",
        NCM_ACTION_SHOW_LYRICS,
        ncm_action_can_run_show_lyrics,
        ncm_action_run_show_lyrics,
    },
    {
        (char *)"quit",
        NCM_ACTION_QUIT,
        ncm_action_can_run_quit,
        ncm_action_run_quit,
    },
    {
        (char *)"next_screen",
        NCM_ACTION_NEXT_SCREEN,
        ncm_action_can_run_next_screen,
        ncm_action_run_next_screen,
    },
    {
        (char *)"previous_screen",
        NCM_ACTION_PREVIOUS_SCREEN,
        ncm_action_can_run_previous_screen,
        ncm_action_run_previous_screen,
    },
    {
        (char *)"show_help",
        NCM_ACTION_SHOW_HELP,
        ncm_action_can_run_show_help,
        ncm_action_run_show_help,
    },
    {
        (char *)"show_playlist",
        NCM_ACTION_SHOW_PLAYLIST,
        ncm_action_can_run_show_playlist,
        ncm_action_run_show_playlist,
    },
    {
        (char *)"show_browser",
        NCM_ACTION_SHOW_BROWSER,
        ncm_action_can_run_show_browser,
        ncm_action_run_show_browser,
    },
    {
        (char *)"change_browse_mode",
        NCM_ACTION_CHANGE_BROWSE_MODE,
        ncm_action_can_run_change_browse_mode,
        ncm_action_run_change_browse_mode,
    },
    {
        (char *)"show_search_engine",
        NCM_ACTION_SHOW_SEARCH_ENGINE,
        ncm_action_can_run_show_search_engine,
        ncm_action_run_show_search_engine,
    },
    {
        (char *)"reset_search_engine",
        NCM_ACTION_RESET_SEARCH_ENGINE,
        ncm_action_can_run_reset_search_engine,
        ncm_action_run_reset_search_engine,
    },
    {
        (char *)"show_media_library",
        NCM_ACTION_SHOW_MEDIA_LIBRARY,
        ncm_action_can_run_show_media_library,
        ncm_action_run_show_media_library,
    },
    {
        (char *)"toggle_media_library_columns_mode",
        NCM_ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE,
        ncm_action_can_run_toggle_media_library_columns_mode,
        ncm_action_run_toggle_media_library_columns_mode,
    },
    {
        (char *)"show_playlist_editor",
        NCM_ACTION_SHOW_PLAYLIST_EDITOR,
        ncm_action_can_run_show_playlist_editor,
        ncm_action_run_show_playlist_editor,
    },
    {
        (char *)"show_tag_editor",
        NCM_ACTION_SHOW_TAG_EDITOR,
        ncm_action_can_run_show_tag_editor,
        ncm_action_run_show_tag_editor,
    },
    {
        (char *)"show_outputs",
        NCM_ACTION_SHOW_OUTPUTS,
        ncm_action_can_run_show_outputs,
        ncm_action_run_show_outputs,
    },
    {
        (char *)"show_visualizer",
        NCM_ACTION_SHOW_VISUALIZER,
        ncm_action_can_run_show_visualizer,
        ncm_action_run_show_visualizer,
    },
    {
        (char *)"show_server_info",
        NCM_ACTION_SHOW_SERVER_INFO,
        ncm_action_can_run_show_server_info,
        ncm_action_run_show_server_info,
    },
};


static int32
ncm_action_name_len(char *name) {
    int32 len;

    len = 0;
    while (name[len] != '\0') {
        len += 1;
    }
    return len;
}

static bool
ncm_action_name_equals(char *left, int32 left_len, char *right) {
    int32 right_len;

    right_len = ncm_action_name_len(right);
    if (left_len != right_len) {
        return false;
    }
    return cbase_memcmp(left, right, left_len) == 0;
}

static void
ncm_action_error(NcmError *error, char *message, int32 message_len) {
    ncm_error_set(error, 1, message, message_len);
    return;
}

int32
ncm_action_count(void) {
    return NCM_ARRAY_LEN(action_defs);
}

NcmActionDef *
ncm_action_table(void) {
    return action_defs;
}

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
        if ((defs[i].name != NULL)
            && ncm_action_name_equals(name, name_len, defs[i].name)) {
            return defs + i;
        }
    }
    return NULL;
}

bool
ncm_action_table_validate(NcmActionDef *defs, int32 defs_len,
                          NcmError *error) {
    if (defs == NULL) {
        ncm_action_error(error, STRLIT_ARGS("missing action table"));
        return false;
    }
    if (defs_len <= 0) {
        ncm_action_error(error, STRLIT_ARGS("empty action table"));
        return false;
    }

    for (int32 i = 0; i < defs_len; i += 1) {
        if (defs[i].name == NULL) {
            ncm_action_error(error, STRLIT_ARGS("action without name"));
            return false;
        }
        if ((defs[i].type < 0) || (defs[i].type >= NCM_ACTION_LAST)) {
            ncm_action_error(error, STRLIT_ARGS("invalid action type"));
            return false;
        }
        if (defs[i].can_run == NULL) {
            ncm_action_error(error, STRLIT_ARGS("action without can_run"));
            return false;
        }
        if (defs[i].run == NULL) {
            ncm_action_error(error, STRLIT_ARGS("action without run"));
            return false;
        }

        for (int32 j = i + 1; j < defs_len; j += 1) {
            if (defs[i].type == defs[j].type) {
                ncm_action_error(error,
                                 STRLIT_ARGS("duplicate action type"));
                return false;
            }
            if ((defs[j].name != NULL)
                && ncm_action_name_equals(defs[i].name,
                                          ncm_action_name_len(defs[i].name),
                                          defs[j].name)) {
                ncm_action_error(error,
                                 STRLIT_ARGS("duplicate action name"));
                return false;
            }
        }
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_action_validate(NcmError *error) {
    bool seen[NCM_ACTION_LAST] = {0};

    if (!ncm_action_table_validate(action_defs, NCM_ARRAY_LEN(action_defs),
                                   error)) {
        return false;
    }
    if (NCM_ARRAY_LEN(action_defs) != NCM_ACTION_LAST) {
        ncm_action_error(error, STRLIT_ARGS("incomplete action table"));
        return false;
    }

    for (int32 i = 0; i < NCM_ARRAY_LEN(action_defs); i += 1) {
        seen[action_defs[i].type] = true;
    }
    for (int32 i = 0; i < NCM_ACTION_LAST; i += 1) {
        if (!seen[i]) {
            ncm_action_error(error, STRLIT_ARGS("missing action type"));
            return false;
        }
    }

    ncm_error_clear(error);
    return true;
}

NcmActionDef *
ncm_action_get(enum NcmActionType type) {
    return ncm_action_table_get(action_defs, NCM_ARRAY_LEN(action_defs), type);
}

NcmActionDef *
ncm_action_find(char *name, int32 name_len) {
    return ncm_action_table_find(action_defs, NCM_ARRAY_LEN(action_defs), name,
                                 name_len);
}

char *
ncm_action_type_name(enum NcmActionType type) {
    NcmActionDef *action;

    action = ncm_action_get(type);
    if (action == NULL) {
        return (char *)"";
    }
    return action->name;
}

bool
ncm_action_type_parse(char *name, int32 name_len,
                      enum NcmActionType *type) {
    NcmActionDef *action;

    if (type == NULL) {
        return false;
    }

    action = ncm_action_find(name, name_len);
    if (action == NULL) {
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
ncm_action_set_callbacks(enum NcmActionType type,
                         NcmActionCanRunFn can_run,
                         NcmActionRunFn run) {
    NcmActionDef *action;

    action = ncm_action_get(type);
    if (action == NULL) {
        return false;
    }
    if (can_run != NULL) {
        action->can_run = can_run;
    }
    if (run != NULL) {
        action->run = run;
    }
    return true;
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
    NcmBuffer previous;
} ActionRuntimeCommandPrompt;

typedef struct ActionRuntimeSearchPrompt {
    enum SearchDirection direction;
} ActionRuntimeSearchPrompt;

static NcmActionRuntime *action_runtime_or_global(
    NcmActionRuntime *runtime);
static int32 action_runtime_call_hook(NcmActionRuntimeHook hook,
                                      enum NcmActionType type,
                                      void *user);
static bool action_runtime_hook_allowed(int32 result, bool *handled);
static bool action_runtime_hook_denied(int32 result, bool *handled);
static bool action_runtime_builtin_can_run(NcmActionRuntime *runtime,
                                           enum NcmActionType type);
static bool action_runtime_builtin_run(NcmActionRuntime *runtime,
                                       enum NcmActionType type);
static bool action_runtime_current_screen_is(enum ScreenType type);
static bool action_runtime_switch_to_screen(enum ScreenType type);
static bool action_runtime_switch_to_next_screen(bool reverse);
static bool action_runtime_mpd_error(NcmError *error);
static bool action_runtime_queue_find_song(NcmMpdSongList *queue,
                                           NcmSong *song,
                                           NcmSong **match);
static bool action_runtime_queue_remove_song(NcmMpdSongList *queue,
                                             NcmSong *song,
                                             NcmError *error);
static bool action_runtime_mpd_simple(
    bool (*func)(NcmMpdClient *client, NcmError *error));
static bool action_runtime_mpd_toggle(
    bool (*func)(NcmMpdClient *client, bool mode, NcmError *error),
    bool current);
static bool action_runtime_volume(int32 change);
static bool action_runtime_update_database(void);
static bool action_runtime_replay_song(void);
static bool action_runtime_update_environment(void);
static bool action_runtime_execute_command(void);
static bool action_runtime_execute_binding(NcmBinding *binding);
static bool action_runtime_apply_filter(void);
static bool action_runtime_find_item(enum SearchDirection direction);
static bool action_runtime_repeat_search(enum SearchDirection direction);
static bool action_runtime_command_prompt_hook(char *text, void *user);
static bool action_runtime_filter_prompt_hook(char *text, void *user);
static bool action_runtime_search_prompt_hook(char *text, void *user);
static bool action_runtime_prompt_result(NcmBuffer *result,
                                         NcPrompt *prompt,
                                         NcWindow *window);
static bool action_runtime_prompt_string(char *prefix, int32 prefix_len,
                                         char *initial_text,
                                         bool remember,
                                         NcPromptHook hook,
                                         void *hook_user,
                                         NcmBuffer *result);
static bool action_runtime_confirm(char *message, int32 message_len);
static bool action_runtime_parse_seek_position(char *text, int32 text_len,
                                               uint32 total,
                                               uint32 *position);
static int32 action_runtime_cstring_len(char *string);
static void action_runtime_print_format_string(char *format,
                                               int32 format_len,
                                               char *text,
                                               int32 text_len);
static bool action_runtime_toggle_crossfade(void);
static bool action_runtime_set_crossfade(void);
static bool action_runtime_set_volume(void);
static bool action_runtime_add_random_items(void);
static NcMenu *action_runtime_current_menu(void);
static enum NcMenuItemSource action_runtime_menu_item_source(
    NcMenu *menu);
static bool action_runtime_menu_has_items(void);
static bool action_runtime_selected_songs(NcmSongArray *songs);
static bool action_runtime_has_selected_songs(void);
static bool action_runtime_current_song(NcmSong *song);
static bool action_runtime_add_selected_songs(bool play);
static bool action_runtime_add_playlist_editor_item(bool play);
static bool action_runtime_add_raw_path_to_playlist_editor(void);
static bool action_runtime_delete_playlist_items(void);
static bool action_runtime_delete_browser_items(void);
static bool action_runtime_browser_item_name(NcmMpdItem *item,
                                            NcmBuffer *name);
static void action_runtime_print_renamed(char *prefix, int32 prefix_len,
                                         NcmBuffer *name);
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

void
ncm_action_runtime_init(NcmActionRuntime *runtime) {
    if (runtime == NULL) {
        return;
    }

    runtime->can_run_hook = NULL;
    runtime->run_hook = NULL;
    runtime->user = NULL;
    runtime->exit_requested = false;
    return;
}

void
ncm_action_runtime_set_hooks(NcmActionRuntime *runtime,
                             NcmActionRuntimeHook can_run_hook,
                             NcmActionRuntimeHook run_hook,
                             void *user) {
    if (runtime == NULL) {
        return;
    }

    runtime->can_run_hook = can_run_hook;
    runtime->run_hook = run_hook;
    runtime->user = user;
    return;
}

NcmActionRuntime *
ncm_action_runtime_global(void) {
    if (!action_global_runtime_initialized) {
        ncm_action_runtime_init(&action_global_runtime);
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
ncm_action_runtime_clear_exit_request(NcmActionRuntime *runtime) {
    runtime = action_runtime_or_global(runtime);
    runtime->exit_requested = false;
    return;
}

bool
ncm_action_runtime_can_run(NcmActionRuntime *runtime,
                           enum NcmActionType type) {
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
ncm_action_runtime_run(NcmActionRuntime *runtime,
                       enum NcmActionType type) {
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
    if (runtime != NULL) {
        return runtime;
    }
    return ncm_action_runtime_global();
}

static int32
action_runtime_call_hook(NcmActionRuntimeHook hook,
                         enum NcmActionType type,
                         void *user) {
    if (hook == NULL) {
        return NCM_ACTION_RUNTIME_DEFER;
    }
    return hook(type, user);
}

static bool
action_runtime_hook_allowed(int32 result, bool *handled) {
    if (handled != NULL) {
        *handled = result != NCM_ACTION_RUNTIME_DEFER;
    }
    return result == NCM_ACTION_RUNTIME_ALLOW;
}

static bool
action_runtime_hook_denied(int32 result, bool *handled) {
    if (handled != NULL) {
        *handled = result != NCM_ACTION_RUNTIME_DEFER;
    }
    return result == NCM_ACTION_RUNTIME_DENY;
}

static bool
action_runtime_current_screen_is(enum ScreenType type) {
    NcScreen *screen;
    int32 native_type;

    screen = app_controller_current_screen();
    if (screen == NULL) {
        return false;
    }

    native_type = screen_type_to_native_type(type);
    return nc_screen_type(screen) == native_type;
}

static bool
action_runtime_switch_to_screen(enum ScreenType type) {
    if ((type != NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER)
        && action_runtime_current_screen_is(
            NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER)
        && !native_selected_items_adder_screen_return_to_previous(
            native_c_screen_selected_items_adder())) {
        return false;
    }

    switch (type) {
    case NCM_SCREEN_TYPE_BROWSER:
        native_c_screen_browser_register();
        native_c_screen_browser_switch_to();
        return true;
    case NCM_SCREEN_TYPE_HELP:
        native_c_screen_help_register();
        native_c_screen_help_switch_to();
        return true;
    case NCM_SCREEN_TYPE_LASTFM:
        native_c_screen_lastfm_register();
        native_c_screen_lastfm_switch_to();
        return true;
    case NCM_SCREEN_TYPE_LYRICS:
        native_c_screen_lyrics_register();
        native_c_screen_lyrics_switch_to();
        return true;
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        native_c_screen_media_library_register();
        native_c_screen_media_library_switch_to();
        return true;
#if defined(ENABLE_OUTPUTS)
    case NCM_SCREEN_TYPE_OUTPUTS:
        native_c_screen_outputs_register();
        native_c_screen_outputs_switch_to();
        return true;
#endif
    case NCM_SCREEN_TYPE_PLAYLIST:
        native_c_screen_playlist_register();
        native_c_screen_playlist_switch_to();
        return true;
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        native_c_screen_playlist_editor_register();
        native_c_screen_playlist_editor_switch_to();
        return true;
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
        native_c_screen_search_engine_register();
        native_c_screen_search_engine_switch_to();
        return true;
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
        native_c_screen_selected_items_adder_register();
        native_c_screen_selected_items_adder_switch_to();
        return true;
    case NCM_SCREEN_TYPE_SERVER_INFO:
        native_c_screen_server_info_register();
        native_c_screen_server_info_switch_to();
        return true;
    case NCM_SCREEN_TYPE_SONG_INFO:
        native_c_screen_song_info_register();
        native_c_screen_song_info_switch_to();
        return true;
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
        native_c_screen_sort_playlist_dialog_register();
        return native_c_screen_sort_playlist_dialog_switch_to();
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        native_c_screen_tag_editor_register();
        native_c_screen_tag_editor_switch_to();
        return true;
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
        native_c_screen_tiny_tag_editor_register();
        native_c_screen_tiny_tag_editor_switch_to();
        return true;
#endif
#if defined(ENABLE_VISUALIZER)
    case NCM_SCREEN_TYPE_VISUALIZER:
        return ncm_action_show_visualizer();
#endif
    case NCM_SCREEN_TYPE_UNKNOWN:
    case NCM_SCREEN_TYPE_LAST:
        break;
    }

    return false;
}

bool
ncm_action_show_visualizer(void) {
#if defined(ENABLE_VISUALIZER)
    native_c_screen_visualizer_register();
    return native_c_screens_switch_to_type(NCM_SCREEN_TYPE_VISUALIZER);
#else
    return false;
#endif
}

bool
ncm_action_toggle_visualization_type(void) {
#if defined(ENABLE_VISUALIZER)
    if (!native_c_screen_visualizer_is_current()) {
        return false;
    }
    native_visualizer_screen_toggle_type(native_c_screen_visualizer());
    return true;
#else
    return false;
#endif
}

static bool
action_runtime_switch_to_next_screen(bool reverse) {
    ScreenTypeArray *sequence;
    NcScreen *current;
    bool selected_items_adder;
    int32 current_index;
    int32 next_index;

    selected_items_adder = action_runtime_current_screen_is(
        NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER);
    if (selected_items_adder && Config.screen_switcher_previous) {
        return native_selected_items_adder_screen_return_to_previous(
            native_c_screen_selected_items_adder());
    }

    sequence = &Config.screen_sequence;
    if (sequence->len <= 0) {
        return false;
    }

    current = app_controller_current_screen();
    if (current == NULL) {
        return action_runtime_switch_to_screen(sequence->items[0]);
    }

    current_index = -1;
    for (int32 i = 0; i < sequence->len; i += 1) {
        if (screen_type_to_native_type(sequence->items[i])
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

static void
action_runtime_mpd_noidle_callback(int32 flags, void *user) {
    ncm_statusbar_mpd_noidle_callback(flags, user);
    return;
}

static bool
action_runtime_mpd_error(NcmError *error) {
    if ((error != NULL) && ncm_error_is_set(error)) {
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    error->message);
    }
    return false;
}

static bool
action_runtime_queue_find_song(NcmMpdSongList *queue,
                               NcmSong *song,
                               NcmSong **match) {
    NcmSong *item;

    if (match != NULL) {
        *match = NULL;
    }
    if (queue == NULL) {
        return false;
    }
    if (song == NULL) {
        return false;
    }

    for (int32 i = 0; i < queue->count; i += 1) {
        item = &queue->items[i];
        if (ncm_song_equal(item, song)) {
            if (match != NULL) {
                *match = item;
            }
            return true;
        }
    }

    return false;
}

static bool
action_runtime_queue_remove_song(NcmMpdSongList *queue,
                                 NcmSong *song,
                                 NcmError *error) {
    NcmSong *item;
    bool ok;

    if (queue == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD queue"));
        return false;
    }
    if (song == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD song"));
        return false;
    }

    ok = ncm_mpd_client_start_command_list(&global_mpd, error);
    for (int32 i = queue->count; ok && (i > 0); i -= 1) {
        item = &queue->items[i - 1];
        if (!ncm_song_equal(item, song)) {
            continue;
        }
        ok = ncm_mpd_client_delete(&global_mpd, ncm_song_position(item),
                                   error);
    }
    if (ok) {
        ok = ncm_mpd_client_commit_command_list(&global_mpd, error);
    }
    if (!ok && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }

    return ok;
}

static bool
action_runtime_mpd_simple(
    bool (*func)(NcmMpdClient *client, NcmError *error)) {
    NcmError error;

    ncm_error_clear(&error);
    if (!func(&global_mpd, &error)) {
        return action_runtime_mpd_error(&error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &error);
    return true;
}

bool
ncm_action_add_song_to_playlist_with_mode(NcmSong *song, bool play,
                                          int32 position,
                                          enum SpaceAddMode space_add_mode) {
    NcmMpdSongList queue;
    NcmSong *match;
    NcmError error;
    NcmBuffer formatted;
    NcmBuffer message;
    int32 id;
    bool ok;

    if (song == NULL) {
        return false;
    }
    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    ncm_error_clear(&error);
    ncm_mpd_song_list_init(&queue);
    match = NULL;
    if (space_add_mode == NCM_SPACE_ADD_MODE_ADD_REMOVE) {
        if (!ncm_mpd_client_get_queue(&global_mpd, &queue, &error)) {
            ncm_mpd_song_list_destroy(&queue);
            return action_runtime_mpd_error(&error);
        }
        if (action_runtime_queue_find_song(&queue, song, &match)) {
            if (play) {
                ok = ncm_mpd_client_play_id(&global_mpd,
                                            (int32)ncm_song_id(match),
                                            &error);
            } else {
                ok = action_runtime_queue_remove_song(&queue, song, &error);
            }
            ncm_mpd_song_list_destroy(&queue);
            if (!ok) {
                return action_runtime_mpd_error(&error);
            }
            (void)ncm_status_update_full(&global_mpd, NULL, &error);
            return true;
        }
    }
    ncm_mpd_song_list_destroy(&queue);

    id = -1;
    if (!ncm_mpd_client_add_song_value(&global_mpd, song, position,
                                       &id, &error)) {
        return action_runtime_mpd_error(&error);
    }

    formatted = ncm_format_render_string(&Config.song_status_format, song);
    ncm_buffer_init(&message);
    ncm_buffer_append(&message, STRLIT_ARGS("Added to playlist: "));
    ncm_buffer_append(&message, formatted.data, formatted.len);
    ncm_statusbar_print((int32)Config.message_delay_time,
                        message.data, message.len);
    ncm_buffer_destroy(&message);
    ncm_buffer_destroy(&formatted);

    if (play && (id >= 0)) {
        if (!ncm_mpd_client_play_id(&global_mpd, id, &error)) {
            return action_runtime_mpd_error(&error);
        }
    }

    (void)ncm_status_update_full(&global_mpd, NULL, &error);
    return true;
}

bool
ncm_action_add_song_to_playlist(NcmSong *song, bool play,
                                int32 position) {
    return ncm_action_add_song_to_playlist_with_mode(
        song, play, position, Config.space_add_mode);
}

static bool
action_runtime_mpd_toggle(
    bool (*func)(NcmMpdClient *client, bool mode, NcmError *error),
    bool current) {
    NcmError error;

    ncm_error_clear(&error);
    if (!func(&global_mpd, !current, &error)) {
        return action_runtime_mpd_error(&error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &error);
    return true;
}

static bool
action_runtime_volume(int32 change) {
    NcmError error;

    ncm_error_clear(&error);
    if (!ncm_mpd_client_change_volume(&global_mpd, change, &error)) {
        return action_runtime_mpd_error(&error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &error);
    return true;
}

static char *
action_runtime_update_database_path(void) {
    NcmStringView view;

    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        view = native_browser_screen_current_directory(
            native_c_screen_browser());
        if (view.data != NULL) {
            return view.data;
        }
        return (char *)"";
    }

#if defined(HAVE_TAGLIB_H)
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TAG_EDITOR)) {
        if (native_tag_editor_screen_current_dir(
                native_c_screen_tag_editor(), &view)) {
            return view.data;
        }
    }
#endif

    return (char *)"/";
}

static bool
action_runtime_update_database(void) {
    NcmError error;
    char *path;

    path = action_runtime_update_database_path();

    ncm_error_clear(&error);
    if (!ncm_mpd_client_update_directory(&global_mpd, path, NULL,
                                         &error)) {
        return action_runtime_mpd_error(&error);
    }
    return true;
}

static bool
action_runtime_replay_song(void) {
    NcmError error;
    int32 position;

    position = ncm_status_state_current_song_position();
    if (position < 0) {
        return false;
    }

    ncm_error_clear(&error);
    if (!ncm_mpd_client_play_pos(&global_mpd, (uint32)position,
                                 &error)) {
        return action_runtime_mpd_error(&error);
    }
    return true;
}

static bool
action_runtime_toggle_crossfade(void) {
    NcmError error;
    uint32 seconds;

    ncm_error_clear(&error);
    seconds = Config.crossfade_time;
    if (ncm_status_state_crossfade()) {
        seconds = 0;
    }
    if (!ncm_mpd_client_set_crossfade(&global_mpd, seconds, &error)) {
        return action_runtime_mpd_error(&error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &error);
    return true;
}

static bool
action_runtime_set_crossfade(void) {
    NcmBuffer input;
    NcmError error;
    uint32 seconds;
    bool prompted;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    ncm_buffer_init(&input);
    prompted = action_runtime_prompt_string(
        STRLIT_ARGS("Set crossfade to: "), (char *)"", false,
        NULL, NULL, &input);
    if (!prompted) {
        ncm_buffer_destroy(&input);
        return true;
    }

    ncm_error_clear(&error);
    if (!ncm_parse_uint32(input.data, input.len, &seconds, &error)) {
        ncm_buffer_destroy(&input);
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time,
            (char *)"Crossfade must be a non-negative number");
        return true;
    }
    ncm_buffer_destroy(&input);

    Config.crossfade_time = seconds;
    ncm_error_clear(&error);
    if (!ncm_mpd_client_set_crossfade(&global_mpd, seconds, &error)) {
        return action_runtime_mpd_error(&error);
    }
    return true;
}

static bool
action_runtime_set_volume(void) {
    NcmStringFormatArg arg;
    NcmBuffer input;
    NcmError error;
    uint32 volume;
    bool prompted;

    if (!ncm_mpd_client_connected(&global_mpd)
        || (ncm_status_state_volume() < 0)) {
        return false;
    }

    ncm_buffer_init(&input);
    prompted = action_runtime_prompt_string(
        STRLIT_ARGS("Set volume to: "), (char *)"", false,
        NULL, NULL, &input);
    if (!prompted) {
        ncm_buffer_destroy(&input);
        return true;
    }

    ncm_error_clear(&error);
    if (!ncm_parse_uint32(input.data, input.len, &volume, &error)
        || (volume > 100)) {
        ncm_buffer_destroy(&input);
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time,
            (char *)"Volume must be between 0 and 100");
        return true;
    }
    ncm_buffer_destroy(&input);

    ncm_error_clear(&error);
    if (!ncm_mpd_client_set_volume(&global_mpd, volume, &error)) {
        return action_runtime_mpd_error(&error);
    }
    arg = ncm_string_format_arg_u64(volume);
    ncm_statusbar_format((int32)Config.message_delay_time,
                         STRLIT_ARGS("Volume set to %1%%%"),
                         &arg, 1);
    return true;
}

static bool
action_runtime_add_random_items(void) {
    NcmStatusbarScopedLock lock;
    NcmStringFormatArg args[3];
    NcmBuffer input;
    NcmBuffer prompt;
    NcmError error;
    NcWindow *window;
    enum mpd_tag_type tag_type;
    char values[] = {
        's',
        'a',
        'A',
        'b',
    };
    char tag_name[32];
    char random_type;
    char *plural;
    char *source_name;
    int32 count;
    int32 source_name_len;
    uint32 number;
    bool prompted;
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    random_type = 0;
    prompted = false;
    ncm_statusbar_scoped_lock_init(&lock);
    window = ncm_statusbar_put();
    if (window) {
        nc_window_print_data(
            window,
            STRLIT_ARGS("Add random? [s]ongs/[a]rtists/"
                        "album [A]rtists/al[b]ums "));
        prompted = ncm_statusbar_prompt_return_one_of(
            window, values, LENGTH(values), &random_type);
    }
    ncm_statusbar_scoped_lock_destroy(&lock);
    if (!prompted) {
        return true;
    }

    tag_type = MPD_TAG_ARTIST;
    if (random_type == 's') {
        source_name = (char *)"song";
        source_name_len = STRLIT_LEN("song");
    } else {
        tag_type = ncm_char_to_tag_type(random_type);
        source_name = ncm_tag_type_name(tag_type);
        source_name_len = action_runtime_cstring_len(source_name);
        if (source_name_len >= (int32)sizeof(tag_name)) {
            return false;
        }
        ncm_memcpy(tag_name, source_name, source_name_len);
        tag_name[source_name_len] = '\0';
        ncm_string_lowercase_ascii(tag_name, source_name_len);
        source_name = tag_name;
    }

    args[0] = ncm_string_format_arg_string(source_name, source_name_len);
    prompt = ncm_string_format_make(
        STRLIT_ARGS("Number of random %1%s: "), args, 1);
    ncm_buffer_init(&input);
    prompted = action_runtime_prompt_string(
        prompt.data, prompt.len, (char *)"", false,
        NULL, NULL, &input);
    ncm_buffer_destroy(&prompt);
    if (!prompted) {
        ncm_buffer_destroy(&input);
        return true;
    }

    ncm_error_clear(&error);
    if (!ncm_parse_uint32(input.data, input.len, &number, &error)
        || (number > (uint32)INT32_MAX)) {
        ncm_buffer_destroy(&input);
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time,
            (char *)"Random item count must be a non-negative number");
        return true;
    }
    ncm_buffer_destroy(&input);
    count = (int32)number;
    if (count <= 0) {
        return true;
    }

    ncm_error_clear(&error);
    if (random_type == 's') {
        success = ncm_mpd_client_add_random_songs(
            &global_mpd, count, Config.random_exclude_pattern,
            Config.random_exclude_pattern_len, &global_random, &error);
    } else {
        success = ncm_mpd_client_add_random_tag(
            &global_mpd, tag_type, count, &global_random, &error);
    }
    if (!success) {
        return action_runtime_mpd_error(&error);
    }

    if (count == 1) {
        plural = (char *)"";
    } else {
        plural = (char *)"s";
    }
    args[0] = ncm_string_format_arg_i64(count);
    args[1] = ncm_string_format_arg_string(source_name, source_name_len);
    args[2] = ncm_string_format_arg_cstring(plural);
    ncm_statusbar_format(
        (int32)Config.message_delay_time,
        STRLIT_ARGS("%1% random %2%%3% added to playlist"),
        args, LENGTH(args));
    return true;
}

static bool
action_runtime_update_environment(void) {
    return ncmpcpp_legacy_update_environment(true, true, true);
}

static void
action_runtime_print_toggle(char *format, int32 format_len,
                            char *value) {
    NcmStringFormatArg arg;

    arg = ncm_string_format_arg_cstring(value);
    ncm_statusbar_format((int32)Config.message_delay_time, format,
                         format_len, &arg, 1);
    return;
}

static bool
action_runtime_toggle_interface(void) {
    NcmStatusbarScopedLock lock;

    switch (Config.design) {
    case NCM_DESIGN_CLASSIC:
        Config.design = NCM_DESIGN_ALTERNATIVE;
        Config.statusbar_visibility = false;
        break;
    case NCM_DESIGN_ALTERNATIVE:
        Config.design = NCM_DESIGN_CLASSIC;
        Config.statusbar_visibility =
            ui_state_statusbar_visibility_baseline();
        break;
    case NCM_DESIGN_LAST:
        return false;
    }

    ncmpcpp_legacy_resize_screen(false);
    ncm_progressbar_scoped_lock_init(&lock);
    ncm_progressbar_scoped_lock_destroy(&lock);
    ncm_status_changes_mixer();
    ncm_status_changes_elapsed_time(false);
    action_runtime_print_toggle(STRLIT_ARGS("User interface: %1%"),
                                ncm_design_str(Config.design));
    return true;
}

static bool
action_runtime_toggle_separators_between_albums(void) {
    Config.playlist_separate_albums = !Config.playlist_separate_albums;
    app_controller_request_current_screen_resize();
    if (Config.playlist_separate_albums) {
        action_runtime_print_toggle(
            STRLIT_ARGS("Separators between albums: %1%"),
            (char *)"on");
    } else {
        action_runtime_print_toggle(
            STRLIT_ARGS("Separators between albums: %1%"),
            (char *)"off");
    }
    return true;
}

static bool
action_runtime_toggle_lyrics_update_on_song_change(void) {
    if (!native_c_screen_lyrics_is_current()) {
        return false;
    }
    Config.now_playing_lyrics = !Config.now_playing_lyrics;
    if (Config.now_playing_lyrics) {
        action_runtime_print_toggle(
            STRLIT_ARGS("Update lyrics if song changes: %1%"),
            (char *)"on");
    } else {
        action_runtime_print_toggle(
            STRLIT_ARGS("Update lyrics if song changes: %1%"),
            (char *)"off");
    }
    return true;
}

static bool
action_runtime_toggle_fetching_lyrics_in_background(void) {
    Config.fetch_lyrics_in_background =
        !Config.fetch_lyrics_in_background;
    if (Config.fetch_lyrics_in_background) {
        action_runtime_print_toggle(
            STRLIT_ARGS(
                "Fetching lyrics for playing songs in background: %1%"),
            (char *)"on");
    } else {
        action_runtime_print_toggle(
            STRLIT_ARGS(
                "Fetching lyrics for playing songs in background: %1%"),
            (char *)"off");
    }
    return true;
}

static bool
action_runtime_toggle_add_mode(void) {
    char *mode_desc;

    mode_desc = (char *)"";
    switch (Config.space_add_mode) {
    case NCM_SPACE_ADD_MODE_ADD_REMOVE:
        Config.space_add_mode = NCM_SPACE_ADD_MODE_ALWAYS_ADD;
        mode_desc = (char *)"always add an item to playlist";
        break;
    case NCM_SPACE_ADD_MODE_ALWAYS_ADD:
        Config.space_add_mode = NCM_SPACE_ADD_MODE_ADD_REMOVE;
        mode_desc =
            (char *)"add an item to playlist or remove if already added";
        break;
    case NCM_SPACE_ADD_MODE_LAST:
        mode_desc = (char *)"";
        break;
    }
    action_runtime_print_toggle(STRLIT_ARGS("Add mode: %1%"), mode_desc);
    return true;
}

static bool
action_runtime_toggle_mouse(void) {
    Config.mouse_support = !Config.mouse_support;
    if (Config.mouse_support) {
        nc_mouse_enable();
        action_runtime_print_toggle(STRLIT_ARGS("Mouse support %1%"),
                                    (char *)"enabled");
    } else {
        nc_mouse_disable();
        action_runtime_print_toggle(STRLIT_ARGS("Mouse support %1%"),
                                    (char *)"disabled");
    }
    return true;
}

static bool
action_runtime_toggle_bitrate_visibility(void) {
    Config.display_bitrate = !Config.display_bitrate;
    if (Config.display_bitrate) {
        action_runtime_print_toggle(STRLIT_ARGS("Bitrate visibility %1%"),
                                    (char *)"enabled");
    } else {
        action_runtime_print_toggle(STRLIT_ARGS("Bitrate visibility %1%"),
                                    (char *)"disabled");
    }
    return true;
}

static int32
action_runtime_cstring_len(char *string) {
    int32 len;

    len = 0;
    if (string == NULL) {
        return 0;
    }
    while (string[len] != '\0') {
        len += 1;
    }
    return len;
}

static void
action_runtime_print_format_string(char *format, int32 format_len,
                                   char *text, int32 text_len) {
    NcmStringFormatArg arg;

    arg = ncm_string_format_arg_string(text, text_len);
    ncm_statusbar_format((int32)Config.message_delay_time, format,
                         format_len, &arg, 1);
    return;
}

bool
ncm_action_immediate_command_prompt_should_stop(NcmBuffer *previous,
                                                char *text,
                                                int32 text_len) {
    NcmCommand *command;

    if (previous == NULL) {
        return false;
    }
    if (text == NULL) {
        text = (char *)"";
        text_len = 0;
    }

    if ((previous->len == text_len)
        && ((text_len == 0)
            || (cbase_memcmp(previous->data, text, text_len) == 0))) {
        return false;
    }

    if (!ncm_buffer_set(previous, text, text_len)) {
        return false;
    }

    command = ncm_bindings_configuration_find_command(&Bindings, text,
                                                      text_len);
    if ((command != NULL) && command->immediate) {
        return true;
    }
    return false;
}

static bool
action_runtime_command_prompt_hook(char *text, void *user) {
    ActionRuntimeCommandPrompt *state;
    int32 text_len;

    state = user;
    text_len = action_runtime_cstring_len(text);
    if (!ncm_statusbar_main_hook(text, text_len)) {
        return false;
    }
    if (ncm_action_immediate_command_prompt_should_stop(
            &state->previous, text, text_len)) {
        return false;
    }
    return true;
}

static bool
action_runtime_filter_prompt_hook(char *text, void *user) {
    NcmError error;
    int32 text_len;

    (void)user;
    text_len = action_runtime_cstring_len(text);
    if (!ncm_statusbar_main_hook(text, text_len)) {
        return false;
    }

    ncm_error_clear(&error);
    (void)current_screen_apply_filter(text, text_len, &error);
    return true;
}

static bool
action_runtime_search_prompt_hook(char *text, void *user) {
    ActionRuntimeSearchPrompt *state;
    NcmError error;
    int32 text_len;

    state = user;
    text_len = action_runtime_cstring_len(text);
    if (!ncm_statusbar_main_hook(text, text_len)) {
        return false;
    }

    ncm_error_clear(&error);
    (void)current_screen_search(state->direction, text, text_len,
                                Config.wrapped_search, false, &error);
    return true;
}

static bool
action_runtime_prompt_result(NcmBuffer *result, NcPrompt *prompt,
                             NcWindow *window) {
    enum NcPromptStatus status;
    char *text;
    int32 text_len;
    bool ok;

    if (result == NULL) {
        return false;
    }

    if (window == NULL) {
        return false;
    }

    text = NULL;
    status = nc_window_prompt(window, prompt, &text);
    if ((status != NC_PROMPT_ACCEPTED) || (text == NULL)) {
        nc_window_prompt_result_destroy(text);
        return false;
    }

    text_len = action_runtime_cstring_len(text);
    ok = ncm_buffer_set(result, text, text_len);
    nc_window_prompt_result_destroy(text);
    return ok;
}

static bool
action_runtime_prompt_string(char *prefix, int32 prefix_len,
                             char *initial_text, bool remember,
                             NcPromptHook hook, void *hook_user,
                             NcmBuffer *result) {
    NcmStatusbarScopedLock lock;
    NcPrompt prompt;
    NcWindow *window;
    bool ok;

    if (initial_text == NULL) {
        initial_text = (char *)"";
    }

    ok = false;
    ncm_statusbar_scoped_lock_init(&lock);
    window = ncm_statusbar_put();
    if (window != NULL) {
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
    ncm_statusbar_scoped_lock_destroy(&lock);
    return ok;
}

static bool
action_runtime_confirm(char *message, int32 message_len) {
    NcmStatusbarScopedLock lock;
    NcWindow *window;
    char values[] = {
        'y',
        'n',
    };
    char answer;
    bool prompted;

    prompted = false;
    answer = 'n';
    ncm_statusbar_scoped_lock_init(&lock);
    window = ncm_statusbar_put();
    if (window != NULL) {
        nc_window_print_data(window, message, message_len);
        nc_window_print_data(window, STRLIT_ARGS(" [y/n] "));
        prompted = ncm_statusbar_prompt_return_one_of(
            window, values, LENGTH(values), &answer);
    }
    ncm_statusbar_scoped_lock_destroy(&lock);

    if (!prompted || (answer == 'n')) {
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    (char *)"Action cancelled");
        return false;
    }
    return true;
}

static bool
action_runtime_parse_seek_position(char *text, int32 text_len,
                                   uint32 total, uint32 *position) {
    NcmError error;
    uint32 first;
    uint32 second;
    uint32 third;
    uint64 result;
    int32 first_colon;
    int32 second_colon;
    int32 number_len;

    if ((text == NULL) || (text_len <= 0) || (position == NULL)) {
        return false;
    }

    first_colon = -1;
    second_colon = -1;
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

    ncm_error_clear(&error);
    if (first_colon >= 0) {
        if ((first_colon == 0) || (first_colon == text_len - 1)) {
            return false;
        }
        if (second_colon < 0) {
            if ((text_len - first_colon - 1) != 2) {
                return false;
            }
            if (!ncm_parse_uint32(text, first_colon, &first, &error)
                || !ncm_parse_uint32(text + first_colon + 1, 2,
                                     &second, &error)
                || (second > 60)) {
                return false;
            }
            result = (uint64)first*60 + second;
        } else {
            if (((second_colon - first_colon - 1) != 2)
                || ((text_len - second_colon - 1) != 2)) {
                return false;
            }
            if (!ncm_parse_uint32(text, first_colon, &first, &error)
                || !ncm_parse_uint32(text + first_colon + 1, 2,
                                     &second, &error)
                || !ncm_parse_uint32(text + second_colon + 1, 2,
                                     &third, &error)
                || (second > 60) || (third > 60)) {
                return false;
            }
            result = (uint64)first*3600 + (uint64)second*60 + third;
        }
        if (result > UINT32_MAX) {
            return false;
        }
        *position = (uint32)result;
        return true;
    }

    number_len = text_len;
    if (text[text_len - 1] == 's') {
        number_len -= 1;
        if (number_len <= 0) {
            return false;
        }
        if (!ncm_parse_uint32(text, number_len, &first, &error)) {
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
    if (!ncm_parse_uint32(text, number_len, &first, &error)
        || (first > 100)) {
        return false;
    }
    *position = (uint32)(((uint64)first*total)/100);
    return true;
}

static bool
action_runtime_execute_binding(NcmBinding *binding) {
#if defined(__GNUC__)
    if (ncmpcpp_legacy_execute_binding != NULL) {
        return ncmpcpp_legacy_execute_binding(binding);
    }
#endif
    return ncm_binding_execute_default(binding);
}

static bool
action_runtime_execute_command(void) {
    ActionRuntimeCommandPrompt state;
    NcmBuffer command_name;
    NcmCommand *command;
    bool prompted;
    bool result;

    ncm_buffer_init(&state.previous);
    ncm_buffer_init(&command_name);
    prompted = action_runtime_prompt_string(STRLIT_ARGS(":"),
                                            (char *)"", true,
                                            action_runtime_command_prompt_hook,
                                            &state, &command_name);
    if (!prompted && (state.previous.len > 0)) {
        ncm_buffer_copy(&command_name, &state.previous);
        prompted = true;
    }
    ncm_buffer_destroy(&state.previous);

    if (!prompted) {
        ncm_buffer_destroy(&command_name);
        return true;
    }

    command = ncm_bindings_configuration_find_command(
        &Bindings, command_name.data, command_name.len);
    if (command == NULL) {
        action_runtime_print_format_string(STRLIT_ARGS(
            "No command named \"%1%\""),
            command_name.data, command_name.len);
        ncm_buffer_destroy(&command_name);
        return true;
    }

    action_runtime_print_format_string(STRLIT_ARGS("Executing %1%..."),
                                       command_name.data, command_name.len);
    result = action_runtime_execute_binding(&command->binding);
    if (result) {
        action_runtime_print_format_string(STRLIT_ARGS(
            "Execution of command \"%1%\" successful."),
            command_name.data, command_name.len);
    } else {
        action_runtime_print_format_string(STRLIT_ARGS(
            "Execution of command \"%1%\" unsuccessful."),
            command_name.data, command_name.len);
    }

    ncm_buffer_destroy(&command_name);
    return result;
}

static bool
action_runtime_save_playlist(void) {
    NcmBuffer name;
    NcmError error;
    bool prompted;
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    ncm_buffer_init(&name);
    prompted = action_runtime_prompt_string(
        STRLIT_ARGS("Save playlist as: "), (char *)"", false,
        NULL, NULL, &name);
    if (!prompted) {
        ncm_buffer_destroy(&name);
        return true;
    }

    ncm_error_clear(&error);
    success = ncm_mpd_client_save_playlist(&global_mpd, name.data,
                                           &error);
    if (!success
        && (ncm_mpd_client_server_error_code(&global_mpd)
            == MPD_SERVER_ERROR_EXIST)) {
        NcmBuffer question;

        ncm_buffer_init(&question);
        ncm_buffer_append(&question, STRLIT_ARGS("Playlist \""));
        ncm_buffer_append(&question, name.data, name.len);
        ncm_buffer_append(&question,
                          STRLIT_ARGS("\" already exists, overwrite?"));
        success = action_runtime_confirm(question.data, question.len);
        ncm_buffer_destroy(&question);
        if (!success) {
            ncm_buffer_destroy(&name);
            return true;
        }

        ncm_error_clear(&error);
        success = ncm_mpd_client_delete_playlist(&global_mpd, name.data,
                                                 &error);
        if (success) {
            success = ncm_mpd_client_save_playlist(&global_mpd,
                                                   name.data, &error);
        }
        if (success) {
            ncm_statusbar_print_cstring(
                (int32)Config.message_delay_time,
                (char *)"Playlist overwritten");
        }
    } else if (success) {
        action_runtime_print_format_string(
            STRLIT_ARGS("Playlist saved as \"%1%\""),
            name.data, name.len);
    }

    ncm_buffer_destroy(&name);
    if (!success) {
        return action_runtime_mpd_error(&error);
    }
    return true;
}

static bool
action_runtime_apply_filter(void) {
    NcmStringView current_filter;
    NcmBuffer filter;
    NcmBuffer previous_filter;
    NcmError error;
    bool old_autocenter_mode;
    bool prompted;

    if (!current_screen_allows_filter()) {
        return false;
    }

    ncm_buffer_init(&filter);
    ncm_buffer_init(&previous_filter);
    current_filter = current_screen_current_filter();
    if ((current_filter.data != NULL) && (current_filter.len > 0)) {
        ncm_buffer_set(&filter, current_filter.data, current_filter.len);
        ncm_buffer_copy(&previous_filter, &filter);
        ncm_error_clear(&error);
        (void)current_screen_apply_filter(filter.data, filter.len, &error);
    }

    old_autocenter_mode = Config.autocenter_mode;
    Config.autocenter_mode = false;
    prompted = action_runtime_prompt_string(STRLIT_ARGS("Apply filter: "),
                                            filter.data, false,
                                            action_runtime_filter_prompt_hook,
                                            NULL, &filter);
    Config.autocenter_mode = old_autocenter_mode;

    if (!prompted) {
        ncm_error_clear(&error);
        (void)current_screen_apply_filter(previous_filter.data,
                                          previous_filter.len, &error);
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    (char *)"Action cancelled");
        ncm_buffer_destroy(&previous_filter);
        ncm_buffer_destroy(&filter);
        return true;
    }

    ncm_error_clear(&error);
    (void)current_screen_apply_filter(filter.data, filter.len, &error);
    if (filter.len == 0) {
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    (char *)"Filtering disabled");
    } else {
        action_runtime_print_format_string(STRLIT_ARGS(
            "Using filter \"%1%\""), filter.data, filter.len);
    }

    ncm_buffer_destroy(&previous_filter);
    ncm_buffer_destroy(&filter);
    return true;
}

static bool
action_runtime_find_item(enum SearchDirection direction) {
    ActionRuntimeSearchPrompt state;
    NcmStringView current_constraint;
    NcmBuffer constraint;
    NcmBuffer previous_constraint;
    NcmError error;
    bool old_autocenter_mode;
    bool prompted;
    char prompt[64];
    int32 prompt_len;

    if (!current_screen_allows_search()) {
        return false;
    }

    state.direction = direction;
    ncm_buffer_init(&constraint);
    ncm_buffer_init(&previous_constraint);
    current_constraint = current_screen_current_search_constraint();
    if ((current_constraint.data != NULL) && (current_constraint.len > 0)) {
        ncm_buffer_set(&constraint, current_constraint.data,
                       current_constraint.len);
        ncm_buffer_copy(&previous_constraint, &constraint);
    }

    prompt_len = snprintf(prompt, (size_t)SIZEOF(prompt), "Find %s: ",
                          ncm_search_direction_str(direction));
    if (prompt_len < 0) {
        prompt_len = 0;
    }
    if (prompt_len >= (int32)SIZEOF(prompt)) {
        prompt_len = (int32)SIZEOF(prompt) - 1;
    }

    old_autocenter_mode = Config.autocenter_mode;
    Config.autocenter_mode = false;
    prompted = action_runtime_prompt_string(prompt, prompt_len,
                                            constraint.data, false,
                                            action_runtime_search_prompt_hook,
                                            &state, &constraint);
    Config.autocenter_mode = old_autocenter_mode;

    if (!prompted) {
        if (previous_constraint.len == 0) {
            current_screen_clear_search_constraint();
        } else {
            ncm_error_clear(&error);
            (void)current_screen_search(direction, previous_constraint.data,
                                        previous_constraint.len,
                                        Config.wrapped_search, false,
                                        &error);
        }
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    (char *)"Action cancelled");
        ncm_buffer_destroy(&previous_constraint);
        ncm_buffer_destroy(&constraint);
        return true;
    }

    if (constraint.len == 0) {
        current_screen_clear_search_constraint();
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    (char *)"Constraint unset");
    } else {
        ncm_error_clear(&error);
        (void)current_screen_search(direction, constraint.data,
                                    constraint.len, Config.wrapped_search,
                                    false, &error);
        action_runtime_print_format_string(STRLIT_ARGS(
            "Using constraint \"%1%\""),
            constraint.data, constraint.len);
    }

    ncm_buffer_destroy(&previous_constraint);
    ncm_buffer_destroy(&constraint);
    return true;
}

static bool
action_runtime_repeat_search(enum SearchDirection direction) {
    NcmStringView constraint;
    NcmError error;

    if (!current_screen_allows_search()) {
        return false;
    }

    constraint = current_screen_current_search_constraint();
    if ((constraint.data == NULL) || (constraint.len <= 0)) {
        return true;
    }

    ncm_error_clear(&error);
    (void)current_screen_search(direction, constraint.data,
                                constraint.len, Config.wrapped_search,
                                true, &error);
    if (ncm_error_is_set(&error)) {
        return action_runtime_mpd_error(&error);
    }
    return true;
}

static enum NcMenuItemSource
action_runtime_menu_item_source(NcMenu *menu) {
    if ((menu != NULL) && nc_menu_is_filtered(menu)) {
        return NC_MENU_ITEMS_FILTERED;
    }
    return NC_MENU_ITEMS_ALL;
}

static NcMenu *
action_runtime_current_menu(void) {
    switch (native_c_screens_current_type()) {
    case NCM_SCREEN_TYPE_BROWSER:
        return native_browser_screen_menu(native_c_screen_browser());
    case NCM_SCREEN_TYPE_PLAYLIST:
        return native_playlist_screen_menu(native_c_screen_playlist());
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        return native_playlist_editor_screen_active_menu(
            native_c_screen_playlist_editor());
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
        return native_search_engine_screen_menu(
            native_c_screen_search_engine());
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        return native_media_library_screen_active_menu(
            native_c_screen_media_library());
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
        return native_selected_items_adder_screen_active_menu(
            native_c_screen_selected_items_adder());
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
        return nc_editor_sort_menu_base(
            native_sort_playlist_dialog_menu(
                native_c_screen_sort_playlist_dialog()));
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        return native_tag_editor_screen_active_menu(
            native_c_screen_tag_editor());
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
        return nc_editor_buffer_menu_base(
            native_tiny_tag_editor_screen_rows(
                native_c_screen_tiny_tag_editor()));
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
    case NCM_SCREEN_TYPE_UNKNOWN:
    case NCM_SCREEN_TYPE_LAST:
        break;
    }
    return NULL;
}

static bool
action_runtime_menu_has_items(void) {
    NcMenu *menu;

    menu = action_runtime_current_menu();
    if (menu == NULL) {
        return false;
    }
    return !nc_menu_empty(menu);
}

static bool
action_runtime_selected_songs(NcmSongArray *songs) {
    if (songs == NULL) {
        return false;
    }

    switch (native_c_screens_current_type()) {
    case NCM_SCREEN_TYPE_BROWSER:
        return native_browser_screen_selected_songs(
            native_c_screen_browser(), songs);
    case NCM_SCREEN_TYPE_PLAYLIST:
        return native_playlist_screen_selected_songs(
            native_c_screen_playlist(), songs);
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        return native_playlist_editor_screen_selected_songs(
            native_c_screen_playlist_editor(), songs);
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
        return native_search_engine_screen_selected_songs(
            native_c_screen_search_engine(), songs);
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        return native_media_library_screen_selected_songs(
            native_c_screen_media_library(), songs);
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        return native_tag_editor_screen_selected_songs(
            native_c_screen_tag_editor(), songs);
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
    case NCM_SCREEN_TYPE_UNKNOWN:
    case NCM_SCREEN_TYPE_LAST:
        break;
    }
    return false;
}

static bool
action_runtime_has_selected_songs(void) {
    NcmSongArray songs;
    bool result;

    ncm_song_array_init(&songs);
    result = action_runtime_selected_songs(&songs) && (songs.len > 0);
    ncm_song_array_destroy(&songs);
    return result;
}

static bool
action_runtime_has_current_song(void) {
    NcmSong song;
    bool result;

    ncm_song_init(&song);
    result = action_runtime_current_song(&song);
    ncm_song_destroy(&song);
    return result;
}

bool
ncm_action_current_song(NcmSong *song) {
    return action_runtime_current_song(song);
}

static bool
action_runtime_current_song(NcmSong *song) {
    NcmSong *lyrics_song;

    if (song == NULL) {
        return false;
    }

    switch (native_c_screens_current_type()) {
    case NCM_SCREEN_TYPE_BROWSER:
        return native_browser_screen_current_song(
            native_c_screen_browser(), song);
    case NCM_SCREEN_TYPE_PLAYLIST:
        return native_playlist_screen_current_song(
            native_c_screen_playlist(), song);
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        return native_playlist_editor_screen_current_song(
            native_c_screen_playlist_editor(), song);
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
        return native_search_engine_screen_current_song(
            native_c_screen_search_engine(), song);
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        return native_media_library_screen_current_song(
            native_c_screen_media_library(), song);
    case NCM_SCREEN_TYPE_LYRICS:
        lyrics_song = native_lyrics_screen_song(native_c_screen_lyrics());
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
    case NCM_SCREEN_TYPE_UNKNOWN:
    case NCM_SCREEN_TYPE_LAST:
        break;
    }
    return false;
}

static void
action_runtime_sort_positions(uint32 *positions, int32 count,
                              bool descending) {
    uint32 value;

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
action_runtime_song_positions(NcmSongArray *songs, uint32 **positions,
                              int32 *count) {
    uint32 *result;

    if ((songs == NULL) || (positions == NULL) || (count == NULL)) {
        return false;
    }
    if (songs->len <= 0) {
        return false;
    }

    result = ncm_malloc((uint64)songs->len*SIZEOF(*result));
    if (result == NULL) {
        return false;
    }
    for (int32 i = 0; i < songs->len; i += 1) {
        result[i] = ncm_song_position(&songs->items[i]);
    }

    *positions = result;
    *count = songs->len;
    return true;
}

static bool
action_runtime_add_selected_songs(bool play) {
    NcmSongArray songs;
    bool success;
    bool first;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    ncm_song_array_init(&songs);
    success = action_runtime_selected_songs(&songs) && (songs.len > 0);
    if (!success) {
        ncm_song_array_destroy(&songs);
        return false;
    }

    first = true;
    for (int32 i = 0; success && (i < songs.len); i += 1) {
        success = ncm_action_add_song_to_playlist(&songs.items[i],
                                                  play && first, -1);
        first = false;
    }

    ncm_song_array_destroy(&songs);
    return success;
}

static bool
action_runtime_playlist_editor_playlists_active(void) {
    NativePlaylistEditorScreen *screen;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return false;
    }
    screen = native_c_screen_playlist_editor();
    return native_playlist_editor_screen_active_menu(screen)
        == nc_playlist_entry_menu_base(
            native_playlist_editor_screen_playlists(screen));
}

static bool
action_runtime_playlist_editor_content_active(void) {
    NativePlaylistEditorScreen *screen;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return false;
    }
    screen = native_c_screen_playlist_editor();
    return native_playlist_editor_screen_active_menu(screen)
        == nc_song_menu_base(native_playlist_editor_screen_content(screen));
}

static bool
action_runtime_playlist_editor_has_playlists(void) {
    NativePlaylistEditorScreen *screen;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return false;
    }
    screen = native_c_screen_playlist_editor();
    return nc_menu_all_item_count(nc_playlist_entry_menu_base(
               native_playlist_editor_screen_playlists(screen))) > 0;
}

static bool
action_runtime_playlist_editor_has_content(void) {
    NativePlaylistEditorScreen *screen;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return false;
    }
    screen = native_c_screen_playlist_editor();
    return nc_menu_all_item_count(nc_song_menu_base(
               native_playlist_editor_screen_content(screen))) > 0;
}

static bool
action_runtime_add_playlist_editor_item(bool play) {
    NativePlaylistEditorScreen *screen;
    NcmPlaylist playlist;
    NcmError error;
    uint32 play_position;
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

    screen = native_c_screen_playlist_editor();
    ncm_playlist_init(&playlist);
    success = native_playlist_editor_screen_current_playlist(screen,
                                                            &playlist);
    if (!success) {
        ncm_playlist_destroy(&playlist);
        return false;
    }

    loaded = false;
    play_position = ncm_status_state_playlist_length();
    ncm_error_clear(&error);
    success = ncm_mpd_client_load_playlist(&global_mpd, playlist.path,
                                           &loaded, &error);
    if (success && play && loaded) {
        success = ncm_mpd_client_play_pos(&global_mpd,
                                          (int32)play_position,
                                          &error);
    }
    ncm_playlist_destroy(&playlist);
    if (!success) {
        return action_runtime_mpd_error(&error);
    }

    (void)ncm_status_update_full(&global_mpd, NULL, &error);
    return true;
}

static bool
action_runtime_add_raw_path_to_playlist_editor(void) {
    NativePlaylistEditorScreen *screen;
    NcmPlaylist playlist;
    NcmBuffer path;
    NcmError error;
    bool prompted;
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (!action_runtime_playlist_editor_has_playlists()) {
        return false;
    }

    screen = native_c_screen_playlist_editor();
    ncm_playlist_init(&playlist);
    success = native_playlist_editor_screen_current_playlist(screen,
                                                            &playlist);
    if (!success) {
        ncm_playlist_destroy(&playlist);
        return false;
    }

    ncm_buffer_init(&path);
    prompted = action_runtime_prompt_string(
        STRLIT_ARGS("Add to playlist: "), (char *)"", false,
        NULL, NULL, &path);
    if (!prompted) {
        ncm_buffer_destroy(&path);
        ncm_playlist_destroy(&playlist);
        return true;
    }

    if ((path.len <= 0)
        && !action_runtime_confirm(
            STRLIT_ARGS("Are you sure you want to add the whole database?"))) {
        ncm_buffer_destroy(&path);
        ncm_playlist_destroy(&playlist);
        return true;
    }

    ncm_statusbar_print_cstring(0, (char *)"Adding...");
    ncm_error_clear(&error);
    success = ncm_mpd_client_add_to_playlist(&global_mpd, playlist.path,
                                             path.data, &error);
    ncm_buffer_destroy(&path);
    ncm_playlist_destroy(&playlist);
    if (!success) {
        return action_runtime_mpd_error(&error);
    }
    native_playlist_editor_screen_request_content_update(screen);
    return true;
}

static bool
action_runtime_delete_browser_items(void) {
    NativeBrowserScreen *screen;
    NcMenu *menu;
    NcmMpdItem *item;
    NcmBuffer question;
    NcmBuffer name;
    NcmError error;
    bool success;
    bool has_selected;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        return false;
    }
    screen = native_c_screen_browser();
    menu = native_browser_screen_menu(screen);
    if ((menu == NULL) || nc_menu_empty(menu)) {
        return false;
    }
    if (!Config.allow_for_physical_item_deletion) {
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time,
            (char *)"Flag \"allow_for_physical_item_deletion\" needs to be "
                    "enabled in configuration file");
        return false;
    }
    if (!native_browser_screen_is_local(screen)
        && (Config.mpd_music_dir_len <= 0)) {
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time,
            (char *)"Proper mpd_music_dir variable has to be set in "
                    "configuration file");
        return false;
    }

    has_selected = nc_menu_has_selected(menu);
    ncm_buffer_init(&question);
    ncm_buffer_init(&name);
    if (has_selected) {
        ncm_buffer_append(&question, STRLIT_ARGS("Delete selected items?"));
    } else {
        item = nc_menu_current_item(menu);
        if (native_browser_screen_item_is_parent(item)) {
            ncm_buffer_destroy(&name);
            ncm_buffer_destroy(&question);
            return true;
        }
        if (!action_runtime_browser_item_name(item, &name)) {
            ncm_buffer_destroy(&name);
            ncm_buffer_destroy(&question);
            return false;
        }
        ncm_buffer_append(&question, STRLIT_ARGS("Delete \""));
        ncm_buffer_append(&question, name.data, name.len);
        ncm_buffer_append(&question, STRLIT_ARGS("\"?"));
    }

    success = action_runtime_confirm(question.data, question.len);
    ncm_buffer_destroy(&name);
    ncm_buffer_destroy(&question);
    if (!success) {
        return true;
    }

    ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                (char *)"Deleting items...");
    ncm_error_clear(&error);
    if (!native_browser_screen_delete_items(screen, &global_mpd, &error)) {
        return action_runtime_mpd_error(&error);
    }
    ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                (char *)"Item(s) deleted");
    return true;
}

static bool
action_runtime_browser_item_name(NcmMpdItem *item, NcmBuffer *name) {
    NcmStringView view;
    int32 basename;

    if (item == NULL || name == NULL) {
        return false;
    }
    ncm_buffer_clear(name);
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
    case NCM_MPD_ITEM_UNKNOWN:
        return false;
    }

    basename = ncm_path_basename_start(view.data, view.len);
    ncm_buffer_append(name, view.data + basename, view.len - basename);
    return true;
}

static void
action_runtime_print_renamed(char *prefix, int32 prefix_len,
                             NcmBuffer *name) {
    NcmBuffer message;

    if (name == NULL) {
        return;
    }

    ncm_buffer_init(&message);
    ncm_buffer_append(&message, prefix, prefix_len);
    ncm_buffer_append(&message, name->data, name->len);
    ncm_buffer_append(&message, STRLIT_ARGS("\""));
    ncm_statusbar_print((int32)Config.message_delay_time,
                        message.data, message.len);
    ncm_buffer_destroy(&message);
    return;
}

bool
ncm_action_delete_playlist_items(void) {
    return action_runtime_delete_playlist_items();
}

static bool
action_runtime_delete_main_playlist_items(void) {
    NcmSongArray songs;
    NcmError error;
    uint32 *positions;
    int32 count;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
        return false;
    }

    ncm_song_array_init(&songs);
    if (!native_playlist_screen_selected_songs(native_c_screen_playlist(),
                                               &songs)) {
        ncm_song_array_destroy(&songs);
        return false;
    }
    if (!action_runtime_song_positions(&songs, &positions, &count)) {
        ncm_song_array_destroy(&songs);
        return false;
    }

    ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                (char *)"Deleting items...");
    action_runtime_sort_positions(positions, count, true);
    ncm_error_clear(&error);
    for (int32 i = 0; i < count; i += 1) {
        if (!ncm_mpd_client_delete(&global_mpd, positions[i], &error)) {
            ncm_free(positions, (uint64)count*SIZEOF(*positions));
            ncm_song_array_destroy(&songs);
            return action_runtime_mpd_error(&error);
        }
    }

    ncm_free(positions, (uint64)count*SIZEOF(*positions));
    ncm_song_array_destroy(&songs);
    (void)ncm_status_update_full(&global_mpd, NULL, &error);
    ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                (char *)"Item(s) deleted");
    return true;
}

static bool
action_runtime_delete_playlist_editor_items(void) {
    NativePlaylistEditorScreen *screen;
    NcmPlaylist playlist;
    NcmSongArray songs;
    NcmError error;
    uint32 *positions;
    int32 count;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (!action_runtime_playlist_editor_content_active()) {
        return false;
    }

    screen = native_c_screen_playlist_editor();
    ncm_playlist_init(&playlist);
    ncm_song_array_init(&songs);
    if (!native_playlist_editor_screen_current_playlist(screen,
                                                        &playlist)) {
        ncm_playlist_destroy(&playlist);
        ncm_song_array_destroy(&songs);
        return false;
    }
    if (!native_playlist_editor_screen_selected_songs(screen, &songs)) {
        ncm_playlist_destroy(&playlist);
        ncm_song_array_destroy(&songs);
        return false;
    }
    if (!action_runtime_song_positions(&songs, &positions, &count)) {
        ncm_playlist_destroy(&playlist);
        ncm_song_array_destroy(&songs);
        return false;
    }

    ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                (char *)"Deleting items...");
    action_runtime_sort_positions(positions, count, true);
    ncm_error_clear(&error);
    for (int32 i = 0; i < count; i += 1) {
        if (!ncm_mpd_client_playlist_delete(&global_mpd, playlist.path,
                                            positions[i], &error)) {
            ncm_free(positions, (uint64)count*SIZEOF(*positions));
            ncm_playlist_destroy(&playlist);
            ncm_song_array_destroy(&songs);
            return action_runtime_mpd_error(&error);
        }
    }

    ncm_free(positions, (uint64)count*SIZEOF(*positions));
    ncm_playlist_destroy(&playlist);
    ncm_song_array_destroy(&songs);
    native_playlist_editor_screen_request_content_update(screen);
    ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                (char *)"Item(s) deleted");
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
    NativePlaylistEditorScreen *screen;
    NcMenu *menu;
    NcmPlaylist *playlist;
    NcmBuffer question;
    NcmError error;
    enum NcMenuItemSource source;
    int64 count;
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

    screen = native_c_screen_playlist_editor();
    menu = nc_playlist_entry_menu_base(
        native_playlist_editor_screen_playlists(screen));
    source = action_runtime_menu_item_source(menu);
    has_selected = nc_menu_has_selected(menu);

    ncm_buffer_init(&question);
    if (has_selected) {
        ncm_buffer_append(&question, STRLIT_ARGS("Delete selected playlists?"));
    } else {
        playlist = nc_menu_current_item(menu);
        if (playlist == NULL || playlist->path == NULL) {
            ncm_buffer_destroy(&question);
            return false;
        }
        ncm_buffer_append(&question, STRLIT_ARGS("Delete playlist \""));
        ncm_buffer_append(&question, playlist->path, playlist->path_len);
        ncm_buffer_append(&question, STRLIT_ARGS("\"?"));
    }
    success = action_runtime_confirm(question.data, question.len);
    ncm_buffer_destroy(&question);
    if (!success) {
        return true;
    }

    ncm_error_clear(&error);
    success = true;
    count = nc_menu_item_count(menu);
    for (int64 i = 0; success && (i < count); i += 1) {
        if (has_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!has_selected && (i != nc_menu_highlight(menu))) {
            continue;
        }
        playlist = nc_menu_item_at(menu, source, i);
        if (playlist == NULL || playlist->path == NULL) {
            success = false;
            break;
        }
        success = ncm_mpd_client_delete_playlist(&global_mpd,
                                                 playlist->path,
                                                 &error);
    }
    if (!success) {
        return action_runtime_mpd_error(&error);
    }

    native_playlist_editor_screen_request_playlists_update(screen);
    if (has_selected) {
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    (char *)"Playlists deleted");
    } else {
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    (char *)"Playlist deleted");
    }
    return true;
}

static bool
action_runtime_clear_playlist(bool main_playlist) {
    NativePlaylistEditorScreen *screen;
    NcmPlaylist playlist;
    NcmBuffer question;
    NcmBuffer message;
    NcmError error;
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    ncm_error_clear(&error);
    if (main_playlist) {
        if (!native_playlist_screen_empty(native_c_screen_playlist())
            && Config.ask_before_clearing_playlists
            && !action_runtime_confirm(
                STRLIT_ARGS("Do you really want to clear main playlist?"))) {
            return true;
        }
        if (!ncm_mpd_client_clear_queue(&global_mpd, &error)) {
            return action_runtime_mpd_error(&error);
        }
        native_playlist_screen_clear(native_c_screen_playlist());
        (void)ncm_status_update_full(&global_mpd, NULL, &error);
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    (char *)"Playlist cleared");
        return true;
    }

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return false;
    }
    if (!action_runtime_playlist_editor_has_playlists()) {
        return false;
    }

    screen = native_c_screen_playlist_editor();
    ncm_playlist_init(&playlist);
    success = native_playlist_editor_screen_current_playlist(screen,
                                                            &playlist);
    if (!success) {
        ncm_playlist_destroy(&playlist);
        return false;
    }

    if (Config.ask_before_clearing_playlists) {
        ncm_buffer_init(&question);
        ncm_buffer_append(&question, STRLIT_ARGS(
            "Do you really want to clear playlist \""));
        ncm_buffer_append(&question, playlist.path, playlist.path_len);
        ncm_buffer_append(&question, STRLIT_ARGS("\"?"));
        success = action_runtime_confirm(question.data, question.len);
        ncm_buffer_destroy(&question);
        if (!success) {
            ncm_playlist_destroy(&playlist);
            return true;
        }
    }

    success = ncm_mpd_client_clear_playlist(&global_mpd,
                                            playlist.path, &error);
    if (success) {
        ncm_buffer_init(&message);
        ncm_buffer_append(&message, STRLIT_ARGS("Playlist \""));
        ncm_buffer_append(&message, playlist.path, playlist.path_len);
        ncm_buffer_append(&message, STRLIT_ARGS("\" cleared"));
        ncm_statusbar_print((int32)Config.message_delay_time,
                            message.data, message.len);
        ncm_buffer_destroy(&message);
    }
    ncm_playlist_destroy(&playlist);
    if (!success) {
        return action_runtime_mpd_error(&error);
    }
    native_playlist_editor_screen_request_content_update(screen);
    return true;
}

bool
ncm_action_crop_main_playlist(void) {
    return action_runtime_crop_playlist(true);
}

static bool
action_runtime_crop_playlist(bool main_playlist) {
    NativePlaylistEditorScreen *editor;
    NcmPlaylist playlist;
    NcmSongArray songs;
    NcmBuffer question;
    NcmBuffer message;
    NcmError error;
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    ncm_song_array_init(&songs);
    success = false;
    if (main_playlist) {
        if (native_playlist_screen_song_count(
                native_c_screen_playlist()) <= 1) {
            ncm_song_array_destroy(&songs);
            return true;
        }
        if (Config.ask_before_clearing_playlists
            && !action_runtime_confirm(
                STRLIT_ARGS("Do you really want to crop main playlist?"))) {
            ncm_song_array_destroy(&songs);
            return true;
        }
        success = native_playlist_screen_selected_songs(
            native_c_screen_playlist(), &songs);
    } else if (action_runtime_current_screen_is(
                   NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        if (!action_runtime_playlist_editor_has_playlists()) {
            ncm_song_array_destroy(&songs);
            return false;
        }
        if (action_runtime_playlist_editor_has_content()
            && (nc_menu_all_item_count(nc_song_menu_base(
                    native_playlist_editor_screen_content(
                        native_c_screen_playlist_editor()))) <= 1)) {
            ncm_song_array_destroy(&songs);
            return true;
        }
        success = native_playlist_editor_screen_selected_songs(
            native_c_screen_playlist_editor(), &songs);
    }
    if (!success || (songs.len <= 0)) {
        ncm_song_array_destroy(&songs);
        return false;
    }

    ncm_error_clear(&error);
    if (main_playlist) {
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    (char *)"Cropping playlist...");
        if (!ncm_mpd_client_clear_queue(&global_mpd, &error)) {
            ncm_song_array_destroy(&songs);
            return action_runtime_mpd_error(&error);
        }
        for (int32 i = 0; i < songs.len; i += 1) {
            if (!ncm_mpd_client_add_song_value(&global_mpd,
                                               &songs.items[i], -1,
                                               NULL, &error)) {
                ncm_song_array_destroy(&songs);
                return action_runtime_mpd_error(&error);
            }
        }
        (void)ncm_status_update_full(&global_mpd, NULL, &error);
        ncm_song_array_destroy(&songs);
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    (char *)"Playlist cropped");
        return true;
    }

    editor = native_c_screen_playlist_editor();
    ncm_playlist_init(&playlist);
    success = native_playlist_editor_screen_current_playlist(editor,
                                                            &playlist);
    if (success && Config.ask_before_clearing_playlists) {
        ncm_buffer_init(&question);
        ncm_buffer_append(&question, STRLIT_ARGS(
            "Do you really want to crop playlist \""));
        ncm_buffer_append(&question, playlist.path, playlist.path_len);
        ncm_buffer_append(&question, STRLIT_ARGS("\"?"));
        success = action_runtime_confirm(question.data, question.len);
        ncm_buffer_destroy(&question);
        if (!success) {
            ncm_playlist_destroy(&playlist);
            ncm_song_array_destroy(&songs);
            return true;
        }
    }
    if (success) {
        ncm_buffer_init(&message);
        ncm_buffer_append(&message, STRLIT_ARGS("Cropping playlist \""));
        ncm_buffer_append(&message, playlist.path, playlist.path_len);
        ncm_buffer_append(&message, STRLIT_ARGS("\"..."));
        ncm_statusbar_print((int32)Config.message_delay_time,
                            message.data, message.len);
        ncm_buffer_destroy(&message);
        success = ncm_mpd_client_clear_playlist(&global_mpd,
                                                playlist.path, &error);
    }
    for (int32 i = 0; success && (i < songs.len); i += 1) {
        success = ncm_mpd_client_add_song_to_playlist(&global_mpd,
                                                      playlist.path,
                                                      &songs.items[i],
                                                      &error);
    }
    if (success) {
        ncm_buffer_init(&message);
        ncm_buffer_append(&message, STRLIT_ARGS("Playlist \""));
        ncm_buffer_append(&message, playlist.path, playlist.path_len);
        ncm_buffer_append(&message, STRLIT_ARGS("\" cropped"));
        ncm_statusbar_print((int32)Config.message_delay_time,
                            message.data, message.len);
        ncm_buffer_destroy(&message);
    }
    ncm_playlist_destroy(&playlist);
    ncm_song_array_destroy(&songs);
    if (!success) {
        return action_runtime_mpd_error(&error);
    }
    native_playlist_editor_screen_request_content_update(editor);
    return true;
}

static bool
action_runtime_move_main_playlist_items(NcmSongArray *songs,
                                        bool down) {
    NcmError error;
    uint32 *positions;
    int32 count;
    bool success;

    if (!action_runtime_song_positions(songs, &positions, &count)) {
        return false;
    }

    action_runtime_sort_positions(positions, count, down);
    ncm_error_clear(&error);
    success = ncm_mpd_client_start_command_list(&global_mpd, &error);
    for (int32 i = 0; success && (i < count); i += 1) {
        if (down) {
            if (positions[i] + 1 >= ncm_status_state_playlist_length()) {
                continue;
            }
            success = ncm_mpd_client_swap(
                &global_mpd, positions[i], positions[i] + 1, &error);
        } else {
            if (positions[i] == 0) {
                continue;
            }
            success = ncm_mpd_client_swap(
                &global_mpd, positions[i], positions[i] - 1, &error);
        }
    }
    if (success) {
        success = ncm_mpd_client_commit_command_list(&global_mpd, &error);
    }
    if (!success && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }

    ncm_free(positions, (uint64)count*SIZEOF(*positions));
    if (!success) {
        return action_runtime_mpd_error(&error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &error);
    return true;
}

static bool
action_runtime_move_stored_playlist_items(NcmSongArray *songs,
                                          bool down) {
    NativePlaylistEditorScreen *screen;
    NcmPlaylist playlist;
    NcmError error;
    uint32 *positions;
    int64 item_count;
    int32 count;
    bool success;

    if (!action_runtime_song_positions(songs, &positions, &count)) {
        return false;
    }

    screen = native_c_screen_playlist_editor();
    ncm_playlist_init(&playlist);
    success = native_playlist_editor_screen_current_playlist(screen,
                                                            &playlist);
    if (!success) {
        ncm_free(positions, (uint64)count*SIZEOF(*positions));
        ncm_playlist_destroy(&playlist);
        return false;
    }

    action_runtime_sort_positions(positions, count, down);
    item_count = nc_menu_all_item_count(
        native_playlist_editor_screen_active_menu(screen));
    ncm_error_clear(&error);
    success = ncm_mpd_client_start_command_list(&global_mpd, &error);
    for (int32 i = 0; success && (i < count); i += 1) {
        if (down) {
            if ((int64)positions[i] + 1 >= item_count) {
                continue;
            }
            success = ncm_mpd_client_playlist_move(
                &global_mpd, playlist.path, positions[i],
                positions[i] + 1, &error);
        } else if (positions[i] > 0) {
            success = ncm_mpd_client_playlist_move(
                &global_mpd, playlist.path, positions[i],
                positions[i] - 1, &error);
        }
    }
    if (success) {
        success = ncm_mpd_client_commit_command_list(&global_mpd, &error);
    }
    if (!success && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }

    ncm_free(positions, (uint64)count*SIZEOF(*positions));
    ncm_playlist_destroy(&playlist);
    if (!success) {
        return action_runtime_mpd_error(&error);
    }
    native_playlist_editor_screen_request_content_update(screen);
    return true;
}

static bool
action_runtime_move_selected_items(bool down) {
    NcmSongArray songs;
    NcMenu *menu;
    enum ScreenType screen_type;
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    screen_type = native_c_screens_current_type();
    if ((screen_type != NCM_SCREEN_TYPE_PLAYLIST)
        && (screen_type != NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return false;
    }
    menu = action_runtime_current_menu();
    if ((menu != NULL) && nc_menu_is_filtered(menu)) {
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time,
            (char *)"Moving items is disabled in filtered playlist");
        return true;
    }

    ncm_song_array_init(&songs);
    success = action_runtime_selected_songs(&songs) && (songs.len > 0);
    if (!success) {
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
    NativePlaylistScreen *screen;
    NcMenu *menu;
    NcmSong *song;
    NcmError error;
    uint32 *positions;
    uint32 target;
    uint32 destination;
    int64 item_count;
    int32 count;
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
        return false;
    }

    screen = native_c_screen_playlist();
    menu = native_playlist_screen_menu(screen);
    if ((menu == NULL) || !nc_menu_has_selected(menu)) {
        return false;
    }
    song = nc_menu_active_item_at(menu, nc_menu_highlight(menu));
    if (song == NULL) {
        return false;
    }
    target = ncm_song_position(song);

    item_count = nc_menu_all_item_count(menu);
    positions = ncm_malloc((uint64)item_count*SIZEOF(*positions));
    if (positions == NULL) {
        return false;
    }
    count = 0;
    for (int64 i = 0; i < item_count; i += 1) {
        uint32 flags;

        flags = nc_menu_item_flags_at(menu, NC_MENU_ITEMS_ALL, i);
        if (!(flags & NC_MENU_ITEM_SELECTED)) {
            continue;
        }
        song = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, i);
        if (song == NULL) {
            ncm_free(positions, (uint64)item_count*SIZEOF(*positions));
            return false;
        }
        positions[count++] = ncm_song_position(song);
    }
    if (count <= 0) {
        ncm_free(positions, (uint64)item_count*SIZEOF(*positions));
        return false;
    }

    action_runtime_sort_positions(positions, count, false);
    if ((target >= positions[0]) && (target <= positions[count - 1])) {
        ncm_free(positions, (uint64)item_count*SIZEOF(*positions));
        return true;
    }

    ncm_error_clear(&error);
    success = ncm_mpd_client_start_command_list(&global_mpd, &error);
    if (success && (target > positions[0])) {
        destination = target - (uint32)count;
        for (int32 i = count; success && (i > 0); i -= 1) {
            success = ncm_mpd_client_move(
                &global_mpd, positions[i - 1],
                destination + (uint32)i - 1, &error);
        }
    } else if (success) {
        destination = target;
        for (int32 i = 0; success && (i < count); i += 1) {
            success = ncm_mpd_client_move(
                &global_mpd, positions[i], destination + (uint32)i,
                &error);
        }
    }
    if (success) {
        success = ncm_mpd_client_commit_command_list(&global_mpd, &error);
    }
    if (!success && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }
    ncm_free(positions, (uint64)item_count*SIZEOF(*positions));
    if (!success) {
        return action_runtime_mpd_error(&error);
    }

    (void)ncm_status_update_full(&global_mpd, NULL, &error);
    return true;
}

static bool
action_runtime_move_playlist_editor_items_to(void) {
    NativePlaylistEditorScreen *screen;
    NcmPlaylist playlist;
    NcMenu *menu;
    NcmSong *song;
    NcmError error;
    uint32 *positions;
    uint32 target;
    uint32 destination;
    int64 item_count;
    int32 count;
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (!action_runtime_playlist_editor_content_active()) {
        return false;
    }

    screen = native_c_screen_playlist_editor();
    menu = nc_song_menu_base(native_playlist_editor_screen_content(screen));
    if ((menu == NULL) || !nc_menu_has_selected(menu)) {
        return false;
    }
    if (nc_menu_is_filtered(menu)) {
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time,
            (char *)"Moving items is disabled in filtered playlist");
        return true;
    }

    song = nc_menu_active_item_at(menu, nc_menu_highlight(menu));
    if (song == NULL) {
        return false;
    }
    target = ncm_song_position(song);

    item_count = nc_menu_all_item_count(menu);
    positions = ncm_malloc((uint64)item_count*SIZEOF(*positions));
    if (positions == NULL) {
        return false;
    }
    count = 0;
    for (int64 i = 0; i < item_count; i += 1) {
        uint32 flags;

        flags = nc_menu_item_flags_at(menu, NC_MENU_ITEMS_ALL, i);
        if (!(flags & NC_MENU_ITEM_SELECTED)) {
            continue;
        }
        song = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, i);
        if (song == NULL) {
            ncm_free(positions, (uint64)item_count*SIZEOF(*positions));
            return false;
        }
        positions[count++] = ncm_song_position(song);
    }
    if (count <= 0) {
        ncm_free(positions, (uint64)item_count*SIZEOF(*positions));
        return false;
    }

    ncm_playlist_init(&playlist);
    success = native_playlist_editor_screen_current_playlist(screen,
                                                            &playlist);
    if (!success) {
        ncm_playlist_destroy(&playlist);
        ncm_free(positions, (uint64)item_count*SIZEOF(*positions));
        return false;
    }

    action_runtime_sort_positions(positions, count, false);
    if ((target >= positions[0]) && (target <= positions[count - 1])) {
        ncm_playlist_destroy(&playlist);
        ncm_free(positions, (uint64)item_count*SIZEOF(*positions));
        return true;
    }

    ncm_error_clear(&error);
    success = ncm_mpd_client_start_command_list(&global_mpd, &error);
    if (success && (target > positions[0])) {
        destination = target - (uint32)count;
        for (int32 i = count; success && (i > 0); i -= 1) {
            success = ncm_mpd_client_playlist_move(
                &global_mpd, playlist.path, positions[i - 1],
                destination + (uint32)i - 1, &error);
        }
    } else if (success) {
        destination = target;
        for (int32 i = 0; success && (i < count); i += 1) {
            success = ncm_mpd_client_playlist_move(
                &global_mpd, playlist.path, positions[i],
                destination + (uint32)i, &error);
        }
    }
    if (success) {
        success = ncm_mpd_client_commit_command_list(&global_mpd, &error);
    }
    if (!success && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }

    ncm_playlist_destroy(&playlist);
    ncm_free(positions, (uint64)item_count*SIZEOF(*positions));
    if (!success) {
        return action_runtime_mpd_error(&error);
    }
    native_playlist_editor_screen_request_content_update(screen);
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
action_runtime_playlist_range(NcMenu *menu, uint32 *first,
                              uint32 *last) {
    enum NcMenuItemSource source;
    int64 range_first;
    int64 range_last;
    NcmSong *song;

    if ((menu == NULL) || (first == NULL) || (last == NULL)) {
        return false;
    }
    source = action_runtime_menu_item_source(menu);
    if (!ncm_menu_find_full_selected_range(menu, source,
                                           &range_first, &range_last)) {
        return false;
    }
    if (range_first >= range_last) {
        return false;
    }

    song = nc_menu_active_item_at(menu, range_first);
    if (song == NULL) {
        return false;
    }
    *first = ncm_song_position(song);
    song = nc_menu_active_item_at(menu, range_last - 1);
    if (song == NULL) {
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
    NcmError error;
    int64 first;
    int64 last;
    bool success;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
        return false;
    }

    menu = action_runtime_current_menu();
    source = action_runtime_menu_item_source(menu);
    if (!ncm_menu_find_full_selected_range(menu, source,
                                           &first, &last)) {
        return false;
    }
    if (first >= last) {
        return false;
    }

    last -= 1;
    ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                (char *)"Reversing range...");
    ncm_error_clear(&error);
    success = ncm_mpd_client_start_command_list(&global_mpd, &error);
    while (success && (first < last)) {
        left = nc_menu_active_item_at(menu, first);
        right = nc_menu_active_item_at(menu, last);
        if ((left == NULL) || (right == NULL)) {
            success = false;
            break;
        }
        success = ncm_mpd_client_swap(
            &global_mpd, ncm_song_position(left),
            ncm_song_position(right), &error);
        first += 1;
        last -= 1;
    }
    if (success) {
        success = ncm_mpd_client_commit_command_list(&global_mpd, &error);
    }
    if (!success && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }
    if (!success) {
        return action_runtime_mpd_error(&error);
    }

    (void)ncm_status_update_full(&global_mpd, NULL, &error);
    ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                (char *)"Range reversed");
    return true;
}

static bool
action_runtime_shuffle_playlist(void) {
    NcMenu *menu;
    NcmError error;
    uint32 first;
    uint32 last;

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
            STRLIT_ARGS("Do you really want to shuffle selected range?"))) {
        return true;
    }

    ncm_error_clear(&error);
    if (!ncm_mpd_client_shuffle_range(&global_mpd, first, last,
                                      &error)) {
        return action_runtime_mpd_error(&error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &error);
    ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                (char *)"Range shuffled");
    return true;
}

static bool
action_runtime_set_selected_items_priority(void) {
    NcmBuffer input;
    NcmError error;
    uint32 priority;
    bool prompted;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }
    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
        return false;
    }
    if (ncm_mpd_client_version(&global_mpd) < 17) {
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time,
            (char *)"Priorities are supported in MPD >= 0.17.0");
        return false;
    }

    ncm_buffer_init(&input);
    prompted = action_runtime_prompt_string(
        STRLIT_ARGS("Set priority [0-255]: "), (char *)"", false,
        NULL, NULL, &input);
    if (!prompted) {
        ncm_buffer_destroy(&input);
        return true;
    }

    ncm_error_clear(&error);
    if (!ncm_parse_uint32(input.data, input.len, &priority, &error)
        || (priority > 255)) {
        ncm_buffer_destroy(&input);
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time,
            (char *)"Priority must be between 0 and 255");
        return true;
    }
    ncm_buffer_destroy(&input);

    ncm_error_clear(&error);
    if (!native_playlist_screen_set_selected_priority(
            native_c_screen_playlist(), &global_mpd, (int32)priority,
            &error)) {
        return action_runtime_mpd_error(&error);
    }
    ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                (char *)"Priority set");
    return true;
}

static bool
action_runtime_jump_to_position_in_song(void) {
    NcmBuffer input;
    NcmError error;
    int32 song_position;
    uint32 total;
    uint32 target;
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

    ncm_buffer_init(&input);
    prompted = action_runtime_prompt_string(
        STRLIT_ARGS("Position to go (in %/h:m:ss/m:ss/seconds(s)): "),
        (char *)"", false, NULL, NULL, &input);
    if (!prompted) {
        ncm_buffer_destroy(&input);
        return true;
    }
    if (!action_runtime_parse_seek_position(input.data, input.len,
                                            total, &target)) {
        ncm_buffer_destroy(&input);
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time,
            (char *)"Invalid format ([h]:[mm]:[ss], [m]:[ss], "
                    "[s]s, [%]%, [%] accepted)");
        return true;
    }
    ncm_buffer_destroy(&input);

    ncm_error_clear(&error);
    if (!ncm_mpd_client_seek_pos(&global_mpd, (uint32)song_position,
                                 target, &error)) {
        return action_runtime_mpd_error(&error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &error);
    return true;
}

static bool
action_runtime_select_album(void) {
    NativePlaylistScreen *screen;
    NcmBuffer album;
    NcmBuffer candidate;
    NcmSong *song;
    NcMenu *menu;
    int64 current;
    int64 count;
    int64 position;
    bool equal;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
        return false;
    }

    screen = native_c_screen_playlist();
    menu = native_playlist_screen_menu(screen);
    if ((menu == NULL) || nc_menu_empty(menu)) {
        return false;
    }

    current = nc_menu_highlight(menu);
    song = nc_menu_active_item_at(menu, current);
    if (song == NULL) {
        return false;
    }

    album = ncm_song_tags_buffer(
        song, NCM_SONG_GETTER_ALBUM, Config.tags_separator,
        Config.tags_separator_len, Config.show_duplicate_tags);
    for (position = current; position >= 0; position -= 1) {
        song = nc_menu_active_item_at(menu, position);
        if (song == NULL) {
            break;
        }
        candidate = ncm_song_tags_buffer(
            song, NCM_SONG_GETTER_ALBUM, Config.tags_separator,
            Config.tags_separator_len, Config.show_duplicate_tags);
        equal = ncm_string_equal(album.data, album.len,
                                 candidate.data, candidate.len);
        ncm_buffer_destroy(&candidate);
        if (!equal) {
            break;
        }
        (void)nc_menu_set_position_selected(menu, position, true);
    }

    count = nc_menu_item_count(menu);
    for (position = current + 1; position < count; position += 1) {
        song = nc_menu_active_item_at(menu, position);
        if (song == NULL) {
            break;
        }
        candidate = ncm_song_tags_buffer(
            song, NCM_SONG_GETTER_ALBUM, Config.tags_separator,
            Config.tags_separator_len, Config.show_duplicate_tags);
        equal = ncm_string_equal(album.data, album.len,
                                 candidate.data, candidate.len);
        ncm_buffer_destroy(&candidate);
        if (!equal) {
            break;
        }
        (void)nc_menu_set_position_selected(menu, position, true);
    }
    ncm_buffer_destroy(&album);

    ncm_statusbar_print_cstring(
        (int32)Config.message_delay_time,
        (char *)"Album around cursor position selected");
    return true;
}

static bool
action_runtime_select_found_items(void) {
    NativePlaylistScreen *screen;
    NcmStringView constraint;
    NcMenu *menu;
    NcmError error;
    int64 original;
    int64 height;
    bool found;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
        return false;
    }
    constraint = current_screen_current_search_constraint();
    if ((constraint.data == NULL) || (constraint.len <= 0)) {
        return false;
    }

    screen = native_c_screen_playlist();
    menu = native_playlist_screen_menu(screen);
    if ((menu == NULL) || nc_menu_empty(menu)) {
        return false;
    }

    original = nc_menu_highlight(menu);
    height = nc_playlist_screen_height(
        native_playlist_screen_playlist(screen));
    nc_menu_highlight_position(menu, 0, height);
    ncm_error_clear(&error);
    found = current_screen_search(NCM_SEARCH_DIRECTION_FORWARD,
                                  constraint.data, constraint.len,
                                  false, false, &error);
    if (ncm_error_is_set(&error)) {
        nc_menu_highlight_position(menu, original, height);
        return action_runtime_mpd_error(&error);
    }

    if (found) {
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    (char *)"Searching for items...");
        (void)nc_menu_set_current_selected(menu, true);
        while (true) {
            found = current_screen_search(
                NCM_SEARCH_DIRECTION_FORWARD, constraint.data,
                constraint.len, false, true, &error);
            if (!found) {
                break;
            }
            (void)nc_menu_set_current_selected(menu, true);
        }
        if (ncm_error_is_set(&error)) {
            nc_menu_highlight_position(menu, original, height);
            return action_runtime_mpd_error(&error);
        }
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    (char *)"Found items selected");
    }
    nc_menu_highlight_position(menu, original, height);
    return true;
}

static bool
action_runtime_previous_column_available(void) {
    switch (native_c_screens_current_type()) {
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        return native_media_library_screen_previous_column_available(
            native_c_screen_media_library());
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        return native_playlist_editor_screen_previous_column_available(
            native_c_screen_playlist_editor());
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        return native_tag_editor_screen_previous_column_available(
            native_c_screen_tag_editor());
#endif
    default:
        break;
    }
    return false;
}

static bool
action_runtime_next_column_available(void) {
    switch (native_c_screens_current_type()) {
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        return native_media_library_screen_next_column_available(
            native_c_screen_media_library());
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        return native_playlist_editor_screen_next_column_available(
            native_c_screen_playlist_editor());
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        return native_tag_editor_screen_next_column_available(
            native_c_screen_tag_editor());
#endif
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
    switch (native_c_screens_current_type()) {
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        native_media_library_screen_previous_column(
            native_c_screen_media_library());
        return true;
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        native_playlist_editor_screen_previous_column(
            native_c_screen_playlist_editor());
        return true;
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        native_tag_editor_screen_previous_column(native_c_screen_tag_editor());
        return true;
#endif
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
    switch (native_c_screens_current_type()) {
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        native_media_library_screen_next_column(
            native_c_screen_media_library());
        return true;
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        native_playlist_editor_screen_next_column(
            native_c_screen_playlist_editor());
        return true;
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        native_tag_editor_screen_next_column(native_c_screen_tag_editor());
        return true;
#endif
    default:
        break;
    }
    return false;
}

static bool
action_runtime_enter_directory(void) {
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        return native_browser_screen_enter_directory(
            native_c_screen_browser());
    }
#if defined(HAVE_TAGLIB_H)
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TAG_EDITOR)) {
        return native_tag_editor_screen_enter_directory(
            native_c_screen_tag_editor());
    }
#endif
    return false;
}

static bool
action_runtime_jump_to_parent_directory(void) {
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        return native_browser_screen_go_to_parent(native_c_screen_browser());
    }
#if defined(HAVE_TAGLIB_H)
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TAG_EDITOR)) {
        return native_tag_editor_screen_go_to_parent(
            native_c_screen_tag_editor());
    }
#endif
    return false;
}

static bool
action_runtime_seek_relative(bool forward) {
    NcmError error;
    int32 position;
    uint32 elapsed;
    uint32 total;
    uint32 target;

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

    ncm_error_clear(&error);
    if (!ncm_mpd_client_seek_pos(&global_mpd, (uint32)position,
                                 target, &error)) {
        return action_runtime_mpd_error(&error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &error);
    return true;
}

static bool
action_runtime_jump_to_playing_song(void) {
    NcmSong song;
    NcmError error;
    int32 position;
    bool success;

    position = ncm_status_state_current_song_position();
    if (position < 0) {
        return false;
    }

    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
        success = native_playlist_screen_locate_position(
            native_c_screen_playlist(), (uint32)position);
        if (!success) {
            ncm_statusbar_print_cstring(
                (int32)Config.message_delay_time,
                (char *)"Song is filtered out");
        }
        return true;
    }
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY)) {
        ncm_song_init(&song);
        ncm_error_clear(&error);
        success = ncm_mpd_client_get_current_song(&global_mpd,
                                                  &song, &error);
        if (success) {
            success = native_media_library_screen_locate_song(
                native_c_screen_media_library(), &song, &error);
        }
        ncm_song_destroy(&song);
        if (!success) {
            return action_runtime_mpd_error(&error);
        }
        return true;
    }
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        return action_runtime_jump_to_browser();
    }
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
        ncm_song_init(&song);
        ncm_error_clear(&error);
        success = ncm_mpd_client_get_current_song(&global_mpd,
                                                  &song, &error);
        if (success) {
            success = native_playlist_editor_screen_locate_song(
                native_c_screen_playlist_editor(), &global_mpd,
                &song, &error);
        }
        ncm_song_destroy(&song);
        if (!success && ncm_error_is_set(&error)) {
            return action_runtime_mpd_error(&error);
        }
        return true;
    }
    return false;
}

static bool
action_runtime_jump_to_browser(void) {
    NcmSong song;
    NcmError error;
    bool success;

    ncm_song_init(&song);
    success = action_runtime_current_song(&song);
    if (!success) {
        ncm_song_destroy(&song);
        return false;
    }

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        success = action_runtime_switch_to_screen(NCM_SCREEN_TYPE_BROWSER);
    }
    if (success) {
        ncm_error_clear(&error);
        success = native_browser_screen_locate_song(
            native_c_screen_browser(), &song, &global_mpd, &error);
        if (!success) {
            (void)action_runtime_mpd_error(&error);
        }
    }

    ncm_song_destroy(&song);
    return success;
}

static bool
action_runtime_jump_to_playlist_editor(void) {
    NativeBrowserScreen *browser;
    NcmStringView path;
    NcmError error;
    bool success;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        return action_runtime_switch_to_screen(
            NCM_SCREEN_TYPE_PLAYLIST_EDITOR);
    }

    browser = native_c_screen_browser();
    if (!native_browser_screen_current_playlist_path(browser, &path)) {
        return false;
    }

    success = action_runtime_switch_to_screen(
        NCM_SCREEN_TYPE_PLAYLIST_EDITOR);
    if (!success) {
        return false;
    }

    ncm_error_clear(&error);
    success = native_playlist_editor_screen_locate_playlist(
        native_c_screen_playlist_editor(), &global_mpd, path.data,
        path.len, &error);
    if (!success) {
        return action_runtime_mpd_error(&error);
    }
    return true;
}

static bool
action_runtime_jump_to_media_library(void) {
    NcmSong song;
    NcmError error;
    bool success;

    ncm_song_init(&song);
    success = action_runtime_current_song(&song);
    if (!success) {
        ncm_song_destroy(&song);
        return false;
    }

    success = action_runtime_switch_to_screen(
        NCM_SCREEN_TYPE_MEDIA_LIBRARY);
    if (success) {
        ncm_statusbar_print_cstring(
            0, (char *)"Jumping to song...");
        ncm_error_clear(&error);
        success = native_media_library_screen_locate_song(
            native_c_screen_media_library(), &song, &error);
        if (!success) {
            (void)action_runtime_mpd_error(&error);
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
            (int32)Config.message_delay_time,
            (char *)"Proper mpd_music_dir variable has to be set in "
                    "configuration file");
        return false;
    }

    ncm_song_init(&song);
    success = action_runtime_current_song(&song);
    if (success) {
        success = ncm_song_directory_view(&song, 0, &directory)
                  && (directory.len > 0);
    }
    if (success) {
        success = action_runtime_switch_to_screen(
            NCM_SCREEN_TYPE_TAG_EDITOR);
    }
    if (success) {
        success = native_tag_editor_screen_locate_song(
            native_c_screen_tag_editor(), &song);
    }
    ncm_song_destroy(&song);
    return success;
#else
    return false;
#endif
}

static bool
action_runtime_edit_directory_name(void) {
    NativeBrowserScreen *browser;
    NcmStringView path;
    NcmBuffer name;
    NcmError error;
    bool prompted;
    bool success;

    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        browser = native_c_screen_browser();
        if (!native_browser_screen_current_directory_path(browser,
                                                          &path)) {
            return false;
        }

        ncm_buffer_init(&name);
        prompted = action_runtime_prompt_string(
            STRLIT_ARGS("Directory: "), path.data, false,
            NULL, NULL, &name);
        if (!prompted) {
            ncm_buffer_destroy(&name);
            return true;
        }
        if ((name.len <= 0)
            || ncm_string_equal(name.data, name.len,
                                path.data, path.len)) {
            ncm_buffer_destroy(&name);
            return true;
        }

        ncm_error_clear(&error);
        success = native_browser_screen_rename_current_directory(
            browser, name.data, name.len, &global_mpd, &error);
        if (success) {
            action_runtime_print_renamed(
                STRLIT_ARGS("Directory renamed to \""), &name);
        }
        ncm_buffer_destroy(&name);
        if (!success) {
            return action_runtime_mpd_error(&error);
        }
        return true;
    }

#if defined(HAVE_TAGLIB_H)
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TAG_EDITOR)) {
        return native_tag_editor_screen_rename_current_directory(
            native_c_screen_tag_editor(), Config.mpd_music_dir,
            Config.mpd_music_dir_len);
    }
#endif
    return false;
}

static bool
action_runtime_edit_playlist_name(void) {
    NativeBrowserScreen *browser;
    NativePlaylistEditorScreen *screen;
    NcmPlaylist playlist;
    NcmStringView path;
    NcmBuffer name;
    NcmError error;
    bool prompted;
    bool success;

    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        if (!ncm_mpd_client_connected(&global_mpd)) {
            return false;
        }
        browser = native_c_screen_browser();
        if (!native_browser_screen_current_playlist_path(browser,
                                                         &path)) {
            return false;
        }

        ncm_buffer_init(&name);
        prompted = action_runtime_prompt_string(
            STRLIT_ARGS("Playlist: "), path.data, false,
            NULL, NULL, &name);
        if (!prompted) {
            ncm_buffer_destroy(&name);
            return true;
        }
        if ((name.len <= 0)
            || ncm_string_equal(name.data, name.len,
                                path.data, path.len)) {
            ncm_buffer_destroy(&name);
            return true;
        }

        ncm_error_clear(&error);
        success = native_browser_screen_rename_current_playlist(
            browser, name.data, name.len, &global_mpd, &error);
        if (success) {
            action_runtime_print_renamed(
                STRLIT_ARGS("Playlist renamed to \""), &name);
        }
        ncm_buffer_destroy(&name);
        if (!success) {
            return action_runtime_mpd_error(&error);
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

    screen = native_c_screen_playlist_editor();
    ncm_playlist_init(&playlist);
    success = native_playlist_editor_screen_current_playlist(screen,
                                                            &playlist);
    if (!success) {
        ncm_playlist_destroy(&playlist);
        return false;
    }

    ncm_buffer_init(&name);
    prompted = action_runtime_prompt_string(STRLIT_ARGS("Playlist: "),
                                            playlist.path, false,
                                            NULL, NULL, &name);
    if (!prompted) {
        ncm_buffer_destroy(&name);
        ncm_playlist_destroy(&playlist);
        return true;
    }
    if ((name.len <= 0)
        || ncm_string_equal(name.data, name.len,
                            playlist.path, playlist.path_len)) {
        ncm_buffer_destroy(&name);
        ncm_playlist_destroy(&playlist);
        return true;
    }

    ncm_error_clear(&error);
    success = ncm_mpd_client_rename_playlist(&global_mpd,
                                             playlist.path,
                                             name.data, &error);
    if (success) {
        action_runtime_print_renamed(
            STRLIT_ARGS("Playlist renamed to \""), &name);
        native_playlist_editor_screen_request_playlists_update(screen);
    }
    ncm_buffer_destroy(&name);
    ncm_playlist_destroy(&playlist);
    if (!success) {
        return action_runtime_mpd_error(&error);
    }
    return true;
}

static bool
action_runtime_toggle_display_mode(void) {
    NcmStringFormatArg arg;
    enum DisplayMode *mode;
    enum ScreenType screen_type;

    if (action_runtime_current_screen_is(
            NCM_SCREEN_TYPE_SEARCH_ENGINE)) {
        NativeSearchEngineScreen *screen;
        NcmStringFormatArg arg;
        enum DisplayMode search_mode;

        screen = native_c_screen_search_engine();
        search_mode = native_search_engine_screen_toggle_display_mode(
            screen);
        arg = ncm_string_format_arg_cstring(
            ncm_display_mode_str(search_mode));
        ncm_statusbar_format(
            (int32)Config.message_delay_time,
            STRLIT_ARGS("Search engine display mode: %1%"),
            &arg, 1);
        app_controller_request_current_screen_resize();
        app_controller_refresh_current_screen();
        return true;
    }

    mode = NULL;
    screen_type = native_c_screens_current_type();
    switch (screen_type) {
    case NCM_SCREEN_TYPE_BROWSER:
        mode = &Config.browser_display_mode;
        break;
    case NCM_SCREEN_TYPE_PLAYLIST:
        mode = &Config.playlist_display_mode;
        break;
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        mode = &Config.playlist_editor_display_mode;
        break;
    default:
        break;
    }
    if (mode == NULL) {
        return false;
    }

    if (*mode == NCM_DISPLAY_MODE_CLASSIC) {
        *mode = NCM_DISPLAY_MODE_COLUMNS;
    } else {
        *mode = NCM_DISPLAY_MODE_CLASSIC;
    }
    if (screen_type == NCM_SCREEN_TYPE_BROWSER) {
        native_browser_screen_set_display_mode(native_c_screen_browser(),
                                               *mode);
    }
    if (screen_type == NCM_SCREEN_TYPE_PLAYLIST) {
        native_playlist_screen_update_column_title(
            native_c_screen_playlist());
    }
    app_controller_request_current_screen_resize();
    if (screen_type == NCM_SCREEN_TYPE_PLAYLIST) {
        app_controller_refresh_current_screen();
        arg = ncm_string_format_arg_cstring(
            ncm_display_mode_str(*mode));
        ncm_statusbar_format(
            (int32)Config.message_delay_time,
            STRLIT_ARGS("Playlist display mode: %1%"), &arg, 1);
    }
    return true;
}

static bool
action_runtime_change_browse_mode(void) {
    NativeBrowserScreen *browser;
    NcmError error;
    char *message;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        return false;
    }

    browser = native_c_screen_browser();
    ncm_error_clear(&error);
    if (!native_browser_screen_change_browse_mode(browser, &global_mpd,
                                                  &error)) {
        if (error.code == EINVAL) {
            ncm_statusbar_print_cstring(
                (int32)Config.message_delay_time,
                (char *)"For browsing local filesystem connection to MPD "
                "via UNIX Socket is required");
        } else if (ncm_error_is_set(&error)) {
            ncm_statusbar_print_cstring(
                (int32)Config.message_delay_time, error.message);
        }
        return false;
    }

    if (native_browser_screen_is_local(browser)) {
        message = (char *)"Browse mode: local filesystem";
    } else {
        message = (char *)"Browse mode: MPD database";
    }
    ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                message);
    return true;
}

static bool
action_runtime_toggle_browser_sort_mode(void) {
    char *message;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
        return false;
    }
    Config.browser_sort_mode += 1;
    if (Config.browser_sort_mode >= NCM_SORT_MODE_LAST) {
        Config.browser_sort_mode = NCM_SORT_MODE_TYPE;
    }

    switch (Config.browser_sort_mode) {
    case NCM_SORT_MODE_TYPE:
        message = (char *)"Sort songs by: type";
        break;
    case NCM_SORT_MODE_NAME:
        message = (char *)"Sort songs by: name";
        break;
    case NCM_SORT_MODE_MODIFICATION_TIME:
        message = (char *)"Sort songs by: modification time";
        break;
    case NCM_SORT_MODE_CUSTOM_FORMAT:
        message = (char *)"Sort songs by: custom format";
        break;
    case NCM_SORT_MODE_NONE:
        message = (char *)"Do not sort songs";
        break;
    case NCM_SORT_MODE_LAST:
        message = (char *)"Sort songs by: type";
        break;
    }
    ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                message);
    (void)native_browser_screen_sort(native_c_screen_browser());
    app_controller_request_current_screen_update();
    return true;
}

static bool
action_runtime_toggle_library_tag_type(void) {
    NativeMediaLibraryScreen *screen;
    enum mpd_tag_type tag_type;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY)) {
        return false;
    }

    switch (Config.media_lib_primary_tag) {
    case MPD_TAG_ARTIST:
        tag_type = MPD_TAG_ALBUM_ARTIST;
        break;
    case MPD_TAG_ALBUM_ARTIST:
        tag_type = MPD_TAG_DATE;
        break;
    case MPD_TAG_DATE:
        tag_type = MPD_TAG_GENRE;
        break;
    case MPD_TAG_GENRE:
        tag_type = MPD_TAG_COMPOSER;
        break;
    case MPD_TAG_COMPOSER:
        tag_type = MPD_TAG_PERFORMER;
        break;
    default:
        tag_type = MPD_TAG_ARTIST;
        break;
    }

    screen = native_c_screen_media_library();
    return native_media_library_screen_set_primary_tag_type(screen,
                                                             tag_type);
}

static bool
action_runtime_toggle_media_library_sort_mode(void) {
    bool sort_by_mtime;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY)) {
        return false;
    }
    sort_by_mtime = native_media_library_screen_toggle_sort_mode(
        native_c_screen_media_library());
    if (sort_by_mtime) {
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time,
            (char *)"Sorting library by: modification time");
    } else {
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time,
            (char *)"Sorting library by: name");
    }
    return true;
}

static bool
action_runtime_toggle_media_library_columns(void) {
    NativeMediaLibraryScreen *screen;

    if (!action_runtime_current_screen_is(NCM_SCREEN_TYPE_MEDIA_LIBRARY)) {
        return false;
    }
    screen = native_c_screen_media_library();
    (void)native_media_library_screen_toggle_mode(screen);
    app_controller_request_current_screen_resize();
    return true;
}

static bool
action_runtime_toggle_replay_gain_mode(void) {
    NcmError error;
    NcWindow *window;
    char choice;
    enum NcmMpdReplayGainMode mode;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    window = app_controller_active_window();
    if (window == NULL) {
        return false;
    }

    ncm_statusbar_print_cstring(0,
                                "Replay gain: off [o], track [t], album [a]");
    choice = 'o';
    if (!ncm_statusbar_prompt_return_one_of(window, "ota", 3,
                                            &choice)) {
        return false;
    }

    mode = NCM_MPD_REPLAY_GAIN_OFF;
    if (choice == 't') {
        mode = NCM_MPD_REPLAY_GAIN_TRACK;
    } else if (choice == 'a') {
        mode = NCM_MPD_REPLAY_GAIN_ALBUM;
    }

    ncm_error_clear(&error);
    if (!ncm_mpd_client_set_replay_gain_mode(&global_mpd, mode, &error)) {
        return action_runtime_mpd_error(&error);
    }
    return true;
}

static bool
action_runtime_save_tag_changes(void) {
#if defined(HAVE_TAGLIB_H)
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TAG_EDITOR)) {
        if (!native_tag_editor_screen_save_action_available(
                native_c_screen_tag_editor())) {
            return false;
        }
        return native_tag_editor_screen_save_modified(
            native_c_screen_tag_editor(), Config.mpd_music_dir);
    }
    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TINY_TAG_EDITOR)) {
        return native_tiny_tag_editor_screen_run_row(
            native_c_screen_tiny_tag_editor(),
            NATIVE_TINY_TAG_EDITOR_SAVE_ROW);
    }
#endif
    return false;
}

bool
ncm_action_edit_song(NcmSong *song) {
#if defined(HAVE_TAGLIB_H)
    enum NativeTinyTagEditorOpenResult open_result;
    NcmStringFormatArg arg;
    NcmBuffer path;
    int32 path_len;
    int32 path_width;
    bool success;

    if (song == NULL) {
        return false;
    }
    if (Config.mpd_music_dir_len <= 0) {
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time,
            (char *)"Proper mpd_music_dir variable has to be set in "
                    "configuration file");
        return false;
    }

    ncm_buffer_init(&path);
    open_result = native_tiny_tag_editor_screen_open_song(
        native_c_screen_tiny_tag_editor(), song, Config.mpd_music_dir,
        Config.mpd_music_dir_len, Config.tags_separator,
        Config.tags_separator_len, Config.show_duplicate_tags, &path);
    success = false;
    switch (open_result) {
    case NATIVE_TINY_TAG_EDITOR_OPEN_SUCCESS:
        success = action_runtime_switch_to_screen(
            NCM_SCREEN_TYPE_TINY_TAG_EDITOR);
        break;
    case NATIVE_TINY_TAG_EDITOR_OPEN_STREAM:
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time,
            (char *)"Streams can't be edited");
        break;
    case NATIVE_TINY_TAG_EDITOR_OPEN_MISSING_MUSIC_DIRECTORY:
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time,
            (char *)"Proper mpd_music_dir variable has to be set in "
                    "configuration file");
        break;
    case NATIVE_TINY_TAG_EDITOR_OPEN_UNREADABLE_FILE:
        path_width = COLS - STRLIT_LEN("Couldn't read file \"%1%\"");
        if (path_width < 0) {
            path_width = 0;
        }
        path_len = ncm_utf8_cut_width(path.data, path.len, path_width);
        arg = ncm_string_format_arg_string(path.data, path_len);
        ncm_statusbar_format(
            (int32)Config.message_delay_time,
            STRLIT_ARGS("Couldn't read file \"%1%\""), &arg, 1);
        break;
    case NATIVE_TINY_TAG_EDITOR_OPEN_INVALID_ARGUMENT:
    case NATIVE_TINY_TAG_EDITOR_OPEN_PREPARE_FAILED:
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time,
            (char *)"Couldn't prepare tiny tag editor");
        break;
    }
    ncm_buffer_destroy(&path);
    return success;
#else
    (void)song;
    return false;
#endif
}

static bool
action_runtime_edit_current_song(void) {
#if defined(HAVE_TAGLIB_H)
    NcmSong song;
    bool success;

    if (!action_runtime_has_selected_songs()) {
        return false;
    }
    ncm_song_init(&song);
    success = action_runtime_current_song(&song);
    if (success) {
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

    fetcher = native_lyrics_screen_toggle_fetcher(
        native_c_screen_lyrics(), &Config.lyrics_fetchers);
    if (fetcher == NULL) {
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    (char *)"Using all lyrics fetchers");
        return true;
    }

    arg = ncm_string_format_arg_string(
        ncm_lyrics_fetcher_name(fetcher),
        ncm_lyrics_fetcher_name_len(fetcher));
    ncm_statusbar_format((int32)Config.message_delay_time,
                         STRLIT_ARGS("Using lyrics fetcher: %1%"),
                         &arg,
                         1);
    return true;
}

static bool
action_runtime_edit_lyrics(void) {
    NativeLyricsScreen *lyrics;
    NcmSong *song;
    NcmBuffer *filename;
    NcmBuffer escaped;
    NcmBuffer command;
    NcmError error;
    bool success;

    if (Config.external_editor_len <= 0) {
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time,
            (char *)"external_editor variable has to be set in "
                    "configuration file");
        return false;
    }

    lyrics = native_c_screen_lyrics();
    song = native_lyrics_screen_song(lyrics);
    if (song == NULL) {
        return false;
    }

    if (!native_lyrics_screen_build_filename(
            lyrics, song, Config.mpd_music_dir, Config.mpd_music_dir_len,
            Config.lyrics_directory, Config.lyrics_directory_len,
            Config.store_lyrics_in_song_dir,
            Config.generate_win32_compatible_filenames)) {
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    (char *)"failed to build lyrics "
                                            "filename");
        return false;
    }

    ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                (char *)"Opening lyrics in external "
                                        "editor...");
    filename = native_lyrics_screen_filename(lyrics);
    ncm_buffer_init(&escaped);
    ncm_buffer_init(&command);
    ncm_string_append_shell_escaped_single_quotes(&escaped,
                                                  filename->data,
                                                  filename->len);
    ncm_buffer_append(&command, Config.external_editor,
                      Config.external_editor_len);
    ncm_buffer_append(&command, STRLIT_ARGS(" '"));
    ncm_buffer_append(&command, escaped.data, escaped.len);
    ncm_buffer_append_byte(&command, '\'');

    ncm_error_clear(&error);
    if (Config.use_console_editor) {
        nc_pause_screen();
        success = ncm_macro_run_external_console_command(command.data,
                                                         command.len,
                                                         &error);
        nc_unpause_screen();
        if (!success) {
            ncm_buffer_destroy(&command);
            ncm_buffer_destroy(&escaped);
            return action_runtime_mpd_error(&error);
        }

        if (!native_lyrics_screen_load_file(lyrics, filename->data,
                                            filename->len, &error)) {
            lyrics->has_song = false;
            success = native_lyrics_screen_fetch(lyrics, song, NULL,
                                                 &error);
        }
    } else {
        success = ncm_macro_run_external_command(command.data,
                                                 command.len, false,
                                                 &error);
    }

    ncm_buffer_destroy(&command);
    ncm_buffer_destroy(&escaped);
    if (!success) {
        return action_runtime_mpd_error(&error);
    }
    ncm_error_clear(&error);
    return true;
}

static bool
action_runtime_fetch_lyrics_background(void) {
    NcmSongArray songs;
    NcmError error;
    bool success;

    ncm_song_array_init(&songs);
    success = action_runtime_selected_songs(&songs) && (songs.len > 0);
    if (!success) {
        ncm_song_array_destroy(&songs);
        return false;
    }

    ncm_error_clear(&error);
    for (int32 i = 0; i < songs.len; i += 1) {
        if (!native_lyrics_screen_fetch_in_background(
                native_c_screen_lyrics(), &songs.items[i], true,
                &error)) {
            ncm_song_array_destroy(&songs);
            return action_runtime_mpd_error(&error);
        }
    }

    ncm_song_array_destroy(&songs);
    ncm_statusbar_print_cstring(
        (int32)Config.message_delay_time,
        (char *)"Selected songs queued for lyrics fetching");
    return true;
}

static bool
action_runtime_refetch_lyrics(void) {
    NcmError error;

    ncm_error_clear(&error);
    native_lyrics_screen_refetch_current(native_c_screen_lyrics(), &error);
    return !ncm_error_is_set(&error);
}

static bool
action_runtime_show_lyrics(void) {
    NcmSong song;
    NcmSong *lyrics_song;
    NcmError error;
    bool success;

    if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_LYRICS)) {
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_LYRICS);
    }

    ncm_song_init(&song);
    success = action_runtime_current_song(&song);
    if (success) {
        lyrics_song = native_lyrics_screen_song(native_c_screen_lyrics());
        if ((lyrics_song == NULL) || !ncm_song_equal(lyrics_song, &song)) {
            ncm_error_clear(&error);
            success = native_lyrics_screen_fetch(native_c_screen_lyrics(),
                                                 &song, NULL, &error);
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
    NcmError error;
    bool success;

    ncm_song_init(&song);
    success = action_runtime_current_song(&song);
    if (success) {
        success = ncm_song_tag_view(&song, MPD_TAG_ARTIST, 0, &artist);
    }
    if (success) {
        ncm_error_clear(&error);
        success = native_lastfm_screen_queue_artist_info(
            native_c_screen_lastfm(), artist.data, artist.len,
            Config.lastfm_preferred_language,
            Config.lastfm_preferred_language_len, &error);
    }
    ncm_song_destroy(&song);
    if (!success) {
        return false;
    }
    return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_LASTFM);
}

static bool
action_runtime_mouse_event(void) {
    NcWindow *window;
    MEVENT *event;

    if (!Config.mouse_support) {
        return false;
    }

    window = app_controller_active_window();
    if (window == NULL) {
        return false;
    }
    event = nc_window_mouse_event(window);
    if (event == NULL) {
        return false;
    }

    app_controller_mouse_button_pressed_current(*event);
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
    case NCM_ACTION_SCROLL_UP_ARTIST:
    case NCM_ACTION_SCROLL_UP_ALBUM:
    case NCM_ACTION_SCROLL_DOWN_ARTIST:
    case NCM_ACTION_SCROLL_DOWN_ALBUM:
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
    case NCM_ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE:
        return native_c_screen_lyrics_is_current();
    case NCM_ACTION_SHOW_HELP:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_HELP)) {
            return false;
        }
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_TINY_TAG_EDITOR)) {
            return false;
        }
#endif
        return true;
    case NCM_ACTION_SHOW_PLAYLIST:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_PLAYLIST)) {
            return false;
        }
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_TINY_TAG_EDITOR)) {
            return false;
        }
#endif
        return true;
    case NCM_ACTION_SHOW_PLAYLIST_EDITOR:
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
            return false;
        }
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_TINY_TAG_EDITOR)) {
            return false;
        }
#endif
        return true;
    case NCM_ACTION_SHOW_SERVER_INFO:
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_TINY_TAG_EDITOR)) {
            return false;
        }
#endif
        return true;
    case NCM_ACTION_MOUSE_EVENT:
        return Config.mouse_support;
    case NCM_ACTION_SHOW_MEDIA_LIBRARY:
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_MEDIA_LIBRARY)) {
            return false;
        }
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_TINY_TAG_EDITOR)) {
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
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
            return action_runtime_playlist_editor_has_playlists();
        }
        return action_runtime_has_selected_songs();
    case NCM_ACTION_PLAY_ITEM:
        if (!ncm_mpd_client_connected(&global_mpd)) {
            return false;
        }
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_PLAYLIST)) {
            return action_runtime_has_current_song();
        }
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
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
        return nc_screen_can_run_current(
            app_controller_current_screen());
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
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_PLAYLIST)) {
            return nc_menu_has_selected(native_playlist_screen_menu(
                native_c_screen_playlist()));
        }
        return action_runtime_playlist_editor_content_active()
            && nc_menu_has_selected(nc_song_menu_base(
                   native_playlist_editor_screen_content(
                       native_c_screen_playlist_editor())));
    case NCM_ACTION_ADD:
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
            return ncm_mpd_client_connected(&global_mpd)
                && action_runtime_playlist_editor_has_playlists();
        }
        return action_runtime_menu_has_items();
    case NCM_ACTION_LOAD:
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
            return false;
        }
        return action_runtime_menu_has_items();
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
    case NCM_ACTION_SHUFFLE:
    {
        uint32 first;
        uint32 last;

        if (!ncm_mpd_client_connected(&global_mpd)
            || !action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_PLAYLIST)) {
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
            return native_tag_editor_screen_save_action_available(
                native_c_screen_tag_editor());
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
        return action_runtime_has_selected_songs();
#else
        return false;
#endif
    case NCM_ACTION_JUMP_TO_BROWSER:
        return action_runtime_has_current_song();
    case NCM_ACTION_JUMP_TO_PLAYLIST_EDITOR:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
            NcmMpdItem *item;

            item = native_browser_screen_current_item(
                native_c_screen_browser());
            return (item != NULL)
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
    case NCM_ACTION_SELECT_RANGE:
    case NCM_ACTION_REVERSE_SELECTION:
    case NCM_ACTION_REMOVE_SELECTION:
        return action_runtime_menu_has_items();
    case NCM_ACTION_ADD_SELECTED_ITEMS:
        return action_runtime_has_selected_songs();
    case NCM_ACTION_CROP_MAIN_PLAYLIST:
        if (!ncm_mpd_client_connected(&global_mpd)
            || !action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_PLAYLIST)) {
            return false;
        }
        return (native_playlist_screen_song_count(
                    native_c_screen_playlist()) > 1)
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
            && native_playlist_screen_has_sortable_range(
                   native_c_screen_playlist());
    case NCM_ACTION_REVERSE_PLAYLIST:
    {
        NcMenu *menu;
        int64 first;
        int64 last;

        if (!ncm_mpd_client_connected(&global_mpd)
            || !action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_PLAYLIST)) {
            return false;
        }
        menu = action_runtime_current_menu();
        if ((menu == NULL) || nc_menu_empty(menu)) {
            return false;
        }
        return ncm_menu_find_full_selected_range(
            menu, action_runtime_menu_item_source(menu),
            &first, &last);
    }
    case NCM_ACTION_TOGGLE_BROWSER_SORT_MODE:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER);
    case NCM_ACTION_TOGGLE_LIBRARY_TAG_TYPE:
    case NCM_ACTION_TOGGLE_MEDIA_LIBRARY_SORT_MODE:
        return action_runtime_current_screen_is(
            NCM_SCREEN_TYPE_MEDIA_LIBRARY);
    case NCM_ACTION_FETCH_LYRICS_IN_BACKGROUND:
        return action_runtime_has_selected_songs();
    case NCM_ACTION_EDIT_LYRICS:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_LYRICS);
    case NCM_ACTION_REFETCH_LYRICS:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_LYRICS);
    case NCM_ACTION_SHOW_ARTIST_INFO:
        return action_runtime_has_current_song();
    case NCM_ACTION_SHOW_LYRICS:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_LYRICS)
            || action_runtime_has_current_song();
#if defined(ENABLE_OUTPUTS)
    case NCM_ACTION_SHOW_OUTPUTS:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_OUTPUTS)) {
            return false;
        }
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_TINY_TAG_EDITOR)) {
            return false;
        }
#endif
        return true;
    case NCM_ACTION_TOGGLE_OUTPUT:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_OUTPUTS);
#endif
#if defined(ENABLE_VISUALIZER)
    case NCM_ACTION_SHOW_VISUALIZER:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_VISUALIZER)) {
            return false;
        }
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_TINY_TAG_EDITOR)) {
            return false;
        }
#endif
        return true;
    case NCM_ACTION_TOGGLE_VISUALIZATION_TYPE:
        return action_runtime_current_screen_is(
            NCM_SCREEN_TYPE_VISUALIZER);
#endif
#if defined(HAVE_TAGLIB_H)
    case NCM_ACTION_SHOW_TAG_EDITOR:
        return true;
#endif
    case NCM_ACTION_SHOW_BROWSER:
        return !action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER);
    case NCM_ACTION_CHANGE_BROWSE_MODE:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER);
    case NCM_ACTION_RESET_SEARCH_ENGINE:
        return action_runtime_current_screen_is(
            NCM_SCREEN_TYPE_SEARCH_ENGINE);
    case NCM_ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE:
        return action_runtime_current_screen_is(
            NCM_SCREEN_TYPE_MEDIA_LIBRARY);
    case NCM_ACTION_EXECUTE_COMMAND:
        return true;
    case NCM_ACTION_APPLY_FILTER:
        return current_screen_allows_filter();
    case NCM_ACTION_FIND_ITEM_FORWARD:
    case NCM_ACTION_FIND_ITEM_BACKWARD:
    case NCM_ACTION_NEXT_FOUND_ITEM:
    case NCM_ACTION_PREVIOUS_FOUND_ITEM:
        return current_screen_allows_search();
    case NCM_ACTION_TOGGLE_FIND_MODE:
        return true;
    case NCM_ACTION_START_SEARCHING:
        return action_runtime_current_screen_is(
                   NCM_SCREEN_TYPE_SEARCH_ENGINE)
            && !native_search_engine_screen_constraints_locked(
                   native_c_screen_search_engine());
    case NCM_ACTION_SAVE_PLAYLIST:
        return ncm_mpd_client_connected(&global_mpd);
    case NCM_ACTION_JUMP_TO_POSITION_IN_SONG:
        return ncm_mpd_client_connected(&global_mpd)
            && (ncm_status_state_player() != NCM_STATUS_PLAYER_STOP)
            && (ncm_status_state_total_time() > 0)
            && (ncm_status_state_current_song_position() >= 0);
    case NCM_ACTION_SELECT_FOUND_ITEMS:
    {
        NcmStringView constraint;

        if (!action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_PLAYLIST)) {
            return false;
        }
        constraint = current_screen_current_search_constraint();
        return action_runtime_menu_has_items()
            && (constraint.data != NULL) && (constraint.len > 0);
    }
    case NCM_ACTION_SELECT_ALBUM:
        if (!action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_PLAYLIST)) {
            return false;
        }
        return action_runtime_menu_has_items();
    case NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY:
        if (!ncm_mpd_client_connected(&global_mpd)
            || !action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_PLAYLIST)
            || !action_runtime_has_selected_songs()) {
            return false;
        }
        if (ncm_mpd_client_version(&global_mpd) < 17) {
            ncm_statusbar_print_cstring(
                (int32)Config.message_delay_time,
                (char *)"Priorities are supported in MPD >= 0.17.0");
            return false;
        }
        return true;
    case NCM_ACTION_EDIT_PLAYLIST_NAME:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
            return ncm_mpd_client_connected(&global_mpd)
                && native_browser_screen_rename_playlist_available(
                    native_c_screen_browser());
        }
        return ncm_mpd_client_connected(&global_mpd)
            && action_runtime_playlist_editor_playlists_active()
            && action_runtime_playlist_editor_has_playlists();
    case NCM_ACTION_EDIT_DIRECTORY_NAME:
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER)) {
            return native_browser_screen_rename_directory_available(
                native_c_screen_browser());
        }
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(NCM_SCREEN_TYPE_TAG_EDITOR)) {
            return native_tag_editor_screen_rename_directory_available(
                native_c_screen_tag_editor(), Config.mpd_music_dir,
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
        if (!native_browser_screen_is_local(native_c_screen_browser())
            && (Config.mpd_music_dir_len <= 0)) {
            return false;
        }
        return action_runtime_menu_has_items();
    case NCM_ACTION_EDIT_LIBRARY_TAG:
    case NCM_ACTION_EDIT_LIBRARY_ALBUM:
    case NCM_ACTION_FIND:
        return false;
    default:
        return false;
    }
}

static bool
action_runtime_builtin_run(NcmActionRuntime *runtime,
                           enum NcmActionType type) {
    switch (type) {
    case NCM_ACTION_DUMMY:
        return true;
    case NCM_ACTION_UPDATE_ENVIRONMENT:
        return action_runtime_update_environment();
    case NCM_ACTION_MOUSE_EVENT:
        return action_runtime_mouse_event();
    case NCM_ACTION_SCROLL_UP:
    case NCM_ACTION_SCROLL_UP_ARTIST:
    case NCM_ACTION_SCROLL_UP_ALBUM:
        app_controller_scroll_current_screen(NC_SCROLL_UP);
        return true;
    case NCM_ACTION_SCROLL_DOWN:
    case NCM_ACTION_SCROLL_DOWN_ARTIST:
    case NCM_ACTION_SCROLL_DOWN_ALBUM:
        app_controller_scroll_current_screen(NC_SCROLL_DOWN);
        return true;
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
        return action_runtime_volume((int32)Config.volume_change_step);
    case NCM_ACTION_VOLUME_DOWN:
        return action_runtime_volume(-((int32)Config.volume_change_step));
    case NCM_ACTION_ADD_ITEM_TO_PLAYLIST:
        return action_runtime_add_selected_songs(false);
    case NCM_ACTION_PLAY_ITEM:
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_PLAYLIST)) {
            return nc_playlist_screen_activate_current(
                native_playlist_screen_playlist(
                    native_c_screen_playlist()));
        }
        return action_runtime_add_selected_songs(true);
    case NCM_ACTION_DELETE_PLAYLIST_ITEMS:
        return action_runtime_delete_playlist_items();
    case NCM_ACTION_DELETE_STORED_PLAYLIST:
        return action_runtime_delete_stored_playlists();
    case NCM_ACTION_RUN_ACTION:
        return nc_screen_run_current(app_controller_current_screen());
    case NCM_ACTION_MOVE_SORT_ORDER_UP:
        return native_sort_playlist_dialog_move_current_up(
            native_c_screen_sort_playlist_dialog());
    case NCM_ACTION_MOVE_SORT_ORDER_DOWN:
        return native_sort_playlist_dialog_move_current_down(
            native_c_screen_sort_playlist_dialog());
    case NCM_ACTION_MOVE_SELECTED_ITEMS_UP:
        return action_runtime_move_selected_items(false);
    case NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN:
        return action_runtime_move_selected_items(true);
    case NCM_ACTION_MOVE_SELECTED_ITEMS_TO:
        return action_runtime_move_selected_items_to();
    case NCM_ACTION_ADD:
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER)) {
            return native_selected_items_adder_screen_run_current(
                native_c_screen_selected_items_adder());
        }
        return action_runtime_add_selected_songs(false);
    case NCM_ACTION_LOAD:
        if (action_runtime_current_screen_is(
                NCM_SCREEN_TYPE_PLAYLIST_EDITOR)) {
            NcmError error;
            bool loaded;

            loaded = false;
            ncm_error_clear(&error);
            if (!native_playlist_editor_screen_load_current_playlist(
                    native_c_screen_playlist_editor(), &global_mpd,
                    &loaded, &error)) {
                return action_runtime_mpd_error(&error);
            }
            return loaded;
        }
        return false;
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
                (void)native_playlist_screen_locate_position(
                    native_c_screen_playlist(), (uint32)position);
            }
        }
        if (Config.autocenter_mode) {
            ncm_statusbar_print_cstring(
                (int32)Config.message_delay_time,
                (char *)"Centering playing song: on");
        } else {
            ncm_statusbar_print_cstring(
                (int32)Config.message_delay_time,
                (char *)"Centering playing song: off");
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
        if (app_controller_locked_screen() != NULL) {
            app_controller_unlock_screen();
        } else {
            (void)app_controller_lock_current_screen();
        }
        return true;
    case NCM_ACTION_JUMP_TO_TAG_EDITOR:
        return action_runtime_jump_to_tag_editor();
    case NCM_ACTION_JUMP_TO_POSITION_IN_SONG:
        return action_runtime_jump_to_position_in_song();
    case NCM_ACTION_SELECT_ITEM:
        return nc_menu_toggle_current_selected(action_runtime_current_menu());
    case NCM_ACTION_SELECT_RANGE:
    {
        NcMenu *menu;
        int64 first;
        int64 current;

        menu = action_runtime_current_menu();
        if (menu == NULL) {
            return false;
        }
        first = nc_menu_first_selected_position(menu);
        current = nc_menu_highlight(menu);
        if (first < 0) {
            return nc_menu_toggle_current_selected(menu);
        }
        return nc_menu_select_range(menu, first, current, true);
    }
    case NCM_ACTION_REVERSE_SELECTION:
    {
        NcMenu *menu;

        menu = action_runtime_current_menu();
        if (menu == NULL) {
            return false;
        }
        ncm_menu_reverse_selection(
            menu, action_runtime_menu_item_source(menu));
        return true;
    }
    case NCM_ACTION_REMOVE_SELECTION:
        nc_menu_clear_selection(action_runtime_current_menu());
        return true;
    case NCM_ACTION_ADD_SELECTED_ITEMS:
    {
        NcmSongArray songs;
        NcmError error;
        bool success;

        ncm_song_array_init(&songs);
        success = action_runtime_selected_songs(&songs) && (songs.len > 0);
        if (!success) {
            ncm_song_array_destroy(&songs);
            return false;
        }

        ncm_error_clear(&error);
        success = native_c_screen_selected_items_adder_open(&songs, &error);
        ncm_song_array_destroy(&songs);
        if (!success) {
            return action_runtime_mpd_error(&error);
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
        native_search_engine_screen_reset(native_c_screen_search_engine());
        return true;
    case NCM_ACTION_SHOW_MEDIA_LIBRARY:
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_MEDIA_LIBRARY);
    case NCM_ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE:
        return action_runtime_toggle_media_library_columns();
    case NCM_ACTION_SHOW_PLAYLIST_EDITOR:
        return action_runtime_switch_to_screen(
            NCM_SCREEN_TYPE_PLAYLIST_EDITOR);
    case NCM_ACTION_SHOW_SERVER_INFO:
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_SERVER_INFO);
    case NCM_ACTION_SHOW_SONG_INFO:
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_SONG_INFO);
#if defined(ENABLE_OUTPUTS)
    case NCM_ACTION_SHOW_OUTPUTS:
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_OUTPUTS);
    case NCM_ACTION_TOGGLE_OUTPUT:
        native_c_screen_outputs_toggle();
        return true;
#endif
#if defined(ENABLE_VISUALIZER)
    case NCM_ACTION_SHOW_VISUALIZER:
        return ncm_action_show_visualizer();
    case NCM_ACTION_TOGGLE_VISUALIZATION_TYPE:
        return ncm_action_toggle_visualization_type();
#endif
#if defined(HAVE_TAGLIB_H)
    case NCM_ACTION_SHOW_TAG_EDITOR:
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_TAG_EDITOR);
#endif
    case NCM_ACTION_EXECUTE_COMMAND:
        return action_runtime_execute_command();
    case NCM_ACTION_SAVE_PLAYLIST:
        return action_runtime_save_playlist();
    case NCM_ACTION_APPLY_FILTER:
        return action_runtime_apply_filter();
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
            ncm_statusbar_print_cstring(
                (int32)Config.message_delay_time,
                (char *)"Search mode: Wrapped");
        } else {
            ncm_statusbar_print_cstring(
                (int32)Config.message_delay_time,
                (char *)"Search mode: Normal");
        }
        return true;
    case NCM_ACTION_START_SEARCHING: {
        NcmError error;

        ncm_error_clear(&error);
        return native_search_engine_screen_start_searching(
            native_c_screen_search_engine(), &global_mpd, &error);
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
    case NCM_ACTION_EDIT_LIBRARY_ALBUM:
    case NCM_ACTION_FIND:
        return false;
    default:
        return false;
    }
}
