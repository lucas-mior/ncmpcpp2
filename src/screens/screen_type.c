#if !defined(SCREEN_TYPE_C)
#define SCREEN_TYPE_C

#include "cbase.h"

#include "c/ncm_c.h"
#include "screens/nc_screens.h"

enum NcScreenType
screen_type_to_nc_type(enum ScreenType screen_type) {
    switch (screen_type) {
    #define SCREEN_TO_NC_CASE( \
        screen_type_value, nc_type, nc_value, alias, flags \
    ) \
        case screen_type_value: \
            return nc_type;

    SCREEN_TYPES(SCREEN_TO_NC_CASE)

    #undef SCREEN_TO_NC_CASE
    case SCREEN_TYPE_COUNT:
        break;
    default:
        break;
    }

    return NC_SCREEN_TYPE_UNKNOWN;
}

enum ScreenType
screen_type_from_nc_type(enum NcScreenType nc_type) {
    switch (nc_type) {
    #define SCREEN_FROM_NC_CASE( \
        screen_type_value, nc_type_value, nc_value, alias, flags \
    ) \
        case nc_type_value: \
            return screen_type_value;

    SCREEN_TYPES(SCREEN_FROM_NC_CASE)

    #undef SCREEN_FROM_NC_CASE
    case NC_SCREEN_TYPE_UNKNOWN:
        break;
    default:
        break;
    }

    return SCREEN_TYPE_COUNT;
}

bool
screen_type_is_startup(enum ScreenType screen_type) {
    switch (screen_type) {
    #define SCREEN_STARTUP_CASE( \
        screen_type_value, nc_type, nc_value, alias, flags \
    ) \
        case screen_type_value: \
            return (flags & SCREEN_FLAG_STARTUP) != 0;

    SCREEN_TYPES(SCREEN_STARTUP_CASE)

    #undef SCREEN_STARTUP_CASE
    case SCREEN_TYPE_COUNT:
        break;
    default:
        break;
    }

    return false;
}

static int32
screen_type_parse_checked(char *string, int32 string_len, bool startup_only,
                          enum ScreenType *screen_type) {
    if ((string == NULL) || (string_len < 0) || (screen_type == NULL)) {
        return -EINVAL;
    }

    #define SCREEN_PARSE_CHECKED( \
        screen_type_value, nc_type, nc_value, alias, flags \
    ) \
        if ((!startup_only || ((flags & SCREEN_FLAG_STARTUP) != 0)) \
            && STREQUAL(string, string_len, #alias)) { \
            *screen_type = SCREEN_TYPE_parse(string, string_len); \
            return 0; \
        }

    SCREEN_TYPES(SCREEN_PARSE_CHECKED)

    #undef SCREEN_PARSE_CHECKED
    *screen_type = SCREEN_TYPE_COUNT;
    return -NCM_ERROR_PARSE;
}

int32
screen_type_parse_startup(char *string, int32 string_len,
                          enum ScreenType *screen_type) {
    return screen_type_parse_checked(string, string_len, true, screen_type);
}

int32
screen_type_parse(char *string, int32 string_len,
                  enum ScreenType *screen_type) {
    return screen_type_parse_checked(string, string_len, false, screen_type);
}

#endif /* SCREEN_TYPE_C */
