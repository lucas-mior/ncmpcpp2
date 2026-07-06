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

#include <cassert>

#include "app_state.h"
#include "global.h"
#include "interfaces.h"
#include "screens/screen.h"
#include "settings.h"

using Global::myScreen;
using Global::myLockedScreen;
using Global::myInactiveScreen;

namespace {

NC::Scroll toCppScroll(enum NcScroll where)
{
    switch (where)
    {
        case NC_SCROLL_UP:
            return NC::Scroll::Up;
        case NC_SCROLL_DOWN:
            return NC::Scroll::Down;
        case NC_SCROLL_PAGE_UP:
            return NC::Scroll::PageUp;
        case NC_SCROLL_PAGE_DOWN:
            return NC::Scroll::PageDown;
        case NC_SCROLL_HOME:
            return NC::Scroll::Home;
        case NC_SCROLL_END:
            return NC::Scroll::End;
    }
    return NC::Scroll::Up;
}

int32 toNativeType(ScreenType type)
{
    switch (type)
    {
        case ScreenType::Browser:
            return NC_SCREEN_TYPE_BROWSER;
        case ScreenType::Help:
            return NC_SCREEN_TYPE_HELP;
        case ScreenType::Lastfm:
            return NC_SCREEN_TYPE_LASTFM;
        case ScreenType::Lyrics:
            return NC_SCREEN_TYPE_LYRICS;
        case ScreenType::MediaLibrary:
            return NC_SCREEN_TYPE_MEDIA_LIBRARY;
#ifdef ENABLE_OUTPUTS
        case ScreenType::Outputs:
            return NC_SCREEN_TYPE_OUTPUTS;
#endif // ENABLE_OUTPUTS
        case ScreenType::Playlist:
            return NC_SCREEN_TYPE_PLAYLIST;
        case ScreenType::PlaylistEditor:
            return NC_SCREEN_TYPE_PLAYLIST_EDITOR;
        case ScreenType::SearchEngine:
            return NC_SCREEN_TYPE_SEARCH_ENGINE;
        case ScreenType::SelectedItemsAdder:
            return NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER;
        case ScreenType::ServerInfo:
            return NC_SCREEN_TYPE_SERVER_INFO;
        case ScreenType::SongInfo:
            return NC_SCREEN_TYPE_SONG_INFO;
        case ScreenType::SortPlaylistDialog:
            return NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG;
#ifdef HAVE_TAGLIB_H
        case ScreenType::TagEditor:
            return NC_SCREEN_TYPE_TAG_EDITOR;
        case ScreenType::TinyTagEditor:
            return NC_SCREEN_TYPE_TINY_TAG_EDITOR;
#endif // HAVE_TAGLIB_H
#ifdef ENABLE_VISUALIZER
        case ScreenType::Visualizer:
            return NC_SCREEN_TYPE_VISUALIZER;
#endif // ENABLE_VISUALIZER
        case ScreenType::Unknown:
            return NC_SCREEN_TYPE_UNKNOWN;
    }
    return NC_SCREEN_TYPE_UNKNOWN;
}

BaseScreen *legacyOwner(NcScreen *screen)
{
    if (screen == nullptr)
        return nullptr;
    return static_cast<BaseScreen *>(nc_screen_user(screen));
}

BaseScreen *previousLegacyScreen()
{
    return legacyOwner(app_state_get_previous_screen());
}

void setTabPreviousScreen(BaseScreen *screen)
{
    BaseScreen *previous_screen = previousLegacyScreen();
    auto tab = dynamic_cast<Tabbable *>(screen);

    if (tab == nullptr)
        return;
    if (dynamic_cast<Tabbable *>(previous_screen) == nullptr)
        return;
    tab->setPreviousScreen(previous_screen);
}

}

void drawSeparator(int x)
{
    color_set(Config.main_color.pairNumber(), nullptr);
    mvvline(Global::MainStartY, x, 0, Global::MainHeight);
    standend();
    refresh();
}

void genericMouseButtonPressed(NC::Window &w, MEVENT me)
{
    if (me.bstate & BUTTON5_PRESSED)
    {
        if (Config.mouse_list_scroll_whole_page)
            w.scroll(NC::Scroll::PageDown);
        else
            for (size_t i = 0; i < Config.lines_scrolled; ++i)
                w.scroll(NC::Scroll::Down);
    }
    else if (me.bstate & BUTTON4_PRESSED)
    {
        if (Config.mouse_list_scroll_whole_page)
            w.scroll(NC::Scroll::PageUp);
        else
            for (size_t i = 0; i < Config.lines_scrolled; ++i)
                w.scroll(NC::Scroll::Up);
    }
}

void scrollpadMouseButtonPressed(NC::Scrollpad &w, MEVENT me)
{
    if (me.bstate & BUTTON5_PRESSED)
    {
        for (size_t i = 0; i < Config.lines_scrolled; ++i)
            w.scroll(NC::Scroll::Down);
    }
    else if (me.bstate & BUTTON4_PRESSED)
    {
        for (size_t i = 0; i < Config.lines_scrolled; ++i)
            w.scroll(NC::Scroll::Up);
    }
}

/***********************************************************************/

BaseScreen::BaseScreen()
: hasToBeResized(false)
{
    nc_screen_init(&m_native_screen, makeNativeCallbacks(), this,
                   NC_SCREEN_TYPE_UNKNOWN);
}

BaseScreen::~BaseScreen()
{
    if (app_state_is_screen_registered(&m_native_screen))
    {
        app_state_unregister_screen(&m_native_screen);
    }
}

void BaseScreen::registerNativeScreen()
{
    NcScreen *screen = nativeScreen();

    nc_screen_set_type(screen, toNativeType(type()));
    if (!app_state_is_screen_registered(screen))
    {
        bool success = app_state_register_screen(screen);
        assert(success);
        (void)success;
    }
}

BaseScreen *BaseScreen::legacyFromNativeScreen(NcScreen *screen)
{
    return fromNativeScreen(screen);
}

BaseScreen *BaseScreen::fromNativeScreen(NcScreen *screen)
{
    return legacyOwner(screen);
}

NcScreenCallbacks BaseScreen::makeNativeCallbacks()
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

NcWindow *BaseScreen::nativeActiveWindowCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner == nullptr || owner->activeWindow() == nullptr)
        return nullptr;
    return owner->activeWindow()->nativeWindow();
}

void BaseScreen::nativeRefreshCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner != nullptr)
        owner->refresh();
}

void BaseScreen::nativeRefreshWindowCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner != nullptr)
        owner->refreshWindow();
}

void BaseScreen::nativeScrollCallback(NcScreen *screen, enum NcScroll where)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner != nullptr)
        owner->scroll(toCppScroll(where));
}

void BaseScreen::nativeSwitchToCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner == nullptr)
        return;
    if (myScreen != owner)
        setTabPreviousScreen(owner);
    syncLegacyScreenPointers();
}

void BaseScreen::nativeResizeCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner != nullptr)
        owner->resize();
}

int32 BaseScreen::nativeWindowTimeoutCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner == nullptr)
        return defaultWindowTimeout;
    return owner->windowTimeout();
}

char *BaseScreen::nativeTitleCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner == nullptr)
        return nullptr;
    owner->m_native_title_cache = owner->title();
    return const_cast<char *>(owner->m_native_title_cache.c_str());
}

void BaseScreen::nativeUpdateCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner != nullptr)
        owner->update();
}

void BaseScreen::nativeMouseButtonPressedCallback(NcScreen *screen,
                                                 MEVENT event)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner != nullptr)
        owner->mouseButtonPressed(event);
}

bool BaseScreen::nativeIsLockableCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner == nullptr)
        return false;
    return owner->isLockable();
}

bool BaseScreen::nativeIsMergableCallback(NcScreen *screen)
{
    BaseScreen *owner = fromNativeScreen(screen);

    if (owner == nullptr)
        return false;
    return owner->isMergable();
}

void BaseScreen::getWindowResizeParams(size_t &x_offset, size_t &width,
                                       bool adjust_locked_screen)
{
    NcScreen *locked_screen;
    NcScreen *inactive_screen;

    locked_screen = app_state_get_locked_screen();
    inactive_screen = app_state_get_inactive_screen();

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

bool BaseScreen::lock()
{
    assert(app_state_get_locked_screen() == nullptr);
    if (!app_state_lock_current_screen())
        return false;
    syncLegacyScreenPointers();
    return true;
}

void BaseScreen::unlock()
{
    app_state_unlock_screen();
    syncLegacyScreenPointers();
}

/***********************************************************************/

void applyToVisibleScreens(std::function<void(NcScreen *)> f)
{
    app_state_each_visible_screen(
        [](NcScreen *screen, void *user) {
            auto *callback = static_cast<
                std::function<void(NcScreen *)> *>(user);
            (*callback)(screen);
        },
        &f);
}

void applyToVisibleWindows(std::function<void(BaseScreen *)> f)
{
    applyToVisibleScreens([&f](NcScreen *screen) {
        BaseScreen *owner = legacyOwner(screen);

        if (owner != nullptr)
            f(owner);
    });
}

void syncLegacyScreenPointers()
{
    myScreen = legacyOwner(app_state_get_screen());
    myLockedScreen = legacyOwner(app_state_get_locked_screen());
    myInactiveScreen = legacyOwner(app_state_get_inactive_screen());
}

bool isVisible(BaseScreen *screen)
{
    assert(screen != 0);
    return app_state_is_screen_visible(screen->nativeScreen());
}
