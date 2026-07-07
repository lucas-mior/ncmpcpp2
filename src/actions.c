#include "actions.h"

#include <string.h>

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

int32
ncm_action_count(void) {
    return NCM_ARRAY_LEN(action_defs);
}

NcmActionDef *
ncm_action_table(void) {
    return action_defs;
}

NcmActionDef *
ncm_action_get(enum NcmActionType type) {
    for (int32 i = 0; i < NCM_ARRAY_LEN(action_defs); i += 1) {
        if (action_defs[i].type == type) {
            return action_defs + i;
        }
    }
    return 0;
}

NcmActionDef *
ncm_action_find(char *name, int32 name_len) {
    for (int32 i = 0; i < NCM_ARRAY_LEN(action_defs); i += 1) {
        if (ncm_action_name_equals(name, name_len, action_defs[i].name)) {
            return action_defs + i;
        }
    }
    return 0;
}

char *
ncm_action_type_name(enum NcmActionType type) {
    NcmActionDef *action;

    action = ncm_action_get(type);
    if (action == 0) {
        return (char *)"";
    }
    return action->name;
}

bool
ncm_action_type_parse(char *name, int32 name_len,
                      enum NcmActionType *type) {
    NcmActionDef *action;

    action = ncm_action_find(name, name_len);
    if (action == 0) {
        return false;
    }
    *type = action->type;
    return true;
}

bool
ncm_action_can_run(enum NcmActionType type, void *user) {
    NcmActionDef *action;

    action = ncm_action_get(type);
    if ((action == 0) || (action->can_run == 0)) {
        return false;
    }
    return action->can_run(user);
}

bool
ncm_action_run(enum NcmActionType type, void *user) {
    NcmActionDef *action;

    action = ncm_action_get(type);
    if ((action == 0) || (action->run == 0)) {
        return false;
    }
    return action->run(user);
}
