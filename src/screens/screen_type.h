#if !defined(NCMPCPP_SCREEN_TYPE_H)
#define NCMPCPP_SCREEN_TYPE_H

#include "config.h"
#include "c/ncm_defs.h"
#include "screens/nc_screen.h"

#if defined(ENABLE_OUTPUTS)
#define NCM_SCREEN_TYPE_OUTPUTS_FIELD X(NCM_SCREEN_TYPE_OUTPUTS)
#else
#define NCM_SCREEN_TYPE_OUTPUTS_FIELD
#endif

#if defined(HAVE_TAGLIB_H)
#define NCM_SCREEN_TYPE_TAG_EDITOR_FIELDS \
    X(NCM_SCREEN_TYPE_TAG_EDITOR) \
    X(NCM_SCREEN_TYPE_TINY_TAG_EDITOR)
#else
#define NCM_SCREEN_TYPE_TAG_EDITOR_FIELDS
#endif

#if defined(ENABLE_VISUALIZER)
#define NCM_SCREEN_TYPE_VISUALIZER_FIELD X(NCM_SCREEN_TYPE_VISUALIZER)
#else
#define NCM_SCREEN_TYPE_VISUALIZER_FIELD
#endif

#define ENUM_NAME ScreenType
#define ENUM_PREFIX_ NCM_SCREEN_TYPE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(NCM_SCREEN_TYPE_BROWSER) \
    X(NCM_SCREEN_TYPE_HELP) \
    X(NCM_SCREEN_TYPE_LASTFM) \
    X(NCM_SCREEN_TYPE_LYRICS) \
    X(NCM_SCREEN_TYPE_MEDIA_LIBRARY) \
    NCM_SCREEN_TYPE_OUTPUTS_FIELD \
    X(NCM_SCREEN_TYPE_PLAYLIST) \
    X(NCM_SCREEN_TYPE_PLAYLIST_EDITOR) \
    X(NCM_SCREEN_TYPE_SEARCH_ENGINE) \
    X(NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER) \
    X(NCM_SCREEN_TYPE_SERVER_INFO) \
    X(NCM_SCREEN_TYPE_SONG_INFO) \
    X(NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG) \
    NCM_SCREEN_TYPE_TAG_EDITOR_FIELDS \
    X(NCM_SCREEN_TYPE_UNKNOWN) \
    NCM_SCREEN_TYPE_VISUALIZER_FIELD
#include "cbase/xenums.c"
#undef NCM_SCREEN_TYPE_OUTPUTS_FIELD
#undef NCM_SCREEN_TYPE_TAG_EDITOR_FIELDS
#undef NCM_SCREEN_TYPE_VISUALIZER_FIELD

int32 screen_type_to_native_type(enum ScreenType screen_type);
bool screen_type_parse_startup(char *string, int32 string_len,
                               enum ScreenType *screen_type);
bool screen_type_parse(char *string, int32 string_len,
                       enum ScreenType *screen_type);

#endif /* NCMPCPP_SCREEN_TYPE_H */
