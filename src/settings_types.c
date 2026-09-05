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

void
screen_type_array_clear(ScreenTypeArray *array) {
    array->len = 0;
    return;
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
#define XX_COLOR(NAME, DEFAULT_VALUE) \
    config->NAME = nc_color_default();
#define XX_FORMATTED_COLOR(NAME, DEFAULT_VALUE) \
    nc_formatted_color_init(&config->NAME);
#define XX_BORDER(NAME, DEFAULT_VALUE) \
    config->NAME = nc_border_none();
#define XX_FORMAT(NAME, DEFAULT_VALUE, FLAGS) \
    config->NAME = (NcmFormatAst){0};
#include "configuration_options.def"
#undef XX_FORMAT
#undef XX_BORDER
#undef XX_FORMATTED_COLOR
#undef XX_COLOR
#undef XX_ENUM
#undef XX_DOUBLE_RANGE
#undef XX_INT_RANGE
#undef XX_DIR
#undef XX_PATH
#undef XX_STRING
#undef XX_BOOL

    config->visualizer_fifo_path = NULL;
    config->visualizer_fifo_path_len = 0;

    config->progressbar_look = (StrBuilder){0};
    config->visualizer_look = (StrBuilder){0};

    config->browser_playlist_prefix = (NcBuffer){0};
    config->selected_item_prefix = (NcBuffer){0};
    config->selected_item_suffix = (NcBuffer){0};
    config->now_playing_prefix = (NcBuffer){0};
    config->now_playing_suffix = (NcBuffer){0};
    config->modified_item_prefix = (NcBuffer){0};
    config->current_item_prefix = (NcBuffer){0};
    config->current_item_suffix = (NcBuffer){0};
    config->current_item_inactive_column_prefix = (NcBuffer){0};
    config->current_item_inactive_column_suffix = (NcBuffer){0};

    config->song_columns_mode_format = (NcmFormatAst){0};

    config->playlist_editor_column_width_ratio = (NcmInt32Array){0};
    config->media_library_column_width_ratio_two = (NcmInt32Array){0};
    config->media_library_column_width_ratio_three = (NcmInt32Array){0};
    config->song_columns_list_format = (ColumnArray){0};
    config->visualizer_color = (NcmFormattedColorArray){0};
    config->screen_switcher_mode = (ScreenTypeArray){0};
    config->lyrics_fetchers = (NcmLyricsFetcherRegistry){0};

    config->screen_switcher_previous = false;
    config->default_find_mode = false;
    config->default_place_to_search_in = false;
    config->has_startup_slave_screen_type = false;

    config->lyrics_db = 0;
    config->regular_expressions = NCM_REGEX_EXTENDED_CASE_INSENSITIVE;

    config->selected_item_prefix_length = 0;
    config->selected_item_suffix_length = 0;
    config->now_playing_prefix_length = 0;
    config->now_playing_suffix_length = 0;
    config->current_item_prefix_length = 0;
    config->current_item_suffix_length = 0;
    config->current_item_inactive_column_prefix_length = 0;
    config->current_item_inactive_column_suffix_length = 0;

    config->startup_slave_screen = NCM_SCREEN_TYPE_COUNT;
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
#define XX_COLOR(NAME, DEFAULT_VALUE)
#define XX_FORMATTED_COLOR(NAME, DEFAULT_VALUE) \
    nc_formatted_color_destroy(&config->NAME);
#define XX_BORDER(NAME, DEFAULT_VALUE)
#define XX_FORMAT(NAME, DEFAULT_VALUE, FLAGS) \
    ncm_format_ast_destroy(&config->NAME);
#include "configuration_options.def"
#undef XX_FORMAT
#undef XX_BORDER
#undef XX_FORMATTED_COLOR
#undef XX_COLOR
#undef XX_ENUM
#undef XX_DOUBLE_RANGE
#undef XX_INT_RANGE
#undef XX_DIR
#undef XX_PATH
#undef XX_STRING
#undef XX_BOOL

    free2(config->visualizer_fifo_path, config->visualizer_fifo_path_len + 1);
    config->visualizer_fifo_path = NULL;
    config->visualizer_fifo_path_len = 0;

    sb_free(&config->progressbar_look);
    sb_free(&config->visualizer_look);

    nc_buffer_destroy(&config->browser_playlist_prefix);
    nc_buffer_destroy(&config->selected_item_prefix);
    nc_buffer_destroy(&config->selected_item_suffix);
    nc_buffer_destroy(&config->now_playing_prefix);
    nc_buffer_destroy(&config->now_playing_suffix);
    nc_buffer_destroy(&config->modified_item_prefix);
    nc_buffer_destroy(&config->current_item_prefix);
    nc_buffer_destroy(&config->current_item_suffix);
    nc_buffer_destroy(&config->current_item_inactive_column_prefix);
    nc_buffer_destroy(&config->current_item_inactive_column_suffix);

    ncm_format_ast_destroy(&config->song_columns_mode_format);

    ncm_int32_array_destroy(&config->playlist_editor_column_width_ratio);
    ncm_int32_array_destroy(&config->media_library_column_width_ratio_two);
    ncm_int32_array_destroy(&config->media_library_column_width_ratio_three);
    column_array_clear(&config->song_columns_list_format);
    free2(config->song_columns_list_format.items,
          config->song_columns_list_format.cap
              *SIZEOF(*config->song_columns_list_format.items));
    config->song_columns_list_format = (ColumnArray){0};
    ncm_formatted_color_array_clear(&config->visualizer_color);

    free2(config->visualizer_color.items,
          config->visualizer_color.cap
              *SIZEOF(*config->visualizer_color.items));

    config->visualizer_color = (NcmFormattedColorArray){0};
    free2(config->screen_switcher_mode.items,
          config->screen_switcher_mode.cap
              *SIZEOF(*config->screen_switcher_mode.items));
    config->screen_switcher_mode = (ScreenTypeArray){0};
    ncm_lyrics_fetcher_registry_destroy(&config->lyrics_fetchers);

    configuration_init_unchecked(config);

    return;
}

void
configuration_clear(Configuration *config) {
    configuration_destroy(config);
    return;
}

#endif /* NCMPCPP_SETTINGS_TYPES_C */
