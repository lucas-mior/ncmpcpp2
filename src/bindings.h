#if !defined(NCMPCPP_BINDINGS_H)
#define NCMPCPP_BINDINGS_H

#include <stdbool.h>

#include "c/ncm_defs.h"
#include "c/ncm_error.h"
#include "curses/nc_window.h"
#include "screens/screen_type.h"

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

enum NcmBindingActionKind {
    NCM_BINDING_ACTION_NORMAL,
    NCM_BINDING_ACTION_PUSH_CHARACTERS,
    NCM_BINDING_ACTION_REQUIRE_SCREEN,
    NCM_BINDING_ACTION_REQUIRE_RUNNABLE,
    NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND,
    NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND,
};

typedef struct NcmBindingAction {
    char *argument;
    NcKey *keys;

    int32 argument_len;
    int32 argument_cap;
    int32 keys_len;
    int32 keys_cap;

    enum NcmActionType type;
    enum NcmBindingActionKind kind;
    enum ScreenType screen_type;
} NcmBindingAction;

typedef struct NcmBinding {
    NcmBindingAction *actions;
    int32 actions_len;
    int32 actions_cap;
} NcmBinding;

typedef struct NcmCommand {
    char *name;
    int32 name_len;
    int32 name_cap;
    NcmBinding binding;
    bool immediate;
} NcmCommand;

typedef struct NcmKeyBindings {
    NcmBinding *bindings;
    int32 bindings_len;
    int32 bindings_cap;
    NcKey key;
} NcmKeyBindings;

typedef struct NcmBindingSlice {
    NcmBinding *data;
    int32 len;
} NcmBindingSlice;

typedef bool (*NcmBindingActionRunner)(NcmBindingAction *action,
                                       void *user);

typedef struct NcmBindingsConfiguration {
    NcmCommand *commands;
    NcmKeyBindings *keys;
    int32 commands_len;
    int32 commands_cap;
    int32 keys_len;
    int32 keys_cap;
} NcmBindingsConfiguration;

extern NcmBindingsConfiguration Bindings;

void ncm_binding_action_init(NcmBindingAction *action);
void ncm_binding_action_destroy(NcmBindingAction *action);
bool ncm_binding_action_copy(NcmBindingAction *dest,
                             NcmBindingAction *source);

void ncm_binding_init(NcmBinding *binding);
void ncm_binding_destroy(NcmBinding *binding);
void ncm_binding_clear(NcmBinding *binding);
bool ncm_binding_append_action(NcmBinding *binding,
                               NcmBindingAction *action);
bool ncm_binding_copy(NcmBinding *dest, NcmBinding *source);
bool ncm_binding_is_single(NcmBinding *binding);
bool ncm_binding_execute(NcmBinding *binding,
                         NcmBindingActionRunner runner, void *user);

void ncm_command_init(NcmCommand *command);
void ncm_command_destroy(NcmCommand *command);

void ncm_key_bindings_init(NcmKeyBindings *key_bindings);
void ncm_key_bindings_destroy(NcmKeyBindings *key_bindings);

void ncm_bindings_configuration_init(NcmBindingsConfiguration *bindings);
void ncm_bindings_configuration_destroy(NcmBindingsConfiguration *bindings);
void ncm_bindings_configuration_clear(NcmBindingsConfiguration *bindings);
bool ncm_bindings_configuration_read(NcmBindingsConfiguration *bindings,
                                     char *path, int32 path_len,
                                     NcmError *error);
bool ncm_bindings_configuration_read_paths(NcmBindingsConfiguration *bindings,
                                           char **paths, int32 *path_lens,
                                           int32 paths_len,
                                           NcmError *error);
void ncm_bindings_configuration_generate_defaults(
    NcmBindingsConfiguration *bindings);
NcmCommand *ncm_bindings_configuration_find_command(
    NcmBindingsConfiguration *bindings, char *name, int32 name_len);
NcmBindingSlice ncm_bindings_configuration_get(
    NcmBindingsConfiguration *bindings, NcKey key);

char *ncm_action_type_name(enum NcmActionType type);
bool ncm_action_type_parse(char *name, int32 name_len,
                           enum NcmActionType *type);
NcKey ncm_bindings_string_to_key(char *string, int32 string_len);
void ncm_bindings_format_key(NcmBuffer *buffer, NcKey key);
NcKey ncm_read_key(NcWindow *window);
NcKey ncm_bindings_read_key(NcWindow *window);
int32 ncm_bindings_key_name(NcKey key, char *buffer, int32 buffer_len);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_BINDINGS_H */
