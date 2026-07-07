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

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iterator>
#include <sstream>

#include "curses/formatted_color.h"
#include "app_controller.h"
#include "global.h"
#include "ui_state_legacy.h"
#include "helpers_legacy.h"
#include "screens/server_info.h"
#include "screens/screen_switcher.h"
#include "screens/screen_legacy.h"
#include "statusbar.h"


namespace {

NcScroll toNcScroll(enum NcScroll where)
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

void appendBoldLabel(NcBuffer *buffer, const char *label)
{
    int32 begin = nc_buffer_len(buffer);

    appendCString(buffer, label);
    nc_buffer_add_format(buffer, begin, NC_FORMAT_BOLD, 0);
    nc_buffer_add_format(buffer,
                         nc_buffer_len(buffer),
                         NC_FORMAT_NO_BOLD,
                         0);
}

std::string showLongTime(unsigned long seconds)
{
    std::ostringstream out;

    ShowTime(out, seconds, true);
    return out.str();
}

}

ServerInfo *myServerInfo;

ServerInfo::ServerInfo()
: m_timer()
{
    nc_server_info_screen_init(&m_screen,
                               makeHooks(),
                               COLS,
                               LINES,
                               ui_state_legacy_main_start_y(),
                               ui_state_legacy_main_height(),
                               NC::toNcColor(Config.main_color),
                               toNcBorder(Config.window_border));

    bool register_success = app_controller_register_screen(nativeScreen());
    assert(register_success);
    (void)register_success;
}

ServerInfo::~ServerInfo()
{
    if (app_controller_is_screen_registered(nativeScreen()))
    {
        app_controller_unregister_screen(nativeScreen());
    }
    nc_server_info_screen_destroy(&m_screen);
}

bool ServerInfo::isActiveWindow(const NC::Window &w_) const
{
    (void)w_;
    return false;
}

NC::Window *ServerInfo::activeWindow()
{
    return nullptr;
}

const NC::Window *ServerInfo::activeWindow() const
{
    return nullptr;
}

void ServerInfo::refresh()
{
    nc_screen_refresh(nativeScreen());
}

void ServerInfo::refreshWindow()
{
    nc_screen_refresh_window(nativeScreen());
}

void ServerInfo::scroll(enum NcScroll where)
{
    nc_screen_scroll(nativeScreen(), toNcScroll(where));
}

void ServerInfo::switchTo()
{
    nc_screen_set_has_to_be_resized(nativeScreen(), hasToBeResized);
    app_controller_switch_to_screen(nativeScreen());
}

void ServerInfo::resize()
{
    nc_screen_resize(nativeScreen());
}

int ServerInfo::windowTimeout()
{
    return nc_screen_window_timeout(nativeScreen());
}

std::string ServerInfo::title()
{
    char *result = nc_screen_title(nativeScreen());

    if (result == nullptr)
    {
        return "";
    }
    return result;
}

void ServerInfo::update()
{
    nc_screen_update(nativeScreen());
}

void ServerInfo::mouseButtonPressed(MEVENT me)
{
    nc_screen_mouse_button_pressed(nativeScreen(), me);
}

bool ServerInfo::isLockable()
{
    return nc_screen_is_lockable(nativeScreen());
}

bool ServerInfo::isMergable()
{
    return nc_screen_is_mergable(nativeScreen());
}

NcScreen *ServerInfo::nativeScreen()
{
    return nc_server_info_screen_base(&m_screen);
}

const NcScreen *ServerInfo::nativeScreen() const
{
    return nc_server_info_screen_base(const_cast<NcServerInfoScreen *>(
        &m_screen));
}

void ServerInfo::loadServerInfoLists()
{
    m_url_handlers.clear();
    std::copy(std::make_move_iterator(Mpd.GetURLHandlers()),
              std::make_move_iterator(MPD::StringIterator()),
              std::back_inserter(m_url_handlers));

    m_tag_types.clear();
    std::copy(std::make_move_iterator(Mpd.GetTagTypes()),
              std::make_move_iterator(MPD::StringIterator()),
              std::back_inserter(m_tag_types));
}

bool ServerInfo::renderServerInfo(NcBuffer *buffer)
{
    if (global_timer_elapsed_ms(m_timer) < 1000)
    {
        return false;
    }
    m_timer = global_timer;

    NcmMpdStats stats;
    NcmError error{};

    if (!ncm_mpd_client_get_stats(&global_mpd, &stats, &error))
    {
        return false;
    }

    appendBoldLabel(buffer, "Version: ");
    appendCString(buffer, "0.");
    nc_buffer_append_uint32(buffer, ncm_mpd_client_version(&global_mpd));
    appendCString(buffer, ".*\n");

    appendBoldLabel(buffer, "Uptime: ");
    appendData(buffer, showLongTime(stats.uptime));
    appendCString(buffer, "\n");

    appendBoldLabel(buffer, "Time playing: ");
    char play_time[64];
    ncm_song_show_time((uint32)stats.play_time, play_time, (int32)sizeof(play_time));
    appendData(buffer, play_time);
    appendCString(buffer, "\n\n");

    appendBoldLabel(buffer, "Total playtime: ");
    appendData(buffer, showLongTime(stats.db_play_time));
    appendCString(buffer, "\n");

    appendBoldLabel(buffer, "Artist names: ");
    nc_buffer_append_uint32(buffer, stats.artists);
    appendCString(buffer, "\n");

    appendBoldLabel(buffer, "Album names: ");
    nc_buffer_append_uint32(buffer, stats.albums);
    appendCString(buffer, "\n");

    appendBoldLabel(buffer, "Songs in database: ");
    nc_buffer_append_uint32(buffer, stats.songs);
    appendCString(buffer, "\n\n");

    appendBoldLabel(buffer, "Last DB update: ");
    appendData(buffer, Timestamp(stats.db_update_time));
    appendCString(buffer, "\n\n");

    appendBoldLabel(buffer, "URL Handlers:");
    for (auto it = m_url_handlers.begin(); it != m_url_handlers.end(); ++it)
    {
        appendCString(buffer, it != m_url_handlers.begin() ? ", " : " ");
        appendData(buffer, *it);
    }
    appendCString(buffer, "\n\n");

    appendBoldLabel(buffer, "Tag Types:");
    for (auto it = m_tag_types.begin(); it != m_tag_types.end(); ++it)
    {
        appendCString(buffer, it != m_tag_types.begin() ? ", " : " ");
        appendData(buffer, *it);
    }
    return true;
}

void ServerInfo::setDimensions()
{
    nc_server_info_screen_set_dimensions(&m_screen,
                                         COLS,
                                         LINES,
                                         ui_state_legacy_main_start_y(),
                                         ui_state_legacy_main_height());
}

NcServerInfoHooks ServerInfo::makeHooks()
{
    NcServerInfoHooks hooks = {0};

    hooks.load_lists = loadListsHook;
    hooks.render = renderHook;
    hooks.switch_to = switchToHook;
    hooks.resize_layout = resizeLayoutHook;
    hooks.resize_background = resizeBackgroundHook;
    hooks.title = titleHook;
    hooks.destroy = destroyHook;
    hooks.user = this;
    return hooks;
}

void ServerInfo::loadListsHook(void *user)
{
    static_cast<ServerInfo *>(user)->loadServerInfoLists();
}

bool ServerInfo::renderHook(void *user, NcBuffer *buffer)
{
    return static_cast<ServerInfo *>(user)->renderServerInfo(buffer);
}

void ServerInfo::switchToHook(void *user)
{
    ServerInfo *server_info = static_cast<ServerInfo *>(user);
    
    if (screenLegacySwitchChanged())
    {
        SwitchTo::finishNativeSwitch(server_info);
    }
    else
    {
        server_info->switchToPreviousScreen();
    }
}

void ServerInfo::resizeLayoutHook(void *user)
{
    static_cast<ServerInfo *>(user)->setDimensions();
}

void ServerInfo::resizeBackgroundHook(void *user)
{
    ServerInfo *server_info = static_cast<ServerInfo *>(user);

    if (server_info->previousScreen()
        && server_info->previousScreen()->hasToBeResized)
    {
        server_info->previousScreen()->resize();
        server_info->previousScreen()->refresh();
    }
    server_info->hasToBeResized = false;
}

char *ServerInfo::titleHook(void *user)
{
    ServerInfo *server_info = static_cast<ServerInfo *>(user);

    if (server_info->previousScreen())
    {
        server_info->m_title_cache = server_info->previousScreen()->title();
    }
    else
    {
        server_info->m_title_cache.clear();
    }
    return const_cast<char *>(server_info->m_title_cache.c_str());
}

void ServerInfo::destroyHook(void *user)
{
    ServerInfo *server_info = static_cast<ServerInfo *>(user);

    if (myServerInfo == server_info)
    {
        myServerInfo = nullptr;
    }
    delete server_info;
}
