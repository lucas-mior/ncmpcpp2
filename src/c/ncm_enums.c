#include "c/ncm_enums.h"

#include "c/ncm_string.h"
#include "cbase/util.c"

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
    return STREQUAL(string, string_len, expected,
                            ncm_enum_cstr_len(expected));
}

char *
ncm_search_direction_str(enum SearchDirection value) {
    switch (value) {
    case NCM_SEARCH_DIRECTION_BACKWARD:
        return "backward";
    case NCM_SEARCH_DIRECTION_FORWARD:
        return "forward";
    case NCM_SEARCH_DIRECTION_LAST:
        break;
    }

    return "unknown";
}

bool
ncm_space_add_mode_parse(char *string, int32 string_len,
                         enum SpaceAddMode *value) {
    if (ncm_enum_equal(string, string_len, "add_remove")) {
        *value = NCM_SPACE_ADD_MODE_ADD_REMOVE;
        return true;
    }
    if (ncm_enum_equal(string, string_len, "always_add")) {
        *value = NCM_SPACE_ADD_MODE_ALWAYS_ADD;
        return true;
    }

    return false;
}

bool
ncm_sort_mode_parse(char *string, int32 string_len,
                    enum SortMode *value) {
    if (ncm_enum_equal(string, string_len, "type")) {
        *value = NCM_SORT_MODE_TYPE;
        return true;
    }
    if (ncm_enum_equal(string, string_len, "name")) {
        *value = NCM_SORT_MODE_NAME;
        return true;
    }
    if (ncm_enum_equal(string, string_len, "mtime")) {
        *value = NCM_SORT_MODE_MODIFICATION_TIME;
        return true;
    }
    if (ncm_enum_equal(string, string_len, "format")) {
        *value = NCM_SORT_MODE_CUSTOM_FORMAT;
        return true;
    }
    if (ncm_enum_equal(string, string_len, "none")) {
        *value = NCM_SORT_MODE_NONE;
        return true;
    }

    return false;
}

char *
ncm_display_mode_str(enum DisplayMode value) {
    switch (value) {
    case NCM_DISPLAY_MODE_CLASSIC:
        return "classic";
    case NCM_DISPLAY_MODE_COLUMNS:
        return "columns";
    case NCM_DISPLAY_MODE_LAST:
        break;
    }

    return "unknown";
}

bool
ncm_display_mode_parse(char *string, int32 string_len,
                       enum DisplayMode *value) {
    if (ncm_enum_equal(string, string_len, "classic")) {
        *value = NCM_DISPLAY_MODE_CLASSIC;
        return true;
    }
    if (ncm_enum_equal(string, string_len, "columns")) {
        *value = NCM_DISPLAY_MODE_COLUMNS;
        return true;
    }

    return false;
}

char *
ncm_design_str(enum Design value) {
    switch (value) {
    case NCM_DESIGN_CLASSIC:
        return "classic";
    case NCM_DESIGN_ALTERNATIVE:
        return "alternative";
    case NCM_DESIGN_LAST:
        break;
    }

    return "unknown";
}

bool
ncm_design_parse(char *string, int32 string_len, enum Design *value) {
    if (ncm_enum_equal(string, string_len, "classic")) {
        *value = NCM_DESIGN_CLASSIC;
        return true;
    }
    if (ncm_enum_equal(string, string_len, "alternative")) {
        *value = NCM_DESIGN_ALTERNATIVE;
        return true;
    }

    return false;
}

bool
ncm_visualizer_type_parse(char *string, int32 string_len,
                          enum VisualizerType *value) {
    if (ncm_enum_equal(string, string_len, "wave")) {
        *value = NCM_VISUALIZER_TYPE_WAVE;
        return true;
    }
    if (ncm_enum_equal(string, string_len, "wave_filled")) {
        *value = NCM_VISUALIZER_TYPE_WAVE_FILLED;
        return true;
    }
#if defined(HAVE_FFTW3_H)
    if (ncm_enum_equal(string, string_len, "spectrum")) {
        *value = NCM_VISUALIZER_TYPE_SPECTRUM;
        return true;
    }
#endif
    if (ncm_enum_equal(string, string_len, "ellipse")) {
        *value = NCM_VISUALIZER_TYPE_ELLIPSE;
        return true;
    }

    return false;
}
