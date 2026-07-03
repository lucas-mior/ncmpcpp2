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

int32 ncm_channels_to_string(int32 channels, char *buffer, int32 buffer_cap);
int32 ncm_color_index_from_char(char c);
char *ncm_tag_type_name(enum mpd_tag_type tag);
bool ncm_char_is_tag_type(char c);
enum mpd_tag_type ncm_char_to_tag_type(char c);
char *ncm_item_type_name(enum NcmItemType type);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_TYPE_CONVERSIONS_H */
