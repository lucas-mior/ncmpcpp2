#include "screens/screen_type.h"

#include "c/ncm_string.h"
#include "cbase/util.c"

char *
screen_type_str(enum ScreenType screen_type) {
    switch (screen_type) {
    case NCM_SCREEN_TYPE_BROWSER:
        return "browser";
    case NCM_SCREEN_TYPE_HELP:
        return "help";
    case NCM_SCREEN_TYPE_LASTFM:
        return "last_fm";
    case NCM_SCREEN_TYPE_LYRICS:
        return "lyrics";
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        return "media_library";
#if defined(ENABLE_OUTPUTS)
    case NCM_SCREEN_TYPE_OUTPUTS:
        return "outputs";
#endif
    case NCM_SCREEN_TYPE_PLAYLIST:
        return "playlist";
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        return "playlist_editor";
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
        return "search_engine";
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
        return "selected_items_adder";
    case NCM_SCREEN_TYPE_SERVER_INFO:
        return "server_info";
    case NCM_SCREEN_TYPE_SONG_INFO:
        return "song_info";
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
        return "sort_playlist_dialog";
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        return "tag_editor";
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
        return "tiny_tag_editor";
#endif
    case NCM_SCREEN_TYPE_UNKNOWN:
        return "unknown";
#if defined(ENABLE_VISUALIZER)
    case NCM_SCREEN_TYPE_VISUALIZER:
        return "visualizer";
#endif
    case NCM_SCREEN_TYPE_LAST:
        break;
    }

    return "unknown";
}

int32
screen_type_to_native_type(enum ScreenType screen_type) {
    switch (screen_type) {
    case NCM_SCREEN_TYPE_BROWSER:
        return NC_SCREEN_TYPE_BROWSER;
    case NCM_SCREEN_TYPE_HELP:
        return NC_SCREEN_TYPE_HELP;
    case NCM_SCREEN_TYPE_LASTFM:
        return NC_SCREEN_TYPE_LASTFM;
    case NCM_SCREEN_TYPE_LYRICS:
        return NC_SCREEN_TYPE_LYRICS;
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        return NC_SCREEN_TYPE_MEDIA_LIBRARY;
#if defined(ENABLE_OUTPUTS)
    case NCM_SCREEN_TYPE_OUTPUTS:
        return NC_SCREEN_TYPE_OUTPUTS;
#endif
    case NCM_SCREEN_TYPE_PLAYLIST:
        return NC_SCREEN_TYPE_PLAYLIST;
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        return NC_SCREEN_TYPE_PLAYLIST_EDITOR;
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
        return NC_SCREEN_TYPE_SEARCH_ENGINE;
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
        return NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER;
    case NCM_SCREEN_TYPE_SERVER_INFO:
        return NC_SCREEN_TYPE_SERVER_INFO;
    case NCM_SCREEN_TYPE_SONG_INFO:
        return NC_SCREEN_TYPE_SONG_INFO;
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
        return NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG;
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        return NC_SCREEN_TYPE_TAG_EDITOR;
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
        return NC_SCREEN_TYPE_TINY_TAG_EDITOR;
#endif
#if defined(ENABLE_VISUALIZER)
    case NCM_SCREEN_TYPE_VISUALIZER:
        return NC_SCREEN_TYPE_VISUALIZER;
#endif
    case NCM_SCREEN_TYPE_UNKNOWN:
        return NC_SCREEN_TYPE_UNKNOWN;
    case NCM_SCREEN_TYPE_LAST:
        break;
    }

    return NC_SCREEN_TYPE_UNKNOWN;
}

bool
screen_type_parse_startup(char *string, int32 string_len,
                          enum ScreenType *screen_type) {
    if (STREQUAL(string, string_len, "browser")) {
        *screen_type = NCM_SCREEN_TYPE_BROWSER;
        return true;
    }
    if (STREQUAL(string, string_len, "help")) {
        *screen_type = NCM_SCREEN_TYPE_HELP;
        return true;
    }
    if (STREQUAL(string, string_len,
                                 "media_library")) {
        *screen_type = NCM_SCREEN_TYPE_MEDIA_LIBRARY;
        return true;
    }
#if defined(ENABLE_OUTPUTS)
    if (STREQUAL(string, string_len, "outputs")) {
        *screen_type = NCM_SCREEN_TYPE_OUTPUTS;
        return true;
    }
#endif
    if (STREQUAL(string, string_len, "playlist")) {
        *screen_type = NCM_SCREEN_TYPE_PLAYLIST;
        return true;
    }
    if (STREQUAL(string, string_len,
                                 "playlist_editor")) {
        *screen_type = NCM_SCREEN_TYPE_PLAYLIST_EDITOR;
        return true;
    }
    if (STREQUAL(string, string_len,
                                 "search_engine")) {
        *screen_type = NCM_SCREEN_TYPE_SEARCH_ENGINE;
        return true;
    }
#if defined(HAVE_TAGLIB_H)
    if (STREQUAL(string, string_len,
                                 "tag_editor")) {
        *screen_type = NCM_SCREEN_TYPE_TAG_EDITOR;
        return true;
    }
#endif
#if defined(ENABLE_VISUALIZER)
    if (STREQUAL(string, string_len,
                                 "visualizer")) {
        *screen_type = NCM_SCREEN_TYPE_VISUALIZER;
        return true;
    }
#endif
    if (STREQUAL(string, string_len, "lyrics")) {
        *screen_type = NCM_SCREEN_TYPE_LYRICS;
        return true;
    }
    if (STREQUAL(string, string_len, "last_fm")) {
        *screen_type = NCM_SCREEN_TYPE_LASTFM;
        return true;
    }

    *screen_type = NCM_SCREEN_TYPE_UNKNOWN;
    return false;
}

bool
screen_type_parse(char *string, int32 string_len,
                  enum ScreenType *screen_type) {
    if (screen_type_parse_startup(string, string_len, screen_type)) {
        return true;
    }
    if (STREQUAL(string, string_len,
                                 "selected_items_adder")) {
        *screen_type = NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER;
        return true;
    }
    if (STREQUAL(string, string_len,
                                 "server_info")) {
        *screen_type = NCM_SCREEN_TYPE_SERVER_INFO;
        return true;
    }
    if (STREQUAL(string, string_len, "song_info")) {
        *screen_type = NCM_SCREEN_TYPE_SONG_INFO;
        return true;
    }
    if (STREQUAL(string, string_len,
                                 "sort_playlist_dialog")) {
        *screen_type = NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG;
        return true;
    }
#if defined(HAVE_TAGLIB_H)
    if (STREQUAL(string, string_len,
                                 "tiny_tag_editor")) {
        *screen_type = NCM_SCREEN_TYPE_TINY_TAG_EDITOR;
        return true;
    }
#endif

    *screen_type = NCM_SCREEN_TYPE_UNKNOWN;
    return false;
}
