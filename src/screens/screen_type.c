#if !defined(NCMPCPP_SCREEN_TYPE_C)
#define NCMPCPP_SCREEN_TYPE_C

#include "cbase.h"

#include "c/ncm_c.h"
#include "screens/nc_screens.h"

int32
screen_type_to_nc_type(enum ScreenType screen_type) {
    switch (screen_type) {
    #define NCM_SCREEN_TO_NC_CASE( \
        screen_type_value, nc_type, nc_value, alias, flags \
    ) \
        case screen_type_value: \
            return nc_type;

    NCM_SCREEN_TYPES(NCM_SCREEN_TO_NC_CASE)

    #undef NCM_SCREEN_TO_NC_CASE
    case NCM_SCREEN_TYPE_COUNT:
        break;
    default:
        break;
    }

    return NC_SCREEN_TYPE_UNKNOWN;
}

enum ScreenType
screen_type_from_nc_type(int32 nc_type) {
    switch (nc_type) {
    #define NCM_SCREEN_FROM_NC_CASE( \
        screen_type_value, nc_type_value, nc_value, alias, flags \
    ) \
        case nc_type_value: \
            return screen_type_value;

    NCM_SCREEN_TYPES(NCM_SCREEN_FROM_NC_CASE)

    #undef NCM_SCREEN_FROM_NC_CASE
    case NC_SCREEN_TYPE_UNKNOWN:
        break;
    default:
        break;
    }

    return NCM_SCREEN_TYPE_COUNT;
}

int32
screen_type_parse_startup(char *string, int32 string_len,
                          enum ScreenType *screen_type) {
    if ((string == NULL) || (string_len < 0) || (screen_type == NULL)) {
        return -EINVAL;
    }

    #define NCM_SCREEN_PARSE_STARTUP( \
        screen_type_value, nc_type, nc_value, alias, flags \
    ) \
        if (((flags & NCM_SCREEN_FLAG_STARTUP) != 0) \
            && STREQUAL(string, string_len, #alias)) { \
            *screen_type = screen_type_value; \
            return 0; \
        }

    NCM_SCREEN_TYPES(NCM_SCREEN_PARSE_STARTUP)

    #undef NCM_SCREEN_PARSE_STARTUP
    *screen_type = NCM_SCREEN_TYPE_COUNT;
    return -NCM_ERROR_PARSE;
}

bool
screen_type_parse(char *string, int32 string_len,
                  enum ScreenType *screen_type) {
    #define NCM_SCREEN_PARSE( \
        screen_type_value, nc_type, nc_value, alias, flags \
    ) \
        if (STREQUAL(string, string_len, #alias)) { \
            *screen_type = screen_type_value; \
            return true; \
        }

    NCM_SCREEN_TYPES(NCM_SCREEN_PARSE)

    #undef NCM_SCREEN_PARSE
    *screen_type = NCM_SCREEN_TYPE_COUNT;
    return false;
}

#endif /* NCMPCPP_SCREEN_TYPE_C */
