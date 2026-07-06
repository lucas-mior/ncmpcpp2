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
#include <string>

#include "c/ncm_type_conversions.h"
#include "curses/formatted_color.h"
#include "app_state.h"
#include "global.h"
#include "helpers.h"
#include "screens/screen_switcher.h"
#include "screens/song_info.h"
#include "settings.h"
#include "title.h"

#if defined(HAVE_TAGLIB_H)
# include "c/ncm_taglib.h"
# include "c/ncm_tags.h"
#endif

using Global::MainHeight;
using Global::MainStartY;

namespace {

NcScroll toNcScroll(NC::Scroll where)
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

NcBorder toNcBorder(NC::Border border)
{
    NcBorder result = {};

    if (border)
    {
        result.enabled = true;
        result.color = NC::toNcColor(*border);
    }
    return result;
}

void appendData(NcBuffer *buffer, const std::string &value)
{
    nc_buffer_append_data(buffer,
                          const_cast<char *>(value.data()),
                          static_cast<int32>(value.size()));
}

void appendCString(NcBuffer *buffer, const char *value)
{
    nc_buffer_append_cstring(buffer, const_cast<char *>(value));
}

void appendColor(NcBuffer *buffer, NC::Color color)
{
    nc_buffer_add_color(buffer, nc_buffer_len(buffer), NC::toNcColor(color), 0);
}

void appendFormattedColor(NcBuffer *buffer, NC::FormattedColor &color)
{
    nc_buffer_add_formatted_color(buffer,
                                  nc_buffer_len(buffer),
                                  color.cFormattedColor(),
                                  0);
}

void appendFormattedColorEnd(NcBuffer *buffer, NC::FormattedColor &color)
{
    nc_buffer_add_formatted_color_end(buffer,
                                      nc_buffer_len(buffer),
                                      color.cFormattedColor(),
                                      0);
}

void appendFormat(NcBuffer *buffer, enum NcFormat format)
{
    nc_buffer_add_format(buffer, nc_buffer_len(buffer), format, 0);
}

void appendShownTag(NcBuffer *buffer, const std::string &tag)
{
    if (tag.empty())
    {
        appendFormattedColor(buffer, Config.empty_tags_color);
        appendData(buffer, Config.empty_tag);
        appendFormattedColorEnd(buffer, Config.empty_tags_color);
        return;
    }
    appendData(buffer, tag);
}

void appendKeyValue(NcBuffer *buffer, const char *key,
                    const std::string &value)
{
    appendFormat(buffer, NC_FORMAT_BOLD);
    appendFormattedColor(buffer, Config.color1);
    appendCString(buffer, key);
    appendCString(buffer, ":");
    appendFormattedColorEnd(buffer, Config.color1);
    appendFormat(buffer, NC_FORMAT_NO_BOLD);
    appendCString(buffer, " ");
    appendFormattedColor(buffer, Config.color2);
    appendData(buffer, value);
    appendFormattedColorEnd(buffer, Config.color2);
    appendCString(buffer, "\n");
}

void appendKeyShownTag(NcBuffer *buffer, const char *key,
                       const std::string &value)
{
    appendFormat(buffer, NC_FORMAT_BOLD);
    appendFormattedColor(buffer, Config.color1);
    appendCString(buffer, key);
    appendCString(buffer, ":");
    appendFormattedColorEnd(buffer, Config.color1);
    appendFormat(buffer, NC_FORMAT_NO_BOLD);
    appendCString(buffer, " ");
    appendFormattedColor(buffer, Config.color2);
    appendShownTag(buffer, value);
    appendFormattedColorEnd(buffer, Config.color2);
    appendCString(buffer, "\n");
}

std::string channelsString(int channels)
{
    char buffer[32];
    int32 len = ncm_channels_to_string(
        static_cast<int32>(channels),
        buffer,
        static_cast<int32>(sizeof(buffer)));
    return std::string(buffer, static_cast<size_t>(len));
}

}

SongInfo *mySongInfo;

const SongInfo::Metadata SongInfo::Tags[] =
{
    {"Title", NCM_SONG_GETTER_TITLE, NCM_TAGS_FIELD_TITLE},
    {"Artist", NCM_SONG_GETTER_ARTIST, NCM_TAGS_FIELD_ARTIST},
    {"Album Artist",
     NCM_SONG_GETTER_ALBUM_ARTIST,
     NCM_TAGS_FIELD_ALBUM_ARTIST},
    {"Album", NCM_SONG_GETTER_ALBUM, NCM_TAGS_FIELD_ALBUM},
    {"Date", NCM_SONG_GETTER_DATE, NCM_TAGS_FIELD_DATE},
    {"Track", NCM_SONG_GETTER_TRACK, NCM_TAGS_FIELD_TRACK},
    {"Genre", NCM_SONG_GETTER_GENRE, NCM_TAGS_FIELD_GENRE},
    {"Composer", NCM_SONG_GETTER_COMPOSER, NCM_TAGS_FIELD_COMPOSER},
    {"Performer", NCM_SONG_GETTER_PERFORMER, NCM_TAGS_FIELD_PERFORMER},
    {"Disc", NCM_SONG_GETTER_DISC, NCM_TAGS_FIELD_DISC},
    {"Comment", NCM_SONG_GETTER_COMMENT, NCM_TAGS_FIELD_COMMENT},
    {0, NCM_SONG_GETTER_NONE, NCM_TAGS_FIELD_LAST}
};

SongInfo::SongInfo()
: m_pending_song(nullptr)
{
    nc_song_info_screen_init(&m_screen,
                             makeHooks(),
                             0,
                             COLS,
                             MainStartY,
                             MainHeight,
                             NC::toNcColor(Config.main_color),
                             toNcBorder(NC::Border()),
                             static_cast<int64>(Config.lines_scrolled));

    bool register_success = app_state_register_screen(nativeScreen());
    assert(register_success);
    (void)register_success;
}

SongInfo::~SongInfo()
{
    if (app_state_is_screen_registered(nativeScreen()))
    {
        app_state_unregister_screen(nativeScreen());
    }
    nc_song_info_screen_destroy(&m_screen);
}

bool SongInfo::isActiveWindow(const NC::Window &w_) const
{
    (void)w_;
    return false;
}

NC::Window *SongInfo::activeWindow()
{
    return nullptr;
}

const NC::Window *SongInfo::activeWindow() const
{
    return nullptr;
}

void SongInfo::refresh()
{
    nc_screen_refresh(nativeScreen());
}

void SongInfo::refreshWindow()
{
    nc_screen_refresh_window(nativeScreen());
}

void SongInfo::scroll(NC::Scroll where)
{
    nc_screen_scroll(nativeScreen(), toNcScroll(where));
}

void SongInfo::switchTo()
{
    using Global::myScreen;

    if ((myScreen != this) && !currentSong(myScreen))
    {
        return;
    }
    nc_screen_set_has_to_be_resized(nativeScreen(), hasToBeResized);
    app_state_switch_to_screen(nativeScreen());
}

void SongInfo::resize()
{
    nc_screen_resize(nativeScreen());
}

int SongInfo::windowTimeout()
{
    return nc_screen_window_timeout(nativeScreen());
}

std::string SongInfo::title()
{
    char *result = nc_screen_title(nativeScreen());

    if (result == nullptr)
    {
        return "";
    }
    return result;
}

void SongInfo::update()
{
    nc_screen_update(nativeScreen());
}

void SongInfo::mouseButtonPressed(MEVENT me)
{
    nc_screen_mouse_button_pressed(nativeScreen(), me);
}

bool SongInfo::isLockable()
{
    return nc_screen_is_lockable(nativeScreen());
}

bool SongInfo::isMergable()
{
    return nc_screen_is_mergable(nativeScreen());
}

NcScreen *SongInfo::nativeScreen()
{
    return nc_song_info_screen_base(&m_screen);
}

const NcScreen *SongInfo::nativeScreen() const
{
    return nc_song_info_screen_base(const_cast<NcSongInfoScreen *>(
        &m_screen));
}

bool SongInfo::renderSong(NcBuffer *buffer)
{
    if (m_pending_song == nullptr)
    {
        return false;
    }

    const MPD::Song &s = *m_pending_song;
    appendKeyValue(buffer, "Filename", s.getName());
    appendKeyShownTag(buffer, "Directory", s.getDirectory());
    appendCString(buffer, "\n");
    appendKeyValue(buffer, "Length", s.getLength());

#if defined(HAVE_TAGLIB_H)
    if (!Config.mpd_music_dir.empty() && !s.isStream())
    {
        std::string path_to_file;
        if (s.isFromDatabase())
        {
            path_to_file += Config.mpd_music_dir;
        }
        path_to_file += s.getURI();
        NcmTaglibFile file;
        NcmTaglibAudioProperties properties;

        ncm_taglib_file_init(&file);
        if (ncm_taglib_file_open(&file,
                                  const_cast<char *>(path_to_file.c_str())))
        {
            if (ncm_taglib_file_audio_properties(&file, &properties))
            {
                appendKeyValue(buffer,
                               "Bitrate",
                               std::to_string(properties.bitrate) + " kbps");
                appendKeyValue(
                    buffer,
                    "Sample rate",
                    std::to_string(properties.sample_rate) + " Hz");
                appendKeyValue(buffer,
                               "Channels",
                               channelsString(properties.channels));
            }

            NcmTagsReplayGainInfo rginfo;
            ncm_tags_replay_gain_info_init(&rginfo);
            if (ncm_tags_read_replay_gain(
                    const_cast<char *>(path_to_file.c_str()),
                    &rginfo)
                && !ncm_tags_replay_gain_info_empty(&rginfo))
            {
                auto viewString = [](NcmStringView view) {
                    return view.data != nullptr ? view.data : "";
                };
                appendCString(buffer, "\n");
                appendKeyValue(buffer,
                               "Reference loudness",
                               viewString(rginfo.reference_loudness));
                appendKeyValue(buffer,
                               "Track gain",
                               viewString(rginfo.track_gain));
                appendKeyValue(buffer,
                               "Track peak",
                               viewString(rginfo.track_peak));
                appendKeyValue(buffer,
                               "Album gain",
                               viewString(rginfo.album_gain));
                appendKeyValue(buffer,
                               "Album peak",
                               viewString(rginfo.album_peak));
            }
            ncm_tags_replay_gain_info_destroy(&rginfo);
            ncm_taglib_file_close(&file);
        }
    }
#endif

    appendColor(buffer, NC::Color::Default);
    for (const Metadata *m = Tags; m->Name; m += 1)
    {
        appendFormat(buffer, NC_FORMAT_BOLD);
        appendCString(buffer, "\n");
        appendCString(buffer, m->Name);
        appendCString(buffer, ":");
        appendFormat(buffer, NC_FORMAT_NO_BOLD);
        appendCString(buffer, " ");
        appendShownTag(buffer, s.getTags(m->Get));
    }
    return true;
}

void SongInfo::setGeometry(NcSongInfoScreen *screen)
{
    size_t x_offset;
    size_t width;

    getWindowResizeParams(x_offset, width);
    nc_song_info_screen_set_geometry(screen,
                                     static_cast<int64>(x_offset),
                                     static_cast<int64>(width),
                                     MainStartY,
                                     MainHeight);
    hasToBeResized = false;
}

NcSongInfoHooks SongInfo::makeHooks()
{
    NcSongInfoHooks hooks = {0};

    hooks.render = renderHook;
    hooks.switch_to = switchToHook;
    hooks.resize_layout = resizeLayoutHook;
    hooks.destroy = destroyHook;
    hooks.user = this;
    return hooks;
}

bool SongInfo::renderHook(void *user, NcSongInfoScreen *screen,
                          NcBuffer *buffer)
{
    (void)screen;
    return static_cast<SongInfo *>(user)->renderSong(buffer);
}

void SongInfo::switchToHook(void *user, NcSongInfoScreen *screen)
{
    SongInfo *song_info = static_cast<SongInfo *>(user);
    using Global::myScreen;

    if (myScreen != song_info)
    {
        const MPD::Song *song = currentSong(myScreen);
        if (song == nullptr)
        {
            return;
        }

        SwitchTo::finishNativeSwitch(song_info);
        song_info->m_pending_song = song;
        nc_song_info_screen_prepare_current(screen);
        song_info->m_pending_song = nullptr;
        drawHeader();
    }
    else
    {
        song_info->switchToPreviousScreen();
    }
}

void SongInfo::resizeLayoutHook(void *user, NcSongInfoScreen *screen)
{
    static_cast<SongInfo *>(user)->setGeometry(screen);
}

void SongInfo::destroyHook(void *user)
{
    SongInfo *song_info = static_cast<SongInfo *>(user);

    if (mySongInfo == song_info)
    {
        mySongInfo = nullptr;
    }
    delete song_info;
}
