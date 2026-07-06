#include "screens/screen_type.h"

#include "c/ncm_string.h"

static int32 screen_type_cstr_len(char *string);
static bool screen_type_string_equal(char *string, int32 string_len,
                                     char *expected);

static int32
screen_type_cstr_len(char *string) {
    int32 result;

    result = 0;
    while (string[result] != '\0') {
        result += 1;
    }

    return result;
}

static bool
screen_type_string_equal(char *string, int32 string_len, char *expected) {
    return ncm_string_equal(string, string_len, expected,
                            screen_type_cstr_len(expected));
}

char *
screen_type_str(enum ScreenType screen_type) {
    switch (screen_type) {
    case NCM_SCREEN_TYPE_BROWSER:
        return (char *)"browser";
    case NCM_SCREEN_TYPE_HELP:
        return (char *)"help";
    case NCM_SCREEN_TYPE_LASTFM:
        return (char *)"last_fm";
    case NCM_SCREEN_TYPE_LYRICS:
        return (char *)"lyrics";
    case NCM_SCREEN_TYPE_MEDIA_LIBRARY:
        return (char *)"media_library";
#if defined(ENABLE_OUTPUTS)
    case NCM_SCREEN_TYPE_OUTPUTS:
        return (char *)"outputs";
#endif
    case NCM_SCREEN_TYPE_PLAYLIST:
        return (char *)"playlist";
    case NCM_SCREEN_TYPE_PLAYLIST_EDITOR:
        return (char *)"playlist_editor";
    case NCM_SCREEN_TYPE_SEARCH_ENGINE:
        return (char *)"search_engine";
    case NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
        return (char *)"selected_items_adder";
    case NCM_SCREEN_TYPE_SERVER_INFO:
        return (char *)"server_info";
    case NCM_SCREEN_TYPE_SONG_INFO:
        return (char *)"song_info";
    case NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
        return (char *)"sort_playlist_dialog";
#if defined(HAVE_TAGLIB_H)
    case NCM_SCREEN_TYPE_TAG_EDITOR:
        return (char *)"tag_editor";
    case NCM_SCREEN_TYPE_TINY_TAG_EDITOR:
        return (char *)"tiny_tag_editor";
#endif
    case NCM_SCREEN_TYPE_UNKNOWN:
        return (char *)"unknown";
#if defined(ENABLE_VISUALIZER)
    case NCM_SCREEN_TYPE_VISUALIZER:
        return (char *)"visualizer";
#endif
    case NCM_SCREEN_TYPE_LAST:
        break;
    }

    return (char *)"unknown";
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
    if (screen_type_string_equal(string, string_len, (char *)"browser")) {
        *screen_type = NCM_SCREEN_TYPE_BROWSER;
        return true;
    }
    if (screen_type_string_equal(string, string_len, (char *)"help")) {
        *screen_type = NCM_SCREEN_TYPE_HELP;
        return true;
    }
    if (screen_type_string_equal(string, string_len,
                                 (char *)"media_library")) {
        *screen_type = NCM_SCREEN_TYPE_MEDIA_LIBRARY;
        return true;
    }
#if defined(ENABLE_OUTPUTS)
    if (screen_type_string_equal(string, string_len, (char *)"outputs")) {
        *screen_type = NCM_SCREEN_TYPE_OUTPUTS;
        return true;
    }
#endif
    if (screen_type_string_equal(string, string_len, (char *)"playlist")) {
        *screen_type = NCM_SCREEN_TYPE_PLAYLIST;
        return true;
    }
    if (screen_type_string_equal(string, string_len,
                                 (char *)"playlist_editor")) {
        *screen_type = NCM_SCREEN_TYPE_PLAYLIST_EDITOR;
        return true;
    }
    if (screen_type_string_equal(string, string_len,
                                 (char *)"search_engine")) {
        *screen_type = NCM_SCREEN_TYPE_SEARCH_ENGINE;
        return true;
    }
#if defined(HAVE_TAGLIB_H)
    if (screen_type_string_equal(string, string_len,
                                 (char *)"tag_editor")) {
        *screen_type = NCM_SCREEN_TYPE_TAG_EDITOR;
        return true;
    }
#endif
#if defined(ENABLE_VISUALIZER)
    if (screen_type_string_equal(string, string_len,
                                 (char *)"visualizer")) {
        *screen_type = NCM_SCREEN_TYPE_VISUALIZER;
        return true;
    }
#endif
    if (screen_type_string_equal(string, string_len, (char *)"lyrics")) {
        *screen_type = NCM_SCREEN_TYPE_LYRICS;
        return true;
    }
    if (screen_type_string_equal(string, string_len, (char *)"last_fm")) {
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
    if (screen_type_string_equal(string, string_len,
                                 (char *)"selected_items_adder")) {
        *screen_type = NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER;
        return true;
    }
    if (screen_type_string_equal(string, string_len,
                                 (char *)"server_info")) {
        *screen_type = NCM_SCREEN_TYPE_SERVER_INFO;
        return true;
    }
    if (screen_type_string_equal(string, string_len, (char *)"song_info")) {
        *screen_type = NCM_SCREEN_TYPE_SONG_INFO;
        return true;
    }
    if (screen_type_string_equal(string, string_len,
                                 (char *)"sort_playlist_dialog")) {
        *screen_type = NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG;
        return true;
    }
#if defined(HAVE_TAGLIB_H)
    if (screen_type_string_equal(string, string_len,
                                 (char *)"tiny_tag_editor")) {
        *screen_type = NCM_SCREEN_TYPE_TINY_TAG_EDITOR;
        return true;
    }
#endif

    *screen_type = NCM_SCREEN_TYPE_UNKNOWN;
    return false;
}
