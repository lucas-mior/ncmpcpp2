/*
 * X-macro expansion helper for configuration_options.def.
 *
 * This file intentionally has no include guard. Define only the XX_* macros
 * needed by a generation pass before including it. Undefined option macros
 * expand to nothing. XX_OPTION can be defined as a common fallback for every
 * option type when a pass only needs the option name/default metadata.
 * All pass macros are undefined after the expansion.
 */

#if !defined(XX_BOOL)
#if defined(XX_OPTION)
#define XX_BOOL XX_OPTION
#else
#define XX_BOOL(NAME, DEFAULT)
#endif
#endif

#if !defined(XX_STRING)
#if defined(XX_OPTION)
#define XX_STRING XX_OPTION
#else
#define XX_STRING(NAME, DEFAULT)
#endif
#endif

#if !defined(XX_PATH)
#if defined(XX_OPTION)
#define XX_PATH XX_OPTION
#else
#define XX_PATH(NAME, DEFAULT)
#endif
#endif

#if !defined(XX_DIR)
#if defined(XX_OPTION)
#define XX_DIR XX_OPTION
#else
#define XX_DIR(NAME, DEFAULT)
#endif
#endif

#if !defined(XX_INTEGER)
#if defined(XX_OPTION)
#define XX_INTEGER XX_OPTION
#else
#define XX_INTEGER(NAME, DEFAULT, MINIMUM, MAXIMUM)
#endif
#endif

#if !defined(XX_DOUBLE)
#if defined(XX_OPTION)
#define XX_DOUBLE XX_OPTION
#else
#define XX_DOUBLE(NAME, DEFAULT, MINIMUM, MAXIMUM)
#endif
#endif

#if !defined(XX_ENUM)
#if defined(XX_OPTION)
#define XX_ENUM(NAME, DEFAULT, ENUM_PREFIX_) \
    XX_OPTION(NAME, DEFAULT, ENUM_PREFIX_)
#else
#define XX_ENUM(NAME, DEFAULT, ENUM_PREFIX_)
#endif
#endif

#if !defined(XX_MPD_TAG)
#if defined(XX_OPTION)
#define XX_MPD_TAG(NAME, DEFAULT) \
    XX_OPTION(NAME, DEFAULT)
#else
#define XX_MPD_TAG(NAME, DEFAULT)
#endif
#endif

#if !defined(XX_STARTUP_SCREEN)
#if defined(XX_OPTION)
#define XX_STARTUP_SCREEN(NAME, DEFAULT) \
    XX_OPTION(NAME, DEFAULT)
#else
#define XX_STARTUP_SCREEN(NAME, DEFAULT)
#endif
#endif

#if !defined(XX_OPT_STARTUP_SCREEN)
#if defined(XX_OPTION)
#define XX_OPT_STARTUP_SCREEN(NAME, DEFAULT, PRESENT_FIELD, UNSET_VALUE) \
    XX_OPTION(NAME, DEFAULT, PRESENT_FIELD, UNSET_VALUE)
#else
#define XX_OPT_STARTUP_SCREEN(NAME, DEFAULT, PRESENT_FIELD, UNSET_VALUE)
#endif
#endif

#if !defined(XX_COLOR)
#if defined(XX_OPTION)
#define XX_COLOR XX_OPTION
#else
#define XX_COLOR(NAME, DEFAULT)
#endif
#endif

#if !defined(XX_FORMATTED_COLOR)
#if defined(XX_OPTION)
#define XX_FORMATTED_COLOR XX_OPTION
#else
#define XX_FORMATTED_COLOR(NAME, DEFAULT)
#endif
#endif

#if !defined(XX_BORDER)
#if defined(XX_OPTION)
#define XX_BORDER XX_OPTION
#else
#define XX_BORDER(NAME, DEFAULT)
#endif
#endif

#if !defined(XX_FORMAT)
#if defined(XX_OPTION)
#define XX_FORMAT XX_OPTION
#else
#define XX_FORMAT(NAME, DEFAULT, FLAGS)
#endif
#endif

#if !defined(XX_BUFFER)
#if defined(XX_OPTION)
#define XX_BUFFER XX_OPTION
#else
#define XX_BUFFER(NAME, DEFAULT, KEEP_EXISTING)
#endif
#endif


#if !defined(XX_LOOK)
#if defined(XX_OPTION)
#define XX_LOOK XX_OPTION
#else
#define XX_LOOK(NAME, DEFAULT, MIN_CHARS, MAX_CHARS, PAD_TO_MAX)
#endif
#endif

#if !defined(XX_RATIO)
#if defined(XX_OPTION)
#define XX_RATIO XX_OPTION
#else
#define XX_RATIO(NAME, DEFAULT, EXPECTED_LEN)
#endif
#endif

#if !defined(XX_FORMATTED_COLOR_LIST)
#if defined(XX_OPTION)
#define XX_FORMATTED_COLOR_LIST XX_OPTION
#else
#define XX_FORMATTED_COLOR_LIST(NAME, DEFAULT)
#endif
#endif

#if !defined(XX_LYRICS_FETCHERS)
#if defined(XX_OPTION)
#define XX_LYRICS_FETCHERS XX_OPTION
#else
#define XX_LYRICS_FETCHERS(NAME, DEFAULT)
#endif
#endif

#if !defined(XX_SCREEN_LIST)
#if defined(XX_OPTION)
#define XX_SCREEN_LIST XX_OPTION
#else
#define XX_SCREEN_LIST(NAME, DEFAULT, PREVIOUS_FIELD)
#endif
#endif

#if !defined(XX_UINT32_CHOICE)
#if defined(XX_OPTION)
#define XX_UINT32_CHOICE XX_OPTION
#else
#define XX_UINT32_CHOICE(NAME, DEFAULT, PARSER, UNSET_VALUE)
#endif
#endif

#if !defined(XX_COLUMNS)
#if defined(XX_OPTION)
#define XX_COLUMNS XX_OPTION
#else
#define XX_COLUMNS(NAME, DEFAULT, FORMAT_FIELD)
#endif
#endif

#include "config_options.h"

#undef XX_COLUMNS
#undef XX_UINT32_CHOICE
#undef XX_SCREEN_LIST
#undef XX_LYRICS_FETCHERS
#undef XX_FORMATTED_COLOR_LIST
#undef XX_RATIO
#undef XX_LOOK
#undef XX_BUFFER
#undef XX_FORMAT
#undef XX_BORDER
#undef XX_FORMATTED_COLOR
#undef XX_COLOR
#undef XX_OPT_STARTUP_SCREEN
#undef XX_STARTUP_SCREEN
#undef XX_MPD_TAG
#undef XX_ENUM
#undef XX_DOUBLE
#undef XX_INTEGER
#undef XX_DIR
#undef XX_PATH
#undef XX_STRING
#undef XX_BOOL
#undef XX_OPTION
