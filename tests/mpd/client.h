#if !defined(NCMPCPP_TESTS_MPD_CLIENT_H)
#define NCMPCPP_TESTS_MPD_CLIENT_H

#include <mpd/tag.h>

struct mpd_connection;
struct mpd_directory;
struct mpd_entity;
struct mpd_playlist;
struct mpd_song;
struct mpd_status;
struct mpd_stats;
struct mpd_output;

enum mpd_error {
    MPD_ERROR_SUCCESS,
    MPD_ERROR_OOM,
    MPD_ERROR_ARGUMENT,
    MPD_ERROR_STATE,
    MPD_ERROR_TIMEOUT,
    MPD_ERROR_SYSTEM,
    MPD_ERROR_RESOLVER,
    MPD_ERROR_MALFORMED,
    MPD_ERROR_CLOSED,
    MPD_ERROR_SERVER,
};

enum mpd_server_error {
    MPD_SERVER_ERROR_UNK = -1,
};

enum mpd_state {
    MPD_STATE_UNKNOWN,
    MPD_STATE_STOP,
    MPD_STATE_PLAY,
    MPD_STATE_PAUSE,
};

enum mpd_idle {
    MPD_IDLE_DATABASE = 1 << 0,
    MPD_IDLE_STORED_PLAYLIST = 1 << 1,
    MPD_IDLE_QUEUE = 1 << 2,
    MPD_IDLE_PLAYER = 1 << 3,
    MPD_IDLE_MIXER = 1 << 4,
    MPD_IDLE_OUTPUT = 1 << 5,
    MPD_IDLE_OPTIONS = 1 << 6,
    MPD_IDLE_UPDATE = 1 << 7,
    MPD_IDLE_STICKER = 1 << 8,
    MPD_IDLE_SUBSCRIPTION = 1 << 9,
    MPD_IDLE_MESSAGE = 1 << 10,
    MPD_IDLE_NEIGHBOR = 1 << 11,
    MPD_IDLE_MOUNT = 1 << 12,
    MPD_IDLE_PARTITION = 1 << 13,
};

#endif /* NCMPCPP_TESTS_MPD_CLIENT_H */
