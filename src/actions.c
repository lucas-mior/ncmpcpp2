#if !defined(ACTIONS_C)
#define ACTIONS_C

#include "cbase.h"

#include "actions.h"
#include "app_controller.h"
#include "app_legacy_bridge.h"
#include "bindings.h"
#include "c/ncm_c.h"
#include "curses/nc_curses.h"
#include "global.h"
#include "helpers.h"
#include "screens/nc_screens.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "title.h"
#include "ui_state.h"

int32
ncm_action_type_parse(char *name, int32 name_len, enum ActionType *type) {
    enum ActionType action_type;

    if ((name == NULL) || (name_len < 0) || (type == NULL)) {
        return -EINVAL;
    }

    action_type = ACTION_parse(name, name_len);
    if (action_type == ACTION_COUNT) {
        return -NCM_ERROR_PARSE;
    }

    *type = action_type;
    return 0;
}

bool
ncm_action_can_run(enum ActionType type, void *user) {
    return ncm_action_runtime_can_run(user, type);
}

static ActionRuntime action_global_runtime;
static bool action_global_runtime_initialized;

typedef struct ActionRuntimeCommandPrompt {
    StrBuilder previous;
} ActionRuntimeCommandPrompt;

typedef SearchPromptState ActionRuntimeSearchPrompt;

static ActionRuntime *
action_runtime_or_global(ActionRuntime *runtime) {
    if (runtime) {
        return runtime;
    }
    return ncm_action_runtime_global();
}

static int32
action_runtime_call_hook(ActionRuntimeHook hook, enum ActionType type,
                         void *user) {
    if (hook == NULL) {
        return ACTION_RUNTIME_DEFER;
    }
    return hook(type, user);
}


static bool
action_runtime_hook_allowed(int32 result, bool *handled) {
    *handled = result != ACTION_RUNTIME_DEFER;
    return result == ACTION_RUNTIME_ALLOW;
}

static bool
action_runtime_hook_denied(int32 result, bool *handled) {
    *handled = result != ACTION_RUNTIME_DEFER;
    return result == ACTION_RUNTIME_DENY;
}

static bool
action_runtime_current_screen_is(enum ScreenType type) {
    NcScreen *screen;
    enum NcScreenType nc_type;

    if ((screen = app_controller_current_screen()) == NULL) {
        return false;
    }

    nc_type = screen_type_to_nc_type(type);
    return nc_screen_type(screen) == nc_type;
}

static NcScreen *
current_screen(void) {
    return app_controller_current_screen();
}

static void
current_screen_finish_immediate_change(void) {
    nc_screen_finish_action_change(current_screen());
    return;
}

static bool
current_screen_can_filter(void) {
    return nc_screen_can_filter(current_screen());
}

static StringView
current_screen_current_filter(void) {
    return nc_screen_current_filter(current_screen());
}

static int32
current_screen_apply_filter(char *pattern, int32 pattern_len,
                            NcmError *ncm_error) {
    NcScreen *screen = current_screen();
    int32 status;

    status = nc_screen_apply_filter(screen, pattern, pattern_len,
                                    Config.regular_expressions, ncm_error);
    if (status < 0) {
        if (!ncm_error_is_set(ncm_error)) {
            return ncm_error_set_status(ncm_error, status,
                                        STRLIT("screen cannot filter"));
        }
        return status;
    }

    current_screen_finish_immediate_change();
    return ncm_error_ok(ncm_error);
}

static bool
current_screen_can_search(void) {
    return nc_screen_can_search(current_screen());
}

static bool
current_screen_can_find(void) {
    return nc_screen_can_find(current_screen());
}

static StringView
current_screen_current_search_constraint(void) {
    return nc_screen_current_search_constraint(current_screen());
}

static int32
current_screen_search(enum SearchDirection direction, char *pattern,
                      int32 pattern_len, bool wrap, bool skip_current,
                      NcmError *ncm_error) {
    NcScreen *screen = current_screen();
    int32 status;

    if ((pattern == NULL) || (pattern_len <= 0)) {
        if (nc_screen_can_search(screen)) {
            nc_screen_clear_search_constraint(screen);
            current_screen_finish_immediate_change();
        }
        return ncm_error_ok(ncm_error);
    }

    status = nc_screen_search(screen, direction, pattern, pattern_len,
                              Config.regular_expressions, wrap,
                              skip_current, ncm_error);
    if (status < 0) {
        if (!ncm_error_is_set(ncm_error)) {
            return ncm_error_set_status(ncm_error, status,
                                        STRLIT("screen cannot search"));
        }
        return status;
    }

    current_screen_finish_immediate_change();
    return ncm_error_ok(ncm_error);
}

static void
current_screen_clear_search_constraint(void) {
    nc_screen_clear_search_constraint(current_screen());
    return;
}

static int32
action_runtime_switch_to_screen(enum ScreenType type) {
    int32 status;

    if ((type != SCREEN_TYPE_SELECTED_ITEMS_ADDER)
        && action_runtime_current_screen_is(SCREEN_TYPE_SELECTED_ITEMS_ADDER)
        && ((status = selected_items_adder_screen_return_to_previous(
                 app_screen_selected_items_adder())) < 0)) {
        return status;
    }

    return app_screens_switch_or_open_type(type);
}

int32
ncm_action_show_visualizer(void) {
    return app_screens_switch_or_open_type(SCREEN_TYPE_VISUALIZER);
}

int32
ncm_action_toggle_visualization_type(void) {
#if defined(ENABLE_VISUALIZER)
    if (!app_screen_visualizer_is_current()) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    visualizer_screen_toggle_type(app_screen_visualizer());
    return 0;
#else
    return -NCM_ERROR_UNAVAILABLE;
#endif
}

static int32
action_runtime_switch_to_next_screen(bool reverse) {
    ScreenTypeArray *sequence = &Config.screen_switcher_mode;
    NcScreen *current;
    bool selected_items_adder = action_runtime_current_screen_is(
        SCREEN_TYPE_SELECTED_ITEMS_ADDER);
    int32 current_index;
    int32 next_index;

    if (selected_items_adder && Config.screen_switcher_previous) {
        return selected_items_adder_screen_return_to_previous(
            app_screen_selected_items_adder());
    }
    if (Config.screen_switcher_previous) {
        if ((current = app_controller_previous_screen()) == NULL) {
            return -NCM_ERROR_UNAVAILABLE;
        }
        return nc_screen_switcher_switch_to(current,
                                            current->has_to_be_resized);
    }

    if (sequence->len <= 0) {
        return -NCM_ERROR_UNAVAILABLE;
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

static void
action_runtime_mpd_error(NcmError *ncm_error) {
    if (ncm_error_is_set(ncm_error)) {
        ncm_statusbar_print(Config.message_delay_time,
                            ncm_error->message, ncm_error->message_len);
    }
    return;
}

static int32
action_runtime_mpd_error_status(NcmError *ncm_error) {
    (void)action_runtime_mpd_error(ncm_error);
    if (ncm_error_is_set(ncm_error)) {
        return ncm_error_status(ncm_error);
    }

    return -NCM_ERROR_UNAVAILABLE;
}

static int32
action_runtime_playlist_find_song(NcmSong *song, NcmSong **match) {
    PlaylistScreen *screen = app_screen_playlist();
    NcSongMenu *song_menu;
    NcMenu *menu;
    int32 count;

    ASSERT(match != NULL);

    *match = NULL;
    if ((song_menu = playlist_screen_song_menu(screen)) == NULL) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    menu = nc_song_menu_base(song_menu);
    count = nc_menu_all_item_count(menu);
    for (int32 i = 0; i < count; i += 1) {
        NcmSong *item;

        if ((item = nc_song_menu_item_at(song_menu,
                                         NC_MENU_ITEMS_ALL, i)) == NULL) {
            continue;
        }
        if (!ncm_song_is_equal(item, song)) {
            continue;
        }
        *match = item;
        return 1;
    }

    return 0;
}

static int32
action_runtime_playlist_remove_song(NcmSong *song, NcmError *ncm_error) {
    PlaylistScreen *screen = app_screen_playlist();
    NcSongMenu *song_menu;
    NcMenu *menu;
    int32 position;
    int32 count;
    bool ok;

    if ((song_menu = playlist_screen_song_menu(screen)) == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing playlist screen"));
        return -NCM_ERROR_UNAVAILABLE;
    }
    menu = nc_song_menu_base(song_menu);
    count = nc_menu_all_item_count(menu);

    ok = ncm_mpd_client_start_command_list(&global_mpd, ncm_error) == 0;
    for (int32 i = count; ok && (i > 0); i -= 1) {
        NcmSong *item;

        if (((item = nc_song_menu_item_at(song_menu, NC_MENU_ITEMS_ALL,
                                          i - 1)) == NULL)
            || !ncm_song_is_equal(item, song)) {
            continue;
        }

        position = ncm_song_position(item);
        if (position < 0) {
            continue;
        }
        ok = ncm_mpd_client_delete(&global_mpd, position, ncm_error) == 0;
    }
    if (ok) {
        ok = ncm_mpd_client_commit_command_list(&global_mpd, ncm_error) == 0;
    }
    if (!ok && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }
    if (!ok) {
        return action_runtime_mpd_error_status(ncm_error);
    }
    return 0;
}

static int32
action_runtime_mpd_simple(int32 (*func)(MpdClient *client,
                                        NcmError *ncm_error)) {
    NcmError ncm_error;

    ncm_error_clear(&ncm_error);
    if (func(&global_mpd, &ncm_error) < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return 0;
}

int32
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
        return -EINVAL;
    }
    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    ncm_error_clear(&ncm_error);
    match = NULL;
    if ((space_add_mode == NCM_SPACE_ADD_MODE_ADD_REMOVE)
        && (action_runtime_playlist_find_song(song, &match) > 0)) {
        if (play) {
            ok = ncm_mpd_client_play_id(&global_mpd,
                                        ncm_song_id(match), &ncm_error) == 0;
        } else {
            ok = action_runtime_playlist_remove_song(song, &ncm_error) == 0;
        }
        if (!ok) {
            return action_runtime_mpd_error_status(&ncm_error);
        }
        (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
        return 0;
    }

    id = -1;
    if (ncm_mpd_client_add_song_value(&global_mpd, song, position, &id,
                                       &ncm_error) < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }

    formatted = ncm_format_render_string(&Config.song_status_format, song);
    SB_APPEND(&message, "Added to playlist: ");
    SB_APPEND(&message, formatted.data, formatted.len);
    ncm_statusbar_print(Config.message_delay_time, message.data, message.len);
    sb_free(&message);
    sb_free(&formatted);

    if (play && (id >= 0)) {
        if (ncm_mpd_client_play_id(&global_mpd, id, &ncm_error) < 0) {
            return action_runtime_mpd_error_status(&ncm_error);
        }
    }

    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return 0;
}

int32
ncm_action_add_song_to_playlist(NcmSong *song, bool play, int32 position) {
    return ncm_action_add_song_to_playlist_with_mode(song, play, position,
                                                     Config.space_add_mode);
}

static int32
action_runtime_mpd_toggle(int32 (*func)(MpdClient *client, bool mode,
                                        NcmError *ncm_error), bool current) {
    NcmError ncm_error;

    ncm_error_clear(&ncm_error);
    if (func(&global_mpd, !current, &ncm_error) < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return 0;
}

static int32
action_runtime_volume(int32 change) {
    NcmError ncm_error;

    ncm_error_clear(&ncm_error);
    if (ncm_mpd_client_change_volume(&global_mpd, change, &ncm_error) < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return 0;
}

static int32
action_runtime_update_database(void) {
    StringView view;
    NcmError ncm_error;
    char *path = "/";

    if (action_runtime_current_screen_is(SCREEN_TYPE_BROWSER)) {
        view = browser_screen_current_directory(app_screen_browser());
        if (view.data) {
            path = view.data;
        } else {
            path = "";
        }
    }

#if defined(HAVE_TAGLIB_H)
    if (action_runtime_current_screen_is(SCREEN_TYPE_TAG_EDIT)) {
        if (tag_edit_screen_current_dir(app_screen_tag_edit(), &view) == 0) {
            path = view.data;
        }
    }
#endif

    ncm_error_clear(&ncm_error);
    if (ncm_mpd_client_update_directory(&global_mpd, path, NULL,
                                        &ncm_error) < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    return 0;
}

static int32
action_runtime_replay_song(void) {
    NcmError ncm_error;
    int32 position = ncm_status_state_current_song_position();

    if (position < 0) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    ncm_error_clear(&ncm_error);
    if (ncm_mpd_client_play_pos(&global_mpd, position, &ncm_error) < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    return 0;
}

static int32
action_runtime_toggle_crossfade(void) {
    NcmError ncm_error;
    int32 seconds = Config.mpd_crossfade_time;

    ncm_error_clear(&ncm_error);
    if (ncm_status_state_crossfade_is_enabled()) {
        seconds = 0;
    }
    if (ncm_mpd_client_set_crossfade(&global_mpd, seconds, &ncm_error) < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return 0;
}

static enum NcMenuItemSource
action_runtime_menu_item_source(NcMenu *menu) {
    if (nc_menu_is_filtered(menu)) {
        return NC_MENU_ITEMS_FILTERED;
    }
    return NC_MENU_ITEMS_ALL;
}

static NcMenu *
action_runtime_current_menu(void) {
    return nc_screen_current_menu(app_controller_current_screen());
}

static bool
action_runtime_menu_has_items(void) {
    NcMenu *menu;

    if ((menu = action_runtime_current_menu()) == NULL) {
        return false;
    }
    return nc_menu_item_count(menu) > 0;
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

static void
action_runtime_print_message(char *prefix, int32 prefix_len,
                             char *text, int32 text_len,
                             char *suffix, int32 suffix_len) {
    StrBuilder message = {0};

    SB_APPEND(&message, prefix, prefix_len);
    if ((text != NULL) && (text_len > 0)) {
        SB_APPEND(&message, text, text_len);
    }
    SB_APPEND(&message, suffix, suffix_len);
    ncm_statusbar_print(Config.message_delay_time, message.data, message.len);
    sb_free(&message);
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

    if ((previous->len == text_len) && ((text_len == 0)
            || (memcmp64(previous->data, text, text_len) == 0))) {
        return false;
    }

    if (sb_set(previous, text, text_len) < 0) {
        return false;
    }

    command = ncm_bindings_config_find_command(&Bindings, text, text_len);
    if (command && command->immediate) {
        return true;
    }
    return false;
}

static bool
action_runtime_command_prompt_should_continue(char *text, void *user) {
    ActionRuntimeCommandPrompt *state = user;
    int32 text_len = optional_strlen32(text);

    if (!ncm_statusbar_prompt_should_continue(text, text_len)) {
        return false;
    }
    if (ncm_action_immediate_command_prompt_should_stop(&state->previous, text,
                                                        text_len)) {
        return false;
    }
    return true;
}

static bool
action_runtime_filter_prompt_should_continue(char *text, void *user) {
    NcmError ncm_error;
    int32 text_len = optional_strlen32(text);

    (void)user;
    if (!ncm_statusbar_prompt_should_continue(text, text_len)) {
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

static int32
action_runtime_search_from_prompt_start(ActionRuntimeSearchPrompt *state,
                                        char *text, int32 text_len, bool *found,
                                        NcmError *ncm_error) {
    NcMenu *menu = action_runtime_current_menu();
    int32 old_beginning = 0;
    int32 old_highlight = 0;
    int32 count;
    int32 status;
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

    status = current_screen_search(
        state->direction, text, text_len,
        Config.default_find_mode == NCM_DEFAULT_FIND_MODE_WRAPPED, false,
        ncm_error);
    if (status < 0) {
        *found = false;
        return status;
    }

    *found = status > 0;
    if (restore && !*found && !ncm_error_is_set(ncm_error)) {
        NcScreen *screen;

        menu->beginning = old_beginning;
        menu->highlight = old_highlight;
        if ((screen = app_controller_current_screen())) {
            nc_screen_refresh_window(screen);
        }
    }
    return 0;
}

static int32
action_runtime_search_prompt_apply(ActionRuntimeSearchPrompt *state, char *text,
                                   int32 text_len, bool *found,
                                   NcmError *ncm_error) {
    bool last_found;
    int32 finish_status;
    int32 status;

    if (text == NULL) {
        text = "";
        text_len = 0;
    }
    if (ncm_search_prompt_state_has_cached_result(state, text, text_len,
                                                   &last_found)) {
        if (found) {
            *found = last_found;
        }
        return 0;
    }

    last_found = false;
    ncm_error_clear(ncm_error);
    status = action_runtime_search_from_prompt_start(state, text, text_len,
                                                     &last_found, ncm_error);
    finish_status = ncm_search_prompt_state_finish_result(state, text, text_len,
                                                          status == 0,
                                                          last_found);
    if (finish_status < 0) {
        return finish_status;
    }
    if (found) {
        *found = last_found;
    }
    return status;
}

static bool
action_runtime_search_prompt_should_continue(char *text, void *user) {
    ActionRuntimeSearchPrompt *state = user;
    NcmError ncm_error;
    int32 text_len = optional_strlen32(text);

    if (!ncm_statusbar_prompt_should_continue(text, text_len)) {
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
                             bool remember,
                             NcPromptShouldContinueFunc *should_continue,
                             void *should_continue_user, StrBuilder *result) {
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
        prompt.should_continue = should_continue;
        prompt.should_continue_user_data = should_continue_user;
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
    int32 status = 0;

    ncm_statusbar_scoped_lock_init(&scoped_lock);
    if ((window = ncm_statusbar_put())) {
        nc_window_print_data(window, message, message_len);
        nc_window_print_data(window, STRLIT(" [y/n] "));
        status = ncm_statusbar_prompt_return_one_of(window, values,
                                                    LENGTH(values), &answer);
    }
    ncm_statusbar_scoped_lock_destroy(&scoped_lock);

    if ((status <= 0) || (answer == 'n')) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Action cancelled"));
        return false;
    }
    return true;
}

static int32
action_runtime_set_crossfade(void) {
    StrBuilder input = {0};
    NcmError ncm_error;
    int32 seconds;
    bool prompted;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    prompted = action_runtime_prompt_string(STRLIT("Set crossfade to: "),
                                            "", false, NULL, NULL, &input);
    if (!prompted) {
        sb_free(&input);
        return 0;
    }

    ncm_error_clear(&ncm_error);
    if (ncm_parse_int32(input.data, input.len, &seconds, &ncm_error) < 0) {
        sb_free(&input);
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Crossfade must be a non-negative number"));
        return 0;
    }
    sb_free(&input);

    Config.mpd_crossfade_time = seconds;
    ncm_error_clear(&ncm_error);
    if (ncm_mpd_client_set_crossfade(&global_mpd, seconds, &ncm_error) < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    return 0;
}

static int32
action_runtime_set_volume(void) {
    StrBuilder input = {0};
    StrBuilder message = {0};
    NcmError ncm_error;
    int32 volume;
    bool prompted;

    if (!ncm_mpd_client_is_connected(&global_mpd)
        || (ncm_status_state_volume() < 0)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    prompted = action_runtime_prompt_string(STRLIT("Set volume to: "), "",
                                            false, NULL, NULL, &input);
    if (!prompted) {
        sb_free(&input);
        return 0;
    }

    ncm_error_clear(&ncm_error);
    if (ncm_parse_int32(input.data, input.len, &volume, &ncm_error) < 0
        || (volume > 100)) {
        sb_free(&input);
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Volume must be between 0 and 100"));
        return 0;
    }
    sb_free(&input);

    ncm_error_clear(&ncm_error);
    if (ncm_mpd_client_set_volume(&global_mpd, volume, &ncm_error) < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    sb_printf(&message, "Volume set to %d%%", volume);
    ncm_statusbar_print(Config.message_delay_time, message.data, message.len);
    sb_free(&message);
    return 0;
}

static int32
action_runtime_add_random_items(void) {
    NcmStatusbarScopedLock scoped_lock;
    StrBuilder input = {0};
    StrBuilder message = {0};
    StrBuilder prompt = {0};
    NcmError ncm_error;
    NcWindow *window;
    char values[] = {
        's',
        'a',
        'A',
        'b',
    };
    char tag_name[32];
    char *source_name;
    int32 count;
    int32 source_name_len;
    int32 number;
    enum NcmTagType tag_type = NCM_TAG_ARTIST;
    char random_type = 0;
    int32 status = 0;
    bool prompted;
    bool success;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    ncm_statusbar_scoped_lock_init(&scoped_lock);
    if ((window = ncm_statusbar_put())) {
        nc_window_print_data(window, STRLIT("Add random? [s]ongs/[a]rtists/"
                                         "album [A]rtists/al[b]ums "));
        status = ncm_statusbar_prompt_return_one_of(window, values,
                                                    LENGTH(values),
                                                    &random_type);
    }
    ncm_statusbar_scoped_lock_destroy(&scoped_lock);
    if (status < 0) {
        return status;
    }
    if (status == 0) {
        return 0;
    }

    if (random_type == 's') {
        source_name = "song";
        source_name_len = STRLIT_LEN("song");
    } else {
        tag_type = ncm_char_to_tag_type(random_type);
        source_name = ncm_tag_type_name(tag_type);
        source_name_len = optional_strlen32(source_name);
        if (source_name_len >= SIZEOF(tag_name)) {
            return -NCM_ERROR_UNAVAILABLE;
        }
        memcpy64(tag_name, source_name, source_name_len);
        tag_name[source_name_len] = '\0';
        ncm_string_lowercase_ascii(tag_name, source_name_len);
        source_name = tag_name;
    }

    SB_APPEND(&prompt, "Number of random ");
    SB_APPEND(&prompt, source_name, source_name_len);
    SB_APPEND(&prompt, "s: ");
    prompted = action_runtime_prompt_string(prompt.data, prompt.len, "", false,
                                            NULL, NULL, &input);
    sb_free(&prompt);
    if (!prompted) {
        sb_free(&input);
        return 0;
    }

    ncm_error_clear(&ncm_error);
    if (ncm_parse_int32(input.data, input.len, &number, &ncm_error) < 0) {
        sb_free(&input);
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Random item count must be a non-negative "
                                    "number"));
        return 0;
    }
    sb_free(&input);
    count = number;
    if (count <= 0) {
        return 0;
    }

    ncm_error_clear(&ncm_error);
    if (random_type == 's') {
        char *exclude_pattern = Config.random_exclude_pattern;
        int32 exclude_pattern_len = Config.random_exclude_pattern_len;

        success = ncm_mpd_client_add_random_songs(&global_mpd, count,
                                                  exclude_pattern,
                                                  exclude_pattern_len,
                                                  &ncm_error) == 0;
    } else {
        success = ncm_mpd_client_add_random_tag(&global_mpd, tag_type, count,
                                                &ncm_error) == 0;
    }
    if (!success) {
        return action_runtime_mpd_error_status(&ncm_error);
    }

    sb_printf(&message, "%d random ", count);
    SB_APPEND(&message, source_name, source_name_len);
    if (count != 1) {
        SB_APPEND(&message, "s");
    }
    SB_APPEND(&message, " added to playlist");
    ncm_statusbar_print(Config.message_delay_time, message.data, message.len);
    sb_free(&message);
    return 0;
}

static void
action_runtime_print_toggle(char *prefix, int32 prefix_len, char *value) {
    action_runtime_print_message(prefix, prefix_len, value,
                                 optional_strlen32(value), STRLIT(""));
    return;
}

static int32
action_runtime_toggle_interface(void) {
    NcmStatusbarScopedLock scoped_lock;

    switch (Config.user_interface) {
    case NCM_DESIGN_CLASSIC:
        Config.user_interface = NCM_DESIGN_ALTERNATIVE;
        Config.statusbar_visibility = false;
        break;
    case NCM_DESIGN_ALTERNATIVE:
        Config.user_interface = NCM_DESIGN_CLASSIC;
        Config.statusbar_visibility =
            ui_state_statusbar_visibility_is_baseline();
        break;
    case NCM_DESIGN_COUNT:
        return -NCM_ERROR_UNAVAILABLE;
    default:
        return -NCM_ERROR_UNAVAILABLE;
    }

    ncmpcpp_resize_screen(false);
    ncm_progressbar_scoped_lock_init(&scoped_lock);
    ncm_progressbar_scoped_lock_destroy(&scoped_lock);
    ncm_status_changes_mixer();
    ncm_status_changes_elapsed_time(false);
    action_runtime_print_toggle(STRLIT("User interface: "),
                                ncm_design_str(Config.user_interface));
    return 0;
}

static int32
action_runtime_toggle_separators_between_albums(void) {
    Config.playlist_separate_albums = !Config.playlist_separate_albums;
    app_controller_request_current_screen_resize();
    if (Config.playlist_separate_albums) {
        action_runtime_print_toggle(STRLIT("Separators between albums: "),
                                    "on");
    } else {
        action_runtime_print_toggle(STRLIT("Separators between albums: "),
                                    "off");
    }
    return 0;
}

static int32
action_runtime_toggle_lyrics_update_on_song_change(void) {
    if (!app_screen_lyrics_is_current()) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    Config.follow_now_playing_lyrics = !Config.follow_now_playing_lyrics;
    if (Config.follow_now_playing_lyrics) {
        action_runtime_print_toggle(STRLIT("Update lyrics if song changes: "),
                                    "on");
    } else {
        action_runtime_print_toggle(STRLIT("Update lyrics if song changes: "),
                                    "off");
    }
    return 0;
}

static int32
action_runtime_toggle_fetch_lyrics_in_background(void) {
    Config.fetch_lyrics_for_current_song_in_background =
        !Config.fetch_lyrics_for_current_song_in_background;
    if (Config.fetch_lyrics_for_current_song_in_background) {
        action_runtime_print_toggle(STRLIT("Fetching lyrics for playing "
                                            "songs in background: "),
                                    "on");
    } else {
        action_runtime_print_toggle(STRLIT("Fetching lyrics for playing "
                                            "songs in background: "),
                                    "off");
    }
    return 0;
}

static int32
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
        return -NCM_ERROR_UNAVAILABLE;
    }
    action_runtime_print_toggle(STRLIT("Add mode: "), mode_desc);
    return 0;
}

static int32
action_runtime_toggle_mouse(void) {
    Config.mouse_support = !Config.mouse_support;
    if (Config.mouse_support) {
        nc_mouse_enable();
        action_runtime_print_toggle(STRLIT("Mouse support "), "enabled");
    } else {
        nc_mouse_disable();
        action_runtime_print_toggle(STRLIT("Mouse support "), "disabled");
    }
    return 0;
}

static int32
action_runtime_toggle_bitrate_visibility(void) {
    Config.display_bitrate = !Config.display_bitrate;
    if (Config.display_bitrate) {
        action_runtime_print_toggle(STRLIT("Bitrate visibility "), "enabled");
    } else {
        action_runtime_print_toggle(STRLIT("Bitrate visibility "), "disabled");
    }
    return 0;
}

static int32
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

    ASSERT(position != NULL);

    if (text_len <= 0) {
        return -EINVAL;
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
            return -NCM_ERROR_PARSE;
        }
    }

    ncm_error_clear(&ncm_error);
    if (first_colon >= 0) {
        if ((first_colon == 0) || (first_colon == text_len - 1)) {
            return -NCM_ERROR_PARSE;
        }
        if (second_colon < 0) {
            if ((text_len - first_colon - 1) != 2) {
                return -NCM_ERROR_PARSE;
            }
            if (ncm_parse_int32(text, first_colon, &first, &ncm_error) < 0
                || ncm_parse_int32(text + first_colon + 1, 2, &second,
                                    &ncm_error) < 0 || (second > 60)) {
                return -NCM_ERROR_PARSE;
            }
            result = first*60 + second;
        } else {
            if (((second_colon - first_colon - 1) != 2)
                || ((text_len - second_colon - 1) != 2)) {
                return -NCM_ERROR_PARSE;
            }
            if (ncm_parse_int32(text, first_colon, &first, &ncm_error) < 0
                || ncm_parse_int32(text + first_colon + 1, 2, &second,
                                    &ncm_error) < 0
                || ncm_parse_int32(text + second_colon + 1, 2, &third,
                                    &ncm_error) < 0
                || (second > 60) || (third > 60)) {
                return -NCM_ERROR_PARSE;
            }
            result = first*3600 + second*60 + third;
        }
        if (result > MAXOF(*position)) {
            return -NCM_ERROR_PARSE;
        }
        *position = result;
        return 0;
    }

    number_len = text_len;
    if (text[text_len - 1] == 's') {
        number_len -= 1;
        if (number_len <= 0) {
            return -NCM_ERROR_PARSE;
        }
        if (ncm_parse_int32(text, number_len, &first, &ncm_error) < 0) {
            return -NCM_ERROR_PARSE;
        }
        *position = first;
        return 0;
    }
    if (text[text_len - 1] == '%') {
        number_len -= 1;
    }
    if (number_len <= 0) {
        return -NCM_ERROR_PARSE;
    }
    if (ncm_parse_int32(text, number_len, &first, &ncm_error) < 0
        || (first > 100)) {
        return -NCM_ERROR_PARSE;
    }
    *position = (first*total) / 100;
    return 0;
}

static int32
action_runtime_execute_command(void) {
    ActionRuntimeCommandPrompt state = {0};
    StrBuilder command_name = {0};
    NcmCommand *command;
    bool prompted;
    int32 status;

    prompted = action_runtime_prompt_string(
        STRLIT(":"), "", true, action_runtime_command_prompt_should_continue,
        &state, &command_name);
    if (!prompted && (state.previous.len > 0)) {
        sb_copy(&command_name, &state.previous);
        prompted = true;
    }
    sb_free(&state.previous);

    if (!prompted) {
        sb_free(&command_name);
        return 0;
    }

    command = ncm_bindings_config_find_command(&Bindings,
                                               command_name.data,
                                               command_name.len);
    if (command == NULL) {
        action_runtime_print_message(STRLIT("No command named \""),
                                     command_name.data, command_name.len,
                                     STRLIT("\""));
        sb_free(&command_name);
        return 0;
    }

    action_runtime_print_message(STRLIT("Executing "),
                                 command_name.data, command_name.len,
                                 STRLIT("..."));
    status = ncmpcpp_execute_binding(&command->binding);
    if (status == 0) {
        action_runtime_print_message(STRLIT("Execution of command \""),
                                     command_name.data, command_name.len,
                                     STRLIT("\" successful."));
    } else {
        action_runtime_print_message(STRLIT("Execution of command \""),
                                     command_name.data, command_name.len,
                                     STRLIT("\" unsuccessful."));
    }

    sb_free(&command_name);
    if (status == 0) {
        return 0;
    }
    if (status < 0) {
        return status;
    }
    return -NCM_ERROR_EXTERNAL_COMMAND;
}

static int32
action_runtime_save_playlist(void) {
    StrBuilder name = {0};
    NcmError ncm_error;
    bool prompted;
    bool success;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    prompted = action_runtime_prompt_string(STRLIT("Save playlist as: "),
                                            "", false, NULL, NULL, &name);
    if (!prompted) {
        sb_free(&name);
        return 0;
    }

    ncm_error_clear(&ncm_error);
    success = ncm_mpd_client_save_playlist(&global_mpd, name.data,
                                            &ncm_error) == 0;
    if (!success && (ncm_mpd_client_server_error_code(&global_mpd)
            == NCM_MPD_SERVER_ERROR_EXIST)) {
        StrBuilder question = {0};

        SB_APPEND(&question, "Playlist \"");
        SB_APPEND(&question, name.data, name.len);
        SB_APPEND(&question, "\" already exists, overwrite?");
        success = action_runtime_confirm(question.data, question.len);
        sb_free(&question);
        if (!success) {
            sb_free(&name);
            return 0;
        }

        ncm_error_clear(&ncm_error);
        success = ncm_mpd_client_delete_playlist(&global_mpd, name.data,
                                                 &ncm_error) == 0;
        if (success) {
            success = ncm_mpd_client_save_playlist(&global_mpd, name.data,
                                                   &ncm_error) == 0;
        }
        if (success) {
            ncm_statusbar_print(Config.message_delay_time,
                                STRLIT("Playlist overwritten"));
        }
    } else if (success) {
        action_runtime_print_message(STRLIT("Playlist saved as \""),
                                     name.data, name.len, STRLIT("\""));
    }

    sb_free(&name);
    if (!success) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    return 0;
}

static int32
action_runtime_apply_filter(void) {
    StringView current_filter;
    StrBuilder filter = {0};
    StrBuilder previous_filter = {0};
    NcmError ncm_error;
    int32 status;
    bool old_autocenter_mode;
    bool prompted;

    if (!current_screen_can_filter()) {
        return -NCM_ERROR_UNAVAILABLE;
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
        action_runtime_filter_prompt_should_continue, NULL, &filter);
    Config.autocenter_mode = old_autocenter_mode;

    if (!prompted) {
        ncm_error_clear(&ncm_error);
        (void)current_screen_apply_filter(previous_filter.data,
                                          previous_filter.len, &ncm_error);
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Action cancelled"));
        sb_free(&previous_filter);
        sb_free(&filter);
        return 0;
    }

    ncm_error_clear(&ncm_error);
    status = current_screen_apply_filter(filter.data, filter.len, &ncm_error);
    if (status < 0) {
        sb_free(&previous_filter);
        sb_free(&filter);
        return action_runtime_mpd_error_status(&ncm_error);
    }
    if (filter.len == 0) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Filtering disabled"));
    } else {
        action_runtime_print_message(STRLIT("Using filter \""),
                                     filter.data, filter.len, STRLIT("\""));
    }

    sb_free(&previous_filter);
    sb_free(&filter);
    return 0;
}

static int32
action_runtime_find(void) {
    StrBuilder token = {0};
    NcmError ncm_error;
    int32 status;
    bool found;
    bool prompted;

    if (!app_screen_help_is_current() && !app_screen_lastfm_is_current()
        && !app_screen_lyrics_is_current()) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    prompted = action_runtime_prompt_string(STRLIT("Find: "), "", false,
                                            NULL, NULL, &token);
    if (!prompted) {
        sb_free(&token);
        return 0;
    }

    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Searching..."));
    ncm_error_clear(&ncm_error);
    status = current_screen_search(NCM_SEARCH_DIRECTION_FORWARD, token.data,
                                   token.len, false, false, &ncm_error);
    if (status < 0) {
        sb_free(&token);
        return action_runtime_mpd_error_status(&ncm_error);
    }
    found = status > 0;

    if ((token.len == 0) || found) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Done"));
    } else {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("No matching patterns found"));
    }

    sb_free(&token);
    return 0;
}

static int32
action_runtime_find_item(enum SearchDirection direction) {
    ActionRuntimeSearchPrompt state;
    StringView current_constraint;
    StrBuilder constraint = {0};
    StrBuilder previous_constraint = {0};
    NcmError ncm_error;
    bool old_autocenter_mode;
    bool prompted;
    NcPromptShouldContinueFunc *should_continue;
    char prompt[64];
    int32 prompt_len;

    if (!current_screen_can_search()) {
        return -NCM_ERROR_UNAVAILABLE;
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
    should_continue = action_runtime_search_prompt_should_continue;
    prompted = action_runtime_prompt_string(prompt, prompt_len, "", false,
                                            should_continue, &state,
                                            &constraint);
    Config.autocenter_mode = old_autocenter_mode;

    if (!prompted) {
        if (previous_constraint.len == 0) {
            current_screen_clear_search_constraint();
        } else {
            ncm_error_clear(&ncm_error);
            (void)current_screen_search(
                direction, previous_constraint.data, previous_constraint.len,
                Config.default_find_mode == NCM_DEFAULT_FIND_MODE_WRAPPED,
                false, &ncm_error);
        }
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Action cancelled"));
        action_runtime_search_prompt_destroy(&state);
        sb_free(&previous_constraint);
        sb_free(&constraint);
        return 0;
    }

    if (constraint.len == 0) {
        current_screen_clear_search_constraint();
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Constraint unset"));
    } else {
        if (!ncm_search_prompt_state_has_cached_result(&state, constraint.data,
                                                       constraint.len, NULL)) {
            ncm_error_clear(&ncm_error);
            if (action_runtime_search_prompt_apply(&state, constraint.data,
                                                   constraint.len, NULL,
                                                   &ncm_error) < 0) {
                action_runtime_search_prompt_destroy(&state);
                sb_free(&previous_constraint);
                sb_free(&constraint);
                if (ncm_error_is_set(&ncm_error)) {
                    return action_runtime_mpd_error_status(&ncm_error);
                }
                return -NCM_ERROR_UNAVAILABLE;
            }
        }
        action_runtime_print_message(STRLIT("Using constraint \""),
                                     constraint.data, constraint.len,
                                     STRLIT("\""));
    }

    action_runtime_search_prompt_destroy(&state);
    sb_free(&previous_constraint);
    sb_free(&constraint);
    return 0;
}

static int32
action_runtime_repeat_search(enum SearchDirection direction) {
    StringView constraint;
    NcmError ncm_error;
    int32 status;

    if (!current_screen_can_search()) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    constraint = current_screen_current_search_constraint();
    if ((constraint.data == NULL) || (constraint.len <= 0)) {
        return 0;
    }

    ncm_error_clear(&ncm_error);
    status = current_screen_search(
        direction, constraint.data, constraint.len,
        Config.default_find_mode == NCM_DEFAULT_FIND_MODE_WRAPPED, true,
        &ncm_error);
    if (status < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    return 0;
}

static int32
action_runtime_current_menu_height(void) {
    return nc_screen_current_menu_height(app_controller_current_screen());
}

static bool
action_runtime_playlist_edit_playlists_is_active(void) {
    PlaylistEditScreen *screen = app_screen_playlist_edit();

    if (!action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return false;
    }
    return playlist_edit_screen_active_menu(screen)
           == nc_playlist_entry_menu_base(
               playlist_edit_screen_playlists(screen));
}

static bool
action_runtime_playlist_edit_content_is_active(void) {
    PlaylistEditScreen *screen = app_screen_playlist_edit();

    if (!action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return false;
    }
    return playlist_edit_screen_active_menu(screen)
           == nc_song_menu_base(playlist_edit_screen_content(screen));
}

static bool
action_runtime_playlist_edit_has_playlists(void) {
    PlaylistEditScreen *screen = app_screen_playlist_edit();

    if (!action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return false;
    }
    return nc_menu_all_item_count(nc_playlist_entry_menu_base(
        playlist_edit_screen_playlists(screen))) > 0;
}

static bool
action_runtime_playlist_edit_has_content(void) {
    PlaylistEditScreen *screen = app_screen_playlist_edit();

    if (!action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return false;
    }
    return nc_menu_all_item_count(
        nc_song_menu_base(playlist_edit_screen_content(screen))) > 0;
}

static NcMenu *
action_runtime_current_tag_scroll_menu(void) {
    return nc_screen_tag_menu(app_controller_current_screen());
}


static int32
action_runtime_song_tag_at(int32 pos, enum SongGetter getter,
                           StrBuilder *tag) {
    return nc_screen_song_tag_at(app_controller_current_screen(), pos, getter,
                                 tag);
}

static bool
action_runtime_tag_scroll_available(enum SongGetter getter) {
    StrBuilder tag = {0};
    NcMenu *menu;
    bool available;

    if (((menu = action_runtime_current_tag_scroll_menu()) == NULL)
        || (nc_menu_item_count(menu) <= 0)) {
        return false;
    }

    available = action_runtime_song_tag_at(nc_menu_highlight(menu), getter,
                                           &tag) == 0;
    sb_free(&tag);
    return available;
}

static int32
action_runtime_scroll_by_tag(enum SongGetter getter, bool down) {
    StrBuilder current_tag;
    StrBuilder other_tag;
    NcMenu *menu;
    int32 current;
    int32 target;
    int32 count;
    int32 step;
    bool same;

    if (((menu = action_runtime_current_tag_scroll_menu()) == NULL)
        || (nc_menu_item_count(menu) <= 0)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    current = nc_menu_highlight(menu);
    if (action_runtime_song_tag_at(current, getter, &current_tag) < 0) {
        return -NCM_ERROR_UNAVAILABLE;
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
        if (action_runtime_song_tag_at(next, getter, &other_tag) < 0) {
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
    return 0;
}

static int32
action_runtime_selected_songs(NcmSongArray *songs) {
    return nc_screen_selected_songs(app_controller_current_screen(), songs);
}

static bool
action_runtime_has_selected_songs(void) {
    NcmSongArray songs;
    bool result;

    songs = (NcmSongArray){0};
    result = (action_runtime_selected_songs(&songs) == 0) && (songs.len > 0);
    ncm_song_array_destroy(&songs);
    return result;
}

static int32
action_runtime_current_song(NcmSong *song) {
    return nc_screen_current_song(app_controller_current_screen(), song);
}

static bool
action_runtime_has_current_song(void) {
    NcmSong song;
    bool result;

    song = (NcmSong){0};
    result = action_runtime_current_song(&song) == 0;
    ncm_song_destroy(&song);
    return result;
}

static void
action_runtime_sort_positions(int32 *positions, int32 count, bool descending) {
    int32 value;

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

static int32
action_runtime_song_positions(NcmSongArray *songs,
                              int32 **positions, int32 *count) {
    int32 *result;

    if (songs->len <= 0) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    result = malloc2(songs->len*SIZEOF(*result));
    for (int32 i = 0; i < songs->len; i += 1) {
        result[i] = ncm_song_position(&songs->items[i]);
    }

    *positions = result;
    *count = songs->len;
    return 0;
}

static int32
action_runtime_add_prompt(void) {
    StrBuilder path = {0};
    StrBuilder message = {0};
    NcmError ncm_error;
    enum NcmMpdServerError server_error;
    bool prompted;
    bool success;

    prompted = action_runtime_prompt_string(STRLIT("Add: "), "", false,
                                            NULL, NULL, &path);
    if (!prompted) {
        sb_free(&path);
        return 0;
    }

    if ((path.len <= 0) && !action_runtime_confirm(
            STRLIT("Are you sure you want to add the whole database?"))) {
        sb_free(&path);
        return 0;
    }

    {
        char *path_text = path.data;
        bool added = false;

        if (path_text == NULL) {
            path_text = "";
        }

        ncm_statusbar_print(0,
                            STRLIT("Adding..."));
        ncm_error_clear(&ncm_error);
        success = ncm_mpd_client_add(&global_mpd, path_text, &added,
                                     &ncm_error) == 0;
        server_error = ncm_mpd_client_server_error_code(&global_mpd);
        if (!success && (server_error == NCM_MPD_SERVER_ERROR_NO_EXIST)) {
            bool loaded = false;

            ncm_error_clear(&ncm_error);
            success = ncm_mpd_client_load_playlist(&global_mpd, path_text,
                                                   &loaded, &ncm_error) == 0;
            sb_free(&path);
            if (!success) {
                return action_runtime_mpd_error_status(&ncm_error);
            }
            return 0;
        }
    }
    sb_free(&path);

    if (!success && (server_error != NCM_MPD_SERVER_ERROR_NONE)) {
        SB_APPEND(&message, "Error while adding item: ");
        if (ncm_error_is_set(&ncm_error)) {
            SB_APPEND(&message, ncm_error.message,
                      ncm_error.message_len);
        }
        ncm_statusbar_print(Config.message_delay_time, message.data,
                            message.len);
        sb_free(&message);
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!success) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    return 0;
}

static int32
action_runtime_load_prompt(void) {
    StrBuilder name = {0};
    NcmError ncm_error;
    bool prompted = action_runtime_prompt_string(STRLIT("Load playlist: "), "",
                                                 false, NULL, NULL, &name);

    if (!prompted) {
        sb_free(&name);
        return 0;
    }

    {
        char *name_text = name.data;
        bool loaded = false;

        if (name_text == NULL) {
            name_text = "";
        }

        ncm_statusbar_print(0,
                            STRLIT("Loading..."));
        ncm_error_clear(&ncm_error);
        if (ncm_mpd_client_load_playlist(&global_mpd, name_text, &loaded,
                                          &ncm_error) < 0) {
            sb_free(&name);
            return action_runtime_mpd_error_status(&ncm_error);
        }
    }

    sb_free(&name);
    return 0;
}

static int32
action_runtime_add_selected_songs(bool play) {
    NcmSongArray songs;
    int32 status;
    bool first = true;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    songs = (NcmSongArray){0};
    status = action_runtime_selected_songs(&songs);
    if ((status < 0) || (songs.len <= 0)) {
        ncm_song_array_destroy(&songs);
        if (status < 0) {
            return status;
        }
        return -NCM_ERROR_UNAVAILABLE;
    }

    for (int32 i = 0; i < songs.len; i += 1) {
        status = ncm_action_add_song_to_playlist(&songs.items[i],
                                                 play && first, -1);
        if (status < 0) {
            ncm_song_array_destroy(&songs);
            return status;
        }
        first = false;
    }

    ncm_song_array_destroy(&songs);
    return 0;
}

static int32
action_runtime_add_playlist_edit_item(bool play) {
    PlaylistEditScreen *screen = app_screen_playlist_edit();
    NcmPlaylist playlist;
    NcmError ncm_error;
    int32 play_position;
    bool loaded;
    bool success;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (action_runtime_playlist_edit_content_is_active()) {
        return action_runtime_add_selected_songs(play);
    }
    if (!action_runtime_playlist_edit_playlists_is_active()) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    playlist = (NcmPlaylist){0};
    success = playlist_edit_screen_current_playlist(screen, &playlist) > 0;
    if (!success) {
        ncm_playlist_destroy(&playlist);
        return -NCM_ERROR_UNAVAILABLE;
    }

    loaded = false;
    play_position = ncm_status_state_playlist_length();
    ncm_error_clear(&ncm_error);
    success = ncm_mpd_client_load_playlist(&global_mpd, playlist.path, &loaded,
                                           &ncm_error) == 0;
    if (success && play && loaded) {
        success = ncm_mpd_client_play_pos(&global_mpd, play_position,
                                          &ncm_error) == 0;
    }
    ncm_playlist_destroy(&playlist);
    if (!success) {
        return action_runtime_mpd_error_status(&ncm_error);
    }

    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return 0;
}

static int32
action_runtime_add_item_to_playlist(bool play) {
    NcmError ncm_error;
    int32 status;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    if (action_runtime_current_screen_is(SCREEN_TYPE_MEDIA_LIBRARY)) {
        ncm_error_clear(&ncm_error);
        status = media_library_screen_add_item_to_playlist(
            app_screen_media_library(), play, &ncm_error);
        if (status < 0) {
            return action_runtime_mpd_error_status(&ncm_error);
        }
        return 0;
    }

    if (action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return action_runtime_add_playlist_edit_item(play);
    }

    return action_runtime_add_selected_songs(play);
}

static int32
action_runtime_browser_item_name(NcmMpdItem *item, StrBuilder *name) {
    StringView view;
    int32 basename;

    sb_clear(name);
    ncm_string_view_clear(&view);

    switch (ncm_mpd_item_kind(item)) {
    case NCM_MPD_ITEM_DIRECTORY:
        if (!ncm_directory_has_path_view(ncm_mpd_item_directory(item), &view)) {
            return -NCM_ERROR_UNAVAILABLE;
        }
        break;
    case NCM_MPD_ITEM_SONG:
        if (!ncm_song_has_name_view(ncm_mpd_item_song(item), 0, &view)
            && !ncm_song_has_uri_view(ncm_mpd_item_song(item), 0, &view)) {
            return -NCM_ERROR_UNAVAILABLE;
        }
        break;
    case NCM_MPD_ITEM_PLAYLIST:
        if (!ncm_playlist_has_path_view(ncm_mpd_item_playlist(item), &view)) {
            return -NCM_ERROR_UNAVAILABLE;
        }
        break;
    case NCM_MPD_ITEM_COUNT:
    default:
        return -NCM_ERROR_UNAVAILABLE;
    }

    basename = ncm_path_basename_start(view.data, view.len);
    SB_APPEND(name, view.data + basename, view.len - basename);
    return 0;
}

static int32
action_runtime_delete_browser_items(void) {
    BrowserScreen *screen = app_screen_browser();
    NcMenu *menu;
    NcmMpdItem *item;
    StrBuilder question = {0};
    StrBuilder name = {0};
    NcmError ncm_error;
    bool success;
    bool has_selected;

    if (!action_runtime_current_screen_is(SCREEN_TYPE_BROWSER)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (((menu = browser_screen_menu(screen)) == NULL)
        || (nc_menu_item_count(menu) <= 0)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!Config.allow_for_physical_item_deletion) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Flag \"allow_for_physical_item_deletion\" "
                                    "needs to be enabled in configuration "
                                    "file"));
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!browser_screen_is_local(screen) && (Config.mpd_music_dir_len <= 0)) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Proper mpd_music_dir variable has to be "
                                    "set in configuration file"));
        return -NCM_ERROR_UNAVAILABLE;
    }

    has_selected = nc_menu_has_selected(menu);
    if (has_selected) {
        SB_APPEND(&question, "Delete selected items?");
    } else {
        item = nc_menu_current_item(menu);
        if (browser_screen_item_is_parent(item)) {
            sb_free(&name);
            sb_free(&question);
            return 0;
        }
        if (action_runtime_browser_item_name(item, &name) < 0) {
            sb_free(&name);
            sb_free(&question);
            return -NCM_ERROR_UNAVAILABLE;
        }
        SB_APPEND(&question, "Delete \"");
        SB_APPEND(&question, name.data, name.len);
        SB_APPEND(&question, "\"?");
    }

    success = action_runtime_confirm(question.data, question.len);
    sb_free(&name);
    sb_free(&question);
    if (!success) {
        return 0;
    }

    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Deleting items..."));
    ncm_error_clear(&ncm_error);
    if (browser_screen_delete_items(screen, &global_mpd, &ncm_error) < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Item(s) deleted"));
    return 0;
}

static void
action_runtime_print_renamed(char *prefix, int32 prefix_len, StrBuilder *name) {
    StrBuilder message = {0};

    SB_APPEND(&message, prefix, prefix_len);
    SB_APPEND(&message, name->data, name->len);
    SB_APPEND(&message, "\"");
    ncm_statusbar_print(Config.message_delay_time, message.data, message.len);
    sb_free(&message);
    return;
}

static int32
action_runtime_delete_main_playlist_items(void) {
    NcmSongArray songs;
    NcmError ncm_error;
    int32 *positions;
    int32 count;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    songs = (NcmSongArray){0};
    if (playlist_screen_selected_songs(app_screen_playlist(), &songs) < 0) {
        ncm_song_array_destroy(&songs);
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (action_runtime_song_positions(&songs, &positions, &count) < 0) {
        ncm_song_array_destroy(&songs);
        return -NCM_ERROR_UNAVAILABLE;
    }

    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Deleting items..."));
    action_runtime_sort_positions(positions, count, true);
    ncm_error_clear(&ncm_error);
    for (int32 i = 0; i < count; i += 1) {
        if (ncm_mpd_client_delete(&global_mpd, positions[i], &ncm_error) < 0) {
            free2(positions, count*SIZEOF(*positions));
            ncm_song_array_destroy(&songs);
            return action_runtime_mpd_error_status(&ncm_error);
        }
    }

    free2(positions, count*SIZEOF(*positions));
    ncm_song_array_destroy(&songs);
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Item(s) deleted"));
    return 0;
}

static int32
action_runtime_delete_playlist_edit_items(void) {
    PlaylistEditScreen *screen = app_screen_playlist_edit();
    NcmPlaylist playlist;
    NcmSongArray songs;
    NcmError ncm_error;
    int32 *positions;
    int32 count;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!action_runtime_playlist_edit_content_is_active()) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    playlist = (NcmPlaylist){0};
    songs = (NcmSongArray){0};
    if (playlist_edit_screen_current_playlist(screen, &playlist) <= 0) {
        ncm_playlist_destroy(&playlist);
        ncm_song_array_destroy(&songs);
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (playlist_edit_screen_selected_songs(screen, &songs) < 0) {
        ncm_playlist_destroy(&playlist);
        ncm_song_array_destroy(&songs);
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (action_runtime_song_positions(&songs, &positions, &count) < 0) {
        ncm_playlist_destroy(&playlist);
        ncm_song_array_destroy(&songs);
        return -NCM_ERROR_UNAVAILABLE;
    }

    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Deleting items..."));
    action_runtime_sort_positions(positions, count, true);
    ncm_error_clear(&ncm_error);
    for (int32 i = 0; i < count; i += 1) {
        if (ncm_mpd_client_playlist_delete(&global_mpd, playlist.path,
                                            positions[i], &ncm_error) < 0) {
            free2(positions, count*SIZEOF(*positions));
            ncm_playlist_destroy(&playlist);
            ncm_song_array_destroy(&songs);
            return action_runtime_mpd_error_status(&ncm_error);
        }
    }

    free2(positions, count*SIZEOF(*positions));
    ncm_playlist_destroy(&playlist);
    ncm_song_array_destroy(&songs);
    playlist_edit_screen_request_content_update(screen);
    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Item(s) deleted"));
    return 0;
}

static int32
action_runtime_delete_playlist_items(void) {
    if (action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST)) {
        return action_runtime_delete_main_playlist_items();
    }
    if (action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return action_runtime_delete_playlist_edit_items();
    }
    return -NCM_ERROR_UNAVAILABLE;
}

static int32
action_runtime_delete_stored_playlists(void) {
    PlaylistEditScreen *screen = app_screen_playlist_edit();
    NcMenu *menu;
    NcmPlaylist *playlist;
    StrBuilder question = {0};
    NcmError ncm_error;
    enum NcMenuItemSource source;
    int32 count;
    bool has_selected;
    bool success;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!action_runtime_playlist_edit_playlists_is_active()) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!action_runtime_playlist_edit_has_playlists()) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    menu = nc_playlist_entry_menu_base(playlist_edit_screen_playlists(screen));
    source = action_runtime_menu_item_source(menu);
    has_selected = nc_menu_has_selected(menu);

    if (has_selected) {
        SB_APPEND(&question, "Delete selected playlists?");
    } else {
        if (((playlist = nc_menu_current_item(menu)) == NULL)
            || (playlist->path == NULL)) {
            sb_free(&question);
            return -NCM_ERROR_UNAVAILABLE;
        }
        SB_APPEND(&question, "Delete playlist \"");
        SB_APPEND(&question, playlist->path, playlist->path_len);
        SB_APPEND(&question, "\"?");
    }
    success = action_runtime_confirm(question.data, question.len);
    sb_free(&question);
    if (!success) {
        return 0;
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
                                                 &ncm_error) == 0;
    }
    if (!success) {
        return action_runtime_mpd_error_status(&ncm_error);
    }

    playlist_edit_screen_request_playlists_update(screen);
    if (has_selected) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Playlists deleted"));
    } else {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Playlist deleted"));
    }
    return 0;
}

static int32
action_runtime_clear_playlist(bool main_playlist) {
    PlaylistEditScreen *screen = app_screen_playlist_edit();
    NcmPlaylist playlist;
    StrBuilder question = {0};
    StrBuilder message = {0};
    NcmError ncm_error;
    bool success = false;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    ncm_error_clear(&ncm_error);
    if (main_playlist) {
        if (!playlist_screen_is_empty(app_screen_playlist())
            && Config.ask_before_clearing_playlists && !action_runtime_confirm(
                STRLIT("Do you really want to clear main playlist?"))) {
            return 0;
        }
        if (ncm_mpd_client_clear_queue(&global_mpd, &ncm_error) < 0) {
            return action_runtime_mpd_error_status(&ncm_error);
        }
        playlist_screen_clear(app_screen_playlist());
        (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Playlist cleared"));
        return 0;
    }

    if (!action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!action_runtime_playlist_edit_has_playlists()) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    playlist = (NcmPlaylist){0};
    success = playlist_edit_screen_current_playlist(screen, &playlist) > 0;
    if (!success) {
        ncm_playlist_destroy(&playlist);
        return -NCM_ERROR_UNAVAILABLE;
    }

    if (Config.ask_before_clearing_playlists) {

        SB_APPEND(&question, "Do you really want to clear playlist \"");
        SB_APPEND(&question, playlist.path, playlist.path_len);
        SB_APPEND(&question, "\"?");

        success = action_runtime_confirm(question.data, question.len);
        sb_free(&question);
        if (!success) {
            ncm_playlist_destroy(&playlist);
            return 0;
        }
    }

    success = ncm_mpd_client_clear_playlist(&global_mpd, playlist.path,
                                            &ncm_error) == 0;
    if (success) {
        SB_APPEND(&message, "Playlist \"");
        SB_APPEND(&message, playlist.path, playlist.path_len);
        SB_APPEND(&message, "\" cleared");
        ncm_statusbar_print(Config.message_delay_time, message.data,
                            message.len);
        sb_free(&message);
    }
    ncm_playlist_destroy(&playlist);
    if (!success) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    playlist_edit_screen_request_content_update(screen);
    return 0;
}

static int32
action_runtime_crop_playlist(bool main_playlist) {
    PlaylistEditScreen *editor = app_screen_playlist_edit();
    NcmPlaylist playlist;
    NcmSongArray songs;
    StrBuilder question = {0};
    StrBuilder message = {0};
    NcmError ncm_error;
    bool success = false;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    songs = (NcmSongArray){0};
    if (main_playlist) {
        if (playlist_screen_song_count(app_screen_playlist()) <= 1) {
            ncm_song_array_destroy(&songs);
            return 0;
        }
        if (Config.ask_before_clearing_playlists && !action_runtime_confirm(
                STRLIT("Do you really want to crop main playlist?"))) {
            ncm_song_array_destroy(&songs);
            return 0;
        }
        success = playlist_screen_selected_songs(app_screen_playlist(),
                                                 &songs) == 0;
    } else if (action_runtime_current_screen_is(
        SCREEN_TYPE_PLAYLIST_EDITOR)) {
        if (!action_runtime_playlist_edit_has_playlists()) {
            ncm_song_array_destroy(&songs);
            return -NCM_ERROR_UNAVAILABLE;
        }
        if (action_runtime_playlist_edit_has_content()
            && (nc_menu_all_item_count(
                nc_song_menu_base(playlist_edit_screen_content(
                    app_screen_playlist_edit()))) <= 1)) {
            ncm_song_array_destroy(&songs);
            return 0;
        }
        success = playlist_edit_screen_selected_songs(
            app_screen_playlist_edit(), &songs) == 0;
    }
    if (!success || (songs.len <= 0)) {
        ncm_song_array_destroy(&songs);
        return -NCM_ERROR_UNAVAILABLE;
    }

    ncm_error_clear(&ncm_error);
    if (main_playlist) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Cropping playlist..."));
        if (ncm_mpd_client_clear_queue(&global_mpd, &ncm_error) < 0) {
            ncm_song_array_destroy(&songs);
            return action_runtime_mpd_error_status(&ncm_error);
        }
        for (int32 i = 0; i < songs.len; i += 1) {
            if (ncm_mpd_client_add_song_value(&global_mpd, &songs.items[i], -1,
                                               NULL, &ncm_error) < 0) {
                ncm_song_array_destroy(&songs);
                return action_runtime_mpd_error_status(&ncm_error);
            }
        }
        (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
        ncm_song_array_destroy(&songs);
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Playlist cropped"));
        return 0;
    }

    playlist = (NcmPlaylist){0};
    success = playlist_edit_screen_current_playlist(editor, &playlist) > 0;
    if (success && Config.ask_before_clearing_playlists) {
        SB_APPEND(&question, "Do you really want to crop playlist \"");
        SB_APPEND(&question, playlist.path, playlist.path_len);
        SB_APPEND(&question, "\"?");
        success = action_runtime_confirm(question.data, question.len);
        sb_free(&question);
        if (!success) {
            ncm_playlist_destroy(&playlist);
            ncm_song_array_destroy(&songs);
            return 0;
        }
    }
    if (success) {
        SB_APPEND(&message, "Cropping playlist \"");
        SB_APPEND(&message, playlist.path, playlist.path_len);
        SB_APPEND(&message, "\"...");
        ncm_statusbar_print(Config.message_delay_time, message.data,
                            message.len);
        sb_free(&message);
        success = ncm_mpd_client_clear_playlist(&global_mpd, playlist.path,
                                                &ncm_error) == 0;
    }
    for (int32 i = 0; success && (i < songs.len); i += 1) {
        success = ncm_mpd_client_add_song_to_playlist(
            &global_mpd, playlist.path, &songs.items[i], &ncm_error) == 0;
    }
    if (success) {
        SB_APPEND(&message, "Playlist \"");
        SB_APPEND(&message, playlist.path, playlist.path_len);
        SB_APPEND(&message, "\" cropped");
        ncm_statusbar_print(Config.message_delay_time, message.data,
                            message.len);
        sb_free(&message);
    }
    ncm_playlist_destroy(&playlist);
    ncm_song_array_destroy(&songs);
    if (!success) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    playlist_edit_screen_request_content_update(editor);
    return 0;
}

static int32
action_runtime_move_main_playlist_items(NcmSongArray *songs, bool down) {
    NcmError ncm_error;
    int32 *positions;
    int32 count;
    bool success;

    if (action_runtime_song_positions(songs, &positions, &count) < 0) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    action_runtime_sort_positions(positions, count, down);
    ncm_error_clear(&ncm_error);
    success = ncm_mpd_client_start_command_list(&global_mpd, &ncm_error) == 0;
    for (int32 i = 0; success && (i < count); i += 1) {
        if (down) {
            if (positions[i] + 1 >= ncm_status_state_playlist_length()) {
                continue;
            }
            success = ncm_mpd_client_swap(&global_mpd, positions[i],
                                          positions[i] + 1, &ncm_error) == 0;
        } else {
            if (positions[i] == 0) {
                continue;
            }
            success = ncm_mpd_client_swap(&global_mpd, positions[i],
                                          positions[i] - 1, &ncm_error) == 0;
        }
    }
    if (success) {
        success = ncm_mpd_client_commit_command_list(&global_mpd,
                                                      &ncm_error) == 0;
    }
    if (!success && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }

    free2(positions, count*SIZEOF(*positions));
    if (!success) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return 0;
}

static int32
action_runtime_move_stored_playlist_items(NcmSongArray *songs, bool down) {
    PlaylistEditScreen *screen = app_screen_playlist_edit();
    NcmPlaylist playlist;
    NcmError ncm_error;
    int32 *positions;
    int32 item_count;
    int32 count;
    bool success;

    if (action_runtime_song_positions(songs, &positions, &count) < 0) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    playlist = (NcmPlaylist){0};
    success = playlist_edit_screen_current_playlist(screen, &playlist) > 0;
    if (!success) {
        free2(positions, count*SIZEOF(*positions));
        ncm_playlist_destroy(&playlist);
        return -NCM_ERROR_UNAVAILABLE;
    }

    action_runtime_sort_positions(positions, count, down);
    item_count = nc_menu_all_item_count(
        playlist_edit_screen_active_menu(screen));
    ncm_error_clear(&ncm_error);
    success = ncm_mpd_client_start_command_list(&global_mpd, &ncm_error) == 0;
    for (int32 i = 0; success && (i < count); i += 1) {
        if (down) {
            if (positions[i] + 1 >= item_count) {
                continue;
            }
            success = ncm_mpd_client_playlist_move(&global_mpd, playlist.path,
                                                   positions[i],
                                                   positions[i] + 1,
                                                   &ncm_error) == 0;
        } else if (positions[i] > 0) {
            success = ncm_mpd_client_playlist_move(&global_mpd, playlist.path,
                                                   positions[i],
                                                   positions[i] - 1,
                                                   &ncm_error) == 0;
        }
    }
    if (success) {
        success = ncm_mpd_client_commit_command_list(&global_mpd,
                                                      &ncm_error) == 0;
    }
    if (!success && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }

    free2(positions, count*SIZEOF(*positions));
    ncm_playlist_destroy(&playlist);
    if (!success) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    playlist_edit_screen_request_content_update(screen);
    return 0;
}

static int32
action_runtime_move_selected_items(bool down) {
    NcmSongArray songs;
    NcMenu *menu;
    enum ScreenType screen_type = app_screens_current_type();
    int32 status;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    if ((screen_type != SCREEN_TYPE_PLAYLIST)
        && (screen_type != SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if ((menu = action_runtime_current_menu()) && nc_menu_is_filtered(menu)) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Moving items is disabled in filtered "
                                    "playlist"));
        return 0;
    }

    songs = (NcmSongArray){0};
    status = action_runtime_selected_songs(&songs);
    if ((status < 0) || (songs.len <= 0)) {
        ncm_song_array_destroy(&songs);
        if (status < 0) {
            return status;
        }
        return -NCM_ERROR_UNAVAILABLE;
    }

    if (screen_type == SCREEN_TYPE_PLAYLIST) {
        status = action_runtime_move_main_playlist_items(&songs, down);
    } else if (screen_type == SCREEN_TYPE_PLAYLIST_EDITOR) {
        status = action_runtime_move_stored_playlist_items(&songs, down);
    } else {
        status = -NCM_ERROR_UNAVAILABLE;
    }

    ncm_song_array_destroy(&songs);
    return status;
}

static int32
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

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    if (((menu = playlist_screen_menu(screen)) == NULL)
        || !nc_menu_has_selected(menu)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if ((song = nc_menu_active_item_at(menu,
                                        nc_menu_highlight(menu))) == NULL) {
        return -NCM_ERROR_UNAVAILABLE;
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
            return -NCM_ERROR_UNAVAILABLE;
        }
        positions[count] = ncm_song_position(song);
        count += 1;
    }
    if (count <= 0) {
        free2(positions, item_count*SIZEOF(*positions));
        return -NCM_ERROR_UNAVAILABLE;
    }

    action_runtime_sort_positions(positions, count, false);
    if ((target >= positions[0]) && (target <= positions[count - 1])) {
        free2(positions, item_count*SIZEOF(*positions));
        return 0;
    }

    ncm_error_clear(&ncm_error);
    if ((success = ncm_mpd_client_start_command_list(&global_mpd,
                                                     &ncm_error) == 0)
        && (target > positions[0])) {
        destination = target - count;
        for (int32 i = count; success && (i > 0); i -= 1) {
            success = ncm_mpd_client_move(&global_mpd, positions[i - 1],
                                          destination + i - 1, &ncm_error) == 0;
        }
    } else if (success) {
        destination = target;
        for (int32 i = 0; success && (i < count); i += 1) {
            success = ncm_mpd_client_move(&global_mpd, positions[i],
                                          destination + i, &ncm_error) == 0;
        }
    }
    if (success) {
        success = ncm_mpd_client_commit_command_list(&global_mpd,
                                                      &ncm_error) == 0;
    }
    if (!success && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }
    free2(positions, item_count*SIZEOF(*positions));
    if (!success) {
        return action_runtime_mpd_error_status(&ncm_error);
    }

    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return 0;
}

static int32
action_runtime_move_playlist_edit_items_to(void) {
    PlaylistEditScreen *screen = app_screen_playlist_edit();
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

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!action_runtime_playlist_edit_content_is_active()) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    if (((menu = nc_song_menu_base(
             playlist_edit_screen_content(screen))) == NULL)
        || !nc_menu_has_selected(menu)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (nc_menu_is_filtered(menu)) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Moving items is disabled in filtered "
                                    "playlist"));
        return 0;
    }

    if ((song = nc_menu_active_item_at(menu,
                                        nc_menu_highlight(menu))) == NULL) {
        return -NCM_ERROR_UNAVAILABLE;
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
            return -NCM_ERROR_UNAVAILABLE;
        }
        positions[count] = ncm_song_position(song);
        count += 1;
    }
    if (count <= 0) {
        free2(positions, item_count*SIZEOF(*positions));
        return -NCM_ERROR_UNAVAILABLE;
    }

    playlist = (NcmPlaylist){0};
    success = playlist_edit_screen_current_playlist(screen, &playlist) > 0;
    if (!success) {
        ncm_playlist_destroy(&playlist);
        free2(positions, item_count*SIZEOF(*positions));
        return -NCM_ERROR_UNAVAILABLE;
    }

    action_runtime_sort_positions(positions, count, false);
    if ((target >= positions[0]) && (target <= positions[count - 1])) {
        ncm_playlist_destroy(&playlist);
        free2(positions, item_count*SIZEOF(*positions));
        return 0;
    }

    ncm_error_clear(&ncm_error);
    if ((success = ncm_mpd_client_start_command_list(&global_mpd,
                                                     &ncm_error) == 0)
        && (target > positions[0])) {
        destination = target - count;
        for (int32 i = count; success && (i > 0); i -= 1) {
            success = ncm_mpd_client_playlist_move(&global_mpd, playlist.path,
                                                   positions[i - 1],
                                                   destination + i - 1,
                                                   &ncm_error) == 0;
        }
    } else if (success) {
        destination = target;
        for (int32 i = 0; success && (i < count); i += 1) {
            success = ncm_mpd_client_playlist_move(
                &global_mpd, playlist.path, positions[i],
                destination + i, &ncm_error) == 0;
        }
    }
    if (success) {
        success = ncm_mpd_client_commit_command_list(&global_mpd,
                                                      &ncm_error) == 0;
    }
    if (!success && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }

    ncm_playlist_destroy(&playlist);
    free2(positions, item_count*SIZEOF(*positions));
    if (!success) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    playlist_edit_screen_request_content_update(screen);
    return 0;
}

static int32
action_runtime_move_selected_items_to(void) {
    if (action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST)) {
        return action_runtime_move_main_playlist_items_to();
    }
    if (action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST_EDITOR)) {
        return action_runtime_move_playlist_edit_items_to();
    }
    return -NCM_ERROR_UNAVAILABLE;
}

static int32
action_runtime_playlist_range(NcMenu *menu, int32 *first, int32 *last) {
    enum NcMenuItemSource source;
    int32 range_first;
    int32 range_last;
    NcmSong *song;

    ASSERT(first != NULL);
    ASSERT(last != NULL);

    source = action_runtime_menu_item_source(menu);
    if (ncm_menu_find_full_selected_range(menu, source, &range_first,
                                          &range_last) < 0) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (range_first >= range_last) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    if ((song = nc_menu_active_item_at(menu, range_first)) == NULL) {
        return -NCM_ERROR_NOT_FOUND;
    }
    *first = ncm_song_position(song);
    if ((song = nc_menu_active_item_at(menu, range_last - 1)) == NULL) {
        return -NCM_ERROR_NOT_FOUND;
    }
    *last = ncm_song_position(song) + 1;
    return 0;
}

static int32
action_runtime_reverse_playlist(void) {
    enum NcMenuItemSource source;
    NcMenu *menu;
    NcmSong *left;
    NcmSong *right;
    NcmError ncm_error;
    int32 first;
    int32 last;
    bool success;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    menu = action_runtime_current_menu();
    source = action_runtime_menu_item_source(menu);
    if (ncm_menu_find_full_selected_range(menu, source, &first, &last) < 0) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (first >= last) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    last -= 1;
    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Reversing range..."));
    ncm_error_clear(&ncm_error);
    success = ncm_mpd_client_start_command_list(&global_mpd, &ncm_error) == 0;
    while (success && (first < last)) {
        if (((left = nc_menu_active_item_at(menu, first)) == NULL)
            || ((right = nc_menu_active_item_at(menu, last)) == NULL)) {
            success = false;
            break;
        }
        success = ncm_mpd_client_swap(&global_mpd, ncm_song_position(left),
                                      ncm_song_position(right),
                                      &ncm_error) == 0;
        first += 1;
        last -= 1;
    }
    if (success) {
        success = ncm_mpd_client_commit_command_list(&global_mpd,
                                                      &ncm_error) == 0;
    }
    if (!success && global_mpd.command_list_active) {
        global_mpd.command_list_active = false;
    }
    if (!success) {
        return action_runtime_mpd_error_status(&ncm_error);
    }

    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Range reversed"));
    return 0;
}

static int32
action_runtime_shuffle_playlist(void) {
    NcMenu *menu;
    NcmError ncm_error;
    int32 first;
    int32 last;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    menu = action_runtime_current_menu();
    if (action_runtime_playlist_range(menu, &first, &last) < 0) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (Config.ask_before_shuffling_playlists && !action_runtime_confirm(
            STRLIT("Do you really want to shuffle selected range?"))) {
        return 0;
    }

    ncm_error_clear(&ncm_error);
    if (ncm_mpd_client_shuffle_range(&global_mpd, first, last,
                                      &ncm_error) < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Range shuffled"));
    return 0;
}

static int32
action_runtime_set_selected_items_priority(void) {
    StrBuilder input = {0};
    NcmError ncm_error;
    int32 priority;
    bool prompted;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (ncm_mpd_client_version(&global_mpd) < 17) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Priorities are supported in MPD >= "
                                    "0.17.0"));
        return -NCM_ERROR_UNAVAILABLE;
    }

    prompted = action_runtime_prompt_string(STRLIT("Set priority [0-255]: "),
                                            "", false, NULL, NULL, &input);
    if (!prompted) {
        sb_free(&input);
        return 0;
    }

    ncm_error_clear(&ncm_error);
    if (ncm_parse_int32(input.data, input.len, &priority, &ncm_error) < 0
        || (priority > 255)) {
        sb_free(&input);
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Priority must be between 0 and 255"));
        return 0;
    }
    sb_free(&input);

    ncm_error_clear(&ncm_error);
    if (playlist_screen_set_selected_priority(
        app_screen_playlist(), &global_mpd, priority, &ncm_error) < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Priority set"));
    return 0;
}

static int32
action_runtime_jump_to_position_in_song(void) {
    StrBuilder input = {0};
    NcmError ncm_error;
    int32 song_position;
    int32 total;
    int32 target;
    bool prompted;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (ncm_status_state_player() == NCM_STATUS_PLAYER_STOP) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    total = ncm_status_state_total_time();
    song_position = ncm_status_state_current_song_position();
    if ((total == 0) || (song_position < 0)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    prompted = action_runtime_prompt_string(
        STRLIT("Position to go (in %/h:m:ss/m:ss/seconds(s)): "), "",
        false, NULL, NULL, &input);
    if (!prompted) {
        sb_free(&input);
        return 0;
    }
    if (action_runtime_parse_seek_position(input.data, input.len, total,
                                           &target) < 0) {
        sb_free(&input);
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Invalid format ([h]:[mm]:[ss], [m]:[ss], "
                                    "[s]s, [%]%, [%] accepted)"));
        return 0;
    }
    sb_free(&input);

    ncm_error_clear(&ncm_error);
    if (ncm_mpd_client_seek_pos(&global_mpd, song_position, target,
                                 &ncm_error) < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return 0;
}

static int32
action_runtime_select_album(void) {
    StrBuilder album;
    StrBuilder candidate;
    NcMenu *menu;
    int32 current;
    int32 count;
    bool equal;

    if (((menu = action_runtime_current_tag_scroll_menu()) == NULL)
        || (nc_menu_item_count(menu) <= 0)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    current = nc_menu_highlight(menu);
    if (action_runtime_song_tag_at(current, SONG_GETTER_ALBUM,
                                    &album) < 0) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    for (int32 position = current; position >= 0; position -= 1) {
        if (action_runtime_song_tag_at(position, SONG_GETTER_ALBUM,
                                        &candidate) < 0) {
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
        if (action_runtime_song_tag_at(position, SONG_GETTER_ALBUM,
                                        &candidate) < 0) {
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

    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Album around cursor position selected"));
    return 0;
}

static int32
action_runtime_select_found_items(void) {
    StringView constraint;
    NcMenu *menu;
    NcmError ncm_error;
    int32 original;
    int32 height;
    int32 status;
    bool found;

    if (!current_screen_can_search()) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    constraint = current_screen_current_search_constraint();
    if ((constraint.data == NULL) || (constraint.len <= 0)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    if (((menu = action_runtime_current_menu()) == NULL)
        || (nc_menu_item_count(menu) <= 0)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    original = nc_menu_highlight(menu);
    height = action_runtime_current_menu_height();
    nc_menu_highlight_position(menu, 0, height);
    ncm_error_clear(&ncm_error);
    status = current_screen_search(NCM_SEARCH_DIRECTION_FORWARD,
                                   constraint.data, constraint.len, false,
                                   false, &ncm_error);
    if (status < 0) {
        nc_menu_highlight_position(menu, original, height);
        return action_runtime_mpd_error_status(&ncm_error);
    }
    found = status > 0;

    if (found) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Searching for items..."));
        (void)nc_menu_set_current_selected(menu, true);
        while (true) {
            status = current_screen_search(NCM_SEARCH_DIRECTION_FORWARD,
                                           constraint.data, constraint.len,
                                           false, true, &ncm_error);
            if (status < 0) {
                nc_menu_highlight_position(menu, original, height);
                return action_runtime_mpd_error_status(&ncm_error);
            }
            found = status > 0;
            if (!found) {
                break;
            }
            (void)nc_menu_set_current_selected(menu, true);
        }
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Found items selected"));
    }
    nc_menu_highlight_position(menu, original, height);
    nc_screen_finish_list_change(app_controller_current_screen());
    return 0;
}

static bool
action_runtime_previous_column_available(void) {
    return nc_screen_previous_column_available(app_controller_current_screen());
}

static bool
action_runtime_next_column_available(void) {
    return nc_screen_next_column_available(app_controller_current_screen());
}

static int32
action_runtime_previous_column(void) {
    return nc_screen_previous_column(app_controller_current_screen());
}

static int32
action_runtime_next_column(void) {
    return nc_screen_next_column(app_controller_current_screen());
}

static int32
action_runtime_enter_directory(void) {
    if (action_runtime_current_screen_is(SCREEN_TYPE_BROWSER)) {
        return browser_screen_enter_directory(app_screen_browser());
    }
#if defined(HAVE_TAGLIB_H)
    if (action_runtime_current_screen_is(SCREEN_TYPE_TAG_EDIT)) {
        return tag_edit_screen_enter_directory(app_screen_tag_edit());
    }
#endif
    return -NCM_ERROR_UNAVAILABLE;
}

static int32
action_runtime_jump_to_parent_directory(void) {
    if (action_runtime_current_screen_is(SCREEN_TYPE_BROWSER)) {
        return browser_screen_go_to_parent(app_screen_browser());
    }
#if defined(HAVE_TAGLIB_H)
    if (action_runtime_current_screen_is(SCREEN_TYPE_TAG_EDIT)) {
        return tag_edit_screen_go_to_parent(app_screen_tag_edit());
    }
#endif
    return -NCM_ERROR_UNAVAILABLE;
}

static int32
action_runtime_seek_relative(bool forward) {
    NcmError ncm_error;
    int32 position;
    int32 elapsed;
    int32 total;
    int32 target;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (ncm_status_state_player() == NCM_STATUS_PLAYER_STOP) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    position = ncm_status_state_current_song_position();
    if (position < 0) {
        return -NCM_ERROR_UNAVAILABLE;
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
    if (ncm_mpd_client_seek_pos(&global_mpd, position, target,
                                &ncm_error) < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    (void)ncm_status_update_full(&global_mpd, NULL, &ncm_error);
    return 0;
}

static int32
action_runtime_jump_to_browser(void) {
    NcmSong song;
    NcmError ncm_error;
    int32 status;

    song = (NcmSong){0};
    status = action_runtime_current_song(&song);
    if (status < 0) {
        ncm_song_destroy(&song);
        return status;
    }

    if (!action_runtime_current_screen_is(SCREEN_TYPE_BROWSER)) {
        status = action_runtime_switch_to_screen(SCREEN_TYPE_BROWSER);
        if (status < 0) {
            ncm_song_destroy(&song);
            return status;
        }
    }

    ncm_error_clear(&ncm_error);
    status = browser_screen_locate_song(app_screen_browser(), &song,
                                        &global_mpd, &ncm_error);
    ncm_song_destroy(&song);
    if (status < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    return 0;
}

static int32
action_runtime_jump_to_playing_song(void) {
    NcmSong song;
    NcmError ncm_error;
    int32 position = ncm_status_state_current_song_position();
    bool success;

    if (position < 0) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    if (action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST)) {
        success = playlist_screen_locate_position(app_screen_playlist(),
                                                  position) > 0;
        if (!success) {
            ncm_statusbar_print(Config.message_delay_time,
                                STRLIT("Song is filtered out"));
        }
        return 0;
    }
    if (action_runtime_current_screen_is(SCREEN_TYPE_MEDIA_LIBRARY)) {
        song = (NcmSong){0};
        ncm_error_clear(&ncm_error);
        success = ncm_mpd_client_get_current_song(&global_mpd, &song,
                                                  &ncm_error) == 0;
        if (success) {
            success = media_library_screen_locate_song(
                app_screen_media_library(), &song, &ncm_error) == 0;
        }
        ncm_song_destroy(&song);
        if (!success) {
            return action_runtime_mpd_error_status(&ncm_error);
        }
        return 0;
    }
    if (action_runtime_current_screen_is(SCREEN_TYPE_BROWSER)) {
        return action_runtime_jump_to_browser();
    }
    if (action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST_EDITOR)) {
        song = (NcmSong){0};
        ncm_error_clear(&ncm_error);
        success = ncm_mpd_client_get_current_song(&global_mpd, &song,
                                                  &ncm_error) == 0;
        if (success) {
            success = playlist_edit_screen_locate_song(
                app_screen_playlist_edit(), &global_mpd, &song,
                &ncm_error) == 0;
        }
        ncm_song_destroy(&song);
        if (!success && ncm_error_is_set(&ncm_error)) {
            return action_runtime_mpd_error_status(&ncm_error);
        }
        return 0;
    }
    return -NCM_ERROR_UNAVAILABLE;
}

static int32
action_runtime_jump_to_playlist_edit(void) {
    BrowserScreen *browser = app_screen_browser();
    StringView path;
    NcmError ncm_error;
    int32 status;

    if (!action_runtime_current_screen_is(SCREEN_TYPE_BROWSER)) {
        return action_runtime_switch_to_screen(SCREEN_TYPE_PLAYLIST_EDITOR);
    }

    if (!browser_screen_has_current_playlist_path(browser, &path)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    status = action_runtime_switch_to_screen(SCREEN_TYPE_PLAYLIST_EDITOR);
    if (status < 0) {
        return status;
    }

    ncm_error_clear(&ncm_error);
    status = playlist_edit_screen_locate_playlist(app_screen_playlist_edit(),
                                                  &global_mpd, path.data,
                                                  path.len, &ncm_error);
    if (status < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    return 0;
}

static int32
action_runtime_jump_to_media_library(void) {
    NcmSong song;
    NcmError ncm_error;
    int32 status;

    song = (NcmSong){0};
    status = action_runtime_current_song(&song);
    if (status < 0) {
        ncm_song_destroy(&song);
        return status;
    }

    status = action_runtime_switch_to_screen(SCREEN_TYPE_MEDIA_LIBRARY);
    if (status == 0) {
        ncm_statusbar_print(0,
                            STRLIT("Jumping to song..."));
        ncm_error_clear(&ncm_error);
        status = media_library_screen_locate_song(app_screen_media_library(),
                                                  &song, &ncm_error);
        if (status < 0) {
            status = action_runtime_mpd_error_status(&ncm_error);
        }
    }
    ncm_song_destroy(&song);
    return status;
}

static int32
action_runtime_jump_to_tag_edit(void) {
#if defined(HAVE_TAGLIB_H)
    StringView directory;
    NcmSong song;
    int32 status;

    if (Config.mpd_music_dir_len <= 0) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Proper mpd_music_dir variable has to be "
                                    "set in configuration file"));
        return -NCM_ERROR_UNAVAILABLE;
    }

    song = (NcmSong){0};
    status = action_runtime_current_song(&song);
    if ((status == 0) && (!ncm_song_has_directory_view(&song, 0, &directory)
            || (directory.len <= 0))) {
        status = -NCM_ERROR_UNAVAILABLE;
    }
    if (status == 0) {
        status = action_runtime_switch_to_screen(SCREEN_TYPE_TAG_EDIT);
    }
    if (status == 0) {
        status = tag_edit_screen_locate_song(app_screen_tag_edit(), &song);
    }
    ncm_song_destroy(&song);
    return status;
#else
    return -NCM_ERROR_UNAVAILABLE;
#endif
}

static int32
action_runtime_edit_directory_name(void) {
    StringView path;
    StrBuilder name = {0};
    NcmError ncm_error;
    bool prompted;
    bool success;

    if (action_runtime_current_screen_is(SCREEN_TYPE_BROWSER)) {
        BrowserScreen *browser = app_screen_browser();

        if (!browser_screen_has_current_directory_path(browser, &path)) {
            return -NCM_ERROR_UNAVAILABLE;
        }

        prompted = action_runtime_prompt_string(
            STRLIT("Directory: "), path.data, false, NULL, NULL, &name);
        if (!prompted) {
            sb_free(&name);
            return 0;
        }
        if ((name.len <= 0)
            || STREQUAL(name.data, name.len, path.data, path.len)) {
            sb_free(&name);
            return 0;
        }

        ncm_error_clear(&ncm_error);
        success = browser_screen_rename_current_directory(browser, name.data,
                                                          name.len, &global_mpd,
                                                          &ncm_error) == 0;
        if (success) {
            action_runtime_print_renamed(STRLIT("Directory renamed to \""),
                                         &name);
        }
        sb_free(&name);
        if (!success) {
            return action_runtime_mpd_error_status(&ncm_error);
        }
        return 0;
    }

#if defined(HAVE_TAGLIB_H)
    if (action_runtime_current_screen_is(SCREEN_TYPE_TAG_EDIT)) {
        return tag_edit_screen_rename_current_directory(
            app_screen_tag_edit(), Config.mpd_music_dir,
            Config.mpd_music_dir_len);
    }
#endif
    return -NCM_ERROR_UNAVAILABLE;
}

static int32
action_runtime_edit_playlist_name(void) {
    BrowserScreen *browser = app_screen_browser();
    PlaylistEditScreen *screen = app_screen_playlist_edit();
    NcmPlaylist playlist;
    StringView path;
    StrBuilder name = {0};
    NcmError ncm_error;
    bool prompted;
    bool success;

    if (action_runtime_current_screen_is(SCREEN_TYPE_BROWSER)) {
        if (!ncm_mpd_client_is_connected(&global_mpd)) {
            return -NCM_ERROR_UNAVAILABLE;
        }
        if (!browser_screen_has_current_playlist_path(browser, &path)) {
            return -NCM_ERROR_UNAVAILABLE;
        }

        prompted = action_runtime_prompt_string(STRLIT("Playlist: "), path.data,
                                                false, NULL, NULL, &name);
        if (!prompted) {
            sb_free(&name);
            return 0;
        }
        if ((name.len <= 0)
            || STREQUAL(name.data, name.len, path.data, path.len)) {
            sb_free(&name);
            return 0;
        }

        ncm_error_clear(&ncm_error);
        success = browser_screen_rename_current_playlist(browser, name.data,
                                                         name.len, &global_mpd,
                                                         &ncm_error) == 0;
        if (success) {
            action_runtime_print_renamed(STRLIT("Playlist renamed to \""),
                                         &name);
        }
        sb_free(&name);
        if (!success) {
            return action_runtime_mpd_error_status(&ncm_error);
        }
        return 0;
    }

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!action_runtime_playlist_edit_playlists_is_active()) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!action_runtime_playlist_edit_has_playlists()) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    playlist = (NcmPlaylist){0};
    success = playlist_edit_screen_current_playlist(screen, &playlist) > 0;
    if (!success) {
        ncm_playlist_destroy(&playlist);
        return -NCM_ERROR_UNAVAILABLE;
    }

    prompted = action_runtime_prompt_string(STRLIT("Playlist: "), playlist.path,
                                            false, NULL, NULL, &name);
    if (!prompted) {
        sb_free(&name);
        ncm_playlist_destroy(&playlist);
        return 0;
    }
    if ((name.len <= 0)
        || STREQUAL(name.data, name.len, playlist.path, playlist.path_len)) {
        sb_free(&name);
        ncm_playlist_destroy(&playlist);
        return 0;
    }

    ncm_error_clear(&ncm_error);
    success = ncm_mpd_client_rename_playlist(&global_mpd, playlist.path,
                                             name.data, &ncm_error) == 0;
    if (success) {
        action_runtime_print_renamed(STRLIT("Playlist renamed to \""), &name);
        playlist_edit_screen_request_playlists_update(screen);
    }
    sb_free(&name);
    ncm_playlist_destroy(&playlist);
    if (!success) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    return 0;
}

static int32
action_runtime_change_browse_mode(void) {
    BrowserScreen *browser = app_screen_browser();
    NcmError ncm_error;
    char *message;

    if (!action_runtime_current_screen_is(SCREEN_TYPE_BROWSER)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    ncm_error_clear(&ncm_error);
    if (browser_screen_change_browse_mode(browser, &global_mpd,
                                          &ncm_error) < 0) {
        if (ncm_error.code == EINVAL) {
            ncm_statusbar_print(Config.message_delay_time,
                                STRLIT("For browsing local filesystem "
                                        "connection to MPD via UNIX Socket "
                                        "is required"));
        } else if (ncm_error_is_set(&ncm_error)) {
            ncm_statusbar_print(Config.message_delay_time,
                                ncm_error.message, ncm_error.message_len);
        }
        return -NCM_ERROR_UNAVAILABLE;
    }

    if (browser_screen_is_local(browser)) {
        message = "Browse mode: local filesystem";
    } else {
        message = "Browse mode: MPD database";
    }
    ncm_statusbar_print(Config.message_delay_time,
                        message, strlen32(message));
    return 0;
}

static int32
action_runtime_toggle_browser_sort_mode(void) {
    char *message;

    if (!action_runtime_current_screen_is(SCREEN_TYPE_BROWSER)) {
        return -NCM_ERROR_UNAVAILABLE;
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
    ncm_statusbar_print(Config.message_delay_time,
                        message, strlen32(message));
    (void)browser_screen_sort(app_screen_browser());
    app_controller_request_current_screen_update();
    return 0;
}

static int32
action_runtime_toggle_library_tag_type(void) {
    MediaLibraryScreen *screen = app_screen_media_library();
    enum NcmTagType tag_type = NCM_TAG_ARTIST;
    enum MediaLibraryColumn column;

    if (!action_runtime_current_screen_is(SCREEN_TYPE_MEDIA_LIBRARY)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    column = media_library_screen_active_column(screen);
    if ((column != MEDIA_LIBRARY_COLUMN_TAGS)
        && ((media_library_screen_column_count(screen) != 2)
            || (column != MEDIA_LIBRARY_COLUMN_ALBUMS))) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    if (Config.media_library_primary_tag == NCM_TAG_ARTIST) {
        tag_type = NCM_TAG_ALBUM_ARTIST;
    } else if (Config.media_library_primary_tag == NCM_TAG_ALBUM_ARTIST) {
        tag_type = NCM_TAG_DATE;
    } else if (Config.media_library_primary_tag == NCM_TAG_DATE) {
        tag_type = NCM_TAG_GENRE;
    } else if (Config.media_library_primary_tag == NCM_TAG_GENRE) {
        tag_type = NCM_TAG_COMPOSER;
    } else if (Config.media_library_primary_tag == NCM_TAG_COMPOSER) {
        tag_type = NCM_TAG_PERFORMER;
    }

    return media_library_screen_set_primary_tag_type(screen, tag_type);
}

static int32
action_runtime_toggle_media_library_sort_mode(void) {
    int32 status;
    bool sort_by_mtime;

    if (!action_runtime_current_screen_is(SCREEN_TYPE_MEDIA_LIBRARY)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    status = media_library_screen_toggle_sort_mode(app_screen_media_library(),
                                                   &sort_by_mtime);
    if (status < 0) {
        return status;
    }
    if (sort_by_mtime) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Sorting library by: modification time"));
    } else {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Sorting library by: name"));
    }
    return 0;
}

static int32
action_runtime_toggle_media_library_columns(void) {
    MediaLibraryScreen *screen = app_screen_media_library();
    int32 status;

    if (!action_runtime_current_screen_is(SCREEN_TYPE_MEDIA_LIBRARY)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    status = media_library_screen_toggle_mode(screen, NULL);
    if (status < 0) {
        return status;
    }
    app_controller_request_current_screen_resize();
    return 0;
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

static int32
action_runtime_toggle_replay_gain_mode(void) {
    NcmError ncm_error;
    NcWindow *window;
    char choice;
    enum NcmMpdReplayGainMode mode;
    int32 status;

    if (!ncm_mpd_client_is_connected(&global_mpd)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    if ((window = app_controller_active_window()) == NULL) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    ncm_statusbar_print(0,
                        STRLIT("Replay gain: off [o], track [t], album [a]"));
    choice = 'o';
    status = ncm_statusbar_prompt_return_one_of(window, "ota", 3, &choice);
    if (status < 0) {
        return status;
    }
    if (status == 0) {
        return 0;
    }

    mode = NCM_MPD_REPLAY_GAIN_OFF;
    if (choice == 't') {
        mode = NCM_MPD_REPLAY_GAIN_TRACK;
    } else if (choice == 'a') {
        mode = NCM_MPD_REPLAY_GAIN_ALBUM;
    }

    ncm_error_clear(&ncm_error);
    if (ncm_mpd_client_set_replay_gain_mode(&global_mpd, mode,
                                            &ncm_error) < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    ncm_error_clear(&ncm_error);
    if (ncm_mpd_client_get_replay_gain_mode(&global_mpd, &mode,
                                            &ncm_error) < 0) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    action_runtime_print_toggle(STRLIT("Replay gain mode: "),
                                action_runtime_replay_gain_mode_name(mode));
    return 0;
}

static int32
action_runtime_save_tag_changes(void) {
#if defined(HAVE_TAGLIB_H)
    if (action_runtime_current_screen_is(SCREEN_TYPE_TAG_EDIT)) {
        if (!tag_edit_screen_save_action_available(app_screen_tag_edit())) {
            return -NCM_ERROR_UNAVAILABLE;
        }
        return tag_edit_screen_save_modified(app_screen_tag_edit(),
                                             Config.mpd_music_dir);
    }
    if (action_runtime_current_screen_is(SCREEN_TYPE_TINY_TAG_EDIT)) {
        return tiny_tag_edit_screen_run_row(app_screen_tiny_tag_edit(),
                                            TINY_TAG_EDIT_SAVE_ROW);
    }
#endif
    return -NCM_ERROR_UNAVAILABLE;
}

int32
ncm_action_edit_song(NcmSong *song) {
#if defined(HAVE_TAGLIB_H)
    enum TinyTagEditOpenResult open_result;
    StrBuilder path = {0};
    int32 path_len;
    int32 path_width;
    int32 status = -NCM_ERROR_UNAVAILABLE;

    if (song == NULL) {
        return -EINVAL;
    }
    if (Config.mpd_music_dir_len <= 0) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Proper mpd_music_dir variable has to be "
                                    "set in configuration file"));
        return -NCM_ERROR_UNAVAILABLE;
    }

    open_result = tiny_tag_edit_screen_open_song(app_screen_tiny_tag_edit(),
                                                 song, Config.mpd_music_dir,
                                                 Config.mpd_music_dir_len,
                                                 Config.tags_separator,
                                                 Config.tags_separator_len,
                                                 Config.show_duplicate_tags,
                                                 &path);
    switch (open_result) {
    case TINY_TAG_EDIT_OPEN_SUCCESS:
        status = action_runtime_switch_to_screen(SCREEN_TYPE_TINY_TAG_EDIT);
        break;
    case TINY_TAG_EDIT_OPEN_STREAM:
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Streams can't be edited"));
        break;
    case TINY_TAG_EDIT_OPEN_MISSING_MUSIC_DIRECTORY:
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Proper mpd_music_dir variable has to be "
                                    "set in configuration file"));
        break;
    case TINY_TAG_EDIT_OPEN_UNREADABLE_FILE:
        path_width = COLS - STRLIT_LEN("Couldn't read file \"\"");
        if (path_width < 0) {
            path_width = 0;
        }
        path_len = utf8_cut_width(path.data, path.len, path_width);
        action_runtime_print_message(STRLIT("Couldn't read file \""),
                                     path.data, path_len, STRLIT("\""));
        break;
    case TINY_TAG_EDIT_OPEN_INVALID_ARGUMENT:
    case TINY_TAG_EDIT_OPEN_PREPARE_FAILED:
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Couldn't prepare tiny tag editor"));
        break;
    case TINY_TAG_EDIT_OPEN_COUNT:
    default:
        break;
    }
    sb_free(&path);
    return status;
#else
    (void)song;
    return -NCM_ERROR_UNAVAILABLE;
#endif
}

static bool
action_runtime_media_library_current_artist_tag(char **artist,
                                                int32 *artist_len) {
    MediaLibraryScreen *library = app_screen_media_library();
    char *value;
    int32 value_len;

    if (!action_runtime_current_screen_is(SCREEN_TYPE_MEDIA_LIBRARY)) {
        return false;
    }
    if (Config.media_library_primary_tag != NCM_TAG_ARTIST) {
        return false;
    }

    if (media_library_screen_active_column(library)
        != MEDIA_LIBRARY_COLUMN_TAGS) {
        return false;
    }
    if (!media_library_screen_has_current_primary_tag_value(library, &value,
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

static int32
action_runtime_toggle_screen_lock(void) {
    StrBuilder input = {0};
    StrBuilder message = {0};
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
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Screen unlocked"));
        return 0;
    }

    if (((current = app_controller_current_screen()) == NULL)
        || !nc_screen_is_lockable(current)) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Current screen can't be locked"));
        return 0;
    }

    if (Config.ask_for_locked_screen_width_part) {
        SNPRINTF(initial, "%d", part);
        prompted = action_runtime_prompt_string(
            STRLIT("% of the locked screen's width to be reserved "
                        "(20-80): "), initial, true, NULL, NULL, &input);
        if (!prompted) {
            sb_free(&input);
            ncm_statusbar_print(Config.message_delay_time,
                                STRLIT("Action aborted"));
            return 0;
        }

        ncm_error_clear(&ncm_error);
        if (ncm_parse_int32(input.data, input.len, &part, &ncm_error) < 0) {
            action_runtime_print_message(STRLIT("Invalid value: "),
                                         input.data, input.len, STRLIT(""));
            sb_free(&input);
            return 0;
        }
        sb_free(&input);
    }

    if ((part < 20) || (part > 80)) {
        sb_printf(&message, "Error: value is out of bounds "
                  "([20, 80] expected, %d given)", part);
        ncm_statusbar_print(Config.message_delay_time,
                                    message.data, message.len);
        sb_free(&message);
        return 0;
    }

    Config.locked_screen_width_part = part / 100.0;
    if (app_controller_lock_current_screen() == 0) {
        sb_printf(&message, "Screen locked (with %d%% width)", part);
        ncm_statusbar_print(Config.message_delay_time,
                                    message.data, message.len);
        sb_free(&message);
    } else {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Current screen can't be locked"));
    }
    return 0;
}

#if defined(HAVE_TAGLIB_H)
static bool
action_runtime_mpd_music_dir_is_set(void) {
    if (Config.mpd_music_dir_len > 0) {
        return true;
    }

    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Proper mpd_music_dir variable"
                                " has to be set in configuration file"));
    return false;
}

static bool
action_runtime_media_library_current_tag(char **tag, int32 *tag_len) {
    MediaLibraryScreen *library = app_screen_media_library();

    if (!action_runtime_current_screen_is(SCREEN_TYPE_MEDIA_LIBRARY)) {
        return false;
    }

    if (media_library_screen_active_column(library)
        != MEDIA_LIBRARY_COLUMN_TAGS) {
        return false;
    }
    return media_library_screen_has_current_primary_tag_value(library, tag,
                                                              tag_len);
}

static bool
action_runtime_media_library_current_album(char **album, int32 *album_len) {
    MediaLibraryScreen *library = app_screen_media_library();

    if (!action_runtime_current_screen_is(SCREEN_TYPE_MEDIA_LIBRARY)) {
        return false;
    }

    if (media_library_screen_active_column(library)
        != MEDIA_LIBRARY_COLUMN_ALBUMS) {
        return false;
    }
    return media_library_screen_has_current_album_value(library, album,
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
action_runtime_song_uri_view(NcmSong *song, StringView *uri) {
    *uri = (StringView){0};
    return ncm_song_has_uri_view(song, 0, uri);
}

static bool
action_runtime_song_name_or_uri_view(NcmSong *song, StringView *view) {
    *view = (StringView){0};
    if (ncm_song_has_name_view(song, 0, view)) {
        return true;
    }
    return action_runtime_song_uri_view(song, view);
}

static int32
action_runtime_shared_directory_update(StrBuilder *shared_directory,
                                       bool *valid, char *directory,
                                       int32 directory_len) {
    StrBuilder shared;

    ASSERT(shared_directory != NULL);
    ASSERT(valid != NULL);

    if (!*valid) {
        *valid = true;
        if (sb_set(shared_directory, directory, directory_len) < 0) {
            return -EINVAL;
        }
        return 0;
    }

    shared = ncm_string_shared_directory(shared_directory->data,
                                         shared_directory->len, directory,
                                         directory_len);
    sb_free(shared_directory);
    *shared_directory = shared;
    return 0;
}

static void
action_runtime_print_updating_song(NcmSong *song) {
    StringView name;
    StrBuilder message = {0};

    if (!action_runtime_song_name_or_uri_view(song, &name)) {
        return;
    }

    SB_APPEND(&message, "Updating tags in \"");
    SB_APPEND(&message, name.data, name.len);
    SB_APPEND(&message, "\"...");
    ncm_statusbar_print(0, message.data, message.len);
    sb_free(&message);
    return;
}

static void
action_runtime_print_album_file_error(char *prefix, int32 prefix_len,
                                      NcmSong *song) {
    StringView uri;
    int32 width;
    int32 uri_len;

    if (!action_runtime_song_uri_view(song, &uri)) {
        return;
    }

    width = COLS - prefix_len - STRLIT_LEN("\"");
    if (width < 0) {
        width = 0;
    }
    uri_len = utf8_cut_width(uri.data, uri.len, width);
    action_runtime_print_message(prefix, prefix_len, uri.data, uri_len,
                                 STRLIT("\""));
    return;
}

static int32
action_runtime_update_tag_directory(StrBuilder *shared_directory, bool valid) {
    NcmError ncm_error;

    if (!valid) {
        return 0;
    }

    {
        char *directory = shared_directory->data;

        if (directory == NULL) {
            directory = "";
        }

        ncm_error_clear(&ncm_error);
        if (ncm_mpd_client_update_directory(&global_mpd, directory, NULL,
                                             &ncm_error) < 0) {
            return action_runtime_mpd_error_status(&ncm_error);
        }
    }
    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Tags updated successfully"));
    return 0;
}

static int32
action_runtime_edit_library_tag(void) {
    enum TagsField field;
    NcmMpdSongList songs = {0};
    StrBuilder current_tag = {0};
    StrBuilder prompt = {0};
    StrBuilder new_tag = {0};
    StrBuilder shared_directory = {0};
    NcmError ncm_error;
    char *tag;
    int32 tag_len;
    int32 status = -NCM_ERROR_UNAVAILABLE;
    bool shared_directory_valid = false;
    bool prompted;

    if (!action_runtime_mpd_music_dir_is_set()) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!action_runtime_media_library_current_tag(&tag, &tag_len)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    status = sb_set(&current_tag, tag, tag_len);
    if (status < 0) {
        goto cleanup;
    }
    SB_APPEND(&prompt, ncm_tag_type_name(Config.media_library_primary_tag),
        optional_strlen32(ncm_tag_type_name(Config.media_library_primary_tag)));
    SB_APPEND(&prompt, ": ");
    prompted = action_runtime_prompt_string(prompt.data, prompt.len,
                                            current_tag.data, false, NULL, NULL,
                                            &new_tag);
    if (!prompted) {
        status = 0;
        goto cleanup;
    }
    if ((new_tag.len <= 0)
        || STREQUAL(new_tag.data, new_tag.len, current_tag.data,
                    current_tag.len)) {
        status = 0;
        goto cleanup;
    }

    field = ncm_tags_field_from_tag_type(Config.media_library_primary_tag);
    if (field == NCM_TAGS_FIELD_COUNT) {
        status = -NCM_ERROR_UNAVAILABLE;
        goto cleanup;
    }

    ncm_statusbar_print(0,
                        STRLIT("Updating tags..."));
    ncm_error_clear(&ncm_error);
    if (ncm_mpd_client_start_search(&global_mpd, true, &ncm_error) < 0
        || ncm_mpd_client_add_search_tag(&global_mpd,
                                         Config.media_library_primary_tag,
                                         current_tag.data, &ncm_error) < 0
        || ncm_mpd_client_commit_search_songs(&global_mpd, &songs,
                                               &ncm_error) < 0) {
        status = action_runtime_mpd_error_status(&ncm_error);
        goto cleanup;
    }

    status = 0;
    for (int32 i = 0; (status == 0) && (i < ncm_mpd_song_list_count(&songs));
         i += 1) {
        NcmSong *song = ncm_mpd_song_list_at(&songs, i);
        MutableSong mutable_song = {0};
        StringView uri;

        status = mutable_song_load_originals_from_song(&mutable_song, song);
        if (status == 0) {
            status = mutable_song_set_tags(&mutable_song, field,
                                               new_tag.data, new_tag.len,
                                               Config.tags_separator,
                                               Config.tags_separator_len);
        }
        if (status < 0) {
            mutable_song_destroy(&mutable_song);
            break;
        }

        action_runtime_print_updating_song(song);
        status = mutable_song_write(&mutable_song, Config.mpd_music_dir);
        if (status < 0) {
            StringView name;

            if (action_runtime_song_name_or_uri_view(song, &name)) {
                StrBuilder message = {0};
                char *error_message = strerror(errno);

                SB_APPEND(&message, "Error while writing tags to \"");
                SB_APPEND(&message, name.data, name.len);
                SB_APPEND(&message, "\": ");
                SB_APPEND(&message, error_message,
                          optional_strlen32(error_message));
                ncm_statusbar_print(Config.message_delay_time,
                                                    message.data, message.len);
                sb_free(&message);
            }
            mutable_song_destroy(&mutable_song);
            break;
        }

        if (action_runtime_song_uri_view(song, &uri)) {
            status = action_runtime_shared_directory_update(
                &shared_directory, &shared_directory_valid, uri.data, uri.len);
        }
        mutable_song_destroy(&mutable_song);
    }

    if (status == 0) {
        status = action_runtime_update_tag_directory(&shared_directory,
                                                     shared_directory_valid);
    }

cleanup:
    ncm_mpd_song_list_destroy(&songs);
    sb_free(&shared_directory);
    sb_free(&new_tag);
    sb_free(&prompt);
    sb_free(&current_tag);
    return status;
}


static int32
action_runtime_edit_library_album(void) {
    NcmSongArray songs = {0};
    StrBuilder current_album = {0};
    StrBuilder new_album = {0};
    StrBuilder path = {0};
    StrBuilder shared_directory = {0};
    NcmError ncm_error;
    char *album;
    int32 album_len;
    int32 status = -NCM_ERROR_UNAVAILABLE;
    bool shared_directory_valid = false;
    bool prompted;

    if (!action_runtime_mpd_music_dir_is_set()) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!action_runtime_media_library_current_album(&album, &album_len)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    ncm_error_clear(&ncm_error);

    status = sb_set(&current_album, album, album_len);
    if (status < 0) {
        goto cleanup;
    }
    prompted = action_runtime_prompt_string(STRLIT("Album: "),
                                            current_album.data, false, NULL,
                                            NULL, &new_album);
    if (!prompted) {
        status = 0;
        goto cleanup;
    }
    if ((new_album.len <= 0)
        || STREQUAL(new_album.data, new_album.len,
                    current_album.data, current_album.len)) {
        status = 0;
        goto cleanup;
    }

    status = media_library_screen_copy_visible_songs(app_screen_media_library(),
                                                     &songs, &ncm_error);
    if (status < 0) {
        if (ncm_error_is_set(&ncm_error)) {
            ncm_statusbar_print(Config.message_delay_time,
                                ncm_error.message, ncm_error.message_len);
        }
        goto cleanup;
    }

    ncm_statusbar_print(0,
                        STRLIT("Updating tags..."));
    for (int32 i = 0; (status == 0) && (i < songs.len); i += 1) {
        NcmSong *song = &songs.items[i];
        StringView directory;
        StringView uri;
        NcmTaglibFile file = {0};

        action_runtime_print_updating_song(song);
        sb_clear(&path);
        if (!action_runtime_song_uri_view(song, &uri)) {
            status = -NCM_ERROR_UNAVAILABLE;
            break;
        }
        SB_APPEND(&path, Config.mpd_music_dir, Config.mpd_music_dir_len);
        SB_APPEND(&path, uri.data, uri.len);
        if (ncm_song_has_directory_view(song, 0, &directory)) {
            status = action_runtime_shared_directory_update(
                &shared_directory, &shared_directory_valid, directory.data,
                directory.len);
            if (status < 0) {
                break;
            }
        }

        status = ncm_taglib_file_open(&file, path.data);
        if (status < 0) {
            action_runtime_print_album_file_error(
                STRLIT("Error while opening file \""), song);
            break;
        }
        status = ncm_taglib_clear_property(&file, "ALBUM");
        if (status == 0) {
            status = ncm_taglib_append_property(&file, "ALBUM", new_album.data);
        }
        if (status == 0) {
            status = ncm_taglib_file_save(&file);
        }
        ncm_taglib_file_close(&file);
        if (status < 0) {
            action_runtime_print_album_file_error(
                STRLIT("Error while writing tags in \""), song);
            break;
        }
    }

    if (status == 0) {
        status = action_runtime_update_tag_directory(&shared_directory,
                                                     shared_directory_valid);
    }

cleanup:
    sb_free(&shared_directory);
    sb_free(&path);
    sb_free(&new_album);
    sb_free(&current_album);
    ncm_song_array_destroy(&songs);
    return status;
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

static int32
action_runtime_edit_library_tag(void) {
    return -NCM_ERROR_UNAVAILABLE;
}

static int32
action_runtime_edit_library_album(void) {
    return -NCM_ERROR_UNAVAILABLE;
}
#endif

static int32
action_runtime_edit_current_song(void) {
#if defined(HAVE_TAGLIB_H)
    NcmSong song = {0};
    int32 status;

    if (action_runtime_current_screen_is(SCREEN_TYPE_LYRICS)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!action_runtime_has_current_song()) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    status = action_runtime_current_song(&song);
    if (status == 0) {
        status = ncm_action_edit_song(&song);
    }
    ncm_song_destroy(&song);
    return status;
#else
    return -NCM_ERROR_UNAVAILABLE;
#endif
}

static int32
action_runtime_toggle_lyrics_fetcher(void) {
    LyricsFetcherDef *fetcher;

    fetcher = lyrics_screen_toggle_fetcher(app_screen_lyrics(),
                                           &Config.lyrics_fetchers);
    if (fetcher == NULL) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Using all lyrics fetchers"));
        return 0;
    }

    action_runtime_print_message(STRLIT("Using lyrics fetcher: "),
                                 ncm_lyrics_fetcher_name(fetcher),
                                 ncm_lyrics_fetcher_name_len(fetcher),
                                 STRLIT(""));
    return 0;
}

static int32
action_runtime_edit_lyrics(void) {
    LyricsScreen *lyrics = app_screen_lyrics();
    NcmSong *song;
    StrBuilder *filename;
    StrBuilder escaped = {0};
    StrBuilder command = {0};
    NcmError ncm_error;
    int32 status;
    bool success;

    if (Config.external_editor_len <= 0) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("external_editor variable has to be set in "
                                    "configuration file"));
        return -NCM_ERROR_UNAVAILABLE;
    }

    if ((song = lyrics_screen_song(lyrics)) == NULL) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    status = lyrics_screen_build_filename(
        lyrics, song, Config.mpd_music_dir, Config.mpd_music_dir_len,
        Config.lyrics_directory, Config.lyrics_directory_len,
        Config.store_lyrics_in_song_dir,
        Config.generate_win32_compatible_filenames);
    if (status < 0) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("failed to build lyrics "
                                    "filename"));
        return -NCM_ERROR_UNAVAILABLE;
    }

    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Opening lyrics in external "
                                "editor..."));
    filename = lyrics_screen_filename(lyrics);
    ncm_string_append_shell_escaped_single_quotes(&escaped, filename->data,
                                                  filename->len);
    SB_APPEND(&command, Config.external_editor, Config.external_editor_len);
    SB_APPEND(&command, " '");
    SB_APPEND(&command, escaped.data, escaped.len);
    sb_append_byte(&command, '\'');

    ncm_error_clear(&ncm_error);
    if (Config.use_console_editor) {
        nc_pause_screen();
        success = ncm_run_external_console_command(command.data, command.len,
                                                         &ncm_error) == 0;
        nc_unpause_screen();
        if (!success) {
            sb_free(&command);
            sb_free(&escaped);
            return action_runtime_mpd_error_status(&ncm_error);
        }

        status = lyrics_screen_load_file(lyrics, filename->data,
                                         filename->len, &ncm_error);
        if (status < 0) {
            lyrics->has_song = false;
            status = lyrics_screen_fetch(lyrics, song, NULL, &ncm_error);
            success = status >= 0;
        }
    } else {
        success = ncm_run_external_command(command.data, command.len,
                                                 false, &ncm_error) == 0;
    }

    sb_free(&command);
    sb_free(&escaped);
    if (!success) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    ncm_error_clear(&ncm_error);
    return 0;
}

static int32
action_runtime_fetch_lyrics_background(void) {
    NcmSongArray songs = {0};
    NcmError ncm_error;
    int32 status;

    if ((action_runtime_selected_songs(&songs) < 0) || (songs.len <= 0)) {
        ncm_song_array_destroy(&songs);
        return -NCM_ERROR_UNAVAILABLE;
    }

    ncm_error_clear(&ncm_error);
    for (int32 i = 0; i < songs.len; i += 1) {
        status = lyrics_screen_fetch_in_background(app_screen_lyrics(),
                                                   &songs.items[i], true,
                                                   &ncm_error);
        if (status < 0) {
            ncm_song_array_destroy(&songs);
            return action_runtime_mpd_error_status(&ncm_error);
        }
    }

    ncm_song_array_destroy(&songs);
    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Selected songs queued for lyrics fetching"));
    return 0;
}

static int32
action_runtime_refetch_lyrics(void) {
    NcmError ncm_error;

    ncm_error_clear(&ncm_error);
    lyrics_screen_refetch_current(app_screen_lyrics(), &ncm_error);
    if (ncm_error_is_set(&ncm_error)) {
        return action_runtime_mpd_error_status(&ncm_error);
    }
    return 0;
}

static int32
action_runtime_show_lyrics(void) {
    NcmSong song = {0};
    NcmSong *lyrics_song;
    NcmError ncm_error;
    int32 status;

    if (action_runtime_current_screen_is(SCREEN_TYPE_LYRICS)) {
        return action_runtime_switch_to_screen(SCREEN_TYPE_LYRICS);
    }

    status = action_runtime_current_song(&song);
    if (status == 0) {
        if (((lyrics_song = lyrics_screen_song(app_screen_lyrics())) == NULL)
            || !ncm_song_is_equal(lyrics_song, &song)) {
            ncm_error_clear(&ncm_error);
            status = lyrics_screen_fetch(app_screen_lyrics(), &song, NULL,
                                         &ncm_error);
            if (status < 0) {
                status = action_runtime_mpd_error_status(&ncm_error);
            }
        }
    }
    ncm_song_destroy(&song);
    if (status < 0) {
        return status;
    }
    return action_runtime_switch_to_screen(SCREEN_TYPE_LYRICS);
}

static int32
action_runtime_show_artist_info(void) {
    NcmSong song = {0};
    StringView artist = {0};
    NcmError ncm_error;
    char *media_library_artist = NULL;
    int32 media_library_artist_len = 0;
    bool has_artist = false;
    int32 status;

    if (action_runtime_current_screen_is(SCREEN_TYPE_LASTFM)) {
        return action_runtime_switch_to_screen(SCREEN_TYPE_LASTFM);
    }

    if (action_runtime_media_library_current_artist_tag(
        &media_library_artist, &media_library_artist_len)) {
        artist.data = media_library_artist;
        artist.len = media_library_artist_len;
        has_artist = true;
    }

    if (!has_artist) {
        if (action_runtime_current_song(&song) < 0) {
            ncm_song_destroy(&song);
            return -NCM_ERROR_UNAVAILABLE;
        }
        has_artist = ncm_song_has_tag_view(&song, NCM_TAG_ARTIST, 0, &artist);
    }

    if (has_artist && (artist.len > 0)) {
        ncm_error_clear(&ncm_error);
        status = lastfm_screen_queue_artist_info(
            app_screen_lastfm(), artist.data, artist.len,
            Config.lastfm_preferred_language,
            Config.lastfm_preferred_language_len, &ncm_error);
        ncm_song_destroy(&song);
        if (status < 0) {
            return action_runtime_mpd_error_status(&ncm_error);
        }
        if (!app_controller_is_screen_visible(app_screen_lastfm_base())) {
            return action_runtime_switch_to_screen(SCREEN_TYPE_LASTFM);
        }
        return 0;
    }

    ncm_song_destroy(&song);
    return 0;
}

static int32
action_runtime_mouse_event(void) {
    NcmError ncm_error;
    NcWindow *window;
    MEVENT *event;
    int32 position;
    int32 progressbar_y;
    int32 player_state_y;
    int32 seconds;

    if (!Config.mouse_support) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    if ((window = ui_state_footer_window()) == NULL) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if ((event = nc_window_mouse_event(window)) == NULL) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    progressbar_y = LINES - 1;
    if (Config.statusbar_visibility) {
        progressbar_y -= 1;
    }
    player_state_y = LINES - 1;
    if (Config.user_interface == NCM_DESIGN_ALTERNATIVE) {
        player_state_y = 1;
    }

    if ((event->bstate & BUTTON1_PRESSED) && (event->y == progressbar_y)) {
        if (ncm_status_state_player() == NCM_STATUS_PLAYER_STOP) {
            return 0;
        }
        position = ncm_status_state_current_song_position();
        seconds = (ncm_status_state_total_time()*event->x / COLS);
        ncm_error_clear(&ncm_error);
        if (ncm_mpd_client_seek_pos(&global_mpd, position, seconds,
                                     &ncm_error) < 0) {
            return action_runtime_mpd_error_status(&ncm_error);
        }
    } else if ((event->bstate & BUTTON1_PRESSED)
               && (Config.statusbar_visibility
                   || (Config.user_interface == NCM_DESIGN_ALTERNATIVE))
               && (ncm_status_state_player() != NCM_STATUS_PLAYER_STOP)
               && (event->y == player_state_y) && (event->x < 9)) {
        ncm_error_clear(&ncm_error);
        if (ncm_mpd_client_toggle_pause(&global_mpd, &ncm_error) < 0) {
            return action_runtime_mpd_error_status(&ncm_error);
        }
    } else if (((event->bstate & BUTTON5_PRESSED)
                || (event->bstate & BUTTON4_PRESSED))
               && (Config.header_visibility
                   || (Config.user_interface == NCM_DESIGN_ALTERNATIVE))
               && (event->y == 0)
               && (event->x > COLS - global_volume_state_len())) {
        if (event->bstate & BUTTON5_PRESSED) {
            return ncm_action_runtime_run(NULL, ACTION_VOLUME_DOWN);
        }
        return ncm_action_runtime_run(NULL, ACTION_VOLUME_UP);
    } else if (event->bstate
               & (BUTTON1_PRESSED | BUTTON3_PRESSED | BUTTON4_PRESSED
                  | BUTTON5_PRESSED)) {
        app_controller_mouse_button_pressed_current(*event);
    }
    return 0;
}

#define ACTION_REQ_FIELDS(XX)                                               \
    XX(ACTION_REQ_MPD_CONNECTED)                                           \
    XX(ACTION_REQ_VOLUME)                                                  \
    XX(ACTION_REQ_PLAYER_NOT_STOPPED)                                      \
    XX(ACTION_REQ_TOTAL_TIME)                                              \
    XX(ACTION_REQ_CURRENT_SONG_POSITION)                                   \
    XX(ACTION_REQ_CURRENT_SONG)                                            \
    XX(ACTION_REQ_SELECTED_SONGS)                                          \
    XX(ACTION_REQ_CURRENT_MENU)                                            \
    XX(ACTION_REQ_MENU_SELECTABLE_ITEM)                                    \
    XX(ACTION_REQ_MENU_SELECTION)                                          \
    XX(ACTION_REQ_FILTER)                                                  \
    XX(ACTION_REQ_FIND)                                                    \
    XX(ACTION_REQ_SEARCH)                                                  \
    XX(ACTION_REQ_PREVIOUS_COLUMN)                                         \
    XX(ACTION_REQ_NEXT_COLUMN)                                             \
    XX(ACTION_REQ_DISPLAY_MODE)                                            \
    XX(ACTION_REQ_CAN_SHOW_LOCKED_SCREEN)                                  \
    XX(ACTION_REQ_CAN_SHOW_INACTIVE_SCREEN)                                \
    XX(ACTION_REQ_TAGLIB)                                                  \
    XX(ACTION_REQ_MPD_MUSIC_DIR)                                           \
    XX(ACTION_REQ_NOT_TINY_TAG_EDIT)

#define ENUM_NAME ActionRequirement
#define ENUM_PREFIX_ ACTION_REQ_
#define ENUM_BITFLAGS 1
#define ENUM_FIELDS ACTION_REQ_FIELDS(XX)
#include "cbase/xenums.c"
enum ActionScreenRequirement {
    ACTION_SCREEN_IS_TARGET = 1,
    ACTION_SCREEN_IS_NOT_TARGET,
};

typedef struct ActionAvailability {
    enum ActionRequirement requirements;
    enum ActionScreenRequirement screen_requirement;
    enum ScreenType screen_type;
    int32 argument;
    bool custom;
} ActionAvailability;

#define AR0 ACTION_REQ_NONE
#define AR_MPD ACTION_REQ_MPD_CONNECTED
#define AR_VOL ACTION_REQ_VOLUME
#define AR_ACTIVE ACTION_REQ_PLAYER_NOT_STOPPED
#define AR_TOTAL ACTION_REQ_TOTAL_TIME
#define AR_POS ACTION_REQ_CURRENT_SONG_POSITION
#define AR_SONG ACTION_REQ_CURRENT_SONG
#define AR_SONGS ACTION_REQ_SELECTED_SONGS
#define AR_MENU ACTION_REQ_CURRENT_MENU
#define AR_SELECT ACTION_REQ_MENU_SELECTABLE_ITEM
#define AR_SELECTION ACTION_REQ_MENU_SELECTION
#define AR_FILTER ACTION_REQ_FILTER
#define AR_FIND ACTION_REQ_FIND
#define AR_SEARCH ACTION_REQ_SEARCH
#define AR_PREV_COL ACTION_REQ_PREVIOUS_COLUMN
#define AR_NEXT_COL ACTION_REQ_NEXT_COLUMN
#define AR_DISPLAY ACTION_REQ_DISPLAY_MODE
#define AR_LOCK ACTION_REQ_CAN_SHOW_LOCKED_SCREEN
#define AR_INACTIVE ACTION_REQ_CAN_SHOW_INACTIVE_SCREEN
#define AR_TAGLIB ACTION_REQ_TAGLIB
#define AR_MUSIC_DIR ACTION_REQ_MPD_MUSIC_DIR
#define AR_NOT_TINY ACTION_REQ_NOT_TINY_TAG_EDIT

#define A(reqs) {.requirements = (reqs)}
#define AS(reqs, screen_req, target) \
    {.requirements = (reqs), \
     .screen_requirement = (screen_req), \
     .screen_type = (target)}
#define C(reqs) {.requirements = (reqs), .custom = true}
#define CA(reqs, value) \
    {.requirements = (reqs), .argument = (value), .custom = true}
#define SC(reqs, screen_req, target) \
    {.requirements = (reqs), \
     .screen_requirement = (screen_req), \
     .screen_type = (target), \
     .custom = true}

#define ACTION_MPD_AVAILABILITY(XX)                                         \
    XX(ACTION_PREVIOUS)                                                     \
    XX(ACTION_NEXT)                                                         \
    XX(ACTION_STOP)                                                         \
    XX(ACTION_PLAY)                                                         \
    XX(ACTION_SAVE_PLAYLIST)                                                \
    XX(ACTION_UPDATE_DATABASE)                                              \
    XX(ACTION_TOGGLE_REPEAT)                                                \
    XX(ACTION_TOGGLE_RANDOM)                                                \
    XX(ACTION_TOGGLE_SINGLE)                                                \
    XX(ACTION_TOGGLE_CONSUME)                                               \
    XX(ACTION_TOGGLE_CROSSFADE)                                             \
    XX(ACTION_SET_CROSSFADE)                                                \
    XX(ACTION_CLEAR_MAIN_PLAYLIST)                                          \
    XX(ACTION_TOGGLE_REPLAY_GAIN_MODE)                                      \
    XX(ACTION_ADD_RANDOM_ITEMS)

#define ACTION_SEARCH_AVAILABILITY(XX)                                      \
    XX(ACTION_FIND_ITEM_FORWARD)                                            \
    XX(ACTION_FIND_ITEM_BACKWARD)                                           \
    XX(ACTION_NEXT_FOUND_ITEM)                                              \
    XX(ACTION_PREVIOUS_FOUND_ITEM)

#define ACTION_MPD_AVAILABLE(action) [action] = A(AR_MPD),
#define ACTION_SEARCH_AVAILABLE(action) [action] = A(AR_SEARCH),

static const ActionAvailability action_availability_table[ACTION_COUNT] = {
    ACTION_MPD_AVAILABILITY(ACTION_MPD_AVAILABLE)
    ACTION_SEARCH_AVAILABILITY(ACTION_SEARCH_AVAILABLE)

    [ACTION_MOUSE_EVENT] = C(AR0),
    [ACTION_SCROLL_UP_ARTIST] = CA(AR0, SONG_GETTER_ARTIST),
    [ACTION_SCROLL_UP_ALBUM] = CA(AR0, SONG_GETTER_ALBUM),
    [ACTION_SCROLL_DOWN_ARTIST] = CA(AR0, SONG_GETTER_ARTIST),
    [ACTION_SCROLL_DOWN_ALBUM] = CA(AR0, SONG_GETTER_ALBUM),
    [ACTION_JUMP_TO_PARENT_DIRECTORY] = C(AR0),
    [ACTION_RUN_ACTION] = C(AR0),
    [ACTION_PREVIOUS_COLUMN] = A(AR_PREV_COL),
    [ACTION_NEXT_COLUMN] = A(AR_NEXT_COL),
    [ACTION_MASTER_SCREEN] = A(AR_LOCK),
    [ACTION_SLAVE_SCREEN] = A(AR_INACTIVE),
    [ACTION_VOLUME_UP] = A(AR_MPD|AR_VOL),
    [ACTION_VOLUME_DOWN] = A(AR_MPD|AR_VOL),
    [ACTION_ADD_ITEM_TO_PLAYLIST] = C(AR_MPD),
    [ACTION_PLAY_ITEM] = C(AR_MPD),
    [ACTION_DELETE_PLAYLIST_ITEMS] = C(AR_MPD),
    [ACTION_DELETE_STORED_PLAYLIST] = C(AR_MPD),
    [ACTION_DELETE_BROWSER_ITEMS] = SC(AR0, ACTION_SCREEN_IS_TARGET,
                                       SCREEN_TYPE_BROWSER),
    [ACTION_REPLAY_SONG] = A(AR_MPD|AR_POS),
    [ACTION_PAUSE] = A(AR_ACTIVE),
    [ACTION_MOVE_SORT_ORDER_UP] = AS(AR0, ACTION_SCREEN_IS_TARGET,
                                    SCREEN_TYPE_SORT_PLAYLIST_DIALOG),
    [ACTION_MOVE_SORT_ORDER_DOWN] = AS(AR0, ACTION_SCREEN_IS_TARGET,
                                      SCREEN_TYPE_SORT_PLAYLIST_DIALOG),
    [ACTION_MOVE_SELECTED_ITEMS_UP] = C(AR_MPD),
    [ACTION_MOVE_SELECTED_ITEMS_DOWN] = C(AR_MPD),
    [ACTION_MOVE_SELECTED_ITEMS_TO] = C(AR_MPD),
    [ACTION_SEEK_FORWARD] = A(AR_MPD|AR_ACTIVE|AR_TOTAL),
    [ACTION_SEEK_BACKWARD] = A(AR_MPD|AR_ACTIVE|AR_TOTAL),
    [ACTION_TOGGLE_DISPLAY_MODE] = A(AR_DISPLAY),
    [ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE] = AS(
        AR0, ACTION_SCREEN_IS_TARGET, SCREEN_TYPE_LYRICS),
    [ACTION_JUMP_TO_PLAYING_SONG] = A(AR_MPD|AR_POS),
    [ACTION_SHUFFLE] = SC(AR_MPD, ACTION_SCREEN_IS_TARGET,
                          SCREEN_TYPE_PLAYLIST),
    [ACTION_START_SEARCHING] = SC(AR0, ACTION_SCREEN_IS_TARGET,
                                  SCREEN_TYPE_SEARCH_ENGINE),
    [ACTION_SAVE_TAG_CHANGES] = C(AR0),
    [ACTION_SET_VOLUME] = A(AR_MPD|AR_VOL),
    [ACTION_ENTER_DIRECTORY] = C(AR0),
    [ACTION_EDIT_SONG] = AS(AR_TAGLIB|AR_MUSIC_DIR|AR_SONG,
                           ACTION_SCREEN_IS_NOT_TARGET, SCREEN_TYPE_LYRICS),
    [ACTION_EDIT_LIBRARY_TAG] = C(AR_TAGLIB),
    [ACTION_EDIT_LIBRARY_ALBUM] = C(AR_TAGLIB),
    [ACTION_EDIT_DIRECTORY_NAME] = C(AR0),
    [ACTION_EDIT_PLAYLIST_NAME] = C(AR_MPD),
    [ACTION_EDIT_LYRICS] = AS(AR0, ACTION_SCREEN_IS_TARGET,
                             SCREEN_TYPE_LYRICS),
    [ACTION_JUMP_TO_BROWSER] = A(AR_SONG),
    [ACTION_JUMP_TO_MEDIA_LIBRARY] = A(AR_SONG),
    [ACTION_JUMP_TO_PLAYLIST_EDITOR] = C(AR0),
    [ACTION_JUMP_TO_TAG_EDIT] = A(AR_TAGLIB|AR_MUSIC_DIR|AR_SONG),
    [ACTION_JUMP_TO_POSITION_IN_SONG] = A(
        AR_MPD|AR_ACTIVE|AR_TOTAL|AR_POS),
    [ACTION_SELECT_ITEM] = A(AR_SELECT),
    [ACTION_SELECT_RANGE] = A(AR_SELECTION),
    [ACTION_REVERSE_SELECTION] = A(AR_MENU),
    [ACTION_REMOVE_SELECTION] = A(AR_MENU),
    [ACTION_SELECT_ALBUM] = CA(AR0, SONG_GETTER_ALBUM),
    [ACTION_SELECT_FOUND_ITEMS] = C(AR_SEARCH),
    [ACTION_ADD_SELECTED_ITEMS] = A(AR_SONGS),
    [ACTION_CROP_MAIN_PLAYLIST] = SC(AR_MPD|AR_SONGS,
                                     ACTION_SCREEN_IS_TARGET,
                                     SCREEN_TYPE_PLAYLIST),
    [ACTION_CROP_PLAYLIST] = C(AR_MPD),
    [ACTION_CLEAR_PLAYLIST] = C(AR_MPD),
    [ACTION_SORT_PLAYLIST] = SC(AR_MPD, ACTION_SCREEN_IS_TARGET,
                                SCREEN_TYPE_PLAYLIST),
    [ACTION_REVERSE_PLAYLIST] = SC(AR_MPD, ACTION_SCREEN_IS_TARGET,
                                   SCREEN_TYPE_PLAYLIST),
    [ACTION_APPLY_FILTER] = A(AR_FILTER),
    [ACTION_FIND] = A(AR_FIND),
    [ACTION_FETCH_LYRICS_IN_BACKGROUND] = A(AR_SONGS),
    [ACTION_REFETCH_LYRICS] = AS(AR0, ACTION_SCREEN_IS_TARGET,
                                SCREEN_TYPE_LYRICS),
    [ACTION_SET_SELECTED_ITEMS_PRIORITY] = SC(AR_MPD|AR_SONGS,
                                              ACTION_SCREEN_IS_TARGET,
                                              SCREEN_TYPE_PLAYLIST),
    [ACTION_TOGGLE_OUTPUT] = C(AR0),
    [ACTION_TOGGLE_VISUALIZATION_TYPE] = C(AR0),
    [ACTION_SHOW_ARTIST_INFO] = C(AR0),
    [ACTION_SHOW_LYRICS] = C(AR0),
    [ACTION_SHOW_HELP] = AS(AR_NOT_TINY, ACTION_SCREEN_IS_NOT_TARGET,
                           SCREEN_TYPE_HELP),
    [ACTION_SHOW_PLAYLIST] = AS(AR_NOT_TINY, ACTION_SCREEN_IS_NOT_TARGET,
                               SCREEN_TYPE_PLAYLIST),
    [ACTION_SHOW_BROWSER] = AS(AR0, ACTION_SCREEN_IS_NOT_TARGET,
                              SCREEN_TYPE_BROWSER),
    [ACTION_CHANGE_BROWSE_MODE] = AS(AR0, ACTION_SCREEN_IS_TARGET,
                                    SCREEN_TYPE_BROWSER),
    [ACTION_RESET_SEARCH_ENGINE] = AS(AR0, ACTION_SCREEN_IS_TARGET,
                                     SCREEN_TYPE_SEARCH_ENGINE),
    [ACTION_SHOW_MEDIA_LIBRARY] = AS(AR_NOT_TINY, ACTION_SCREEN_IS_NOT_TARGET,
                                    SCREEN_TYPE_MEDIA_LIBRARY),
    [ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE] = AS(
        AR0, ACTION_SCREEN_IS_TARGET, SCREEN_TYPE_MEDIA_LIBRARY),
    [ACTION_SHOW_PLAYLIST_EDITOR] = AS(AR_NOT_TINY,
                                      ACTION_SCREEN_IS_NOT_TARGET,
                                      SCREEN_TYPE_PLAYLIST_EDITOR),
    [ACTION_SHOW_TAG_EDIT] = A(AR_TAGLIB),
    [ACTION_SHOW_OUTPUTS] = C(AR_NOT_TINY),
    [ACTION_SHOW_VISUALIZER] = C(AR_NOT_TINY),
    [ACTION_SHOW_SERVER_INFO] = A(AR_NOT_TINY),
    [ACTION_TOGGLE_LIBRARY_TAG_TYPE] = C(AR0),
    [ACTION_TOGGLE_BROWSER_SORT_MODE] = AS(AR0, ACTION_SCREEN_IS_TARGET,
                                          SCREEN_TYPE_BROWSER),
    [ACTION_TOGGLE_MEDIA_LIBRARY_SORT_MODE] = AS(
        AR0, ACTION_SCREEN_IS_TARGET, SCREEN_TYPE_MEDIA_LIBRARY),
};

static bool
action_availability_custom_can_run(enum ActionType type, int32 argument) {
    switch ((int32)type) {
    case ACTION_MOUSE_EVENT:
        return Config.mouse_support;
    case ACTION_SCROLL_UP_ARTIST:
    case ACTION_SCROLL_DOWN_ARTIST:
    case ACTION_SCROLL_UP_ALBUM:
    case ACTION_SCROLL_DOWN_ALBUM:
    case ACTION_SELECT_ALBUM:
        return action_runtime_tag_scroll_available((enum SongGetter)argument);
    case ACTION_JUMP_TO_PARENT_DIRECTORY:
    case ACTION_ENTER_DIRECTORY:
        if (action_runtime_current_screen_is(SCREEN_TYPE_BROWSER)) {
            return true;
        }
#if defined(HAVE_TAGLIB_H)
        return action_runtime_current_screen_is(SCREEN_TYPE_TAG_EDIT);
#else
        return false;
#endif
    case ACTION_RUN_ACTION:
        return nc_screen_can_run_current(app_controller_current_screen());
    case ACTION_ADD_ITEM_TO_PLAYLIST:
        if (action_runtime_current_screen_is(SCREEN_TYPE_MEDIA_LIBRARY)) {
            return media_library_screen_has_available_item(
                app_screen_media_library());
        }
        if (action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST_EDITOR)) {
            return action_runtime_playlist_edit_has_playlists();
        }
        return action_runtime_has_selected_songs();
    case ACTION_PLAY_ITEM:
        if (action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST)) {
            return action_runtime_has_current_song();
        }
        if (action_runtime_current_screen_is(SCREEN_TYPE_MEDIA_LIBRARY)) {
            return media_library_screen_has_available_item(
                app_screen_media_library());
        }
        if (action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST_EDITOR)) {
            return action_runtime_playlist_edit_has_playlists();
        }
        return action_runtime_has_selected_songs();
    case ACTION_DELETE_PLAYLIST_ITEMS:
    case ACTION_MOVE_SELECTED_ITEMS_UP:
    case ACTION_MOVE_SELECTED_ITEMS_DOWN:
        if (action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST)) {
            return action_runtime_has_selected_songs();
        }
        return action_runtime_playlist_edit_content_is_active()
               && action_runtime_playlist_edit_has_content();
    case ACTION_DELETE_STORED_PLAYLIST:
    case ACTION_CROP_PLAYLIST:
    case ACTION_CLEAR_PLAYLIST:
        return action_runtime_playlist_edit_has_playlists();
    case ACTION_DELETE_BROWSER_ITEMS:
        if (!Config.allow_for_physical_item_deletion) {
            return false;
        }
        if (!browser_screen_is_local(app_screen_browser())
            && (Config.mpd_music_dir_len <= 0)) {
            return false;
        }
        return action_runtime_menu_has_items();
    case ACTION_MOVE_SELECTED_ITEMS_TO:
        if (action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST)) {
            return nc_menu_has_selected(
                playlist_screen_menu(app_screen_playlist()));
        }
        return action_runtime_playlist_edit_content_is_active()
               && nc_menu_has_selected(
                   nc_song_menu_base(playlist_edit_screen_content(
                       app_screen_playlist_edit())));
    case ACTION_SHUFFLE: {
        int32 first;
        int32 last;

        return action_runtime_playlist_range(action_runtime_current_menu(),
                                             &first, &last) == 0;
    }
    case ACTION_SAVE_TAG_CHANGES:
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(SCREEN_TYPE_TAG_EDIT)) {
            return tag_edit_screen_save_action_available(app_screen_tag_edit());
        }
        return action_runtime_current_screen_is(SCREEN_TYPE_TINY_TAG_EDIT);
#else
        return false;
#endif
    case ACTION_JUMP_TO_PLAYLIST_EDITOR:
        if (action_runtime_current_screen_is(SCREEN_TYPE_BROWSER)) {
            NcmMpdItem *item;

            item = browser_screen_current_item(app_screen_browser());
            return item && (ncm_mpd_item_kind(item) == NCM_MPD_ITEM_PLAYLIST);
        }
        return true;
    case ACTION_CROP_MAIN_PLAYLIST:
        return playlist_screen_song_count(app_screen_playlist()) > 1;
    case ACTION_SORT_PLAYLIST:
        return playlist_screen_has_sortable_range(app_screen_playlist());
    case ACTION_REVERSE_PLAYLIST: {
        NcMenu *menu;
        int32 first;
        int32 last;

        if (((menu = action_runtime_current_menu()) == NULL)
            || (nc_menu_item_count(menu) <= 0)) {
            return false;
        }
        return ncm_menu_find_full_selected_range(
            menu, action_runtime_menu_item_source(menu), &first, &last) == 0;
    }
    case ACTION_TOGGLE_LIBRARY_TAG_TYPE: {
        MediaLibraryScreen *library;
        enum MediaLibraryColumn column;

        if (!action_runtime_current_screen_is(SCREEN_TYPE_MEDIA_LIBRARY)) {
            return false;
        }
        library = app_screen_media_library();
        column = media_library_screen_active_column(library);
        return (column == MEDIA_LIBRARY_COLUMN_TAGS)
               || ((media_library_screen_column_count(library) == 2)
                   && (column == MEDIA_LIBRARY_COLUMN_ALBUMS));
    }
    case ACTION_SHOW_ARTIST_INFO:
        if (action_runtime_current_screen_is(SCREEN_TYPE_LASTFM)) {
            return true;
        }
        if (action_runtime_media_library_current_artist_tag(NULL, NULL)) {
            return true;
        }
        return action_runtime_has_current_song();
    case ACTION_SHOW_LYRICS:
        return action_runtime_current_screen_is(SCREEN_TYPE_LYRICS)
               || action_runtime_has_current_song();
    case ACTION_TOGGLE_OUTPUT:
#if defined(ENABLE_OUTPUTS)
        return action_runtime_current_screen_is(SCREEN_TYPE_OUTPUTS);
#else
        return false;
#endif
    case ACTION_TOGGLE_VISUALIZATION_TYPE:
#if defined(ENABLE_VISUALIZER)
        return action_runtime_current_screen_is(SCREEN_TYPE_VISUALIZER);
#else
        return false;
#endif
    case ACTION_START_SEARCHING:
        return !search_engine_screen_has_locked_constraints(
            app_screen_search_engine());
    case ACTION_SELECT_FOUND_ITEMS: {
        StringView constraint;

        constraint = current_screen_current_search_constraint();
        return action_runtime_menu_has_items() && constraint.data
               && (constraint.len > 0);
    }
    case ACTION_SET_SELECTED_ITEMS_PRIORITY:
        return ncm_mpd_client_version(&global_mpd) >= 17;
    case ACTION_EDIT_PLAYLIST_NAME:
        if (action_runtime_current_screen_is(SCREEN_TYPE_BROWSER)) {
            return browser_screen_can_rename_playlist(app_screen_browser());
        }
        return action_runtime_playlist_edit_playlists_is_active()
               && action_runtime_playlist_edit_has_playlists();
    case ACTION_EDIT_DIRECTORY_NAME:
        if (action_runtime_current_screen_is(SCREEN_TYPE_BROWSER)) {
            return browser_screen_can_rename_directory(app_screen_browser());
        }
#if defined(HAVE_TAGLIB_H)
        if (action_runtime_current_screen_is(SCREEN_TYPE_TAG_EDIT)) {
            return tag_edit_screen_rename_directory_available(
                app_screen_tag_edit(), Config.mpd_music_dir,
                Config.mpd_music_dir_len);
        }
#endif
        return false;
    case ACTION_EDIT_LIBRARY_TAG:
        return action_runtime_can_edit_library_tag();
    case ACTION_EDIT_LIBRARY_ALBUM:
        return action_runtime_can_edit_library_album();
    case ACTION_SHOW_OUTPUTS:
#if defined(ENABLE_OUTPUTS)
        return !action_runtime_current_screen_is(SCREEN_TYPE_OUTPUTS);
#else
        return false;
#endif
    case ACTION_SHOW_VISUALIZER:
#if defined(ENABLE_VISUALIZER)
        return !action_runtime_current_screen_is(SCREEN_TYPE_VISUALIZER);
#else
        return false;
#endif
    default:
        return false;
    }
}

#define CHECK_REQUIREMENT(req, expr)                                         \
    do {                                                                    \
        if ((requirements & (req)) && !(expr)) {                            \
            return false;                                                   \
        }                                                                   \
    } while (0)

static bool
action_availability_requirements_met(enum ActionRequirement requirements) {
#if !defined(HAVE_TAGLIB_H)
    if (requirements & AR_TAGLIB) {
        return false;
    }
#endif
#if defined(HAVE_TAGLIB_H)
    CHECK_REQUIREMENT(AR_NOT_TINY,
                      !action_runtime_current_screen_is(
                          SCREEN_TYPE_TINY_TAG_EDIT));
#endif
    CHECK_REQUIREMENT(AR_MPD, ncm_mpd_client_is_connected(&global_mpd));
    CHECK_REQUIREMENT(AR_VOL, ncm_status_state_volume() >= 0);
    CHECK_REQUIREMENT(AR_ACTIVE,
                      ncm_status_state_player() != NCM_STATUS_PLAYER_STOP);
    CHECK_REQUIREMENT(AR_TOTAL, ncm_status_state_total_time() > 0);
    CHECK_REQUIREMENT(AR_POS,
                      ncm_status_state_current_song_position() >= 0);
    CHECK_REQUIREMENT(AR_SONG, action_runtime_has_current_song());
    CHECK_REQUIREMENT(AR_SONGS, action_runtime_has_selected_songs());
    CHECK_REQUIREMENT(AR_MENU, action_runtime_current_menu() != NULL);
    CHECK_REQUIREMENT(AR_SELECT, action_runtime_menu_has_selectable_item());
    CHECK_REQUIREMENT(AR_SELECTION, action_runtime_menu_has_selection());
    CHECK_REQUIREMENT(AR_FILTER, current_screen_can_filter());
    CHECK_REQUIREMENT(AR_FIND, current_screen_can_find());
    CHECK_REQUIREMENT(AR_SEARCH, current_screen_can_search());
    CHECK_REQUIREMENT(AR_PREV_COL, action_runtime_previous_column_available());
    CHECK_REQUIREMENT(AR_NEXT_COL, action_runtime_next_column_available());
    CHECK_REQUIREMENT(AR_DISPLAY,
                      nc_screen_has_capability(
                          current_screen(), NC_SCREEN_CAPABILITY_DISPLAY_MODE));
    CHECK_REQUIREMENT(AR_LOCK, app_controller_can_show_locked_screen());
    CHECK_REQUIREMENT(AR_INACTIVE, app_controller_can_show_inactive_screen());
    CHECK_REQUIREMENT(AR_MUSIC_DIR, Config.mpd_music_dir_len > 0);
    return true;
}

#undef CHECK_REQUIREMENT
static bool
action_availability_can_run(enum ActionType type) {
    ActionAvailability availability;

    if ((uint32)type >= ACTION_COUNT) {
        return false;
    }

    availability = action_availability_table[type];
    if (!action_availability_requirements_met(availability.requirements)) {
        return false;
    }
    if ((availability.screen_requirement == ACTION_SCREEN_IS_TARGET)
        && !action_runtime_current_screen_is(availability.screen_type)) {
        return false;
    }
    if ((availability.screen_requirement == ACTION_SCREEN_IS_NOT_TARGET)
        && action_runtime_current_screen_is(availability.screen_type)) {
        return false;
    }
    if (availability.custom) {
        return action_availability_custom_can_run(type, availability.argument);
    }
    return true;
}

static bool
action_runtime_builtin_can_run(ActionRuntime *runtime, enum ActionType type) {
    (void)runtime;
    return action_availability_can_run(type);
}

enum ActionRunner {
    ACTION_RUN_CUSTOM = 0,
    ACTION_RUN_NOOP,
    ACTION_RUN_NO_ARG,
    ACTION_RUN_SCROLL,
    ACTION_RUN_SCROLL_TAG,
    ACTION_RUN_COLUMN,
    ACTION_RUN_CONTROLLER_SCREEN,
    ACTION_RUN_MPD_SIMPLE,
    ACTION_RUN_MPD_TOGGLE,
    ACTION_RUN_VOLUME,
    ACTION_RUN_SEEK,
    ACTION_RUN_DISPLAY_MODE,
    ACTION_RUN_CURRENT_SCREEN_ACTION,
    ACTION_RUN_SORT_ORDER,
    ACTION_RUN_MOVE_SELECTED_ITEMS,
    ACTION_RUN_SWITCH_SCREEN,
    ACTION_RUN_SWITCH_NEXT_SCREEN,
    ACTION_RUN_MENU_SELECT_CURRENT,
    ACTION_RUN_FIND_ITEM,
    ACTION_RUN_REPEAT_SEARCH,
    ACTION_RUN_REQUEST_EXIT,
};

typedef int32 ActionRunNoArgFunction(void);
typedef int32 ActionRunMpdSimpleFunction(MpdClient *, NcmError *);
typedef int32 ActionRunMpdToggleFunction(MpdClient *, bool, NcmError *);
typedef bool ActionRunBoolFunction(void);

typedef struct ActionRun {
    enum ActionRunner runner;
    ActionRunNoArgFunction *no_arg;
    ActionRunMpdSimpleFunction *mpd_simple;
    ActionRunMpdToggleFunction *mpd_toggle;
    ActionRunBoolFunction *current_toggle_state;

    int32 argument;
} ActionRun;

#define R0(action) [action] = {.runner = ACTION_RUN_NOOP},
#define RF(action, func) \
    [action] = {.runner = ACTION_RUN_NO_ARG, .no_arg = (func)},
#define R1(action, run_, arg_) \
    [action] = {.runner = (run_), .argument = (arg_)},
#define RM(action, func) \
    [action] = {.runner = ACTION_RUN_MPD_SIMPLE, .mpd_simple = (func)},
#define RT(action, func, state) \
    [action] = {.runner = ACTION_RUN_MPD_TOGGLE, \
                .mpd_toggle = (func), \
                .current_toggle_state = (state)},

static const ActionRun action_run_table[ACTION_COUNT] = {
    R0(ACTION_DUMMY)
    RF(ACTION_MOUSE_EVENT, action_runtime_mouse_event)
    R1(ACTION_SCROLL_UP, ACTION_RUN_SCROLL, NC_SCROLL_UP)
    R1(ACTION_SCROLL_DOWN, ACTION_RUN_SCROLL, NC_SCROLL_DOWN)
    R1(ACTION_SCROLL_UP_ARTIST, ACTION_RUN_SCROLL_TAG, SONG_GETTER_ARTIST)
    R1(ACTION_SCROLL_UP_ALBUM, ACTION_RUN_SCROLL_TAG, SONG_GETTER_ALBUM)
    R1(ACTION_SCROLL_DOWN_ARTIST, ACTION_RUN_SCROLL_TAG,
       SONG_GETTER_ARTIST + 1000)
    R1(ACTION_SCROLL_DOWN_ALBUM, ACTION_RUN_SCROLL_TAG,
       SONG_GETTER_ALBUM + 1000)
    R1(ACTION_PAGE_UP, ACTION_RUN_SCROLL, NC_SCROLL_PAGE_UP)
    R1(ACTION_PAGE_DOWN, ACTION_RUN_SCROLL, NC_SCROLL_PAGE_DOWN)
    R1(ACTION_MOVE_HOME, ACTION_RUN_SCROLL, NC_SCROLL_HOME)
    R1(ACTION_MOVE_END, ACTION_RUN_SCROLL, NC_SCROLL_END)
    RF(ACTION_TOGGLE_INTERFACE, action_runtime_toggle_interface)
    RF(ACTION_JUMP_TO_PARENT_DIRECTORY, action_runtime_jump_to_parent_directory)
    [ACTION_RUN_ACTION] = {.runner = ACTION_RUN_CURRENT_SCREEN_ACTION},
    R1(ACTION_PREVIOUS_COLUMN, ACTION_RUN_COLUMN, false)
    R1(ACTION_NEXT_COLUMN, ACTION_RUN_COLUMN, true)
    R1(ACTION_MASTER_SCREEN, ACTION_RUN_CONTROLLER_SCREEN, true)
    R1(ACTION_SLAVE_SCREEN, ACTION_RUN_CONTROLLER_SCREEN, false)
    RM(ACTION_PLAY, ncm_mpd_client_play)
    RM(ACTION_PAUSE, ncm_mpd_client_toggle_pause)
    RM(ACTION_STOP, ncm_mpd_client_stop)
    RM(ACTION_NEXT, ncm_mpd_client_next)
    RM(ACTION_PREVIOUS, ncm_mpd_client_previous)
    RF(ACTION_REPLAY_SONG, action_runtime_replay_song)
    R1(ACTION_VOLUME_UP, ACTION_RUN_VOLUME, 1)
    R1(ACTION_VOLUME_DOWN, ACTION_RUN_VOLUME, -1)
    RF(ACTION_DELETE_PLAYLIST_ITEMS, action_runtime_delete_playlist_items)
    RF(ACTION_DELETE_STORED_PLAYLIST, action_runtime_delete_stored_playlists)
    RF(ACTION_DELETE_BROWSER_ITEMS, action_runtime_delete_browser_items)
    RF(ACTION_EXECUTE_COMMAND, action_runtime_execute_command)
    RF(ACTION_SAVE_PLAYLIST, action_runtime_save_playlist)
    R1(ACTION_MOVE_SORT_ORDER_UP, ACTION_RUN_SORT_ORDER, false)
    R1(ACTION_MOVE_SORT_ORDER_DOWN, ACTION_RUN_SORT_ORDER, true)
    R1(ACTION_MOVE_SELECTED_ITEMS_UP, ACTION_RUN_MOVE_SELECTED_ITEMS, false)
    R1(ACTION_MOVE_SELECTED_ITEMS_DOWN, ACTION_RUN_MOVE_SELECTED_ITEMS, true)
    RF(ACTION_MOVE_SELECTED_ITEMS_TO, action_runtime_move_selected_items_to)
    RF(ACTION_LOAD, action_runtime_load_prompt)
    R1(ACTION_SEEK_FORWARD, ACTION_RUN_SEEK, true)
    R1(ACTION_SEEK_BACKWARD, ACTION_RUN_SEEK, false)
    [ACTION_TOGGLE_DISPLAY_MODE] = {.runner = ACTION_RUN_DISPLAY_MODE},
    RF(ACTION_TOGGLE_SEPARATORS_BETWEEN_ALBUMS,
                            action_runtime_toggle_separators_between_albums)
    RF(ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE,
                            action_runtime_toggle_lyrics_update_on_song_change)
    RF(ACTION_TOGGLE_LYRICS_FETCHER, action_runtime_toggle_lyrics_fetcher)
    RF(ACTION_TOGGLE_FETCHING_LYRICS_IN_BACKGROUND,
                            action_runtime_toggle_fetch_lyrics_in_background)
    RF(ACTION_UPDATE_DATABASE, action_runtime_update_database)
    RF(ACTION_JUMP_TO_PLAYING_SONG, action_runtime_jump_to_playing_song)
    RT(ACTION_TOGGLE_REPEAT,
                                ncm_mpd_client_set_repeat,
                                ncm_status_state_repeat_is_enabled)
    RT(ACTION_TOGGLE_RANDOM,
                                ncm_mpd_client_set_random,
                                ncm_status_state_random_is_enabled)
    RF(ACTION_SAVE_TAG_CHANGES, action_runtime_save_tag_changes)
    RT(ACTION_TOGGLE_SINGLE,
                                ncm_mpd_client_set_single,
                                ncm_status_state_single_is_enabled)
    RT(ACTION_TOGGLE_CONSUME,
                                ncm_mpd_client_set_consume,
                                ncm_status_state_consume_is_enabled)
    RF(ACTION_TOGGLE_CROSSFADE, action_runtime_toggle_crossfade)
    RF(ACTION_SET_CROSSFADE, action_runtime_set_crossfade)
    RF(ACTION_SET_VOLUME, action_runtime_set_volume)
    RF(ACTION_ENTER_DIRECTORY, action_runtime_enter_directory)
    RF(ACTION_EDIT_SONG, action_runtime_edit_current_song)
    RF(ACTION_EDIT_LIBRARY_TAG, action_runtime_edit_library_tag)
    RF(ACTION_EDIT_LIBRARY_ALBUM, action_runtime_edit_library_album)
    RF(ACTION_EDIT_DIRECTORY_NAME, action_runtime_edit_directory_name)
    RF(ACTION_EDIT_PLAYLIST_NAME, action_runtime_edit_playlist_name)
    RF(ACTION_EDIT_LYRICS, action_runtime_edit_lyrics)
    RF(ACTION_JUMP_TO_BROWSER, action_runtime_jump_to_browser)
    RF(ACTION_JUMP_TO_MEDIA_LIBRARY, action_runtime_jump_to_media_library)
    RF(ACTION_JUMP_TO_PLAYLIST_EDITOR, action_runtime_jump_to_playlist_edit)
    RF(ACTION_TOGGLE_SCREEN_LOCK, action_runtime_toggle_screen_lock)
    RF(ACTION_JUMP_TO_TAG_EDIT, action_runtime_jump_to_tag_edit)
    RF(ACTION_JUMP_TO_POSITION_IN_SONG, action_runtime_jump_to_position_in_song)
    [ACTION_SELECT_ITEM] = {.runner = ACTION_RUN_MENU_SELECT_CURRENT},
    RF(ACTION_SELECT_ALBUM, action_runtime_select_album)
    RF(ACTION_SELECT_FOUND_ITEMS, action_runtime_select_found_items)
    R1(ACTION_SORT_PLAYLIST, ACTION_RUN_SWITCH_SCREEN,
       SCREEN_TYPE_SORT_PLAYLIST_DIALOG)
    RF(ACTION_REVERSE_PLAYLIST, action_runtime_reverse_playlist)
    RF(ACTION_APPLY_FILTER, action_runtime_apply_filter)
    RF(ACTION_FIND, action_runtime_find)
    R1(ACTION_FIND_ITEM_FORWARD, ACTION_RUN_FIND_ITEM,
       NCM_SEARCH_DIRECTION_FORWARD)
    R1(ACTION_FIND_ITEM_BACKWARD, ACTION_RUN_FIND_ITEM,
       NCM_SEARCH_DIRECTION_BACKWARD)
    R1(ACTION_NEXT_FOUND_ITEM, ACTION_RUN_REPEAT_SEARCH,
       NCM_SEARCH_DIRECTION_FORWARD)
    R1(ACTION_PREVIOUS_FOUND_ITEM, ACTION_RUN_REPEAT_SEARCH,
       NCM_SEARCH_DIRECTION_BACKWARD)
    RF(ACTION_TOGGLE_REPLAY_GAIN_MODE, action_runtime_toggle_replay_gain_mode)
    RF(ACTION_TOGGLE_ADD_MODE, action_runtime_toggle_add_mode)
    RF(ACTION_TOGGLE_MOUSE, action_runtime_toggle_mouse)
    RF(ACTION_TOGGLE_BITRATE_VISIBILITY,
                            action_runtime_toggle_bitrate_visibility)
    RF(ACTION_ADD_RANDOM_ITEMS, action_runtime_add_random_items)
    RF(ACTION_TOGGLE_BROWSER_SORT_MODE, action_runtime_toggle_browser_sort_mode)
    RF(ACTION_TOGGLE_LIBRARY_TAG_TYPE, action_runtime_toggle_library_tag_type)
    RF(ACTION_TOGGLE_MEDIA_LIBRARY_SORT_MODE,
                            action_runtime_toggle_media_library_sort_mode)
    RF(ACTION_FETCH_LYRICS_IN_BACKGROUND,
                            action_runtime_fetch_lyrics_background)
    RF(ACTION_REFETCH_LYRICS, action_runtime_refetch_lyrics)
    RF(ACTION_SET_SELECTED_ITEMS_PRIORITY,
                            action_runtime_set_selected_items_priority)
    RF(ACTION_TOGGLE_VISUALIZATION_TYPE, ncm_action_toggle_visualization_type)
    R1(ACTION_SHOW_SONG_INFO, ACTION_RUN_SWITCH_SCREEN, SCREEN_TYPE_SONG_INFO)
    RF(ACTION_SHOW_ARTIST_INFO, action_runtime_show_artist_info)
    RF(ACTION_SHOW_LYRICS, action_runtime_show_lyrics)
    [ACTION_QUIT] = {.runner = ACTION_RUN_REQUEST_EXIT},
    R1(ACTION_NEXT_SCREEN, ACTION_RUN_SWITCH_NEXT_SCREEN, false)
    R1(ACTION_PREVIOUS_SCREEN, ACTION_RUN_SWITCH_NEXT_SCREEN, true)
    R1(ACTION_SHOW_HELP, ACTION_RUN_SWITCH_SCREEN, SCREEN_TYPE_HELP)
    R1(ACTION_SHOW_PLAYLIST, ACTION_RUN_SWITCH_SCREEN, SCREEN_TYPE_PLAYLIST)
    R1(ACTION_SHOW_BROWSER, ACTION_RUN_SWITCH_SCREEN, SCREEN_TYPE_BROWSER)
    RF(ACTION_CHANGE_BROWSE_MODE, action_runtime_change_browse_mode)
    R1(ACTION_SHOW_SEARCH_ENGINE, ACTION_RUN_SWITCH_SCREEN,
       SCREEN_TYPE_SEARCH_ENGINE)
    R1(ACTION_SHOW_MEDIA_LIBRARY, ACTION_RUN_SWITCH_SCREEN,
       SCREEN_TYPE_MEDIA_LIBRARY)
    RF(ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE,
                            action_runtime_toggle_media_library_columns)
    R1(ACTION_SHOW_PLAYLIST_EDITOR, ACTION_RUN_SWITCH_SCREEN,
       SCREEN_TYPE_PLAYLIST_EDITOR)
    R1(ACTION_SHOW_SERVER_INFO, ACTION_RUN_SWITCH_SCREEN,
       SCREEN_TYPE_SERVER_INFO)
};

#undef R0
#undef RF
#undef R1
#undef RM
#undef RT

static int32
action_runtime_custom_run(ActionRuntime *runtime, enum ActionType type) {
    (void)runtime;
    switch ((int32)type) {
    case ACTION_UPDATE_ENVIRONMENT:
        return ncmpcpp_update_environment(true, true, true);
    case ACTION_ADD_ITEM_TO_PLAYLIST: {
        int32 status;

        status = action_runtime_add_item_to_playlist(false);
        if (status < 0) {
            return status;
        }
        app_controller_scroll_current_screen(NC_SCROLL_DOWN);
        nc_screen_finish_list_change(app_controller_current_screen());
        return 0;
    }
    case ACTION_PLAY_ITEM: {
        int32 status;

        if (action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST)) {
            return nc_playlist_screen_activate_current(
                playlist_screen_playlist(app_screen_playlist()));
        }
        status = action_runtime_add_item_to_playlist(true);
        if (status < 0) {
            return status;
        }
        nc_screen_finish_list_change(app_controller_current_screen());
        return 0;
    }
    case ACTION_ADD:
        if (action_runtime_current_screen_is(
            SCREEN_TYPE_SELECTED_ITEMS_ADDER)) {
            return selected_items_adder_screen_run_current(
                app_screen_selected_items_adder());
        }
        return action_runtime_add_prompt();
    case ACTION_TOGGLE_PLAYING_SONG_CENTERING:
        Config.autocenter_mode = !Config.autocenter_mode;
        if (Config.autocenter_mode) {
            int32 position;

            position = ncm_status_state_current_song_position();
            if (position >= 0) {
                (void)playlist_screen_locate_position(app_screen_playlist(),
                                                      position);
            }
        }
        if (Config.autocenter_mode) {
            ncm_statusbar_print(Config.message_delay_time,
                                STRLIT("Centering playing song: on"));
        } else {
            ncm_statusbar_print(Config.message_delay_time,
                                STRLIT("Centering playing song: off"));
        }
        return 0;
    case ACTION_SHUFFLE:
        if (action_runtime_current_screen_is(SCREEN_TYPE_PLAYLIST)) {
            return action_runtime_shuffle_playlist();
        }
        return action_runtime_mpd_simple(ncm_mpd_client_shuffle);
    case ACTION_SELECT_RANGE: {
        enum NcMenuItemSource source;
        NcMenu *menu;
        int32 first;
        int32 last;

        if ((menu = action_runtime_current_menu()) == NULL) {
            return -NCM_ERROR_UNAVAILABLE;
        }
        source = action_runtime_menu_item_source(menu);
        if (ncm_menu_find_selected_range(menu, source, &first, &last) <= 0) {
            return -NCM_ERROR_UNAVAILABLE;
        }
        for (int32 i = first; i < last; i += 1) {
            (void)nc_menu_set_position_selected(menu, i, true);
        }
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Range selected"));
        return 0;
    }
    case ACTION_REVERSE_SELECTION: {
        NcMenu *menu;

        if ((menu = action_runtime_current_menu()) == NULL) {
            return -NCM_ERROR_UNAVAILABLE;
        }
        ncm_menu_reverse_selection(menu, action_runtime_menu_item_source(menu));
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Selection reversed"));
        return 0;
    }
    case ACTION_REMOVE_SELECTION:
        nc_menu_clear_selection(action_runtime_current_menu());
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Selection removed"));
        return 0;
    case ACTION_ADD_SELECTED_ITEMS: {
        NcmSongArray songs = {0};
        NcmError ncm_error;
        int32 status;

        if ((action_runtime_selected_songs(&songs) < 0) || (songs.len <= 0)) {
            ncm_song_array_destroy(&songs);
            return -NCM_ERROR_UNAVAILABLE;
        }

        ncm_error_clear(&ncm_error);
        status = app_screen_selected_items_adder_open(&songs, &ncm_error);
        ncm_song_array_destroy(&songs);
        if (status < 0) {
            return action_runtime_mpd_error_status(&ncm_error);
        }
        return 0;
    }
    case ACTION_CROP_MAIN_PLAYLIST:
        return action_runtime_crop_playlist(true);
    case ACTION_CROP_PLAYLIST:
        return action_runtime_crop_playlist(false);
    case ACTION_CLEAR_MAIN_PLAYLIST:
        return action_runtime_clear_playlist(true);
    case ACTION_CLEAR_PLAYLIST:
        return action_runtime_clear_playlist(false);
    case ACTION_RESET_SEARCH_ENGINE:
        search_engine_screen_reset(app_screen_search_engine());
        return 0;
    case ACTION_SHOW_OUTPUTS:
#if defined(ENABLE_OUTPUTS)
        return action_runtime_switch_to_screen(SCREEN_TYPE_OUTPUTS);
#else
        return -NCM_ERROR_UNAVAILABLE;
#endif
    case ACTION_TOGGLE_OUTPUT:
#if defined(ENABLE_OUTPUTS)
        app_screen_outputs_toggle();
        return 0;
#else
        return -NCM_ERROR_UNAVAILABLE;
#endif
    case ACTION_SHOW_VISUALIZER:
#if defined(ENABLE_VISUALIZER)
        return ncm_action_show_visualizer();
#else
        return -NCM_ERROR_UNAVAILABLE;
#endif
    case ACTION_SHOW_TAG_EDIT:
#if defined(HAVE_TAGLIB_H)
        return action_runtime_switch_to_screen(SCREEN_TYPE_TAG_EDIT);
#else
        return -NCM_ERROR_UNAVAILABLE;
#endif
    case ACTION_TOGGLE_FIND_MODE:
        if (Config.default_find_mode == NCM_DEFAULT_FIND_MODE_WRAPPED) {
            Config.default_find_mode = NCM_DEFAULT_FIND_MODE_NORMAL;
        } else {
            Config.default_find_mode = NCM_DEFAULT_FIND_MODE_WRAPPED;
        }
        if (Config.default_find_mode == NCM_DEFAULT_FIND_MODE_WRAPPED) {
            ncm_statusbar_print(Config.message_delay_time,
                                STRLIT("Search mode: Wrapped"));
        } else {
            ncm_statusbar_print(Config.message_delay_time,
                                STRLIT("Search mode: Normal"));
        }
        return 0;
    case ACTION_START_SEARCHING: {
        NcmError ncm_error;

        ncm_error_clear(&ncm_error);
        return search_engine_screen_start_searching(app_screen_search_engine(),
                                                    &global_mpd, &ncm_error);
    }
    case ACTION_COUNT:
    default:
        return -NCM_ERROR_UNAVAILABLE;
    }
}


static int32
action_run_execute(ActionRuntime *runtime, enum ActionType type,
                   ActionRun run) {
    int32 change;

    switch (run.runner) {
    case ACTION_RUN_CUSTOM:
        return action_runtime_custom_run(runtime, type);
    case ACTION_RUN_NOOP:
        return 0;
    case ACTION_RUN_NO_ARG:
        return run.no_arg();
    case ACTION_RUN_SCROLL:
        app_controller_scroll_current_screen((enum NcScroll)run.argument);
        return 0;
    case ACTION_RUN_SCROLL_TAG:
        if (run.argument >= 1000) {
            return action_runtime_scroll_by_tag(
                (enum SongGetter)(run.argument - 1000), true);
        }
        return action_runtime_scroll_by_tag(
            (enum SongGetter)run.argument, false);
    case ACTION_RUN_COLUMN:
        if (run.argument) {
            return action_runtime_next_column();
        }
        return action_runtime_previous_column();
    case ACTION_RUN_CONTROLLER_SCREEN:
        if (run.argument) {
            change = app_controller_show_locked_screen();
        } else {
            change = app_controller_show_inactive_screen();
        }
        if (change < 0) {
            return -NCM_ERROR_UNAVAILABLE;
        }
        ncm_title_draw_current_header();
        return 0;
    case ACTION_RUN_MPD_SIMPLE:
        return action_runtime_mpd_simple(run.mpd_simple);
    case ACTION_RUN_MPD_TOGGLE:
        return action_runtime_mpd_toggle(run.mpd_toggle,
                                         run.current_toggle_state());
    case ACTION_RUN_VOLUME:
        change = run.argument*Config.volume_change_step;
        return action_runtime_volume(change);
    case ACTION_RUN_SEEK:
        return action_runtime_seek_relative(run.argument != 0);
    case ACTION_RUN_DISPLAY_MODE:
        return nc_screen_toggle_display_mode(current_screen());
    case ACTION_RUN_CURRENT_SCREEN_ACTION:
        return nc_screen_run_current(app_controller_current_screen());
    case ACTION_RUN_SORT_ORDER:
        if (run.argument) {
            return sort_playlist_dialog_move_current_down(
                app_screen_sort_playlist_dialog());
        }
        return sort_playlist_dialog_move_current_up(
            app_screen_sort_playlist_dialog());
    case ACTION_RUN_MOVE_SELECTED_ITEMS:
        return action_runtime_move_selected_items(run.argument != 0);
    case ACTION_RUN_SWITCH_SCREEN:
        return action_runtime_switch_to_screen((enum ScreenType)run.argument);
    case ACTION_RUN_SWITCH_NEXT_SCREEN:
        return action_runtime_switch_to_next_screen(run.argument != 0);
    case ACTION_RUN_MENU_SELECT_CURRENT:
        return nc_menu_toggle_current_selected(action_runtime_current_menu());
    case ACTION_RUN_FIND_ITEM:
        return action_runtime_find_item((enum SearchDirection)run.argument);
    case ACTION_RUN_REPEAT_SEARCH:
        return action_runtime_repeat_search(
            (enum SearchDirection)run.argument);
    case ACTION_RUN_REQUEST_EXIT:
        runtime->exit_requested = true;
        return 0;
    default:
        return -NCM_ERROR_UNAVAILABLE;
    }
}

static int32
action_runtime_builtin_run(ActionRuntime *runtime, enum ActionType type) {
    if ((uint32)type >= ACTION_COUNT) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    return action_run_execute(runtime, type, action_run_table[type]);
}

ActionRuntime *
ncm_action_runtime_global(void) {
    if (!action_global_runtime_initialized) {
        action_global_runtime = (ActionRuntime){0};
        action_global_runtime_initialized = true;
    }
    return &action_global_runtime;
}

bool
ncm_action_runtime_exit_requested(ActionRuntime *runtime) {
    runtime = action_runtime_or_global(runtime);
    return runtime->exit_requested;
}

void
ncm_action_runtime_request_exit(ActionRuntime *runtime) {
    runtime = action_runtime_or_global(runtime);
    runtime->exit_requested = true;
    return;
}

bool
ncm_action_runtime_can_run(ActionRuntime *runtime, enum ActionType type) {
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

int32
ncm_action_runtime_run(ActionRuntime *runtime, enum ActionType type) {
    int32 hook_result;
    bool handled;

    runtime = action_runtime_or_global(runtime);
    if (!ncm_action_runtime_can_run(runtime, type)) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    hook_result = action_runtime_call_hook(runtime->run_hook, type,
                                           runtime->user);
    if (action_runtime_hook_allowed(hook_result, &handled)) {
        return 0;
    }
    if (action_runtime_hook_denied(hook_result, &handled)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (handled) {
        return hook_result;
    }

    return action_runtime_builtin_run(runtime, type);
}

#endif /* ACTIONS_C */
