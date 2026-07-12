#include <stdio.h>
#include <stdlib.h>

#include "status.h"
#include "screens/nc_visualizer.h"

#define REQUIRE(COND) do {                                                \
    if (!(COND)) {                                                        \
        fail(__FILE__, __LINE__, (char *)#COND);                          \
    }                                                                     \
} while (0)

#define REQUIRE_INT(ACTUAL, EXPECTED)                                     \
    require_int(__FILE__, __LINE__, (char *)#ACTUAL,                      \
                (int32)(ACTUAL), (int32)(EXPECTED))

#define REQUIRE_UINT(ACTUAL, EXPECTED)                                    \
    require_uint(__FILE__, __LINE__, (char *)#ACTUAL,                     \
                 (uint32)(ACTUAL), (uint32)(EXPECTED))

typedef struct StatusHookProbe {
    int32 playlist_calls;
    int32 stored_playlist_calls;
    int32 database_calls;
    int32 player_state_calls;
    int32 song_id_calls;
    int32 elapsed_time_calls;
    int32 flags_calls;
    int32 mixer_calls;
    int32 outputs_calls;
    int32 refresh_footer_calls;
    int32 refresh_visible_screens_calls;
    int32 notifications;
    int32 ui_playlist_calls;
    int32 ui_stored_playlist_calls;
    int32 ui_database_calls;
    int32 ui_player_state_calls;
    int32 ui_player_stopped_calls;
    int32 ui_song_id_calls;
    int32 ui_current_song_calls;
    int32 init_calls;
    int32 init_order[8];
    int32 visualizer_screen_calls;
    int32 visualizer_close_calls;
    int32 visualizer_open_calls;
    int32 visualizer_find_calls;
    int32 visualizer_calls;
    int32 visualizer_call_order[3];

    uint32 previous_playlist_version;
    uint32 ui_previous_playlist_version;
    int32 song_id;
    bool elapsed_update;
} StatusHookProbe;

static StatusHookProbe *status_visualizer_probe;
static NativeVisualizerScreen status_visualizer_screen;

static void fail(char *file, int32 line, char *condition);
static void require_int(char *file, int32 line, char *name,
                        int32 actual, int32 expected);
static void require_uint(char *file, int32 line, char *name,
                         uint32 actual, uint32 expected);
static void status_record_init_call(StatusHookProbe *probe, int32 id);
static void status_record_visualizer_call(StatusHookProbe *probe, int32 id);
static NcmMpdStatus status_make(void);
static NcmStatusHooks status_hooks_make(StatusHookProbe *probe);
static NcmStatusUiHooks status_ui_hooks_make(StatusHookProbe *probe);
static NcmStatusInitHooks status_init_hooks_make(StatusHookProbe *probe);
static void status_hook_playlist(uint32 previous_version, void *user);
static void status_hook_stored_playlists(void *user);
static void status_hook_database(void *user);
static void status_hook_player_state(void *user);
static void status_hook_song_id(int32 song_id, void *user);
static void status_hook_elapsed_time(bool update_elapsed, void *user);
static void status_hook_flags(void *user);
static void status_hook_mixer(void *user);
static void status_hook_outputs(void *user);
static void status_hook_refresh_footer(void *user);
static void status_hook_refresh_visible_screens(void *user);
static void status_hook_ui_playlist(uint32 previous_version, void *user);
static void status_hook_ui_stored_playlists(void *user);
static void status_hook_ui_database(void *user);
static void status_hook_ui_player_state(enum NcmStatusPlayerState state,
                                        void *user);
static void status_hook_ui_player_stopped(void *user);
static void status_hook_ui_song_id(int32 song_id, void *user);
static void status_hook_ui_current_song(NcmSong *song, void *user);
static void status_hook_init_jump_to_now_playing(void *user);
static void status_hook_init_set_tcp_nodelay(void *user);
static void status_hook_init_load_browser_supported_extensions(void *user);
static void status_hook_init_fetch_outputs(void *user);
static void status_hook_init_setup_visualizer_datasource(void *user);
static void status_hook_init_register_mpd_fd_callback(void *user);
static void status_hook_init_show_connected_message(void *user);
static void status_hook_notification(void *user);
static void test_initial_options_do_not_notify(void);
static void test_option_changes_update_state_and_hooks(void);
static void test_playlist_hook_gets_previous_version(void);
static void test_player_hooks_and_song_id_change(void);
static void test_elapsed_time_state_updates_without_windows(void);
static void test_output_database_and_stored_playlist_hooks(void);
static void test_registered_hooks_use_c_ported_fallbacks(void);
static void test_player_and_song_id_c_fallbacks_use_ui_hooks(void);
static void test_database_and_stored_playlist_c_fallbacks_use_ui_hooks(void);
static void test_playlist_c_fallback_uses_ui_hook(void);
static void test_connection_initialization_runs_init_hooks(void);
static void test_connection_initialization_uses_c_visualizer_fallback(void);

NativeVisualizerScreen *__real_native_c_screen_visualizer(void);
void __real_native_visualizer_screen_close_data_source(
    NativeVisualizerScreen *screen);
bool __real_native_visualizer_screen_open_data_source(
    NativeVisualizerScreen *screen);
bool __real_native_visualizer_screen_find_output_id(
    NativeVisualizerScreen *screen);
NativeVisualizerScreen *__wrap_native_c_screen_visualizer(void);
void __wrap_native_visualizer_screen_close_data_source(
    NativeVisualizerScreen *screen);
bool __wrap_native_visualizer_screen_open_data_source(
    NativeVisualizerScreen *screen);
bool __wrap_native_visualizer_screen_find_output_id(
    NativeVisualizerScreen *screen);

int
main(void) {
    test_initial_options_do_not_notify();
    test_option_changes_update_state_and_hooks();
    test_playlist_hook_gets_previous_version();
    test_player_hooks_and_song_id_change();
    test_elapsed_time_state_updates_without_windows();
    test_output_database_and_stored_playlist_hooks();
    test_registered_hooks_use_c_ported_fallbacks();
    test_player_and_song_id_c_fallbacks_use_ui_hooks();
    test_database_and_stored_playlist_c_fallbacks_use_ui_hooks();
    test_playlist_c_fallback_uses_ui_hook();
    test_connection_initialization_runs_init_hooks();
    test_connection_initialization_uses_c_visualizer_fallback();
    return EXIT_SUCCESS;
}

static void
fail(char *file, int32 line, char *condition) {
    fprintf(stderr, "%s:%d: requirement failed: %s\n",
            file, line, condition);
    exit(EXIT_FAILURE);
}

static void
require_int(char *file, int32 line, char *name,
            int32 actual, int32 expected) {
    if (actual != expected) {
        fprintf(stderr, "%s:%d: %s: expected %d, got %d\n",
                file, line, name, expected, actual);
        exit(EXIT_FAILURE);
    }
    return;
}

static void
require_uint(char *file, int32 line, char *name,
             uint32 actual, uint32 expected) {
    if (actual != expected) {
        fprintf(stderr, "%s:%d: %s: expected %u, got %u\n",
                file, line, name, expected, actual);
        exit(EXIT_FAILURE);
    }
    return;
}

static void
status_record_init_call(StatusHookProbe *probe, int32 id) {
    probe->init_order[probe->init_calls] = id;
    probe->init_calls += 1;
    return;
}

static void
status_record_visualizer_call(StatusHookProbe *probe, int32 id) {
    REQUIRE(probe->visualizer_calls < 3);
    probe->visualizer_call_order[probe->visualizer_calls] = id;
    probe->visualizer_calls += 1;
    return;
}

NativeVisualizerScreen *
__wrap_native_c_screen_visualizer(void) {
    if (status_visualizer_probe == NULL) {
        return __real_native_c_screen_visualizer();
    }

    status_visualizer_probe->visualizer_screen_calls += 1;
    return &status_visualizer_screen;
}

void
__wrap_native_visualizer_screen_close_data_source(
    NativeVisualizerScreen *screen) {
    if (status_visualizer_probe == NULL) {
        __real_native_visualizer_screen_close_data_source(screen);
        return;
    }

    REQUIRE(screen == &status_visualizer_screen);
    status_visualizer_probe->visualizer_close_calls += 1;
    status_record_visualizer_call(status_visualizer_probe, 1);
    return;
}

bool
__wrap_native_visualizer_screen_open_data_source(
    NativeVisualizerScreen *screen) {
    if (status_visualizer_probe == NULL) {
        return __real_native_visualizer_screen_open_data_source(screen);
    }

    REQUIRE(screen == &status_visualizer_screen);
    status_visualizer_probe->visualizer_open_calls += 1;
    status_record_visualizer_call(status_visualizer_probe, 2);
    return true;
}

bool
__wrap_native_visualizer_screen_find_output_id(
    NativeVisualizerScreen *screen) {
    if (status_visualizer_probe == NULL) {
        return __real_native_visualizer_screen_find_output_id(screen);
    }

    REQUIRE(screen == &status_visualizer_screen);
    status_visualizer_probe->visualizer_find_calls += 1;
    status_record_visualizer_call(status_visualizer_probe, 3);
    return true;
}

static NcmMpdStatus
status_make(void) {
    NcmMpdStatus status = {0};

    status.volume = 40;
    status.queue_length = 12;
    status.queue_version = 3;
    status.state = MPD_STATE_STOP;
    status.song_pos = -1;
    status.song_id = -1;
    status.next_song_pos = -1;
    status.next_song_id = -1;
    return status;
}

static NcmStatusHooks
status_hooks_make(StatusHookProbe *probe) {
    NcmStatusHooks hooks = {
        .user = probe,
        .playlist_changed = status_hook_playlist,
        .stored_playlists_changed = status_hook_stored_playlists,
        .database_changed = status_hook_database,
        .player_state_changed = status_hook_player_state,
        .song_id_changed = status_hook_song_id,
        .elapsed_time_changed = status_hook_elapsed_time,
        .flags_changed = status_hook_flags,
        .mixer_changed = status_hook_mixer,
        .outputs_changed = status_hook_outputs,
        .refresh_footer = status_hook_refresh_footer,
        .refresh_visible_screens = status_hook_refresh_visible_screens,
    };

    return hooks;
}

static NcmStatusUiHooks
status_ui_hooks_make(StatusHookProbe *probe) {
    NcmStatusUiHooks hooks = {
        .user = probe,
        .playlist_changed = status_hook_ui_playlist,
        .stored_playlists_changed = status_hook_ui_stored_playlists,
        .database_changed = status_hook_ui_database,
        .player_state_changed = status_hook_ui_player_state,
        .player_stopped = status_hook_ui_player_stopped,
        .song_id_changed = status_hook_ui_song_id,
        .current_song_changed = status_hook_ui_current_song,
    };

    return hooks;
}

static NcmStatusInitHooks
status_init_hooks_make(StatusHookProbe *probe) {
    NcmStatusInitHooks hooks = {
        .user = probe,
        .jump_to_now_playing = status_hook_init_jump_to_now_playing,
        .set_tcp_nodelay = status_hook_init_set_tcp_nodelay,
        .load_browser_supported_extensions =
            status_hook_init_load_browser_supported_extensions,
        .fetch_outputs = status_hook_init_fetch_outputs,
        .setup_visualizer_datasource =
            status_hook_init_setup_visualizer_datasource,
        .register_mpd_fd_callback =
            status_hook_init_register_mpd_fd_callback,
        .show_connected_message = status_hook_init_show_connected_message,
    };

    return hooks;
}

static void
status_hook_playlist(uint32 previous_version, void *user) {
    StatusHookProbe *probe;

    probe = user;
    probe->playlist_calls += 1;
    probe->previous_playlist_version = previous_version;
    return;
}

static void
status_hook_stored_playlists(void *user) {
    StatusHookProbe *probe;

    probe = user;
    probe->stored_playlist_calls += 1;
    return;
}

static void
status_hook_database(void *user) {
    StatusHookProbe *probe;

    probe = user;
    probe->database_calls += 1;
    return;
}

static void
status_hook_player_state(void *user) {
    StatusHookProbe *probe;

    probe = user;
    probe->player_state_calls += 1;
    return;
}

static void
status_hook_song_id(int32 song_id, void *user) {
    StatusHookProbe *probe;

    probe = user;
    probe->song_id_calls += 1;
    probe->song_id = song_id;
    return;
}

static void
status_hook_elapsed_time(bool update_elapsed, void *user) {
    StatusHookProbe *probe;

    probe = user;
    probe->elapsed_time_calls += 1;
    probe->elapsed_update = update_elapsed;
    return;
}

static void
status_hook_flags(void *user) {
    StatusHookProbe *probe;

    probe = user;
    probe->flags_calls += 1;
    return;
}

static void
status_hook_mixer(void *user) {
    StatusHookProbe *probe;

    probe = user;
    probe->mixer_calls += 1;
    return;
}

static void
status_hook_outputs(void *user) {
    StatusHookProbe *probe;

    probe = user;
    probe->outputs_calls += 1;
    return;
}

static void
status_hook_refresh_footer(void *user) {
    StatusHookProbe *probe;

    probe = user;
    probe->refresh_footer_calls += 1;
    return;
}

static void
status_hook_refresh_visible_screens(void *user) {
    StatusHookProbe *probe;

    probe = user;
    probe->refresh_visible_screens_calls += 1;
    return;
}


static void
status_hook_ui_playlist(uint32 previous_version, void *user) {
    StatusHookProbe *probe;

    probe = user;
    probe->ui_playlist_calls += 1;
    probe->ui_previous_playlist_version = previous_version;
    return;
}

static void
status_hook_ui_stored_playlists(void *user) {
    StatusHookProbe *probe;

    probe = user;
    probe->ui_stored_playlist_calls += 1;
    return;
}

static void
status_hook_ui_database(void *user) {
    StatusHookProbe *probe;

    probe = user;
    probe->ui_database_calls += 1;
    return;
}

static void
status_hook_ui_player_state(enum NcmStatusPlayerState state, void *user) {
    StatusHookProbe *probe;

    (void)state;
    probe = user;
    probe->ui_player_state_calls += 1;
    return;
}

static void
status_hook_ui_player_stopped(void *user) {
    StatusHookProbe *probe;

    probe = user;
    probe->ui_player_stopped_calls += 1;
    return;
}

static void
status_hook_ui_song_id(int32 song_id, void *user) {
    StatusHookProbe *probe;

    (void)song_id;
    probe = user;
    probe->ui_song_id_calls += 1;
    return;
}

static void
status_hook_ui_current_song(NcmSong *song, void *user) {
    StatusHookProbe *probe;

    (void)song;
    probe = user;
    probe->ui_current_song_calls += 1;
    return;
}

static void
status_hook_init_jump_to_now_playing(void *user) {
    status_record_init_call(user, 1);
    return;
}

static void
status_hook_init_set_tcp_nodelay(void *user) {
    status_record_init_call(user, 2);
    return;
}

static void
status_hook_init_load_browser_supported_extensions(void *user) {
    status_record_init_call(user, 3);
    return;
}

static void
status_hook_init_fetch_outputs(void *user) {
    status_record_init_call(user, 4);
    return;
}

static void
status_hook_init_setup_visualizer_datasource(void *user) {
    status_record_init_call(user, 5);
    return;
}

static void
status_hook_init_register_mpd_fd_callback(void *user) {
    status_record_init_call(user, 6);
    return;
}

static void
status_hook_init_show_connected_message(void *user) {
    status_record_init_call(user, 7);
    return;
}

static void
status_hook_notification(void *user) {
    StatusHookProbe *probe;

    probe = user;
    probe->notifications += 1;
    return;
}

static void
test_initial_options_do_not_notify(void) {
    StatusHookProbe probe = {0};
    NcmStatusHooks hooks;
    NcmMpdStatus status;

    ncm_status_clear();
    hooks = status_hooks_make(&probe);
    ncm_status_set_notification_observer(status_hook_notification, &probe);

    status = status_make();
    status.repeat = true;
    status.random = true;
    status.single = true;
    status.consume = true;
    status.crossfade = 9;

    REQUIRE(ncm_status_apply_mpd_status(&status, MPD_IDLE_OPTIONS,
                                        &hooks, NULL));
    REQUIRE_INT(probe.notifications, 0);
    REQUIRE_INT(probe.flags_calls, 1);
    REQUIRE(ncm_status_state_repeat());
    REQUIRE(ncm_status_state_random());
    REQUIRE(ncm_status_state_single());
    REQUIRE(ncm_status_state_consume());
    REQUIRE(ncm_status_state_crossfade());

    ncm_status_set_notification_observer(NULL, NULL);
    return;
}

static void
test_option_changes_update_state_and_hooks(void) {
    StatusHookProbe probe = {0};
    NcmStatusHooks hooks;
    NcmMpdStatus status;

    ncm_status_clear();
    hooks = status_hooks_make(&probe);
    ncm_status_set_notification_observer(status_hook_notification, &probe);

    status = status_make();
    REQUIRE(ncm_status_apply_mpd_status(&status, MPD_IDLE_OPTIONS,
                                        &hooks, NULL));

    probe = (StatusHookProbe){0};
    status.repeat = true;
    status.random = true;
    status.single = true;
    status.consume = true;
    status.crossfade = 7;
    REQUIRE(ncm_status_apply_mpd_status(&status, MPD_IDLE_OPTIONS,
                                        &hooks, NULL));

    REQUIRE_INT(probe.flags_calls, 1);
    REQUIRE_INT(probe.notifications, 5);
    REQUIRE(ncm_status_state_repeat());
    REQUIRE(ncm_status_state_random());
    REQUIRE(ncm_status_state_single());
    REQUIRE(ncm_status_state_consume());
    REQUIRE(ncm_status_state_crossfade());

    ncm_status_set_notification_observer(NULL, NULL);
    return;
}

static void
test_playlist_hook_gets_previous_version(void) {
    StatusHookProbe probe = {0};
    NcmStatusHooks hooks;
    NcmMpdStatus status;

    ncm_status_clear();
    hooks = status_hooks_make(&probe);
    status = status_make();
    status.queue_version = 11;
    REQUIRE(ncm_status_apply_mpd_status(&status, MPD_IDLE_PLAYLIST,
                                        &hooks, NULL));

    probe = (StatusHookProbe){0};
    status.queue_version = 29;
    REQUIRE(ncm_status_apply_mpd_status(&status, MPD_IDLE_PLAYLIST,
                                        &hooks, NULL));

    REQUIRE_INT(probe.playlist_calls, 1);
    REQUIRE_UINT(probe.previous_playlist_version, 11);
    REQUIRE_UINT(ncm_status_state_playlist_version(), 29);
    REQUIRE_INT(probe.refresh_visible_screens_calls, 1);
    return;
}

static void
test_player_hooks_and_song_id_change(void) {
    StatusHookProbe probe = {0};
    NcmStatusHooks hooks;
    NcmMpdStatus status;

    ncm_status_clear();
    hooks = status_hooks_make(&probe);
    status = status_make();
    status.state = MPD_STATE_PLAY;
    status.song_pos = 4;
    status.song_id = 70;

    REQUIRE(ncm_status_apply_mpd_status(&status, MPD_IDLE_PLAYER,
                                        &hooks, NULL));
    REQUIRE_INT(probe.player_state_calls, 1);
    REQUIRE_INT(probe.song_id_calls, 1);
    REQUIRE_INT(probe.song_id, 70);
    REQUIRE_INT(probe.refresh_footer_calls, 1);
    REQUIRE_INT(probe.refresh_visible_screens_calls, 1);
    REQUIRE_INT(ncm_status_state_player(), NCM_STATUS_PLAYER_PLAY);
    REQUIRE_INT(ncm_status_state_current_song_id(), 70);
    REQUIRE_INT(ncm_status_state_current_song_position(), 4);

    probe = (StatusHookProbe){0};
    status.state = MPD_STATE_PAUSE;
    REQUIRE(ncm_status_apply_mpd_status(&status, MPD_IDLE_PLAYER,
                                        &hooks, NULL));
    REQUIRE_INT(probe.player_state_calls, 1);
    REQUIRE_INT(probe.song_id_calls, 0);
    REQUIRE_INT(ncm_status_state_player(), NCM_STATUS_PLAYER_PAUSE);
    REQUIRE_INT(ncm_status_state_current_song_id(), 70);

    probe = (StatusHookProbe){0};
    status.song_id = 71;
    REQUIRE(ncm_status_apply_mpd_status(&status, MPD_IDLE_PLAYER,
                                        &hooks, NULL));
    REQUIRE_INT(probe.song_id_calls, 1);
    REQUIRE_INT(probe.song_id, 71);
    REQUIRE_INT(ncm_status_state_current_song_id(), 71);
    return;
}

static void
test_elapsed_time_state_updates_without_windows(void) {
    NcmMpdStatus status;

    ncm_status_clear();
    status = status_make();
    status.elapsed_time = 42;
    status.total_time = 120;
    status.kbit_rate = 320;

    REQUIRE(ncm_status_apply_mpd_status(&status, 0, NULL, NULL));
    REQUIRE_UINT(ncm_status_state_elapsed_time(), 42);
    REQUIRE_UINT(ncm_status_state_total_time(), 120);
    REQUIRE_UINT(ncm_status_state_kbps(), 320);

    ncm_status_changes_elapsed_time(true);
    REQUIRE_UINT(ncm_status_state_elapsed_time(), 43);
    ncm_status_changes_elapsed_time(false);
    REQUIRE_UINT(ncm_status_state_elapsed_time(), 43);
    return;
}

static void
test_output_database_and_stored_playlist_hooks(void) {
    StatusHookProbe probe = {0};
    NcmStatusHooks hooks;
    NcmMpdStatus status;
    int32 event;

    ncm_status_clear();
    hooks = status_hooks_make(&probe);
    status = status_make();
    event = MPD_IDLE_OUTPUT
            |MPD_IDLE_DATABASE
            |MPD_IDLE_STORED_PLAYLIST
            |MPD_IDLE_MIXER;

    REQUIRE(ncm_status_apply_mpd_status(&status, event, &hooks, NULL));
    REQUIRE_INT(probe.outputs_calls, 1);
    REQUIRE_INT(probe.database_calls, 1);
    REQUIRE_INT(probe.stored_playlist_calls, 1);
    REQUIRE_INT(probe.mixer_calls, 1);
    REQUIRE_INT(probe.refresh_visible_screens_calls, 1);
    return;
}

static void
test_registered_hooks_use_c_ported_fallbacks(void) {
    StatusHookProbe probe = {0};
    NcmStatusHooks hooks;
    NcmMpdStatus status;
    int32 event;

    ncm_status_clear();
    hooks = status_hooks_make(&probe);
    hooks.elapsed_time_changed = NULL;
    hooks.flags_changed = NULL;
    hooks.mixer_changed = NULL;
    hooks.outputs_changed = NULL;
    ncm_status_set_hooks(&hooks);

    status = status_make();
    status.state = MPD_STATE_PLAY;
    status.elapsed_time = 10;
    status.kbit_rate = 192;
    status.repeat = true;
    status.random = true;
    status.single = true;
    status.consume = true;
    status.crossfade = 4;
    status.volume = 55;
    event = MPD_IDLE_OPTIONS | MPD_IDLE_MIXER | MPD_IDLE_OUTPUT;

    REQUIRE(ncm_status_apply_mpd_status(&status, event, NULL, NULL));
    REQUIRE_INT(probe.flags_calls, 0);
    REQUIRE_INT(probe.mixer_calls, 0);
    REQUIRE_INT(probe.outputs_calls, 0);
    REQUIRE(ncm_status_state_repeat());
    REQUIRE(ncm_status_state_random());
    REQUIRE(ncm_status_state_single());
    REQUIRE(ncm_status_state_consume());
    REQUIRE(ncm_status_state_crossfade());
    REQUIRE_INT(ncm_status_state_volume(), 55);

    ncm_status_changes_elapsed_time(true);
    REQUIRE_UINT(ncm_status_state_elapsed_time(), 11);

    ncm_status_set_hooks(NULL);
    return;
}

static void
test_player_and_song_id_c_fallbacks_use_ui_hooks(void) {
    StatusHookProbe probe = {0};
    NcmStatusHooks hooks;
    NcmStatusUiHooks ui_hooks;
    NcmMpdStatus status;

    ncm_status_clear();
    hooks = status_hooks_make(&probe);
    hooks.player_state_changed = NULL;
    hooks.song_id_changed = NULL;
    ui_hooks = status_ui_hooks_make(&probe);
    ncm_status_set_hooks(&hooks);
    ncm_status_set_ui_hooks(&ui_hooks);

    status = status_make();
    status.state = MPD_STATE_PLAY;
    status.song_pos = -1;
    status.song_id = 91;
    REQUIRE(ncm_status_apply_mpd_status(&status, MPD_IDLE_PLAYER,
                                        NULL, NULL));
    REQUIRE_INT(probe.player_state_calls, 0);
    REQUIRE_INT(probe.song_id_calls, 0);
    REQUIRE_INT(probe.ui_player_state_calls, 1);
    REQUIRE_INT(probe.ui_song_id_calls, 1);
    REQUIRE_INT(probe.ui_player_stopped_calls, 0);
    REQUIRE_INT(ncm_status_state_current_song_id(), 91);

    probe = (StatusHookProbe){0};
    status.state = MPD_STATE_STOP;
    status.song_id = 92;
    REQUIRE(ncm_status_apply_mpd_status(&status, MPD_IDLE_PLAYER,
                                        NULL, NULL));
    REQUIRE_INT(probe.player_state_calls, 0);
    REQUIRE_INT(probe.song_id_calls, 0);
    REQUIRE_INT(probe.ui_player_state_calls, 1);
    REQUIRE_INT(probe.ui_song_id_calls, 1);
    REQUIRE_INT(probe.ui_player_stopped_calls, 1);
    REQUIRE_INT(ncm_status_state_current_song_id(), 92);

    ncm_status_set_hooks(NULL);
    ncm_status_set_ui_hooks(NULL);
    return;
}

static void
test_database_and_stored_playlist_c_fallbacks_use_ui_hooks(void) {
    StatusHookProbe probe = {0};
    NcmStatusHooks hooks;
    NcmStatusUiHooks ui_hooks;
    NcmMpdStatus status;
    int32 event;

    ncm_status_clear();
    hooks = status_hooks_make(&probe);
    hooks.database_changed = NULL;
    hooks.stored_playlists_changed = NULL;
    ui_hooks = status_ui_hooks_make(&probe);
    ncm_status_set_hooks(&hooks);
    ncm_status_set_ui_hooks(&ui_hooks);

    status = status_make();
    event = MPD_IDLE_DATABASE | MPD_IDLE_STORED_PLAYLIST;
    REQUIRE(ncm_status_apply_mpd_status(&status, event, NULL, NULL));
    REQUIRE_INT(probe.database_calls, 0);
    REQUIRE_INT(probe.stored_playlist_calls, 0);
    REQUIRE_INT(probe.ui_database_calls, 1);
    REQUIRE_INT(probe.ui_stored_playlist_calls, 1);
    REQUIRE_INT(probe.refresh_visible_screens_calls, 1);

    ncm_status_set_hooks(NULL);
    ncm_status_set_ui_hooks(NULL);
    return;
}

static void
test_playlist_c_fallback_uses_ui_hook(void) {
    StatusHookProbe probe = {0};
    NcmStatusHooks hooks;
    NcmStatusUiHooks ui_hooks;
    NcmMpdStatus status;

    ncm_status_clear();
    hooks = status_hooks_make(&probe);
    hooks.playlist_changed = NULL;
    ui_hooks = status_ui_hooks_make(&probe);
    ncm_status_set_hooks(&hooks);
    ncm_status_set_ui_hooks(&ui_hooks);

    status = status_make();
    status.queue_version = 7;
    REQUIRE(ncm_status_apply_mpd_status(&status, MPD_IDLE_PLAYLIST,
                                        NULL, NULL));

    probe = (StatusHookProbe){0};
    status.queue_version = 19;
    REQUIRE(ncm_status_apply_mpd_status(&status, MPD_IDLE_PLAYLIST,
                                        NULL, NULL));
    REQUIRE_INT(probe.playlist_calls, 0);
    REQUIRE_INT(probe.ui_playlist_calls, 1);
    REQUIRE_UINT(probe.ui_previous_playlist_version, 7);
    REQUIRE_UINT(ncm_status_state_playlist_version(), 19);
    REQUIRE_INT(probe.refresh_visible_screens_calls, 1);

    ncm_status_set_hooks(NULL);
    ncm_status_set_ui_hooks(NULL);
    return;
}

static void
test_connection_initialization_runs_init_hooks(void) {
    StatusHookProbe probe = {0};
    NcmStatusHooks hooks;
    NcmStatusInitHooks init_hooks;
    NcmMpdStatus status;

    ncm_status_clear();
    hooks = status_hooks_make(&probe);
    init_hooks = status_init_hooks_make(&probe);
    ncm_status_set_init_hooks(&init_hooks);
    ncm_status_set_notification_observer(status_hook_notification, &probe);

    status = status_make();
    status.repeat = true;
    status.random = true;
    status.single = true;
    status.consume = true;
    status.crossfade = 3;
    REQUIRE(ncm_status_initialize_from_mpd_status(&status, &hooks, NULL));

    REQUIRE_INT(probe.flags_calls, 1);
    REQUIRE_INT(probe.notifications, 0);
    REQUIRE_INT(probe.init_calls, 7);
    REQUIRE_INT(probe.init_order[0], 1);
    REQUIRE_INT(probe.init_order[1], 2);
    REQUIRE_INT(probe.init_order[2], 3);
    REQUIRE_INT(probe.init_order[3], 4);
    REQUIRE_INT(probe.init_order[4], 5);
    REQUIRE_INT(probe.init_order[5], 6);
    REQUIRE_INT(probe.init_order[6], 7);

    ncm_status_set_init_hooks(NULL);
    ncm_status_set_notification_observer(NULL, NULL);
    return;
}

static void
test_connection_initialization_uses_c_visualizer_fallback(void) {
    StatusHookProbe probe = {0};
    NcmStatusHooks hooks;
    NcmStatusInitHooks init_hooks;
    NcmMpdStatus status;

    ncm_status_clear();
    hooks = status_hooks_make(&probe);
    init_hooks = status_init_hooks_make(&probe);
    init_hooks.setup_visualizer_datasource = NULL;
    ncm_status_set_init_hooks(&init_hooks);
    status_visualizer_probe = &probe;

    status = status_make();
    REQUIRE(ncm_status_initialize_from_mpd_status(&status, &hooks, NULL));

    REQUIRE_INT(probe.init_calls, 6);
    REQUIRE_INT(probe.init_order[0], 1);
    REQUIRE_INT(probe.init_order[1], 2);
    REQUIRE_INT(probe.init_order[2], 3);
    REQUIRE_INT(probe.init_order[3], 4);
    REQUIRE_INT(probe.init_order[4], 6);
    REQUIRE_INT(probe.init_order[5], 7);
    REQUIRE_INT(probe.visualizer_screen_calls, 1);
    REQUIRE_INT(probe.visualizer_close_calls, 1);
    REQUIRE_INT(probe.visualizer_open_calls, 1);
    REQUIRE_INT(probe.visualizer_find_calls, 1);
    REQUIRE_INT(probe.visualizer_calls, 3);
    REQUIRE_INT(probe.visualizer_call_order[0], 1);
    REQUIRE_INT(probe.visualizer_call_order[1], 2);
    REQUIRE_INT(probe.visualizer_call_order[2], 3);

    status_visualizer_probe = NULL;
    ncm_status_set_init_hooks(NULL);
    return;
}
