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
#define XX_BOOL(NAME, DEFAULT_VALUE)
#endif
#endif

#if !defined(XX_STRING)
#if defined(XX_OPTION)
#define XX_STRING XX_OPTION
#else
#define XX_STRING(NAME, DEFAULT_VALUE)
#endif
#endif

#if !defined(XX_PATH)
#if defined(XX_OPTION)
#define XX_PATH XX_OPTION
#else
#define XX_PATH(NAME, DEFAULT_VALUE)
#endif
#endif

#if !defined(XX_DIR)
#if defined(XX_OPTION)
#define XX_DIR XX_OPTION
#else
#define XX_DIR(NAME, DEFAULT_VALUE)
#endif
#endif

#if !defined(XX_INT_RANGE)
#if defined(XX_OPTION)
#define XX_INT_RANGE XX_OPTION
#else
#define XX_INT_RANGE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM)
#endif
#endif

#if !defined(XX_DOUBLE_RANGE)
#if defined(XX_OPTION)
#define XX_DOUBLE_RANGE XX_OPTION
#else
#define XX_DOUBLE_RANGE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM)
#endif
#endif

#if !defined(XX_ENUM)
#if defined(XX_OPTION)
#define XX_ENUM(NAME, C_TYPE, DEFAULT_VALUE, PARSER) \
    XX_OPTION(NAME, DEFAULT_VALUE, C_TYPE, PARSER)
#else
#define XX_ENUM(NAME, C_TYPE, DEFAULT_VALUE, PARSER)
#endif
#endif

#if !defined(XX_OPTIONAL_ENUM)
#if defined(XX_OPTION)
#define XX_OPTIONAL_ENUM( \
    NAME, C_TYPE, DEFAULT_VALUE, PARSER, PRESENT_FIELD, UNSET_VALUE \
) \
    XX_OPTION(NAME, DEFAULT_VALUE, C_TYPE, PARSER, PRESENT_FIELD, UNSET_VALUE)
#else
#define XX_OPTIONAL_ENUM( \
    NAME, C_TYPE, DEFAULT_VALUE, PARSER, PRESENT_FIELD, UNSET_VALUE \
)
#endif
#endif

#if !defined(XX_COLOR)
#if defined(XX_OPTION)
#define XX_COLOR XX_OPTION
#else
#define XX_COLOR(NAME, DEFAULT_VALUE)
#endif
#endif

#if !defined(XX_FORMATTED_COLOR)
#if defined(XX_OPTION)
#define XX_FORMATTED_COLOR XX_OPTION
#else
#define XX_FORMATTED_COLOR(NAME, DEFAULT_VALUE)
#endif
#endif

#if !defined(XX_BORDER)
#if defined(XX_OPTION)
#define XX_BORDER XX_OPTION
#else
#define XX_BORDER(NAME, DEFAULT_VALUE)
#endif
#endif

#if !defined(XX_FORMAT)
#if defined(XX_OPTION)
#define XX_FORMAT XX_OPTION
#else
#define XX_FORMAT(NAME, DEFAULT_VALUE, FLAGS)
#endif
#endif

#if !defined(XX_BUFFER)
#if defined(XX_OPTION)
#define XX_BUFFER XX_OPTION
#else
#define XX_BUFFER(NAME, DEFAULT_VALUE, KEEP_EXISTING)
#endif
#endif

#if !defined(XX_BUFFER_WIDTH)
#if defined(XX_OPTION)
#define XX_BUFFER_WIDTH XX_OPTION
#else
#define XX_BUFFER_WIDTH(NAME, DEFAULT_VALUE, KEEP_EXISTING)
#endif
#endif

#if !defined(XX_LOOK)
#if defined(XX_OPTION)
#define XX_LOOK XX_OPTION
#else
#define XX_LOOK(NAME, DEFAULT_VALUE, MIN_CHARS, MAX_CHARS, PAD_TO_MAX)
#endif
#endif

#if !defined(XX_RATIO)
#if defined(XX_OPTION)
#define XX_RATIO XX_OPTION
#else
#define XX_RATIO(NAME, DEFAULT_VALUE, EXPECTED_LEN)
#endif
#endif

#if !defined(XX_FORMATTED_COLOR_LIST)
#if defined(XX_OPTION)
#define XX_FORMATTED_COLOR_LIST XX_OPTION
#else
#define XX_FORMATTED_COLOR_LIST(NAME, DEFAULT_VALUE)
#endif
#endif

#if !defined(XX_LYRICS_FETCHERS)
#if defined(XX_OPTION)
#define XX_LYRICS_FETCHERS XX_OPTION
#else
#define XX_LYRICS_FETCHERS(NAME, DEFAULT_VALUE)
#endif
#endif

#if !defined(XX_SCREEN_LIST)
#if defined(XX_OPTION)
#define XX_SCREEN_LIST XX_OPTION
#else
#define XX_SCREEN_LIST(NAME, DEFAULT_VALUE, PREVIOUS_FIELD)
#endif
#endif

#if !defined(XX_NAMED_BOOL)
#if defined(XX_OPTION)
#define XX_NAMED_BOOL XX_OPTION
#else
#define XX_NAMED_BOOL(NAME, DEFAULT_VALUE, TRUE_VALUE, FALSE_VALUE)
#endif
#endif

#if !defined(XX_UINT32_CHOICE)
#if defined(XX_OPTION)
#define XX_UINT32_CHOICE XX_OPTION
#else
#define XX_UINT32_CHOICE(NAME, DEFAULT_VALUE, PARSER, UNSET_VALUE)
#endif
#endif

#if !defined(XX_COLUMNS)
#if defined(XX_OPTION)
#define XX_COLUMNS XX_OPTION
#else
#define XX_COLUMNS(NAME, DEFAULT_VALUE, FORMAT_FIELD)
#endif
#endif

#include "config_options.h"

#undef XX_COLUMNS
#undef XX_UINT32_CHOICE
#undef XX_NAMED_BOOL
#undef XX_SCREEN_LIST
#undef XX_LYRICS_FETCHERS
#undef XX_FORMATTED_COLOR_LIST
#undef XX_RATIO
#undef XX_LOOK
#undef XX_BUFFER_WIDTH
#undef XX_BUFFER
#undef XX_FORMAT
#undef XX_BORDER
#undef XX_FORMATTED_COLOR
#undef XX_COLOR
#undef XX_OPTIONAL_ENUM
#undef XX_ENUM
#undef XX_DOUBLE_RANGE
#undef XX_INT_RANGE
#undef XX_DIR
#undef XX_PATH
#undef XX_STRING
#undef XX_BOOL
#undef XX_OPTION
