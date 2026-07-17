#if !defined(NCMPCPP_TITLE_C)
#define NCMPCPP_TITLE_C

#include "title.h"

#include <stdio.h>
#include <string.h>

#include "app_controller.h"
#include "cbase/utf8.c"
#include "cbase/util.c"
#include "global.h"
#include "settings.h"
#include "ui_state.h"

static char *
title_current_screen_title(void) {
    NcScreen *screen;
    char *title;

    screen = app_controller_current_screen();
    if (screen == NULL) {
        return "";
    }

    title = nc_screen_title(screen);
    if (title == NULL) {
        return "";
    }

    return title;
}

static void
title_apply_formatted_color(NcWindow *window, NcFormattedColor *color) {
    enum NcFormat *formats;
    int32 count;

    if (color == NULL) {
        return;
    }

    nc_window_push_color(window, color->color);
    formats = nc_formatted_color_formats(color);
    count = nc_formatted_color_format_count(color);
    for (int32 i = 0; i < count; i += 1) {
        nc_window_apply_format(window, formats[i]);
    }
    return;
}

static void
title_apply_formatted_color_end(NcWindow *window,
                                NcFormattedColor *color) {
    enum NcFormat *formats;
    int32 count;

    if (color == NULL) {
        return;
    }

    if (!nc_color_is_default(color->color)) {
        nc_window_push_color(window, nc_color_end());
    }
    formats = nc_formatted_color_formats(color);
    count = nc_formatted_color_format_count(color);
    for (int32 i = count - 1; i >= 0; i -= 1) {
        nc_window_apply_format(window, nc_format_reverse(formats[i]));
    }
    return;
}

static void
title_draw_classic(NcWindow *window, char *title, int32 title_len,
                   NcFormattedColor *volume_color) {
    int32 volume_x;

    nc_window_go_to_xy(window, 0, 0);
    nc_window_apply_term_manip(window, NC_TERM_CLEAR_TO_EOL);
    nc_window_apply_format(window, NC_FORMAT_BOLD);
    nc_window_print_data(window, title, title_len);
    nc_window_apply_format(window, NC_FORMAT_NO_BOLD);

    volume_x = (int32)(nc_window_width(window) - global_volume_state_len());
    if (volume_x < 0) {
        volume_x = 0;
    }
    nc_window_go_to_xy(window, volume_x, 0);
    title_apply_formatted_color(window, volume_color);
    nc_window_print_data(window, global_volume_state_cstr(),
                         global_volume_state_len());
    title_apply_formatted_color_end(window, volume_color);
    return;
}

static void
title_draw_alternative(NcWindow *window, char *title, int32 title_len,
                       NcFormattedColor *separator_color) {
    int32 title_x;
    int32 title_width;

    nc_window_go_to_xy(window, 0, 3);
    nc_window_apply_term_manip(window, NC_TERM_CLEAR_TO_EOL);
    title_apply_formatted_color(window, separator_color);
    mvwhline(nc_window_raw(window), 2, 0, 0, COLS);
    mvwhline(nc_window_raw(window), 4, 0, 0, COLS);
    title_apply_formatted_color_end(window, separator_color);

    title_width = utf8_width(title, title_len);
    title_x = (COLS - title_width)/2;
    nc_window_go_to_xy(window, title_x, 3);
    nc_window_apply_format(window, NC_FORMAT_BOLD);
    nc_window_print_data(window, title, title_len);
    nc_window_apply_format(window, NC_FORMAT_NO_BOLD);
    return;
}

void
ncm_window_title_write(char *title, int32 title_len) {
    printf("\033]0;");
    if ((title != NULL) && (title_len > 0)) {
        fwrite(title, 1, (size_t)title_len, stdout);
    }
    printf("\7");
    fflush(stdout);
    return;
}

void
ncm_window_title_set(char *title, int32 title_len) {
    if (!Config.set_window_title) {
        return;
    }

    ncm_window_title_write(title, title_len);
    return;
}

void
ncm_window_title_set_cstring(char *title) {
    ncm_window_title_set(title, optional_strlen32(title));
    return;
}

void
ncm_title_draw_header_with_config(char *title, int32 title_len,
                                  bool header_visibility,
                                  enum Design design,
                                  NcFormattedColor *volume_color,
                                  NcFormattedColor *separator_color) {
    NcWindow *window;

    if (!header_visibility) {
        return;
    }

    window = ui_state_header_window();
    if ((window == NULL) || (window->window == NULL)) {
        return;
    }

    if (title == NULL) {
        title = "";
        title_len = 0;
    }

    switch (design) {
    case NCM_DESIGN_CLASSIC:
        title_draw_classic(window, title, title_len, volume_color);
        break;
    case NCM_DESIGN_ALTERNATIVE:
        title_draw_alternative(window, title, title_len, separator_color);
        break;
    case NCM_DESIGN_LAST:
        break;
    }

    nc_window_refresh(window);
    return;
}

void
ncm_title_draw_header(char *title, int32 title_len) {
    ncm_title_draw_header_with_config(title, title_len,
                                      Config.header_visibility,
                                      Config.design,
                                      &Config.volume_color,
                                      &Config.alternative_ui_separator_color);
    return;
}

void
ncm_title_draw_current_header(void) {
    char *title;

    title = title_current_screen_title();
    ncm_title_draw_header(title, optional_strlen32(title));
    return;
}

#endif /* NCMPCPP_TITLE_C */
