#if !defined(NCMPCPP_NC_FORMATTED_COLOR_H)
#define NCMPCPP_NC_FORMATTED_COLOR_H

#include "curses/nc_window.h"

typedef struct NcFormattedColor {
    enum NcFormat *formats;
    NcColor color;
} NcFormattedColor;

void nc_formatted_color_init(NcFormattedColor *formatted_color);
void nc_formatted_color_init_color(NcFormattedColor *formatted_color,
                                   NcColor color);
void nc_formatted_color_copy(NcFormattedColor *dest,
                             NcFormattedColor *source);
void nc_formatted_color_move(NcFormattedColor *dest,
                             NcFormattedColor *source);
void nc_formatted_color_destroy(NcFormattedColor *formatted_color);
void nc_formatted_color_add_format(NcFormattedColor *formatted_color,
                                   enum NcFormat format);
enum NcFormat *nc_formatted_color_formats(NcFormattedColor *formatted_color);
int32 nc_formatted_color_format_count(NcFormattedColor *formatted_color);

#endif /* NCMPCPP_NC_FORMATTED_COLOR_H */
