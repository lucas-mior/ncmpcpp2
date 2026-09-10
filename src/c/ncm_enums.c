#if !defined(NCM_ENUMS_C)
#define NCM_ENUMS_C

#include "cbase.h"
#include "ncmpcpp2.h"

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

#endif /* NCM_ENUMS_C */
