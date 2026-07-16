#if !defined(NCMPCPP_STATUSBAR_H)
#define NCMPCPP_STATUSBAR_H

#include <stdbool.h>

#include "c/ncm_base.h"
#include "c/ncm_string_format.h"
#include "curses/nc_window.h"

typedef struct NcmStatusbarScopedLock {
    bool locked_statusbar;
    bool locked_progressbar;
} NcmStatusbarScopedLock;

void ncm_progressbar_scoped_lock_init(NcmStatusbarScopedLock *lock);
void ncm_progressbar_scoped_lock_destroy(NcmStatusbarScopedLock *lock);
bool ncm_progressbar_is_unlocked(void);
void ncm_progressbar_draw(uint32 elapsed, uint32 time);

void ncm_statusbar_scoped_lock_init(NcmStatusbarScopedLock *lock);
void ncm_statusbar_scoped_lock_destroy(NcmStatusbarScopedLock *lock);
bool ncm_statusbar_is_unlocked(void);
void ncm_statusbar_try_redraw(void);
NcWindow *ncm_statusbar_put(void);
void ncm_statusbar_print(int32 delay_seconds,
                         char *message,
                         int32 message_len);
void ncm_statusbar_print_cstring(int32 delay_seconds, char *message);
void ncm_statusbar_format(int32 delay_seconds,
                          char *format,
                          int32 format_len,
                          NcmStringFormatArg *args,
                          int32 args_len);
void ncm_statusbar_mpd_noidle_callback(int32 flags, void *user);
void ncm_statusbar_mpd_idle_callback(void);
bool ncm_statusbar_main_hook(char *string, int32 string_len);
bool ncm_statusbar_prompt_return_one_of(NcWindow *window,
                                         char *values,
                                         int32 values_len,
                                         char *result);
int32 ncm_statusbar_message_delay_time(void);


#endif /* NCMPCPP_STATUSBAR_H */
