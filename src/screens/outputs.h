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

#ifndef NCMPCPP_OUTPUTS_H
#define NCMPCPP_OUTPUTS_H

#include "config.h"

#ifdef ENABLE_OUTPUTS

#include <string>

#include "interfaces.h"
#include "screens/nc_outputs.h"
#include "screens/screen_cpp_compat.h"

struct Outputs: BaseScreen, Tabbable
{
    Outputs();
    virtual ~Outputs();

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
    virtual ScreenType type() override { return NCM_SCREEN_TYPE_OUTPUTS; }
    virtual void update() override;
    virtual void mouseButtonPressed(MEVENT me) override;
    virtual bool isLockable() override;
    virtual bool isMergable() override;
    virtual NcScreen *nativeScreen() override;
    virtual const NcScreen *nativeScreen() const override;

    void fetchList();
    void toggleOutput();

private:
    void loadOutputs(NcOutputsScreen *screen);
    bool toggleOutput(uint32 id, bool enabled, char *name, int32 name_len);
    NcOutputsHooks makeHooks();

    static void fetchOutputsHook(void *user, NcOutputsScreen *screen);
    static bool toggleOutputHook(void *user, uint32 id, bool enabled,
                                 char *name, int32 name_len);
    static void switchToHook(void *user);
    static void resizeLayoutHook(void *user, NcOutputsScreen *screen);
    static void resizeBackgroundHook(void *user);
    static void destroyHook(void *user);

    NcOutputsScreen m_screen;
};

extern Outputs *myOutputs;

#endif // ENABLE_OUTPUTS

#endif // NCMPCPP_OUTPUTS_H
