#if !defined(NCMPCPP_SETTINGS_H)
#define NCMPCPP_SETTINGS_H

#include "cbase.h"

#include <mpd/tag.h>

#include "c/ncm_c.h"
#include "curses/nc_curses.h"
#include "lyrics_fetcher.h"
#include "screens/nc_screens.h"

typedef struct Column {
    char *name;
    char *type;

    int32 name_len;
    int32 name_cap;
    int32 type_len;
    int32 type_cap;

    int32 width;
    int32 stretch_limit;
    NcColor color;

    bool fixed;
    bool right_alignment;
    bool display_empty_tag;
} Column;

typedef struct ColumnArray {
    Column *items;
    int32 len;
    int32 cap;
} ColumnArray;

NCM_ARRAY_DECLARE_TYPE(ScreenTypeArray, enum ScreenType)
NCM_ARRAY_DECLARE_CLEAR(screen_type_array, ScreenTypeArray)
NCM_ARRAY_DECLARE_DESTROY(screen_type_array, ScreenTypeArray)
NCM_ARRAY_DECLARE_RESERVE(screen_type_array, ScreenTypeArray)
NCM_ARRAY_DECLARE_APPEND(screen_type_array, ScreenTypeArray, enum ScreenType)

NCM_ARRAY_DECLARE_TYPE(NcmInt32Array, int32)
NCM_ARRAY_DECLARE_CLEAR(ncm_int32_array, NcmInt32Array)
NCM_ARRAY_DECLARE_DESTROY(ncm_int32_array, NcmInt32Array)
NCM_ARRAY_DECLARE_RESERVE(ncm_int32_array, NcmInt32Array)
NCM_ARRAY_DECLARE_APPEND(ncm_int32_array, NcmInt32Array, int32)

NCM_ARRAY_DECLARE_TYPE(NcmFormattedColorArray, NcFormattedColor)
NCM_ARRAY_DECLARE_CLEAR(ncm_formatted_color_array, NcmFormattedColorArray)
NCM_ARRAY_DECLARE_DESTROY(ncm_formatted_color_array, NcmFormattedColorArray)
NCM_ARRAY_DECLARE_RESERVE(ncm_formatted_color_array, NcmFormattedColorArray)
NCM_ARRAY_DECLARE_APPEND(ncm_formatted_color_array, NcmFormattedColorArray,
                         NcFormattedColor)

enum SettingsOptionId {
#define XX_OPTION(NAME, DEFAULT_VALUE, ...) SETTINGS_OPTION_##NAME,
#include "config_options_pass.h"
    SETTINGS_OPTION_COUNT,
};

/* Fields and intrinsic companion state come from the option schema. */
typedef struct Configuration {
#define XX_BOOL(NAME, DEFAULT_VALUE) bool NAME;
#define XX_STRING(NAME, DEFAULT_VALUE) \
    char *NAME; \
    int32 NAME##_len;
#define XX_PATH(NAME, DEFAULT_VALUE) XX_STRING(NAME, DEFAULT_VALUE)
#define XX_DIR(NAME, DEFAULT_VALUE) XX_STRING(NAME, DEFAULT_VALUE)
#define XX_INTEGER(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM) int32 NAME;
#define XX_DOUBLE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM) \
    double NAME;
#define XX_ENUM(NAME, C_TYPE, DEFAULT_VALUE, PARSER) C_TYPE NAME;
#define XX_OPTIONAL_ENUM( \
    NAME, C_TYPE, DEFAULT_VALUE, PARSER, PRESENT_FIELD, UNSET_VALUE \
) \
    C_TYPE NAME; \
    bool PRESENT_FIELD;
#define XX_COLOR(NAME, DEFAULT_VALUE) NcColor NAME;
#define XX_FORMATTED_COLOR(NAME, DEFAULT_VALUE) NcFormattedColor NAME;
#define XX_BORDER(NAME, DEFAULT_VALUE) NcBorder NAME;
#define XX_FORMAT(NAME, DEFAULT_VALUE, FLAGS) NcmFormatAst NAME;
#define XX_BUFFER(NAME, DEFAULT_VALUE, KEEP_EXISTING) NcBuffer NAME;
#define XX_BUFFER_WIDTH(NAME, DEFAULT_VALUE, KEEP_EXISTING) \
    NcBuffer NAME; \
    int32 NAME##_length;
#define XX_LOOK(NAME, DEFAULT_VALUE, MIN_CHARS, MAX_CHARS, PAD_TO_MAX) \
    StrBuilder NAME;
#define XX_RATIO(NAME, DEFAULT_VALUE, EXPECTED_LEN) NcmInt32Array NAME;
#define XX_FORMATTED_COLOR_LIST(NAME, DEFAULT_VALUE) \
    NcmFormattedColorArray NAME;
#define XX_LYRICS_FETCHERS(NAME, DEFAULT_VALUE) NcmLyricsFetcherRegistry NAME;
#define XX_SCREEN_LIST(NAME, DEFAULT_VALUE, PREVIOUS_FIELD) \
    ScreenTypeArray NAME; \
    bool PREVIOUS_FIELD;
#define XX_NAMED_BOOL(NAME, DEFAULT_VALUE, TRUE_VALUE, FALSE_VALUE) bool NAME;
#define XX_UINT32_CHOICE(NAME, DEFAULT_VALUE, PARSER, UNSET_VALUE) uint32 NAME;
#define XX_COLUMNS(NAME, DEFAULT_VALUE, FORMAT_FIELD) \
    NcmFormatAst FORMAT_FIELD; \
    ColumnArray NAME;
#include "config_options_pass.h"

} Configuration;

void column_array_clear(ColumnArray *array);
void column_array_destroy(ColumnArray *array);
int32 column_array_reserve(ColumnArray *array, int32 extra);
Column *column_array_append(ColumnArray *array);

void configuration_init(Configuration *config);
void configuration_destroy(Configuration *config);
void configuration_clear(Configuration *config);
int32 configuration_validate(Configuration *config, NcmError *ncm_error);
double configuration_locked_screen_width_fraction(Configuration *config);
enum SearchEngineSearchMode configuration_search_engine_default_mode(
    Configuration *config
);
int32 configuration_read(Configuration *config,
                         NcmStringViewArray *config_paths,
                         bool ignore_errors, bool quiet,
                         NcmError *ncm_error);
int32 configuration_apply_runtime(Configuration *config,
                                  NcmMpdClient *client, bool quiet,
                                  NcmError *ncm_error);

extern Configuration Config;

#endif /* NCMPCPP_SETTINGS_H */
