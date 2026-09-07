#if !defined(STATUSBAR_H)
#define STATUSBAR_H

#include "cbase.h"

#include "c/ncm_c.h"
#include "curses/nc_curses.h"

typedef struct NcmStatusbarScopedLock {
    bool locked_statusbar;
    bool locked_progressbar;
} NcmStatusbarScopedLock;

void ncm_progressbar_scoped_lock_init(NcmStatusbarScopedLock *);
void ncm_progressbar_scoped_lock_destroy(NcmStatusbarScopedLock *);
bool ncm_progressbar_is_unlocked(void);
void ncm_progressbar_draw(int32 elapsed, int32 time);

void ncm_statusbar_scoped_lock_init(NcmStatusbarScopedLock *);
void ncm_statusbar_scoped_lock_destroy(NcmStatusbarScopedLock *);
bool ncm_statusbar_is_unlocked(void);
void ncm_statusbar_try_redraw(void);
NcWindow *ncm_statusbar_put(void);
void ncm_statusbar_print(int32 delay_seconds, char *, int32 message_len);
void ncm_statusbar_print_cstring(int32, char *);
void ncm_statusbar_mpd_idle_callback(void);
bool ncm_statusbar_prompt_should_continue(char *, int32);
int32 ncm_statusbar_prompt_return_one_of(NcWindow *, char *values, int32,
                                         char *result);
int32 ncm_statusbar_message_delay_time(void);

#endif /* STATUSBAR_H */
