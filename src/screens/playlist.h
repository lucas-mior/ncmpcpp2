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

#ifndef NCMPCPP_PLAYLIST_H
#define NCMPCPP_PLAYLIST_H

#include <algorithm>
#include <cassert>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "app_controller.h"
#include "c/ncm_error.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_time.h"
#include "format.h"
#include "global.h"
#include "interfaces.h"
#include "regex_filter.h"
#include "screens/native_c_screens.h"
#include "screens/nc_playlist.h"
#include "screens/screen_cpp_compat.h"
#include "screens/screen_cpp_switcher.h"
#include "settings_legacy.h"
#include "song.h"
#include "song_list.h"

namespace Display {
std::string Columns(size_t width);
void Songs(NC::Menu<MPD::Song> &menu, const SongList &list,
           const Format::AST<char> &ast);
void SongsInColumns(NC::Menu<MPD::Song> &menu, const SongList &list);
}

#include "status.h"
#include "statusbar.h"
#include "ui_state.h"

struct Playlist: Screen<SongMenu>, Filterable, HasSongs, Searchable, Tabbable
{
    Playlist();
    virtual ~Playlist();

    virtual void refresh() override;
    virtual void refreshWindow() override;
    virtual void scroll(enum NcScroll where) override;
    virtual void switchTo() override;
    virtual void resize() override;
    virtual int windowTimeout() override;

    virtual std::string title() override;
    virtual ScreenType type() override { return NCM_SCREEN_TYPE_PLAYLIST; }

    virtual void update() override;

    virtual void mouseButtonPressed(MEVENT me) override;

    virtual bool isLockable() override;
    virtual bool isMergable() override;
    virtual NcScreen *nativeScreen() override;
    virtual const NcScreen *nativeScreen() const override;

    virtual bool allowsSearching() override;
    virtual const std::string &searchConstraint() override;
    virtual void setSearchConstraint(const std::string &constraint) override;
    virtual void clearSearchConstraint() override;
    virtual bool search(SearchDirection direction, bool wrap,
                        bool skip_current) override;

    virtual bool allowsFiltering() override;
    virtual std::string currentFilter() override;
    virtual void applyFilter(const std::string &filter) override;

    virtual bool itemAvailable() override;
    virtual bool addItemToPlaylist(bool play) override;
    virtual std::vector<MPD::Song> getSelectedSongs() override;

    MPD::Song nowPlayingSong();
    void locateSong(const MPD::Song &s);
    void enableHighlighting();
    void setSelectedItemsPriority(int prio);

    bool checkForSong(const MPD::Song &s);
    void registerSong(const MPD::Song &s);
    void unregisterSong(const MPD::Song &s);

    void reloadTotalLength() { m_reload_total_length = true; }
    void reloadRemaining() { m_reload_remaining = true; }

private:
    void syncNative();
    void resizeNativeWindow();
    static void syncNativeCallback(void *user);
    static NcWindow *nativeActiveWindowCallback(void *user);
    static void nativeRefreshCallback(void *user);
    static void nativeRefreshWindowCallback(void *user);
    static void nativeResizeCallback(void *user);
    std::string getTotalLength();
    std::string songToString(const MPD::Song &s);
    bool playlistEntryMatcher(const Regex::Regex &rx,
                              const MPD::Song &s);

    std::string m_stats;
    std::string m_title_cache;
    std::string m_search_constraint;
    Regex::Filter<MPD::Song> m_search_predicate;

    std::unordered_map<MPD::Song, int, MPD::Song::Hash> m_song_refs;

    size_t m_total_length;
    size_t m_remaining_time;
    size_t m_scroll_begin;

    NcmTimePoint m_timer;

    bool m_reload_total_length;
    bool m_reload_remaining;
};

inline Playlist *myPlaylist = nullptr;

inline Playlist::Playlist()
: m_total_length(0)
, m_remaining_time(0)
, m_scroll_begin(0)
, m_timer()
, m_reload_total_length(false)
, m_reload_remaining(false)
{
    w = NC::Menu<MPD::Song>(0,
                            static_cast<size_t>(ui_state_main_start_y()),
                            COLS,
                            static_cast<size_t>(ui_state_main_height()),
                            Config.playlist_display_mode
                                == NCM_DISPLAY_MODE_COLUMNS
                                && Config.titles_visibility
                                ? Display::Columns(COLS)
                                : "",
                            Config.main_color,
                            NC::Border());
    w.cyclicScrolling(Config.use_cyclic_scrolling);
    w.centeredCursor(Config.centered_cursor);
    w.setHighlightPrefix(Config.current_item_prefix);
    w.setHighlightSuffix(Config.current_item_suffix);
    w.setSelectedPrefix(Config.selected_item_prefix);
    w.setSelectedSuffix(Config.selected_item_suffix);
    switch (Config.playlist_display_mode)
    {
    case NCM_DISPLAY_MODE_CLASSIC:
        w.setItemDisplayer([this](NC::Menu<MPD::Song> &menu) {
            Display::Songs(menu, w, Config.song_list_format);
        });
        break;
    case NCM_DISPLAY_MODE_COLUMNS:
        w.setItemDisplayer([this](NC::Menu<MPD::Song> &menu) {
            Display::SongsInColumns(menu, w);
        });
        break;
    }
    w.setItemActivator([](NC::Menu<MPD::Song> &,
                          NC::Menu<MPD::Song>::Item &item) {
        NcmError error = {0};

        if (!ncm_mpd_client_play_id(&global_mpd,
                                    static_cast<int32>(item.value().getID()),
                                    &error))
            Statusbar::print(error.message);
    });

    native_c_screen_playlist_init();
    native_playlist_screen_set_sync_callback(native_c_screen_playlist(),
                                             syncNativeCallback, this);
    {
        NativePlaylistBridge bridge = {};

        bridge.active_window = nativeActiveWindowCallback;
        bridge.refresh = nativeRefreshCallback;
        bridge.refresh_window = nativeRefreshWindowCallback;
        bridge.resize = nativeResizeCallback;
        bridge.user = this;
        native_playlist_screen_set_bridge(native_c_screen_playlist(),
                                          bridge);
    }
    native_playlist_screen_set_display_menu(native_c_screen_playlist(),
                                            w.nativeMenu());
    screen_compat::bind_legacy_owner(nativeScreen(), this);
}

inline Playlist::~Playlist()
{
    NativePlaylistBridge bridge = {};

    native_playlist_screen_set_sync_callback(native_c_screen_playlist(),
                                             nullptr, nullptr);
    native_playlist_screen_set_bridge(native_c_screen_playlist(), bridge);
    native_playlist_screen_set_display_menu(native_c_screen_playlist(),
                                            nullptr);
    screen_compat::bind_legacy_owner(nativeScreen(), nullptr);
    if (app_controller_is_screen_registered(nativeScreen()))
        app_controller_unregister_screen(nativeScreen());
}

inline void Playlist::refresh()
{
    syncNative();
    nc_screen_refresh(nativeScreen());
}

inline void Playlist::refreshWindow()
{
    syncNative();
    nc_screen_refresh_window(nativeScreen());
}

inline void Playlist::scroll(enum NcScroll where)
{
    syncNative();
    nc_screen_scroll(nativeScreen(), where);
}

inline void Playlist::switchTo()
{
    syncNative();
    nc_screen_set_has_to_be_resized(nativeScreen(), hasToBeResized);
    app_controller_switch_to_screen(nativeScreen());
}

inline void Playlist::resize()
{
    syncNative();
    nc_screen_resize(nativeScreen());
    hasToBeResized = false;
}

inline int Playlist::windowTimeout()
{
    return nc_screen_window_timeout(nativeScreen());
}

inline std::string Playlist::title()
{
    syncNative();
    char *result = nc_screen_title(nativeScreen());
    if (result == nullptr)
        return "";
    return result;
}

inline void Playlist::update()
{
    if (w.isHighlighted()
        && Config.playlist_disable_highlight_delay_seconds > 0
        && global_timer_elapsed_seconds(m_timer)
           > static_cast<int64>(
                 Config.playlist_disable_highlight_delay_seconds))
    {
        w.setHighlighting(false);
        syncNative();
        nc_screen_refresh(nativeScreen());
    }
    nc_screen_update(nativeScreen());
}

inline void Playlist::mouseButtonPressed(MEVENT me)
{
    syncNative();
    nc_screen_mouse_button_pressed(nativeScreen(), me);
}

inline bool Playlist::isLockable()
{
    return nc_screen_is_lockable(nativeScreen());
}

inline bool Playlist::isMergable()
{
    return nc_screen_is_mergable(nativeScreen());
}

inline NcScreen *Playlist::nativeScreen()
{
    return native_c_screen_playlist_native();
}

inline const NcScreen *Playlist::nativeScreen() const
{
    return native_c_screen_playlist_native();
}

inline bool Playlist::allowsSearching()
{
    return true;
}

inline const std::string &Playlist::searchConstraint()
{
    return m_search_constraint;
}

inline void Playlist::setSearchConstraint(const std::string &constraint)
{
    m_search_predicate = Regex::Filter<MPD::Song>(
        constraint,
        Config.regex_type,
        [this](const Regex::Regex &rx, const MPD::Song &song) {
            return playlistEntryMatcher(rx, song);
        });
    m_search_constraint = constraint;
}

inline void Playlist::clearSearchConstraint()
{
    m_search_predicate.clear();
    m_search_constraint.clear();
}

inline bool Playlist::search(SearchDirection direction, bool wrap,
                             bool skip_current)
{
    if (!m_search_predicate.defined())
        return false;

    switch (direction)
    {
    case NCM_SEARCH_DIRECTION_BACKWARD:
    {
        auto current = w.rcurrent();
        if (skip_current && current != w.rend())
            ++current;
        auto it = std::find_if(current, w.rend(), m_search_predicate);
        if (it == w.rend() && wrap)
            it = std::find_if(w.rbegin(), current, m_search_predicate);
        if (it != w.rend())
        {
            w.highlight(it.base()-w.begin()-1);
            syncNative();
            return true;
        }
        break;
    }
    case NCM_SEARCH_DIRECTION_FORWARD:
    {
        auto current = w.current();
        if (skip_current && current != w.end())
            ++current;
        auto it = std::find_if(current, w.end(), m_search_predicate);
        if (it == w.end() && wrap)
            it = std::find_if(w.begin(), current, m_search_predicate);
        if (it != w.end())
        {
            w.highlight(it-w.begin());
            syncNative();
            return true;
        }
        break;
    }
    }
    return false;
}

inline bool Playlist::allowsFiltering()
{
    return allowsSearching();
}

inline std::string Playlist::currentFilter()
{
    std::string result;
    if (auto pred = w.filterPredicate<Regex::Filter<MPD::Song>>())
        result = pred->constraint();
    return result;
}

inline void Playlist::applyFilter(const std::string &constraint)
{
    if (!constraint.empty())
    {
        w.applyFilter(Regex::Filter<MPD::Song>(
                          constraint,
                          Config.regex_type,
                          [this](const Regex::Regex &rx,
                                 const MPD::Song &song) {
                              return playlistEntryMatcher(rx, song);
                          }));
    }
    else
        w.clearFilter();
    syncNative();
}

inline bool Playlist::itemAvailable()
{
    return !w.empty();
}

inline bool Playlist::addItemToPlaylist(bool play)
{
    syncNative();
    if (play)
        return nc_playlist_screen_activate_current(
            native_c_screen_playlist_typed());
    return true;
}

inline std::vector<MPD::Song> Playlist::getSelectedSongs()
{
    return w.getSelectedSongs();
}

inline MPD::Song Playlist::nowPlayingSong()
{
    MPD::Song s;
    if (ncm_status_state_player() != NCM_STATUS_PLAYER_UNKNOWN)
    {
        bool was_filtered = w.isFiltered();
        if (was_filtered)
            w.showAllItems();
        auto sp = ncm_status_state_current_song_position();
        if (sp >= 0 && size_t(sp) < w.size())
            s = w.at(sp).value();
        if (was_filtered)
            w.showFilteredItems();
    }
    return s;
}

inline void Playlist::locateSong(const MPD::Song &s)
{
    if (!w.isFiltered())
        w.highlight(s.getPosition());
    else
    {
        auto cmp = [](const MPD::Song &a, const MPD::Song &b) {
            return a.getPosition() < b.getPosition();
        };
        auto first = w.beginV(), last = w.endV();
        auto it = std::lower_bound(first, last, s, cmp);
        if (it != last && it->getPosition() == s.getPosition())
            w.highlight(it - first);
        else
            Statusbar::print("Song is filtered out");
    }
    syncNative();
}

inline void Playlist::enableHighlighting()
{
    w.setHighlighting(true);
    m_timer = global_timer;
    syncNative();
}

inline std::string Playlist::getTotalLength()
{
    std::ostringstream result;

    if (m_reload_total_length)
    {
        bool was_filtered = w.isFiltered();
        if (was_filtered)
            w.showAllItems();
        m_total_length = 0;
        for (const auto &s : w)
            m_total_length += s.value().getDuration();
        if (was_filtered)
            w.showFilteredItems();
        m_reload_total_length = false;
    }
    if (Config.playlist_show_remaining_time && m_reload_remaining)
    {
        int current_position = ncm_status_state_current_song_position();
        bool was_filtered = w.isFiltered();

        if (was_filtered)
            w.showAllItems();
        m_remaining_time = 0;
        if (current_position >= 0)
        {
            for (size_t i = static_cast<size_t>(current_position);
                 i < w.size(); ++i)
                m_remaining_time += w[i].value().getDuration();
        }
        if (was_filtered)
            w.showFilteredItems();
        m_reload_remaining = false;
    }

    result << '(' << w.size() << (w.size() == 1 ? " item" : " items");

    if (w.isFiltered())
    {
        bool was_filtered = w.isFiltered();
        if (was_filtered)
            w.showAllItems();
        result << " (out of " << w.size() << ")";
        if (was_filtered)
            w.showFilteredItems();
    }

    if (m_total_length)
    {
        result << ", length: ";
        result << MPD::Song::ShowTime(m_total_length);
    }
    if (Config.playlist_show_remaining_time
        && m_remaining_time
        && w.size() > 1)
    {
        result << ", remaining: ";
        result << MPD::Song::ShowTime(m_remaining_time);
    }
    result << ')';
    return result.str();
}

inline void Playlist::setSelectedItemsPriority(int prio)
{
    NcmError error = {0};

    syncNative();
    if (!native_playlist_screen_set_selected_priority(
            native_c_screen_playlist(), &global_mpd, prio, &error))
    {
        Statusbar::print(error.message);
        return;
    }
    Statusbar::print("Priority set");
}

inline bool Playlist::checkForSong(const MPD::Song &s)
{
    return m_song_refs.find(s) != m_song_refs.end();
}

inline void Playlist::registerSong(const MPD::Song &s)
{
    ++m_song_refs[s];
}

inline void Playlist::unregisterSong(const MPD::Song &s)
{
    auto it = m_song_refs.find(s);
    assert(it != m_song_refs.end());
    if (it->second == 1)
        m_song_refs.erase(it);
    else
        --it->second;
}

inline void Playlist::resizeNativeWindow()
{
    NativePlaylistScreen *native;
    NcPlaylistScreen *playlist;

    native = native_c_screen_playlist();
    playlist = native_playlist_screen_playlist(native);
    w.resize(static_cast<size_t>(nc_playlist_screen_width(playlist)),
             static_cast<size_t>(nc_playlist_screen_height(playlist)));
    w.moveTo(static_cast<size_t>(nc_playlist_screen_start_x(playlist)),
             static_cast<size_t>(nc_playlist_screen_start_y(playlist)));

    switch (Config.playlist_display_mode)
    {
    case NCM_DISPLAY_MODE_CLASSIC:
        w.setTitle("");
        break;
    case NCM_DISPLAY_MODE_COLUMNS:
        if (Config.titles_visibility)
            w.setTitle(Display::Columns(w.getWidth()));
        else
            w.setTitle("");
        break;
    }
}

inline NcWindow *Playlist::nativeActiveWindowCallback(void *user)
{
    Playlist *playlist = static_cast<Playlist *>(user);

    if (playlist == nullptr)
        return nullptr;
    return playlist->w.nativeWindow();
}

inline void Playlist::nativeRefreshCallback(void *user)
{
    Playlist *playlist = static_cast<Playlist *>(user);

    if (playlist == nullptr)
        return;
    playlist->w.display();
}

inline void Playlist::nativeRefreshWindowCallback(void *user)
{
    nativeRefreshCallback(user);
}

inline void Playlist::nativeResizeCallback(void *user)
{
    Playlist *playlist = static_cast<Playlist *>(user);

    if (playlist == nullptr)
        return;
    playlist->resizeNativeWindow();
}

inline void Playlist::syncNativeCallback(void *user)
{
    Playlist *playlist = static_cast<Playlist *>(user);

    if (playlist == nullptr)
        return;
    playlist->syncNative();
}

inline void Playlist::syncNative()
{
    NativePlaylistScreen *native;
    NcMenu *native_menu;
    bool was_filtered;

    native = native_c_screen_playlist();
    native_menu = native_playlist_screen_menu(native);
    was_filtered = w.isFiltered();
    if (was_filtered)
        w.showAllItems();

    native_playlist_screen_clear(native);
    for (auto it = w.begin(); it != w.end(); ++it)
    {
        uint32 flags = NC_MENU_ITEM_SELECTABLE;
        if (it->isSelected())
            flags |= NC_MENU_ITEM_SELECTED;
        if (it->isInactive())
            flags |= NC_MENU_ITEM_INACTIVE;
        if (it->isSeparator())
            flags |= NC_MENU_ITEM_SEPARATOR;
        nc_menu_add_item_with_flags(native_menu, it->value().cSong(), flags);
    }
    if (!w.empty())
        nc_menu_highlight_position(
            native_menu,
            static_cast<int64>(w.choice()),
            nc_playlist_screen_height(native_playlist_screen_playlist(native)));
    if (native_playlist_screen_consume_highlighting_request(native))
    {
        w.setHighlighting(true);
        m_timer = global_timer;
    }
    native_playlist_screen_set_highlighting(native, w.isHighlighted());
    native_playlist_screen_reload_total_length(native);
    native_playlist_screen_reload_remaining(native);

    if (was_filtered)
        w.showFilteredItems();
}

inline std::string Playlist::songToString(const MPD::Song &s)
{
    std::string result;
    switch (Config.playlist_display_mode)
    {
    case NCM_DISPLAY_MODE_CLASSIC:
        result = Format::stringify<char>(Config.song_list_format, &s);
        break;
    case NCM_DISPLAY_MODE_COLUMNS:
        result = Format::stringify<char>(Config.song_columns_mode_format,
                                         &s);
        break;
    }
    return result;
}

inline bool Playlist::playlistEntryMatcher(const Regex::Regex &rx,
                                           const MPD::Song &s)
{
    return Regex::search(songToString(s), rx);
}

#endif // NCMPCPP_PLAYLIST_H
