#include "app_binding_migration.h"

bool
app_binding_migration_action_is_c_safe(enum NcmActionType type) {
    switch (type) {
    case NCM_ACTION_DUMMY:
    case NCM_ACTION_QUIT:
    case NCM_ACTION_VOLUME_UP:
    case NCM_ACTION_VOLUME_DOWN:
    case NCM_ACTION_REPLAY_SONG:
    case NCM_ACTION_TOGGLE_REPEAT:
    case NCM_ACTION_TOGGLE_RANDOM:
    case NCM_ACTION_TOGGLE_SINGLE:
    case NCM_ACTION_TOGGLE_CONSUME:
    case NCM_ACTION_TOGGLE_CROSSFADE:
    case NCM_ACTION_UPDATE_DATABASE:
    case NCM_ACTION_NEXT:
    case NCM_ACTION_PREVIOUS:
    case NCM_ACTION_PAUSE:
    case NCM_ACTION_PLAY:
    case NCM_ACTION_STOP:
    case NCM_ACTION_SEEK_FORWARD:
    case NCM_ACTION_SEEK_BACKWARD:
    case NCM_ACTION_EXECUTE_COMMAND:
    case NCM_ACTION_RUN_ACTION:
    case NCM_ACTION_ADD_SELECTED_ITEMS:
    case NCM_ACTION_MOVE_SORT_ORDER_UP:
    case NCM_ACTION_MOVE_SORT_ORDER_DOWN:
    case NCM_ACTION_SORT_PLAYLIST:
    case NCM_ACTION_APPLY_FILTER:
    case NCM_ACTION_FIND_ITEM_FORWARD:
    case NCM_ACTION_FIND_ITEM_BACKWARD:
    case NCM_ACTION_JUMP_TO_MEDIA_LIBRARY:
    case NCM_ACTION_TOGGLE_MEDIA_LIBRARY_SORT_MODE:
    case NCM_ACTION_SHOW_MEDIA_LIBRARY:
    case NCM_ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE:
    case NCM_ACTION_SHOW_SEARCH_ENGINE:
    case NCM_ACTION_RESET_SEARCH_ENGINE:
        return true;
    default:
        return false;
    }
}

bool
app_binding_migration_screen_is_c_only(enum ScreenType type) {
    return type == NCM_SCREEN_TYPE_PLAYLIST
        || type == NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER
        || type == NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG
        || type == NCM_SCREEN_TYPE_SEARCH_ENGINE
#if defined(HAVE_TAGLIB_H)
        || type == NCM_SCREEN_TYPE_TINY_TAG_EDITOR
#endif
        || type == NCM_SCREEN_TYPE_VISUALIZER;
}

bool
app_binding_migration_action_is_c_safe_for_screen(
    enum NcmActionType type, enum ScreenType screen_type) {
#if defined(HAVE_TAGLIB_H)
    if (screen_type == NCM_SCREEN_TYPE_TINY_TAG_EDITOR) {
        switch (type) {
        case NCM_ACTION_JUMP_TO_MEDIA_LIBRARY:
        case NCM_ACTION_SHOW_MEDIA_LIBRARY:
        case NCM_ACTION_SHOW_SEARCH_ENGINE:
            return false;
        default:
            break;
        }
    }
#endif
    if (app_binding_migration_action_is_c_safe(type)) {
        return true;
    }
    if (!app_binding_migration_screen_is_c_only(screen_type)) {
        return false;
    }

    switch (screen_type) {
    case NCM_SCREEN_TYPE_PLAYLIST:
        switch (type) {
        case NCM_ACTION_UPDATE_ENVIRONMENT:
        case NCM_ACTION_MOUSE_EVENT:
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
        case NCM_ACTION_PREVIOUS_COLUMN:
        case NCM_ACTION_NEXT_COLUMN:
        case NCM_ACTION_MASTER_SCREEN:
        case NCM_ACTION_SLAVE_SCREEN:
        case NCM_ACTION_ADD_ITEM_TO_PLAYLIST:
        case NCM_ACTION_PLAY_ITEM:
        case NCM_ACTION_DELETE_PLAYLIST_ITEMS:
        case NCM_ACTION_SAVE_PLAYLIST:
        case NCM_ACTION_MOVE_SELECTED_ITEMS_UP:
        case NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN:
        case NCM_ACTION_MOVE_SELECTED_ITEMS_TO:
        case NCM_ACTION_ADD:
        case NCM_ACTION_TOGGLE_DISPLAY_MODE:
        case NCM_ACTION_TOGGLE_SEPARATORS_BETWEEN_ALBUMS:
        case NCM_ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE:
        case NCM_ACTION_TOGGLE_LYRICS_FETCHER:
        case NCM_ACTION_TOGGLE_FETCHING_LYRICS_IN_BACKGROUND:
        case NCM_ACTION_TOGGLE_PLAYING_SONG_CENTERING:
        case NCM_ACTION_JUMP_TO_PLAYING_SONG:
        case NCM_ACTION_SHUFFLE:
        case NCM_ACTION_SET_CROSSFADE:
        case NCM_ACTION_SET_VOLUME:
        case NCM_ACTION_EDIT_SONG:
        case NCM_ACTION_JUMP_TO_BROWSER:
        case NCM_ACTION_JUMP_TO_PLAYLIST_EDITOR:
        case NCM_ACTION_TOGGLE_SCREEN_LOCK:
        case NCM_ACTION_JUMP_TO_TAG_EDITOR:
        case NCM_ACTION_JUMP_TO_POSITION_IN_SONG:
        case NCM_ACTION_SELECT_ITEM:
        case NCM_ACTION_SELECT_RANGE:
        case NCM_ACTION_REVERSE_SELECTION:
        case NCM_ACTION_REMOVE_SELECTION:
        case NCM_ACTION_SELECT_ALBUM:
        case NCM_ACTION_SELECT_FOUND_ITEMS:
        case NCM_ACTION_CROP_MAIN_PLAYLIST:
        case NCM_ACTION_CLEAR_MAIN_PLAYLIST:
        case NCM_ACTION_REVERSE_PLAYLIST:
        case NCM_ACTION_NEXT_FOUND_ITEM:
        case NCM_ACTION_PREVIOUS_FOUND_ITEM:
        case NCM_ACTION_TOGGLE_FIND_MODE:
        case NCM_ACTION_TOGGLE_REPLAY_GAIN_MODE:
        case NCM_ACTION_TOGGLE_ADD_MODE:
        case NCM_ACTION_TOGGLE_MOUSE:
        case NCM_ACTION_TOGGLE_BITRATE_VISIBILITY:
        case NCM_ACTION_ADD_RANDOM_ITEMS:
        case NCM_ACTION_FETCH_LYRICS_IN_BACKGROUND:
        case NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY:
        case NCM_ACTION_SHOW_SONG_INFO:
        case NCM_ACTION_SHOW_ARTIST_INFO:
        case NCM_ACTION_SHOW_LYRICS:
        case NCM_ACTION_NEXT_SCREEN:
        case NCM_ACTION_PREVIOUS_SCREEN:
        case NCM_ACTION_SHOW_HELP:
        case NCM_ACTION_SHOW_PLAYLIST:
        case NCM_ACTION_SHOW_BROWSER:
        case NCM_ACTION_SHOW_PLAYLIST_EDITOR:
        case NCM_ACTION_SHOW_TAG_EDITOR:
        case NCM_ACTION_SHOW_OUTPUTS:
        case NCM_ACTION_SHOW_VISUALIZER:
        case NCM_ACTION_SHOW_SERVER_INFO:
            return true;
        default:
            return false;
        }
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
        switch (type) {
        case NCM_ACTION_MOUSE_EVENT:
        case NCM_ACTION_SCROLL_UP:
        case NCM_ACTION_SCROLL_DOWN:
        case NCM_ACTION_PAGE_UP:
        case NCM_ACTION_PAGE_DOWN:
        case NCM_ACTION_MOVE_HOME:
        case NCM_ACTION_MOVE_END:
        case NCM_ACTION_ADD:
        case NCM_ACTION_NEXT_FOUND_ITEM:
        case NCM_ACTION_PREVIOUS_FOUND_ITEM:
        case NCM_ACTION_TOGGLE_FIND_MODE:
        case NCM_ACTION_NEXT_SCREEN:
        case NCM_ACTION_PREVIOUS_SCREEN:
            return true;
        default:
            return false;
        }
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
        switch (type) {
        case NCM_ACTION_SCROLL_UP:
        case NCM_ACTION_SCROLL_DOWN:
        case NCM_ACTION_PAGE_UP:
        case NCM_ACTION_PAGE_DOWN:
        case NCM_ACTION_MOVE_HOME:
        case NCM_ACTION_MOVE_END:
            return true;
        default:
            return false;
        }
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
        switch (type) {
        case NCM_ACTION_MOUSE_EVENT:
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
        case NCM_ACTION_MASTER_SCREEN:
        case NCM_ACTION_SLAVE_SCREEN:
        case NCM_ACTION_ADD_ITEM_TO_PLAYLIST:
        case NCM_ACTION_PLAY_ITEM:
        case NCM_ACTION_TOGGLE_DISPLAY_MODE:
        case NCM_ACTION_START_SEARCHING:
        case NCM_ACTION_JUMP_TO_BROWSER:
        case NCM_ACTION_JUMP_TO_PLAYLIST_EDITOR:
        case NCM_ACTION_JUMP_TO_TAG_EDITOR:
        case NCM_ACTION_TOGGLE_SCREEN_LOCK:
        case NCM_ACTION_SELECT_ITEM:
        case NCM_ACTION_SELECT_RANGE:
        case NCM_ACTION_REVERSE_SELECTION:
        case NCM_ACTION_REMOVE_SELECTION:
        case NCM_ACTION_NEXT_FOUND_ITEM:
        case NCM_ACTION_PREVIOUS_FOUND_ITEM:
        case NCM_ACTION_TOGGLE_FIND_MODE:
        case NCM_ACTION_NEXT_SCREEN:
        case NCM_ACTION_PREVIOUS_SCREEN:
        case NCM_ACTION_SHOW_HELP:
        case NCM_ACTION_SHOW_PLAYLIST:
        case NCM_ACTION_SHOW_BROWSER:
        case NCM_ACTION_SHOW_PLAYLIST_EDITOR:
        case NCM_ACTION_SHOW_TAG_EDITOR:
        case NCM_ACTION_SHOW_OUTPUTS:
        case NCM_ACTION_SHOW_VISUALIZER:
        case NCM_ACTION_SHOW_SERVER_INFO:
        case NCM_ACTION_SHOW_SONG_INFO:
        case NCM_ACTION_SHOW_ARTIST_INFO:
        case NCM_ACTION_SHOW_LYRICS:
            return true;
        default:
            return false;
        }
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
        switch (type) {
        case NCM_ACTION_MOUSE_EVENT:
        case NCM_ACTION_SCROLL_UP:
        case NCM_ACTION_SCROLL_DOWN:
        case NCM_ACTION_PAGE_UP:
        case NCM_ACTION_PAGE_DOWN:
        case NCM_ACTION_MOVE_HOME:
        case NCM_ACTION_MOVE_END:
        case NCM_ACTION_SAVE_TAG_CHANGES:
        case NCM_ACTION_NEXT_SCREEN:
        case NCM_ACTION_PREVIOUS_SCREEN:
            return true;
        default:
            return false;
        }
#endif
    case NCM_SCREEN_TYPE_VISUALIZER:
        switch (type) {
        case NCM_ACTION_SHOW_VISUALIZER:
        case NCM_ACTION_TOGGLE_VISUALIZATION_TYPE:
        case NCM_ACTION_MOUSE_EVENT:
        case NCM_ACTION_MASTER_SCREEN:
        case NCM_ACTION_SLAVE_SCREEN:
        case NCM_ACTION_TOGGLE_SCREEN_LOCK:
        case NCM_ACTION_NEXT_SCREEN:
        case NCM_ACTION_PREVIOUS_SCREEN:
        case NCM_ACTION_SHOW_HELP:
        case NCM_ACTION_SHOW_PLAYLIST:
        case NCM_ACTION_SHOW_BROWSER:
        case NCM_ACTION_SHOW_SEARCH_ENGINE:
        case NCM_ACTION_SHOW_MEDIA_LIBRARY:
        case NCM_ACTION_SHOW_PLAYLIST_EDITOR:
        case NCM_ACTION_SHOW_TAG_EDITOR:
        case NCM_ACTION_SHOW_OUTPUTS:
        case NCM_ACTION_SHOW_SERVER_INFO:
        case NCM_ACTION_SHOW_SONG_INFO:
        case NCM_ACTION_SHOW_ARTIST_INFO:
        case NCM_ACTION_SHOW_LYRICS:
            return true;
        default:
            return false;
        }
    default:
        return false;
    }
}

bool
app_binding_migration_binding_is_plain_action_sequence(
    NcmBinding *binding) {
    if (binding == NULL) {
        return false;
    }
    if (binding->actions_len <= 0) {
        return false;
    }

    for (int32 i = 0; i < binding->actions_len; i += 1) {
        if (binding->actions[i].kind != NCM_BINDING_ACTION_NORMAL) {
            return false;
        }
    }

    return true;
}

static bool
app_binding_migration_action_forces_legacy_binding(
    enum NcmActionType type) {
    switch (type) {
    case NCM_ACTION_UPDATE_ENVIRONMENT:
    case NCM_ACTION_MOUSE_EVENT:
    case NCM_ACTION_SCROLL_UP:
    case NCM_ACTION_SCROLL_DOWN:
    case NCM_ACTION_PAGE_UP:
    case NCM_ACTION_PAGE_DOWN:
    case NCM_ACTION_MOVE_HOME:
    case NCM_ACTION_MOVE_END:
    case NCM_ACTION_SCROLL_UP_ARTIST:
    case NCM_ACTION_SCROLL_UP_ALBUM:
    case NCM_ACTION_SCROLL_DOWN_ARTIST:
    case NCM_ACTION_SCROLL_DOWN_ALBUM:
    case NCM_ACTION_TOGGLE_INTERFACE:
    case NCM_ACTION_JUMP_TO_PARENT_DIRECTORY:
    case NCM_ACTION_PREVIOUS_COLUMN:
    case NCM_ACTION_NEXT_COLUMN:
    case NCM_ACTION_MASTER_SCREEN:
    case NCM_ACTION_SLAVE_SCREEN:
    case NCM_ACTION_ADD_ITEM_TO_PLAYLIST:
    case NCM_ACTION_PLAY_ITEM:
    case NCM_ACTION_DELETE_PLAYLIST_ITEMS:
    case NCM_ACTION_DELETE_STORED_PLAYLIST:
    case NCM_ACTION_DELETE_BROWSER_ITEMS:
    case NCM_ACTION_MOVE_SELECTED_ITEMS_UP:
    case NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN:
    case NCM_ACTION_MOVE_SELECTED_ITEMS_TO:
    case NCM_ACTION_ADD:
    case NCM_ACTION_LOAD:
    case NCM_ACTION_TOGGLE_DISPLAY_MODE:
    case NCM_ACTION_TOGGLE_SEPARATORS_BETWEEN_ALBUMS:
    case NCM_ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE:
    case NCM_ACTION_TOGGLE_LYRICS_FETCHER:
    case NCM_ACTION_TOGGLE_FETCHING_LYRICS_IN_BACKGROUND:
    case NCM_ACTION_TOGGLE_PLAYING_SONG_CENTERING:
    case NCM_ACTION_JUMP_TO_PLAYING_SONG:
    case NCM_ACTION_SHUFFLE:
    case NCM_ACTION_START_SEARCHING:
    case NCM_ACTION_SAVE_TAG_CHANGES:
    case NCM_ACTION_ENTER_DIRECTORY:
    case NCM_ACTION_EDIT_SONG:
    case NCM_ACTION_EDIT_LIBRARY_TAG:
    case NCM_ACTION_EDIT_LIBRARY_ALBUM:
    case NCM_ACTION_EDIT_DIRECTORY_NAME:
    case NCM_ACTION_EDIT_PLAYLIST_NAME:
    case NCM_ACTION_EDIT_LYRICS:
    case NCM_ACTION_JUMP_TO_BROWSER:
    case NCM_ACTION_JUMP_TO_PLAYLIST_EDITOR:
    case NCM_ACTION_TOGGLE_SCREEN_LOCK:
    case NCM_ACTION_JUMP_TO_TAG_EDITOR:
    case NCM_ACTION_SELECT_ITEM:
    case NCM_ACTION_SELECT_RANGE:
    case NCM_ACTION_REVERSE_SELECTION:
    case NCM_ACTION_REMOVE_SELECTION:
    case NCM_ACTION_SELECT_ALBUM:
    case NCM_ACTION_SELECT_FOUND_ITEMS:
    case NCM_ACTION_CROP_PLAYLIST:
    case NCM_ACTION_CLEAR_PLAYLIST:
    case NCM_ACTION_REVERSE_PLAYLIST:
    case NCM_ACTION_FIND:
    case NCM_ACTION_NEXT_FOUND_ITEM:
    case NCM_ACTION_PREVIOUS_FOUND_ITEM:
    case NCM_ACTION_TOGGLE_FIND_MODE:
    case NCM_ACTION_TOGGLE_BROWSER_SORT_MODE:
    case NCM_ACTION_TOGGLE_LIBRARY_TAG_TYPE:
    case NCM_ACTION_FETCH_LYRICS_IN_BACKGROUND:
    case NCM_ACTION_REFETCH_LYRICS:
    case NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY:
    case NCM_ACTION_TOGGLE_OUTPUT:
    case NCM_ACTION_TOGGLE_VISUALIZATION_TYPE:
    case NCM_ACTION_SHOW_SONG_INFO:
    case NCM_ACTION_SHOW_ARTIST_INFO:
    case NCM_ACTION_SHOW_LYRICS:
    case NCM_ACTION_NEXT_SCREEN:
    case NCM_ACTION_PREVIOUS_SCREEN:
    case NCM_ACTION_SHOW_HELP:
    case NCM_ACTION_SHOW_PLAYLIST:
    case NCM_ACTION_SHOW_BROWSER:
    case NCM_ACTION_CHANGE_BROWSE_MODE:
    case NCM_ACTION_SHOW_PLAYLIST_EDITOR:
    case NCM_ACTION_SHOW_TAG_EDITOR:
    case NCM_ACTION_SHOW_OUTPUTS:
    case NCM_ACTION_SHOW_VISUALIZER:
    case NCM_ACTION_SHOW_SERVER_INFO:
        return true;
    default:
        return false;
    }
}

static bool
app_binding_migration_action_can_run_hybrid_legacy(
    enum NcmActionType type) {
    if (app_binding_migration_action_is_c_safe(type)) {
        return true;
    }
    return !app_binding_migration_action_forces_legacy_binding(type);
}

bool
app_binding_migration_binding_is_c_safe(NcmBinding *binding) {
    if (binding == NULL) {
        return false;
    }
    if (binding->actions_len <= 0) {
        return false;
    }

    for (int32 i = 0; i < binding->actions_len; i += 1) {
        NcmBindingAction *action;

        action = binding->actions + i;
        switch (action->kind) {
        case NCM_BINDING_ACTION_NORMAL:
        case NCM_BINDING_ACTION_REQUIRE_RUNNABLE:
            if (!app_binding_migration_action_is_c_safe(action->type)) {
                return false;
            }
            break;
        case NCM_BINDING_ACTION_PUSH_CHARACTERS:
        case NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND:
        case NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND:
            break;
        case NCM_BINDING_ACTION_REQUIRE_SCREEN:
            return false;
        }
    }

    return true;
}

bool
app_binding_migration_binding_is_c_safe_for_screen(
    NcmBinding *binding, enum ScreenType screen_type) {
    if (!app_binding_migration_screen_is_c_only(screen_type)) {
        return app_binding_migration_binding_is_c_safe(binding);
    }
    if (binding == NULL || binding->actions_len <= 0) {
        return false;
    }

    for (int32 i = 0; i < binding->actions_len; i += 1) {
        NcmBindingAction *action;

        action = binding->actions + i;
        switch (action->kind) {
        case NCM_BINDING_ACTION_NORMAL:
        case NCM_BINDING_ACTION_REQUIRE_RUNNABLE:
            if (!app_binding_migration_action_is_c_safe_for_screen(
                    action->type, screen_type)) {
                return false;
            }
            break;
        case NCM_BINDING_ACTION_PUSH_CHARACTERS:
        case NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND:
        case NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND:
            break;
        case NCM_BINDING_ACTION_REQUIRE_SCREEN:
            return false;
        }
    }

    return true;
}

bool
app_binding_migration_binding_is_hybrid_safe(NcmBinding *binding) {
    if (binding == NULL) {
        return false;
    }
    if (binding->actions_len <= 0) {
        return false;
    }

    for (int32 i = 0; i < binding->actions_len; i += 1) {
        NcmBindingAction *action;

        action = binding->actions + i;
        switch (action->kind) {
        case NCM_BINDING_ACTION_NORMAL:
            if (!app_binding_migration_action_can_run_hybrid_legacy(
                    action->type)) {
                return false;
            }
            break;
        case NCM_BINDING_ACTION_REQUIRE_RUNNABLE:
            if (!app_binding_migration_action_is_c_safe(action->type)) {
                return false;
            }
            break;
        case NCM_BINDING_ACTION_PUSH_CHARACTERS:
        case NCM_BINDING_ACTION_RUN_EXTERNAL_COMMAND:
        case NCM_BINDING_ACTION_RUN_EXTERNAL_CONSOLE_COMMAND:
            break;
        case NCM_BINDING_ACTION_REQUIRE_SCREEN:
            return false;
        }
    }

    return true;
}
