#include "statusbar.h"

#include <string.h>

#include "bindings.h"
#include "cbase/base_macros.h"
#include "global.h"
#include "settings.h"
#include "status.h"
#include "ui_state.h"

static bool progressbar_block_update;
static bool statusbar_block_update;
static bool statusbar_allow_unlock = true;
static NcmTimePoint statusbar_lock_time;
static int64 statusbar_lock_delay_seconds = -1;

static int32
statusbar_cstring_len(char *string) {
    if (string == NULL) {
        return 0;
    }

    return (int32)strlen((const char *)string);
}

static NcWindow *
statusbar_footer_window(void) {
    return ui_state_footer_window();
}

void
ncm_progressbar_scoped_lock_init(NcmStatusbarScopedLock *lock) {
    if (lock != NULL) {
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
ncm_progressbar_draw(uint32 elapsed, uint32 time) {
    NcWindow *window;
    int64 width;
    uint32 filled;

    window = statusbar_footer_window();
    if (window == NULL) {
        return;
    }

    width = nc_window_width(window);
    if (width <= 0) {
        return;
    }

    filled = 0;
    if (time != 0) {
        filled = (uint32)(((uint64)width*elapsed) / time);
    }
    if (filled > (uint32)width) {
        filled = (uint32)width;
    }

    nc_window_go_to_xy(window, 0, 0);
    nc_window_apply_term_manip(window, NC_TERM_CLEAR_TO_EOL);
    for (uint32 i = 0; i < filled; i += 1) {
        nc_window_print_char(window, '#');
    }
    nc_window_go_to_xy(window, 0, 0);
    return;
}

void
ncm_statusbar_scoped_lock_init(NcmStatusbarScopedLock *lock) {
    if (lock != NULL) {
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
        if (Config.statusbar_visibility) {
            statusbar_block_update = false;
        } else {
            progressbar_block_update = false;
        }
    }
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
        if (Config.statusbar_visibility) {
            statusbar_block_update = !statusbar_allow_unlock;
        } else {
            progressbar_block_update = !statusbar_allow_unlock;
        }
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

    if (delay_seconds > 0) {
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
    ncm_statusbar_print(delay_seconds, message,
                        statusbar_cstring_len(message));
    return;
}

void
ncm_statusbar_format(int32 delay_seconds,
                     char *format,
                     int32 format_len,
                     NcmStringFormatArg *args,
                     int32 args_len) {
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
    (void)ncm_status_update(&global_mpd, -1, &error);
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

bool
ncm_statusbar_prompt_return_one_of(NcWindow *window,
                                   char *values,
                                   int32 values_len,
                                   char *result) {
    NcKey key;

    if ((window == NULL) || (values_len <= 0) || (result == NULL)) {
        return false;
    }

    for (;;) {
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
