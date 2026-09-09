#if !defined(ACTIONS_H)
#define ACTIONS_H

#include "cbase.h"

#include "c/ncm_c.h"

enum NcmActionType {
    ACTION_MACRO_UTILITY = -1,
    ACTION_DUMMY,
    ACTION_UPDATE_ENVIRONMENT,
    ACTION_MOUSE_EVENT,
    ACTION_SCROLL_UP,
    ACTION_SCROLL_DOWN,
    ACTION_SCROLL_UP_ARTIST,
    ACTION_SCROLL_UP_ALBUM,
    ACTION_SCROLL_DOWN_ARTIST,
    ACTION_SCROLL_DOWN_ALBUM,
    ACTION_PAGE_UP,
    ACTION_PAGE_DOWN,
    ACTION_MOVE_HOME,
    ACTION_MOVE_END,
    ACTION_TOGGLE_INTERFACE,
    ACTION_JUMP_TO_PARENT_DIRECTORY,
    ACTION_RUN_ACTION,
    ACTION_PREVIOUS_COLUMN,
    ACTION_NEXT_COLUMN,
    ACTION_MASTER_SCREEN,
    ACTION_SLAVE_SCREEN,
    ACTION_VOLUME_UP,
    ACTION_VOLUME_DOWN,
    ACTION_ADD_ITEM_TO_PLAYLIST,
    ACTION_PLAY_ITEM,
    ACTION_DELETE_PLAYLIST_ITEMS,
    ACTION_DELETE_STORED_PLAYLIST,
    ACTION_DELETE_BROWSER_ITEMS,
    ACTION_REPLAY_SONG,
    ACTION_PREVIOUS,
    ACTION_NEXT,
    ACTION_PAUSE,
    ACTION_STOP,
    ACTION_PLAY,
    ACTION_EXECUTE_COMMAND,
    ACTION_SAVE_PLAYLIST,
    ACTION_MOVE_SORT_ORDER_UP,
    ACTION_MOVE_SORT_ORDER_DOWN,
    ACTION_MOVE_SELECTED_ITEMS_UP,
    ACTION_MOVE_SELECTED_ITEMS_DOWN,
    ACTION_MOVE_SELECTED_ITEMS_TO,
    ACTION_ADD,
    ACTION_LOAD,
    ACTION_SEEK_FORWARD,
    ACTION_SEEK_BACKWARD,
    ACTION_TOGGLE_DISPLAY_MODE,
    ACTION_TOGGLE_SEPARATORS_BETWEEN_ALBUMS,
    ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE,
    ACTION_TOGGLE_LYRICS_FETCHER,
    ACTION_TOGGLE_FETCHING_LYRICS_IN_BACKGROUND,
    ACTION_TOGGLE_PLAYING_SONG_CENTERING,
    ACTION_UPDATE_DATABASE,
    ACTION_JUMP_TO_PLAYING_SONG,
    ACTION_TOGGLE_REPEAT,
    ACTION_SHUFFLE,
    ACTION_TOGGLE_RANDOM,
    ACTION_START_SEARCHING,
    ACTION_SAVE_TAG_CHANGES,
    ACTION_TOGGLE_SINGLE,
    ACTION_TOGGLE_CONSUME,
    ACTION_TOGGLE_CROSSFADE,
    ACTION_SET_CROSSFADE,
    ACTION_SET_VOLUME,
    ACTION_ENTER_DIRECTORY,
    ACTION_EDIT_SONG,
    ACTION_EDIT_LIBRARY_TAG,
    ACTION_EDIT_LIBRARY_ALBUM,
    ACTION_EDIT_DIRECTORY_NAME,
    ACTION_EDIT_PLAYLIST_NAME,
    ACTION_EDIT_LYRICS,
    ACTION_JUMP_TO_BROWSER,
    ACTION_JUMP_TO_MEDIA_LIBRARY,
    ACTION_JUMP_TO_PLAYLIST_EDITOR,
    ACTION_TOGGLE_SCREEN_LOCK,
    ACTION_JUMP_TO_TAG_EDIT,
    ACTION_JUMP_TO_POSITION_IN_SONG,
    ACTION_SELECT_ITEM,
    ACTION_SELECT_RANGE,
    ACTION_REVERSE_SELECTION,
    ACTION_REMOVE_SELECTION,
    ACTION_SELECT_ALBUM,
    ACTION_SELECT_FOUND_ITEMS,
    ACTION_ADD_SELECTED_ITEMS,
    ACTION_CROP_MAIN_PLAYLIST,
    ACTION_CROP_PLAYLIST,
    ACTION_CLEAR_MAIN_PLAYLIST,
    ACTION_CLEAR_PLAYLIST,
    ACTION_SORT_PLAYLIST,
    ACTION_REVERSE_PLAYLIST,
    ACTION_APPLY_FILTER,
    ACTION_FIND,
    ACTION_FIND_ITEM_FORWARD,
    ACTION_FIND_ITEM_BACKWARD,
    ACTION_NEXT_FOUND_ITEM,
    ACTION_PREVIOUS_FOUND_ITEM,
    ACTION_TOGGLE_FIND_MODE,
    ACTION_TOGGLE_REPLAY_GAIN_MODE,
    ACTION_TOGGLE_ADD_MODE,
    ACTION_TOGGLE_MOUSE,
    ACTION_TOGGLE_BITRATE_VISIBILITY,
    ACTION_ADD_RANDOM_ITEMS,
    ACTION_TOGGLE_BROWSER_SORT_MODE,
    ACTION_TOGGLE_LIBRARY_TAG_TYPE,
    ACTION_TOGGLE_MEDIA_LIBRARY_SORT_MODE,
    ACTION_FETCH_LYRICS_IN_BACKGROUND,
    ACTION_REFETCH_LYRICS,
    ACTION_SET_SELECTED_ITEMS_PRIORITY,
    ACTION_TOGGLE_OUTPUT,
    ACTION_TOGGLE_VISUALIZATION_TYPE,
    ACTION_SHOW_SONG_INFO,
    ACTION_SHOW_ARTIST_INFO,
    ACTION_SHOW_LYRICS,
    ACTION_QUIT,
    ACTION_NEXT_SCREEN,
    ACTION_PREVIOUS_SCREEN,
    ACTION_SHOW_HELP,
    ACTION_SHOW_PLAYLIST,
    ACTION_SHOW_BROWSER,
    ACTION_CHANGE_BROWSE_MODE,
    ACTION_SHOW_SEARCH_ENGINE,
    ACTION_RESET_SEARCH_ENGINE,
    ACTION_SHOW_MEDIA_LIBRARY,
    ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE,
    ACTION_SHOW_PLAYLIST_EDITOR,
    ACTION_SHOW_TAG_EDIT,
    ACTION_SHOW_OUTPUTS,
    ACTION_SHOW_VISUALIZER,
    ACTION_SHOW_SERVER_INFO,
    ACTION_LAST,
};

typedef bool (NcmActionCanRunFn)(void *user);
typedef int32 (NcmActionRunFn)(void *user);

#define ACTION_RUNTIME_DEFER 0
#define ACTION_RUNTIME_ALLOW 1
#define ACTION_RUNTIME_DENY -1

typedef int32 (NcmActionRuntimeHook)(enum NcmActionType type, void *user);

typedef struct NcmActionRuntime {
    NcmActionRuntimeHook *can_run_hook;
    NcmActionRuntimeHook *run_hook;
    void *user;
    bool exit_requested;
} NcmActionRuntime;

NcmActionRuntime *ncm_action_runtime_global(void);
bool ncm_action_runtime_exit_requested(NcmActionRuntime *);
void ncm_action_runtime_request_exit(NcmActionRuntime *);
bool ncm_action_runtime_can_run(NcmActionRuntime *, enum NcmActionType);
int32 ncm_action_runtime_run(NcmActionRuntime *, enum NcmActionType);
int32 ncm_action_edit_song(NcmSong *);
int32 ncm_action_show_visualizer(void);
int32 ncm_action_toggle_visualization_type(void);
int32 ncm_action_add_song_to_playlist_with_mode(NcmSong *, bool, int32,
                                                enum SpaceAddMode);
int32 ncm_action_add_song_to_playlist(NcmSong *, bool, int32);

typedef struct NcmActionDef {
    char *name;
    int32 name_len;

    enum NcmActionType type;
    NcmActionCanRunFn *can_run;
    NcmActionRunFn *run;
} NcmActionDef;

NcmActionDef *ncm_action_table_get(NcmActionDef *, int32, enum NcmActionType);
NcmActionDef *ncm_action_table_find(NcmActionDef *, int32 defs_len, char *,
                                    int32 name_len);
NcmActionDef *ncm_action_get(enum NcmActionType);
NcmActionDef *ncm_action_find(char *, int32);
int32 ncm_action_type_parse(char *, int32, enum NcmActionType *);
bool ncm_action_def_can_run(NcmActionDef *, void *);
int32 ncm_action_def_run(NcmActionDef *, void *);
bool ncm_action_can_run(enum NcmActionType, void *);
bool ncm_action_immediate_command_prompt_should_stop(StrBuilder *, char *,
                                                     int32);

#endif /* ACTIONS_H */
