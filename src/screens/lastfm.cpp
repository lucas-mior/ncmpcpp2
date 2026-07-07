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
#include "c/ncm_base.h"
#include "app_controller.h"
#include "global.h"
#include "ui_state_legacy.h"
#include "helpers.h"
#include "screens/screen_switcher.h"
#include "screens/screen_legacy.h"
#include "statusbar.h"
#include "title.h"


namespace {

enum NcScroll to_cpp_scroll(enum NcScroll where)
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

enum NcScroll to_nc_scroll(enum NcScroll where)
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


static void destroy_lastfm_service(NcmLastfmService *service)
{
    ncm_lastfm_service_destroy(service);
    ncm_free(service, static_cast<int64>(sizeof(*service)));
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

void Lastfm::scroll(enum NcScroll where)
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
        if (result.success)
        {
            w.clear();
            w << Charset::utf8ToLocale(result.text);
            if (m_service
                && ncm_lastfm_service_type(m_service.get())
                   == NCM_LASTFM_SERVICE_ARTIST_INFO)
            {
                w.setProperties(NC_FORMAT_BOLD, "\n\nSimilar artists:\n",
                                NC_FORMAT_NO_BOLD, 0);
                w.setProperties(NC_FORMAT_BOLD, "\n\nSimilar tags:\n",
                                NC_FORMAT_NO_BOLD, 0);
                w.setProperties(Config.color2, "\n * ", 0);
            }
        }
        else
            w << " " << NC::Color::Red << result.text << NC::Color::End;
        // reset m_worker so it's no longer valid
        m_worker = std::future<AsyncResult>();
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

void Lastfm::queueArtistInfo(const std::string &artist,
                             const std::string &lang)
{
    NcmLastfmService *service;
    NcmLastfmService candidate;

    ncm_lastfm_service_init(&candidate);
    ncm_lastfm_artist_info_init(
        &candidate, const_cast<char *>(artist.data()),
        static_cast<int32>(artist.size()), const_cast<char *>(lang.data()),
        static_cast<int32>(lang.size()));
    if (m_service
        && ncm_lastfm_service_equal(m_service.get(), &candidate))
    {
        ncm_lastfm_service_destroy(&candidate);
        return;
    }

    service = static_cast<NcmLastfmService *>(
        ncm_malloc(static_cast<int64>(sizeof(*service))));
    ncm_lastfm_service_init(service);
    ncm_lastfm_artist_info_init(
        service, const_cast<char *>(artist.data()),
        static_cast<int32>(artist.size()), const_cast<char *>(lang.data()),
        static_cast<int32>(lang.size()));
    ncm_lastfm_service_destroy(&candidate);

    m_service = std::shared_ptr<NcmLastfmService>(service,
                                                 destroy_lastfm_service);
    m_worker = std::async(std::launch::async, [service = m_service] {
        NcmLastfmResult c_result;
        AsyncResult result;

        ncm_lastfm_result_init(&c_result);
        ncm_lastfm_service_fetch(service.get(), &c_result);
        result.success = c_result.success;
        if (c_result.text != nullptr)
            result.text.assign(c_result.text, c_result.text_len);
        ncm_lastfm_result_destroy(&c_result);
        return result;
    });

    w.clear();
    w << "Fetching information...";
    m_refresh_window = true;
    m_title = ncm_lastfm_service_name(m_service.get());
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
