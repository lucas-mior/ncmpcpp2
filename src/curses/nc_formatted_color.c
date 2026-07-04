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
nc_formatted_color_clear(NcFormattedColor *formatted_color) {
    formatted_color->color = nc_color_default();
    ARRAY_CLEAR(formatted_color->formats);
    return;
}

void
nc_formatted_color_add_format(NcFormattedColor *formatted_color,
                              enum NcFormat format) {
    ARRAY_PUSH(formatted_color->formats, format);
    return;
}

NcColor
nc_formatted_color_color(NcFormattedColor *formatted_color) {
    return formatted_color->color;
}

enum NcFormat *
nc_formatted_color_formats(NcFormattedColor *formatted_color) {
    return formatted_color->formats;
}

int32
nc_formatted_color_format_count(NcFormattedColor *formatted_color) {
    return ARRAY_LEN(formatted_color->formats);
}

bool
nc_formatted_color_equal(NcFormattedColor *left,
                         NcFormattedColor *right) {
    int32 left_count;
    int32 right_count;

    if (!nc_color_equal(left->color, right->color)) {
        return false;
    }

    left_count = ARRAY_LEN(left->formats);
    right_count = ARRAY_LEN(right->formats);
    if (left_count != right_count) {
        return false;
    }

    for (int32 i = 0; i < left_count; i += 1) {
        if (left->formats[i] != right->formats[i]) {
            return false;
        }
    }

    return true;
}

bool
nc_formatted_color_can_hold_format(enum NcFormat format) {
    switch (format) {
    case NC_FORMAT_BOLD:
    case NC_FORMAT_UNDERLINE:
    case NC_FORMAT_REVERSE:
    case NC_FORMAT_ALT_CHARSET:
    case NC_FORMAT_ITALIC:
        return true;
    case NC_FORMAT_NO_BOLD:
    case NC_FORMAT_NO_UNDERLINE:
    case NC_FORMAT_NO_REVERSE:
    case NC_FORMAT_NO_ALT_CHARSET:
    case NC_FORMAT_NO_ITALIC:
        return false;
    }

    return false;
}
