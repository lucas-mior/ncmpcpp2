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

#ifndef NCMPCPP_LASTFM_H
#define NCMPCPP_LASTFM_H

#include "config.h"

#include <functional>
#include <future>
#include <memory>

#include "interfaces.h"
#include "lastfm_service.h"
#include "screens/nc_lastfm.h"
#include "screens/screen.h"
#include "utility/utf8.h"

struct Lastfm: BaseScreen, Tabbable
{
    Lastfm();
    virtual ~Lastfm();

    virtual bool isActiveWindow(const NC::Window &w_) const override;
    virtual NC::Window *activeWindow() override;
    virtual const NC::Window *activeWindow() const override;
    virtual void refresh() override;
    virtual void refreshWindow() override;
    virtual void scroll(enum NcScroll where) override;
    virtual void switchTo() override;
    virtual void resize() override;
    virtual int windowTimeout() override;
    virtual std::string title() override;
    virtual ScreenType type() override { return NCM_SCREEN_TYPE_LASTFM; }
    virtual void update() override;
    virtual void mouseButtonPressed(MEVENT me) override;
    virtual bool isLockable() override { return true; }
    virtual bool isMergable() override { return true; }
    virtual NcScreen *nativeScreen() override;
    virtual const NcScreen *nativeScreen() const override;

    template <typename ServiceT>
    void queueJob(ServiceT *service)
    {
        auto old_service = dynamic_cast<ServiceT *>(m_service.get());
        // if the same service and arguments were used, leave old info
        if (old_service != nullptr && *old_service == *service)
            return;

        m_service = std::shared_ptr<ServiceT>(service);
        m_worker = std::async(
            std::launch::async,
            std::bind(&LastFm::Service::fetch, m_service));

        w.clear();
        w << "Fetching information...";
        m_refresh_window = true;
        m_title = m_service->name();
    }

private:
    void setDimensions();
    NcScreenCallbacks makeCallbacks();

    static Lastfm *fromScreen(NcScreen *screen);
    static NcWindow *activeWindowCallback(NcScreen *screen);
    static void refreshCallback(NcScreen *screen);
    static void refreshWindowCallback(NcScreen *screen);
    static void scrollCallback(NcScreen *screen, enum NcScroll where);
    static void switchToCallback(NcScreen *screen);
    static void resizeCallback(NcScreen *screen);
    static int32 windowTimeoutCallback(NcScreen *screen);
    static char *titleCallback(NcScreen *screen);
    static void updateCallback(NcScreen *screen);
    static void mouseButtonPressedCallback(NcScreen *screen, MEVENT event);
    static bool isLockableCallback(NcScreen *screen);
    static bool isMergableCallback(NcScreen *screen);

    NC::Scrollpad w;
    NcLastfmScreen m_screen;
    std::string m_title;
    std::string m_title_cache;
    bool m_refresh_window;

    std::shared_ptr<LastFm::Service> m_service;
    std::future<LastFm::Service::Result> m_worker;
};

extern Lastfm *myLastfm;

#endif // NCMPCPP_LASTFM_H
