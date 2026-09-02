#if !defined(NCM_ENUMS_C)
#define NCM_ENUMS_C

#include "cbase.h"

#include "c/ncm_c.h"

char *
ncm_search_direction_str(enum SearchDirection value) {
    switch (value) {
    case NCM_SEARCH_DIRECTION_BACKWARD:
        return "backward";
    case NCM_SEARCH_DIRECTION_FORWARD:
        return "forward";
    case NCM_SEARCH_DIRECTION_COUNT:
        break;
    default:
        break;
    }

    return "unknown";
}

static int32
ncm_enum_parse_check_input(char *string, int32 string_len, void *value) {
    if (string == NULL) {
        return -EINVAL;
    }
    if (string_len < 0) {
        return -EINVAL;
    }
    if (value == NULL) {
        return -EINVAL;
    }

    return 0;
}

int32
ncm_space_add_mode_parse(char *string, int32 string_len,
                         enum SpaceAddMode *value) {
    int32 status;

    if ((status = ncm_enum_parse_check_input(string, string_len,
                                             value)) < 0) {
        return status;
    }

    if (STREQUAL(string, string_len, "add_remove")) {
        *value = NCM_SPACE_ADD_MODE_ADD_REMOVE;
        return 0;
    }
    if (STREQUAL(string, string_len, "always_add")) {
        *value = NCM_SPACE_ADD_MODE_ALWAYS_ADD;
        return 0;
    }

    return -NCM_ERROR_PARSE;
}

int32
ncm_sort_mode_parse(char *string, int32 string_len,
                    enum SortMode *value) {
    int32 status;

    if ((status = ncm_enum_parse_check_input(string, string_len,
                                             value)) < 0) {
        return status;
    }

    if (STREQUAL(string, string_len, "type")) {
        *value = NCM_SORT_MODE_TYPE;
        return 0;
    }
    if (STREQUAL(string, string_len, "name")) {
        *value = NCM_SORT_MODE_NAME;
        return 0;
    }
    if (STREQUAL(string, string_len, "mtime")) {
        *value = NCM_SORT_MODE_MODIFICATION_TIME;
        return 0;
    }
    if (STREQUAL(string, string_len, "format")) {
        *value = NCM_SORT_MODE_CUSTOM_FORMAT;
        return 0;
    }
    if (STREQUAL(string, string_len, "none")) {
        *value = NCM_SORT_MODE_NONE;
        return 0;
    }

    return -NCM_ERROR_PARSE;
}

char *
ncm_display_mode_str(enum DisplayMode value) {
    switch (value) {
    case NCM_DISPLAY_MODE_CLASSIC:
        return "classic";
    case NCM_DISPLAY_MODE_COLUMNS:
        return "columns";
    case NCM_DISPLAY_MODE_COUNT:
        break;
    default:
        break;
    }

    return "unknown";
}

int32
ncm_display_mode_parse(char *string, int32 string_len,
                       enum DisplayMode *value) {
    int32 status;

    if ((status = ncm_enum_parse_check_input(string, string_len,
                                             value)) < 0) {
        return status;
    }

    if (STREQUAL(string, string_len, "classic")) {
        *value = NCM_DISPLAY_MODE_CLASSIC;
        return 0;
    }
    if (STREQUAL(string, string_len, "columns")) {
        *value = NCM_DISPLAY_MODE_COLUMNS;
        return 0;
    }

    return -NCM_ERROR_PARSE;
}

char *
ncm_design_str(enum Design value) {
    switch (value) {
    case NCM_DESIGN_CLASSIC:
        return "classic";
    case NCM_DESIGN_ALTERNATIVE:
        return "alternative";
    case NCM_DESIGN_COUNT:
        break;
    default:
        break;
    }

    return "unknown";
}

int32
ncm_design_parse(char *string, int32 string_len, enum Design *value) {
    int32 status;

    if ((status = ncm_enum_parse_check_input(string, string_len,
                                             value)) < 0) {
        return status;
    }

    if (STREQUAL(string, string_len, "classic")) {
        *value = NCM_DESIGN_CLASSIC;
        return 0;
    }
    if (STREQUAL(string, string_len, "alternative")) {
        *value = NCM_DESIGN_ALTERNATIVE;
        return 0;
    }

    return -NCM_ERROR_PARSE;
}

int32
ncm_visualizer_type_parse(char *string, int32 string_len,
                          enum VisualizerType *value) {
    int32 status;

    if ((status = ncm_enum_parse_check_input(string, string_len,
                                             value)) < 0) {
        return status;
    }

    if (STREQUAL(string, string_len, "wave")) {
        *value = NCM_VISUALIZER_TYPE_WAVE;
        return 0;
    }
    if (STREQUAL(string, string_len, "wave_filled")) {
        *value = NCM_VISUALIZER_TYPE_WAVE_FILLED;
        return 0;
    }
#if defined(HAVE_FFTW3_H)
    if (STREQUAL(string, string_len, "spectrum")) {
        *value = NCM_VISUALIZER_TYPE_SPECTRUM;
        return 0;
    }
#endif
    if (STREQUAL(string, string_len, "ellipse")) {
        *value = NCM_VISUALIZER_TYPE_ELLIPSE;
        return 0;
    }

    return -NCM_ERROR_PARSE;
}

#endif /* NCM_ENUMS_C */
