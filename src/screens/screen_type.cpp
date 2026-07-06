#include "config.h"
#include "screens/screen_type.h"

#include "screens/browser.h"
#include "screens/lastfm.h"
#include "screens/lyrics.h"
#include "screens/media_library.h"
#include "screens/native_c_screens.h"
#include "screens/playlist.h"
#include "screens/playlist_editor.h"
#include "screens/search_engine.h"
#include "screens/sel_items_adder.h"
#include "screens/sort_playlist.h"
#include "screens/tag_editor.h"
#include "screens/tiny_tag_editor.h"
#include "screens/visualizer.h"

std::string screenTypeToString(ScreenType st)
{
    return screen_type_str(st);
}

ScreenType stringtoStartupScreenType(const std::string &s)
{
    ScreenType result;

    if (!screen_type_parse_startup(const_cast<char *>(s.data()),
                                   static_cast<int32>(s.length()),
                                   &result))
        return NCM_SCREEN_TYPE_UNKNOWN;

    return result;
}

ScreenType stringToScreenType(const std::string &s)
{
    ScreenType result;

    if (!screen_type_parse(const_cast<char *>(s.data()),
                           static_cast<int32>(s.length()), &result))
        return NCM_SCREEN_TYPE_UNKNOWN;

    return result;
}

BaseScreen *toScreen(ScreenType st)
{
    switch (st)
    {
        case NCM_SCREEN_TYPE_BROWSER:
            return myBrowser;
        case NCM_SCREEN_TYPE_HELP:
            return native_c_screen_help_legacy();
        case NCM_SCREEN_TYPE_LASTFM:
            return myLastfm;
        case NCM_SCREEN_TYPE_LYRICS:
            return myLyrics;
        case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
            return myLibrary;
#       ifdef ENABLE_OUTPUTS
        case NCM_SCREEN_TYPE_OUTPUTS:
            return native_c_screen_outputs_legacy();
#       endif // ENABLE_OUTPUTS
        case NCM_SCREEN_TYPE_PLAYLIST:
            return myPlaylist;
        case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
            return myPlaylistEditor;
        case NCM_SCREEN_TYPE_SEARCH_ENGINE:
            return mySearcher;
        case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
            return mySelectedItemsAdder;
        case NCM_SCREEN_TYPE_SERVER_INFO:
            return native_c_screen_server_info_legacy();
        case NCM_SCREEN_TYPE_SONG_INFO:
            return native_c_screen_song_info_legacy();
        case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
            return mySortPlaylistDialog;
#       ifdef HAVE_TAGLIB_H
        case NCM_SCREEN_TYPE_TAG_EDITOR:
            return myTagEditor;
        case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
            return myTinyTagEditor;
#       endif // HAVE_TAGLIB_H
#       ifdef ENABLE_VISUALIZER
        case NCM_SCREEN_TYPE_VISUALIZER:
            return myVisualizer;
#       endif // ENABLE_VISUALIZER
        default:
            return nullptr;
    }
}

NcScreen *toNativeScreen(ScreenType st)
{
    switch (st)
    {
        case NCM_SCREEN_TYPE_HELP:
            return native_c_screen_help_native();
#       ifdef ENABLE_OUTPUTS
        case NCM_SCREEN_TYPE_OUTPUTS:
            return native_c_screen_outputs_native();
#       endif // ENABLE_OUTPUTS
        case NCM_SCREEN_TYPE_SERVER_INFO:
            return native_c_screen_server_info_native();
        case NCM_SCREEN_TYPE_SONG_INFO:
            return native_c_screen_song_info_native();
        default:
            break;
    }

    BaseScreen *screen = toScreen(st);

    if (screen == nullptr)
        return nullptr;
    return screen->nativeScreen();
}
