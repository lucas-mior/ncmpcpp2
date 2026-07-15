#if !defined(NCMPCPP_SCREEN_TYPE_H)
#define NCMPCPP_SCREEN_TYPE_H

#include "config.h"
#include "c/ncm_defs.h"
#include "screens/nc_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

enum ScreenType {
    NCM_SCREEN_TYPE_BROWSER,
    NCM_SCREEN_TYPE_HELP,
    NCM_SCREEN_TYPE_LASTFM,
    NCM_SCREEN_TYPE_LYRICS,
    NCM_SCREEN_TYPE_MEDIA_LIBRARY,
#if defined(ENABLE_OUTPUTS)
    NCM_SCREEN_TYPE_OUTPUTS,
#endif
    NCM_SCREEN_TYPE_PLAYLIST,
    NCM_SCREEN_TYPE_PLAYLIST_EDITOR,
    NCM_SCREEN_TYPE_SEARCH_ENGINE,
    NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER,
    NCM_SCREEN_TYPE_SERVER_INFO,
    NCM_SCREEN_TYPE_SONG_INFO,
    NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG,
#if defined(HAVE_TAGLIB_H)
    NCM_SCREEN_TYPE_TAG_EDITOR,
    NCM_SCREEN_TYPE_TINY_TAG_EDITOR,
#endif
    NCM_SCREEN_TYPE_UNKNOWN,
#if defined(ENABLE_VISUALIZER)
    NCM_SCREEN_TYPE_VISUALIZER,
#endif
    NCM_SCREEN_TYPE_LAST,
};

char *screen_type_str(enum ScreenType screen_type);
int32 screen_type_to_native_type(enum ScreenType screen_type);
bool screen_type_parse_startup(char *string, int32 string_len,
                               enum ScreenType *screen_type);
bool screen_type_parse(char *string, int32 string_len,
                       enum ScreenType *screen_type);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_SCREEN_TYPE_H */
