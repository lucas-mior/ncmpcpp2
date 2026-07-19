#if !defined(NCM_ENUMS_H)
#define NCM_ENUMS_H

#include "config.h"
#include "c/ncm_defs.h"

#define ENUM_NAME SearchDirection
#define ENUM_PREFIX_ NCM_SEARCH_DIRECTION_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(NCM_SEARCH_DIRECTION_BACKWARD) \
    X(NCM_SEARCH_DIRECTION_FORWARD)
#include "cbase/xenums.c"

#define ENUM_NAME SpaceAddMode
#define ENUM_PREFIX_ NCM_SPACE_ADD_MODE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(NCM_SPACE_ADD_MODE_ADD_REMOVE) \
    X(NCM_SPACE_ADD_MODE_ALWAYS_ADD)
#include "cbase/xenums.c"

#define ENUM_NAME SortMode
#define ENUM_PREFIX_ NCM_SORT_MODE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(NCM_SORT_MODE_TYPE) \
    X(NCM_SORT_MODE_NAME) \
    X(NCM_SORT_MODE_MODIFICATION_TIME) \
    X(NCM_SORT_MODE_CUSTOM_FORMAT) \
    X(NCM_SORT_MODE_NONE)
#include "cbase/xenums.c"

#define ENUM_NAME DisplayMode
#define ENUM_PREFIX_ NCM_DISPLAY_MODE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(NCM_DISPLAY_MODE_CLASSIC) \
    X(NCM_DISPLAY_MODE_COLUMNS)
#include "cbase/xenums.c"

#define ENUM_NAME Design
#define ENUM_PREFIX_ NCM_DESIGN_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(NCM_DESIGN_CLASSIC) \
    X(NCM_DESIGN_ALTERNATIVE)
#include "cbase/xenums.c"

#define ENUM_NAME VisualizerType
#define ENUM_PREFIX_ NCM_VISUALIZER_TYPE_
#define ENUM_BITFLAGS 0
#if defined(HAVE_FFTW3_H)
#define ENUM_FIELDS \
    X(NCM_VISUALIZER_TYPE_WAVE) \
    X(NCM_VISUALIZER_TYPE_WAVE_FILLED) \
    X(NCM_VISUALIZER_TYPE_SPECTRUM) \
    X(NCM_VISUALIZER_TYPE_ELLIPSE)
#else
#define ENUM_FIELDS \
    X(NCM_VISUALIZER_TYPE_WAVE) \
    X(NCM_VISUALIZER_TYPE_WAVE_FILLED) \
    X(NCM_VISUALIZER_TYPE_ELLIPSE)
#endif
#include "cbase/xenums.c"

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
