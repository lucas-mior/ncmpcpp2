#if !defined(NCMPCPP_STATUS_C)
#define NCMPCPP_STATUS_C

#include "cbase.h"

#include "config.h"

#include <mpd/client.h>
#include <mpd/status.h>
#if defined(HAVE_NETINET_IN_H) && defined(HAVE_NETINET_TCP_H)
#include <netinet/tcp.h>
#endif

#include "actions.h"
#include "app_controller.h"
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

#define STATUS_MILLISECONDS_PER_SECOND 1000

static bool status_initialized;
static char status_consume;
static char status_crossfade;
static char status_db_updating;
static char status_repeat;
static char status_random;
static char status_single;
static int32 status_current_song_id;
static int32 status_current_song_pos;
static int32 status_elapsed_time;
static int64 status_elapsed_time_ms;
static int32 status_kbps;
static enum NcmStatusPlayerState status_player_state;
static int32 status_playlist_version;
static int32 status_playlist_length;
static int32 status_total_time;
static int32 status_volume;

static bool status_hooks_set;
static NcmStatusHooks status_hooks;
static bool status_ui_hooks_set;
static NcmStatusUiHooks status_ui_hooks;
static bool status_init_hooks_set;
static NcmStatusInitHooks status_init_hooks;
static void (*status_notification_observer)(void *user);
static void *status_notification_observer_user;
static void (*status_database_update_observer)(void *user);
static void *status_database_update_observer_user;
static void (*status_playlist_update_observer)(void *user);
static void *status_playlist_update_observer_user;
static int64 status_past;
static int64 status_elapsed_time_updated_at;
static int32 status_playing_song_scroll_begin;
static int32 status_first_line_scroll_begin;
static int32 status_second_line_scroll_begin;

typedef struct StatusTimeoutContext {
    int32 timeout;
} StatusTimeoutContext;

static int32 status_player_state_string(char *buffer, int32 buffer_cap);
static void status_draw_song_title(NcmSong *song);
static void status_reset_visualizer_for_player_event(int32 event);

static NcmStatusHooks *
status_active_hooks(NcmStatusHooks *hooks) {
    if (hooks) {
        return hooks;
    }

    if (status_hooks_set) {
        return &status_hooks;
    }

    return NULL;
}

static int32
status_full_event_mask(void) {
    return MPD_IDLE_DATABASE | MPD_IDLE_STORED_PLAYLIST | MPD_IDLE_PLAYLIST
           | MPD_IDLE_PLAYER | MPD_IDLE_MIXER | MPD_IDLE_OUTPUT
           | MPD_IDLE_UPDATE | MPD_IDLE_OPTIONS;
}

void
ncm_status_set_database_update_observer(void (*callback)(void *user),
                                        void *user) {
    status_database_update_observer = callback;
    status_database_update_observer_user = user;
    return;
}

void
ncm_status_set_playlist_update_observer(void (*callback)(void *user),
                                        void *user) {
    status_playlist_update_observer = callback;
    status_playlist_update_observer_user = user;
    return;
}

static void
status_update_timeout_from_screen(NcScreen *screen, void *user) {
    StatusTimeoutContext *context = user;
    int32 timeout = nc_screen_window_timeout(screen);

    if (timeout < context->timeout) {
        context->timeout = timeout;
    }
    return;
}

static void
status_refresh_footer(NcmStatusHooks *hooks) {
    NcWindow *footer;

    if (hooks && hooks->refresh_footer) {
        hooks->refresh_footer(hooks->user);
        return;
    }

    if ((footer = ui_state_footer_window())) {
        nc_window_refresh(footer);
    }
    return;
}

static void
status_print_value(char *prefix, int32 prefix_len,
                   char *value, int32 value_len) {
    StrBuilder message = {0};

    if (value_len < 0) {
        value_len = optional_strlen32(value);
    }

    SB_APPEND(&message, prefix, prefix_len);
    SB_APPEND(&message, value, value_len);
    ncm_statusbar_print(Config.message_delay_time, message.data, message.len);
    sb_free(&message);
    return;
}

static void
status_print_client_error(char *message, int32 message_len) {
    status_print_value(STRLIT("ncmpcpp: "), message, message_len);
    return;
}

static void
status_print_server_error(char *message, int32 message_len) {
    status_print_value(STRLIT("MPD: "), message, message_len);
    return;
}

void
ncm_status_handle_server_error_value(NcmMpdClient *client, int32 code,
                                     char *message, int32 message_len) {
    status_print_server_error(message, message_len);
    if ((code == MPD_SERVER_ERROR_PERMISSION) && (client != NULL)) {
        enum NcPromptStatus prompt_status;
        NcPrompt prompt = {0};
        NcWindow *window;
        char *password;
        bool password_allocated;

        if ((window = ncm_statusbar_put()) == NULL) {
            return;
        }

        nc_window_print_cstring(window, "Password: ");
        prompt.initial_text = "";
        prompt.width = -1;
        prompt.encrypted = true;
        prompt.remember = false;

        password = NULL;
        prompt_status = nc_window_prompt(window, &prompt, &password);
        if (prompt_status != NC_PROMPT_ACCEPTED) {
            nc_window_prompt_result_destroy(password);
            ncm_action_runtime_request_exit(NULL);
            return;
        }

        password_allocated = password;
        if (password == NULL) {
            password = "";
        }

        ncm_mpd_client_set_password(client, password, -1, NULL);
        if (password_allocated) {
            nc_window_prompt_result_destroy(password);
        }

        if (ncm_mpd_client_send_password(client, NULL) < 0) {
            if (ncm_mpd_client_error_code(client) == MPD_ERROR_SERVER) {
                status_print_server_error(
                    ncm_mpd_client_error_message(client), -1);
            } else {
                if (!ncm_mpd_client_error_is_clearable(client)) {
                    ncm_mpd_client_disconnect(client);
                }
                status_print_client_error(
                    ncm_mpd_client_error_message(client), -1);
            }
            return;
        }

        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Password accepted"));
    }
    return;
}

void
ncm_status_trace(NcmMpdClient *client, bool update_timer,
                 bool update_window_timeout, NcmError *ncm_error) {
    StatusTimeoutContext timeout_context = {0};
    NcmStatusHooks *hooks;
    NcWindow *footer;

    hooks = status_active_hooks(NULL);

    if (update_timer) {
        global_timer_update();
    }

    if (client && ncm_mpd_client_is_connected(client)) {
        if (!status_initialized) {
            NcmMpdStatus mpd_status = {0};
            NcmStatusInitHooks *init_hooks = NULL;

            if (ncm_mpd_client_get_status(client, &mpd_status,
                                          ncm_error) >= 0) {
                ncm_status_apply_mpd_status(
                    &mpd_status, status_full_event_mask(), NULL, ncm_error);

                if (status_init_hooks_set) {
                    init_hooks = &status_init_hooks;
                }

                if (init_hooks && init_hooks->jump_to_now_playing) {
                    init_hooks->jump_to_now_playing(init_hooks->user);
                } else if (Config.jump_to_now_playing_song_at_start) {
                    int32 position = status_current_song_pos;

                    if (position >= 0) {
                        bool highlighted;

                        highlighted = playlist_screen_locate_position(
                            app_screen_playlist(), position) > 0;
                        if (!highlighted) {
                            ncm_statusbar_print_cstring(
                                Config.message_delay_time,
                                "Song is filtered out");
                        }

                        {
                            NcScreen *playlist_screen2
                                = app_screen_playlist_base();

                            if (highlighted
                                && app_controller_is_screen_visible(
                                    playlist_screen2)) {
                                nc_screen_refresh(playlist_screen2);
                            }
                        }
                    }
                }

                if (init_hooks && init_hooks->set_tcp_nodelay) {
                    init_hooks->set_tcp_nodelay(init_hooks->user);
                } else {
#if defined(HAVE_NETINET_IN_H) && defined(HAVE_NETINET_TCP_H)
                    int32 fd;

                    if ((fd = ncm_mpd_client_fd(&global_mpd)) >= 0) {
                        int32 flag = 1;

                        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag,
                                   (socklen_t)SIZEOF(flag));
                    }
#endif
                }

                if (init_hooks
                    && init_hooks->load_browser_supported_extensions) {
                    init_hooks->load_browser_supported_extensions(
                        init_hooks->user);
                } else {
                    app_screen_browser_fetch_supported_extensions();
                }

                if (init_hooks && init_hooks->fetch_outputs) {
                    init_hooks->fetch_outputs(init_hooks->user);
                } else {
#if defined(ENABLE_OUTPUTS)
                    app_screen_outputs_fetch_list();
#endif
                }

                if (init_hooks && init_hooks->setup_visualizer_datasource) {
                    init_hooks->setup_visualizer_datasource(init_hooks->user);
                } else {
#if defined(ENABLE_VISUALIZER)
                    VisualizerScreen *visualizer2 = app_screen_visualizer();

                    visualizer_screen_close_data_source(visualizer2);
                    visualizer_screen_open_data_source(visualizer2);
                    visualizer_screen_find_output_id(visualizer2);
#endif
                }

                if (init_hooks && init_hooks->register_mpd_fd_callback) {
                    init_hooks->register_mpd_fd_callback(init_hooks->user);
                } else {
                    NcWindow *callback_footer = ui_state_footer_window();
                    int32 fd;

                    if ((callback_footer != NULL)
                        && ((fd = ncm_mpd_client_fd(&global_mpd)) >= 0)) {
                        nc_window_add_fd_callback(
                            callback_footer, fd,
                            ncm_statusbar_mpd_idle_callback);
                    }
                }

                if (init_hooks && init_hooks->show_connected_message) {
                    init_hooks->show_connected_message(init_hooks->user);
                } else if (Config.connected_message_on_startup) {
                    status_print_value(
                        STRLIT("Connected to "),
                        ncm_mpd_client_hostname(&global_mpd), -1);
                }
            }
            hooks = status_active_hooks(NULL);
        }

        if ((status_player_state == NCM_STATUS_PLAYER_PLAY)
            && (global_timer_elapsed_ms(status_past) > 1000)) {
            if (hooks && hooks->elapsed_time_changed) {
                hooks->elapsed_time_changed(true, hooks->user);
            } else {
                ncm_status_changes_elapsed_time(true);
            }
            status_refresh_footer(hooks);
            status_past = global_timer;
        }

        app_controller_update_visible_screens();
        ncm_statusbar_try_redraw();
        ncm_mpd_client_idle(client, ncm_error);
    }

    if (update_window_timeout) {
        timeout_context.timeout = INT_MAX;
        app_controller_each_visible_screen(status_update_timeout_from_screen,
                                           &timeout_context);

        if ((footer = ui_state_footer_window())) {
            nc_window_set_timeout(footer, timeout_context.timeout);
        }
    }
    return;
}

static void
status_notify_statusbar(void) {
    if (status_notification_observer) {
        status_notification_observer(status_notification_observer_user);
    }
    return;
}

static void
statusbar_print_cstring_value(char *prefix, int32 prefix_len, char *value) {
    status_notify_statusbar();
    status_print_value(prefix, prefix_len, value, -1);
    return;
}

static char *
status_on_off(char status) {
    if (status == 0) {
        return "off";
    }

    return "on";
}

static int64
status_elapsed_time_ms_now(void) {
    int64 elapsed;
    int64 total;
    int64 delta;

    elapsed = status_elapsed_time_ms;
    if (status_player_state == NCM_STATUS_PLAYER_PLAY) {
        delta = global_timer_elapsed_ms(status_elapsed_time_updated_at);
        if (delta > 0) {
            elapsed += delta;
        }
    }

    total = (int64)status_total_time*STATUS_MILLISECONDS_PER_SECOND;
    if ((total > 0) && (elapsed > total)) {
        elapsed = total;
    }
    return elapsed;
}

static void
status_rebase_elapsed_time(int32 elapsed_time, int64 elapsed_time_ms) {
    if (elapsed_time < 0) {
        elapsed_time = 0;
    }
    if ((elapsed_time_ms <= 0) && (elapsed_time > 0)) {
        elapsed_time_ms = (int64)elapsed_time*STATUS_MILLISECONDS_PER_SECOND;
    }

    status_elapsed_time = elapsed_time;
    status_elapsed_time_ms = elapsed_time_ms;
    status_elapsed_time_updated_at = global_timer;
    return;
}

int32
ncm_status_apply_mpd_status(NcmMpdStatus *mpd_status, int32 event,
                            NcmStatusHooks *hooks, NcmError *ncm_error) {
    int32 previous_playlist_version;
    char new_consume;
    char new_crossfade;
    char new_random;
    char new_repeat;
    char new_single;
    NcmStatusHooks *active_hooks;

    if (mpd_status == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("MPD status is NULL"));
    }

    active_hooks = status_active_hooks(hooks);

    status_current_song_pos = mpd_status->song_pos;
    switch (mpd_status->state) {
    case MPD_STATE_STOP:
        status_player_state = NCM_STATUS_PLAYER_STOP;
        break;
    case MPD_STATE_PLAY:
        status_player_state = NCM_STATUS_PLAYER_PLAY;
        break;
    case MPD_STATE_PAUSE:
        status_player_state = NCM_STATUS_PLAYER_PAUSE;
        break;
    case MPD_STATE_UNKNOWN:
    default:
        status_player_state = NCM_STATUS_PLAYER_UNKNOWN;
        break;
    }
    status_rebase_elapsed_time(mpd_status->elapsed_time,
                               mpd_status->elapsed_time_ms);
    status_kbps = mpd_status->kbit_rate;
    status_playlist_length = mpd_status->queue_length;
    status_total_time = mpd_status->total_time;
    status_volume = mpd_status->volume;

    if ((event & MPD_IDLE_DATABASE) != 0) {
        if (active_hooks
            && active_hooks->database_changed) {
            active_hooks->database_changed(active_hooks->user);
        } else {
            browser_screen_request_update(app_screen_browser());
#if defined(HAVE_TAGLIB_H)
            tag_editor_screen_clear_directories(app_screen_tag_editor());
#endif
            media_library_screen_request_tags_update(
                app_screen_media_library());
            media_library_screen_request_albums_update(
                app_screen_media_library());
            media_library_screen_request_songs_update(
                app_screen_media_library());
            if (status_database_update_observer) {
                status_database_update_observer(
                    status_database_update_observer_user);
            }
            if (status_ui_hooks_set && status_ui_hooks.database_changed) {
                status_ui_hooks.database_changed(status_ui_hooks.user);
            }
        }
    }

    if ((event & MPD_IDLE_STORED_PLAYLIST) != 0) {
        if (active_hooks
            && active_hooks->stored_playlists_changed) {
            active_hooks->stored_playlists_changed(active_hooks->user);
        } else {
            BrowserScreen *browser;
            PlaylistEditorScreen *editor;

            editor = app_screen_playlist_editor();
            playlist_editor_screen_request_playlists_update(editor);
            playlist_editor_screen_request_content_update(editor);

            if ((browser = app_screen_browser())
                && !browser_screen_is_local(browser)
                && browser_screen_is_in_root_directory(browser)) {
                browser_screen_request_update(browser);
            }
            if (status_ui_hooks_set
                && status_ui_hooks.stored_playlists_changed) {
                status_ui_hooks.stored_playlists_changed(
                    status_ui_hooks.user);
            }
        }
    }

    if ((event & MPD_IDLE_PLAYLIST) != 0) {
        previous_playlist_version = status_playlist_version;
        status_playlist_version = mpd_status->queue_version;
        if (active_hooks
            && active_hooks->playlist_changed) {
            active_hooks->playlist_changed(previous_playlist_version,
                                           active_hooks->user);
        } else {
            NcmError playlist_error = {0};

            if (playlist_screen_reload_from_mpd(
                app_screen_playlist(), &global_mpd, previous_playlist_version,
                status_playlist_length, &playlist_error) < 0) {
                ncm_statusbar_print_cstring(Config.message_delay_time,
                                            playlist_error.message);
            } else if (status_playlist_update_observer) {
                status_playlist_update_observer(
                    status_playlist_update_observer_user);
            }
            if (status_ui_hooks_set && status_ui_hooks.playlist_changed) {
                status_ui_hooks.playlist_changed(previous_playlist_version,
                                                 status_ui_hooks.user);
            }
        }
    }

    if ((event & MPD_IDLE_PLAYER) != 0) {
        if (active_hooks
            && active_hooks->player_state_changed) {
            active_hooks->player_state_changed(active_hooks->user);
        } else {
            ncm_status_changes_player_state();
        }

        if (status_current_song_id != mpd_status->song_id) {
            if (active_hooks
                && active_hooks->song_id_changed) {
                active_hooks->song_id_changed(mpd_status->song_id,
                                              active_hooks->user);
            } else {
                NcmSong song = {0};
                bool has_song;

                playlist_screen_reload_remaining(app_screen_playlist());
                status_playing_song_scroll_begin = 0;
                status_first_line_scroll_begin = 0;
                status_second_line_scroll_begin = 0;
#if defined(ENABLE_VISUALIZER)
                visualizer_screen_reset_auto_scale_multiplier(
                    app_screen_visualizer());
#endif
                if (status_ui_hooks_set
                    && status_ui_hooks.song_id_changed) {
                    status_ui_hooks.song_id_changed(
                        mpd_status->song_id, status_ui_hooks.user);
                }

                if (status_player_state != NCM_STATUS_PLAYER_STOP) {
                    has_song = false;
                    if (playlist_screen_now_playing_song(
                        app_screen_playlist(), status_current_song_pos,
                        &song) == 0) {
                        has_song = true;
                    } else if (ncm_mpd_client_is_connected(&global_mpd)) {
                        if (ncm_mpd_client_get_current_song(
                            &global_mpd, &song, NULL) >= 0) {
                            has_song = !ncm_song_is_empty(&song);
                        }
                    }

                    if (has_song) {
                        if (!ncm_song_is_empty(&song)) {
                            if (Config.execute_on_song_change_len > 0) {
                                ncm_run_external_command(
                                    Config.execute_on_song_change,
                                    Config.execute_on_song_change_len, true,
                                    NULL);
                            }

                            if (Config.fetch_lyrics_for_current_song_in_background) {
                                lyrics_screen_fetch_in_background(
                                    app_screen_lyrics(), &song, false, NULL);
                            }

                            if (Config.autocenter_mode) {
                                playlist_screen_locate_position(
                                    app_screen_playlist(),
                                    ncm_song_position(&song));
                            }

                            if (Config.follow_now_playing_lyrics
                                && app_controller_is_screen_visible(
                                    app_screen_lyrics_base())
                                && (app_controller_previous_screen()
                                    == app_screen_playlist_base())) {
                                lyrics_screen_fetch(
                                    app_screen_lyrics(), &song, NULL, NULL);
                            }

                            if (status_ui_hooks_set
                                && status_ui_hooks.current_song_changed) {
                                status_ui_hooks.current_song_changed(
                                    &song, status_ui_hooks.user);
                            }
                        }
                        status_draw_song_title(&song);
                    }

                    ncm_song_destroy(&song);
                }

                status_current_song_id = mpd_status->song_id;
                ncm_status_changes_elapsed_time(false);
            }
            status_current_song_id = mpd_status->song_id;
        }
    }

    if ((event & MPD_IDLE_MIXER) != 0) {
        if (active_hooks && active_hooks->mixer_changed) {
            active_hooks->mixer_changed(active_hooks->user);
        } else {
            ncm_status_changes_mixer();
        }
    }

    if ((event & MPD_IDLE_OUTPUT) != 0) {
        if (active_hooks && active_hooks->outputs_changed) {
            active_hooks->outputs_changed(active_hooks->user);
        } else {
#if ENABLE_OUTPUTS
            app_screen_outputs_fetch_list();
            app_screen_outputs_refresh_if_visible();
#endif
        }
    }

    if ((event & MPD_IDLE_UPDATE) != 0) {
        status_db_updating = 0;
        if (mpd_status->update_id != 0) {
            status_db_updating = 'U';
        }

        if (status_initialized) {
            if (status_db_updating) {
                statusbar_print_cstring_value(STRLIT("Database update "),
                                              "started");
            } else {
                statusbar_print_cstring_value(STRLIT("Database update "),
                                              "finished");
            }
        }
    }

    if ((event & MPD_IDLE_OPTIONS) != 0) {
        new_repeat = 0;
        if (mpd_status->repeat) {
            new_repeat = 'r';
        }
        if (new_repeat != status_repeat) {
            status_repeat = new_repeat;
            if (status_initialized) {
                statusbar_print_cstring_value(STRLIT("Repeat mode is "),
                                              status_on_off(status_repeat));
            }
        }

        new_random = 0;
        if (mpd_status->random) {
            new_random = 'z';
        }
        if (new_random != status_random) {
            status_random = new_random;
            if (status_initialized) {
                statusbar_print_cstring_value(STRLIT("Random mode is "),
                                              status_on_off(status_random));
            }
        }

        new_single = 0;
        if (mpd_status->single) {
            new_single = 's';
        }
        if (new_single != status_single) {
            status_single = new_single;
            if (status_initialized) {
                statusbar_print_cstring_value(STRLIT("Single mode is "),
                                              status_on_off(status_single));
            }
        }

        new_consume = 0;
        if (mpd_status->consume) {
            new_consume = 'c';
        }
        if (new_consume != status_consume) {
            status_consume = new_consume;
            if (status_initialized) {
                statusbar_print_cstring_value(STRLIT("Consume mode is "),
                                              status_on_off(status_consume));
            }
        }

        new_crossfade = 0;
        if (mpd_status->crossfade != 0) {
            new_crossfade = 'x';
        }
        if (new_crossfade != status_crossfade) {
            status_crossfade = new_crossfade;
            if (status_initialized) {
                StrBuilder message = {0};

                status_notify_statusbar();
                sb_printf(&message, "Crossfade set to %u seconds",
                          (uint32)mpd_status->crossfade);
                ncm_statusbar_print(Config.message_delay_time,
                                    message.data, message.len);
                sb_free(&message);
            }
        }
    }

    if ((event & (MPD_IDLE_UPDATE | MPD_IDLE_OPTIONS)) != 0) {
        if (active_hooks && active_hooks->flags_changed) {
            active_hooks->flags_changed(active_hooks->user);
        } else {
            ncm_status_changes_flags();
        }
    }

    status_initialized = true;

    if ((event & MPD_IDLE_PLAYER) != 0) {
        status_refresh_footer(active_hooks);
    }

    if ((event & (MPD_IDLE_PLAYLIST | MPD_IDLE_DATABASE | MPD_IDLE_PLAYER))) {
        if (active_hooks && active_hooks->refresh_visible_screens) {
            active_hooks->refresh_visible_screens(active_hooks->user);
        } else {
            app_controller_refresh_visible_screens();
        }
    }

    return ncm_error_ok(ncm_error);
}

int32
ncm_status_update(NcmMpdClient *client, int32 event, NcmError *ncm_error) {
    NcmMpdStatus mpd_status;
    int32 status;

    if (client == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("MPD client is NULL"));
    }

    status = ncm_mpd_client_get_status(client, &mpd_status, ncm_error);
    if (status < 0) {
        return status;
    }
    status_reset_visualizer_for_player_event(event);

    return ncm_status_apply_mpd_status(&mpd_status, event, NULL, ncm_error);
}

int32
ncm_status_update_full(NcmMpdClient *client, NcmStatusHooks *hooks,
                       NcmError *ncm_error) {
    NcmMpdStatus mpd_status;
    int32 status;

    if (client == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("MPD client is NULL"));
    }

    status = ncm_mpd_client_get_status(client, &mpd_status, ncm_error);
    if (status < 0) {
        return status;
    }

    return ncm_status_apply_mpd_status(&mpd_status, status_full_event_mask(),
                                       hooks, ncm_error);
}

int32
ncm_status_update_from_noidle(NcmMpdClient *client, NcmStatusHooks *hooks,
                              NcmError *ncm_error) {
    NcmMpdStatus mpd_status;
    int32 flags;
    int32 status;

    if (client == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("MPD client is NULL"));
    }

    flags = 0;
    status = ncm_mpd_client_noidle(client, &flags, ncm_error);
    if (status < 0) {
        return status;
    }

    status = ncm_mpd_client_get_status(client, &mpd_status, ncm_error);
    if (status < 0) {
        return status;
    }
    status_reset_visualizer_for_player_event(flags);

    return ncm_status_apply_mpd_status(&mpd_status, flags, hooks, ncm_error);
}

void
ncm_status_clear(void) {
    status_initialized = false;
    status_consume = 0;
    status_crossfade = 0;
    status_db_updating = 0;
    status_repeat = 0;
    status_random = 0;
    status_single = 0;
    status_current_song_id = -1;
    status_current_song_pos = -1;
    status_elapsed_time = 0;
    status_elapsed_time_ms = 0;
    status_elapsed_time_updated_at = global_timer;
    status_kbps = 0;
    status_player_state = NCM_STATUS_PLAYER_UNKNOWN;
    status_playlist_version = 0;
    status_playlist_length = 0;
    status_total_time = 0;
    status_volume = -1;
    return;
}

bool
ncm_status_state_consume_is_enabled(void) {
    return status_consume != 0;
}

bool
ncm_status_state_crossfade_is_enabled(void) {
    return status_crossfade != 0;
}

bool
ncm_status_state_repeat_is_enabled(void) {
    return status_repeat != 0;
}

bool
ncm_status_state_random_is_enabled(void) {
    return status_random != 0;
}

bool
ncm_status_state_single_is_enabled(void) {
    return status_single != 0;
}

int32
ncm_status_state_current_song_position(void) {
    return status_current_song_pos;
}

int32
ncm_status_state_playlist_length(void) {
    return status_playlist_length;
}

int32
ncm_status_state_elapsed_time(void) {
    return status_elapsed_time;
}

int64
ncm_status_state_elapsed_time_ms(void) {
    return status_elapsed_time_ms_now();
}

enum NcmStatusPlayerState
ncm_status_state_player(void) {
    return status_player_state;
}

int32
ncm_status_state_total_time(void) {
    return status_total_time;
}

int32
ncm_status_state_volume(void) {
    return status_volume;
}

void
ncm_status_changes_player_state(void) {
    NcmSong song = {0};
    char player_state[32];
    int32 player_state_len;
    NcWindow *header;
    NcWindow *state_window;

    if (Config.execute_on_player_state_change_len > 0) {
        char *player_state_env = "unknown";

        switch (status_player_state) {
        case NCM_STATUS_PLAYER_PLAY:
            player_state_env = "play";
            break;
        case NCM_STATUS_PLAYER_STOP:
            player_state_env = "stop";
            break;
        case NCM_STATUS_PLAYER_PAUSE:
            player_state_env = "pause";
            break;
        case NCM_STATUS_PLAYER_UNKNOWN:
        default:
            break;
        }

        setenv("MPD_PLAYER_STATE", player_state_env, 1);
        ncm_run_external_command(
            Config.execute_on_player_state_change,
            Config.execute_on_player_state_change_len, true, NULL);
        unsetenv("MPD_PLAYER_STATE");
    }

    if (status_ui_hooks_set && status_ui_hooks.player_state_changed) {
        status_ui_hooks.player_state_changed(status_player_state,
                                             status_ui_hooks.user);
    }

    switch (status_player_state) {
    case NCM_STATUS_PLAYER_PLAY:
        if (playlist_screen_now_playing_song(
            app_screen_playlist(), status_current_song_pos, &song) == 0) {
            status_draw_song_title(&song);
        }
        ncm_song_destroy(&song);
        playlist_screen_reload_remaining(app_screen_playlist());
        break;
    case NCM_STATUS_PLAYER_STOP:
        ncm_window_title_set(STRLIT("ncmpcpp " VERSION));
        if (ncm_progressbar_is_unlocked()) {
            ncm_progressbar_draw(0, 0);
        }
        playlist_screen_reload_remaining(app_screen_playlist());
        if (Config.user_interface == NCM_DESIGN_ALTERNATIVE) {
            if ((header = ui_state_header_window())) {
                nc_window_go_to_xy(header, 0, 0);
                nc_window_apply_term_manip(header, NC_TERM_CLEAR_TO_EOL);
                nc_window_go_to_xy(header, 0, 1);
                nc_window_apply_term_manip(header, NC_TERM_CLEAR_TO_EOL);
            }
            ncm_status_changes_mixer();
            ncm_status_changes_flags();
        }
        if (status_ui_hooks_set && status_ui_hooks.player_stopped) {
            status_ui_hooks.player_stopped(status_ui_hooks.user);
        }
#if defined(ENABLE_VISUALIZER)
        {
            NcScreen *visualizer = app_screen_visualizer_base();

            if (app_controller_is_screen_visible(visualizer)) {
                visualizer_screen_clear(app_screen_visualizer());
            }
        }
#endif
        break;
    case NCM_STATUS_PLAYER_PAUSE:
    case NCM_STATUS_PLAYER_UNKNOWN:
        break;
    default:
        break;
    }

    player_state_len = status_player_state_string(player_state,
                                                  SIZEOF(player_state));
    switch (Config.user_interface) {
    case NCM_DESIGN_ALTERNATIVE:
        if ((state_window = ui_state_header_window())) {
            nc_window_go_to_xy(state_window, 0, 1);
            nc_window_apply_format(state_window, NC_FORMAT_BOLD);
            nc_window_print_data(state_window, player_state,
                                 player_state_len);
            nc_window_apply_format(state_window, NC_FORMAT_NO_BOLD);
            nc_window_refresh(state_window);
        }
        break;
    case NCM_DESIGN_CLASSIC:
        state_window = ui_state_footer_window();
        if ((state_window != NULL)
            && ncm_statusbar_is_unlocked()
            && Config.statusbar_visibility) {
            nc_window_go_to_xy(state_window, 0, 1);
            if (player_state_len == 0) {
                nc_window_apply_term_manip(state_window,
                                           NC_TERM_CLEAR_TO_EOL);
            } else {
                nc_window_apply_format(state_window, NC_FORMAT_BOLD);
                nc_window_print_data(state_window, player_state,
                                     player_state_len);
                nc_window_apply_format(state_window, NC_FORMAT_NO_BOLD);
            }
        }
        break;
    case NCM_DESIGN_COUNT:
        break;
    default:
        break;
    }
    ncm_status_changes_elapsed_time(false);
    return;
}

static void
status_apply_formatted_color(NcWindow *window, NcFormattedColor *color) {
    enum NcFormat *formats;
    int32 count;

    nc_window_push_color(window, color->color);
    formats = nc_formatted_color_formats(color);
    count = nc_formatted_color_format_count(color);
    for (int32 i = 0; i < count; i += 1) {
        nc_window_apply_format(window, formats[i]);
    }
    return;
}

static void
status_apply_formatted_color_end(NcWindow *window, NcFormattedColor *color) {
    enum NcFormat *formats;
    int32 count;

    if (!nc_color_is_default(color->color)) {
        nc_window_push_color(window, nc_color_end());
    }
    formats = nc_formatted_color_formats(color);
    count = nc_formatted_color_format_count(color);
    for (int32 i = count - 1; i >= 0; i -= 1) {
        nc_window_apply_format(window, nc_format_reverse(formats[i]));
    }
    return;
}

static int32
status_song_time_string(int32 length, char *buffer, int32 buffer_cap) {
    return ncm_helpers_show_song_time(length, buffer, buffer_cap);
}

static int32
status_player_state_string(char *buffer, int32 buffer_cap) {
    char *string = "";
    int32 len;

    switch (status_player_state) {
    case NCM_STATUS_PLAYER_UNKNOWN:
        if (Config.user_interface == NCM_DESIGN_ALTERNATIVE) {
            string = "[unknown]";
        }
        break;
    case NCM_STATUS_PLAYER_PLAY:
        if (Config.user_interface == NCM_DESIGN_ALTERNATIVE) {
            string = "[playing]";
        } else {
            string = "Playing:";
        }
        break;
    case NCM_STATUS_PLAYER_PAUSE:
        if (Config.user_interface == NCM_DESIGN_ALTERNATIVE) {
            string = "[paused]";
        } else {
            string = "Paused:";
        }
        break;
    case NCM_STATUS_PLAYER_STOP:
        if (Config.user_interface == NCM_DESIGN_ALTERNATIVE) {
            string = "[stopped]";
        }
        break;
    default:
        break;
    }

    len = optional_strlen32(string);
    if (len >= buffer_cap) {
        len = buffer_cap - 1;
    }
    if (len > 0) {
        memcpy64(buffer, string, len);
    }
    buffer[len] = 0;
    return len;
}

static void
status_draw_song_title(NcmSong *song) {
    StrBuilder title;

    title = ncm_format_render_string(&Config.song_window_title_format, song);
    ncm_window_title_set(title.data, title.len);
    sb_free(&title);
    return;
}

static void
status_reset_visualizer_for_player_event(int32 event) {
#if defined(ENABLE_VISUALIZER)
    if ((event & MPD_IDLE_PLAYER) != 0) {
        visualizer_screen_reset_audio_state(
            app_screen_visualizer());
    }
#else
    (void)event;
#endif
    return;
}

static void
status_tracklength_buffer(StrBuilder *buffer) {
    char time_buffer[64];
    int32 time_len;

    sb_clear(buffer);
    if ((Config.display_bitrate) && (status_kbps != 0)
        && (Config.user_interface == NCM_DESIGN_CLASSIC)) {
        sb_append_byte(buffer, '(');
        sb_printf(buffer, "%d", status_kbps);
        SB_APPEND(buffer, " kbps) ");
    }

    if (Config.user_interface == NCM_DESIGN_CLASSIC) {
        sb_append_byte(buffer, '[');
    }

    if ((Config.display_remaining_time) && (status_total_time != 0)) {
        sb_append_byte(buffer, '-');
        if (status_elapsed_time < status_total_time) {
            time_len = status_song_time_string(
                status_total_time - status_elapsed_time, time_buffer,
                SIZEOF(time_buffer));
        } else {
            time_len = status_song_time_string(0, time_buffer,
                                               SIZEOF(time_buffer));
        }
    } else {
        time_len = status_song_time_string(status_elapsed_time, time_buffer,
                                           SIZEOF(time_buffer));
    }
    SB_APPEND(buffer, time_buffer, time_len);

    if (status_total_time != 0) {
        sb_append_byte(buffer, '/');
        time_len = status_song_time_string(status_total_time, time_buffer,
                                           SIZEOF(time_buffer));
        SB_APPEND(buffer, time_buffer, time_len);
    }

    if (Config.user_interface == NCM_DESIGN_CLASSIC) {
        sb_append_byte(buffer, ']');
    } else if ((Config.display_bitrate) && (status_kbps != 0)) {
        SB_APPEND(buffer, " (");
        sb_printf(buffer, "%d", status_kbps);
        SB_APPEND(buffer, " kbps)");
    }
    return;
}

void
ncm_status_changes_elapsed_time(bool update_elapsed) {
    NcmSong song = {0};
    NcWindow *footer;
    NcWindow *header;
    char player_state[32];
    int32 player_state_len;

    if (update_elapsed) {
        NcmMpdStatus mpd_status = {0};
        int64 elapsed_ms;

        if (!ncm_mpd_client_is_connected(&global_mpd)) {
            elapsed_ms = status_elapsed_time_ms_now();
            status_rebase_elapsed_time(status_elapsed_time + 1, elapsed_ms);
        } else {
            if (ncm_mpd_client_get_status(&global_mpd, &mpd_status,
                                          NULL) < 0) {
                elapsed_ms = status_elapsed_time_ms_now();
                status_rebase_elapsed_time(status_elapsed_time + 1,
                                           elapsed_ms);
            } else {
                status_rebase_elapsed_time(mpd_status.elapsed_time,
                                           mpd_status.elapsed_time_ms);
                status_kbps = mpd_status.kbit_rate;
            }
        }
    }

    if ((status_player_state == NCM_STATUS_PLAYER_STOP)
        || (status_player_state == NCM_STATUS_PLAYER_UNKNOWN)) {
        if ((footer = ui_state_footer_window()) && ncm_statusbar_is_unlocked()
            && Config.statusbar_visibility) {
            nc_window_go_to_xy(footer, 0, 1);
            nc_window_apply_term_manip(footer, NC_TERM_CLEAR_TO_EOL);
        }
        if (ncm_progressbar_is_unlocked()) {
            ncm_progressbar_draw(0, 0);
        }
        return;
    }

    if (playlist_screen_now_playing_song(
        app_screen_playlist(), status_current_song_pos, &song) < 0) {
        ncm_song_destroy(&song);
        if ((footer = ui_state_footer_window()) && ncm_statusbar_is_unlocked()
            && Config.statusbar_visibility) {
            nc_window_go_to_xy(footer, 0, 1);
            nc_window_apply_term_manip(footer, NC_TERM_CLEAR_TO_EOL);
        }
        if (ncm_progressbar_is_unlocked()) {
            ncm_progressbar_draw(0, 0);
        }
        return;
    }

    player_state_len
        = status_player_state_string(player_state, SIZEOF(player_state));
    status_draw_song_title(&song);

    switch (Config.user_interface) {
    case NCM_DESIGN_CLASSIC:
        footer = ui_state_footer_window();
        if ((footer != NULL)
            && Config.statusbar_visibility
            && ncm_statusbar_is_unlocked()) {
            NcBuffer rendered_song = {0};
            StrBuilder tracklength = {0};
            int32 text_width;
            int32 track_x;
            char separator[] = " ** ";

            status_tracklength_buffer(&tracklength);
            ncm_format_render_buffer(&Config.song_status_format, &song,
                                     &rendered_song, &rendered_song,
                                     NCM_FORMAT_FLAG_ALL);

            nc_window_go_to_xy(footer, 0, 1);
            nc_window_apply_term_manip(footer, NC_TERM_CLEAR_TO_EOL);
            status_apply_formatted_color(footer,
                                         &Config.player_state_color);
            nc_window_print_data(footer, player_state, player_state_len);
            status_apply_formatted_color_end(footer,
                                             &Config.player_state_color);
            nc_window_print_char(footer, ' ');

            text_width = nc_window_width(footer) - player_state_len;
            text_width -= tracklength.len;
            text_width -= 2;
            if (text_width < 0) {
                text_width = 0;
            }
            nc_cyclic_buffer_write(
                &rendered_song, footer, &status_playing_song_scroll_begin,
                text_width, separator, STRLIT_LEN(" ** "));

            track_x = nc_window_width(footer) - tracklength.len;
            if (track_x < 0) {
                track_x = 0;
            }
            nc_window_go_to_xy(footer, track_x, 1);
            status_apply_formatted_color(footer,
                                         &Config.statusbar_time_color);
            nc_window_print_data(footer, tracklength.data, tracklength.len);
            status_apply_formatted_color_end(footer,
                                             &Config.statusbar_time_color);

            sb_free(&tracklength);
            nc_buffer_destroy(&rendered_song);
        }
        break;
    case NCM_DESIGN_ALTERNATIVE:
        header = ui_state_header_window();
        if (header != NULL) {
            NcBuffer first = {0};
            NcBuffer second = {0};
            StrBuilder tracklength = {0};
            int32 first_len;
            int32 first_margin;
            int32 first_start;
            int32 second_len;
            int32 second_margin;
            int32 second_start;
            int32 text_width;
            int32 volume_x;
            char separator[] = " ** ";

            status_tracklength_buffer(&tracklength);

            ncm_format_render_buffer(
                &Config.alternative_header_first_line_format, &song,
                &first, &first, NCM_FORMAT_FLAG_ALL);
            ncm_format_render_buffer(
                &Config.alternative_header_second_line_format, &song,
                &second, &second, NCM_FORMAT_FLAG_ALL);

            first_len = utf8_width(first.data, first.len);
            first_margin = tracklength.len + 1;
            if (first_margin < (global_volume_state_len() + 1)) {
                first_margin = global_volume_state_len() + 1;
            }
            first_margin *= 2;
            first_start = tracklength.len + 1;
            if (first_len < (COLS - first_margin)) {
                first_start = (COLS - first_len) / 2;
            }

            second_len = utf8_width(second.data, second.len);
            second_margin = player_state_len;
            if (second_margin < 8) {
                second_margin = 8;
            }
            second_margin = (second_margin + 1)*2;
            second_start = player_state_len + 1;
            if (second_len < (COLS - second_margin)) {
                second_start = (COLS - second_len) / 2;
            }

            if (!global_seeking_in_progress) {
                nc_window_go_to_xy(header, 0, 0);
                nc_window_apply_term_manip(header, NC_TERM_CLEAR_TO_EOL);
                status_apply_formatted_color(
                    header, &Config.statusbar_time_color);
                nc_window_print_data(header, tracklength.data,
                                     tracklength.len);
                status_apply_formatted_color_end(
                    header, &Config.statusbar_time_color);
            }

            nc_window_go_to_xy(header, first_start, 0);
            text_width = COLS - tracklength.len
                         - global_volume_state_len() - 1;
            if (text_width < 0) {
                text_width = 0;
            }
            nc_cyclic_buffer_write(
                &first, header, &status_first_line_scroll_begin, text_width,
                separator, STRLIT_LEN(" ** "));

            nc_window_go_to_xy(header, 0, 1);
            nc_window_apply_term_manip(header, NC_TERM_CLEAR_TO_EOL);
            status_apply_formatted_color(header,
                                         &Config.player_state_color);
            nc_window_print_data(header, player_state, player_state_len);
            status_apply_formatted_color_end(header,
                                             &Config.player_state_color);
            nc_window_go_to_xy(header, second_start, 1);

            text_width = COLS - player_state_len - 10;
            if (text_width < 0) {
                text_width = 0;
            }
            nc_cyclic_buffer_write(
                &second, header, &status_second_line_scroll_begin, text_width,
                separator, STRLIT_LEN(" ** "));

            volume_x = nc_window_width(header) - global_volume_state_len();
            if (volume_x < 0) {
                volume_x = 0;
            }
            nc_window_go_to_xy(header, volume_x, 0);
            status_apply_formatted_color(header, &Config.volume_color);
            nc_window_print_data(header, global_volume_state_cstr(),
                                 global_volume_state_len());
            status_apply_formatted_color_end(header, &Config.volume_color);

            ncm_status_changes_flags();
            sb_free(&tracklength);
            nc_buffer_destroy(&second);
            nc_buffer_destroy(&first);
        }
        break;
    case NCM_DESIGN_COUNT:
        break;
    default:
        break;
    }

    if (ncm_progressbar_is_unlocked()) {
        ncm_progressbar_draw(status_elapsed_time, status_total_time);
    }

    ncm_song_destroy(&song);
    return;
}

void
ncm_status_changes_flags(void) {
    NcWindow *header;
    StrBuilder switch_state = {0};
    int32 flags_x;

    if (!Config.header_visibility
        && (Config.user_interface == NCM_DESIGN_CLASSIC)) {
        return;
    }

    if ((header = ui_state_header_window()) == NULL) {
        return;
    }

    switch (Config.user_interface) {
    case NCM_DESIGN_CLASSIC:
        sb_append_byte(&switch_state, status_repeat);
        sb_append_byte(&switch_state, status_random);
        sb_append_byte(&switch_state, status_single);
        sb_append_byte(&switch_state, status_consume);
        sb_append_byte(&switch_state, status_crossfade);
        sb_append_byte(&switch_state, status_db_updating);

        status_apply_formatted_color(header, &Config.state_line_color);
        mvwhline(nc_window_raw(header), 1, 0, 0, COLS);
        status_apply_formatted_color_end(header, &Config.state_line_color);

        if (switch_state.len > 0) {
            flags_x = COLS - switch_state.len - 3;
            if (flags_x < 0) {
                flags_x = 0;
            }
            nc_window_go_to_xy(header, flags_x, 1);
            status_apply_formatted_color(header, &Config.state_line_color);
            nc_window_print_char(header, '[');
            status_apply_formatted_color_end(header, &Config.state_line_color);
            status_apply_formatted_color(header, &Config.state_flags_color);
            nc_window_print_data(header, switch_state.data, switch_state.len);
            status_apply_formatted_color_end(header, &Config.state_flags_color);
            status_apply_formatted_color(header, &Config.state_line_color);
            nc_window_print_char(header, ']');
            status_apply_formatted_color_end(header, &Config.state_line_color);
        }
        break;
    case NCM_DESIGN_ALTERNATIVE:
        sb_append_byte(&switch_state, '[');
        if (status_repeat) {
            sb_append_byte(&switch_state, status_repeat);
        } else {
            sb_append_byte(&switch_state, '-');
        }
        if (status_random) {
            sb_append_byte(&switch_state, status_random);
        } else {
            sb_append_byte(&switch_state, '-');
        }
        if (status_single) {
            sb_append_byte(&switch_state, status_single);
        } else {
            sb_append_byte(&switch_state, '-');
        }
        if (status_consume) {
            sb_append_byte(&switch_state, status_consume);
        } else {
            sb_append_byte(&switch_state, '-');
        }
        if (status_crossfade) {
            sb_append_byte(&switch_state, status_crossfade);
        } else {
            sb_append_byte(&switch_state, '-');
        }
        if (status_db_updating) {
            sb_append_byte(&switch_state, status_db_updating);
        } else {
            sb_append_byte(&switch_state, '-');
        }
        sb_append_byte(&switch_state, ']');

        flags_x = COLS - switch_state.len;
        if (flags_x < 0) {
            flags_x = 0;
        }
        nc_window_go_to_xy(header, flags_x, 1);
        status_apply_formatted_color(header, &Config.state_flags_color);
        nc_window_print_data(header, switch_state.data, switch_state.len);
        status_apply_formatted_color_end(header, &Config.state_flags_color);
        if (!Config.header_visibility) {
            status_apply_formatted_color(
                header, &Config.alternative_ui_separator_color);
            mvwhline(nc_window_raw(header), 2, 0, 0, COLS);
            status_apply_formatted_color_end(
                header, &Config.alternative_ui_separator_color);
        }
        break;
    case NCM_DESIGN_COUNT:
        break;
    default:
        break;
    }

    nc_window_refresh(header);
    sb_free(&switch_state);
    return;
}

void
ncm_status_changes_mixer(void) {
    NcWindow *header;
    char volume[32];
    int32 volume_len;
    int32 volume_x;

    if (!Config.display_volume_level
        || (!Config.header_visibility
            && (Config.user_interface == NCM_DESIGN_CLASSIC))) {
        return;
    }

    if ((header = ui_state_header_window()) == NULL) {
        return;
    }

    switch (Config.user_interface) {
    case NCM_DESIGN_CLASSIC:
        global_volume_state_set(" Volume: ", STRLIT_LEN(" Volume: "));
        break;
    case NCM_DESIGN_ALTERNATIVE:
        global_volume_state_set(" Vol: ", STRLIT_LEN(" Vol: "));
        break;
    case NCM_DESIGN_COUNT:
        break;
    default:
        break;
    }

    if (status_volume < 0) {
        global_volume_state_append("n/a", STRLIT_LEN("n/a"));
    } else {
        volume_len = SNPRINTF(volume, "%d", status_volume);
        global_volume_state_append(volume, volume_len);
        global_volume_state_append("%", STRLIT_LEN("%"));
    }

    volume_x = nc_window_width(header) - global_volume_state_len();
    if (volume_x < 0) {
        volume_x = 0;
    }
    nc_window_go_to_xy(header, volume_x, 0);
    status_apply_formatted_color(header, &Config.volume_color);
    nc_window_print_data(header, global_volume_state_cstr(),
                         global_volume_state_len());
    status_apply_formatted_color_end(header, &Config.volume_color);
    nc_window_refresh(header);
    return;
}

#endif /* NCMPCPP_STATUS_C */
