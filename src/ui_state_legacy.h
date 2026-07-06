#if !defined(NCMPCPP_UI_STATE_LEGACY_H)
#define NCMPCPP_UI_STATE_LEGACY_H

#include <cstddef>

#include "ui_state.h"

namespace NC {
struct Window;
}

inline NC::Window *ui_state_legacy_header_window()
{
    return static_cast<NC::Window *>(ui_state_header_window());
}

inline void ui_state_legacy_set_header_window(NC::Window *window)
{
    ui_state_set_header_window(window);
}

inline NC::Window *ui_state_legacy_footer_window()
{
    return static_cast<NC::Window *>(ui_state_footer_window());
}

inline void ui_state_legacy_set_footer_window(NC::Window *window)
{
    ui_state_set_footer_window(window);
}

inline std::size_t ui_state_legacy_screen_width()
{
    return static_cast<std::size_t>(ui_state_screen_width());
}

inline std::size_t ui_state_legacy_screen_height()
{
    return static_cast<std::size_t>(ui_state_screen_height());
}

inline void ui_state_legacy_set_screen_size(std::size_t width,
                                            std::size_t height)
{
    ui_state_set_screen_size(static_cast<int64>(width),
                             static_cast<int64>(height));
}

inline std::size_t ui_state_legacy_main_start_y()
{
    return static_cast<std::size_t>(ui_state_main_start_y());
}

inline void ui_state_legacy_set_main_start_y(std::size_t value)
{
    ui_state_set_main_start_y(static_cast<int64>(value));
}

inline std::size_t ui_state_legacy_main_height()
{
    return static_cast<std::size_t>(ui_state_main_height());
}

inline void ui_state_legacy_set_main_height(std::size_t value)
{
    ui_state_set_main_height(static_cast<int64>(value));
}

inline void ui_state_legacy_set_main_geometry(std::size_t start_y,
                                              std::size_t height)
{
    ui_state_set_main_geometry(static_cast<int64>(start_y),
                               static_cast<int64>(height));
}

#endif /* NCMPCPP_UI_STATE_LEGACY_H */
