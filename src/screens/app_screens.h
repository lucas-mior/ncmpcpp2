#if !defined(NCMPCPP_APP_SCREENS_H)
#define NCMPCPP_APP_SCREENS_H

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

typedef struct HelpScreen HelpScreen;
typedef struct OutputsScreen OutputsScreen;
typedef struct SearchEngineScreen SearchEngineScreen;
typedef struct ServerInfoScreen ServerInfoScreen;
typedef struct SongInfoScreen SongInfoScreen;

void app_screens_init_all(void);
void app_screens_register_initial(void);
void app_screens_request_registered_resize(void);
NcScreen *app_screens_find_type(enum ScreenType screen_type);
bool app_screens_switch_to_type(enum ScreenType screen_type);
bool app_screens_lock_current(void);
enum ScreenType app_screens_current_type(void);

#define NCM_APP_SCREEN_DECLARE_COMMON(suffix) \
    void app_screen_##suffix##_init(void); \
    void app_screen_##suffix##_register(void); \
    bool app_screen_##suffix##_is_current(void); \
    NcScreen *app_screen_##suffix##_base(void);

NCM_APP_SCREEN_IS_CURRENT_TYPES(NCM_APP_SCREEN_DECLARE_COMMON)

#undef NCM_APP_SCREEN_DECLARE_COMMON

#define NCM_APP_SCREEN_DECLARE_DIRECT_ACCESSOR( \
    suffix, type, storage, base_expr \
) \
    type *app_screen_##suffix(void);

NCM_APP_SCREEN_DIRECT_ACCESSOR_TYPES(NCM_APP_SCREEN_DECLARE_DIRECT_ACCESSOR)

#undef NCM_APP_SCREEN_DECLARE_DIRECT_ACCESSOR

#define NCM_APP_SCREEN_DECLARE_TYPED_WRAPPED_ACCESSOR( \
    suffix, function, type, expr \
) \
    type *function(void);

NCM_APP_SCREEN_TYPED_WRAPPED_ACCESSOR_TYPES(
    NCM_APP_SCREEN_DECLARE_TYPED_WRAPPED_ACCESSOR)

#undef NCM_APP_SCREEN_DECLARE_TYPED_WRAPPED_ACCESSOR

#define NCM_APP_SCREEN_DECLARE_VOID_SWITCH(suffix) \
    void app_screen_##suffix##_switch_to(void);

NCM_APP_SCREEN_SIMPLE_SWITCH_TYPES(NCM_APP_SCREEN_DECLARE_VOID_SWITCH)
NCM_APP_SCREEN_REGISTER_SWITCH_TYPES(NCM_APP_SCREEN_DECLARE_VOID_SWITCH)

#undef NCM_APP_SCREEN_DECLARE_VOID_SWITCH

void app_screen_lastfm_switch_to(void);
VisualizerScreen *app_screen_visualizer(void);
void app_screen_lyrics_set_resize(void);
void app_screen_lyrics_switch_to(void);
void app_screen_browser_fetch_supported_extensions(void);
bool app_screen_selected_items_adder_open(
    NcmSongArray *songs,
    NcmError *ncm_error
);
bool app_screen_sort_playlist_dialog_switch_to(void);
void app_screen_outputs_toggle(void);
void app_screen_outputs_fetch_list(void);
void app_screen_outputs_refresh_if_visible(void);

#endif /* NCMPCPP_APP_SCREENS_H */
