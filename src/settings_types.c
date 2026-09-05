#if !defined(NCMPCPP_SETTINGS_TYPES_C)
#define NCMPCPP_SETTINGS_TYPES_C

#include "cbase.h"

#include "c/ncm_c.h"
#include "settings.h"

static NcmArrayItemCallbacks settings_no_callbacks = {0};

static void settings_formatted_color_array_destroy_item(void *item);

static NcmArrayItemCallbacks settings_formatted_color_callbacks = {
    .destroy = settings_formatted_color_array_destroy_item,
};

static void *
settings_array_reserve_append(void *items, int32 len, int32 *cap,
                              int64 item_size) {
    int64 needed = (int64)len + 1;
    int32 old_cap;
    int32 new_cap;

    if (needed <= *cap) {
        return items;
    }
    if (needed >= MAXOF(*cap)) {
        error("Array only supports fewer than 2GB items.\n");
        fatal(EXIT_FAILURE);
    }

    old_cap = *cap;
    new_cap = *cap;
    if (new_cap <= 0) {
        new_cap = 8;
    }
    if (needed >= MAXOF(new_cap)/2) {
        new_cap = (int32)needed;
    } else {
        while (new_cap < needed) {
            new_cap *= 2;
        }
    }

    items = realloc2(items, old_cap, new_cap, item_size);
    *cap = new_cap;
    return items;
}

NCM_ARRAY_DEFINE_CLEAR(screen_type_array, ScreenTypeArray,
                       &settings_no_callbacks)
NCM_ARRAY_DEFINE_DESTROY(screen_type_array, ScreenTypeArray)

NCM_ARRAY_DEFINE_CLEAR(ncm_int32_array, NcmInt32Array, &settings_no_callbacks)
NCM_ARRAY_DEFINE_DESTROY(ncm_int32_array, NcmInt32Array)

int32 *
ncm_int32_array_append(NcmInt32Array *array) {
    int32 *item;

    array->items = settings_array_reserve_append(
        array->items, array->len, &array->cap, SIZEOF(*array->items));
    item = &array->items[array->len];
    array->len += 1;
    *item = 0;
    return item;
}

NCM_ARRAY_DEFINE_CLEAR(ncm_formatted_color_array, NcmFormattedColorArray,
                       &settings_formatted_color_callbacks)
NCM_ARRAY_DEFINE_DESTROY(ncm_formatted_color_array, NcmFormattedColorArray)

NcFormattedColor *
ncm_formatted_color_array_append(NcmFormattedColorArray *array) {
    NcFormattedColor *item;

    array->items = settings_array_reserve_append(
        array->items, array->len, &array->cap, SIZEOF(*array->items));
    item = &array->items[array->len];
    array->len += 1;
    nc_formatted_color_init(item);
    return item;
}

void
column_init(Column *column) {
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

void
column_array_clear(ColumnArray *array) {
    for (int32 i = 0; i < array->len; i += 1) {
        Column *column = &array->items[i];

        stupid_string_free(&column->name, &column->name_len,
                                &column->name_cap);
        stupid_string_free(&column->type, &column->type_len,
                                &column->type_cap);
        column_init(column);
    }
    array->len = 0;
    return;
}

Column *
column_array_append(ColumnArray *array) {
    Column *column;

    array->items = settings_array_reserve_append(
        array->items, array->len, &array->cap, SIZEOF(*array->items));
    column = &array->items[array->len];
    array->len += 1;
    column_init(column);
    return column;
}

enum ScreenType *
screen_type_array_append(ScreenTypeArray *array) {
    enum ScreenType *screen_type;

    array->items = settings_array_reserve_append(
        array->items, array->len, &array->cap, SIZEOF(*array->items));
    screen_type = &array->items[array->len];
    array->len += 1;
    *screen_type = NCM_SCREEN_TYPE_PLAYLIST;
    return screen_type;
}

static void
settings_formatted_color_array_destroy_item(void *item) {
    ASSERT(item != NULL);
    nc_formatted_color_destroy(item);
    return;
}

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
    column_array_clear(&config->NAME); \
    free2(config->NAME.items, \
          config->NAME.cap*SIZEOF(*config->NAME.items)); \
    config->NAME = (ColumnArray){0};
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
