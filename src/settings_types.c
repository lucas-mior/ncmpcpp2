#if !defined(SETTINGS_TYPES_C)
#define SETTINGS_TYPES_C

#include "cbase.h"
#include "ncmpcpp2.h"

#include "c/ncm_c.h"
#include "settings.h"

static void
settings_screen_type_array_init_item(void *item) {
    enum ScreenType *screen = item;
    *screen = SCREEN_TYPE_PLAYLIST;
    return;
}

static void
settings_text_style_array_init_item(void *item) {
    nc_text_style_init(item);
    return;
}

static void
settings_text_style_array_destroy_item(void *item) {
    ASSERT(item != NULL);
    nc_text_style_destroy(item);
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

#define NCM_ARRAY_TYPE ScreenTypeArray
#define NCM_ARRAY_ITEM_TYPE enum ScreenType
#define NCM_ARRAY_PREFIX screen_type_array
#define NCM_ARRAY_ITEM_INIT settings_screen_type_array_init_item
#include "c/ncm_array_impl_template.h"

#define NCM_ARRAY_TYPE NcmInt32Array
#define NCM_ARRAY_ITEM_TYPE int32
#define NCM_ARRAY_PREFIX ncm_int32_array
#include "c/ncm_array_impl_template.h"

#define NCM_ARRAY_TYPE NcmTextStyleArray
#define NCM_ARRAY_ITEM_TYPE NcTextStyle
#define NCM_ARRAY_PREFIX ncm_text_style_array
#define NCM_ARRAY_ITEM_INIT settings_text_style_array_init_item
#define NCM_ARRAY_ITEM_DESTROY settings_text_style_array_destroy_item
#include "c/ncm_array_impl_template.h"

#define NCM_ARRAY_TYPE ColumnArray
#define NCM_ARRAY_ITEM_TYPE Column
#define NCM_ARRAY_PREFIX column_array
#define NCM_ARRAY_ITEM_INIT settings_column_array_init_item
#define NCM_ARRAY_ITEM_DESTROY settings_column_array_destroy_item
#include "c/ncm_array_impl_template.h"

static void
config_init_unchecked(Configuration *config) {
#define XX_BOOL(NAME, DEFAULT)                                            \
    config->NAME = false;
#define XX_STRING(NAME, DEFAULT)                                          \
    config->NAME = NULL;                                                  \
    config->NAME##_len = 0;
#define XX_PATH(NAME, DEFAULT)                                            \
    config->NAME = NULL;                                                  \
    config->NAME##_len = 0;
#define XX_DIR(NAME, DEFAULT)                                             \
    config->NAME = NULL;                                                  \
    config->NAME##_len = 0;
#define XX_INTEGER(NAME, DEFAULT, MINIMUM, MAXIMUM)                       \
    config->NAME = 0;
#define XX_DOUBLE(NAME, DEFAULT, MINIMUM, MAXIMUM)                        \
    config->NAME = 0;
#define XX_ENUM(NAME, DEFAULT, ENUM_PREFIX_)                              \
    config->NAME = (ENUM_PREFIX_)0;
#define XX_MEDIA_LIBRARY_GROUPING_TAG(NAME, DEFAULT)                      \
    config->NAME = TAG_COUNT;
#define XX_STARTUP_SCREEN(NAME, DEFAULT)                                  \
    config->NAME = SCREEN_TYPE_COUNT;
#define XX_OPT_STARTUP_SCREEN(NAME, DEFAULT, PRESENT_FIELD, UNSET_VALUE)  \
    config->NAME = (SCREEN_TYPE_)(UNSET_VALUE);                           \
    config->PRESENT_FIELD = false;
#define XX_COLOR(NAME, DEFAULT)                                           \
    config->NAME = nc_color_default();
#define XX_TEXT_STYLE(NAME, DEFAULT)                                      \
    nc_text_style_init(&config->NAME);
#define XX_BORDER(NAME, DEFAULT)                                          \
    config->NAME = nc_border_none();
#define XX_FORMAT(NAME, DEFAULT, FLAGS)                                   \
    config->NAME = (NcmFormatAst){0};
#define XX_BUFFER(NAME, DEFAULT, KEEP_EXISTING)                           \
    config->NAME = (NcBuffer){0};
#define XX_LOOK(NAME, DEFAULT, MIN_CHARS, MAX_CHARS, PAD_TO_MAX)          \
    config->NAME = (String){0};
#define XX_RATIO(NAME, DEFAULT, EXPECTED_LEN)                             \
    config->NAME = (NcmInt32Array){0};
#define XX_TEXT_STYLE_LIST(NAME, DEFAULT)                                 \
    config->NAME = (NcmTextStyleArray){0};
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
config_init(Configuration *config) {
    if (config == NULL) {
        return;
    }

    config_init_unchecked(config);
    return;
}

void
config_destroy(Configuration *config) {
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
#define XX_TEXT_STYLE(NAME, DEFAULT)                                      \
    nc_text_style_destroy(&config->NAME);
#define XX_FORMAT(NAME, DEFAULT, FLAGS)                                   \
    ncm_format_ast_destroy(&config->NAME);
#define XX_BUFFER(NAME, DEFAULT, KEEP_EXISTING)                           \
    nc_buffer_destroy(&config->NAME);
#define XX_LOOK(NAME, DEFAULT, MIN_CHARS, MAX_CHARS, PAD_TO_MAX)          \
    sb_free(&config->NAME);
#define XX_RATIO(NAME, DEFAULT, EXPECTED_LEN)                             \
    ncm_int32_array_destroy(&config->NAME);
#define XX_TEXT_STYLE_LIST(NAME, DEFAULT)                                 \
    ncm_text_style_array_destroy(&config->NAME);
#define XX_LYRICS_FETCHERS(NAME, DEFAULT)                                 \
    ncm_lyrics_fetcher_registry_destroy(&config->NAME);
#define XX_SCREEN_LIST(NAME, DEFAULT, PREVIOUS_FIELD)                     \
    screen_type_array_destroy(&config->NAME);                             \
    config->PREVIOUS_FIELD = false;
#define XX_COLUMNS(NAME, DEFAULT, FORMAT_FIELD)                           \
    ncm_format_ast_destroy(&config->FORMAT_FIELD);                        \
    column_array_destroy(&config->NAME);

#include "config_options_pass.h"

    config_init_unchecked(config);

    return;
}

double
config_locked_screen_width_fraction(Configuration *config) {
    ASSERT(config != NULL);
    return config->locked_screen_width_part / 100.0;
}

enum SearchEngineSearchMode
config_search_engine_default_mode(Configuration *config) {
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
