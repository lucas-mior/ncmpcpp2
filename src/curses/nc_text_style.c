#if !defined(NC_TEXT_STYLE_C)
#define NC_TEXT_STYLE_C

#include "cbase.h"
#include "ncmpcpp2.h"

#include "curses/nc_curses.h"

void
nc_text_style_init(NcTextStyle *text_style) {
    text_style->formats = NULL;
    text_style->color = nc_color_default();
    return;
}

void
nc_text_style_init_color(NcTextStyle *text_style,
                         NcColor color) {
    nc_text_style_init(text_style);
    text_style->color = color;
    return;
}

void
nc_text_style_copy(NcTextStyle *dest, NcTextStyle *source) {
    nc_text_style_init_color(dest, source->color);
    for (int32 i = 0; i < ARRAY_LEN(source->formats); i += 1) {
        ARRAY_PUSH(dest->formats, source->formats[i]);
    }
    return;
}

void
nc_text_style_move(NcTextStyle *dest, NcTextStyle *source) {
    *dest = *source;
    nc_text_style_init(source);
    return;
}

void
nc_text_style_destroy(NcTextStyle *text_style) {
    ARRAY_FREE(text_style->formats);
    nc_text_style_init(text_style);
    return;
}

void
nc_text_style_add_format(NcTextStyle *text_style,
                         enum NcFormat format) {
    ARRAY_PUSH(text_style->formats, format);
    return;
}

#endif /* NC_TEXT_STYLE_C */
