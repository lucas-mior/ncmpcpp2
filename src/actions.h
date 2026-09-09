#if !defined(ACTIONS_H)
#define ACTIONS_H

#include "cbase.h"

#include "c/ncm_c.h"

#define ACTION_TYPE_FIELDS(XX)                                               \
    XX(ACTION_DUMMY, dummy)                                                 \
    XX(ACTION_UPDATE_ENVIRONMENT, update_environment)                       \
    XX(ACTION_MOUSE_EVENT, mouse_event)                                     \
    XX(ACTION_SCROLL_UP, scroll_up)                                         \
    XX(ACTION_SCROLL_DOWN, scroll_down)                                     \
    XX(ACTION_SCROLL_UP_ARTIST, scroll_up_artist)                           \
    XX(ACTION_SCROLL_UP_ALBUM, scroll_up_album)                             \
    XX(ACTION_SCROLL_DOWN_ARTIST, scroll_down_artist)                       \
    XX(ACTION_SCROLL_DOWN_ALBUM, scroll_down_album)                         \
    XX(ACTION_PAGE_UP, page_up)                                             \
    XX(ACTION_PAGE_DOWN, page_down)                                         \
    XX(ACTION_MOVE_HOME, move_home)                                         \
    XX(ACTION_MOVE_END, move_end)                                           \
    XX(ACTION_TOGGLE_INTERFACE, toggle_interface)                           \
    XX(ACTION_JUMP_TO_PARENT_DIRECTORY, jump_to_parent_directory)           \
    XX(ACTION_RUN_ACTION, run_action)                                       \
    XX(ACTION_PREVIOUS_COLUMN, previous_column)                             \
    XX(ACTION_NEXT_COLUMN, next_column)                                     \
    XX(ACTION_MASTER_SCREEN, master_screen)                                 \
    XX(ACTION_SLAVE_SCREEN, slave_screen)                                   \
    XX(ACTION_VOLUME_UP, volume_up)                                         \
    XX(ACTION_VOLUME_DOWN, volume_down)                                     \
    XX(ACTION_ADD_ITEM_TO_PLAYLIST, add_item_to_playlist)                   \
    XX(ACTION_PLAY_ITEM, play_item)                                         \
    XX(ACTION_DELETE_PLAYLIST_ITEMS, delete_playlist_items)                 \
    XX(ACTION_DELETE_STORED_PLAYLIST, delete_stored_playlist)               \
    XX(ACTION_DELETE_BROWSER_ITEMS, delete_browser_items)                   \
    XX(ACTION_REPLAY_SONG, replay_song)                                     \
    XX(ACTION_PREVIOUS, previous)                                           \
    XX(ACTION_NEXT, next)                                                   \
    XX(ACTION_PAUSE, pause)                                                 \
    XX(ACTION_STOP, stop)                                                   \
    XX(ACTION_PLAY, play)                                                   \
    XX(ACTION_EXECUTE_COMMAND, execute_command)                             \
    XX(ACTION_SAVE_PLAYLIST, save_playlist)                                 \
    XX(ACTION_MOVE_SORT_ORDER_UP, move_sort_order_up)                       \
    XX(ACTION_MOVE_SORT_ORDER_DOWN, move_sort_order_down)                   \
    XX(ACTION_MOVE_SELECTED_ITEMS_UP, move_selected_items_up)               \
    XX(ACTION_MOVE_SELECTED_ITEMS_DOWN, move_selected_items_down)           \
    XX(ACTION_MOVE_SELECTED_ITEMS_TO, move_selected_items_to)               \
    XX(ACTION_ADD, add)                                                     \
    XX(ACTION_LOAD, load)                                                   \
    XX(ACTION_SEEK_FORWARD, seek_forward)                                   \
    XX(ACTION_SEEK_BACKWARD, seek_backward)                                 \
    XX(ACTION_TOGGLE_DISPLAY_MODE, toggle_display_mode)                     \
    XX(ACTION_TOGGLE_SEPARATORS_BETWEEN_ALBUMS,                             \
       toggle_separators_between_albums)                                    \
    XX(ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE,                          \
       toggle_lyrics_update_on_song_change)                                 \
    XX(ACTION_TOGGLE_LYRICS_FETCHER, toggle_lyrics_fetcher)                 \
    XX(ACTION_TOGGLE_FETCHING_LYRICS_IN_BACKGROUND,                         \
       toggle_fetch_lyrics_in_background)                                   \
    XX(ACTION_TOGGLE_PLAYING_SONG_CENTERING, toggle_playing_song_centering) \
    XX(ACTION_UPDATE_DATABASE, update_database)                             \
    XX(ACTION_JUMP_TO_PLAYING_SONG, jump_to_playing_song)                   \
    XX(ACTION_TOGGLE_REPEAT, toggle_repeat)                                 \
    XX(ACTION_SHUFFLE, shuffle)                                             \
    XX(ACTION_TOGGLE_RANDOM, toggle_random)                                 \
    XX(ACTION_START_SEARCHING, start_searching)                             \
    XX(ACTION_SAVE_TAG_CHANGES, save_tag_changes)                           \
    XX(ACTION_TOGGLE_SINGLE, toggle_single)                                 \
    XX(ACTION_TOGGLE_CONSUME, toggle_consume)                               \
    XX(ACTION_TOGGLE_CROSSFADE, toggle_crossfade)                           \
    XX(ACTION_SET_CROSSFADE, set_crossfade)                                 \
    XX(ACTION_SET_VOLUME, set_volume)                                       \
    XX(ACTION_ENTER_DIRECTORY, enter_directory)                             \
    XX(ACTION_EDIT_SONG, edit_song)                                         \
    XX(ACTION_EDIT_LIBRARY_TAG, edit_library_tag)                           \
    XX(ACTION_EDIT_LIBRARY_ALBUM, edit_library_album)                       \
    XX(ACTION_EDIT_DIRECTORY_NAME, edit_directory_name)                     \
    XX(ACTION_EDIT_PLAYLIST_NAME, edit_playlist_name)                       \
    XX(ACTION_EDIT_LYRICS, edit_lyrics)                                     \
    XX(ACTION_JUMP_TO_BROWSER, jump_to_browser)                             \
    XX(ACTION_JUMP_TO_MEDIA_LIBRARY, jump_to_media_library)                 \
    XX(ACTION_JUMP_TO_PLAYLIST_EDITOR, jump_to_playlist_edit)               \
    XX(ACTION_TOGGLE_SCREEN_LOCK, toggle_screen_lock)                       \
    XX(ACTION_JUMP_TO_TAG_EDIT, jump_to_tag_edit)                           \
    XX(ACTION_JUMP_TO_POSITION_IN_SONG, jump_to_position_in_song)           \
    XX(ACTION_SELECT_ITEM, select_item)                                     \
    XX(ACTION_SELECT_RANGE, select_range)                                   \
    XX(ACTION_REVERSE_SELECTION, reverse_selection)                         \
    XX(ACTION_REMOVE_SELECTION, remove_selection)                           \
    XX(ACTION_SELECT_ALBUM, select_album)                                   \
    XX(ACTION_SELECT_FOUND_ITEMS, select_found_items)                       \
    XX(ACTION_ADD_SELECTED_ITEMS, add_selected_items)                       \
    XX(ACTION_CROP_MAIN_PLAYLIST, crop_main_playlist)                       \
    XX(ACTION_CROP_PLAYLIST, crop_playlist)                                 \
    XX(ACTION_CLEAR_MAIN_PLAYLIST, clear_main_playlist)                     \
    XX(ACTION_CLEAR_PLAYLIST, clear_playlist)                               \
    XX(ACTION_SORT_PLAYLIST, sort_playlist)                                 \
    XX(ACTION_REVERSE_PLAYLIST, reverse_playlist)                           \
    XX(ACTION_APPLY_FILTER, apply_filter)                                   \
    XX(ACTION_FIND, find)                                                   \
    XX(ACTION_FIND_ITEM_FORWARD, find_item_forward)                         \
    XX(ACTION_FIND_ITEM_BACKWARD, find_item_backward)                       \
    XX(ACTION_NEXT_FOUND_ITEM, next_found_item)                             \
    XX(ACTION_PREVIOUS_FOUND_ITEM, previous_found_item)                     \
    XX(ACTION_TOGGLE_FIND_MODE, toggle_find_mode)                           \
    XX(ACTION_TOGGLE_REPLAY_GAIN_MODE, toggle_replay_gain_mode)             \
    XX(ACTION_TOGGLE_ADD_MODE, toggle_add_mode)                             \
    XX(ACTION_TOGGLE_MOUSE, toggle_mouse)                                   \
    XX(ACTION_TOGGLE_BITRATE_VISIBILITY, toggle_bitrate_visibility)         \
    XX(ACTION_ADD_RANDOM_ITEMS, add_random_items)                           \
    XX(ACTION_TOGGLE_BROWSER_SORT_MODE, toggle_browser_sort_mode)           \
    XX(ACTION_TOGGLE_LIBRARY_TAG_TYPE, toggle_library_tag_type)             \
    XX(ACTION_TOGGLE_MEDIA_LIBRARY_SORT_MODE,                               \
       toggle_media_library_sort_mode)                                      \
    XX(ACTION_FETCH_LYRICS_IN_BACKGROUND, fetch_lyrics_in_background)       \
    XX(ACTION_REFETCH_LYRICS, refetch_lyrics)                               \
    XX(ACTION_SET_SELECTED_ITEMS_PRIORITY, set_selected_items_priority)     \
    XX(ACTION_TOGGLE_OUTPUT, toggle_output)                                 \
    XX(ACTION_TOGGLE_VISUALIZATION_TYPE, toggle_visualization_type)         \
    XX(ACTION_SHOW_SONG_INFO, show_song_info)                               \
    XX(ACTION_SHOW_ARTIST_INFO, show_artist_info)                           \
    XX(ACTION_SHOW_LYRICS, show_lyrics)                                     \
    XX(ACTION_QUIT, quit)                                                   \
    XX(ACTION_NEXT_SCREEN, next_screen)                                     \
    XX(ACTION_PREVIOUS_SCREEN, previous_screen)                             \
    XX(ACTION_SHOW_HELP, show_help)                                         \
    XX(ACTION_SHOW_PLAYLIST, show_playlist)                                 \
    XX(ACTION_SHOW_BROWSER, show_browser)                                   \
    XX(ACTION_CHANGE_BROWSE_MODE, change_browse_mode)                       \
    XX(ACTION_SHOW_SEARCH_ENGINE, show_search_engine)                       \
    XX(ACTION_RESET_SEARCH_ENGINE, reset_search_engine)                     \
    XX(ACTION_SHOW_MEDIA_LIBRARY, show_media_library)                       \
    XX(ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE,                            \
       toggle_media_library_columns_mode)                                   \
    XX(ACTION_SHOW_PLAYLIST_EDITOR, show_playlist_edit)                     \
    XX(ACTION_SHOW_TAG_EDIT, show_tag_edit)                                 \
    XX(ACTION_SHOW_OUTPUTS, show_outputs)                                   \
    XX(ACTION_SHOW_VISUALIZER, show_visualizer)                             \
    XX(ACTION_SHOW_SERVER_INFO, show_server_info)

#define ENUM_NAME ActionType
#define ENUM_PREFIX_ ACTION_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS ACTION_TYPE_FIELDS(XX)
#include "cbase/xenums.c"

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
bool ncm_action_immediate_command_prompt_should_stop(StrBuilder *, char *,
                                                     int32);

#endif /* ACTIONS_H */
