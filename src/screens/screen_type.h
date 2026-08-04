#if !defined(NCMPCPP_SCREEN_TYPE_H)
#define NCMPCPP_SCREEN_TYPE_H

#include "cbase.h"

#include "c/ncm_defs.h"
#include "screens/nc_screen.h"
#include "screens/screen_defs.h"

#define ENUM_NAME ScreenType
#define ENUM_PREFIX_ NCM_SCREEN_TYPE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS NCM_SCREEN_TYPE_ENUM_FIELDS
#include "cbase/xenums.c"

int32 screen_type_to_native_type(enum ScreenType screen_type);
enum ScreenType screen_type_from_native_type(int32 native_type);
bool screen_type_parse_startup(char *string, int32 string_len,
                               enum ScreenType *screen_type);
bool screen_type_parse(char *string, int32 string_len,
                       enum ScreenType *screen_type);

#endif /* NCMPCPP_SCREEN_TYPE_H */
