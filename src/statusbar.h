#if !defined(NCMPCPP_STATUSBAR_H)
#define NCMPCPP_STATUSBAR_H

#include <stdbool.h>

#include "c/ncm_base.h"
#include "c/ncm_string_format.h"
#include "curses/nc_window.h"

NCM_EXTERN_C_BEGIN

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

NCM_EXTERN_C_END

#if defined(__cplusplus)

#include <string>
#include <utility>

#include "curses/window.h"
#include "ui_state.h"
#include "utility/string_format.h"

namespace Progressbar {

struct ScopedLock {
    ScopedLock() noexcept {
        ncm_progressbar_scoped_lock_init(nullptr);
    }

    ~ScopedLock() noexcept {
        ncm_progressbar_scoped_lock_destroy(nullptr);
    }
};

}

namespace Statusbar {

struct ScopedLock {
    ScopedLock() noexcept {
        ncm_statusbar_scoped_lock_init(nullptr);
    }

    ~ScopedLock() noexcept {
        ncm_statusbar_scoped_lock_destroy(nullptr);
    }
};

inline NC::Window &
put() {
    ncm_statusbar_put();
    return *static_cast<NC::Window *>(ui_state_footer_legacy_window());
}

inline void
print(int delay, const std::string &message) {
    ncm_statusbar_print(delay, const_cast<char *>(message.data()),
                        static_cast<int32>(message.size()));
}

inline void
print(const std::string &message) {
    print(ncm_statusbar_message_delay_time(), message);
}

template <typename FormatT, typename... Args>
void
printf(FormatT &&fmt, Args&&... args) {
    print(ncm_statusbar_message_delay_time(),
          stringFormat(std::forward<FormatT>(fmt),
                       std::forward<Args>(args)...));
}

}

#endif

#endif /* NCMPCPP_STATUSBAR_H */
