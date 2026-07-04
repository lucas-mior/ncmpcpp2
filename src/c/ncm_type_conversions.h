#if !defined(NCM_TYPE_CONVERSIONS_H)
#define NCM_TYPE_CONVERSIONS_H

#include <stdbool.h>
#include <mpd/tag.h>

#include "c/ncm_defs.h"

#if defined(__cplusplus)
extern "C" {
#endif

enum NcmItemType {
    NCM_ITEM_DIRECTORY,
    NCM_ITEM_SONG,
    NCM_ITEM_PLAYLIST,
    NCM_ITEM_UNKNOWN,
};

enum NcmSongGetter {
    NCM_SONG_GETTER_NONE,
    NCM_SONG_GETTER_LENGTH,
    NCM_SONG_GETTER_DIRECTORY,
    NCM_SONG_GETTER_NAME,
    NCM_SONG_GETTER_URI,
    NCM_SONG_GETTER_ARTIST,
    NCM_SONG_GETTER_ALBUM_ARTIST,
    NCM_SONG_GETTER_TITLE,
    NCM_SONG_GETTER_ALBUM,
    NCM_SONG_GETTER_DATE,
    NCM_SONG_GETTER_TRACK_NUMBER,
    NCM_SONG_GETTER_TRACK,
    NCM_SONG_GETTER_GENRE,
    NCM_SONG_GETTER_COMPOSER,
    NCM_SONG_GETTER_PERFORMER,
    NCM_SONG_GETTER_DISC,
    NCM_SONG_GETTER_COMMENT,
    NCM_SONG_GETTER_PRIORITY,
};

int32 ncm_channels_to_string(int32 channels, char *buffer, int32 buffer_cap);
int32 ncm_color_index_from_char(char c);
char *ncm_tag_type_name(enum mpd_tag_type tag);
bool ncm_char_is_tag_type(char c);
enum mpd_tag_type ncm_char_to_tag_type(char c);
enum NcmSongGetter ncm_song_getter_from_char(char c);
enum NcmSongGetter ncm_song_getter_from_tag_type(enum mpd_tag_type tag);
enum mpd_tag_type ncm_song_getter_to_tag_type(enum NcmSongGetter getter);
char *ncm_item_type_name(enum NcmItemType type);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_TYPE_CONVERSIONS_H */
