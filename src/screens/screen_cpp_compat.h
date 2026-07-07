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

#ifndef NCMPCPP_SCREEN_CPP_COMPAT_H
#define NCMPCPP_SCREEN_CPP_COMPAT_H

#include <cassert>
#include <cstddef>
#include <functional>
#include <string>

#include "app_controller.h"
#include "global.h"
#include "settings_legacy.h"
#include "ui_state.h"
#include "curses/menu.h"
#include "curses/scrollpad.h"
#include "screens/nc_screen.h"
#include "screens/screen_switcher.h"
#include "screens/screen_type.h"

void drawSeparator(int x);
void genericMouseButtonPressed(NC::Window &w, MEVENT me);
void scrollpadMouseButtonPressed(NC::Scrollpad &w, MEVENT me);

/// An interface for various instantiations of Screen template class. Since C++ doesn't like
/// comparison of two different instantiations of the same template class we need the most
/// basic class to be non-template to allow it.
struct BaseScreen
{
	BaseScreen();
	virtual ~BaseScreen();

	/// @see Screen::isActiveWindow()
	virtual bool isActiveWindow(const NC::Window &w_) const = 0;

	/// @see Screen::activeWindow()
	virtual NC::Window *activeWindow() = 0;
	virtual const NC::Window *activeWindow() const = 0;

	/// @see Screen::refresh()
	virtual void refresh() = 0;

	/// @see Screen::refreshWindow()
	virtual void refreshWindow() = 0;

	/// @see Screen::scroll()
	virtual void scroll(enum NcScroll where) = 0;

	/// Method used for switching to screen
	virtual void switchTo() = 0;

	/// Method that should resize screen
	/// if requested by hasToBeResized
	virtual void resize() = 0;

	/// @return ncurses timeout parameter for the screen
	virtual int windowTimeout() = 0;

	/// @return title of the screen
	virtual std::string title() = 0;

	/// @return type of the screen
	virtual ScreenType type() = 0;

	/// If the screen contantly has to update itself
	/// somehow, it should be called by this function.
	virtual void update() = 0;

	/// @see Screen::mouseButtonPressed()
	virtual void mouseButtonPressed(MEVENT me) = 0;

	/// @return true if screen can be locked. Note that returning
	/// false here doesn't neccesarily mean that screen is also not
	/// mergable (eg. lyrics screen is not lockable since that wouldn't
	/// make much sense, but it's perfectly fine to merge it).
	virtual bool isLockable() = 0;

	/// @return true if screen is mergable, ie. can be "proper" subwindow
	/// if one of the screens is locked. Screens that somehow resemble popups
	/// will want to return false here.
	virtual bool isMergable() = 0;

	/// @return C screen backing this screen, or null if it is still C++ only.
	virtual NcScreen *nativeScreen() { return &m_native_screen; }
	virtual const NcScreen *nativeScreen() const { return &m_native_screen; }

	/// Legacy C++ owner used by compatibility code that still needs BaseScreen.
	static BaseScreen *legacyFromNativeScreen(NcScreen *screen);

	/// Registers this screen in the C screen registry.
	void registerNativeScreen();

	/// Locks current screen.
	/// @return true if lock was successful, false otherwise. Note that
	/// if there is already locked screen, it'll not overwrite it.
	bool lock();

	/// Should be set to true each time screen needs resize
	bool hasToBeResized;

	/// Unlocks a screen, ie. hides merged window (if there is one set).
	static void unlock();

	const static int defaultWindowTimeout = 500;

protected:
	/// Legacy C++ owner used by the compatibility NcScreen callbacks.
	static BaseScreen *fromNativeScreen(NcScreen *screen);

	/// Gets X offset and width of current screen to be used eg. in resize() function.
	/// @param adjust_locked_screen indicates whether this function should
	/// automatically adjust locked screen's dimensions (if there is one set)
	/// if current screen is going to be subwindow.
	void getWindowResizeParams(size_t &x_offset, size_t &width, bool adjust_locked_screen = true);

private:
	NcScreen m_native_screen;
	std::string m_native_title_cache;

	static NcScreenCallbacks makeNativeCallbacks();
	static NcWindow *nativeActiveWindowCallback(NcScreen *screen);
	static void nativeRefreshCallback(NcScreen *screen);
	static void nativeRefreshWindowCallback(NcScreen *screen);
	static void nativeScrollCallback(NcScreen *screen, enum NcScroll where);
	static void nativeSwitchToCallback(NcScreen *screen);
	static void nativeResizeCallback(NcScreen *screen);
	static int32 nativeWindowTimeoutCallback(NcScreen *screen);
	static char *nativeTitleCallback(NcScreen *screen);
	static void nativeUpdateCallback(NcScreen *screen);
	static void nativeMouseButtonPressedCallback(NcScreen *screen,
	                                            MEVENT event);
	static bool nativeIsLockableCallback(NcScreen *screen);
	static bool nativeIsMergableCallback(NcScreen *screen);
};

void applyToVisibleScreens(std::function<void(NcScreen *)> f);
void applyToVisibleWindows(std::function<void(BaseScreen *)> f);
void syncLegacyScreenPointers();
bool isVisible(BaseScreen *screen);

/// Class that all screens should derive from. It provides basic interface
/// for the screen to be working properly and assumes that we didn't forget
/// about anything vital.
///
template <typename WindowT> struct Screen : public BaseScreen
{
	typedef WindowT WindowType;
	typedef typename std::add_lvalue_reference<
		WindowType
	>::type WindowReference;
	typedef typename std::add_lvalue_reference<
		typename std::add_const<WindowType>::type
	>::type ConstWindowReference;

private:
	template <bool IsPointer, typename Result, typename ConstResult>
	struct getObject {
		static Result &apply(WindowReference w) { return w; }
		static ConstResult &constApply(ConstWindowReference w) { return w; }
	};
	template <typename Result, typename ConstResult>
	struct getObject<true, Result, ConstResult> {
		static Result &apply(WindowType w) { return *w; }
		static ConstResult &constApply(const WindowType w) { return *w; }
	};

	typedef getObject<
		std::is_pointer<WindowT>::value,
		typename std::remove_pointer<WindowT>::type,
		typename std::add_const<
			typename std::remove_pointer<WindowT>::type
		>::type
	> Accessor;

public:
	Screen() { }
	Screen(WindowT w_) : w(w_) { }

	virtual ~Screen() { }

	virtual bool isActiveWindow(const NC::Window &w_) const override {
		return &Accessor::constApply(w) == &w_;
	}

	/// Since some screens contain more that one window
	/// it's useful to determine the one that is being
	/// active
	/// @return address to window object cast to void *
	virtual NC::Window *activeWindow() override {
		return &Accessor::apply(w);
	}
	virtual const NC::Window *activeWindow() const override {
		return &Accessor::constApply(w);
	}

	/// Refreshes whole screen
	virtual void refresh() override {
		Accessor::apply(w).display();
	}

	/// Refreshes active window of the screen
	virtual void refreshWindow() override {
		Accessor::apply(w).display();
	}

	/// Scrolls the screen by given amount of lines and
	/// if fancy scrolling feature is disabled, enters the
	/// loop that holds main loop until user releases the key
	/// @param where indicates where one wants to scroll
	virtual void scroll(enum NcScroll where) override {
		Accessor::apply(w).scroll(where);
	}

	/// @return timeout parameter used for the screen (in ms)
	/// @default defaultWindowTimeout
	virtual int windowTimeout() override {
		return defaultWindowTimeout;
	}

	/// Invoked after there was one of mouse buttons pressed
	/// @param me struct that contains coords of where the click
	/// had its place and button actions
	virtual void mouseButtonPressed(MEVENT me) override {
		genericMouseButtonPressed(Accessor::apply(w), me);
	}

	/// @return currently active window
	WindowReference main() {
		return w;
	}
	ConstWindowReference main() const {
		return w;
	}

protected:
	/// Template parameter that should indicate the main type
	/// of window used by the screen. What is more, it should
	/// always be assigned to the currently active window (if
	/// acreen contains more that one)
	WindowT w;
};

/// Specialization for Screen<Scrollpad>::mouseButtonPressed that should
/// not scroll whole page, but rather a few lines (the number of them is
/// defined in the config)
template <> inline void Screen<NC::Scrollpad>::mouseButtonPressed(MEVENT me) {
	scrollpadMouseButtonPressed(w, me);
}

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
    if (nc_screen_switcher_finish_switch(screen))
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

#endif // NCMPCPP_SCREEN_CPP_COMPAT_H
