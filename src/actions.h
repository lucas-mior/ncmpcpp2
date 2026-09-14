#if !defined(ACTIONS_H)
#define ACTIONS_H

#include "cbase.h"
#include "ncmpcpp2.h"

#include "c/ncm_c.h"

#define ACTION_TYPE_FIELDS(XX)                                                 \
  XX(DUMMY, dummy)                                                             \
  XX(UPDATE_ENVIRONMENT, update_environment)                                   \
  XX(MOUSE_EVENT, mouse_event)                                                 \
  XX(SCROLL_UP, scroll_up)                                                     \
  XX(SCROLL_DOWN, scroll_down)                                                 \
  XX(SCROLL_UP_ARTIST, scroll_up_artist)                                       \
  XX(SCROLL_UP_ALBUM, scroll_up_album)                                         \
  XX(SCROLL_DOWN_ARTIST, scroll_down_artist)                                   \
  XX(SCROLL_DOWN_ALBUM, scroll_down_album)                                     \
  XX(PAGE_UP, page_up)                                                         \
  XX(PAGE_DOWN, page_down)                                                     \
  XX(MOVE_HOME, move_home)                                                     \
  XX(MOVE_END, move_end)                                                       \
  XX(TOGGLE_INTERFACE, toggle_interface)                                       \
  XX(JUMP_TO_PARENT_DIRECTORY, jump_to_parent_directory)                       \
  XX(RUN_ACTION, run_action)                                                   \
  XX(PREVIOUS_COLUMN, previous_column)                                         \
  XX(NEXT_COLUMN, next_column)                                                 \
  XX(MASTER_SCREEN, master_screen)                                             \
  XX(SLAVE_SCREEN, slave_screen)                                               \
  XX(VOLUME_UP, volume_up)                                                     \
  XX(VOLUME_DOWN, volume_down)                                                 \
  XX(ADD_ITEM_TO_PLAYLIST, add_item_to_playlist)                               \
  XX(PLAY_ITEM, play_item)                                                     \
  XX(DELETE_PLAYLIST_ITEMS, delete_playlist_items)                             \
  XX(DELETE_STORED_PLAYLIST, delete_stored_playlist)                           \
  XX(DELETE_BROWSER_ITEMS, delete_browser_items)                               \
  XX(REPLAY_SONG, replay_song)                                                 \
  XX(PREVIOUS, previous)                                                       \
  XX(NEXT, next)                                                               \
  XX(PAUSE, pause)                                                             \
  XX(STOP, stop)                                                               \
  XX(PLAY, play)                                                               \
  XX(EXECUTE_COMMAND, execute_command)                                         \
  XX(SAVE_PLAYLIST, save_playlist)                                             \
  XX(MOVE_SORT_ORDER_UP, move_sort_order_up)                                   \
  XX(MOVE_SORT_ORDER_DOWN, move_sort_order_down)                               \
  XX(MOVE_SELECTED_ITEMS_UP, move_selected_items_up)                           \
  XX(MOVE_SELECTED_ITEMS_DOWN, move_selected_items_down)                       \
  XX(MOVE_SELECTED_ITEMS_TO, move_selected_items_to)                           \
  XX(ADD, add)                                                                 \
  XX(LOAD, load)                                                               \
  XX(SEEK_FORWARD, seek_forward)                                               \
  XX(SEEK_BACKWARD, seek_backward)                                             \
  XX(TOGGLE_DISPLAY_MODE, toggle_display_mode)                                 \
  XX(TOGGLE_SEPARATORS_BETWEEN_ALBUMS, toggle_separators_between_albums)       \
  XX(TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE, toggle_lyrics_update_on_song_change) \
  XX(TOGGLE_LYRICS_FETCHER, toggle_lyrics_fetcher)                             \
  XX(TOGGLE_FETCHING_LYRICS_IN_BACKGROUND, toggle_fetch_lyrics_in_background)  \
  XX(TOGGLE_PLAYING_SONG_CENTERING, toggle_playing_song_centering)             \
  XX(UPDATE_DATABASE, update_database)                                         \
  XX(JUMP_TO_PLAYING_SONG, jump_to_playing_song)                               \
  XX(TOGGLE_REPEAT, toggle_repeat)                                             \
  XX(SHUFFLE, shuffle)                                                         \
  XX(TOGGLE_RANDOM, toggle_random)                                             \
  XX(START_SEARCHING, start_searching)                                         \
  XX(SAVE_TAG_CHANGES, save_tag_changes)                                       \
  XX(TOGGLE_SINGLE, toggle_single)                                             \
  XX(TOGGLE_CONSUME, toggle_consume)                                           \
  XX(TOGGLE_CROSSFADE, toggle_crossfade)                                       \
  XX(SET_CROSSFADE, set_crossfade)                                             \
  XX(SET_VOLUME, set_volume)                                                   \
  XX(ENTER_DIRECTORY, enter_directory)                                         \
  XX(EDIT_SONG, edit_song)                                                     \
  XX(EDIT_LIBRARY_TAG, edit_library_tag)                                       \
  XX(EDIT_LIBRARY_ALBUM, edit_library_album)                                   \
  XX(EDIT_DIRECTORY_NAME, edit_directory_name)                                 \
  XX(EDIT_PLAYLIST_NAME, edit_playlist_name)                                   \
  XX(EDIT_LYRICS, edit_lyrics)                                                 \
  XX(JUMP_TO_BROWSER, jump_to_browser)                                         \
  XX(JUMP_TO_MEDIA_LIBRARY, jump_to_media_library)                             \
  XX(JUMP_TO_PLAYLIST_EDIT, jump_to_playlist_edit)                             \
  XX(TOGGLE_SCREEN_LOCK, toggle_screen_lock)                                   \
  XX(JUMP_TO_TAG_EDIT, jump_to_tag_edit)                                       \
  XX(JUMP_TO_POSITION_IN_SONG, jump_to_position_in_song)                       \
  XX(SELECT_ITEM, select_item)                                                 \
  XX(SELECT_RANGE, select_range)                                               \
  XX(REVERSE_SELECTION, reverse_selection)                                     \
  XX(REMOVE_SELECTION, remove_selection)                                       \
  XX(SELECT_ALBUM, select_album)                                               \
  XX(SELECT_FOUND_ITEMS, select_found_items)                                   \
  XX(ADD_SELECTED_ITEMS, add_selected_items)                                   \
  XX(CROP_MAIN_PLAYLIST, crop_main_playlist)                                   \
  XX(CROP_PLAYLIST, crop_playlist)                                             \
  XX(CLEAR_MAIN_PLAYLIST, clear_main_playlist)                                 \
  XX(CLEAR_PLAYLIST, clear_playlist)                                           \
  XX(SORT_PLAYLIST, sort_playlist)                                             \
  XX(REVERSE_PLAYLIST, reverse_playlist)                                       \
  XX(APPLY_FILTER, apply_filter)                                               \
  XX(FIND, find)                                                               \
  XX(FIND_ITEM_FORWARD, find_item_forward)                                     \
  XX(FIND_ITEM_BACKWARD, find_item_backward)                                   \
  XX(NEXT_FOUND_ITEM, next_found_item)                                         \
  XX(PREVIOUS_FOUND_ITEM, previous_found_item)                                 \
  XX(TOGGLE_FIND_MODE, toggle_find_mode)                                       \
  XX(TOGGLE_REPLAY_GAIN_MODE, toggle_replay_gain_mode)                         \
  XX(TOGGLE_ADD_MODE, toggle_add_mode)                                         \
  XX(TOGGLE_MOUSE, toggle_mouse)                                               \
  XX(TOGGLE_BITRATE_VISIBILITY, toggle_bitrate_visibility)                     \
  XX(ADD_RANDOM_ITEMS, add_random_items)                                       \
  XX(TOGGLE_BROWSER_SORT_MODE, toggle_browser_sort_mode)                       \
  XX(TOGGLE_LIBRARY_TAG_TYPE, toggle_library_tag_type)                         \
  XX(TOGGLE_MEDIA_LIBRARY_SORT_MODE, toggle_media_library_sort_mode)           \
  XX(FETCH_LYRICS_IN_BACKGROUND, fetch_lyrics_in_background)                   \
  XX(REFETCH_LYRICS, refetch_lyrics)                                           \
  XX(SET_SELECTED_ITEMS_PRIORITY, set_selected_items_priority)                 \
  XX(TOGGLE_OUTPUT, toggle_output)                                             \
  XX(TOGGLE_VISUALIZATION_TYPE, toggle_visualization_type)                     \
  XX(SHOW_SONG_INFO, show_song_info)                                           \
  XX(SHOW_ARTIST_INFO, show_artist_info)                                       \
  XX(SHOW_LYRICS, show_lyrics)                                                 \
  XX(QUIT, quit)                                                               \
  XX(NEXT_SCREEN, next_screen)                                                 \
  XX(PREVIOUS_SCREEN, previous_screen)                                         \
  XX(SHOW_HELP, show_help)                                                     \
  XX(SHOW_PLAYLIST, show_playlist)                                             \
  XX(SHOW_BROWSER, show_browser)                                               \
  XX(CHANGE_BROWSE_MODE, change_browse_mode)                                   \
  XX(SHOW_SEARCH_ENGINE, show_search_engine)                                   \
  XX(RESET_SEARCH_ENGINE, reset_search_engine)                                 \
  XX(SHOW_MEDIA_LIBRARY, show_media_library)                                   \
  XX(TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE, toggle_media_library_columns_mode)     \
  XX(SHOW_PLAYLIST_EDIT, show_playlist_edit)                                   \
  XX(SHOW_TAG_EDIT, show_tag_edit)                                             \
  XX(SHOW_OUTPUTS, show_outputs)                                               \
  XX(SHOW_VISUALIZER, show_visualizer)                                         \
  XX(SHOW_SERVER_INFO, show_server_info)

#define ENUM_NAME ActionType
#define ENUM_PREFIX_ ACTION_
#define ENUM_BITFLAGS 0
#define ACTION_TYPE_FIELD_EXPAND(type, alias) XX(type, alias)
#define ACTION_TYPE_FIELD(type, alias) \
  ACTION_TYPE_FIELD_EXPAND(CAT(ACTION_, type), alias)
#define ENUM_FIELDS ACTION_TYPE_FIELDS(ACTION_TYPE_FIELD)
#include "cbase/xenums.c"
#undef ACTION_TYPE_FIELD
#undef ACTION_TYPE_FIELD_EXPAND

#define ACTION_RUNTIME_DEFER 0
#define ACTION_RUNTIME_ALLOW 1
#define ACTION_RUNTIME_DENY -1

typedef int32 ActionRuntimeHook(enum ActionType type, void *user);

typedef struct ActionRuntime {
    ActionRuntimeHook *can_run_hook;
    ActionRuntimeHook *run_hook;
    void *user;
    bool exit_requested;
} ActionRuntime;

ActionRuntime *ncm_action_runtime_global(void);
bool ncm_action_runtime_exit_requested(ActionRuntime *);
void ncm_action_runtime_request_exit(ActionRuntime *);
bool ncm_action_runtime_can_run(ActionRuntime *, enum ActionType);
int32 ncm_action_runtime_run(ActionRuntime *, enum ActionType);
int32 ncm_action_edit_song(NcmSong *);
int32 ncm_action_show_visualizer(void);
int32 ncm_action_toggle_visualization_type(void);
int32 ncm_action_add_song_to_playlist_with_mode(NcmSong *, bool, int32,
                                                enum SpaceAddMode);
int32 ncm_action_add_song_to_playlist(NcmSong *, bool, int32);

int32 ncm_action_type_parse(char *, int32, enum ActionType *);
bool ncm_action_can_run(enum ActionType, void *);
bool ncm_action_immediate_command_prompt_should_stop(StrBuilder *,
                                                     char *, int32);

#endif /* ACTIONS_H */
