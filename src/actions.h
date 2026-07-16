#if !defined(NCMPCPP_ACTIONS_H)
#define NCMPCPP_ACTIONS_H

#include <stdbool.h>

#include "c/ncm_defs.h"
#include "c/ncm_error.h"
#include "c/ncm_enums.h"
#include "c/ncm_song.h"

#if defined(__cplusplus)
extern "C" {
#endif

enum NcmActionType {
    NCM_ACTION_MACRO_UTILITY = -1,
    NCM_ACTION_DUMMY,
    NCM_ACTION_UPDATE_ENVIRONMENT,
    NCM_ACTION_MOUSE_EVENT,
    NCM_ACTION_SCROLL_UP,
    NCM_ACTION_SCROLL_DOWN,
    NCM_ACTION_SCROLL_UP_ARTIST,
    NCM_ACTION_SCROLL_UP_ALBUM,
    NCM_ACTION_SCROLL_DOWN_ARTIST,
    NCM_ACTION_SCROLL_DOWN_ALBUM,
    NCM_ACTION_PAGE_UP,
    NCM_ACTION_PAGE_DOWN,
    NCM_ACTION_MOVE_HOME,
    NCM_ACTION_MOVE_END,
    NCM_ACTION_TOGGLE_INTERFACE,
    NCM_ACTION_JUMP_TO_PARENT_DIRECTORY,
    NCM_ACTION_RUN_ACTION,
    NCM_ACTION_PREVIOUS_COLUMN,
    NCM_ACTION_NEXT_COLUMN,
    NCM_ACTION_MASTER_SCREEN,
    NCM_ACTION_SLAVE_SCREEN,
    NCM_ACTION_VOLUME_UP,
    NCM_ACTION_VOLUME_DOWN,
    NCM_ACTION_ADD_ITEM_TO_PLAYLIST,
    NCM_ACTION_PLAY_ITEM,
    NCM_ACTION_DELETE_PLAYLIST_ITEMS,
    NCM_ACTION_DELETE_STORED_PLAYLIST,
    NCM_ACTION_DELETE_BROWSER_ITEMS,
    NCM_ACTION_REPLAY_SONG,
    NCM_ACTION_PREVIOUS,
    NCM_ACTION_NEXT,
    NCM_ACTION_PAUSE,
    NCM_ACTION_STOP,
    NCM_ACTION_PLAY,
    NCM_ACTION_EXECUTE_COMMAND,
    NCM_ACTION_SAVE_PLAYLIST,
    NCM_ACTION_MOVE_SORT_ORDER_UP,
    NCM_ACTION_MOVE_SORT_ORDER_DOWN,
    NCM_ACTION_MOVE_SELECTED_ITEMS_UP,
    NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN,
    NCM_ACTION_MOVE_SELECTED_ITEMS_TO,
    NCM_ACTION_ADD,
    NCM_ACTION_LOAD,
    NCM_ACTION_SEEK_FORWARD,
    NCM_ACTION_SEEK_BACKWARD,
    NCM_ACTION_TOGGLE_DISPLAY_MODE,
    NCM_ACTION_TOGGLE_SEPARATORS_BETWEEN_ALBUMS,
    NCM_ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE,
    NCM_ACTION_TOGGLE_LYRICS_FETCHER,
    NCM_ACTION_TOGGLE_FETCHING_LYRICS_IN_BACKGROUND,
    NCM_ACTION_TOGGLE_PLAYING_SONG_CENTERING,
    NCM_ACTION_UPDATE_DATABASE,
    NCM_ACTION_JUMP_TO_PLAYING_SONG,
    NCM_ACTION_TOGGLE_REPEAT,
    NCM_ACTION_SHUFFLE,
    NCM_ACTION_TOGGLE_RANDOM,
    NCM_ACTION_START_SEARCHING,
    NCM_ACTION_SAVE_TAG_CHANGES,
    NCM_ACTION_TOGGLE_SINGLE,
    NCM_ACTION_TOGGLE_CONSUME,
    NCM_ACTION_TOGGLE_CROSSFADE,
    NCM_ACTION_SET_CROSSFADE,
    NCM_ACTION_SET_VOLUME,
    NCM_ACTION_ENTER_DIRECTORY,
    NCM_ACTION_EDIT_SONG,
    NCM_ACTION_EDIT_LIBRARY_TAG,
    NCM_ACTION_EDIT_LIBRARY_ALBUM,
    NCM_ACTION_EDIT_DIRECTORY_NAME,
    NCM_ACTION_EDIT_PLAYLIST_NAME,
    NCM_ACTION_EDIT_LYRICS,
    NCM_ACTION_JUMP_TO_BROWSER,
    NCM_ACTION_JUMP_TO_MEDIA_LIBRARY,
    NCM_ACTION_JUMP_TO_PLAYLIST_EDITOR,
    NCM_ACTION_TOGGLE_SCREEN_LOCK,
    NCM_ACTION_JUMP_TO_TAG_EDITOR,
    NCM_ACTION_JUMP_TO_POSITION_IN_SONG,
    NCM_ACTION_SELECT_ITEM,
    NCM_ACTION_SELECT_RANGE,
    NCM_ACTION_REVERSE_SELECTION,
    NCM_ACTION_REMOVE_SELECTION,
    NCM_ACTION_SELECT_ALBUM,
    NCM_ACTION_SELECT_FOUND_ITEMS,
    NCM_ACTION_ADD_SELECTED_ITEMS,
    NCM_ACTION_CROP_MAIN_PLAYLIST,
    NCM_ACTION_CROP_PLAYLIST,
    NCM_ACTION_CLEAR_MAIN_PLAYLIST,
    NCM_ACTION_CLEAR_PLAYLIST,
    NCM_ACTION_SORT_PLAYLIST,
    NCM_ACTION_REVERSE_PLAYLIST,
    NCM_ACTION_APPLY_FILTER,
    NCM_ACTION_FIND,
    NCM_ACTION_FIND_ITEM_FORWARD,
    NCM_ACTION_FIND_ITEM_BACKWARD,
    NCM_ACTION_NEXT_FOUND_ITEM,
    NCM_ACTION_PREVIOUS_FOUND_ITEM,
    NCM_ACTION_TOGGLE_FIND_MODE,
    NCM_ACTION_TOGGLE_REPLAY_GAIN_MODE,
    NCM_ACTION_TOGGLE_ADD_MODE,
    NCM_ACTION_TOGGLE_MOUSE,
    NCM_ACTION_TOGGLE_BITRATE_VISIBILITY,
    NCM_ACTION_ADD_RANDOM_ITEMS,
    NCM_ACTION_TOGGLE_BROWSER_SORT_MODE,
    NCM_ACTION_TOGGLE_LIBRARY_TAG_TYPE,
    NCM_ACTION_TOGGLE_MEDIA_LIBRARY_SORT_MODE,
    NCM_ACTION_FETCH_LYRICS_IN_BACKGROUND,
    NCM_ACTION_REFETCH_LYRICS,
    NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY,
    NCM_ACTION_TOGGLE_OUTPUT,
    NCM_ACTION_TOGGLE_VISUALIZATION_TYPE,
    NCM_ACTION_SHOW_SONG_INFO,
    NCM_ACTION_SHOW_ARTIST_INFO,
    NCM_ACTION_SHOW_LYRICS,
    NCM_ACTION_QUIT,
    NCM_ACTION_NEXT_SCREEN,
    NCM_ACTION_PREVIOUS_SCREEN,
    NCM_ACTION_SHOW_HELP,
    NCM_ACTION_SHOW_PLAYLIST,
    NCM_ACTION_SHOW_BROWSER,
    NCM_ACTION_CHANGE_BROWSE_MODE,
    NCM_ACTION_SHOW_SEARCH_ENGINE,
    NCM_ACTION_RESET_SEARCH_ENGINE,
    NCM_ACTION_SHOW_MEDIA_LIBRARY,
    NCM_ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE,
    NCM_ACTION_SHOW_PLAYLIST_EDITOR,
    NCM_ACTION_SHOW_TAG_EDITOR,
    NCM_ACTION_SHOW_OUTPUTS,
    NCM_ACTION_SHOW_VISUALIZER,
    NCM_ACTION_SHOW_SERVER_INFO,
    NCM_ACTION_LAST,
};


typedef bool (*NcmActionCanRunFn)(void *user);
typedef bool (*NcmActionRunFn)(void *user);

#define NCM_ACTION_RUNTIME_DEFER 0
#define NCM_ACTION_RUNTIME_ALLOW 1
#define NCM_ACTION_RUNTIME_DENY -1

typedef int32 (*NcmActionRuntimeHook)(enum NcmActionType type,
                                      void *user);

typedef struct NcmActionRuntime {
    NcmActionRuntimeHook can_run_hook;
    NcmActionRuntimeHook run_hook;
    void *user;
    bool exit_requested;
} NcmActionRuntime;

void ncm_action_runtime_init(NcmActionRuntime *runtime);
void ncm_action_runtime_set_hooks(NcmActionRuntime *runtime,
                                  NcmActionRuntimeHook can_run_hook,
                                  NcmActionRuntimeHook run_hook,
                                  void *user);
NcmActionRuntime *ncm_action_runtime_global(void);
bool ncm_action_runtime_exit_requested(NcmActionRuntime *runtime);
void ncm_action_runtime_request_exit(NcmActionRuntime *runtime);
void ncm_action_runtime_clear_exit_request(NcmActionRuntime *runtime);
bool ncm_action_runtime_can_run(NcmActionRuntime *runtime,
                                enum NcmActionType type);
bool ncm_action_runtime_run(NcmActionRuntime *runtime,
                            enum NcmActionType type);
bool ncm_action_current_song(NcmSong *song);
bool ncm_action_edit_song(NcmSong *song);
bool ncm_action_show_visualizer(void);
bool ncm_action_toggle_visualization_type(void);
bool ncm_action_delete_playlist_items(void);
bool ncm_action_crop_main_playlist(void);
bool ncm_action_add_song_to_playlist_with_mode(
    NcmSong *song, bool play, int32 position,
    enum SpaceAddMode space_add_mode);
bool ncm_action_add_song_to_playlist(NcmSong *song, bool play,
                                     int32 position);

typedef struct NcmActionDef {
    char *name;

    enum NcmActionType type;
    NcmActionCanRunFn can_run;
    NcmActionRunFn run;
} NcmActionDef;

int32 ncm_action_count(void);
NcmActionDef *ncm_action_table(void);
bool ncm_action_table_validate(NcmActionDef *defs, int32 defs_len,
                               NcmError *error);
NcmActionDef *ncm_action_table_get(NcmActionDef *defs, int32 defs_len,
                                   enum NcmActionType type);
NcmActionDef *ncm_action_table_find(NcmActionDef *defs, int32 defs_len,
                                    char *name, int32 name_len);
bool ncm_action_validate(NcmError *error);
NcmActionDef *ncm_action_get(enum NcmActionType type);
NcmActionDef *ncm_action_find(char *name, int32 name_len);
char *ncm_action_type_name(enum NcmActionType type);
bool ncm_action_type_parse(char *name, int32 name_len,
                           enum NcmActionType *type);
bool ncm_action_def_can_run(NcmActionDef *action, void *user);
bool ncm_action_def_run(NcmActionDef *action, void *user);
bool ncm_action_set_callbacks(enum NcmActionType type,
                              NcmActionCanRunFn can_run,
                              NcmActionRunFn run);
bool ncm_action_can_run(enum NcmActionType type, void *user);
bool ncm_action_run(enum NcmActionType type, void *user);
bool ncm_action_immediate_command_prompt_should_stop(
    NcmBuffer *previous, char *text, int32 text_len);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_ACTIONS_H */
