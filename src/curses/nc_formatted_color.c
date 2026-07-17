#if !defined(NCMPCPP_NC_FORMATTED_COLOR_C)
#define NCMPCPP_NC_FORMATTED_COLOR_C

#include "curses/nc_formatted_color.h"

#include "cbase/array.h"

void
nc_formatted_color_init(NcFormattedColor *formatted_color) {
    formatted_color->formats = NULL;
    formatted_color->color = nc_color_default();
    return;
}

void
nc_formatted_color_init_color(NcFormattedColor *formatted_color,
                              NcColor color) {
    nc_formatted_color_init(formatted_color);
    formatted_color->color = color;
    return;
}

void
nc_formatted_color_copy(NcFormattedColor *dest,
                        NcFormattedColor *source) {
    nc_formatted_color_init_color(dest, source->color);
    for (int32 i = 0; i < ARRAY_LEN(source->formats); i += 1) {
        ARRAY_PUSH(dest->formats, source->formats[i]);
    }
    return;
}

void
nc_formatted_color_move(NcFormattedColor *dest,
                        NcFormattedColor *source) {
    *dest = *source;
    nc_formatted_color_init(source);
    return;
}

void
nc_formatted_color_destroy(NcFormattedColor *formatted_color) {
    ARRAY_FREE(formatted_color->formats);
    nc_formatted_color_init(formatted_color);
    return;
}

void
nc_formatted_color_add_format(NcFormattedColor *formatted_color,
                              enum NcFormat format) {
    ARRAY_PUSH(formatted_color->formats, format);
    return;
}

enum NcFormat *
nc_formatted_color_formats(NcFormattedColor *formatted_color) {
    return formatted_color->formats;
}

int32
nc_formatted_color_format_count(NcFormattedColor *formatted_color) {
    return ARRAY_LEN(formatted_color->formats);
}

#endif /* NCMPCPP_NC_FORMATTED_COLOR_C */
