/***************************************************************************
 *   Copyright (C) 2008-2021 by Andrzej Rybczak                            *
 *   andrzej@rybczak.net                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#ifndef NCMPCPP_SCREEN_COMPAT_IMPL_H
#define NCMPCPP_SCREEN_COMPAT_IMPL_H

#include <cassert>
#include <cstddef>

#include "app_controller.h"
#include "global.h"
#include "settings_legacy.h"
#include "ui_state.h"

namespace screen_compat {

inline enum NcScroll to_cpp_scroll(enum NcScroll where)
{
    switch (where)
    {
        case NC_SCROLL_UP:
            return NC_SCROLL_UP;
        case NC_SCROLL_DOWN:
            return NC_SCROLL_DOWN;
        case NC_SCROLL_PAGE_UP:
            return NC_SCROLL_PAGE_UP;
        case NC_SCROLL_PAGE_DOWN:
            return NC_SCROLL_PAGE_DOWN;
        case NC_SCROLL_HOME:
            return NC_SCROLL_HOME;
        case NC_SCROLL_END:
            return NC_SCROLL_END;
    }
    return NC_SCROLL_UP;
}

inline BaseScreen *legacy_owner(NcScreen *screen)
{
    if (screen == nullptr)
        return nullptr;
    return static_cast<BaseScreen *>(nc_screen_user(screen));
}

inline BaseScreen *previous_legacy_screen()
{
    return legacy_owner(app_controller_previous_screen());
}

void set_tab_previous_screen(BaseScreen *screen);

}

inline void drawSeparator(int x)
{
    color_set(Config.main_color.pairNumber(), nullptr);
    mvvline(static_cast<size_t>(ui_state_main_start_y()), x, 0,
            static_cast<size_t>(ui_state_main_height()));
    standend();
    refresh();
}

inline void genericMouseButtonPressed(NC::Window &w, MEVENT me)
{
    if (me.bstate & BUTTON5_PRESSED)
    {
        if (Config.mouse_list_scroll_whole_page)
            w.scroll(NC_SCROLL_PAGE_DOWN);
        else
            for (size_t i = 0; i < Config.lines_scrolled; ++i)
                w.scroll(NC_SCROLL_DOWN);
    }
    else if (me.bstate & BUTTON4_PRESSED)
    {
        if (Config.mouse_list_scroll_whole_page)
            w.scroll(NC_SCROLL_PAGE_UP);
        else
            for (size_t i = 0; i < Config.lines_scrolled; ++i)
                w.scroll(NC_SCROLL_UP);
    }
}

inline void scrollpadMouseButtonPressed(NC::Scrollpad &w, MEVENT me)
{
    if (me.bstate & BUTTON5_PRESSED)
    {
        for (size_t i = 0; i < Config.lines_scrolled; ++i)
            w.scroll(NC_SCROLL_DOWN);
    }
    else if (me.bstate & BUTTON4_PRESSED)
    {
        for (size_t i = 0; i < Config.lines_scrolled; ++i)
            w.scroll(NC_SCROLL_UP);
    }
}

inline BaseScreen::BaseScreen()
: hasToBeResized(false)
{
    nc_screen_init(&m_native_screen, makeNativeCallbacks(), this,
                   NC_SCREEN_TYPE_UNKNOWN);
}

inline BaseScreen::~BaseScreen()
{
    if (app_controller_is_screen_registered(&m_native_screen))
    {
        app_controller_unregister_screen(&m_native_screen);
    }
}

inline void BaseScreen::registerNativeScreen()
{
    NcScreen *screen = nativeScreen();

    nc_screen_set_type(screen, screen_type_to_native_type(type()));
    if (!app_controller_is_screen_registered(screen))
    {
        bool success = app_controller_register_screen(screen);
        assert(success);
        (void)success;
    }
}

inline BaseScreen *BaseScreen::legacyFromNativeScreen(NcScreen *screen)
{
    return fromNativeScreen(screen);
}

inline BaseScreen *BaseScreen::fromNativeScreen(NcScreen *screen)
{
    return screen_compat::legacy_owner(screen);
}

inline NcScreenCallbacks BaseScreen::makeNativeCallbacks()
{
    NcScreenCallbacks callbacks = {0};

    callbacks.active_window = nativeActiveWindowCallback;
    callbacks.refresh = nativeRefreshCallback;
    callbacks.refresh_window = nativeRefreshWindowCallback;
    callbacks.scroll = nativeScrollCallback;
    callbacks.switch_to = nativeSwitchToCallback;
    callbacks.resize = nativeResizeCallback;
    callbacks.window_timeout = nativeWindowTimeoutCallback;
    callbacks.title = nativeTitleCallback;
    callbacks.update = nativeUpdateCallback;
    callbacks.mouse_button_pressed = nativeMouseButtonPressedCallback;
    callbacks.is_lockable = nativeIsLockableCallback;
    callbacks.is_mergable = nativeIsMergableCallback;
    return callbacks;
}

inline NcWindow *BaseScreen::nativeActiveWindowCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner == nullptr || owner->activeWindow() == nullptr)
        return nullptr;
    return owner->activeWindow()->nativeWindow();
}

inline void BaseScreen::nativeRefreshCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner != nullptr)
        owner->refresh();
}

inline void BaseScreen::nativeRefreshWindowCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner != nullptr)
        owner->refreshWindow();
}

inline void BaseScreen::nativeScrollCallback(NcScreen *screen,
                                             enum NcScroll where)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner != nullptr)
        owner->scroll(screen_compat::to_cpp_scroll(where));
}

inline void BaseScreen::nativeSwitchToCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner == nullptr)
        return;
    if (app_controller_last_switch_changed_screen())
        screen_compat::set_tab_previous_screen(owner);
    syncLegacyScreenPointers();
}

inline void BaseScreen::nativeResizeCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner != nullptr)
        owner->resize();
}

inline int32 BaseScreen::nativeWindowTimeoutCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner == nullptr)
        return defaultWindowTimeout;
    return owner->windowTimeout();
}

inline char *BaseScreen::nativeTitleCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner == nullptr)
        return nullptr;
    owner->m_native_title_cache = owner->title();
    return const_cast<char *>(owner->m_native_title_cache.c_str());
}

inline void BaseScreen::nativeUpdateCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner != nullptr)
        owner->update();
}

inline void BaseScreen::nativeMouseButtonPressedCallback(NcScreen *screen,
                                                         MEVENT event)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner != nullptr)
        owner->mouseButtonPressed(event);
}

inline bool BaseScreen::nativeIsLockableCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner == nullptr)
        return false;
    return owner->isLockable();
}

inline bool BaseScreen::nativeIsMergableCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner == nullptr)
        return false;
    return owner->isMergable();
}

inline void BaseScreen::getWindowResizeParams(size_t &x_offset,
                                              size_t &width,
                                              bool adjust_locked_screen)
{
    NcScreen *locked_screen;
    NcScreen *inactive_screen;

    locked_screen = app_controller_locked_screen();
    inactive_screen = app_controller_inactive_screen();

    width = COLS;
    x_offset = 0;
    if (locked_screen && inactive_screen)
    {
        size_t locked_width = COLS*Config.locked_screen_width_part;
        if (locked_screen == nativeScreen())
            width = locked_width;
        else
        {
            width = COLS-locked_width-1;
            x_offset = locked_width+1;

            if (adjust_locked_screen)
            {
                nc_screen_resize(locked_screen);
                nc_screen_refresh(locked_screen);
                drawSeparator(x_offset-1);
            }
        }
    }
}

inline bool BaseScreen::lock()
{
    assert(app_controller_locked_screen() == nullptr);
    if (!app_controller_lock_current_screen())
        return false;
    syncLegacyScreenPointers();
    return true;
}

inline void BaseScreen::unlock()
{
    app_controller_unlock_screen();
    syncLegacyScreenPointers();
}

inline void applyToVisibleScreens(std::function<void(NcScreen *)> f)
{
    app_controller_each_visible_screen(
        [](NcScreen *screen, void *user) {
            auto *callback = static_cast<
                std::function<void(NcScreen *)> *>(user);
            (*callback)(screen);
        },
        &f);
}

inline void applyToVisibleWindows(std::function<void(BaseScreen *)> f)
{
    applyToVisibleScreens([&f](NcScreen *screen) {
        BaseScreen *owner = screen_compat::legacy_owner(screen);

        if (owner != nullptr)
            f(owner);
    });
}

inline void syncLegacyScreenPointers()
{
}

inline bool isVisible(BaseScreen *screen)
{
    assert(screen != 0);
    return app_controller_is_screen_visible(screen->nativeScreen());
}

#endif // NCMPCPP_SCREEN_COMPAT_IMPL_H
