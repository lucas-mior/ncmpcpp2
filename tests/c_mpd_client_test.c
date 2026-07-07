#include "c/ncm_mpd_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
expect_true(bool value, char *message) {
    if (!value) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
    return;
}

static void
expect_false(bool value, char *message) {
    if (value) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
    return;
}

static void
expect_string(char *actual, char *expected, char *message) {
    if (strcmp(actual, expected) != 0) {
        fprintf(stderr, "%s: got '%s', expected '%s'\n",
                message, actual, expected);
        exit(EXIT_FAILURE);
    }
    return;
}

static void
test_client_defaults(void) {
    NcmMpdClient client;

    ncm_mpd_client_init(&client);
    expect_string(ncm_mpd_client_hostname(&client), "localhost",
                  "default MPD host");
    expect_true(ncm_mpd_client_port(&client) == 6600,
                "default MPD port");
    expect_true(ncm_mpd_client_timeout_ms(&client) == 15000,
                "default MPD timeout");
    expect_true(!ncm_mpd_client_connected(&client),
                "fresh MPD client must be disconnected");
    ncm_mpd_client_destroy(&client);
    return;
}

static void
test_host_password_split(void) {
    NcmMpdClient client;
    NcmError error;

    ncm_mpd_client_init(&client);
    ncm_error_clear(&error);
    expect_true(ncm_mpd_client_set_hostname(&client,
                                            "secret@example.org",
                                            -1, &error),
                "host parsing failed");
    expect_string(ncm_mpd_client_hostname(&client), "example.org",
                  "MPD host after password split");
    ncm_mpd_client_destroy(&client);
    return;
}

static void
test_song_based_api_guards(void) {
    NcmMpdClient client;
    NcmSong song;
    NcmSongList list;
    NcmMpdSongList mpd_list;
    NcmError error;

    ncm_mpd_client_init(&client);
    ncm_song_init(&song);
    ncm_song_list_init(&list);
    ncm_mpd_song_list_init(&mpd_list);
    ncm_error_clear(&error);

    expect_false(ncm_mpd_client_set_priority_song(&client, NULL,
                                                  1, &error),
                 "missing song priority guard failed");
    expect_true(ncm_error_is_set(&error), "missing song error not set");
    ncm_error_clear(&error);

    expect_false(ncm_mpd_client_add_song_value(&client, &song,
                                               -1, NULL, &error),
                 "song without URI guard failed");
    expect_true(ncm_error_is_set(&error), "missing URI error not set");
    ncm_error_clear(&error);

    expect_false(ncm_mpd_client_add_song_list(&client, &mpd_list,
                                               -1, &error),
                 "empty MPD song list guard failed");
    expect_true(ncm_error_is_set(&error), "empty MPD list error not set");
    ncm_error_clear(&error);

    expect_false(ncm_mpd_client_add_selected_songs(&client, &list,
                                                   -1, &error),
                 "empty selected song list guard failed");
    expect_true(ncm_error_is_set(&error), "empty selection error not set");

    ncm_mpd_song_list_destroy(&mpd_list);
    ncm_song_list_destroy(&list);
    ncm_song_destroy(&song);
    ncm_mpd_client_destroy(&client);
    return;
}

int
main(void) {
    test_client_defaults();
    test_host_password_split();
    test_song_based_api_guards();
    return EXIT_SUCCESS;
}
