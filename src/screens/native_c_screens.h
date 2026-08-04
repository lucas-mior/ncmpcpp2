#if !defined(NCMPCPP_NATIVE_C_SCREENS_H)
#define NCMPCPP_NATIVE_C_SCREENS_H

#include "cbase.h"

#include "c/ncm_defs.h"
#include "screens/nc_browser.h"
#include "screens/nc_help.h"
#include "screens/nc_lastfm.h"
#include "screens/nc_lyrics.h"
#include "screens/nc_media_library.h"
#include "screens/nc_outputs.h"
#include "screens/nc_playlist.h"
#include "screens/nc_playlist_editor.h"
#include "screens/nc_screen.h"
#include "screens/nc_sel_items_adder.h"
#include "screens/nc_server_info.h"
#include "screens/nc_song_info.h"
#include "screens/nc_sort_playlist.h"
#include "screens/nc_tag_editor.h"
#include "screens/nc_tiny_tag_editor.h"
#include "screens/nc_visualizer.h"
#include "screens/screen_type.h"

typedef struct NativeHelpScreen NativeHelpScreen;
typedef struct NativeOutputsScreen NativeOutputsScreen;
typedef struct NativeSearchEngineScreen NativeSearchEngineScreen;
typedef struct NativeServerInfoScreen NativeServerInfoScreen;
typedef struct NativeSongInfoScreen NativeSongInfoScreen;

void native_c_screens_init_all(void);
void native_c_screens_register_native_only(void);
void native_c_screens_request_registered_resize(void);
NcScreen *native_c_screens_find_type(enum ScreenType screen_type);
bool native_c_screens_switch_to_type(enum ScreenType screen_type);
bool native_c_screens_lock_current(void);
enum ScreenType native_c_screens_current_type(void);

#define NCM_NATIVE_DECLARE_COMMON(suffix) \
    void native_c_screen_##suffix##_init(void); \
    void native_c_screen_##suffix##_register(void); \
    bool native_c_screen_##suffix##_is_current(void); \
    NcScreen *native_c_screen_##suffix##_native(void);

NCM_NATIVE_IS_CURRENT_TYPES(NCM_NATIVE_DECLARE_COMMON)

#undef NCM_NATIVE_DECLARE_COMMON

#define NCM_NATIVE_DECLARE_DIRECT_ACCESSOR( \
    suffix, type, storage, native_expr \
) \
    type *native_c_screen_##suffix(void);

NCM_NATIVE_DIRECT_ACCESSOR_TYPES(NCM_NATIVE_DECLARE_DIRECT_ACCESSOR)

#undef NCM_NATIVE_DECLARE_DIRECT_ACCESSOR

#define NCM_NATIVE_DECLARE_TYPED_WRAPPED_ACCESSOR( \
    suffix, function, type, expr \
) \
    type *function(void);

NCM_NATIVE_TYPED_WRAPPED_ACCESSOR_TYPES(
    NCM_NATIVE_DECLARE_TYPED_WRAPPED_ACCESSOR)

#undef NCM_NATIVE_DECLARE_TYPED_WRAPPED_ACCESSOR

#define NCM_NATIVE_DECLARE_VOID_SWITCH(suffix) \
    void native_c_screen_##suffix##_switch_to(void);

NCM_NATIVE_SIMPLE_SWITCH_TYPES(NCM_NATIVE_DECLARE_VOID_SWITCH)
NCM_NATIVE_REGISTER_SWITCH_TYPES(NCM_NATIVE_DECLARE_VOID_SWITCH)

#undef NCM_NATIVE_DECLARE_VOID_SWITCH

void native_c_screen_lastfm_switch_to(void);
NativeVisualizerScreen *native_c_screen_visualizer(void);
void native_c_screen_lyrics_set_resize(void);
void native_c_screen_lyrics_switch_to(void);
void native_c_screen_browser_fetch_supported_extensions(void);
bool native_c_screen_selected_items_adder_open(
    NcmSongArray *songs,
    NcmError *error
);
bool native_c_screen_sort_playlist_dialog_switch_to(void);
void native_c_screen_outputs_toggle(void);
void native_c_screen_outputs_fetch_list(void);
void native_c_screen_outputs_refresh_if_visible(void);

#endif /* NCMPCPP_NATIVE_C_SCREENS_H */
