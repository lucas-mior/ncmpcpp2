#if !defined(NCMPCPP_SONG_INFO_H)
#define NCMPCPP_SONG_INFO_H

#include "cbase.h"

#include "c/ncm_defs.h"
#include "c/ncm_tags.h"
#include "c/ncm_type_conversions.h"
#include "screens/app_screens.h"

typedef struct NcmSongInfoMetadata {
    char *name;
    enum NcmSongGetter get;
    enum NcmTagsField field;
} NcmSongInfoMetadata;

extern NcmSongInfoMetadata ncm_song_info_tags[];

#endif /* NCMPCPP_SONG_INFO_H */
