#if !defined(NCMPCPP_NC_SCREENS_H)
#define NCMPCPP_NC_SCREENS_H

#include "cbase.h"

#include "c/ncm_c.h"
#include "curses/nc_curses.h"
#include "configura.h"
#include "lastfm_service.h"
#include "lyrics_fetcher.h"

#include <fftw3.h>

/* screens/screen_defs.h */
#include "configura.h"

#define NCM_SCREEN_FLAG_NONE 0
#define NCM_SCREEN_FLAG_STARTUP 1

#define NCM_SCREEN_TYPE_BROWSER_ENTRY(XX)                                      \
    XX(NCM_SCREEN_TYPE_BROWSER, NC_SCREEN_TYPE_BROWSER, 1, browser,            \
      NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_HELP_ENTRY(XX)                                         \
    XX(NCM_SCREEN_TYPE_HELP, NC_SCREEN_TYPE_HELP, 2, help,                     \
      NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_LASTFM_ENTRY(XX)                                       \
    XX(NCM_SCREEN_TYPE_LASTFM, NC_SCREEN_TYPE_LASTFM, 3, last_fm,              \
      NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_LYRICS_ENTRY(XX)                                       \
    XX(NCM_SCREEN_TYPE_LYRICS, NC_SCREEN_TYPE_LYRICS, 4, lyrics,               \
      NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_MEDIA_LIBRARY_ENTRY(XX)                                \
    XX(NCM_SCREEN_TYPE_MEDIA_LIBRARY, NC_SCREEN_TYPE_MEDIA_LIBRARY, 5,         \
      media_library, NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_OUTPUTS_ENTRY(XX)                                      \
    XX(NCM_SCREEN_TYPE_OUTPUTS, NC_SCREEN_TYPE_OUTPUTS, 6, outputs,            \
      NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_PLAYLIST_ENTRY(XX)                                     \
    XX(NCM_SCREEN_TYPE_PLAYLIST, NC_SCREEN_TYPE_PLAYLIST, 7, playlist,         \
      NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_PLAYLIST_EDITOR_ENTRY(XX)                              \
    XX(NCM_SCREEN_TYPE_PLAYLIST_EDITOR, NC_SCREEN_TYPE_PLAYLIST_EDITOR,        \
      8, playlist_editor, NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_SEARCH_ENGINE_ENTRY(XX)                                \
    XX(NCM_SCREEN_TYPE_SEARCH_ENGINE, NC_SCREEN_TYPE_SEARCH_ENGINE,            \
      9, search_engine, NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER_ENTRY(XX)                         \
    XX(NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER,                                   \
      NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER, 10, selected_items_adder,           \
      NCM_SCREEN_FLAG_NONE)
#define NCM_SCREEN_TYPE_SERVER_INFO_ENTRY(XX)                                  \
    XX(NCM_SCREEN_TYPE_SERVER_INFO, NC_SCREEN_TYPE_SERVER_INFO, 11,            \
      server_info, NCM_SCREEN_FLAG_NONE)
#define NCM_SCREEN_TYPE_SONG_INFO_ENTRY(XX)                                    \
    XX(NCM_SCREEN_TYPE_SONG_INFO, NC_SCREEN_TYPE_SONG_INFO, 12,                \
      song_info, NCM_SCREEN_FLAG_NONE)
#define NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG_ENTRY(XX)                         \
    XX(NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG,                                   \
      NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG, 13, sort_playlist_dialog,           \
      NCM_SCREEN_FLAG_NONE)
#define NCM_SCREEN_TYPE_TAG_EDITOR_ENTRY(XX)                                   \
    XX(NCM_SCREEN_TYPE_TAG_EDITOR, NC_SCREEN_TYPE_TAG_EDITOR, 14,              \
      tag_editor, NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_TINY_TAG_EDITOR_ENTRY(XX)                              \
    XX(NCM_SCREEN_TYPE_TINY_TAG_EDITOR, NC_SCREEN_TYPE_TINY_TAG_EDITOR,        \
      15, tiny_tag_editor, NCM_SCREEN_FLAG_NONE)
#define NCM_SCREEN_TYPE_VISUALIZER_ENTRY(XX)                                   \
    XX(NCM_SCREEN_TYPE_VISUALIZER, NC_SCREEN_TYPE_VISUALIZER, 16,              \
      visualizer, NCM_SCREEN_FLAG_STARTUP)

#define NCM_SCREEN_ALL_TYPES(XX)                                               \
    NCM_SCREEN_TYPE_BROWSER_ENTRY(XX)                                          \
    NCM_SCREEN_TYPE_HELP_ENTRY(XX)                                             \
    NCM_SCREEN_TYPE_LASTFM_ENTRY(XX)                                           \
    NCM_SCREEN_TYPE_LYRICS_ENTRY(XX)                                           \
    NCM_SCREEN_TYPE_MEDIA_LIBRARY_ENTRY(XX)                                    \
    NCM_SCREEN_TYPE_OUTPUTS_ENTRY(XX)                                          \
    NCM_SCREEN_TYPE_PLAYLIST_ENTRY(XX)                                         \
    NCM_SCREEN_TYPE_PLAYLIST_EDITOR_ENTRY(XX)                                  \
    NCM_SCREEN_TYPE_SEARCH_ENGINE_ENTRY(XX)                                    \
    NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER_ENTRY(XX)                             \
    NCM_SCREEN_TYPE_SERVER_INFO_ENTRY(XX)                                      \
    NCM_SCREEN_TYPE_SONG_INFO_ENTRY(XX)                                        \
    NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG_ENTRY(XX)                             \
    NCM_SCREEN_TYPE_TAG_EDITOR_ENTRY(XX)                                       \
    NCM_SCREEN_TYPE_TINY_TAG_EDITOR_ENTRY(XX)                                  \
    NCM_SCREEN_TYPE_VISUALIZER_ENTRY(XX)

#if defined(ENABLE_OUTPUTS)
  #define NCM_SCREEN_ENABLED_OUTPUTS_TYPES(XX)                                 \
      NCM_SCREEN_TYPE_OUTPUTS_ENTRY(XX)
#else
  #define NCM_SCREEN_ENABLED_OUTPUTS_TYPES(XX)
#endif

#if defined(HAVE_TAGLIB_H)
  #define NCM_SCREEN_ENABLED_TAG_EDITOR_TYPES(XX)                              \
      NCM_SCREEN_TYPE_TAG_EDITOR_ENTRY(XX)                                     \
      NCM_SCREEN_TYPE_TINY_TAG_EDITOR_ENTRY(XX)
#else
  #define NCM_SCREEN_ENABLED_TAG_EDITOR_TYPES(XX)
#endif

#if defined(ENABLE_VISUALIZER)
  #define NCM_SCREEN_ENABLED_VISUALIZER_TYPES(XX)                              \
      NCM_SCREEN_TYPE_VISUALIZER_ENTRY(XX)
#else
  #define NCM_SCREEN_ENABLED_VISUALIZER_TYPES(XX)
#endif

#define NCM_SCREEN_TYPES(XX)                                                   \
    NCM_SCREEN_TYPE_BROWSER_ENTRY(XX)                                          \
    NCM_SCREEN_TYPE_HELP_ENTRY(XX)                                             \
    NCM_SCREEN_TYPE_LASTFM_ENTRY(XX)                                           \
    NCM_SCREEN_TYPE_LYRICS_ENTRY(XX)                                           \
    NCM_SCREEN_TYPE_MEDIA_LIBRARY_ENTRY(XX)                                    \
    NCM_SCREEN_ENABLED_OUTPUTS_TYPES(XX)                                       \
    NCM_SCREEN_TYPE_PLAYLIST_ENTRY(XX)                                         \
    NCM_SCREEN_TYPE_PLAYLIST_EDITOR_ENTRY(XX)                                  \
    NCM_SCREEN_TYPE_SEARCH_ENGINE_ENTRY(XX)                                    \
    NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER_ENTRY(XX)                             \
    NCM_SCREEN_TYPE_SERVER_INFO_ENTRY(XX)                                      \
    NCM_SCREEN_TYPE_SONG_INFO_ENTRY(XX)                                        \
    NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG_ENTRY(XX)                             \
    NCM_SCREEN_ENABLED_TAG_EDITOR_TYPES(XX)                                    \
    NCM_SCREEN_ENABLED_VISUALIZER_TYPES(XX)

#define NCM_SCREEN_NC_TYPE_ENUM_FIELD(                                         \
    screen_type, nc_type, nc_value, alias, flags                               \
)                                                                              \
    nc_type = nc_value,

#define NCM_SCREEN_TYPE_XENUM_FIELD(                                           \
    screen_type, nc_type, nc_value, alias, flags                               \
)                                                                              \
    XX(screen_type, alias)

#define NCM_SCREEN_TYPE_ENUM_FIELDS                                            \
    NCM_SCREEN_TYPES(NCM_SCREEN_TYPE_XENUM_FIELD)


#define NCM_APP_SCREEN_DIRECT_STORAGE_TYPES(XX)                                \
    XX(BrowserScreen, browser_screen)                                          \
    XX(LastfmScreen, lastfm_screen)                                            \
    XX(LyricsScreen, lyrics_screen)                                            \
    XX(VisualizerScreen, visualizer_screen)                                    \
    XX(PlaylistScreen, playlist_screen)                                        \
    XX(PlaylistEditorScreen, playlist_editor_screen)                           \
    XX(SelectedItemsAdderScreen, selected_items_adder_screen)                  \
    XX(SortPlaylistDialog, sort_playlist_dialog)                               \
    XX(SearchEngineScreen, search_engine_screen)                               \
    XX(MediaLibraryScreen, media_library_screen)                               \
    XX(TagEditorScreen, tag_editor_screen)                                     \
    XX(TinyTagEditorScreen, tiny_tag_editor_screen)

#define NCM_APP_SCREEN_WRAPPED_STORAGE_TYPES(XX)                               \
    XX(HelpScreen, help_screen)                                                \
    XX(OutputsScreen, outputs_screen)                                          \
    XX(ServerInfoScreen, server_info_screen)                                   \
    XX(SongInfoScreen, song_info_screen)

#define NCM_APP_SCREEN_INIT_FLAGS(XX)                                          \
    XX(browser_screen_initialized)                                             \
    XX(lastfm_screen_initialized)                                              \
    XX(lyrics_screen_initialized)                                              \
    XX(visualizer_screen_initialized)                                          \
    XX(playlist_editor_screen_initialized)                                     \
    XX(selected_items_adder_screen_initialized)                                \
    XX(sort_playlist_dialog_initialized)                                       \
    XX(search_engine_screen_initialized)                                       \
    XX(media_library_screen_initialized)                                       \
    XX(tag_editor_screen_initialized)                                          \
    XX(tiny_tag_editor_screen_initialized)                                     \
    XX(playlist_screen_initialized)

#define NCM_APP_SCREEN_DIRECT_ACCESSOR_TYPES(XX)                               \
    XX(browser, BrowserScreen, browser_screen,                                 \
      browser_screen_base(&browser_screen))                                    \
    XX(lastfm, LastfmScreen, lastfm_screen,                                    \
      lastfm_screen_base(&lastfm_screen))                                      \
    XX(lyrics, LyricsScreen, lyrics_screen,                                    \
      lyrics_screen_base(&lyrics_screen))                                      \
    XX(playlist, PlaylistScreen, playlist_screen,                              \
      playlist_screen_base(&playlist_screen))                                  \
    XX(playlist_editor, PlaylistEditorScreen, playlist_editor_screen,          \
      playlist_editor_screen_base(&playlist_editor_screen))                    \
    XX(selected_items_adder, SelectedItemsAdderScreen,                         \
      selected_items_adder_screen,                                             \
      selected_items_adder_screen_base(&selected_items_adder_screen))          \
    XX(sort_playlist_dialog, SortPlaylistDialog, sort_playlist_dialog,         \
      sort_playlist_dialog_base(&sort_playlist_dialog))                        \
    XX(search_engine, SearchEngineScreen, search_engine_screen,                \
      search_engine_screen_base(&search_engine_screen))                        \
    XX(media_library, MediaLibraryScreen, media_library_screen,                \
      media_library_screen_base(&media_library_screen))                        \
    XX(tag_editor, TagEditorScreen, tag_editor_screen,                         \
      tag_editor_screen_base(&tag_editor_screen))                              \
    XX(tiny_tag_editor, TinyTagEditorScreen, tiny_tag_editor_screen,           \
      tiny_tag_editor_screen_base(&tiny_tag_editor_screen))

#define NCM_APP_SCREEN_WRAPPED_ACCESSOR_TYPES(XX)                              \
    XX(help, nc_help_screen_base(&help_screen.screen))                         \
    XX(server_info, nc_server_info_screen_base(&server_info_screen.screen))    \
    XX(song_info, nc_song_info_screen_base(&song_info_screen.screen))

#define NCM_APP_SCREEN_TYPED_WRAPPED_ACCESSOR_TYPES(XX)                        \
    XX(help, app_screen_help, NcHelpScreen, &help_screen.screen)

#define NCM_APP_SCREEN_STANDARD_REGISTER_TYPES(XX)                             \
    XX(browser)                                                                \
    XX(help)                                                                   \
    XX(lastfm)                                                                 \
    XX(lyrics)                                                                 \
    XX(visualizer)                                                             \
    XX(playlist)                                                               \
    XX(playlist_editor)                                                        \
    XX(search_engine)                                                          \
    XX(media_library)                                                          \
    XX(tag_editor)                                                             \
    XX(tiny_tag_editor)                                                        \
    XX(song_info)                                                              \
    XX(server_info)                                                            \
    XX(outputs)

#define NCM_APP_SCREEN_REPLACE_REGISTER_TYPES(XX)                              \
    XX(selected_items_adder, NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER)              \
    XX(sort_playlist_dialog, NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG)

#define NCM_APP_SCREEN_SIMPLE_SWITCH_TYPES(XX)                                 \
    XX(browser)                                                                \
    XX(help)                                                                   \
    XX(playlist)                                                               \
    XX(playlist_editor)                                                        \
    XX(selected_items_adder)                                                   \
    XX(search_engine)                                                          \
    XX(media_library)                                                          \
    XX(tag_editor)                                                             \
    XX(song_info)                                                              \
    XX(server_info)                                                            \
    XX(outputs)

#define NCM_APP_SCREEN_REGISTER_SWITCH_TYPES(XX)                               \
    XX(tiny_tag_editor)

#define NCM_APP_SCREEN_IS_CURRENT_TYPES(XX)                                    \
    XX(browser)                                                                \
    XX(help)                                                                   \
    XX(lastfm)                                                                 \
    XX(lyrics)                                                                 \
    XX(visualizer)                                                             \
    XX(playlist)                                                               \
    XX(playlist_editor)                                                        \
    XX(selected_items_adder)                                                   \
    XX(sort_playlist_dialog)                                                   \
    XX(search_engine)                                                          \
    XX(media_library)                                                          \
    XX(tag_editor)                                                             \
    XX(tiny_tag_editor)                                                        \
    XX(song_info)                                                              \
    XX(server_info)                                                            \
    XX(outputs)

#if defined(ENABLE_OUTPUTS)
  #define NCM_APP_SCREEN_ENABLED_OUTPUTS(XX) XX(outputs)
  #define NCM_APP_SCREEN_ENABLED_OUTPUTS_RESIZE(XX)                            \
      XX(outputs, NC_SCREEN_TYPE_OUTPUTS)
#else
  #define NCM_APP_SCREEN_ENABLED_OUTPUTS(XX)
  #define NCM_APP_SCREEN_ENABLED_OUTPUTS_RESIZE(XX)
#endif

#if defined(HAVE_TAGLIB_H)
  #define NCM_APP_SCREEN_ENABLED_TAG_EDITOR(XX)                                \
      XX(tag_editor)                                                           \
      XX(tiny_tag_editor)
  #define NCM_APP_SCREEN_ENABLED_TAG_EDITOR_RESIZE(XX)                         \
      XX(tag_editor, NC_SCREEN_TYPE_TAG_EDITOR)                                \
      XX(tiny_tag_editor, NC_SCREEN_TYPE_TINY_TAG_EDITOR)
#else
  #define NCM_APP_SCREEN_ENABLED_TAG_EDITOR(XX)
  #define NCM_APP_SCREEN_ENABLED_TAG_EDITOR_RESIZE(XX)
#endif

#if defined(ENABLE_VISUALIZER)
  #define NCM_APP_SCREEN_ENABLED_VISUALIZER(XX) XX(visualizer)
  #define NCM_APP_SCREEN_ENABLED_VISUALIZER_RESIZE(XX)                         \
      XX(visualizer, NC_SCREEN_TYPE_VISUALIZER)
#else
  #define NCM_APP_SCREEN_ENABLED_VISUALIZER(XX)
  #define NCM_APP_SCREEN_ENABLED_VISUALIZER_RESIZE(XX)
#endif

#define NCM_APP_SCREEN_INIT_ALL_TYPES(XX)                                      \
    XX(browser)                                                                \
    XX(help)                                                                   \
    XX(lastfm)                                                                 \
    XX(lyrics)                                                                 \
    XX(media_library)                                                          \
    XX(playlist)                                                               \
    XX(playlist_editor)                                                        \
    XX(search_engine)                                                          \
    XX(selected_items_adder)                                                   \
    XX(server_info)                                                            \
    XX(song_info)                                                              \
    XX(sort_playlist_dialog)                                                   \
    NCM_APP_SCREEN_ENABLED_TAG_EDITOR(XX)                                      \
    NCM_APP_SCREEN_ENABLED_VISUALIZER(XX)                                      \
    NCM_APP_SCREEN_ENABLED_OUTPUTS(XX)

#define NCM_APP_SCREEN_REGISTER_INITIAL_TYPES(XX)                              \
    XX(browser)                                                                \
    XX(help)                                                                   \
    XX(lastfm)                                                                 \
    XX(media_library)                                                          \
    XX(search_engine)                                                          \
    XX(selected_items_adder)                                                   \
    XX(song_info)                                                              \
    XX(server_info)                                                            \
    NCM_APP_SCREEN_ENABLED_VISUALIZER(XX)                                      \
    NCM_APP_SCREEN_ENABLED_TAG_EDITOR(XX)                                      \
    NCM_APP_SCREEN_ENABLED_OUTPUTS(XX)                                         \
    XX(playlist)                                                               \
    XX(playlist_editor)

#define NCM_APP_SCREEN_RESIZE_REQUEST_TYPES(XX)                                \
    XX(browser, NC_SCREEN_TYPE_BROWSER)                                        \
    XX(help, NC_SCREEN_TYPE_HELP)                                              \
    XX(lastfm, NC_SCREEN_TYPE_LASTFM)                                          \
    XX(lyrics, NC_SCREEN_TYPE_LYRICS)                                          \
    XX(media_library, NC_SCREEN_TYPE_MEDIA_LIBRARY)                            \
    XX(playlist, NC_SCREEN_TYPE_PLAYLIST)                                      \
    XX(playlist_editor, NC_SCREEN_TYPE_PLAYLIST_EDITOR)                        \
    XX(search_engine, NC_SCREEN_TYPE_SEARCH_ENGINE)                            \
    XX(selected_items_adder, NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER)              \
    XX(server_info, NC_SCREEN_TYPE_SERVER_INFO)                                \
    XX(song_info, NC_SCREEN_TYPE_SONG_INFO)                                    \
    XX(sort_playlist_dialog, NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG)              \
    NCM_APP_SCREEN_ENABLED_TAG_EDITOR_RESIZE(XX)                               \
    NCM_APP_SCREEN_ENABLED_VISUALIZER_RESIZE(XX)                               \
    NCM_APP_SCREEN_ENABLED_OUTPUTS_RESIZE(XX)

/* screens/nc_screen.h */
#define NC_SCREEN_DEFAULT_WINDOW_TIMEOUT 500

enum {
    NC_SCREEN_TYPE_UNKNOWN = 0,
    NCM_SCREEN_ALL_TYPES(NCM_SCREEN_NC_TYPE_ENUM_FIELD)
};

#define NC_SCREEN_REGISTRY_MAX_SCREENS 64

typedef struct NcScreen NcScreen;
typedef struct NcScreenRegistry NcScreenRegistry;
typedef struct NcScreenResizeParams NcScreenResizeParams;
typedef void NcScreenEachCallback(NcScreen *screen, void *user);

typedef struct NcScreenResizeParams {
    int32 x_offset;
    int32 width;
} NcScreenResizeParams;

typedef struct NcScreenCallbacks {
    NcWindow *(*active_window)(NcScreen *screen);
    void (*refresh)(NcScreen *screen);
    void (*refresh_window)(NcScreen *screen);
    void (*scroll)(NcScreen *screen, enum NcScroll where);
    void (*list_change_finished)(NcScreen *screen);
    bool (*can_run_current)(NcScreen *screen);
    int32 (*run_current)(NcScreen *screen);
    void (*switch_to)(NcScreen *screen);
    void (*resize)(NcScreen *screen);
    int32 (*window_timeout)(NcScreen *screen);
    char *(*title)(NcScreen *screen);
    void (*update)(NcScreen *screen);
    void (*mouse_button_pressed)(NcScreen *screen, MEVENT event);
    bool (*is_lockable)(NcScreen *screen);
    bool (*is_mergable)(NcScreen *screen);
    void (*destroy)(NcScreen *screen);
} NcScreenCallbacks;

typedef struct NcScreenOps {
    NcWindow *(*active_window)(NcScreen *screen);
    void (*refresh)(NcScreen *screen);
    void (*refresh_window)(NcScreen *screen);
    void (*scroll)(NcScreen *screen, enum NcScroll where);
    void (*list_change_finished)(NcScreen *screen);
    bool (*can_run_current)(NcScreen *screen);
    int32 (*run_current)(NcScreen *screen);
    void (*switch_to)(NcScreen *screen);
    void (*resize)(NcScreen *screen);
    int32 (*window_timeout_callback)(NcScreen *screen);
    char *(*title)(NcScreen *screen);
    void (*update)(NcScreen *screen);
    void (*mouse_button_pressed)(NcScreen *screen, MEVENT event);
    bool (*is_lockable_callback)(NcScreen *screen);
    bool (*is_mergable_callback)(NcScreen *screen);
    void (*destroy)(NcScreen *screen);

    int32 window_timeout;

    bool lockable;
    bool mergable;
} NcScreenOps;

struct NcScreen {
    NcScreenCallbacks callbacks;
    const NcScreenOps *ops;
    NcScreenOps ops_storage;
    void *user;
    int32 type;
    bool has_to_be_resized;
    bool has_to_be_updated;
};

extern const NcScreenOps nc_screen_default_ops;

struct NcScreenRegistry {
    NcScreen *screens[NC_SCREEN_REGISTRY_MAX_SCREENS];
    NcScreen *current_screen;
    NcScreen *previous_screen;
    NcScreen *locked_screen;
    NcScreen *inactive_screen;
    int32 screens_len;
};

void nc_screen_init(NcScreen *screen, NcScreenCallbacks callbacks,
                    void *user, int32 type);
void nc_screen_init_ops(NcScreen *screen, NcScreenOps ops,
                        void *user, int32 type);
NcWindow *nc_screen_default_active_window(NcScreen *screen);
void nc_screen_noop_refresh(NcScreen *screen);
void nc_screen_noop_refresh_window(NcScreen *screen);
void nc_screen_noop_scroll(NcScreen *screen, enum NcScroll where);
void nc_screen_noop_list_change_finished(NcScreen *screen);
bool nc_screen_default_can_run_current(NcScreen *screen);
int32 nc_screen_default_run_current(NcScreen *screen);
void nc_screen_noop_switch_to(NcScreen *screen);
void nc_screen_noop_resize(NcScreen *screen);
char *nc_screen_default_title(NcScreen *screen);
void nc_screen_noop_update(NcScreen *screen);
void nc_screen_noop_mouse_button_pressed(NcScreen *screen, MEVENT event);
void nc_screen_noop_destroy(NcScreen *screen);
NcWindow *nc_screen_active_window(NcScreen *screen);
void nc_screen_refresh(NcScreen *screen);
void nc_screen_refresh_window(NcScreen *screen);
void nc_screen_scroll(NcScreen *screen, enum NcScroll where);
void nc_screen_finish_list_change(NcScreen *screen);
bool nc_screen_can_run_current(NcScreen *screen);
int32 nc_screen_run_current(NcScreen *screen);
void nc_screen_switch_to(NcScreen *screen);
void nc_screen_resize(NcScreen *screen);
int32 nc_screen_window_timeout(NcScreen *screen);
char *nc_screen_title(NcScreen *screen);
int32 nc_screen_type(NcScreen *screen);
void nc_screen_mouse_button_pressed(NcScreen *screen, MEVENT event);
bool nc_screen_is_lockable(NcScreen *screen);
bool nc_screen_is_mergable(NcScreen *screen);
void nc_screen_set_has_to_be_resized(NcScreen *screen,
                                     bool has_to_be_resized);
void nc_screen_set_has_to_be_updated(NcScreen *screen,
                                     bool has_to_be_updated);
void nc_screen_request_resize(NcScreen *screen);
void nc_screen_request_update(NcScreen *screen);
void nc_screen_clear_resize_request(NcScreen *screen);
void nc_screen_clear_update_request(NcScreen *screen);
NcScreenResizeParams nc_screen_resize_params(NcScreen *screen);
void nc_screen_get_resize_params(NcScreen *screen, int32 *x_offset,
                                 int32 *width);
void nc_screen_draw_vertical_separator(int32 x);
void *nc_screen_user(NcScreen *screen);

int32 nc_screen_registry_register(NcScreenRegistry *registry,
                                  NcScreen *screen);
int32 nc_screen_registry_unregister(NcScreenRegistry *registry,
                                    NcScreen *screen);
NcScreen *nc_screen_registry_find(NcScreenRegistry *registry, int32 type);
NcScreen *nc_screen_registry_current(NcScreenRegistry *registry);
NcScreen *nc_screen_registry_previous(NcScreenRegistry *registry);
NcScreen *nc_screen_registry_locked(NcScreenRegistry *registry);
bool nc_screen_registry_is_registered(NcScreenRegistry *registry,
                                      NcScreen *screen);
bool nc_screen_registry_is_current(NcScreenRegistry *registry,
                                   NcScreen *screen);
void nc_screen_registry_request_resize_current(
    NcScreenRegistry *registry);
void nc_screen_registry_request_update_current(
    NcScreenRegistry *registry);
NcScreenResizeParams nc_screen_registry_resize_params(
    NcScreenRegistry *registry, NcScreen *screen,
    bool adjust_locked_screen);
int32 nc_screen_registry_switch_to(NcScreenRegistry *registry,
                                   NcScreen *screen);
int32 nc_screen_registry_lock_current(NcScreenRegistry *registry);
void nc_screen_registry_unlock(NcScreenRegistry *registry);
bool nc_screen_registry_is_visible(NcScreenRegistry *registry,
                                   NcScreen *screen);
void nc_screen_registry_each_visible(NcScreenRegistry *registry,
                                     NcScreenEachCallback *callback,
                                     void *user);
void nc_screen_registry_update_visible(NcScreenRegistry *registry);
void nc_screen_registry_resize_current(NcScreenRegistry *registry);
void nc_screen_registry_resize_visible(NcScreenRegistry *registry);

/* screens/screen_type.h */
#define ENUM_NAME ScreenType
#define ENUM_PREFIX_ NCM_SCREEN_TYPE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS NCM_SCREEN_TYPE_ENUM_FIELDS
#include "cbase/xenums.c"

int32 screen_type_to_nc_type(enum ScreenType screen_type);
enum ScreenType screen_type_from_nc_type(int32 nc_type);
int32 screen_type_parse_startup(char *string, int32 string_len,
                                enum ScreenType *screen_type);
int32 screen_type_parse(char *string, int32 string_len,
                        enum ScreenType *screen_type);

/* screens/screen_switcher.h */
NcScreen *nc_screen_switcher_current(void);
NcScreen *nc_screen_switcher_previous(void);
bool nc_screen_switcher_is_current(NcScreen *screen);
bool nc_screen_switcher_is_visible(NcScreen *screen);
int32 nc_screen_switcher_switch_to(NcScreen *screen,
                                   bool has_to_be_resized);
void nc_screen_switcher_finish_switch(NcScreen *screen);
void nc_screen_switcher_get_resize_params(NcScreen *screen,
                                          int32 *x_offset, int32 *width,
                                          bool adjust_locked_screen);

/* screens/nc_scrollpad_screen.h */
typedef struct NcScrollpadScreen {
    NcScreen base;

    int32 start_x;
    int32 start_y;
    int32 width;
    int32 height;
} NcScrollpadScreen;

void nc_scrollpad_screen_init(NcScrollpadScreen *screen,
                              NcScreenOps callbacks, void *user,
                              int32 type, int32 start_x, int32 start_y,
                              int32 width, int32 height);
void nc_scrollpad_screen_set_geometry(NcScrollpadScreen *screen,
                                      int32 start_x, int32 start_y,
                                      int32 width, int32 height);
void nc_scrollpad_screen_set_main_area(NcScrollpadScreen *screen,
                                       int32 start_x, int32 width,
                                       int32 main_start_y,
                                       int32 main_height);
void nc_scrollpad_screen_set_centered_box(NcScrollpadScreen *screen,
                                          int32 cols, int32 lines,
                                          int32 main_start_y,
                                          int32 main_height,
                                          int32 width_num,
                                          int32 width_den,
                                          int32 height_num,
                                          int32 height_den);
NcScreen *nc_scrollpad_screen_base(NcScrollpadScreen *screen);
int32 nc_scrollpad_screen_start_x(NcScrollpadScreen *screen);
int32 nc_scrollpad_screen_start_y(NcScrollpadScreen *screen);
int32 nc_scrollpad_screen_width(NcScrollpadScreen *screen);
int32 nc_scrollpad_screen_height(NcScrollpadScreen *screen);

/* screens/nc_help.h */
typedef struct NcHelpScreen NcHelpScreen;

typedef struct NcHelpHooks {
    int32 (*render)(void *user, NcBuffer *buffer);
    void (*switch_to)(void *user);
    void (*resize_layout)(void *user, NcHelpScreen *screen);
    void (*resize_background)(void *user);
    void (*destroy)(void *user);
    void *user;
} NcHelpHooks;

struct NcHelpScreen {
    NcScrollpadScreen scrollpad_screen;
    NcWindow window;
    NcScrollpad scrollpad;
    NcBuffer buffer;
    StrBuilder search_constraint;
    NcHelpHooks hooks;

    int32 lines_scrolled;
};

void nc_help_screen_init(NcHelpScreen *screen,
                         NcHelpHooks hooks,
                         int32 start_x, int32 width,
                         int32 main_start_y, int32 main_height,
                         NcColor color, NcBorder border,
                         int32 lines_scrolled);
void nc_help_screen_set_geometry(NcHelpScreen *screen,
                                 int32 start_x, int32 width,
                                 int32 main_start_y,
                                 int32 main_height);
int32 nc_help_screen_reload(NcHelpScreen *screen);
int32 nc_help_screen_find(NcHelpScreen *screen, char *pattern,
                          int32 pattern_len, NcmError *ncm_error);
void nc_help_screen_clear_search(NcHelpScreen *screen);
NcScreen *nc_help_screen_base(NcHelpScreen *screen);
int32 nc_help_screen_start_x(NcHelpScreen *screen);
int32 nc_help_screen_start_y(NcHelpScreen *screen);
int32 nc_help_screen_width(NcHelpScreen *screen);
int32 nc_help_screen_height(NcHelpScreen *screen);

/* screens/nc_lastfm.h */
#include "lastfm_service.h"

typedef struct NcLastfmScreen {
    NcScrollpadScreen scrollpad_screen;
} NcLastfmScreen;

typedef struct LastfmScreen {
    NcLastfmScreen screen;
    NcWindow window;
    NcScrollpad scrollpad;
    NcBuffer buffer;
    StrBuilder search_constraint;

    NcmLastfmService service;
    NcmLastfmResult result;
    NcmJobQueue jobs;

    char *title;
    int32 title_len;
    int32 title_cap;

    bool has_service;
    bool refresh_window;
    bool initialized;
} LastfmScreen;

void nc_lastfm_screen_init(NcLastfmScreen *screen,
                           NcScreenOps callbacks, void *user,
                           int32 start_x, int32 width,
                           int32 main_start_y, int32 main_height);
void nc_lastfm_screen_set_geometry(NcLastfmScreen *screen,
                                   int32 start_x, int32 width,
                                   int32 main_start_y,
                                   int32 main_height);
NcScreen *nc_lastfm_screen_base(NcLastfmScreen *screen);
int32 nc_lastfm_screen_start_x(NcLastfmScreen *screen);
int32 nc_lastfm_screen_start_y(NcLastfmScreen *screen);
int32 nc_lastfm_screen_width(NcLastfmScreen *screen);
int32 nc_lastfm_screen_height(NcLastfmScreen *screen);

void lastfm_screen_init(LastfmScreen *screen,
                        int32 start_x, int32 width,
                        int32 main_start_y, int32 main_height,
                        NcColor color, NcBorder border,
                        int32 lines_scrolled);
void lastfm_screen_destroy(LastfmScreen *screen);
NcScreen *lastfm_screen_base(LastfmScreen *screen);
NcWindow *lastfm_screen_window(LastfmScreen *screen);
void lastfm_screen_set_geometry(LastfmScreen *screen,
                                int32 start_x, int32 width,
                                int32 main_start_y,
                                int32 main_height);
int32 lastfm_screen_queue_artist_info(LastfmScreen *screen,
                                      char *artist, int32 artist_len,
                                      char *lang, int32 lang_len,
                                      NcmError *ncm_error);
int32 lastfm_screen_dispatch_jobs(LastfmScreen *screen);
void lastfm_screen_update(LastfmScreen *screen);
char *lastfm_screen_title(LastfmScreen *screen);
int32 lastfm_screen_take_refresh_request(LastfmScreen *screen);
int32 lastfm_buffer_find(NcBuffer *buffer, char *pattern,
                         int32 pattern_len, NcmError *ncm_error);
int32 lastfm_screen_find(LastfmScreen *screen,
                         char *pattern, int32 pattern_len,
                         NcmError *ncm_error);

/* screens/nc_lyrics.h */
#include "lyrics_fetcher.h"

typedef struct LyricsJob LyricsJob;

typedef enum LyricsMode {
    LYRICS_MODE_PLAIN,
    LYRICS_MODE_SYNCHRONIZED,
    LYRICS_MODE_FETCH_LOG,
} LyricsMode;

typedef struct NcLyricsScreen {
    NcScrollpadScreen scrollpad_screen;

    int32 scroll_begin;
    bool refresh_window;
} NcLyricsScreen;

typedef struct LyricsQueuedSong {
    NcmSong song;
    bool notify;
} LyricsQueuedSong;

typedef struct LyricsScreen {
    NcLyricsScreen screen;
    NcWindow window;
    NcScrollpad scrollpad;
    NcBuffer display;
    StrBuilder search_constraint;

    StrBuilder title;
    NcmSong song;
    StrBuilder filename;
    NcmLrcDocument lrc;
    NcmLyricsResult result;
    NcmJobQueue jobs;
    LyricsJob *foreground_job;
    LyricsQueuedSong *queued_songs;
    StrBuilder consumer_message;

    NcmLyricsFetcherDef *fetcher;
    int32 queued_songs_len;
    int32 queued_songs_cap;
    int32 active_lrc_line;
    LyricsMode mode;

    bool has_song;
    bool initialized;
} LyricsScreen;

void nc_lyrics_screen_init(NcLyricsScreen *screen,
                           NcScreenOps callbacks, void *user,
                           int32 start_x, int32 width,
                           int32 main_start_y, int32 main_height);
void nc_lyrics_screen_set_geometry(NcLyricsScreen *screen,
                                   int32 start_x, int32 width,
                                   int32 main_start_y, int32 main_height);
NcScreen *nc_lyrics_screen_base(NcLyricsScreen *screen);
int32 nc_lyrics_screen_start_x(NcLyricsScreen *screen);
int32 nc_lyrics_screen_start_y(NcLyricsScreen *screen);
int32 nc_lyrics_screen_width(NcLyricsScreen *screen);
int32 nc_lyrics_screen_height(NcLyricsScreen *screen);
void nc_lyrics_screen_request_refresh(NcLyricsScreen *screen);
int32 nc_lyrics_screen_take_refresh_request(NcLyricsScreen *screen);
void nc_lyrics_screen_reset_scroll_begin(NcLyricsScreen *screen);
int32 nc_lyrics_screen_scroll_begin(NcLyricsScreen *screen);
void nc_lyrics_screen_set_scroll_begin(NcLyricsScreen *screen,
                                       int32 scroll_begin);

void lyrics_queued_song_destroy(LyricsQueuedSong *queued);
void lyrics_queued_song_move(LyricsQueuedSong *dest,
                             LyricsQueuedSong *source);

void lyrics_screen_init(LyricsScreen *screen,
                        int32 start_x, int32 width,
                        int32 main_start_y, int32 main_height,
                        NcColor color, NcBorder border,
                        int32 lines_scrolled);
void lyrics_screen_destroy(LyricsScreen *screen);
NcScreen *lyrics_screen_base(LyricsScreen *screen);
NcWindow *lyrics_screen_window(LyricsScreen *screen);
void lyrics_screen_set_geometry(LyricsScreen *screen,
                                int32 start_x, int32 width,
                                int32 main_start_y,
                                int32 main_height);
int32 lyrics_screen_build_filename(LyricsScreen *screen,
                                  NcmSong *song,
                                  char *music_dir,
                                  int32 music_dir_len,
                                  char *lyrics_dir,
                                  int32 lyrics_dir_len,
                                  bool store_in_song_dir,
                                  bool win32_filename);
int32 lyrics_screen_load_file(LyricsScreen *screen,
                             char *filename, int32 filename_len,
                             NcmError *ncm_error);
int32 lyrics_screen_save_file(LyricsScreen *screen,
                             char *filename, int32 filename_len,
                             char *lyrics, int32 lyrics_len,
                             NcmError *ncm_error);
int32 lyrics_screen_fetch(LyricsScreen *screen,
                         NcmSong *song,
                         NcmLyricsFetcherDef *fetcher,
                         NcmError *ncm_error);
int32 lyrics_screen_fetch_in_background(LyricsScreen *screen,
                                       NcmSong *song,
                                       bool notify,
                                       NcmError *ncm_error);
int32 lyrics_screen_dispatch_jobs(LyricsScreen *screen);
void lyrics_screen_update(LyricsScreen *screen);
void lyrics_screen_refetch_current(LyricsScreen *screen,
                                   NcmError *ncm_error);
NcmLyricsFetcherDef *lyrics_screen_toggle_fetcher(
    LyricsScreen *screen, NcmLyricsFetcherRegistry *registry);
int32 lyrics_screen_try_take_consumer_message(
    LyricsScreen *screen, StrBuilder *message);
NcmSong *lyrics_screen_song(LyricsScreen *screen);
StrBuilder *lyrics_screen_filename(LyricsScreen *screen);
LyricsMode lyrics_screen_mode(LyricsScreen *screen);
NcmLrcDocument *lyrics_screen_lrc(LyricsScreen *screen);
int32 lyrics_screen_active_lrc_line(LyricsScreen *screen);
int32 lyrics_buffer_find(NcBuffer *buffer, char *pattern,
                         int32 pattern_len, NcmError *ncm_error);
void lyrics_buffer_clear_sync_highlight(NcBuffer *buffer);
void lyrics_buffer_highlight_sync_line(NcBuffer *buffer,
                                       int32 start, int32 end);
int32 lyrics_screen_find(LyricsScreen *screen,
                         char *pattern, int32 pattern_len,
                         NcmError *ncm_error);

/* screens/nc_outputs.h */
typedef struct NcOutputsScreen NcOutputsScreen;

typedef struct NcOutputsHooks {
    void (*fetch_outputs)(void *user, NcOutputsScreen *screen);
    int32 (*toggle_output)(void *user, int32 id, bool enabled,
                             char *name, int32 name_len);
    void (*switch_to)(void *user);
    void (*resize_layout)(void *user, NcOutputsScreen *screen);
    void (*resize_background)(void *user);
    void (*destroy)(void *user);
    void *user;
} NcOutputsHooks;

typedef struct NcOutputsItem {
    char *name;
    int32 name_len;
    int32 id;
    bool enabled;
} NcOutputsItem;

struct NcOutputsScreen {
    NcScrollpadScreen menu_screen;
    NcWindow window;
    NcMenu menu;
    NcOutputsHooks hooks;
    int32 lines_scrolled;
    bool mouse_scroll_whole_page;
};

void nc_outputs_screen_init(NcOutputsScreen *screen,
                            NcOutputsHooks hooks,
                            int32 start_x, int32 width,
                            int32 main_start_y, int32 main_height,
                            NcColor color, NcBorder border,
                            int32 lines_scrolled,
                            bool mouse_scroll_whole_page);
void nc_outputs_screen_set_geometry(NcOutputsScreen *screen,
                                    int32 start_x, int32 width,
                                    int32 main_start_y,
                                    int32 main_height);
void nc_outputs_screen_set_highlight_prefix(NcOutputsScreen *screen,
                                            NcBuffer *buffer);
void nc_outputs_screen_set_highlight_suffix(NcOutputsScreen *screen,
                                            NcBuffer *buffer);
void nc_outputs_screen_fetch_list(NcOutputsScreen *screen);
void nc_outputs_screen_clear_outputs(NcOutputsScreen *screen);
void nc_outputs_screen_add_output(NcOutputsScreen *screen,
                                  int32 id,
                                  char *name,
                                  int32 name_len,
                                  bool enabled);
int32 nc_outputs_screen_toggle_current(NcOutputsScreen *screen);
NcScreen *nc_outputs_screen_base(NcOutputsScreen *screen);
int32 nc_outputs_screen_start_x(NcOutputsScreen *screen);
int32 nc_outputs_screen_start_y(NcOutputsScreen *screen);
int32 nc_outputs_screen_width(NcOutputsScreen *screen);
int32 nc_outputs_screen_height(NcOutputsScreen *screen);

/* screens/nc_song_info.h */
typedef struct NcSongInfoScreen NcSongInfoScreen;

typedef struct NcSongInfoHooks {
    int32 (*render)(void *user, NcSongInfoScreen *screen, NcBuffer *buffer);
    void (*switch_to)(void *user, NcSongInfoScreen *screen);
    void (*resize_layout)(void *user, NcSongInfoScreen *screen);
    void (*destroy)(void *user);
    void *user;
} NcSongInfoHooks;

struct NcSongInfoScreen {
    NcScrollpadScreen scrollpad_screen;
    NcWindow window;
    NcScrollpad scrollpad;
    NcBuffer buffer;
    NcSongInfoHooks hooks;

    int32 lines_scrolled;
};

void nc_song_info_screen_init(NcSongInfoScreen *screen,
                              NcSongInfoHooks hooks,
                              int32 start_x, int32 width,
                              int32 main_start_y, int32 main_height,
                              NcColor color, NcBorder border,
                              int32 lines_scrolled);
void nc_song_info_screen_set_geometry(NcSongInfoScreen *screen,
                                      int32 start_x, int32 width,
                                      int32 main_start_y,
                                      int32 main_height);
int32 nc_song_info_screen_prepare_current(NcSongInfoScreen *screen);
NcScreen *nc_song_info_screen_base(NcSongInfoScreen *screen);
int32 nc_song_info_screen_start_x(NcSongInfoScreen *screen);
int32 nc_song_info_screen_start_y(NcSongInfoScreen *screen);
int32 nc_song_info_screen_width(NcSongInfoScreen *screen);
int32 nc_song_info_screen_height(NcSongInfoScreen *screen);

/* screens/nc_server_info.h */
typedef struct NcServerInfoHooks {
    void (*load_lists)(void *user);
    int32 (*render)(void *user, NcBuffer *buffer);
    void (*switch_to)(void *user);
    void (*resize_layout)(void *user);
    void (*resize_background)(void *user);
    char *(*title)(void *user);
    void (*destroy)(void *user);
    void *user;
} NcServerInfoHooks;

typedef struct NcServerInfoScreen {
    NcScrollpadScreen scrollpad_screen;
    NcWindow window;
    NcScrollpad scrollpad;
    NcBuffer buffer;
    NcServerInfoHooks hooks;
} NcServerInfoScreen;

void nc_server_info_screen_init(NcServerInfoScreen *screen,
                                NcServerInfoHooks hooks,
                                int32 cols, int32 lines,
                                int32 main_start_y,
                                int32 main_height,
                                NcColor color, NcBorder border);
void nc_server_info_screen_set_dimensions(NcServerInfoScreen *screen,
                                          int32 cols, int32 lines,
                                          int32 main_start_y,
                                          int32 main_height);
NcScreen *nc_server_info_screen_base(NcServerInfoScreen *screen);
int32 nc_server_info_screen_width(NcServerInfoScreen *screen);
int32 nc_server_info_screen_height(NcServerInfoScreen *screen);
int32 nc_server_info_screen_start_x(NcServerInfoScreen *screen);
int32 nc_server_info_screen_start_y(NcServerInfoScreen *screen);

/* screens/nc_visualizer.h */
#include "configura.h"

#if defined(HAVE_FFTW3_H)
#define FFTW_NO_Complex 1
#include <fftw3.h>
#endif


#define VISUALIZER_PI 3.14159265358979323846

#if defined(HAVE_FFTW3_H)
#define VISUALIZER_FREQUENCY_FIELD                                             \
    XX(VISUALIZER_FREQUENCY, spectrum)
#else
#define VISUALIZER_FREQUENCY_FIELD
#endif

#define ENUM_NAME VisualizerScreenType
#define ENUM_PREFIX_ VISUALIZER_TYPE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(VISUALIZER_WAVE, wave)                                                  \
    XX(VISUALIZER_WAVE_FILLED, wave_filled)                                    \
    VISUALIZER_FREQUENCY_FIELD                                                 \
    XX(VISUALIZER_ELLIPSE, ellipse)
#include "cbase/xenums.c"
#undef VISUALIZER_FREQUENCY_FIELD

struct NcmError;
struct NcmMpdClient;
struct NcmMpdOutputList;

typedef struct VisualizerDataSourceHooks {
    int32 (*open_fifo)(void *user, char *location, int32 location_len);
    int32 (*open_udp)(void *user, char *location, int32 location_len,
                      char *port, int32 port_len);
    int32 (*read_source)(void *user, int32 fd, void *buffer, int32 buffer_size);
    void (*close_source)(void *user, int32 fd);
    int32 (*get_outputs)(void *user,
                         struct NcmMpdOutputList *outputs,
                         struct NcmError *ncm_error);
    int32 (*disable_output)(void *user, int32 id,
                            struct NcmError *ncm_error);
    int32 (*enable_output)(void *user, int32 id,
                           struct NcmError *ncm_error);
    void (*sleep_microseconds)(void *user, int32 microseconds);
    void *user;
} VisualizerDataSourceHooks;

typedef struct VisualizerScreenConfig {
    char *source_location;
    char *output_name;
    char *visualizer_chars;
    NcFormattedColor *visualizer_colors;

    int32 source_location_len;
    int32 output_name_len;
    int32 visualizer_chars_len;
    int32 visualizer_colors_len;
    int32 fps;
    int32 spectrum_dft_size;

    double spectrum_gain;
    double spectrum_hz_min;
    double spectrum_hz_max;

    VisualizerDataSourceHooks data_source_hooks;
    enum VisualizerScreenType visualization_type;
    bool autoscale;
    bool stereo;
    bool spectrum_smooth_look;
    bool spectrum_smooth_look_legacy_chars;
    bool spectrum_log_scale_x;
    bool spectrum_log_scale_y;
} VisualizerScreenConfig;

#if defined(HAVE_FFTW3_H)
typedef struct VisualizerBarHeight {
    int32 column;
    double height;
} VisualizerBarHeight;

typedef struct VisualizerFftState {
    double *input;
    fftw_complex *output;
    fftw_plan plan;

    double *freqs_mags;
    double *dft_frequency_space;
    VisualizerBarHeight *bar_heights;

    int32 results_len;
    int32 dft_nonzero_size;
    int32 dft_total_size;

    int32 freqs_mags_len;
    int32 dft_frequency_space_len;
    int32 dft_frequency_space_cap;
    int32 bar_heights_len;
    int32 bar_heights_cap;

    double dynamic_range;
    double hz_min;
    double hz_max;
    double gain;
} VisualizerFftState;
#endif

typedef struct VisualizerScreen {
    NcScreen screen;
    NcWindow window;

    StrBuilder source_location;
    StrBuilder source_port;
    StrBuilder output_name;
    StrBuilder visualizer_chars;
    NcFormattedColor *visualizer_colors;
    VisualizerDataSourceHooks data_source_hooks;

    NcmSampleBuffer incoming_samples;
    NcmSampleBuffer buffered_samples;
    NcmSampleBuffer rendered_samples;
    NcmSampleBuffer left_channel;
    NcmSampleBuffer right_channel;

#if defined(HAVE_FFTW3_H)
    VisualizerFftState fft;
#endif

    double auto_scale_multiplier;
    enum VisualizerScreenType visualization_type;

    int32 source_fd;
    int32 output_id;
    int32 fps;
    int32 sample_rate;
    int64 sample_clock;
    int64 sample_clock_frame_remainder;
    int32 visualizer_colors_len;
    int32 point_char_offset;
    int32 point_char_len;
    int32 bar_char_offset;
    int32 bar_char_len;

    bool reset_output;
    bool sample_clock_initialized;
    bool autoscale;
    bool stereo;
    bool spectrum_smooth_look;
    bool spectrum_smooth_look_legacy_chars;
    bool spectrum_log_scale_x;
    bool spectrum_log_scale_y;
    bool initialized;
} VisualizerScreen;

void visualizer_screen_init(VisualizerScreen *screen,
                            int32 start_x, int32 start_y,
                            int32 width, int32 height,
                            NcColor color, NcBorder border,
                            VisualizerScreenConfig *config);
void visualizer_screen_destroy(VisualizerScreen *screen);
VisualizerDataSourceHooks visualizer_data_source_system_hooks(
    struct NcmMpdClient *client);
void visualizer_screen_init_data_source(VisualizerScreen *screen,
                                        char *source_location,
                                        int32 source_location_len);
int32 visualizer_screen_open_data_source(VisualizerScreen *screen);
void visualizer_screen_close_data_source(VisualizerScreen *screen);
int32 visualizer_screen_drain_data_source(VisualizerScreen *screen);
int32 visualizer_screen_find_output_id(VisualizerScreen *screen);
NcScreen *visualizer_screen_base(VisualizerScreen *screen);
NcWindow *visualizer_screen_window(VisualizerScreen *screen);
void visualizer_screen_set_geometry(VisualizerScreen *screen,
                                    int32 start_x, int32 start_y,
                                    int32 width, int32 height);
void visualizer_screen_init_visualization(VisualizerScreen *screen);
void visualizer_screen_clear(VisualizerScreen *screen);
void visualizer_screen_reset_audio_state(VisualizerScreen *screen);
void visualizer_screen_reset_auto_scale_multiplier(VisualizerScreen *screen);
void visualizer_screen_toggle_type(VisualizerScreen *screen);
int32 visualizer_screen_requested_samples(VisualizerScreen *screen);
void visualizer_screen_push_samples(VisualizerScreen *screen,
                                    int16 *samples,
                                    int32 samples_len);
int32 visualizer_screen_take_render_samples(
    VisualizerScreen *screen, int16 *dest, int32 dest_len);
int32 visualizer_screen_split_stereo(VisualizerScreen *screen,
                                     int16 *samples,
                                     int32 samples_len);
void visualizer_screen_apply_auto_scale(VisualizerScreen *screen,
                                        int16 *samples,
                                        int32 samples_len);
void visualizer_screen_draw(VisualizerScreen *screen,
                            int16 *samples, int32 samples_len);
int16 visualizer_clamp_sample(int32 sample);

/* screens/nc_media_library.h */
#define MEDIA_LIBRARY_FETCH_DELAY_MS 250

#define ENUM_NAME MediaLibraryMode
#define ENUM_PREFIX_ MEDIA_LIBRARY_MODE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(MEDIA_LIBRARY_MODE_THREE_COLUMNS, three_columns)                        \
    XX(MEDIA_LIBRARY_MODE_TWO_COLUMNS, two_columns)                            \
    XX(MEDIA_LIBRARY_MODE_ALBUM_ONLY, album_only)
#include "cbase/xenums.c"

#define ENUM_NAME MediaLibraryColumn
#define ENUM_PREFIX_ MEDIA_LIBRARY_COLUMN_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(MEDIA_LIBRARY_COLUMN_TAGS, Tags)                                        \
    XX(MEDIA_LIBRARY_COLUMN_ALBUMS, Albums)                                    \
    XX(MEDIA_LIBRARY_COLUMN_SONGS, Songs)
#include "cbase/xenums.c"

typedef struct MediaLibraryAlbumItem {
    NcMediaLibraryAlbumRow row;
    uint32 menu_flags;
} MediaLibraryAlbumItem;

NCM_ARRAY_DECLARE_TYPE(MediaLibraryTagArray, NcMediaLibraryTagRow)
NCM_ARRAY_DECLARE_CLEAR(media_library_tag_array, MediaLibraryTagArray)
NCM_ARRAY_DECLARE_DESTROY(media_library_tag_array, MediaLibraryTagArray)
NCM_ARRAY_DECLARE_MOVE(media_library_tag_array, MediaLibraryTagArray)
NCM_ARRAY_DECLARE_RESERVE(media_library_tag_array, MediaLibraryTagArray)

NCM_ARRAY_DECLARE_APPEND(media_library_tag_array, MediaLibraryTagArray,
                         NcMediaLibraryTagRow)

NCM_ARRAY_DECLARE_REMOVE_ORDERED(media_library_tag_array, MediaLibraryTagArray)

NCM_ARRAY_DECLARE_TYPE(MediaLibraryAlbumArray, MediaLibraryAlbumItem)
NCM_ARRAY_DECLARE_CLEAR(media_library_album_array, MediaLibraryAlbumArray)
NCM_ARRAY_DECLARE_DESTROY(media_library_album_array, MediaLibraryAlbumArray)
NCM_ARRAY_DECLARE_MOVE(media_library_album_array, MediaLibraryAlbumArray)
NCM_ARRAY_DECLARE_RESERVE(media_library_album_array, MediaLibraryAlbumArray)

NCM_ARRAY_DECLARE_APPEND(media_library_album_array, MediaLibraryAlbumArray,
                         MediaLibraryAlbumItem)

NCM_ARRAY_DECLARE_REMOVE_ORDERED(media_library_album_array,
                                 MediaLibraryAlbumArray)

typedef struct MediaLibrarySongQuery {
    char *primary_value;
    char *album;
    char *date;

    int32 primary_value_len;
    int32 album_len;
    int32 date_len;

    enum mpd_tag_type primary_tag;
    bool match_primary_tag;
    bool match_album;
    bool match_date;
} MediaLibrarySongQuery;

typedef struct MediaLibraryColumnState {
    StrBuilder filter_constraint;
    StrBuilder search_constraint;
    NcmRegex filter_regex;
    NcmRegex search_regex;
    bool filter_enabled;
    bool search_enabled;
} MediaLibraryColumnState;

typedef struct MediaLibraryHooks {
    int32 (*list_tags)(void *user, enum mpd_tag_type tag_type,
                       NcmStringViewList *tags, NcmError *ncm_error);
    int32 (*list_all_songs)(void *user, NcmMpdSongList *songs,
                            NcmError *ncm_error);
    int32 (*search_songs)(void *user,
                          MediaLibrarySongQuery *query,
                          NcmMpdSongList *songs, NcmError *ncm_error);
    int32 (*add_songs)(void *user, NcmSongArray *songs, bool play,
                       NcmError *ncm_error);
    void (*destroy)(void *user);
    void *user;
} MediaLibraryHooks;

typedef struct MediaLibraryScreen {
    NcScreen screen;
    NcMediaLibraryTagMenu tags;
    NcMediaLibraryAlbumMenu albums;
    NcMediaLibrarySongMenu songs;
    NcWindow tags_window;
    NcWindow albums_window;
    NcWindow songs_window;
    MediaLibraryHooks hooks;

    MediaLibraryColumnState column_state[
        MEDIA_LIBRARY_COLUMN_COUNT];
    StrBuilder tags_title;
    StrBuilder albums_title;
    StrBuilder songs_title;
    int64 update_timer;
    NcMediaLibraryTagRow observed_tag;
    NcMediaLibraryAlbumRow observed_album;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;
    int32 fetching_delay_ms;
    int32 window_timeout_ms;

    enum MediaLibraryMode mode;
    enum MediaLibraryColumn active_column;

    bool tags_update_request;
    bool albums_update_request;
    bool songs_update_request;
    bool sort_by_mtime;
    bool observed_tag_valid;
    bool observed_album_valid;
    bool registered;
} MediaLibraryScreen;

MediaLibraryHooks media_library_mpd_hooks(
    NcmMpdClient *client);
void media_library_screen_init(MediaLibraryScreen *screen,
                               MediaLibraryHooks hooks,
                               int32 start_x, int32 width,
                               int32 main_start_y,
                               int32 main_height, NcColor color,
                               NcBorder border);
void media_library_screen_destroy(MediaLibraryScreen *screen);
NcScreen *media_library_screen_base(MediaLibraryScreen *screen);
NcMenu *media_library_screen_active_menu(
    MediaLibraryScreen *screen);
NcWindow *media_library_screen_active_window(
    MediaLibraryScreen *screen);
void media_library_screen_set_geometry(
    MediaLibraryScreen *screen, int32 start_x, int32 width,
    int32 main_start_y, int32 main_height);

int32 media_library_screen_column_count(MediaLibraryScreen *screen);
int32 media_library_screen_set_mode(MediaLibraryScreen *screen,
                                   enum MediaLibraryMode mode);

int32 media_library_screen_toggle_mode(
    MediaLibraryScreen *screen, enum MediaLibraryMode *mode);
enum MediaLibraryColumn media_library_screen_active_column(
    MediaLibraryScreen *screen);
bool media_library_screen_has_available_item(
    MediaLibraryScreen *screen);
int32 media_library_screen_set_active_column(
    MediaLibraryScreen *screen,
    enum MediaLibraryColumn column);
bool media_library_screen_column_is_visible(
    MediaLibraryScreen *screen,
    enum MediaLibraryColumn column);
MediaLibraryColumnState *media_library_screen_column_state(
    MediaLibraryScreen *screen,
    enum MediaLibraryColumn column);
StrBuilder *media_library_screen_active_filter_constraint(
    MediaLibraryScreen *screen);
StrBuilder *media_library_screen_active_search_constraint(
    MediaLibraryScreen *screen);
NcMediaLibraryTagRow *media_library_screen_current_tag(
    MediaLibraryScreen *screen);
NcMediaLibraryAlbumRow *media_library_screen_current_album(
    MediaLibraryScreen *screen);

bool media_library_screen_has_current_primary_tag_value(
    MediaLibraryScreen *screen, char **value, int32 *value_len);
bool media_library_screen_has_current_album_value(
    MediaLibraryScreen *screen, char **album, int32 *album_len);
void media_library_screen_format_tag_row(
    MediaLibraryScreen *screen, NcMediaLibraryTagRow *row,
    StrBuilder *output);
void media_library_screen_format_album_row(
    MediaLibraryScreen *screen, NcMediaLibraryAlbumRow *row,
    StrBuilder *output);
void media_library_screen_format_song_row(
    MediaLibraryScreen *screen, NcmSong *song, NcBuffer *output);

int32 media_library_tags_from_strings(
    MediaLibraryTagArray *tags, NcmStringViewList *strings);
int32 media_library_tags_from_songs(
    MediaLibraryTagArray *tags, NcmMpdSongList *songs,
    enum mpd_tag_type primary_tag);
int32 media_library_albums_from_songs(
    MediaLibraryAlbumArray *albums, NcmMpdSongList *songs,
    enum MediaLibraryMode mode, enum mpd_tag_type primary_tag,
    char *selected_tag, int32 selected_tag_len);
int32 media_library_songs_from_list(
    NcmSongArray *songs, NcmMpdSongList *source);

int32 media_library_screen_toggle_sort_mode(MediaLibraryScreen *screen,
                                             bool *enabled);
int32 media_library_screen_set_primary_tag_type(MediaLibraryScreen *screen,
                                               enum mpd_tag_type tag_type);
void media_library_screen_request_database_update(MediaLibraryScreen *screen);
int32 media_library_screen_refresh_inactive_songs(MediaLibraryScreen *screen);

bool media_library_screen_can_move_to_previous_column(
    MediaLibraryScreen *screen);
bool media_library_screen_can_move_to_next_column(
    MediaLibraryScreen *screen);
void media_library_screen_previous_column(MediaLibraryScreen *screen);
void media_library_screen_next_column(MediaLibraryScreen *screen);
void media_library_screen_clear(MediaLibraryScreen *screen);
int32 media_library_screen_current_song(MediaLibraryScreen *screen,
                                       NcmSong *song);
int32 media_library_screen_selected_songs(MediaLibraryScreen *screen,
                                         NcmSongArray *songs);
int32 media_library_screen_selected_songs_checked(MediaLibraryScreen *screen,
                                                 NcmSongArray *songs,
                                                 NcmError *ncm_error);
int32 media_library_screen_copy_visible_songs(MediaLibraryScreen *screen,
                                             NcmSongArray *songs,
                                             NcmError *ncm_error);
int32 media_library_screen_apply_filter(MediaLibraryScreen *screen,
                                        char *pattern, int32 pattern_len,
                                        NcmError *ncm_error);
void media_library_screen_clear_filter(MediaLibraryScreen *screen);
int32 media_library_screen_search(MediaLibraryScreen *screen,
                                  char *pattern, int32 pattern_len,
                                  bool forward, bool wrap, bool skip_current,
                                  NcmError *ncm_error);
void media_library_screen_clear_search(MediaLibraryScreen *screen);
void media_library_screen_request_tags_update(MediaLibraryScreen *screen);
void media_library_screen_request_albums_update(MediaLibraryScreen *screen);
void media_library_screen_request_songs_update(MediaLibraryScreen *screen);
void media_library_screen_finish_list_change(MediaLibraryScreen *screen);
int32 media_library_screen_update(MediaLibraryScreen *screen,
                                 NcmError *ncm_error);

int32 media_library_screen_list_tags(MediaLibraryScreen *screen,
                                       enum mpd_tag_type tag_type,
                                       NcmStringViewList *tags,
                                       NcmError *ncm_error);
int32 media_library_screen_list_all_songs(MediaLibraryScreen *screen,
                                         NcmMpdSongList *songs,
                                         NcmError *ncm_error);
int32 media_library_screen_search_songs(MediaLibraryScreen *screen,
                                       MediaLibrarySongQuery *query,
                                       NcmMpdSongList *songs,
                                       NcmError *ncm_error);
int32 media_library_screen_add_songs(MediaLibraryScreen *screen,
                                    NcmSongArray *songs, bool play,
                                    NcmError *ncm_error);
int32 media_library_screen_add_item_to_playlist(
    MediaLibraryScreen *screen, bool play, NcmError *ncm_error);
int32 media_library_screen_locate_song(MediaLibraryScreen *screen,
                                      NcmSong *song, NcmError *ncm_error);

/* screens/nc_playlist_editor.h */
#define ENUM_NAME PlaylistEditorColumn
#define ENUM_PREFIX_ PLAYLIST_EDITOR_COLUMN_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(PLAYLIST_EDITOR_COLUMN_PLAYLISTS, Playlists)                            \
    XX(PLAYLIST_EDITOR_COLUMN_CONTENT, Content)
#include "cbase/xenums.c"

#define PLAYLIST_EDITOR_FETCH_DELAY_MS 250

#define ENUM_NAME PlaylistEditorCommandType
#define ENUM_PREFIX_ PLAYLIST_EDITOR_COMMAND_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(PLAYLIST_EDITOR_COMMAND_NONE, none)                                     \
    XX(PLAYLIST_EDITOR_COMMAND_LOAD, Load)                                     \
    XX(PLAYLIST_EDITOR_COMMAND_SAVE, Save)                                     \
    XX(PLAYLIST_EDITOR_COMMAND_RENAME, Rename)                                 \
    XX(PLAYLIST_EDITOR_COMMAND_DELETE, Delete)
#include "cbase/xenums.c"

typedef struct PlaylistEditorCommand {
    enum PlaylistEditorCommandType type;
    char *playlist;
    char *target;
    int32 playlist_len;
    int32 playlist_cap;
    int32 target_len;
    int32 target_cap;
} PlaylistEditorCommand;

typedef struct PlaylistEditorScreen {
    NcScreen screen;
    NcPlaylistEntryMenu playlists;
    NcSongMenu content;
    NcWindow playlists_window;
    NcWindow content_window;
    StrBuilder playlist_filter_constraint;
    StrBuilder content_filter_constraint;
    StrBuilder playlist_search_constraint;
    StrBuilder content_search_constraint;
    StrBuilder playlists_title;
    StrBuilder content_title;
    StrBuilder displayed_playlist_path;
    StrBuilder observed_playlist_path;

    NcmRegex playlist_filter_regex;
    NcmRegex content_filter_regex;
    NcmRegex playlist_search_regex;
    NcmRegex content_search_regex;

    int64 timer;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;
    int32 left_width;
    int32 right_start_x;
    int32 right_width;
    int32 column_ratio_left;
    int32 column_ratio_right;
    int32 fetching_delay_ms;
    int32 last_playlist_highlight;
    int32 last_known_content_count;
    int32 window_timeout_ms;
    int32 active_column;

    bool playlists_update_requested;
    bool content_update_requested;
    bool playlist_filter_enabled;
    bool content_filter_enabled;
    bool playlist_search_enabled;
    bool content_search_enabled;
    bool displayed_playlist_valid;
    bool observed_playlist_valid;
    bool registered;
} PlaylistEditorScreen;

void playlist_editor_screen_init(PlaylistEditorScreen *screen,
                                 int32 start_x, int32 width,
                                 int32 main_start_y,
                                 int32 main_height,
                                 NcColor color, NcBorder border);
void playlist_editor_screen_destroy(PlaylistEditorScreen *screen);
NcScreen *playlist_editor_screen_base(PlaylistEditorScreen *screen);

NcPlaylistEntryMenu *playlist_editor_screen_playlists(
    PlaylistEditorScreen *screen);
NcSongMenu *playlist_editor_screen_content(
    PlaylistEditorScreen *screen);
NcMenu *playlist_editor_screen_active_menu(
    PlaylistEditorScreen *screen);
NcWindow *playlist_editor_screen_active_window(
    PlaylistEditorScreen *screen);

void playlist_editor_screen_set_geometry(
    PlaylistEditorScreen *screen, int32 start_x, int32 width,
    int32 main_start_y, int32 main_height);
void playlist_editor_screen_set_column_ratio(
    PlaylistEditorScreen *screen, int32 left, int32 right);
bool playlist_editor_screen_can_move_to_previous_column(
    PlaylistEditorScreen *screen);
bool playlist_editor_screen_can_move_to_next_column(
    PlaylistEditorScreen *screen);
void playlist_editor_screen_previous_column(
    PlaylistEditorScreen *screen);
void playlist_editor_screen_next_column(
    PlaylistEditorScreen *screen);
int32 playlist_editor_screen_load_playlists(
    PlaylistEditorScreen *screen, NcmMpdPlaylistList *playlists);
int32 playlist_editor_screen_reload_playlists_from_mpd(
    PlaylistEditorScreen *screen, NcmMpdClient *client,
    NcmError *ncm_error);
int32 playlist_editor_screen_load_content(
    PlaylistEditorScreen *screen, NcmMpdSongList *songs);
int32 playlist_editor_screen_reload_content_from_mpd(
    PlaylistEditorScreen *screen, NcmMpdClient *client,
    NcmError *ncm_error);
int32 playlist_editor_screen_locate_playlist(
    PlaylistEditorScreen *screen, NcmMpdClient *client,
    char *path, int32 path_len, NcmError *ncm_error);
int32 playlist_editor_screen_locate_song(
    PlaylistEditorScreen *screen, NcmMpdClient *client,
    NcmSong *song, NcmError *ncm_error);
int32 playlist_editor_screen_current_playlist(
    PlaylistEditorScreen *screen, NcmPlaylist *playlist);
int32 playlist_editor_screen_current_song(
    PlaylistEditorScreen *screen, NcmSong *song);
int32 playlist_editor_screen_current_content_song(
    PlaylistEditorScreen *screen, NcmSong *song);
int32 playlist_editor_screen_selected_playlist_count(
    PlaylistEditorScreen *screen);
int32 playlist_editor_screen_selected_songs(
    PlaylistEditorScreen *screen, NcmSongArray *songs);
int32 playlist_editor_screen_apply_active_filter(
    PlaylistEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, NcmError *ncm_error);
int32 playlist_editor_screen_search_active(
    PlaylistEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, bool forward, bool wrap, bool skip_current,
    NcmError *ncm_error);
void playlist_editor_screen_request_playlists_update(
    PlaylistEditorScreen *screen);
void playlist_editor_screen_request_content_update(
    PlaylistEditorScreen *screen);

/* screens/nc_playlist.h */
typedef struct NcPlaylistScreen {
    NcScreen screen;
    NcMenu *menu;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;
    int32 lines_scrolled;

    bool mouse_list_scroll_whole_page;
} NcPlaylistScreen;

typedef struct PlaylistScreen {
    NcPlaylistScreen screen;
    NcSongMenu songs;
    NcWindow window;
    StrBuilder title_cache;
    StrBuilder column_title;
    StrBuilder filter_constraint;
    StrBuilder search_constraint;
    NcmRegex filter_regex;

    uint64 total_length;
    uint64 remaining_time;
    int32 scroll_begin;
    int64 highlight_timer;

    bool reload_total_length;
    bool reload_remaining;
    bool registered;
    bool highlighting_requested;
} PlaylistScreen;

void nc_playlist_screen_init(NcPlaylistScreen *screen,
                             NcScreenOps callbacks, void *user,
                             NcMenu *menu, int32 start_x, int32 width,
                             int32 main_start_y, int32 main_height);
void nc_playlist_screen_set_geometry(NcPlaylistScreen *screen,
                                     int32 start_x, int32 width,
                                     int32 main_start_y,
                                     int32 main_height);
void nc_playlist_screen_set_menu(NcPlaylistScreen *screen, NcMenu *menu);
void nc_playlist_screen_set_mouse_config(NcPlaylistScreen *screen,
                                         int32 lines_scrolled,
                                         bool scroll_whole_page);
NcScreen *nc_playlist_screen_base(NcPlaylistScreen *screen);
NcMenu *nc_playlist_screen_menu(NcPlaylistScreen *screen);
int32 nc_playlist_screen_height(NcPlaylistScreen *screen);
void nc_playlist_screen_scroll(NcPlaylistScreen *screen,
                               enum NcScroll where);
int32 nc_playlist_screen_goto_y(NcPlaylistScreen *screen, int32 y);
int32 nc_playlist_screen_activate_current(NcPlaylistScreen *screen);
void nc_playlist_screen_mouse_button_pressed(NcPlaylistScreen *screen,
                                             MEVENT event);

void playlist_screen_init(PlaylistScreen *screen,
                          int32 start_x, int32 width,
                          int32 main_start_y, int32 main_height,
                          NcColor color, NcBorder border);
void playlist_screen_destroy(PlaylistScreen *screen);
int32 playlist_screen_unregister(PlaylistScreen *screen);
NcScreen *playlist_screen_base(PlaylistScreen *screen);
NcPlaylistScreen *playlist_screen_playlist(PlaylistScreen *screen);
NcSongMenu *playlist_screen_song_menu(PlaylistScreen *screen);
NcMenu *playlist_screen_menu(PlaylistScreen *screen);
NcWindow *playlist_screen_window(PlaylistScreen *screen);
void playlist_screen_update_column_title(PlaylistScreen *screen);
void playlist_screen_set_geometry(PlaylistScreen *screen,
                                  int32 start_x, int32 width,
                                  int32 main_start_y,
                                  int32 main_height);
void playlist_screen_set_mouse_config(PlaylistScreen *screen,
                                      int32 lines_scrolled,
                                      bool scroll_whole_page);
void playlist_screen_set_highlighting(PlaylistScreen *screen,
                                      bool enabled);
bool playlist_screen_is_highlighting(PlaylistScreen *screen);
void playlist_screen_request_highlighting(PlaylistScreen *screen);
void playlist_screen_clear(PlaylistScreen *screen);
int32 playlist_screen_reload_from_mpd(PlaylistScreen *screen,
                                     NcmMpdClient *client,
                                     int32 version,
                                     int32 playlist_length,
                                     NcmError *ncm_error);
int32 playlist_screen_song_count(PlaylistScreen *screen);
bool playlist_screen_is_empty(PlaylistScreen *screen);
int32 playlist_screen_current_song(PlaylistScreen *screen,
                                   NcmSong *song);
int32 playlist_screen_update_current_mutable_song(PlaylistScreen *screen,
                                                 NcmMutableSong *song);
int32 playlist_screen_now_playing_song(PlaylistScreen *screen,
                                       int32 position,
                                       NcmSong *song);
int32 playlist_screen_locate_position(PlaylistScreen *screen,
                                     int32 position);
int32 playlist_screen_selected_songs(PlaylistScreen *screen,
                                    NcmSongArray *songs);
bool playlist_screen_has_sortable_range(PlaylistScreen *screen);
int32 playlist_screen_copy_sort_range(PlaylistScreen *screen,
                                     NcmSongArray *songs,
                                     int32 *start_position,
                                     NcmError *ncm_error);
int32 playlist_screen_apply_filter(PlaylistScreen *screen,
                                   char *pattern, int32 pattern_len,
                                   NcmError *ncm_error);
void playlist_screen_clear_filter(PlaylistScreen *screen);
int32 playlist_screen_search(PlaylistScreen *screen,
                             char *pattern, int32 pattern_len,
                             bool forward, bool wrap,
                             bool skip_current, NcmError *ncm_error);
int32 playlist_screen_set_selected_priority(PlaylistScreen *screen,
                                           NcmMpdClient *client,
                                           int32 priority,
                                           NcmError *ncm_error);
void playlist_screen_reload_total_length(PlaylistScreen *screen);
void playlist_screen_reload_remaining(PlaylistScreen *screen);

/* screens/nc_search_engine.h */
#define SEARCH_ENGINE_CONSTRAINT_COUNT 11
#define SEARCH_ENGINE_FIRST_SEPARATOR_ROW 11
#define SEARCH_ENGINE_SEARCH_SOURCE_ROW 12
#define SEARCH_ENGINE_SEARCH_MODE_ROW 13
#define SEARCH_ENGINE_SECOND_SEPARATOR_ROW 14
#define SEARCH_ENGINE_SEARCH_BUTTON_ROW 15
#define SEARCH_ENGINE_RESET_BUTTON_ROW 16
#define SEARCH_ENGINE_RESULT_SEPARATOR_ROW 17
#define SEARCH_ENGINE_RESULT_SUMMARY_ROW 18
#define SEARCH_ENGINE_RESULT_END_SEPARATOR_ROW 19
#define SEARCH_ENGINE_STATIC_ROW_COUNT 20

#define ENUM_NAME SearchEngineSearchMode
#define ENUM_PREFIX_ SEARCH_ENGINE_SEARCH_MODE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(SEARCH_ENGINE_SEARCH_MODE_LITERAL,                                      \
       Match if tag contains searched phrase (no regexes))                    \
    XX(SEARCH_ENGINE_SEARCH_MODE_REGEX,                                        \
       Match if tag contains searched phrase (regexes supported))              \
    XX(SEARCH_ENGINE_SEARCH_MODE_EXACT,                                        \
       Match only if both values are the same)
#include "cbase/xenums.c"

#define ENUM_NAME SearchEnginePromptResult
#define ENUM_PREFIX_ SEARCH_ENGINE_PROMPT_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(SEARCH_ENGINE_PROMPT_ERROR)                                             \
    XX(SEARCH_ENGINE_PROMPT_ABORTED)                                           \
    XX(SEARCH_ENGINE_PROMPT_ACCEPTED)
#include "cbase/xenums.c"

typedef struct SearchEngineHooks {
    NcmMpdClient *client;
    int32 (*list_database_songs)(void *user, NcmSongArray *songs,
                                 NcmError *ncm_error);
    int32 (*snapshot_playlist)(void *user, NcmSongArray *songs,
                               NcmError *ncm_error);
    enum SearchEnginePromptResult (*prompt_constraint)(
        void *user, char *label, int32 label_len, StrBuilder *initial,
        StrBuilder *result);
    void (*status_message)(void *user, char *message, int32 message_len);
    int32 (*add_song)(void *user, NcmSong *song, bool play,
                      NcmError *ncm_error);
    int32 (*format_song)(void *user, NcmSong *song, StrBuilder *text);
    void *user;
} SearchEngineHooks;

typedef struct SearchEngineScreen {
    NcScreen screen;
    NcSearchRowMenu rows;
    NcWindow window;
    SearchEngineHooks hooks;
    StrBuilder constraints[ SEARCH_ENGINE_CONSTRAINT_COUNT];
    StrBuilder filter_constraint;
    StrBuilder search_constraint;
    StrBuilder row_text;
    StrBuilder title;
    StrBuilder column_title;
    NcmRegex filter_regex;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;
    int32 lines_scrolled;
    int32 result_count;

    enum SearchEngineSearchMode search_mode;
    bool search_in_database;
    bool mouse_list_scroll_whole_page;
    bool match_to_pattern;
    bool filter_enabled;
    bool prepared;
    bool result_rows_present;
    bool constraints_locked;
    bool registered;
} SearchEngineScreen;

void search_engine_screen_init(SearchEngineScreen *screen,
                               int32 start_x, int32 width,
                               int32 main_start_y,
                               int32 main_height, NcColor color,
                               NcBorder border);
void search_engine_screen_destroy(SearchEngineScreen *screen);
NcScreen *search_engine_screen_base(SearchEngineScreen *screen);
NcMenu *search_engine_screen_menu(SearchEngineScreen *screen);
NcWindow *search_engine_screen_window(SearchEngineScreen *screen);
void search_engine_screen_set_mouse_config(
    SearchEngineScreen *screen, int32 lines_scrolled,
    bool whole_page);
bool search_engine_screen_has_locked_constraints(
    SearchEngineScreen *screen);
int32 search_engine_screen_format_song_text(
    SearchEngineScreen *screen, NcmSong *song, StrBuilder *text);
void search_engine_screen_update_column_title(
    SearchEngineScreen *screen);
void search_engine_screen_prepare_static_rows(
    SearchEngineScreen *screen);
int32 search_engine_screen_update_search_source_row(
    SearchEngineScreen *screen);
void search_engine_screen_reset(SearchEngineScreen *screen);
int32 search_engine_screen_add_buffer_with_flags(
    SearchEngineScreen *screen, NcBuffer *buffer, uint32 flags);
int32 search_engine_screen_set_search_mode(
    SearchEngineScreen *screen,
    enum SearchEngineSearchMode mode);
void search_engine_screen_set_search_source(
    SearchEngineScreen *screen, bool search_in_database);
void search_engine_screen_set_hooks(
    SearchEngineScreen *screen, SearchEngineHooks hooks);
void search_engine_screen_status_message(
    SearchEngineScreen *screen, char *message, int32 message_len);
bool search_engine_screen_can_run_current(
    SearchEngineScreen *screen);
int32 search_engine_screen_start_searching(
    SearchEngineScreen *screen, NcmMpdClient *client,
    NcmError *ncm_error);
enum DisplayMode search_engine_screen_toggle_display_mode(
    SearchEngineScreen *screen);
bool search_engine_screen_can_search(
    SearchEngineScreen *screen);
int32 search_engine_screen_current_song(
    SearchEngineScreen *screen, NcmSong *song);
int32 search_engine_screen_selected_songs(
    SearchEngineScreen *screen, NcmSongArray *songs);
int32 search_engine_screen_apply_filter(
    SearchEngineScreen *screen, char *pattern, int32 pattern_len,
    NcmError *ncm_error);
void search_engine_screen_clear_filter(
    SearchEngineScreen *screen);
int32 search_engine_screen_search(SearchEngineScreen *screen,
                                  char *pattern, int32 pattern_len,
                                  bool forward, bool wrap,
                                  bool skip_current,
                                  NcmError *ncm_error);

/* screens/nc_sel_items_adder.h */
typedef struct PlaylistScreen PlaylistScreen;

#define ENUM_NAME SelectedItemsAdderMenu
#define ENUM_PREFIX_ SELECTED_ITEMS_ADDER_MENU_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(SELECTED_ITEMS_ADDER_MENU_PLAYLISTS, Playlists)                         \
    XX(SELECTED_ITEMS_ADDER_MENU_POSITIONS, Positions)
#include "cbase/xenums.c"

typedef struct SelectedItemsAdderScreen {
    NcScreen screen;
    NcEditorActionMenu playlist_selector;
    NcEditorActionMenu position_selector;
    NcWindow playlist_window;
    NcWindow position_window;
    NcmSongArray selected_songs;
    NcmRegex search_regex;
    StrBuilder search_constraint;
    PlaylistScreen *playlist;
    NcScreen *previous_screen;
    NcmMpdClient *client;

    int32 playlist_width;
    int32 playlist_height;
    int32 position_width;
    int32 position_height;
    int32 active_menu;

    bool local_browser;
    bool search_enabled;
    bool registered;
    bool ready;
} SelectedItemsAdderScreen;

void selected_items_adder_screen_init(
    SelectedItemsAdderScreen *screen, int32 start_x, int32 start_y,
    int32 width, int32 height, NcColor color, NcBorder border);
void selected_items_adder_screen_destroy(SelectedItemsAdderScreen *screen);
NcScreen *selected_items_adder_screen_base(SelectedItemsAdderScreen *screen);
NcMenu *selected_items_adder_screen_active_menu(SelectedItemsAdderScreen *);
NcWindow *selected_items_adder_screen_active_window(SelectedItemsAdderScreen *);
int32 selected_items_adder_screen_open(SelectedItemsAdderScreen *screen,
                                       NcmSongArray *songs,
                                       PlaylistScreen *playlist,
                                       NcmMpdClient *client,
                                       NcmError *ncm_error);
int32 selected_items_adder_screen_run_current(
    SelectedItemsAdderScreen *screen);
int32 selected_items_adder_screen_return_to_previous(
    SelectedItemsAdderScreen *screen);
int32 selected_items_adder_screen_search(
    SelectedItemsAdderScreen *screen, char *pattern,
    int32 pattern_len, uint32 regex_flags, bool forward, bool wrap,
    bool skip_current, NcmError *ncm_error);

/* screens/nc_sort_playlist.h */
typedef struct NcmMpdClient NcmMpdClient;
typedef struct PlaylistScreen PlaylistScreen;

typedef struct SortPlaylistDialog {
    NcScreen screen;
    NcEditorSortMenu rows;
    NcWindow window;
    NcmSongArray songs;

    PlaylistScreen *playlist;
    NcScreen *previous_screen;
    NcmMpdClient *client;

    int32 start_x;
    int32 start_y;
    int32 width;
    int32 height;
    int32 start_position;

    bool ignore_leading_the;
    bool ready;
} SortPlaylistDialog;

void sort_playlist_dialog_init(SortPlaylistDialog *dialog,
                               int32 start_x, int32 start_y,
                               int32 width, int32 height,
                               NcColor color, NcBorder border);
void sort_playlist_dialog_destroy(SortPlaylistDialog *dialog);
NcScreen *sort_playlist_dialog_base(SortPlaylistDialog *dialog);
NcEditorSortMenu *sort_playlist_dialog_menu(SortPlaylistDialog *dialog);
int32 sort_playlist_dialog_open(
    SortPlaylistDialog *dialog, PlaylistScreen *playlist,
    NcmMpdClient *client, bool ignore_leading_the, NcmError *ncm_error);
int32 sort_playlist_dialog_move_current_up(SortPlaylistDialog *dialog);
int32 sort_playlist_dialog_move_current_down(SortPlaylistDialog *dialog);

/* screens/nc_tag_editor.h */
#define ENUM_NAME TagEditorColumn
#define ENUM_PREFIX_ TAG_EDITOR_COLUMN_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(TAG_EDITOR_COLUMN_DIRECTORIES, Directories)                             \
    XX(TAG_EDITOR_COLUMN_TAG_TYPES, Tag types)                                 \
    XX(TAG_EDITOR_COLUMN_TAGS, Tags)
#include "cbase/xenums.c"

#define ENUM_NAME TagEditorParserMode
#define ENUM_PREFIX_ TAG_EDITOR_PARSER_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(TAG_EDITOR_PARSER_NONE, Pattern)                                        \
    XX(TAG_EDITOR_PARSER_TAGS_FROM_FILENAME, Get tags from filename)           \
    XX(TAG_EDITOR_PARSER_RENAME_FILES, Rename files)
#include "cbase/xenums.c"

#define ENUM_NAME TagEditorFocus
#define ENUM_PREFIX_ TAG_EDITOR_FOCUS_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(TAG_EDITOR_FOCUS_DIRECTORIES, Directories)                              \
    XX(TAG_EDITOR_FOCUS_TAG_TYPES, Tag types)                                  \
    XX(TAG_EDITOR_FOCUS_TAGS, Tags)                                            \
    XX(TAG_EDITOR_FOCUS_PARSER_CHOICE, Pattern)                                \
    XX(TAG_EDITOR_FOCUS_PARSER_ACTIONS, Pattern actions)                       \
    XX(TAG_EDITOR_FOCUS_PARSER_LEGEND, Legend)                                 \
    XX(TAG_EDITOR_FOCUS_PARSER_PREVIEW, Preview)
#include "cbase/xenums.c"

#define ENUM_NAME TagEditorPromptResult
#define ENUM_PREFIX_ TAG_EDITOR_PROMPT_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(TAG_EDITOR_PROMPT_ERROR)                                                \
    XX(TAG_EDITOR_PROMPT_ABORTED)                                              \
    XX(TAG_EDITOR_PROMPT_ACCEPTED)
#include "cbase/xenums.c"

typedef struct TagEditorHooks {
    enum TagEditorPromptResult (*prompt)(
        void *user, char *label, int32 label_len, NcmStringView initial,
        StrBuilder *result);
    bool (*confirm)(void *user, char *message, int32 message_len);
    void (*status_message)(void *user, char *message, int32 message_len);
    void (*update_directory)(void *user, char *directory,
                             int32 directory_len);
    void *user;
} TagEditorHooks;

typedef struct TagEditorScreen {
    NcScreen screen;
    NcEditorPairMenu directories;
    NcEditorStringMenu tag_types;
    NcTagRowMenu tags;
    NcEditorStringMenu parser_dialog;
    NcEditorStringMenu parser_rows;
    NcEditorStringMenu parser_actions;
    NcWindow directories_window;
    NcWindow tag_types_window;
    NcWindow tags_window;
    NcWindow parser_dialog_window;
    NcWindow parser_window;
    NcWindow parser_helper_window;
    TagEditorHooks hooks;
    StrBuilder current_dir;
    StrBuilder displayed_dir;
    StrBuilder observed_dir;
    StrBuilder highlighted_dir;
    StrBuilder directories_title;
    StrBuilder tag_types_title;
    StrBuilder tags_title;
    StrBuilder parser_dialog_title;
    StrBuilder parser_title;
    StrBuilder parser_helper_title;
    StrBuilder parser_legend;
    StrBuilder parser_preview;
    StrBuilderArray recent_patterns;
    StrBuilder directory_filter_constraint;
    StrBuilder tag_filter_constraint;
    StrBuilder directory_search_constraint;
    StrBuilder tag_search_constraint;
    StrBuilder pattern;

    NcmRegex directory_filter_regex;
    NcmRegex tag_filter_regex;
    NcmRegex directory_search_regex;
    NcmRegex tag_search_regex;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;
    int32 left_width;
    int32 middle_start_x;
    int32 middle_width;
    int32 right_start_x;
    int32 right_width;
    int32 parser_dialog_start_x;
    int32 parser_dialog_start_y;
    int32 parser_dialog_width;
    int32 parser_dialog_height;
    int32 parser_start_x;
    int32 parser_start_y;
    int32 parser_width;
    int32 parser_width_one;
    int32 parser_width_two;
    int32 parser_height;
    int32 parser_helper_start_x;
    enum TagEditorColumn active_column;
    int32 last_directory_highlight;
    int32 last_tag_type_highlight;
    int32 last_known_directory_count;
    int32 last_known_tag_count;
    int32 window_timeout_ms;

    enum TagEditorParserMode parser_mode;
    enum TagEditorFocus active_focus;
    bool directories_update_requested;
    bool tags_update_requested;
    bool directory_filter_enabled;
    bool tag_filter_enabled;
    bool directory_search_enabled;
    bool tag_search_enabled;
    bool parser_preview_enabled;
    bool recent_patterns_loaded;
    bool displayed_dir_valid;
    bool observed_dir_valid;
    bool registered;
} TagEditorScreen;

void tag_editor_screen_init(TagEditorScreen *screen,
                            int32 start_x, int32 width,
                            int32 main_start_y,
                            int32 main_height, NcColor color,
                            NcBorder border);
void tag_editor_screen_destroy(TagEditorScreen *screen);
NcScreen *tag_editor_screen_base(TagEditorScreen *screen);

void tag_editor_screen_set_hooks(TagEditorScreen *screen,
                                 TagEditorHooks hooks);
NcMenu *tag_editor_screen_active_menu(TagEditorScreen *screen);
NcWindow *tag_editor_screen_active_window(TagEditorScreen *screen);
void tag_editor_screen_set_geometry(TagEditorScreen *screen,
                                    int32 start_x, int32 width,
                                    int32 main_start_y,
                                    int32 main_height);
void tag_editor_screen_clear_directories(TagEditorScreen *screen);
void tag_editor_screen_clear_stale_tags(TagEditorScreen *screen);
void tag_editor_screen_finish_directory_change(TagEditorScreen *screen);
void tag_editor_screen_set_current_dir(TagEditorScreen *screen,
                                       char *dir, int32 dir_len);
int32 tag_editor_screen_current_dir(TagEditorScreen *screen,
                                    NcmStringView *view);
int32 tag_editor_screen_current_directory_path(
    TagEditorScreen *screen, NcmStringView *view);
int32 tag_editor_screen_enter_directory(TagEditorScreen *screen);
int32 tag_editor_screen_go_to_parent(TagEditorScreen *screen);
int32 tag_editor_screen_locate_song(TagEditorScreen *screen,
                                    NcmSong *song);
bool tag_editor_screen_rename_directory_available(
    TagEditorScreen *screen, char *music_dir, int32 music_dir_len);
int32 tag_editor_screen_rename_current_directory(
    TagEditorScreen *screen, char *music_dir, int32 music_dir_len);
void tag_editor_screen_add_directory(TagEditorScreen *screen,
                                     char *label, int32 label_len,
                                     char *path, int32 path_len);
void tag_editor_screen_load_songs(TagEditorScreen *screen,
                                  NcmSongArray *songs);
void tag_editor_screen_add_mutable_song(TagEditorScreen *screen,
                                        NcmMutableSong *song);
int32 tag_editor_screen_selected_songs(TagEditorScreen *screen,
                                       NcmSongArray *songs);
bool tag_editor_screen_previous_column_available(TagEditorScreen *screen);
bool tag_editor_screen_next_column_available(TagEditorScreen *screen);
void tag_editor_screen_previous_column(TagEditorScreen *screen);
void tag_editor_screen_next_column(TagEditorScreen *screen);
int32 tag_editor_screen_apply_tag_to_selection(
    TagEditorScreen *screen, enum NcmTagsField field, char *value,
    int32 value_len, char *separator, int32 separator_len);
int32 tag_editor_screen_number_tracks(TagEditorScreen *screen,
                                      bool extended);
void tag_editor_screen_capitalize_first_letters(
    TagEditorScreen *screen);
void tag_editor_screen_lower_all_letters(TagEditorScreen *screen);
void tag_editor_screen_clear_modifications(TagEditorScreen *screen);
int32 tag_editor_screen_save_modified(TagEditorScreen *screen,
                                      char *music_dir);
bool tag_editor_screen_save_action_available(TagEditorScreen *screen);
int32 tag_editor_screen_apply_directory_filter(
    TagEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, NcmError *ncm_error);
int32 tag_editor_screen_apply_tag_filter(
    TagEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, NcmError *ncm_error);
int32 tag_editor_screen_search(
    TagEditorScreen *screen, char *pattern, int32 pattern_len,
    bool forward, bool wrap, bool skip_current, NcmError *ncm_error);
void tag_editor_screen_prepare_parser_rows(
    TagEditorScreen *screen, enum TagEditorParserMode mode,
    char *pattern, int32 pattern_len);
void tag_editor_screen_show_parser_dialog(TagEditorScreen *screen);
void tag_editor_screen_show_parser_actions(
    TagEditorScreen *screen, enum TagEditorParserMode mode);
void tag_editor_screen_show_parser_legend(TagEditorScreen *screen);
void tag_editor_screen_show_parser_preview(TagEditorScreen *screen);
void tag_editor_screen_close_parser(TagEditorScreen *screen);
int32 tag_editor_parse_filename(NcmMutableSong *song, char *mask,
                                int32 mask_len, bool preview,
                                StrBuilder *preview_buffer);
int32 tag_editor_generate_filename(NcmMutableSong *song,
                                   char *pattern, int32 pattern_len,
                                   StrBuilder *filename);
int32 tag_editor_song_display_value(NcmMutableSong *song,
                                    enum NcmTagsField field,
                                    StrBuilder *buffer);

/* screens/nc_tiny_tag_editor.h */
#define TINY_TAG_EDITOR_TAG_ROW(FIELD) \
    ((int32) TINY_TAG_EDITOR_FIRST_TAG_ROW + (int32)(FIELD))

enum TinyTagEditorRow {
    TINY_TAG_EDITOR_FILE_NAME_INFO_ROW,
    TINY_TAG_EDITOR_DIRECTORY_INFO_ROW,
    TINY_TAG_EDITOR_UNUSED_INFO_ROW,
    TINY_TAG_EDITOR_LENGTH_INFO_ROW,
    TINY_TAG_EDITOR_BITRATE_INFO_ROW,
    TINY_TAG_EDITOR_SAMPLE_RATE_INFO_ROW,
    TINY_TAG_EDITOR_CHANNELS_INFO_ROW,
    TINY_TAG_EDITOR_FIRST_SEPARATOR_ROW,
    TINY_TAG_EDITOR_FIRST_TAG_ROW,
    TINY_TAG_EDITOR_LAST_TAG_ROW =
        TINY_TAG_EDITOR_FIRST_TAG_ROW + NCM_TAGS_FIELD_COUNT - 1,
    TINY_TAG_EDITOR_SECOND_SEPARATOR_ROW,
    TINY_TAG_EDITOR_FILE_NAME_EDIT_ROW,
    TINY_TAG_EDITOR_THIRD_SEPARATOR_ROW,
    TINY_TAG_EDITOR_SAVE_ROW,
    TINY_TAG_EDITOR_CANCEL_ROW,
    TINY_TAG_EDITOR_ROW_COUNT,
};

#define ENUM_NAME TinyTagEditorOpenResult
#define ENUM_PREFIX_ TINY_TAG_EDITOR_OPEN_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(TINY_TAG_EDITOR_OPEN_SUCCESS)                                           \
    XX(TINY_TAG_EDITOR_OPEN_INVALID_ARGUMENT)                                  \
    XX(TINY_TAG_EDITOR_OPEN_STREAM)                                            \
    XX(TINY_TAG_EDITOR_OPEN_MISSING_MUSIC_DIRECTORY)                           \
    XX(TINY_TAG_EDITOR_OPEN_UNREADABLE_FILE)                                   \
    XX(TINY_TAG_EDITOR_OPEN_PREPARE_FAILED)
#include "cbase/xenums.c"

#define ENUM_NAME TinyTagEditorPromptResult
#define ENUM_PREFIX_ TINY_TAG_EDITOR_PROMPT_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(TINY_TAG_EDITOR_PROMPT_ERROR)                                           \
    XX(TINY_TAG_EDITOR_PROMPT_ABORTED)                                         \
    XX(TINY_TAG_EDITOR_PROMPT_ACCEPTED)
#include "cbase/xenums.c"

typedef struct TinyTagEditorHooks {
    enum TinyTagEditorPromptResult (*prompt)(
        void *user, char *label, int32 label_len, NcmStringView initial,
        StrBuilder *result);
    void (*status_message)(void *user, char *message, int32 message_len);
    int32 (*taglib_open)(void *user, NcmTaglibFile *file, char *path,
                          int32 path_len);
    int32 (*taglib_audio_properties)(
        void *user, NcmTaglibFile *file,
        NcmTaglibAudioProperties *properties);
    bool (*taglib_file_can_set_extended_tags)(void *user, NcmTaglibFile *file);
    void (*taglib_close)(void *user, NcmTaglibFile *file);
    int32 (*write_song)(void *user, NcmMutableSong *song, char *music_dir);
    void (*update_directory)(void *user, char *directory,
                             int32 directory_len);
    void (*update_playlist_song)(void *user, NcmMutableSong *song);
    void (*request_browser_update)(void *user);
    void (*switch_to_screen)(void *user, NcScreen *screen);
    void *user;
} TinyTagEditorHooks;

typedef struct TinyTagEditorScreen {
    NcScreen screen;
    NcEditorBufferMenu rows;
    NcWindow window;
    TinyTagEditorHooks hooks;
    NcmMutableSong edited;
    StrBuilder music_dir;
    StrBuilder tag_separator;
    NcScreen *previous_screen;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;

    bool has_edited;
    bool show_duplicate_tags;
    bool registered;
} TinyTagEditorScreen;

void tiny_tag_editor_screen_init(
    TinyTagEditorScreen *screen, int32 start_x, int32 width,
    int32 main_start_y, int32 main_height, NcColor color, NcBorder border);
void tiny_tag_editor_screen_destroy(
    TinyTagEditorScreen *screen);
NcScreen *tiny_tag_editor_screen_base(
    TinyTagEditorScreen *screen);

void tiny_tag_editor_screen_set_hooks(
    TinyTagEditorScreen *screen, TinyTagEditorHooks hooks);
NcEditorBufferMenu *tiny_tag_editor_screen_rows(
    TinyTagEditorScreen *screen);
enum TinyTagEditorOpenResult
tiny_tag_editor_screen_open_song(
    TinyTagEditorScreen *screen, NcmSong *song,
    char *music_dir, int32 music_dir_len, char *tag_separator,
    int32 tag_separator_len, bool show_duplicate_tags, StrBuilder *path);
int32 tiny_tag_editor_screen_run_row(
    TinyTagEditorScreen *screen, int32 row);
int32 tiny_tag_editor_screen_run_current(
    TinyTagEditorScreen *screen);
bool tiny_tag_editor_screen_action_runnable(
    TinyTagEditorScreen *screen);

/* screens/nc_browser.h */
typedef struct BrowserScreen {
    NcScreen screen;
    NcBrowserEntryMenu entries;
    NcWindow window;
    StrBuilder current_directory;
    StrBuilder last_highlighted_directory;
    StrBuilder title_text;
    StrBuilder column_title_text;
    StrBuilder filter_constraint;
    StrBuilder search_constraint;
    StrBuilder item_text_buffer;
    StrBuilder path_buffer;
    StrBuilder scratch_buffer;
    StrBuilderArray supported_extensions;
    NcmRegex filter_regex;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;
    int32 lines_scrolled;
    int32 title_scroll_beginning;

    enum DisplayMode active_display_mode;

    bool mouse_list_scroll_whole_page;
    bool redraw_header;
    bool update_requested;
    bool local_browser;
    bool filter_enabled;
    bool registered;
} BrowserScreen;

void browser_screen_init(BrowserScreen *screen,
                         int32 start_x, int32 width,
                         int32 main_start_y, int32 main_height,
                         NcColor color, NcBorder border);
void browser_screen_destroy(BrowserScreen *screen);
NcScreen *browser_screen_base(BrowserScreen *screen);
NcBrowserEntryMenu *browser_screen_entries(
    BrowserScreen *screen);
NcMenu *browser_screen_menu(BrowserScreen *screen);
NcWindow *browser_screen_window(BrowserScreen *screen);
void browser_screen_set_mouse_config(BrowserScreen *screen,
                                     int32 lines_scrolled,
                                     bool scroll_whole_page);
void browser_screen_clear(BrowserScreen *screen);
void browser_screen_add_item_move(BrowserScreen *screen,
                                  NcmMpdItem *item);
int32 browser_screen_sort(BrowserScreen *screen);
int32 browser_screen_set_current_directory(
    BrowserScreen *screen, char *directory, int32 directory_len);
NcmStringView browser_screen_current_directory(
    BrowserScreen *screen);
void browser_screen_update_title_text(BrowserScreen *screen);
void browser_screen_update_column_title(BrowserScreen *screen);
void browser_screen_draw_header(BrowserScreen *screen);
void browser_screen_set_display_mode(BrowserScreen *screen,
                                     enum DisplayMode mode);
int32 browser_screen_fetch_supported_extensions(
    BrowserScreen *screen, NcmMpdClient *client, NcmError *ncm_error);
void browser_screen_clear_update_request(BrowserScreen *screen);
bool browser_screen_is_in_root_directory(BrowserScreen *screen);
void browser_screen_set_local(BrowserScreen *screen,
                              bool local_browser);
bool browser_screen_is_local(BrowserScreen *screen);
int32 browser_screen_change_browse_mode(BrowserScreen *screen,
                                       NcmMpdClient *client,
                                       NcmError *ncm_error);
NcmMpdItem *browser_screen_current_item(BrowserScreen *screen);
int32 browser_screen_current_song(BrowserScreen *screen,
                                 NcmSong *song);
int32 browser_screen_selected_songs(BrowserScreen *screen,
                                   NcmSongArray *songs);
int32 browser_screen_delete_items(BrowserScreen *screen,
                                 NcmMpdClient *client,
                                 NcmError *ncm_error);
bool browser_screen_has_current_directory_path(
    BrowserScreen *screen, NcmStringView *path);
bool browser_screen_has_current_playlist_path(
    BrowserScreen *screen, NcmStringView *path);
bool browser_screen_can_rename_directory(
    BrowserScreen *screen);
bool browser_screen_can_rename_playlist(
    BrowserScreen *screen);
int32 browser_screen_rename_current_directory(
    BrowserScreen *screen, char *new_path, int32 new_path_len,
    NcmMpdClient *client, NcmError *ncm_error);
int32 browser_screen_rename_current_playlist(
    BrowserScreen *screen, char *new_path, int32 new_path_len,
    NcmMpdClient *client, NcmError *ncm_error);
int32 browser_screen_locate_song(BrowserScreen *screen,
                                NcmSong *song,
                                NcmMpdClient *client,
                                NcmError *ncm_error);
int32 browser_screen_enter_directory(BrowserScreen *screen);
int32 browser_screen_go_to_parent(BrowserScreen *screen);
int32 browser_screen_apply_filter(BrowserScreen *screen,
                                  char *pattern, int32 pattern_len,
                                  NcmError *ncm_error);
void browser_screen_clear_filter(BrowserScreen *screen);
int32 browser_screen_search(BrowserScreen *screen,
                            char *pattern, int32 pattern_len,
                            bool forward, bool wrap,
                            bool skip_current, NcmError *ncm_error);
void browser_screen_request_update(BrowserScreen *screen);
bool browser_screen_item_is_parent(NcmMpdItem *item);

/* screens/song_info.h */
typedef struct NcmSongInfoMetadata {
    char *name;
    enum NcmSongGetter get;
    enum NcmTagsField field;
} NcmSongInfoMetadata;

extern NcmSongInfoMetadata ncm_song_info_tags[];

/* screens/app_screens.h */
typedef struct HelpScreen HelpScreen;
typedef struct OutputsScreen OutputsScreen;
typedef struct SearchEngineScreen SearchEngineScreen;
typedef struct ServerInfoScreen ServerInfoScreen;
typedef struct SongInfoScreen SongInfoScreen;

void app_screens_init_all(void);
void app_screens_register_initial(void);
void app_screens_request_registered_resize(void);
NcScreen *app_screens_find_type(enum ScreenType screen_type);
int32 app_screens_switch_to_type(enum ScreenType screen_type);
int32 app_screens_lock_current(void);
enum ScreenType app_screens_current_type(void);

#define NCM_APP_SCREEN_DECLARE_COMMON(suffix)                                  \
    void app_screen_##suffix##_init(void);                                     \
    void app_screen_##suffix##_register(void);                                 \
    bool app_screen_##suffix##_is_current(void);                               \
    NcScreen *app_screen_##suffix##_base(void);

NCM_APP_SCREEN_IS_CURRENT_TYPES(NCM_APP_SCREEN_DECLARE_COMMON)

#undef NCM_APP_SCREEN_DECLARE_COMMON

#define NCM_APP_SCREEN_DECLARE_DIRECT_ACCESSOR(                                \
    suffix, type, storage, base_expr                                           \
)                                                                              \
    type *app_screen_##suffix(void);

NCM_APP_SCREEN_DIRECT_ACCESSOR_TYPES(NCM_APP_SCREEN_DECLARE_DIRECT_ACCESSOR)

#undef NCM_APP_SCREEN_DECLARE_DIRECT_ACCESSOR

#define NCM_APP_SCREEN_DECLARE_TYPED_WRAPPED_ACCESSOR(                         \
    suffix, function, type, expr                                               \
)                                                                              \
    type *function(void);

NCM_APP_SCREEN_TYPED_WRAPPED_ACCESSOR_TYPES(
    NCM_APP_SCREEN_DECLARE_TYPED_WRAPPED_ACCESSOR)

#undef NCM_APP_SCREEN_DECLARE_TYPED_WRAPPED_ACCESSOR

#define NCM_APP_SCREEN_DECLARE_VOID_SWITCH(suffix)                             \
    void app_screen_##suffix##_switch_to(void);

NCM_APP_SCREEN_SIMPLE_SWITCH_TYPES(NCM_APP_SCREEN_DECLARE_VOID_SWITCH)
NCM_APP_SCREEN_REGISTER_SWITCH_TYPES(NCM_APP_SCREEN_DECLARE_VOID_SWITCH)

#undef NCM_APP_SCREEN_DECLARE_VOID_SWITCH

void app_screen_lastfm_switch_to(void);
VisualizerScreen *app_screen_visualizer(void);
void app_screen_lyrics_set_resize(void);
void app_screen_lyrics_switch_to(void);
void app_screen_browser_fetch_supported_extensions(void);
int32 app_screen_selected_items_adder_open(NcmSongArray *songs,
                                           NcmError *ncm_error);
int32 app_screen_sort_playlist_dialog_switch_to(void);
void app_screen_outputs_toggle(void);
void app_screen_outputs_fetch_list(void);
void app_screen_outputs_refresh_if_visible(void);

#endif /* NCMPCPP_NC_SCREENS_H */
