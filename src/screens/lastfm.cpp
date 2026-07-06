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

#include "screens/lastfm.h"

#include <chrono>

#include "charset.h"
#include "app_controller.h"
#include "global.h"
#include "ui_state_legacy.h"
#include "helpers.h"
#include "screens/screen_switcher.h"
#include "screens/screen_legacy.h"
#include "statusbar.h"
#include "title.h"


namespace {

NC::Scroll to_cpp_scroll(enum NcScroll where)
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

enum NcScroll to_nc_scroll(NC::Scroll where)
{
    switch (where)
    {
        case NC::Scroll::Up:
            return NC_SCROLL_UP;
        case NC::Scroll::Down:
            return NC_SCROLL_DOWN;
        case NC::Scroll::PageUp:
            return NC_SCROLL_PAGE_UP;
        case NC::Scroll::PageDown:
            return NC_SCROLL_PAGE_DOWN;
        case NC::Scroll::Home:
            return NC_SCROLL_HOME;
        case NC::Scroll::End:
            return NC_SCROLL_END;
    }
    return NC_SCROLL_UP;
}

}

Lastfm *myLastfm;

Lastfm::Lastfm()
: m_refresh_window(false)
{
    NcScreenCallbacks callbacks = makeCallbacks();

    nc_lastfm_screen_init(&m_screen,
                          callbacks,
                          this,
                          0,
                          COLS,
                          ui_state_legacy_main_start_y(),
                          ui_state_legacy_main_height());
    w = NC::Scrollpad(nc_lastfm_screen_start_x(&m_screen),
                      nc_lastfm_screen_start_y(&m_screen),
                      nc_lastfm_screen_width(&m_screen),
                      nc_lastfm_screen_height(&m_screen),
                      "",
                      Config.main_color,
                      NC::Border());
}

Lastfm::~Lastfm()
{
    if (app_controller_is_screen_registered(nativeScreen()))
    {
        app_controller_unregister_screen(nativeScreen());
    }
}

bool Lastfm::isActiveWindow(const NC::Window &w_) const
{
    return &w == &w_;
}

NC::Window *Lastfm::activeWindow()
{
    return &w;
}

const NC::Window *Lastfm::activeWindow() const
{
    return &w;
}

void Lastfm::refresh()
{
    nc_screen_refresh(nativeScreen());
}

void Lastfm::refreshWindow()
{
    nc_screen_refresh_window(nativeScreen());
}

void Lastfm::scroll(NC::Scroll where)
{
    nc_screen_scroll(nativeScreen(), to_nc_scroll(where));
}

void Lastfm::resize()
{
    nc_screen_resize(nativeScreen());
}

int Lastfm::windowTimeout()
{
    return nc_screen_window_timeout(nativeScreen());
}

std::string Lastfm::title()
{
    if (m_title.empty())
        return "Last.fm";
    else
        return m_title;
}

void Lastfm::update()
{
    if (m_worker.valid()
        && (m_worker.wait_for(std::chrono::seconds(0))
            == std::future_status::ready))
    {
        auto result = m_worker.get();
        if (result.first)
        {
            w.clear();
            w << Charset::utf8ToLocale(result.second);
            m_service->beautifyOutput(w);
        }
        else
            w << " " << NC::Color::Red << result.second << NC::Color::End;
        // reset m_worker so it's no longer valid
        m_worker = std::future<LastFm::Service::Result>();
        m_refresh_window = true;
    }

    if (m_refresh_window)
    {
        m_refresh_window = false;
        w.flush();
        w.refresh();
    }
}

void Lastfm::mouseButtonPressed(MEVENT me)
{
    nc_screen_mouse_button_pressed(nativeScreen(), me);
}

NcScreen *Lastfm::nativeScreen()
{
    return nc_lastfm_screen_base(&m_screen);
}

const NcScreen *Lastfm::nativeScreen() const
{
    return nc_lastfm_screen_base(const_cast<NcLastfmScreen *>(&m_screen));
}

void Lastfm::switchTo()
{
    
    if (screenLegacyCurrent() != this)
    {
        nc_screen_set_has_to_be_resized(nativeScreen(), hasToBeResized);
        app_controller_switch_to_screen(nativeScreen());
        drawHeader();
    }
    else
        switchToPreviousScreen();
}

void Lastfm::setDimensions()
{
    size_t x_offset;
    size_t width;

    getWindowResizeParams(x_offset, width);
    nc_lastfm_screen_set_geometry(&m_screen,
                                  static_cast<int64>(x_offset),
                                  static_cast<int64>(width),
                                  ui_state_legacy_main_start_y(),
                                  ui_state_legacy_main_height());
}

NcScreenCallbacks Lastfm::makeCallbacks()
{
    NcScreenCallbacks callbacks = {0};

    callbacks.active_window = activeWindowCallback;
    callbacks.refresh = refreshCallback;
    callbacks.refresh_window = refreshWindowCallback;
    callbacks.scroll = scrollCallback;
    callbacks.switch_to = switchToCallback;
    callbacks.resize = resizeCallback;
    callbacks.window_timeout = windowTimeoutCallback;
    callbacks.title = titleCallback;
    callbacks.update = updateCallback;
    callbacks.mouse_button_pressed = mouseButtonPressedCallback;
    callbacks.is_lockable = isLockableCallback;
    callbacks.is_mergable = isMergableCallback;
    return callbacks;
}

Lastfm *Lastfm::fromScreen(NcScreen *screen)
{
    return static_cast<Lastfm *>(nc_screen_user(screen));
}

NcWindow *Lastfm::activeWindowCallback(NcScreen *screen)
{
    return fromScreen(screen)->w.nativeWindow();
}

void Lastfm::refreshCallback(NcScreen *screen)
{
    fromScreen(screen)->w.display();
}

void Lastfm::refreshWindowCallback(NcScreen *screen)
{
    fromScreen(screen)->w.display();
}

void Lastfm::scrollCallback(NcScreen *screen, enum NcScroll where)
{
    fromScreen(screen)->w.scroll(to_cpp_scroll(where));
}

void Lastfm::switchToCallback(NcScreen *screen)
{
    Lastfm *lastfm = fromScreen(screen);
    
    if (screenLegacySwitchChanged())
        SwitchTo::finishNativeSwitch(lastfm);
    else
        lastfm->switchToPreviousScreen();
}

void Lastfm::resizeCallback(NcScreen *screen)
{
    Lastfm *lastfm = fromScreen(screen);

    lastfm->setDimensions();
    lastfm->w.resize(nc_lastfm_screen_width(&lastfm->m_screen),
                     nc_lastfm_screen_height(&lastfm->m_screen));
    lastfm->w.moveTo(nc_lastfm_screen_start_x(&lastfm->m_screen),
                     nc_lastfm_screen_start_y(&lastfm->m_screen));
    lastfm->hasToBeResized = false;
}

int32 Lastfm::windowTimeoutCallback(NcScreen *screen)
{
    (void)screen;
    return defaultWindowTimeout;
}

char *Lastfm::titleCallback(NcScreen *screen)
{
    Lastfm *lastfm = fromScreen(screen);

    lastfm->m_title_cache = lastfm->title();
    return const_cast<char *>(lastfm->m_title_cache.c_str());
}

void Lastfm::updateCallback(NcScreen *screen)
{
    fromScreen(screen)->update();
}

void Lastfm::mouseButtonPressedCallback(NcScreen *screen, MEVENT event)
{
    scrollpadMouseButtonPressed(fromScreen(screen)->w, event);
}

bool Lastfm::isLockableCallback(NcScreen *screen)
{
    (void)screen;
    return true;
}

bool Lastfm::isMergableCallback(NcScreen *screen)
{
    (void)screen;
    return true;
}
