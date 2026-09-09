#if !defined(SETTINGS_TYPES_C)
#define SETTINGS_TYPES_C

#include "cbase.h"

#include "c/ncm_c.h"
#include "settings.h"

static void
settings_screen_type_array_init_item(void *item) {
    enum ScreenType *screen = item;
    *screen = SCREEN_TYPE_PLAYLIST;
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
    column->type_len = 0;
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

    stupid_string_free(&column->name, &column->name_len);
    stupid_string_free(&column->type, &column->type_len);
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
#define XX_BOOL(NAME, DEFAULT)                                            \
    config->NAME = false;
#define XX_STRING(NAME, DEFAULT)                                          \
    config->NAME = NULL;                                                  \
    config->NAME##_len = 0;
#define XX_PATH(NAME, DEFAULT)                                            \
    XX_STRING(NAME, DEFAULT)
#define XX_DIR(NAME, DEFAULT)                                             \
    XX_STRING(NAME, DEFAULT)
#define XX_INTEGER(NAME, DEFAULT, MINIMUM, MAXIMUM)                       \
    config->NAME = 0;
#define XX_DOUBLE(NAME, DEFAULT, MINIMUM, MAXIMUM)                        \
    config->NAME = 0;
#define XX_ENUM(NAME, DEFAULT, ENUM_PREFIX_)                              \
    config->NAME = (ENUM_PREFIX_)0;
#define XX_MPD_TAG(NAME, DEFAULT)                                         \
    config->NAME = NCM_TAG_UNKNOWN;
#define XX_STARTUP_SCREEN(NAME, DEFAULT)                                  \
    config->NAME = SCREEN_TYPE_COUNT;
#define XX_OPT_STARTUP_SCREEN(NAME, DEFAULT, PRESENT_FIELD, UNSET_VALUE)  \
    config->NAME = (SCREEN_TYPE_)(UNSET_VALUE);                           \
    config->PRESENT_FIELD = false;
#define XX_COLOR(NAME, DEFAULT)                                           \
    config->NAME = nc_color_default();
#define XX_FORMATTED_COLOR(NAME, DEFAULT)                                 \
    nc_formatted_color_init(&config->NAME);
#define XX_BORDER(NAME, DEFAULT)                                          \
    config->NAME = nc_border_none();
#define XX_FORMAT(NAME, DEFAULT, FLAGS)                                   \
    config->NAME = (NcmFormatAst){0};
#define XX_BUFFER(NAME, DEFAULT, KEEP_EXISTING)                           \
    config->NAME = (NcBuffer){0};
#define XX_LOOK(NAME, DEFAULT, MIN_CHARS, MAX_CHARS, PAD_TO_MAX)          \
    config->NAME = (StrBuilder){0};
#define XX_RATIO(NAME, DEFAULT, EXPECTED_LEN)                             \
    config->NAME = (NcmInt32Array){0};
#define XX_FORMATTED_COLOR_LIST(NAME, DEFAULT)                            \
    config->NAME = (NcmFormattedColorArray){0};
#define XX_LYRICS_FETCHERS(NAME, DEFAULT)                                 \
    config->NAME = (LyricsFetcherRegistry){0};
#define XX_SCREEN_LIST(NAME, DEFAULT, PREVIOUS_FIELD)                     \
    config->NAME = (ScreenTypeArray){0};                                  \
    config->PREVIOUS_FIELD = false;
#define XX_UINT32_CHOICE(NAME, DEFAULT, PARSER, UNSET_VALUE)              \
    config->NAME = (UNSET_VALUE);
#define XX_COLUMNS(NAME, DEFAULT, FORMAT_FIELD)                           \
    config->FORMAT_FIELD = (NcmFormatAst){0};                             \
    config->NAME = (ColumnArray){0};

#include "config_options_pass.h"

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

#define XX_STRING(NAME, DEFAULT)                                          \
    free2(config->NAME, config->NAME##_len + 1);                          \
    config->NAME = NULL;                                                  \
    config->NAME##_len = 0;
#define XX_PATH(NAME, DEFAULT)                                            \
    free2(config->NAME, config->NAME##_len + 1);                          \
    config->NAME = NULL;                                                  \
    config->NAME##_len = 0;
#define XX_DIR(NAME, DEFAULT)                                             \
    free2(config->NAME, config->NAME##_len + 1);                          \
    config->NAME = NULL;                                                  \
    config->NAME##_len = 0;
#define XX_FORMATTED_COLOR(NAME, DEFAULT)                                 \
    nc_formatted_color_destroy(&config->NAME);
#define XX_FORMAT(NAME, DEFAULT, FLAGS)                                   \
    ncm_format_ast_destroy(&config->NAME);
#define XX_BUFFER(NAME, DEFAULT, KEEP_EXISTING)                           \
    nc_buffer_destroy(&config->NAME);
#define XX_LOOK(NAME, DEFAULT, MIN_CHARS, MAX_CHARS, PAD_TO_MAX)          \
    sb_free(&config->NAME);
#define XX_RATIO(NAME, DEFAULT, EXPECTED_LEN)                             \
    ncm_int32_array_destroy(&config->NAME);
#define XX_FORMATTED_COLOR_LIST(NAME, DEFAULT)                            \
    ncm_formatted_color_array_destroy(&config->NAME);
#define XX_LYRICS_FETCHERS(NAME, DEFAULT)                                 \
    ncm_lyrics_fetcher_registry_destroy(&config->NAME);
#define XX_SCREEN_LIST(NAME, DEFAULT, PREVIOUS_FIELD)                     \
    screen_type_array_destroy(&config->NAME);                             \
    config->PREVIOUS_FIELD = false;
#define XX_COLUMNS(NAME, DEFAULT, FORMAT_FIELD)                           \
    ncm_format_ast_destroy(&config->FORMAT_FIELD);                        \
    column_array_destroy(&config->NAME);

#include "config_options_pass.h"

    configuration_init_unchecked(config);

    return;
}

double
configuration_locked_screen_width_fraction(Configuration *config) {
    ASSERT(config != NULL);
    return config->locked_screen_width_part / 100.0;
}

enum SearchEngineSearchMode
configuration_search_engine_default_mode(Configuration *config) {
    int32 mode;

    ASSERT(config != NULL);

    mode = config->search_engine_default_search_mode - 1;
    if ((mode < (int32)SEARCH_ENGINE_SEARCH_MODE_LITERAL)
        || (mode >= (int32)SEARCH_ENGINE_SEARCH_MODE_COUNT)) {
        return SEARCH_ENGINE_SEARCH_MODE_LITERAL;
    }
    return (enum SearchEngineSearchMode)mode;
}

#endif /* SETTINGS_TYPES_C */
