#include "actions.h"

#include <string.h>

#include "app_controller.h"
#include "global.h"
#include "settings.h"
#include "screens/native_c_screens.h"
#include "screens/screen_type.h"
#include "status.h"
#include "statusbar.h"

#include "cbase/base_macros.h"

static bool ncm_action_can_run_always(void *user);
static bool ncm_action_run_noop(void *user);
static bool ncm_action_run_unimplemented(void *user);
static int32 ncm_action_name_len(char *name);
static bool ncm_action_name_equals(char *left, int32 left_len,
                                   char *right);

static NcmActionDef action_defs[] = {
    { (char *)"dummy", NCM_ACTION_DUMMY, ncm_action_can_run_always, ncm_action_run_noop },
    { (char *)"update_environment", NCM_ACTION_UPDATE_ENVIRONMENT, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"mouse_event", NCM_ACTION_MOUSE_EVENT, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"scroll_up", NCM_ACTION_SCROLL_UP, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"scroll_down", NCM_ACTION_SCROLL_DOWN, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"scroll_up_artist", NCM_ACTION_SCROLL_UP_ARTIST, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"scroll_up_album", NCM_ACTION_SCROLL_UP_ALBUM, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"scroll_down_artist", NCM_ACTION_SCROLL_DOWN_ARTIST, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"scroll_down_album", NCM_ACTION_SCROLL_DOWN_ALBUM, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"page_up", NCM_ACTION_PAGE_UP, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"page_down", NCM_ACTION_PAGE_DOWN, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"move_home", NCM_ACTION_MOVE_HOME, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"move_end", NCM_ACTION_MOVE_END, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_interface", NCM_ACTION_TOGGLE_INTERFACE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"jump_to_parent_directory", NCM_ACTION_JUMP_TO_PARENT_DIRECTORY, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"run_action", NCM_ACTION_RUN_ACTION, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"previous_column", NCM_ACTION_PREVIOUS_COLUMN, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"next_column", NCM_ACTION_NEXT_COLUMN, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"master_screen", NCM_ACTION_MASTER_SCREEN, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"slave_screen", NCM_ACTION_SLAVE_SCREEN, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"volume_up", NCM_ACTION_VOLUME_UP, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"volume_down", NCM_ACTION_VOLUME_DOWN, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"add_item_to_playlist", NCM_ACTION_ADD_ITEM_TO_PLAYLIST, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"play_item", NCM_ACTION_PLAY_ITEM, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"delete_playlist_items", NCM_ACTION_DELETE_PLAYLIST_ITEMS, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"delete_stored_playlist", NCM_ACTION_DELETE_STORED_PLAYLIST, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"delete_browser_items", NCM_ACTION_DELETE_BROWSER_ITEMS, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"replay_song", NCM_ACTION_REPLAY_SONG, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"previous", NCM_ACTION_PREVIOUS, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"next", NCM_ACTION_NEXT, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"pause", NCM_ACTION_PAUSE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"stop", NCM_ACTION_STOP, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"play", NCM_ACTION_PLAY, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"execute_command", NCM_ACTION_EXECUTE_COMMAND, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"save_playlist", NCM_ACTION_SAVE_PLAYLIST, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"move_sort_order_up", NCM_ACTION_MOVE_SORT_ORDER_UP, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"move_sort_order_down", NCM_ACTION_MOVE_SORT_ORDER_DOWN, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"move_selected_items_up", NCM_ACTION_MOVE_SELECTED_ITEMS_UP, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"move_selected_items_down", NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"move_selected_items_to", NCM_ACTION_MOVE_SELECTED_ITEMS_TO, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"add", NCM_ACTION_ADD, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"load", NCM_ACTION_LOAD, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"seek_forward", NCM_ACTION_SEEK_FORWARD, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"seek_backward", NCM_ACTION_SEEK_BACKWARD, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_display_mode", NCM_ACTION_TOGGLE_DISPLAY_MODE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_separators_between_albums", NCM_ACTION_TOGGLE_SEPARATORS_BETWEEN_ALBUMS, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_lyrics_update_on_song_change", NCM_ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_lyrics_fetcher", NCM_ACTION_TOGGLE_LYRICS_FETCHER, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_fetching_lyrics_in_background", NCM_ACTION_TOGGLE_FETCHING_LYRICS_IN_BACKGROUND, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_playing_song_centering", NCM_ACTION_TOGGLE_PLAYING_SONG_CENTERING, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"update_database", NCM_ACTION_UPDATE_DATABASE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"jump_to_playing_song", NCM_ACTION_JUMP_TO_PLAYING_SONG, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_repeat", NCM_ACTION_TOGGLE_REPEAT, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"shuffle", NCM_ACTION_SHUFFLE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_random", NCM_ACTION_TOGGLE_RANDOM, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"start_searching", NCM_ACTION_START_SEARCHING, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"save_tag_changes", NCM_ACTION_SAVE_TAG_CHANGES, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_single", NCM_ACTION_TOGGLE_SINGLE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_consume", NCM_ACTION_TOGGLE_CONSUME, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_crossfade", NCM_ACTION_TOGGLE_CROSSFADE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"set_crossfade", NCM_ACTION_SET_CROSSFADE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"set_volume", NCM_ACTION_SET_VOLUME, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"enter_directory", NCM_ACTION_ENTER_DIRECTORY, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"edit_song", NCM_ACTION_EDIT_SONG, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"edit_library_tag", NCM_ACTION_EDIT_LIBRARY_TAG, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"edit_library_album", NCM_ACTION_EDIT_LIBRARY_ALBUM, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"edit_directory_name", NCM_ACTION_EDIT_DIRECTORY_NAME, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"edit_playlist_name", NCM_ACTION_EDIT_PLAYLIST_NAME, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"edit_lyrics", NCM_ACTION_EDIT_LYRICS, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"jump_to_browser", NCM_ACTION_JUMP_TO_BROWSER, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"jump_to_media_library", NCM_ACTION_JUMP_TO_MEDIA_LIBRARY, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"jump_to_playlist_editor", NCM_ACTION_JUMP_TO_PLAYLIST_EDITOR, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_screen_lock", NCM_ACTION_TOGGLE_SCREEN_LOCK, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"jump_to_tag_editor", NCM_ACTION_JUMP_TO_TAG_EDITOR, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"jump_to_position_in_song", NCM_ACTION_JUMP_TO_POSITION_IN_SONG, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"select_item", NCM_ACTION_SELECT_ITEM, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"select_range", NCM_ACTION_SELECT_RANGE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"reverse_selection", NCM_ACTION_REVERSE_SELECTION, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"remove_selection", NCM_ACTION_REMOVE_SELECTION, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"select_album", NCM_ACTION_SELECT_ALBUM, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"select_found_items", NCM_ACTION_SELECT_FOUND_ITEMS, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"add_selected_items", NCM_ACTION_ADD_SELECTED_ITEMS, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"crop_main_playlist", NCM_ACTION_CROP_MAIN_PLAYLIST, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"crop_playlist", NCM_ACTION_CROP_PLAYLIST, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"clear_main_playlist", NCM_ACTION_CLEAR_MAIN_PLAYLIST, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"clear_playlist", NCM_ACTION_CLEAR_PLAYLIST, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"sort_playlist", NCM_ACTION_SORT_PLAYLIST, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"reverse_playlist", NCM_ACTION_REVERSE_PLAYLIST, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"apply_filter", NCM_ACTION_APPLY_FILTER, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"find", NCM_ACTION_FIND, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"find_item_forward", NCM_ACTION_FIND_ITEM_FORWARD, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"find_item_backward", NCM_ACTION_FIND_ITEM_BACKWARD, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"next_found_item", NCM_ACTION_NEXT_FOUND_ITEM, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"previous_found_item", NCM_ACTION_PREVIOUS_FOUND_ITEM, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_find_mode", NCM_ACTION_TOGGLE_FIND_MODE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_replay_gain_mode", NCM_ACTION_TOGGLE_REPLAY_GAIN_MODE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_add_mode", NCM_ACTION_TOGGLE_ADD_MODE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_mouse", NCM_ACTION_TOGGLE_MOUSE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_bitrate_visibility", NCM_ACTION_TOGGLE_BITRATE_VISIBILITY, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"add_random_items", NCM_ACTION_ADD_RANDOM_ITEMS, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_browser_sort_mode", NCM_ACTION_TOGGLE_BROWSER_SORT_MODE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_library_tag_type", NCM_ACTION_TOGGLE_LIBRARY_TAG_TYPE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_media_library_sort_mode", NCM_ACTION_TOGGLE_MEDIA_LIBRARY_SORT_MODE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"fetch_lyrics_in_background", NCM_ACTION_FETCH_LYRICS_IN_BACKGROUND, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"refetch_lyrics", NCM_ACTION_REFETCH_LYRICS, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"set_selected_items_priority", NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_output", NCM_ACTION_TOGGLE_OUTPUT, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_visualization_type", NCM_ACTION_TOGGLE_VISUALIZATION_TYPE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"show_song_info", NCM_ACTION_SHOW_SONG_INFO, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"show_artist_info", NCM_ACTION_SHOW_ARTIST_INFO, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"show_lyrics", NCM_ACTION_SHOW_LYRICS, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"quit", NCM_ACTION_QUIT, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"next_screen", NCM_ACTION_NEXT_SCREEN, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"previous_screen", NCM_ACTION_PREVIOUS_SCREEN, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"show_help", NCM_ACTION_SHOW_HELP, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"show_playlist", NCM_ACTION_SHOW_PLAYLIST, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"show_browser", NCM_ACTION_SHOW_BROWSER, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"change_browse_mode", NCM_ACTION_CHANGE_BROWSE_MODE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"show_search_engine", NCM_ACTION_SHOW_SEARCH_ENGINE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"reset_search_engine", NCM_ACTION_RESET_SEARCH_ENGINE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"show_media_library", NCM_ACTION_SHOW_MEDIA_LIBRARY, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"toggle_media_library_columns_mode", NCM_ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"show_playlist_editor", NCM_ACTION_SHOW_PLAYLIST_EDITOR, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"show_tag_editor", NCM_ACTION_SHOW_TAG_EDITOR, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"show_outputs", NCM_ACTION_SHOW_OUTPUTS, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"show_visualizer", NCM_ACTION_SHOW_VISUALIZER, ncm_action_can_run_always, ncm_action_run_unimplemented },
    { (char *)"show_server_info", NCM_ACTION_SHOW_SERVER_INFO, ncm_action_can_run_always, ncm_action_run_unimplemented },
};

static bool
ncm_action_can_run_always(void *user) {
    (void)user;
    return true;
}

static bool
ncm_action_run_noop(void *user) {
    (void)user;
    return true;
}

static bool
ncm_action_run_unimplemented(void *user) {
    (void)user;
    return false;
}

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
    return memcmp(left, right, (size_t)left_len) == 0;
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
static bool action_runtime_mpd_simple(
    bool (*func)(NcmMpdClient *client, NcmError *error));
static bool action_runtime_mpd_toggle(
    bool (*func)(NcmMpdClient *client, bool mode, NcmError *error),
    bool current);
static bool action_runtime_volume(int32 change);
static bool action_runtime_update_database(void);
static bool action_runtime_replay_song(void);
static bool action_runtime_update_environment(void);
static bool action_runtime_toggle_crossfade(void);

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
        native_c_screen_sort_playlist_dialog_switch_to();
        return true;
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
        native_c_screen_visualizer_register();
        native_c_screen_visualizer_switch_to();
        return true;
#endif
    case NCM_SCREEN_TYPE_UNKNOWN:
    case NCM_SCREEN_TYPE_LAST:
        break;
    }

    return false;
}

static bool
action_runtime_switch_to_next_screen(bool reverse) {
    ScreenTypeArray *sequence;
    NcScreen *current;
    int32 current_index;
    int32 next_index;

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
        current_index = 0;
    }

    if (reverse) {
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
action_runtime_mpd_error(NcmError *error) {
    if ((error != NULL) && ncm_error_is_set(error)) {
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    error->message);
    }
    return false;
}

static bool
action_runtime_mpd_simple(
    bool (*func)(NcmMpdClient *client, NcmError *error)) {
    NcmError error;

    ncm_error_clear(&error);
    if (!func(&global_mpd, &error)) {
        return action_runtime_mpd_error(&error);
    }
    (void)ncm_status_update(&global_mpd, -1, &error);
    return true;
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
    (void)ncm_status_update(&global_mpd, -1, &error);
    return true;
}

static bool
action_runtime_volume(int32 change) {
    NcmError error;

    ncm_error_clear(&error);
    if (!ncm_mpd_client_change_volume(&global_mpd, change, &error)) {
        return action_runtime_mpd_error(&error);
    }
    (void)ncm_status_update(&global_mpd, -1, &error);
    return true;
}

static bool
action_runtime_update_database(void) {
    NcmError error;

    ncm_error_clear(&error);
    if (!ncm_mpd_client_update_directory(&global_mpd, NULL, NULL,
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
    (void)ncm_status_update(&global_mpd, -1, &error);
    return true;
}

static bool
action_runtime_update_environment(void) {
    NcmError error;
    int32 flags;

    ncm_error_clear(&error);
    ncm_status_trace(&global_mpd, true, true, &error);
    app_controller_refresh_current_window();

    flags = 0;
    if (ncm_mpd_client_connected(&global_mpd)) {
        if (ncm_mpd_client_noidle(&global_mpd, &flags, &error)) {
            if (flags != 0) {
                (void)ncm_status_update(&global_mpd, flags, &error);
            }
        }
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
    case NCM_ACTION_SHOW_HELP:
    case NCM_ACTION_SHOW_PLAYLIST:
    case NCM_ACTION_SHOW_BROWSER:
    case NCM_ACTION_SHOW_SEARCH_ENGINE:
    case NCM_ACTION_SHOW_MEDIA_LIBRARY:
    case NCM_ACTION_SHOW_PLAYLIST_EDITOR:
    case NCM_ACTION_SHOW_SERVER_INFO:
    case NCM_ACTION_SHOW_SONG_INFO:
    case NCM_ACTION_TOGGLE_SCREEN_LOCK:
    case NCM_ACTION_TOGGLE_PLAYING_SONG_CENTERING:
    case NCM_ACTION_TOGGLE_MOUSE:
    case NCM_ACTION_TOGGLE_BITRATE_VISIBILITY:
    case NCM_ACTION_TOGGLE_ADD_MODE:
    case NCM_ACTION_TOGGLE_LYRICS_FETCHER:
    case NCM_ACTION_TOGGLE_FETCHING_LYRICS_IN_BACKGROUND:
    case NCM_ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE:
        return true;
    case NCM_ACTION_PAUSE:
        return ncm_status_state_player() != NCM_STATUS_PLAYER_STOP;
    case NCM_ACTION_PLAY:
    case NCM_ACTION_STOP:
    case NCM_ACTION_NEXT:
    case NCM_ACTION_PREVIOUS:
    case NCM_ACTION_VOLUME_UP:
    case NCM_ACTION_VOLUME_DOWN:
        return ncm_mpd_client_connected(&global_mpd);
    case NCM_ACTION_REPLAY_SONG:
        return ncm_mpd_client_connected(&global_mpd)
            && (ncm_status_state_current_song_position() >= 0);
    case NCM_ACTION_TOGGLE_REPEAT:
    case NCM_ACTION_TOGGLE_RANDOM:
    case NCM_ACTION_TOGGLE_SINGLE:
    case NCM_ACTION_TOGGLE_CONSUME:
    case NCM_ACTION_TOGGLE_CROSSFADE:
    case NCM_ACTION_SHUFFLE:
    case NCM_ACTION_UPDATE_DATABASE:
        return ncm_mpd_client_connected(&global_mpd);
#if defined(ENABLE_OUTPUTS)
    case NCM_ACTION_SHOW_OUTPUTS:
    case NCM_ACTION_TOGGLE_OUTPUT:
        return true;
#endif
#if defined(ENABLE_VISUALIZER)
    case NCM_ACTION_SHOW_VISUALIZER:
    case NCM_ACTION_TOGGLE_VISUALIZATION_TYPE:
        return true;
#endif
#if defined(HAVE_TAGLIB_H)
    case NCM_ACTION_SHOW_TAG_EDITOR:
        return true;
#endif
    case NCM_ACTION_CHANGE_BROWSE_MODE:
        return action_runtime_current_screen_is(NCM_SCREEN_TYPE_BROWSER);
    case NCM_ACTION_RESET_SEARCH_ENGINE:
        return action_runtime_current_screen_is(
            NCM_SCREEN_TYPE_SEARCH_ENGINE);
    case NCM_ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE:
        return action_runtime_current_screen_is(
            NCM_SCREEN_TYPE_MEDIA_LIBRARY);
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
    case NCM_ACTION_SCROLL_UP:
        app_controller_scroll_current_screen(NC_SCROLL_UP);
        return true;
    case NCM_ACTION_SCROLL_DOWN:
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
        Config.header_visibility = !Config.header_visibility;
        Config.statusbar_visibility = !Config.statusbar_visibility;
        app_controller_request_visible_screens_resize();
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
    case NCM_ACTION_TOGGLE_REPEAT:
        return action_runtime_mpd_toggle(ncm_mpd_client_set_repeat,
                                         ncm_status_state_repeat());
    case NCM_ACTION_TOGGLE_RANDOM:
        return action_runtime_mpd_toggle(ncm_mpd_client_set_random,
                                         ncm_status_state_random());
    case NCM_ACTION_TOGGLE_SINGLE:
        return action_runtime_mpd_toggle(ncm_mpd_client_set_single,
                                         ncm_status_state_single());
    case NCM_ACTION_TOGGLE_CONSUME:
        return action_runtime_mpd_toggle(ncm_mpd_client_set_consume,
                                         ncm_status_state_consume());
    case NCM_ACTION_TOGGLE_CROSSFADE:
        return action_runtime_toggle_crossfade();
    case NCM_ACTION_SHUFFLE:
        return action_runtime_mpd_simple(ncm_mpd_client_shuffle);
    case NCM_ACTION_UPDATE_DATABASE:
        return action_runtime_update_database();
    case NCM_ACTION_TOGGLE_PLAYING_SONG_CENTERING:
        Config.autocenter_mode = !Config.autocenter_mode;
        return true;
    case NCM_ACTION_TOGGLE_MOUSE:
        Config.mouse_support = !Config.mouse_support;
        return true;
    case NCM_ACTION_TOGGLE_BITRATE_VISIBILITY:
        Config.display_bitrate = !Config.display_bitrate;
        return true;
    case NCM_ACTION_TOGGLE_ADD_MODE:
        if (Config.space_add_mode == NCM_SPACE_ADD_MODE_ADD_REMOVE) {
            Config.space_add_mode = NCM_SPACE_ADD_MODE_ALWAYS_ADD;
        } else {
            Config.space_add_mode = NCM_SPACE_ADD_MODE_ADD_REMOVE;
        }
        return true;
    case NCM_ACTION_TOGGLE_LYRICS_FETCHER:
        Config.lyrics_db += 1;
        if (Config.lyrics_db >= (uint32)Config.lyrics_fetchers.fetchers.len) {
            Config.lyrics_db = 0;
        }
        return true;
    case NCM_ACTION_TOGGLE_FETCHING_LYRICS_IN_BACKGROUND:
        Config.fetch_lyrics_in_background =
            !Config.fetch_lyrics_in_background;
        return true;
    case NCM_ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE:
        Config.now_playing_lyrics = !Config.now_playing_lyrics;
        return true;
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
    case NCM_ACTION_SHOW_SEARCH_ENGINE:
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_SEARCH_ENGINE);
    case NCM_ACTION_SHOW_MEDIA_LIBRARY:
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_MEDIA_LIBRARY);
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
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_VISUALIZER);
    case NCM_ACTION_TOGGLE_VISUALIZATION_TYPE:
        native_visualizer_screen_toggle_type(native_c_screen_visualizer());
        return true;
#endif
#if defined(HAVE_TAGLIB_H)
    case NCM_ACTION_SHOW_TAG_EDITOR:
        return action_runtime_switch_to_screen(NCM_SCREEN_TYPE_TAG_EDITOR);
#endif
    case NCM_ACTION_TOGGLE_SCREEN_LOCK:
        if (app_controller_locked_screen() != NULL) {
            app_controller_unlock_screen();
        } else {
            (void)app_controller_lock_current_screen();
        }
        return true;
    case NCM_ACTION_CHANGE_BROWSE_MODE:
    case NCM_ACTION_RESET_SEARCH_ENGINE:
    case NCM_ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE:
        return true;
    default:
        return false;
    }
}
