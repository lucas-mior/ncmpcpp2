#if !defined(NCM_MPD_ITEM_C)
#define NCM_MPD_ITEM_C

#include "cbase.h"
#include "ncmpcpp2.h"

#include "c/ncm_c.h"
#include "ncmpcpp2_mpd.h"

typedef struct NcmMpdItemTagsContext {
    struct mpd_song *song;
} NcmMpdItemTagsContext;

static void
ncm_mpd_item_set_attribute(struct mpd_song *song, char *name, char *value) {
    struct mpd_pair pair;

    if ((song == NULL) || (name == NULL) || (value == NULL)) {
        return;
    }

    pair.name = name;
    pair.value = value;
    mpd_song_feed(song, &pair);
    return;
}

static void
ncm_mpd_item_mapped_property_callback(char *name, char *value, void *user) {
    NcmMpdItemTagsContext *context = user;

    if (context == NULL) {
        return;
    }

    ncm_mpd_item_set_attribute(context->song, name, value);
    return;
}

void
ncm_mpd_item_init(NcmMpdItem *item) {
    item->kind = NCM_MPD_ITEM_COUNT;
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
    case NCM_MPD_ITEM_COUNT:
    default:
        break;
    }

    item->kind = NCM_MPD_ITEM_COUNT;
    return;
}

void
ncm_mpd_item_move(NcmMpdItem *dest, NcmMpdItem *source) {
    if (dest == NULL) {
        return;
    }
    if (dest == source) {
        return;
    }

    ncm_mpd_item_destroy(dest);
    if (source == NULL) {
        ncm_mpd_item_init(dest);
        return;
    }

    *dest = *source;
    ncm_mpd_item_init(source);
    return;
}

int32
ncm_mpd_item_set_song(NcmMpdItem *item, NcmSong *source) {
    NcmMpdItem replacement;
    int32 status;

    if (item == NULL) {
        return -EINVAL;
    }
    if (source == NULL) {
        return -EINVAL;
    }

    ncm_mpd_item_init(&replacement);
    replacement.kind = NCM_MPD_ITEM_SONG;
    replacement.value.song = (NcmSong){0};
    if ((status = ncm_song_copy(&replacement.value.song, source)) < 0) {
        ncm_mpd_item_destroy(&replacement);
        return status;
    }

    ncm_mpd_item_destroy(item);
    *item = replacement;
    return 0;
}

int32
ncm_mpd_item_set_directory(NcmMpdItem *item, NcmDirectory *source) {
    NcmMpdItem replacement;
    int32 status;

    if (item == NULL) {
        return -EINVAL;
    }
    if (source == NULL) {
        return -EINVAL;
    }

    ncm_mpd_item_init(&replacement);
    replacement.kind = NCM_MPD_ITEM_DIRECTORY;
    replacement.value.directory = (NcmDirectory){0};
    if ((status = ncm_directory_copy(&replacement.value.directory,
                                     source)) < 0) {
        ncm_mpd_item_destroy(&replacement);
        return status;
    }

    ncm_mpd_item_destroy(item);
    *item = replacement;
    return 0;
}

int32
ncm_mpd_item_copy(NcmMpdItem *dest, NcmMpdItem *source) {
    NcmMpdItem replacement;
    int32 status;

    if (dest == NULL) {
        return -EINVAL;
    }
    if (source == NULL) {
        return -EINVAL;
    }

    ncm_mpd_item_init(&replacement);
    replacement.kind = source->kind;
    status = 0;
    switch (source->kind) {
    case NCM_MPD_ITEM_SONG:
        replacement.value.song = (NcmSong){0};
        status = ncm_song_copy(&replacement.value.song, &source->value.song);
        break;
    case NCM_MPD_ITEM_DIRECTORY:
        replacement.value.directory = (NcmDirectory){0};
        status = ncm_directory_copy(&replacement.value.directory,
                                    &source->value.directory);
        break;
    case NCM_MPD_ITEM_PLAYLIST:
        replacement.value.playlist = (NcmPlaylist){0};
        status = ncm_playlist_copy(&replacement.value.playlist,
                                   &source->value.playlist);
        break;
    case NCM_MPD_ITEM_COUNT:
    default:
        break;
    }

    if (status < 0) {
        ncm_mpd_item_destroy(&replacement);
        return status;
    }

    ncm_mpd_item_destroy(dest);
    *dest = replacement;
    return 0;
}

#define NCM_MPD_ITEM_TAG_ENTRY(suffix)                                      \
    {CAT(MPD_TAG_, suffix), CAT(NCM_TAG_, suffix)},

int32
ncm_mpd_item_song_from_mpd_song_copy(NcmSong *dest, void *mpd_song) {
    NcmSong replacement = {0};
    struct mpd_song *source = mpd_song;
    struct {
        enum mpd_tag_type mpd;
        enum NcmTagType ncm;
    } tags[] = {
        NCM_MPD_TAG_DEFS(NCM_MPD_ITEM_TAG_ENTRY)
    };
    char *uri;
    int32 tags_len = NCM_MPD_TAG_COUNT;
    int32 status;

    if (dest == NULL) {
        return -EINVAL;
    }
    if (source == NULL) {
        return -EINVAL;
    }

    if ((uri = (char *)mpd_song_get_uri(source)) == NULL) {
        return -NCM_ERROR_NOT_FOUND;
    }

    if ((status = ncm_song_set_uri(&replacement, uri, strlen32(uri))) < 0) {
        return status;
    }

    for (int32 i = 0; i < tags_len; i += 1) {
        for (uint32 j = 0; ; j += 1) {
            char *value = (char *)mpd_song_get_tag(source, tags[i].mpd, j);

            if (value == NULL) {
                break;
            }
            status = ncm_song_add_tag(&replacement, tags[i].ncm, value,
                                      strlen32(value));
            if (status < 0) {
                ncm_song_destroy(&replacement);
                return status;
            }
        }
    }

    replacement.duration = (int32)mpd_song_get_duration(source);
    replacement.position = (int32)mpd_song_get_pos(source);
    replacement.id = (int32)mpd_song_get_id(source);
    replacement.priority = (int32)mpd_song_get_prio(source);
    replacement.last_modified = mpd_song_get_last_modified(source);

    ncm_song_destroy(dest);
    *dest = replacement;
    return 0;
}

#undef NCM_MPD_ITEM_TAG_ENTRY

int32
ncm_mpd_item_playlist_from_mpd_playlist(NcmPlaylist *dest, void *mpd_playlist) {
    struct mpd_playlist *source = mpd_playlist;
    char *path;
    int32 path_len;
    time_t last_modified;

    if (dest == NULL) {
        return -EINVAL;
    }
    if (source == NULL) {
        return -EINVAL;
    }

    if ((path = (char *)mpd_playlist_get_path(source)) == NULL) {
        return -NCM_ERROR_NOT_FOUND;
    }

    path_len = optional_strlen32(path);
    last_modified = mpd_playlist_get_last_modified(source);
    return ncm_playlist_set(dest, path, path_len, last_modified);
}

void
ncm_mpd_item_local_song(NcmSong *song, char *path, int32 path_len,
                        time_t mtime) {
    int32 status;

    if ((song == NULL) || (path == NULL) || (path_len < 0)) {
        return;
    }

    if (ncm_song_set_uri(song, path, path_len) < 0) {
        return;
    }
    ncm_song_set_mtime(song, mtime);

#if defined(HAVE_TAGLIB_H)
    {
        struct mpd_pair pair;
        struct mpd_song *mpd_song;

        pair.name = "file";
        pair.value = path;
        mpd_song = mpd_song_begin(&pair);
        if (mpd_song == NULL) {
            return;
        }

        {
            NcmTaglibFile file = {0};
            NcmTaglibAudioProperties properties;
            NcmMpdItemTagsContext context;
            NcmTaglibPairCallback *callback;
            char time_buffer[32];
            int32 written;
            int32 count;

            status = ncm_taglib_file_open(&file,
                                          (char *)mpd_song_get_uri(mpd_song));
            if (status >= 0) {
                count = 0;
                if (ncm_taglib_file_audio_properties(&file, &properties) == 0) {
                    written = SNPRINTF(time_buffer, "%d", properties.length);
                    if (written > 0) {
                        ncm_mpd_item_set_attribute(mpd_song, "Time",
                                                   time_buffer);
                        count += 1;
                    }
                }

                context.song = mpd_song;
                callback = ncm_mpd_item_mapped_property_callback;
                status = ncm_taglib_read_mapped_properties(&file,
                                                            callback, &context);
                if (status >= 0) {
                    count += status;
                    if (count > 0) {
                        ncm_mpd_item_song_from_mpd_song_copy(song, mpd_song);
                        ncm_song_set_mtime(song, mtime);
                    }
                }
                ncm_taglib_file_close(&file);
                ncm_taglib_clear_strings();
            }
        }
        mpd_song_free(mpd_song);
    }
#else
    (void)status;
#endif

    return;
}

int32
ncm_mpd_item_from_entity_copy(NcmMpdItem *item, void *mpd_entity) {
    struct mpd_entity *entity = mpd_entity;

    if (item == NULL) {
        return -EINVAL;
    }
    if (entity == NULL) {
        return -EINVAL;
    }

    switch (mpd_entity_get_type(entity)) {
    case MPD_ENTITY_TYPE_SONG: {
        NcmMpdItem replacement = {0};
        struct mpd_song *source =
            (struct mpd_song *)mpd_entity_get_song(entity);
        NcmSong *song;
        int32 status;

        ncm_mpd_item_init(&replacement);
        replacement.kind = NCM_MPD_ITEM_SONG;
        replacement.value.song = (NcmSong){0};
        song = &replacement.value.song;
        if ((status = ncm_mpd_item_song_from_mpd_song_copy(song, source)) < 0) {
            ncm_mpd_item_destroy(&replacement);
            return status;
        }

        ncm_mpd_item_destroy(item);
        *item = replacement;
        return 0;
    }
    case MPD_ENTITY_TYPE_DIRECTORY: {
        NcmMpdItem replacement = {0};
        struct mpd_directory *source =
            (struct mpd_directory *)mpd_entity_get_directory(entity);
        char *path;
        int32 status;

        ncm_mpd_item_init(&replacement);
        replacement.kind = NCM_MPD_ITEM_DIRECTORY;
        replacement.value.directory = (NcmDirectory){0};
        if ((source == NULL)
            || ((path = (char *)mpd_directory_get_path(source)) == NULL)) {
            status = -NCM_ERROR_NOT_FOUND;
        } else {
            status = ncm_directory_set(&replacement.value.directory,
                                       path, optional_strlen32(path),
                                       mpd_directory_get_last_modified(source));
        }
        if (status < 0) {
            ncm_mpd_item_destroy(&replacement);
            return status;
        }

        ncm_mpd_item_destroy(item);
        *item = replacement;
        return 0;
    }
    case MPD_ENTITY_TYPE_PLAYLIST: {
        NcmMpdItem replacement = {0};
        struct mpd_playlist *source =
            (struct mpd_playlist *)mpd_entity_get_playlist(entity);
        NcmPlaylist *playlist;
        int32 status;

        ncm_mpd_item_init(&replacement);
        replacement.kind = NCM_MPD_ITEM_PLAYLIST;
        replacement.value.playlist = (NcmPlaylist){0};
        playlist = &replacement.value.playlist;
        status = ncm_mpd_item_playlist_from_mpd_playlist(playlist, source);
        if (status < 0) {
            ncm_mpd_item_destroy(&replacement);
            return status;
        }

        ncm_mpd_item_destroy(item);
        *item = replacement;
        return 0;
    }
    case MPD_ENTITY_TYPE_UNKNOWN:
        return -NCM_ERROR_UNAVAILABLE;
    default:
        return -NCM_ERROR_UNAVAILABLE;
    }
}

enum NcmMpdItemKind
ncm_mpd_item_kind(NcmMpdItem *item) {
    if (item == NULL) {
        return NCM_MPD_ITEM_COUNT;
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

#endif /* NCM_MPD_ITEM_C */
