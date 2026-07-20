#if !defined(NCMPCPP_STATUSBAR_C)
#define NCMPCPP_STATUSBAR_C

#include "statusbar.h"

#include <string.h>

#include "bindings.h"
#include "cbase/utf8.c"
#include "cbase/base_macros.h"
#include "cbase/util.c"
#include "global.h"
#include "settings.h"
#include "status.h"
#include "ui_state.h"

static bool progressbar_block_update;
static bool statusbar_block_update;
static bool statusbar_allow_unlock = true;
static NcmTimePoint statusbar_lock_time;
static int32 statusbar_lock_delay_seconds = -1;

static NcWindow *
statusbar_footer_window(void) {
    return ui_state_footer_window();
}

static void
statusbar_set_active_footer_line_locked(bool locked) {
    if (Config.statusbar_visibility) {
        statusbar_block_update = locked;
    } else {
        progressbar_block_update = locked;
    }
    return;
}

static void
statusbar_redraw_after_stop_unlock(void) {
    NcWindow *window;

    if (ncm_status_state_player() != NCM_STATUS_PLAYER_STOP) {
        return;
    }

    window = statusbar_footer_window();
    if (window == NULL) {
        return;
    }

    switch (Config.design) {
    case NCM_DESIGN_CLASSIC:
        (void)ncm_statusbar_put();
        break;
    case NCM_DESIGN_ALTERNATIVE:
        ncm_progressbar_draw(ncm_status_state_elapsed_time(),
                             ncm_status_state_total_time());
        break;
    case NCM_DESIGN_LAST:
        break;
    default:
        break;
    }
    nc_window_refresh(window);
    return;
}

static void
statusbar_redraw_after_unlock(void) {
    NcWindow *window;

    if (statusbar_block_update || progressbar_block_update) {
        return;
    }

    window = statusbar_footer_window();
    if (window == NULL) {
        return;
    }

    switch (Config.design) {
    case NCM_DESIGN_CLASSIC:
        switch (ncm_status_state_player()) {
        case NCM_STATUS_PLAYER_UNKNOWN:
        case NCM_STATUS_PLAYER_STOP:
            (void)ncm_statusbar_put();
            break;
        case NCM_STATUS_PLAYER_PLAY:
        case NCM_STATUS_PLAYER_PAUSE:
            ncm_status_changes_elapsed_time(false);
            break;
        default:
            break;
        }
        break;
    case NCM_DESIGN_ALTERNATIVE:
        ncm_progressbar_draw(ncm_status_state_elapsed_time(),
                             ncm_status_state_total_time());
        break;
    case NCM_DESIGN_LAST:
        break;
    default:
        break;
    }
    nc_window_refresh(window);
    return;
}

static void
statusbar_apply_formatted_color(NcWindow *window, NcFormattedColor *color) {
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
statusbar_apply_formatted_color_end(NcWindow *window, NcFormattedColor *color) {
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
statusbar_progressbar_split(NcmStringView items[3]) {
    int32 byte;

    byte = 0;
    for (int32 i = 0; i < 3; i += 1) {
        int32 next;

        items[i].data = "";
        items[i].len = 0;
        if (byte >= Config.progressbar.len) {
            continue;
        }

        next = utf8_next_position(Config.progressbar.data,
                                  Config.progressbar.len, byte);
        items[i].data = Config.progressbar.data + byte;
        items[i].len = next - byte;
        byte = next;
    }
    return;
}

void
ncm_progressbar_scoped_lock_init(NcmStatusbarScopedLock *lock) {
    if (lock) {
        lock->locked_progressbar = true;
        lock->locked_statusbar = false;
    }
    progressbar_block_update = true;
    return;
}

void
ncm_progressbar_scoped_lock_destroy(NcmStatusbarScopedLock *lock) {
    (void)lock;
    progressbar_block_update = false;
    return;
}

bool
ncm_progressbar_is_unlocked(void) {
    return !progressbar_block_update;
}

void
ncm_progressbar_draw(int32 elapsed, int32 time) {
    NcmStringView progressbar[3];
    NcWindow *window;
    int32 width;
    int32 filled;
    int64 howlong;

    window = statusbar_footer_window();
    if (window == NULL) {
        return;
    }

    width = (int32)nc_window_width(window);
    if (width <= 0) {
        return;
    }

    howlong = 0;
    if (time != 0) {
        howlong = (width*elapsed) / time;
    }
    if (howlong > (int64)width) {
        filled = width;
    } else {
        filled = (int32)howlong;
    }

    statusbar_progressbar_split(progressbar);
    nc_window_go_to_xy(window, 0, 0);
    statusbar_apply_formatted_color(window, &Config.progressbar_color);
    if ((progressbar[2].len > 0) && (progressbar[2].data[0] != '\0')) {
        for (int32 i = 0; i < width; i += 1) {
            nc_window_print_data(window, progressbar[2].data,
                                 progressbar[2].len);
        }
        nc_window_go_to_xy(window, 0, 0);
    } else {
        mvwhline(nc_window_raw(window), 0, 0, 0, width);
        nc_window_go_to_xy(window, 0, 0);
    }
    statusbar_apply_formatted_color_end(window, &Config.progressbar_color);

    if (time != 0) {
        statusbar_apply_formatted_color(window,
                                        &Config.progressbar_elapsed_color);
        for (int32 i = 0; i < filled; i += 1) {
            nc_window_print_data(window, progressbar[0].data,
                                 progressbar[0].len);
        }
        if (howlong < (int64)width) {
            nc_window_print_data(window, progressbar[1].data,
                                 progressbar[1].len);
        }
        statusbar_apply_formatted_color_end(window,
                                            &Config.progressbar_elapsed_color);
    }
    nc_window_go_to_xy(window, 0, 0);
    return;
}

void
ncm_statusbar_scoped_lock_init(NcmStatusbarScopedLock *lock) {
    if (lock) {
        lock->locked_statusbar = Config.statusbar_visibility;
        lock->locked_progressbar = !Config.statusbar_visibility;
    }

    if (Config.statusbar_visibility) {
        statusbar_block_update = true;
    } else {
        progressbar_block_update = true;
    }
    statusbar_allow_unlock = false;
    return;
}

void
ncm_statusbar_scoped_lock_destroy(NcmStatusbarScopedLock *lock) {
    (void)lock;
    statusbar_allow_unlock = true;
    if (statusbar_lock_delay_seconds < 0) {
        statusbar_set_active_footer_line_locked(false);
    }
    statusbar_redraw_after_stop_unlock();
    return;
}

bool
ncm_statusbar_is_unlocked(void) {
    return !statusbar_block_update;
}

void
ncm_statusbar_try_redraw(void) {
    if ((statusbar_lock_delay_seconds > 0)
        && (global_timer_elapsed_seconds(statusbar_lock_time)
            > statusbar_lock_delay_seconds)) {
        statusbar_lock_delay_seconds = -1;
        statusbar_set_active_footer_line_locked(!statusbar_allow_unlock);
        statusbar_redraw_after_unlock();
    }
    return;
}

NcWindow *
ncm_statusbar_put(void) {
    NcWindow *window;
    int32 y;

    window = statusbar_footer_window();
    if (window == NULL) {
        return NULL;
    }

    y = 0;
    if (Config.statusbar_visibility) {
        y = 1;
    }
    nc_window_go_to_xy(window, 0, y);
    nc_window_apply_term_manip(window, NC_TERM_CLEAR_TO_EOL);
    nc_window_go_to_xy(window, 0, y);
    return window;
}

void
ncm_statusbar_print(int32 delay_seconds, char *message, int32 message_len) {
    NcWindow *window;
    int32 y;

    if (!statusbar_allow_unlock) {
        return;
    }

    window = statusbar_footer_window();
    if (window == NULL) {
        return;
    }

    if (delay_seconds != 0) {
        statusbar_lock_time = global_timer;
        statusbar_lock_delay_seconds = delay_seconds;
        if (Config.statusbar_visibility) {
            statusbar_block_update = true;
        } else {
            progressbar_block_update = true;
        }
    }

    y = 0;
    if (Config.statusbar_visibility) {
        y = 1;
    }
    nc_window_go_to_xy(window, 0, y);
    nc_window_apply_term_manip(window, NC_TERM_CLEAR_TO_EOL);
    nc_window_print_data(window, message, message_len);
    nc_window_apply_term_manip(window, NC_TERM_CLEAR_TO_EOL);
    nc_window_go_to_xy(window, 0, y);
    nc_window_refresh(window);
    return;
}

void
ncm_statusbar_print_cstring(int32 delay_seconds, char *message) {
    ncm_statusbar_print(delay_seconds, message, optional_strlen32(message));
    return;
}

void
ncm_statusbar_format(int32 delay_seconds, char *format, int32 format_len,
                     NcmStringFormatArg *args, int32 args_len) {
    NcmBuffer buffer;

    ncm_buffer_init(&buffer);
    ncm_string_format_apply(&buffer, format, format_len, args, args_len);
    ncm_statusbar_print(delay_seconds, buffer.data, buffer.len);
    ncm_buffer_destroy(&buffer);
    return;
}

void
ncm_statusbar_mpd_idle_callback(void) {
    NcmError error;

    ncm_error_clear(&error);
    (void)ncm_status_update_from_noidle(&global_mpd, NULL, &error);
    return;
}

bool
ncm_statusbar_main_hook(char *string, int32 string_len) {
    NcmError error;

    (void)string;
    (void)string_len;
    ncm_error_clear(&error);
    ncm_status_trace(&global_mpd, true, false, &error);
    return true;
}

int32
ncm_statusbar_message_delay_time(void) {
    return Config.message_delay_time;
}

bool
ncm_statusbar_prompt_return_one_of(NcWindow *window, char *values,
                                   int32 values_len, char *result) {
    NcKey key;

    if ((window == NULL) || (values_len <= 0) || (result == NULL)) {
        return false;
    }

    while (true) {
        if (nc_window_raw(window)) {
            nc_window_refresh(window);
        }
        key = ncm_read_key(window);
        if ((key == NC_KEY_CTRL_C) || (key == NC_KEY_CTRL_G)) {
            return false;
        }

        for (int32 i = 0; i < values_len; i += 1) {
            if (key == (NcKey)values[i]) {
                *result = values[i];
                return true;
            }
        }
    }
}

#endif /* NCMPCPP_STATUSBAR_C */
