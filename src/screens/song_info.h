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

#ifndef NCMPCPP_SONG_INFO_H
#define NCMPCPP_SONG_INFO_H

#include <string>

#include "c/ncm_type_conversions.h"
#include "interfaces.h"
#include "song.h"
#include "screens/nc_song_info.h"
#include "screens/screen.h"

struct SongInfo: BaseScreen, Tabbable
{
    struct Metadata
    {
        const char *Name;
        enum NcmSongGetter Get;
        enum NcmTagsField Field;
    };

    SongInfo();
    virtual ~SongInfo();

    virtual bool isActiveWindow(const NC::Window &w_) const override;
    virtual NC::Window *activeWindow() override;
    virtual const NC::Window *activeWindow() const override;
    virtual void refresh() override;
    virtual void refreshWindow() override;
    virtual void scroll(NC::Scroll where) override;
    virtual void switchTo() override;
    virtual void resize() override;
    virtual int windowTimeout() override;
    virtual std::string title() override;
    virtual ScreenType type() override { return ScreenType::SongInfo; }
    virtual void update() override;
    virtual void mouseButtonPressed(MEVENT me) override;
    virtual bool isLockable() override;
    virtual bool isMergable() override;
    virtual NcScreen *nativeScreen() override;
    virtual const NcScreen *nativeScreen() const override;

    static const Metadata Tags[];

private:
    bool renderSong(NcBuffer *buffer);
    void setGeometry(NcSongInfoScreen *screen);
    NcSongInfoHooks makeHooks();

    static bool renderHook(void *user, NcSongInfoScreen *screen,
                           NcBuffer *buffer);
    static void switchToHook(void *user, NcSongInfoScreen *screen);
    static void resizeLayoutHook(void *user, NcSongInfoScreen *screen);
    static void destroyHook(void *user);

    NcSongInfoScreen m_screen;
    const MPD::Song *m_pending_song;
};

extern SongInfo *mySongInfo;

#endif // NCMPCPP_SONG_INFO_H
