#if !defined(NCM_ENUMS_H)
#define NCM_ENUMS_H

#include "config.h"
#include "c/ncm_defs.h"

enum SearchDirection {
    NCM_SEARCH_DIRECTION_BACKWARD,
    NCM_SEARCH_DIRECTION_FORWARD,
    NCM_SEARCH_DIRECTION_LAST,
};

enum SpaceAddMode {
    NCM_SPACE_ADD_MODE_ADD_REMOVE,
    NCM_SPACE_ADD_MODE_ALWAYS_ADD,
    NCM_SPACE_ADD_MODE_LAST,
};

enum SortMode {
    NCM_SORT_MODE_TYPE,
    NCM_SORT_MODE_NAME,
    NCM_SORT_MODE_MODIFICATION_TIME,
    NCM_SORT_MODE_CUSTOM_FORMAT,
    NCM_SORT_MODE_NONE,
    NCM_SORT_MODE_LAST,
};

enum DisplayMode {
    NCM_DISPLAY_MODE_CLASSIC,
    NCM_DISPLAY_MODE_COLUMNS,
    NCM_DISPLAY_MODE_LAST,
};

enum Design {
    NCM_DESIGN_CLASSIC,
    NCM_DESIGN_ALTERNATIVE,
    NCM_DESIGN_LAST,
};

enum VisualizerType {
    NCM_VISUALIZER_TYPE_WAVE,
    NCM_VISUALIZER_TYPE_WAVE_FILLED,
#if defined(HAVE_FFTW3_H)
    NCM_VISUALIZER_TYPE_SPECTRUM,
#endif
    NCM_VISUALIZER_TYPE_ELLIPSE,
    NCM_VISUALIZER_TYPE_LAST,
};

char *ncm_search_direction_str(enum SearchDirection value);
bool ncm_space_add_mode_parse(char *string, int32 string_len,
                              enum SpaceAddMode *value);
bool ncm_sort_mode_parse(char *string, int32 string_len,
                         enum SortMode *value);
char *ncm_display_mode_str(enum DisplayMode value);
bool ncm_display_mode_parse(char *string, int32 string_len,
                            enum DisplayMode *value);
char *ncm_design_str(enum Design value);
bool ncm_design_parse(char *string, int32 string_len, enum Design *value);
bool ncm_visualizer_type_parse(char *string, int32 string_len,
                               enum VisualizerType *value);

#endif /* NCM_ENUMS_H */
