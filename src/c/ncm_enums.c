#include "c/ncm_enums.h"

#include "c/ncm_string.h"

static int32 ncm_enum_cstr_len(char *string);
static bool ncm_enum_equal(char *string, int32 string_len, char *expected);

static int32
ncm_enum_cstr_len(char *string) {
    int32 result;

    result = 0;
    while (string[result] != '\0') {
        result += 1;
    }

    return result;
}

static bool
ncm_enum_equal(char *string, int32 string_len, char *expected) {
    return ncm_string_equal(string, string_len, expected,
                            ncm_enum_cstr_len(expected));
}

char *
ncm_search_direction_str(enum SearchDirection value) {
    switch (value) {
    case NCM_SEARCH_DIRECTION_BACKWARD:
        return (char *)"backward";
    case NCM_SEARCH_DIRECTION_FORWARD:
        return (char *)"forward";
    case NCM_SEARCH_DIRECTION_LAST:
        break;
    }

    return (char *)"unknown";
}

bool
ncm_search_direction_parse(char *string, int32 string_len,
                           enum SearchDirection *value) {
    if (ncm_enum_equal(string, string_len, (char *)"backward")) {
        *value = NCM_SEARCH_DIRECTION_BACKWARD;
        return true;
    }
    if (ncm_enum_equal(string, string_len, (char *)"forward")) {
        *value = NCM_SEARCH_DIRECTION_FORWARD;
        return true;
    }

    return false;
}

char *
ncm_space_add_mode_str(enum SpaceAddMode value) {
    switch (value) {
    case NCM_SPACE_ADD_MODE_ADD_REMOVE:
        return (char *)"add_remove";
    case NCM_SPACE_ADD_MODE_ALWAYS_ADD:
        return (char *)"always_add";
    case NCM_SPACE_ADD_MODE_LAST:
        break;
    }

    return (char *)"unknown";
}

bool
ncm_space_add_mode_parse(char *string, int32 string_len,
                         enum SpaceAddMode *value) {
    if (ncm_enum_equal(string, string_len, (char *)"add_remove")) {
        *value = NCM_SPACE_ADD_MODE_ADD_REMOVE;
        return true;
    }
    if (ncm_enum_equal(string, string_len, (char *)"always_add")) {
        *value = NCM_SPACE_ADD_MODE_ALWAYS_ADD;
        return true;
    }

    return false;
}

char *
ncm_sort_mode_str(enum SortMode value) {
    switch (value) {
    case NCM_SORT_MODE_TYPE:
        return (char *)"type";
    case NCM_SORT_MODE_NAME:
        return (char *)"name";
    case NCM_SORT_MODE_MODIFICATION_TIME:
        return (char *)"mtime";
    case NCM_SORT_MODE_CUSTOM_FORMAT:
        return (char *)"format";
    case NCM_SORT_MODE_NONE:
        return (char *)"none";
    case NCM_SORT_MODE_LAST:
        break;
    }

    return (char *)"unknown";
}

bool
ncm_sort_mode_parse(char *string, int32 string_len,
                    enum SortMode *value) {
    if (ncm_enum_equal(string, string_len, (char *)"type")) {
        *value = NCM_SORT_MODE_TYPE;
        return true;
    }
    if (ncm_enum_equal(string, string_len, (char *)"name")) {
        *value = NCM_SORT_MODE_NAME;
        return true;
    }
    if (ncm_enum_equal(string, string_len, (char *)"mtime")) {
        *value = NCM_SORT_MODE_MODIFICATION_TIME;
        return true;
    }
    if (ncm_enum_equal(string, string_len, (char *)"format")) {
        *value = NCM_SORT_MODE_CUSTOM_FORMAT;
        return true;
    }
    if (ncm_enum_equal(string, string_len, (char *)"none")) {
        *value = NCM_SORT_MODE_NONE;
        return true;
    }

    return false;
}

char *
ncm_display_mode_str(enum DisplayMode value) {
    switch (value) {
    case NCM_DISPLAY_MODE_CLASSIC:
        return (char *)"classic";
    case NCM_DISPLAY_MODE_COLUMNS:
        return (char *)"columns";
    case NCM_DISPLAY_MODE_LAST:
        break;
    }

    return (char *)"unknown";
}

bool
ncm_display_mode_parse(char *string, int32 string_len,
                       enum DisplayMode *value) {
    if (ncm_enum_equal(string, string_len, (char *)"classic")) {
        *value = NCM_DISPLAY_MODE_CLASSIC;
        return true;
    }
    if (ncm_enum_equal(string, string_len, (char *)"columns")) {
        *value = NCM_DISPLAY_MODE_COLUMNS;
        return true;
    }

    return false;
}

char *
ncm_design_str(enum Design value) {
    switch (value) {
    case NCM_DESIGN_CLASSIC:
        return (char *)"classic";
    case NCM_DESIGN_ALTERNATIVE:
        return (char *)"alternative";
    case NCM_DESIGN_LAST:
        break;
    }

    return (char *)"unknown";
}

bool
ncm_design_parse(char *string, int32 string_len, enum Design *value) {
    if (ncm_enum_equal(string, string_len, (char *)"classic")) {
        *value = NCM_DESIGN_CLASSIC;
        return true;
    }
    if (ncm_enum_equal(string, string_len, (char *)"alternative")) {
        *value = NCM_DESIGN_ALTERNATIVE;
        return true;
    }

    return false;
}

char *
ncm_visualizer_type_str(enum VisualizerType value) {
    switch (value) {
    case NCM_VISUALIZER_TYPE_WAVE:
        return (char *)"sound wave";
    case NCM_VISUALIZER_TYPE_WAVE_FILLED:
        return (char *)"sound wave filled";
#if defined(HAVE_FFTW3_H)
    case NCM_VISUALIZER_TYPE_SPECTRUM:
        return (char *)"frequency spectrum";
#endif
    case NCM_VISUALIZER_TYPE_ELLIPSE:
        return (char *)"sound ellipse";
    case NCM_VISUALIZER_TYPE_LAST:
        break;
    }

    return (char *)"unknown";
}

bool
ncm_visualizer_type_parse(char *string, int32 string_len,
                          enum VisualizerType *value) {
    if (ncm_enum_equal(string, string_len, (char *)"wave")) {
        *value = NCM_VISUALIZER_TYPE_WAVE;
        return true;
    }
    if (ncm_enum_equal(string, string_len, (char *)"wave_filled")) {
        *value = NCM_VISUALIZER_TYPE_WAVE_FILLED;
        return true;
    }
#if defined(HAVE_FFTW3_H)
    if (ncm_enum_equal(string, string_len, (char *)"spectrum")) {
        *value = NCM_VISUALIZER_TYPE_SPECTRUM;
        return true;
    }
#endif
    if (ncm_enum_equal(string, string_len, (char *)"ellipse")) {
        *value = NCM_VISUALIZER_TYPE_ELLIPSE;
        return true;
    }

    return false;
}
