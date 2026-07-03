#include "c/ncm_song.h"

#include <mpd/client.h>

void
ncm_song_init(NcmSong *song) {
    song->song = NULL;
    return;
}

void
ncm_song_destroy(NcmSong *song) {
    if (song->song) {
        mpd_song_free(song->song);
    }

    song->song = NULL;
    return;
}

bool
ncm_song_copy(NcmSong *dest, NcmSong *source) {
    struct mpd_song *copy;

    if (dest == NULL) {
        return false;
    }
    if (source == NULL) {
        return false;
    }
    if (source->song == NULL) {
        ncm_song_destroy(dest);
        return true;
    }

    copy = mpd_song_dup(source->song);
    if (copy == NULL) {
        return false;
    }

    ncm_song_destroy(dest);
    dest->song = copy;
    return true;
}

bool
ncm_song_from_mpd_song(NcmSong *dest, struct mpd_song *source) {
    NcmSong replacement;

    if (dest == NULL) {
        return false;
    }
    if (source == NULL) {
        return false;
    }

    if (mpd_song_get_uri(source) == NULL) {
        return false;
    }

    ncm_song_init(&replacement);
    replacement.song = mpd_song_dup(source);
    if (replacement.song == NULL) {
        return false;
    }

    ncm_song_destroy(dest);
    *dest = replacement;
    return true;
}

bool
ncm_song_empty(NcmSong *song) {
    if (song == NULL) {
        return true;
    }

    return song->song == NULL;
}

struct mpd_song *
ncm_song_mpd_song(NcmSong *song) {
    if (song == NULL) {
        return NULL;
    }

    return song->song;
}

struct mpd_song *
ncm_song_dup_mpd_song(NcmSong *song) {
    if (song == NULL) {
        return NULL;
    }
    if (song->song == NULL) {
        return NULL;
    }

    return mpd_song_dup(song->song);
}
