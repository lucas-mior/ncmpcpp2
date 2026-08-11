#if !defined(NCMPCPP_SCREEN_DEFS_H)
#define NCMPCPP_SCREEN_DEFS_H

#include "cbase.h"

#include "config.h"

#define NCM_SCREEN_FLAG_NONE 0
#define NCM_SCREEN_FLAG_STARTUP 1

#define NCM_SCREEN_TYPE_BROWSER_ENTRY(X) \
    X(NCM_SCREEN_TYPE_BROWSER, NC_SCREEN_TYPE_BROWSER, 1, browser, \
      NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_HELP_ENTRY(X) \
    X(NCM_SCREEN_TYPE_HELP, NC_SCREEN_TYPE_HELP, 2, help, \
      NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_LASTFM_ENTRY(X) \
    X(NCM_SCREEN_TYPE_LASTFM, NC_SCREEN_TYPE_LASTFM, 3, last_fm, \
      NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_LYRICS_ENTRY(X) \
    X(NCM_SCREEN_TYPE_LYRICS, NC_SCREEN_TYPE_LYRICS, 4, lyrics, \
      NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_MEDIA_LIBRARY_ENTRY(X) \
    X(NCM_SCREEN_TYPE_MEDIA_LIBRARY, NC_SCREEN_TYPE_MEDIA_LIBRARY, 5, \
      media_library, NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_OUTPUTS_ENTRY(X) \
    X(NCM_SCREEN_TYPE_OUTPUTS, NC_SCREEN_TYPE_OUTPUTS, 6, outputs, \
      NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_PLAYLIST_ENTRY(X) \
    X(NCM_SCREEN_TYPE_PLAYLIST, NC_SCREEN_TYPE_PLAYLIST, 7, playlist, \
      NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_PLAYLIST_EDITOR_ENTRY(X) \
    X(NCM_SCREEN_TYPE_PLAYLIST_EDITOR, NC_SCREEN_TYPE_PLAYLIST_EDITOR, \
      8, playlist_editor, NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_SEARCH_ENGINE_ENTRY(X) \
    X(NCM_SCREEN_TYPE_SEARCH_ENGINE, NC_SCREEN_TYPE_SEARCH_ENGINE, \
      9, search_engine, NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER_ENTRY(X) \
    X(NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER, \
      NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER, 10, selected_items_adder, \
      NCM_SCREEN_FLAG_NONE)
#define NCM_SCREEN_TYPE_SERVER_INFO_ENTRY(X) \
    X(NCM_SCREEN_TYPE_SERVER_INFO, NC_SCREEN_TYPE_SERVER_INFO, 11, \
      server_info, NCM_SCREEN_FLAG_NONE)
#define NCM_SCREEN_TYPE_SONG_INFO_ENTRY(X) \
    X(NCM_SCREEN_TYPE_SONG_INFO, NC_SCREEN_TYPE_SONG_INFO, 12, \
      song_info, NCM_SCREEN_FLAG_NONE)
#define NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG_ENTRY(X) \
    X(NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG, \
      NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG, 13, sort_playlist_dialog, \
      NCM_SCREEN_FLAG_NONE)
#define NCM_SCREEN_TYPE_TAG_EDITOR_ENTRY(X) \
    X(NCM_SCREEN_TYPE_TAG_EDITOR, NC_SCREEN_TYPE_TAG_EDITOR, 14, \
      tag_editor, NCM_SCREEN_FLAG_STARTUP)
#define NCM_SCREEN_TYPE_TINY_TAG_EDITOR_ENTRY(X) \
    X(NCM_SCREEN_TYPE_TINY_TAG_EDITOR, NC_SCREEN_TYPE_TINY_TAG_EDITOR, \
      15, tiny_tag_editor, NCM_SCREEN_FLAG_NONE)
#define NCM_SCREEN_TYPE_VISUALIZER_ENTRY(X) \
    X(NCM_SCREEN_TYPE_VISUALIZER, NC_SCREEN_TYPE_VISUALIZER, 16, \
      visualizer, NCM_SCREEN_FLAG_STARTUP)

#define NCM_SCREEN_ALL_TYPES_BEFORE_UNKNOWN(X) \
    NCM_SCREEN_TYPE_BROWSER_ENTRY(X) \
    NCM_SCREEN_TYPE_HELP_ENTRY(X) \
    NCM_SCREEN_TYPE_LASTFM_ENTRY(X) \
    NCM_SCREEN_TYPE_LYRICS_ENTRY(X) \
    NCM_SCREEN_TYPE_MEDIA_LIBRARY_ENTRY(X) \
    NCM_SCREEN_TYPE_OUTPUTS_ENTRY(X) \
    NCM_SCREEN_TYPE_PLAYLIST_ENTRY(X) \
    NCM_SCREEN_TYPE_PLAYLIST_EDITOR_ENTRY(X) \
    NCM_SCREEN_TYPE_SEARCH_ENGINE_ENTRY(X) \
    NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER_ENTRY(X) \
    NCM_SCREEN_TYPE_SERVER_INFO_ENTRY(X) \
    NCM_SCREEN_TYPE_SONG_INFO_ENTRY(X) \
    NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG_ENTRY(X) \
    NCM_SCREEN_TYPE_TAG_EDITOR_ENTRY(X) \
    NCM_SCREEN_TYPE_TINY_TAG_EDITOR_ENTRY(X)

#define NCM_SCREEN_ALL_TYPES_AFTER_UNKNOWN(X) \
    NCM_SCREEN_TYPE_VISUALIZER_ENTRY(X)

#define NCM_SCREEN_ALL_TYPES(X) \
    NCM_SCREEN_ALL_TYPES_BEFORE_UNKNOWN(X) \
    NCM_SCREEN_ALL_TYPES_AFTER_UNKNOWN(X)

#if defined(ENABLE_OUTPUTS)
  #define NCM_SCREEN_ENABLED_OUTPUTS_TYPES(X) \
      NCM_SCREEN_TYPE_OUTPUTS_ENTRY(X)
#else
  #define NCM_SCREEN_ENABLED_OUTPUTS_TYPES(X)
#endif

#if defined(HAVE_TAGLIB_H)
  #define NCM_SCREEN_ENABLED_TAG_EDITOR_TYPES(X) \
      NCM_SCREEN_TYPE_TAG_EDITOR_ENTRY(X) \
      NCM_SCREEN_TYPE_TINY_TAG_EDITOR_ENTRY(X)
#else
  #define NCM_SCREEN_ENABLED_TAG_EDITOR_TYPES(X)
#endif

#if defined(ENABLE_VISUALIZER)
  #define NCM_SCREEN_ENABLED_VISUALIZER_TYPES(X) \
      NCM_SCREEN_TYPE_VISUALIZER_ENTRY(X)
#else
  #define NCM_SCREEN_ENABLED_VISUALIZER_TYPES(X)
#endif

#define NCM_SCREEN_TYPES_BEFORE_UNKNOWN(X) \
    NCM_SCREEN_TYPE_BROWSER_ENTRY(X) \
    NCM_SCREEN_TYPE_HELP_ENTRY(X) \
    NCM_SCREEN_TYPE_LASTFM_ENTRY(X) \
    NCM_SCREEN_TYPE_LYRICS_ENTRY(X) \
    NCM_SCREEN_TYPE_MEDIA_LIBRARY_ENTRY(X) \
    NCM_SCREEN_ENABLED_OUTPUTS_TYPES(X) \
    NCM_SCREEN_TYPE_PLAYLIST_ENTRY(X) \
    NCM_SCREEN_TYPE_PLAYLIST_EDITOR_ENTRY(X) \
    NCM_SCREEN_TYPE_SEARCH_ENGINE_ENTRY(X) \
    NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER_ENTRY(X) \
    NCM_SCREEN_TYPE_SERVER_INFO_ENTRY(X) \
    NCM_SCREEN_TYPE_SONG_INFO_ENTRY(X) \
    NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG_ENTRY(X) \
    NCM_SCREEN_ENABLED_TAG_EDITOR_TYPES(X)

#define NCM_SCREEN_TYPES_AFTER_UNKNOWN(X) \
    NCM_SCREEN_ENABLED_VISUALIZER_TYPES(X)

#define NCM_SCREEN_TYPES(X) \
    NCM_SCREEN_TYPES_BEFORE_UNKNOWN(X) \
    NCM_SCREEN_TYPES_AFTER_UNKNOWN(X)

#define NCM_SCREEN_NC_TYPE_ENUM_FIELD( \
    screen_type, nc_type, nc_value, alias, flags \
) \
    nc_type = nc_value,

#define NCM_SCREEN_TYPE_XENUM_FIELD( \
    screen_type, nc_type, nc_value, alias, flags \
) \
    X(screen_type, alias)

#define NCM_SCREEN_TYPE_ENUM_FIELDS \
    NCM_SCREEN_TYPES_BEFORE_UNKNOWN(NCM_SCREEN_TYPE_XENUM_FIELD) \
    X(NCM_SCREEN_TYPE_UNKNOWN, unknown) \
    NCM_SCREEN_TYPES_AFTER_UNKNOWN(NCM_SCREEN_TYPE_XENUM_FIELD)


#define NCM_APP_SCREEN_DIRECT_STORAGE_TYPES(X) \
    X(BrowserScreen, browser_screen) \
    X(LastfmScreen, lastfm_screen) \
    X(LyricsScreen, lyrics_screen) \
    X(VisualizerScreen, visualizer_screen) \
    X(PlaylistScreen, playlist_screen) \
    X(PlaylistEditorScreen, playlist_editor_screen) \
    X(SelectedItemsAdderScreen, selected_items_adder_screen) \
    X(SortPlaylistDialog, sort_playlist_dialog) \
    X(SearchEngineScreen, search_engine_screen) \
    X(MediaLibraryScreen, media_library_screen) \
    X(TagEditorScreen, tag_editor_screen) \
    X(TinyTagEditorScreen, tiny_tag_editor_screen)

#define NCM_APP_SCREEN_WRAPPED_STORAGE_TYPES(X) \
    X(HelpScreen, help_screen) \
    X(OutputsScreen, outputs_screen) \
    X(ServerInfoScreen, server_info_screen) \
    X(SongInfoScreen, song_info_screen)

#define NCM_APP_SCREEN_INIT_FLAGS(X) \
    X(browser_screen_initialized) \
    X(lastfm_screen_initialized) \
    X(lyrics_screen_initialized) \
    X(visualizer_screen_initialized) \
    X(playlist_editor_screen_initialized) \
    X(selected_items_adder_screen_initialized) \
    X(sort_playlist_dialog_initialized) \
    X(search_engine_screen_initialized) \
    X(media_library_screen_initialized) \
    X(tag_editor_screen_initialized) \
    X(tiny_tag_editor_screen_initialized) \
    X(playlist_screen_initialized)

#define NCM_APP_SCREEN_DIRECT_ACCESSOR_TYPES(X) \
    X(browser, BrowserScreen, browser_screen, \
      browser_screen_base(&browser_screen)) \
    X(lastfm, LastfmScreen, lastfm_screen, \
      lastfm_screen_base(&lastfm_screen)) \
    X(lyrics, LyricsScreen, lyrics_screen, \
      lyrics_screen_base(&lyrics_screen)) \
    X(playlist, PlaylistScreen, playlist_screen, \
      playlist_screen_base(&playlist_screen)) \
    X(playlist_editor, PlaylistEditorScreen, playlist_editor_screen, \
      playlist_editor_screen_base(&playlist_editor_screen)) \
    X(selected_items_adder, SelectedItemsAdderScreen, \
      selected_items_adder_screen, \
      selected_items_adder_screen_base(&selected_items_adder_screen)) \
    X(sort_playlist_dialog, SortPlaylistDialog, sort_playlist_dialog, \
      sort_playlist_dialog_base(&sort_playlist_dialog)) \
    X(search_engine, SearchEngineScreen, search_engine_screen, \
      search_engine_screen_base(&search_engine_screen)) \
    X(media_library, MediaLibraryScreen, media_library_screen, \
      media_library_screen_base(&media_library_screen)) \
    X(tag_editor, TagEditorScreen, tag_editor_screen, \
      tag_editor_screen_base(&tag_editor_screen)) \
    X(tiny_tag_editor, TinyTagEditorScreen, tiny_tag_editor_screen, \
      tiny_tag_editor_screen_base(&tiny_tag_editor_screen))

#define NCM_APP_SCREEN_WRAPPED_ACCESSOR_TYPES(X) \
    X(help, nc_help_screen_base(&help_screen.screen)) \
    X(server_info, nc_server_info_screen_base(&server_info_screen.screen)) \
    X(song_info, nc_song_info_screen_base(&song_info_screen.screen))

#define NCM_APP_SCREEN_TYPED_WRAPPED_ACCESSOR_TYPES(X) \
    X(help, app_screen_help, NcHelpScreen, &help_screen.screen)

#define NCM_APP_SCREEN_STANDARD_REGISTER_TYPES(X) \
    X(browser) \
    X(help) \
    X(lastfm) \
    X(lyrics) \
    X(visualizer) \
    X(playlist) \
    X(playlist_editor) \
    X(search_engine) \
    X(media_library) \
    X(tag_editor) \
    X(tiny_tag_editor) \
    X(song_info) \
    X(server_info) \
    X(outputs)

#define NCM_APP_SCREEN_REPLACE_REGISTER_TYPES(X) \
    X(selected_items_adder, NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER) \
    X(sort_playlist_dialog, NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG)

#define NCM_APP_SCREEN_SIMPLE_SWITCH_TYPES(X) \
    X(browser) \
    X(help) \
    X(playlist) \
    X(playlist_editor) \
    X(selected_items_adder) \
    X(search_engine) \
    X(media_library) \
    X(tag_editor) \
    X(song_info) \
    X(server_info) \
    X(outputs)

#define NCM_APP_SCREEN_REGISTER_SWITCH_TYPES(X) \
    X(tiny_tag_editor)

#define NCM_APP_SCREEN_IS_CURRENT_TYPES(X) \
    X(browser) \
    X(help) \
    X(lastfm) \
    X(lyrics) \
    X(visualizer) \
    X(playlist) \
    X(playlist_editor) \
    X(selected_items_adder) \
    X(sort_playlist_dialog) \
    X(search_engine) \
    X(media_library) \
    X(tag_editor) \
    X(tiny_tag_editor) \
    X(song_info) \
    X(server_info) \
    X(outputs)

#if defined(ENABLE_OUTPUTS)
  #define NCM_APP_SCREEN_ENABLED_OUTPUTS(X) X(outputs)
  #define NCM_APP_SCREEN_ENABLED_OUTPUTS_RESIZE(X) \
      X(outputs, NC_SCREEN_TYPE_OUTPUTS)
#else
  #define NCM_APP_SCREEN_ENABLED_OUTPUTS(X)
  #define NCM_APP_SCREEN_ENABLED_OUTPUTS_RESIZE(X)
#endif

#if defined(HAVE_TAGLIB_H)
  #define NCM_APP_SCREEN_ENABLED_TAG_EDITOR(X) \
      X(tag_editor) \
      X(tiny_tag_editor)
  #define NCM_APP_SCREEN_ENABLED_TAG_EDITOR_RESIZE(X) \
      X(tag_editor, NC_SCREEN_TYPE_TAG_EDITOR) \
      X(tiny_tag_editor, NC_SCREEN_TYPE_TINY_TAG_EDITOR)
#else
  #define NCM_APP_SCREEN_ENABLED_TAG_EDITOR(X)
  #define NCM_APP_SCREEN_ENABLED_TAG_EDITOR_RESIZE(X)
#endif

#if defined(ENABLE_VISUALIZER)
  #define NCM_APP_SCREEN_ENABLED_VISUALIZER(X) X(visualizer)
  #define NCM_APP_SCREEN_ENABLED_VISUALIZER_RESIZE(X) \
      X(visualizer, NC_SCREEN_TYPE_VISUALIZER)
#else
  #define NCM_APP_SCREEN_ENABLED_VISUALIZER(X)
  #define NCM_APP_SCREEN_ENABLED_VISUALIZER_RESIZE(X)
#endif

#define NCM_APP_SCREEN_INIT_ALL_TYPES(X) \
    X(browser) \
    X(help) \
    X(lastfm) \
    X(lyrics) \
    X(media_library) \
    X(playlist) \
    X(playlist_editor) \
    X(search_engine) \
    X(selected_items_adder) \
    X(server_info) \
    X(song_info) \
    X(sort_playlist_dialog) \
    NCM_APP_SCREEN_ENABLED_TAG_EDITOR(X) \
    NCM_APP_SCREEN_ENABLED_VISUALIZER(X) \
    NCM_APP_SCREEN_ENABLED_OUTPUTS(X)

#define NCM_APP_SCREEN_REGISTER_INITIAL_TYPES(X) \
    X(browser) \
    X(help) \
    X(lastfm) \
    X(media_library) \
    X(search_engine) \
    X(selected_items_adder) \
    X(song_info) \
    X(server_info) \
    NCM_APP_SCREEN_ENABLED_VISUALIZER(X) \
    NCM_APP_SCREEN_ENABLED_TAG_EDITOR(X) \
    NCM_APP_SCREEN_ENABLED_OUTPUTS(X) \
    X(playlist) \
    X(playlist_editor)

#define NCM_APP_SCREEN_RESIZE_REQUEST_TYPES(X) \
    X(browser, NC_SCREEN_TYPE_BROWSER) \
    X(help, NC_SCREEN_TYPE_HELP) \
    X(lastfm, NC_SCREEN_TYPE_LASTFM) \
    X(lyrics, NC_SCREEN_TYPE_LYRICS) \
    X(media_library, NC_SCREEN_TYPE_MEDIA_LIBRARY) \
    X(playlist, NC_SCREEN_TYPE_PLAYLIST) \
    X(playlist_editor, NC_SCREEN_TYPE_PLAYLIST_EDITOR) \
    X(search_engine, NC_SCREEN_TYPE_SEARCH_ENGINE) \
    X(selected_items_adder, NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER) \
    X(server_info, NC_SCREEN_TYPE_SERVER_INFO) \
    X(song_info, NC_SCREEN_TYPE_SONG_INFO) \
    X(sort_playlist_dialog, NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG) \
    NCM_APP_SCREEN_ENABLED_TAG_EDITOR_RESIZE(X) \
    NCM_APP_SCREEN_ENABLED_VISUALIZER_RESIZE(X) \
    NCM_APP_SCREEN_ENABLED_OUTPUTS_RESIZE(X)

#endif /* NCMPCPP_SCREEN_DEFS_H */
