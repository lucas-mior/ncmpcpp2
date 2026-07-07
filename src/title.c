#include "title.h"

#include <stdio.h>
#include <string.h>

#include "global.h"
#include "settings.h"
#include "ui_state.h"

static int32
title_cstring_len(char *string) {
    if (string == NULL) {
        return 0;
    }

    return (int32)strlen((const char *)string);
}

void
ncm_window_title_set(char *title, int32 title_len) {
    if (!Config.set_window_title) {
        return;
    }

    printf("\033]0;");
    if ((title != NULL) && (title_len > 0)) {
        fwrite(title, 1, (size_t)title_len, stdout);
    }
    printf("\7");
    fflush(stdout);
    return;
}

void
ncm_window_title_set_cstring(char *title) {
    ncm_window_title_set(title, title_cstring_len(title));
    return;
}

void
ncm_title_draw_header(char *title, int32 title_len) {
    NcWindow *window;
    int64 width;
    int32 volume_x;

    if (!Config.header_visibility) {
        return;
    }

    window = (NcWindow *)ui_state_header_window();
    if (window == NULL) {
        return;
    }

    width = nc_window_width(window);
    nc_window_go_to_xy(window, 0, 0);
    nc_window_apply_term_manip(window, NC_TERM_CLEAR_TO_EOL);
    nc_window_apply_format(window, NC_FORMAT_BOLD);
    nc_window_print_data(window, title, title_len);
    nc_window_apply_format(window, NC_FORMAT_NO_BOLD);

    volume_x = (int32)(width - global_volume_state_len());
    if (volume_x > 0) {
        nc_window_go_to_xy(window, volume_x, 0);
        nc_window_print_data(window, global_volume_state_cstr(),
                             global_volume_state_len());
    }
    nc_window_refresh(window);
    return;
}
