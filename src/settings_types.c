#if !defined(NCMPCPP_SETTINGS_TYPES_C)
#define NCMPCPP_SETTINGS_TYPES_C

#include "cbase.h"

#include "c/ncm_c.h"
#include "settings.h"

static void
settings_screen_type_array_init_item(void *item) {
    enum ScreenType *screen = item;

    *screen = NCM_SCREEN_TYPE_PLAYLIST;
    return;
}

static void
settings_formatted_color_array_init_item(void *item) {
    nc_formatted_color_init(item);
    return;
}

static void
settings_formatted_color_array_destroy_item(void *item) {
    ASSERT(item != NULL);
    nc_formatted_color_destroy(item);
    return;
}

static void
settings_column_array_init_item(void *item) {
    Column *column = item;

    column->name = NULL;
    column->type = NULL;
    column->name_len = 0;
    column->name_cap = 0;
    column->type_len = 0;
    column->type_cap = 0;
    column->width = 0;
    column->stretch_limit = -1;
    column->color = nc_color_default();
    column->fixed = false;
    column->right_alignment = false;
    column->display_empty_tag = true;
    return;
}

static void
settings_column_array_destroy_item(void *item) {
    Column *column = item;

    stupid_string_free(&column->name, &column->name_len, &column->name_cap);
    stupid_string_free(&column->type, &column->type_len, &column->type_cap);
    return;
}

static NcmArrayItemCallbacks settings_screen_type_callbacks = {
    .init = settings_screen_type_array_init_item,
};

static NcmArrayItemCallbacks settings_formatted_color_callbacks = {
    .init = settings_formatted_color_array_init_item,
    .destroy = settings_formatted_color_array_destroy_item,
};

static NcmArrayItemCallbacks settings_column_callbacks = {
    .init = settings_column_array_init_item,
    .destroy = settings_column_array_destroy_item,
};

NCM_ARRAY_DEFINE_CLEAR(screen_type_array, ScreenTypeArray,
                       &settings_screen_type_callbacks)
NCM_ARRAY_DEFINE_DESTROY(screen_type_array, ScreenTypeArray)
NCM_ARRAY_DEFINE_RESERVE(screen_type_array, ScreenTypeArray)
NCM_ARRAY_DEFINE_APPEND(screen_type_array, ScreenTypeArray, enum ScreenType,
                        &settings_screen_type_callbacks)

NCM_ARRAY_DEFINE_CLEAR(ncm_int32_array, NcmInt32Array, NULL)
NCM_ARRAY_DEFINE_DESTROY(ncm_int32_array, NcmInt32Array)
NCM_ARRAY_DEFINE_RESERVE(ncm_int32_array, NcmInt32Array)
NCM_ARRAY_DEFINE_APPEND(ncm_int32_array, NcmInt32Array, int32, NULL)

NCM_ARRAY_DEFINE_CLEAR(ncm_formatted_color_array, NcmFormattedColorArray,
                       &settings_formatted_color_callbacks)
NCM_ARRAY_DEFINE_DESTROY(ncm_formatted_color_array, NcmFormattedColorArray)
NCM_ARRAY_DEFINE_RESERVE(ncm_formatted_color_array, NcmFormattedColorArray)
NCM_ARRAY_DEFINE_APPEND(ncm_formatted_color_array, NcmFormattedColorArray,
                        NcFormattedColor, &settings_formatted_color_callbacks)

NCM_ARRAY_DEFINE_CLEAR(column_array, ColumnArray, &settings_column_callbacks)
NCM_ARRAY_DEFINE_DESTROY(column_array, ColumnArray)
NCM_ARRAY_DEFINE_RESERVE(column_array, ColumnArray)
NCM_ARRAY_DEFINE_APPEND(column_array, ColumnArray, Column,
                        &settings_column_callbacks)

static void
configuration_init_unchecked(Configuration *config) {
#define XX_BOOL(NAME, DEFAULT_VALUE) config->NAME = false;
#define XX_STRING(NAME, DEFAULT_VALUE) \
    config->NAME = NULL; \
    config->NAME##_len = 0;
#define XX_PATH(NAME, DEFAULT_VALUE) XX_STRING(NAME, DEFAULT_VALUE)
#define XX_DIR(NAME, DEFAULT_VALUE) XX_STRING(NAME, DEFAULT_VALUE)
#define XX_INT_RANGE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM) \
    config->NAME = 0;
#define XX_DOUBLE_RANGE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM) \
    config->NAME = 0;
#define XX_ENUM(NAME, C_TYPE, DEFAULT_VALUE, PARSER) \
    config->NAME = (C_TYPE)0;
#define XX_OPTIONAL_ENUM( \
    NAME, C_TYPE, DEFAULT_VALUE, PARSER, PRESENT_FIELD, UNSET_VALUE \
) \
    config->NAME = (C_TYPE)(UNSET_VALUE); \
    config->PRESENT_FIELD = false;
#define XX_COLOR(NAME, DEFAULT_VALUE) \
    config->NAME = nc_color_default();
#define XX_FORMATTED_COLOR(NAME, DEFAULT_VALUE) \
    nc_formatted_color_init(&config->NAME);
#define XX_BORDER(NAME, DEFAULT_VALUE) \
    config->NAME = nc_border_none();
#define XX_FORMAT(NAME, DEFAULT_VALUE, FLAGS) \
    config->NAME = (NcmFormatAst){0};
#define XX_BUFFER(NAME, DEFAULT_VALUE, KEEP_EXISTING) \
    config->NAME = (NcBuffer){0};
#define XX_BUFFER_WIDTH(NAME, DEFAULT_VALUE, KEEP_EXISTING) \
    config->NAME = (NcBuffer){0}; \
    config->NAME##_length = 0;
#define XX_LOOK(NAME, DEFAULT_VALUE, MIN_CHARS, MAX_CHARS, PAD_TO_MAX) \
    config->NAME = (StrBuilder){0};
#define XX_RATIO(NAME, DEFAULT_VALUE, EXPECTED_LEN) \
    config->NAME = (NcmInt32Array){0};
#define XX_FORMATTED_COLOR_LIST(NAME, DEFAULT_VALUE) \
    config->NAME = (NcmFormattedColorArray){0};
#define XX_LYRICS_FETCHERS(NAME, DEFAULT_VALUE) \
    config->NAME = (NcmLyricsFetcherRegistry){0};
#define XX_SCREEN_LIST(NAME, DEFAULT_VALUE, PREVIOUS_FIELD) \
    config->NAME = (ScreenTypeArray){0}; \
    config->PREVIOUS_FIELD = false;
#define XX_NAMED_BOOL(NAME, DEFAULT_VALUE, TRUE_VALUE, FALSE_VALUE) \
    config->NAME = false;
#define XX_UINT32_CHOICE(NAME, DEFAULT_VALUE, PARSER, UNSET_VALUE) \
    config->NAME = (UNSET_VALUE);
#define XX_COLUMNS(NAME, DEFAULT_VALUE, FORMAT_FIELD) \
    config->FORMAT_FIELD = (NcmFormatAst){0}; \
    config->NAME = (ColumnArray){0};
#include "configuration_options_pass.h"

    return;
}

void
configuration_init(Configuration *config) {
    if (config == NULL) {
        return;
    }

    configuration_init_unchecked(config);
    return;
}

void
configuration_destroy(Configuration *config) {
    if (config == NULL) {
        return;
    }

#define XX_BOOL(NAME, DEFAULT_VALUE)
#define XX_STRING(NAME, DEFAULT_VALUE)           \
    free2(config->NAME, config->NAME##_len + 1); \
    config->NAME = NULL;                         \
    config->NAME##_len = 0;
#define XX_PATH(NAME, DEFAULT_VALUE) XX_STRING(NAME, DEFAULT_VALUE)
#define XX_DIR(NAME, DEFAULT_VALUE) XX_STRING(NAME, DEFAULT_VALUE)
#define XX_INT_RANGE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM)
#define XX_DOUBLE_RANGE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM)
#define XX_ENUM(NAME, C_TYPE, DEFAULT_VALUE, PARSER)
#define XX_OPTIONAL_ENUM( \
    NAME, C_TYPE, DEFAULT_VALUE, PARSER, PRESENT_FIELD, UNSET_VALUE \
)
#define XX_COLOR(NAME, DEFAULT_VALUE)
#define XX_FORMATTED_COLOR(NAME, DEFAULT_VALUE) \
    nc_formatted_color_destroy(&config->NAME);
#define XX_BORDER(NAME, DEFAULT_VALUE)
#define XX_FORMAT(NAME, DEFAULT_VALUE, FLAGS) \
    ncm_format_ast_destroy(&config->NAME);
#define XX_BUFFER(NAME, DEFAULT_VALUE, KEEP_EXISTING) \
    nc_buffer_destroy(&config->NAME);
#define XX_BUFFER_WIDTH(NAME, DEFAULT_VALUE, KEEP_EXISTING) \
    nc_buffer_destroy(&config->NAME); \
    config->NAME##_length = 0;
#define XX_LOOK(NAME, DEFAULT_VALUE, MIN_CHARS, MAX_CHARS, PAD_TO_MAX) \
    sb_free(&config->NAME);
#define XX_RATIO(NAME, DEFAULT_VALUE, EXPECTED_LEN) \
    ncm_int32_array_destroy(&config->NAME);
#define XX_FORMATTED_COLOR_LIST(NAME, DEFAULT_VALUE) \
    ncm_formatted_color_array_destroy(&config->NAME);
#define XX_LYRICS_FETCHERS(NAME, DEFAULT_VALUE) \
    ncm_lyrics_fetcher_registry_destroy(&config->NAME);
#define XX_SCREEN_LIST(NAME, DEFAULT_VALUE, PREVIOUS_FIELD) \
    screen_type_array_destroy(&config->NAME); \
    config->PREVIOUS_FIELD = false;
#define XX_NAMED_BOOL(NAME, DEFAULT_VALUE, TRUE_VALUE, FALSE_VALUE)
#define XX_UINT32_CHOICE(NAME, DEFAULT_VALUE, PARSER, UNSET_VALUE)
#define XX_COLUMNS(NAME, DEFAULT_VALUE, FORMAT_FIELD) \
    ncm_format_ast_destroy(&config->FORMAT_FIELD); \
    column_array_destroy(&config->NAME);
#include "configuration_options_pass.h"

    configuration_init_unchecked(config);

    return;
}

void
configuration_clear(Configuration *config) {
    configuration_destroy(config);
    return;
}

double
configuration_locked_screen_width_fraction(const Configuration *config) {
    ASSERT(config != NULL);
    return config->locked_screen_width_part / 100.0;
}

enum SearchEngineSearchMode
configuration_search_engine_default_mode(const Configuration *config) {
    int32 mode;

    ASSERT(config != NULL);

    mode = config->search_engine_default_search_mode - 1;
    if ((mode < (int32)SEARCH_ENGINE_SEARCH_MODE_LITERAL)
        || (mode >= (int32)SEARCH_ENGINE_SEARCH_MODE_COUNT)) {
        return SEARCH_ENGINE_SEARCH_MODE_LITERAL;
    }
    return (enum SearchEngineSearchMode)mode;
}

#endif /* NCMPCPP_SETTINGS_TYPES_C */
