#include "c/ncm_mpd_item.h"

#include <mpd/client.h>

static bool ncm_mpd_item_set_song_copy(NcmMpdItem *item,
                                       struct mpd_song *source);
static bool ncm_mpd_item_set_song_borrow(NcmMpdItem *item,
                                         struct mpd_song *source);
static bool ncm_mpd_item_set_directory(NcmMpdItem *item,
                                       struct mpd_directory *source);
static bool ncm_mpd_item_set_playlist(NcmMpdItem *item,
                                      struct mpd_playlist *source);

static bool
ncm_mpd_item_set_song_copy(NcmMpdItem *item,
                           struct mpd_song *source) {
    NcmMpdItem replacement;

    ncm_mpd_item_init(&replacement);
    replacement.kind = NCM_MPD_ITEM_SONG;
    ncm_song_init(&replacement.value.song);
    if (!ncm_song_from_mpd_song_copy(&replacement.value.song, source)) {
        ncm_mpd_item_destroy(&replacement);
        return false;
    }

    ncm_mpd_item_destroy(item);
    *item = replacement;
    return true;
}

static bool
ncm_mpd_item_set_song_borrow(NcmMpdItem *item,
                             struct mpd_song *source) {
    NcmMpdItem replacement;

    ncm_mpd_item_init(&replacement);
    replacement.kind = NCM_MPD_ITEM_SONG;
    ncm_song_init(&replacement.value.song);
    if (!ncm_song_borrow_mpd_song(&replacement.value.song, source)) {
        ncm_mpd_item_destroy(&replacement);
        return false;
    }

    ncm_mpd_item_destroy(item);
    *item = replacement;
    return true;
}

static bool
ncm_mpd_item_set_directory(NcmMpdItem *item,
                           struct mpd_directory *source) {
    NcmMpdItem replacement;

    ncm_mpd_item_init(&replacement);
    replacement.kind = NCM_MPD_ITEM_DIRECTORY;
    ncm_directory_init(&replacement.value.directory);
    if (!ncm_directory_from_mpd_directory(&replacement.value.directory,
                                          source)) {
        ncm_mpd_item_destroy(&replacement);
        return false;
    }

    ncm_mpd_item_destroy(item);
    *item = replacement;
    return true;
}

static bool
ncm_mpd_item_set_playlist(NcmMpdItem *item,
                          struct mpd_playlist *source) {
    NcmMpdItem replacement;

    ncm_mpd_item_init(&replacement);
    replacement.kind = NCM_MPD_ITEM_PLAYLIST;
    ncm_playlist_init(&replacement.value.playlist);
    if (!ncm_playlist_from_mpd_playlist(&replacement.value.playlist,
                                        source)) {
        ncm_mpd_item_destroy(&replacement);
        return false;
    }

    ncm_mpd_item_destroy(item);
    *item = replacement;
    return true;
}

void
ncm_mpd_item_init(NcmMpdItem *item) {
    item->kind = NCM_MPD_ITEM_UNKNOWN;
    return;
}

void
ncm_mpd_item_destroy(NcmMpdItem *item) {
    if (item == NULL) {
        return;
    }

    switch (item->kind) {
    case NCM_MPD_ITEM_SONG:
        ncm_song_destroy(&item->value.song);
        break;
    case NCM_MPD_ITEM_DIRECTORY:
        ncm_directory_destroy(&item->value.directory);
        break;
    case NCM_MPD_ITEM_PLAYLIST:
        ncm_playlist_destroy(&item->value.playlist);
        break;
    case NCM_MPD_ITEM_UNKNOWN:
        break;
    }

    item->kind = NCM_MPD_ITEM_UNKNOWN;
    return;
}

bool
ncm_mpd_item_copy(NcmMpdItem *dest, NcmMpdItem *source) {
    NcmMpdItem replacement;
    bool copied;

    if (dest == NULL) {
        return false;
    }
    if (source == NULL) {
        return false;
    }

    ncm_mpd_item_init(&replacement);
    replacement.kind = source->kind;
    copied = true;
    switch (source->kind) {
    case NCM_MPD_ITEM_SONG:
        ncm_song_init(&replacement.value.song);
        copied = ncm_song_copy(&replacement.value.song,
                               &source->value.song);
        break;
    case NCM_MPD_ITEM_DIRECTORY:
        ncm_directory_init(&replacement.value.directory);
        copied = ncm_directory_copy(&replacement.value.directory,
                                    &source->value.directory);
        break;
    case NCM_MPD_ITEM_PLAYLIST:
        ncm_playlist_init(&replacement.value.playlist);
        copied = ncm_playlist_copy(&replacement.value.playlist,
                                   &source->value.playlist);
        break;
    case NCM_MPD_ITEM_UNKNOWN:
        break;
    }

    if (!copied) {
        ncm_mpd_item_destroy(&replacement);
        return false;
    }

    ncm_mpd_item_destroy(dest);
    *dest = replacement;
    return true;
}

bool
ncm_mpd_item_from_mpd_song_copy(NcmMpdItem *item,
                                struct mpd_song *source) {
    if (item == NULL) {
        return false;
    }

    return ncm_mpd_item_set_song_copy(item, source);
}

bool
ncm_mpd_item_from_mpd_song_borrow(NcmMpdItem *item,
                                  struct mpd_song *source) {
    if (item == NULL) {
        return false;
    }

    return ncm_mpd_item_set_song_borrow(item, source);
}

bool
ncm_mpd_item_from_entity(NcmMpdItem *item, struct mpd_entity *entity) {
    return ncm_mpd_item_from_entity_copy(item, entity);
}

bool
ncm_mpd_item_from_entity_copy(NcmMpdItem *item,
                              struct mpd_entity *entity) {
    if (item == NULL) {
        return false;
    }
    if (entity == NULL) {
        return false;
    }

    switch (mpd_entity_get_type(entity)) {
    case MPD_ENTITY_TYPE_SONG:
        return ncm_mpd_item_set_song_copy(
            item, (struct mpd_song *)mpd_entity_get_song(entity));
    case MPD_ENTITY_TYPE_DIRECTORY:
        return ncm_mpd_item_set_directory(
            item, (struct mpd_directory *)mpd_entity_get_directory(entity));
    case MPD_ENTITY_TYPE_PLAYLIST:
        return ncm_mpd_item_set_playlist(
            item, (struct mpd_playlist *)mpd_entity_get_playlist(entity));
    default:
        return false;
    }
}

bool
ncm_mpd_item_from_entity_borrow(NcmMpdItem *item,
                                struct mpd_entity *entity) {
    if (item == NULL) {
        return false;
    }
    if (entity == NULL) {
        return false;
    }

    switch (mpd_entity_get_type(entity)) {
    case MPD_ENTITY_TYPE_SONG:
        return ncm_mpd_item_set_song_borrow(
            item, (struct mpd_song *)mpd_entity_get_song(entity));
    case MPD_ENTITY_TYPE_DIRECTORY:
        return ncm_mpd_item_set_directory(
            item, (struct mpd_directory *)mpd_entity_get_directory(entity));
    case MPD_ENTITY_TYPE_PLAYLIST:
        return ncm_mpd_item_set_playlist(
            item, (struct mpd_playlist *)mpd_entity_get_playlist(entity));
    default:
        return false;
    }
}

enum NcmMpdItemKind
ncm_mpd_item_kind(NcmMpdItem *item) {
    if (item == NULL) {
        return NCM_MPD_ITEM_UNKNOWN;
    }

    return item->kind;
}

NcmSong *
ncm_mpd_item_song(NcmMpdItem *item) {
    if (item == NULL) {
        return NULL;
    }
    if (item->kind != NCM_MPD_ITEM_SONG) {
        return NULL;
    }

    return &item->value.song;
}

NcmDirectory *
ncm_mpd_item_directory(NcmMpdItem *item) {
    if (item == NULL) {
        return NULL;
    }
    if (item->kind != NCM_MPD_ITEM_DIRECTORY) {
        return NULL;
    }

    return &item->value.directory;
}

NcmPlaylist *
ncm_mpd_item_playlist(NcmMpdItem *item) {
    if (item == NULL) {
        return NULL;
    }
    if (item->kind != NCM_MPD_ITEM_PLAYLIST) {
        return NULL;
    }

    return &item->value.playlist;
}
