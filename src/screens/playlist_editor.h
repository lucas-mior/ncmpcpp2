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

#ifndef NCMPCPP_PLAYLIST_EDITOR_H
#define NCMPCPP_PLAYLIST_EDITOR_H

#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <locale>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "charset.h"
#include "curses/menu_impl.h"
#include "format.h"
#include "format_impl.h"
#include "global.h"
#include "helpers/song_iterator_maker.h"
#include "helpers_legacy.h"
#include "interfaces.h"
#include "mpdpp.h"
#include "regex_filter.h"
#include "screens/native_c_screens.h"
#include "screens/nc_playlist_editor.h"
#include "screens/playlist.h"
#include "screens/screen_cpp_compat.h"
#include "screens/tag_editor.h"
#include "song_list.h"
#include "status_legacy.h"
#include "statusbar_legacy.h"
#include "title_legacy.h"
#include "ui_state.h"
#include "utility/comparators.h"
#include "utility/functional.h"
#include "utility/string_format.h"

namespace Display {
std::string Columns(size_t width);
void Songs(NC::Menu<MPD::Song> &menu, const SongList &list,
           const Format::AST<char> &ast);
void SongsInColumns(NC::Menu<MPD::Song> &menu, const SongList &list);
}

struct PlaylistEditor: Screen<NC::Window *>, Filterable, HasColumns,
                       HasSongs, Searchable, Tabbable
{
    PlaylistEditor();
    virtual ~PlaylistEditor();

    virtual void refresh() override;
    virtual void refreshWindow() override;
    virtual void scroll(enum NcScroll where) override;
    virtual void switchTo() override;
    virtual void resize() override;

    virtual std::string title() override;
    virtual ScreenType type() override
    {
        return NCM_SCREEN_TYPE_PLAYLIST_EDITOR;
    }
    virtual NcScreen *nativeScreen() override;
    virtual const NcScreen *nativeScreen() const override;

    virtual void update() override;

    virtual int windowTimeout() override;

    virtual void mouseButtonPressed(MEVENT me) override;

    virtual bool isLockable() override { return true; }
    virtual bool isMergable() override { return true; }

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

    virtual bool previousColumnAvailable() override;
    virtual void previousColumn() override;

    virtual bool nextColumnAvailable() override;
    virtual void nextColumn() override;

    void updateTimer();

    void requestPlaylistsUpdate() { m_playlists_update_requested = true; }
    void requestContentUpdate() { m_content_update_requested = true; }

    void locatePlaylist(const MPD::Playlist &playlist);
    void locateSong(const MPD::Song &s);

    NC::Menu<MPD::Playlist> Playlists;
    SongMenu Content;

private:
    void syncNative();
    void resizeLegacyColumns();
    static NcWindow *nativeActiveWindowCallback(void *user);
    static void nativeRefreshCallback(void *user);
    static void nativeRefreshWindowCallback(void *user);
    static void nativeScrollCallback(void *user, enum NcScroll where);
    static void nativeSwitchToCallback(void *user);
    static void nativeResizeCallback(void *user);
    static int32 nativeWindowTimeoutCallback(void *user);
    static char *nativeTitleCallback(void *user);
    static void nativeUpdateCallback(void *user);
    static void nativeMouseButtonPressedCallback(void *user, MEVENT event);

    bool m_playlists_update_requested;
    bool m_content_update_requested;

    NcmTimePoint m_timer;

    const int m_window_timeout;
    const int64 m_fetching_delay_ms;

    Regex::Filter<MPD::Playlist> m_playlists_search_predicate;
    Regex::Filter<MPD::Song> m_content_search_predicate;
    std::string m_title_cache;
};

inline PlaylistEditor *myPlaylistEditor = nullptr;

namespace playlist_editor_compat {

inline size_t LeftColumnStartX;
inline size_t LeftColumnWidth;
inline size_t RightColumnStartX;
inline size_t RightColumnWidth;

inline std::string SongToString(const MPD::Song &s);
inline bool PlaylistEntryMatcher(const Regex::Regex &rx,
                                 const MPD::Playlist &playlist);
inline bool SongEntryMatcher(const Regex::Regex &rx, const MPD::Song &s);
inline std::optional<size_t> GetSongIndexInPlaylist(
    MPD::Playlist playlist, const MPD::Song &song);

template <typename ItemT>
inline uint32 nativeFlags(const typename NC::Menu<ItemT>::Item &item)
{
    uint32 flags = 0;

    if (item.isSelectable())
        flags |= NC_MENU_ITEM_SELECTABLE;
    if (item.isSelected())
        flags |= NC_MENU_ITEM_SELECTED;
    if (item.isInactive())
        flags |= NC_MENU_ITEM_INACTIVE;
    if (item.isSeparator())
        flags |= NC_MENU_ITEM_SEPARATOR;
    return flags;
}

inline void addNativePlaylist(NativePlaylistEditorScreen *native,
                              NC::Menu<MPD::Playlist>::Item &item)
{
    nc_playlist_entry_menu_add_with_flags(
        native_playlist_editor_screen_playlists(native),
        item.value().cPlaylist(),
        nativeFlags<MPD::Playlist>(item));
}

inline void addNativeSong(NativePlaylistEditorScreen *native,
                          NC::Menu<MPD::Song>::Item &item)
{
    nc_song_menu_add_with_flags(
        native_playlist_editor_screen_content(native),
        item.value().cSong(),
        nativeFlags<MPD::Song>(item));
}

}

inline PlaylistEditor::PlaylistEditor()
: m_timer()
, m_window_timeout(Config.data_fetching_delay
                   ? 250
                   : BaseScreen::defaultWindowTimeout)
, m_fetching_delay_ms(Config.data_fetching_delay ? 250 : -1)
{
    namespace ph = std::placeholders;
    size_t ra = Config.playlist_editor_column_width_ratio[0];
    size_t rb = Config.playlist_editor_column_width_ratio[1];

    playlist_editor_compat::LeftColumnWidth = COLS*ra/(ra+rb)-1;
    playlist_editor_compat::RightColumnStartX =
        playlist_editor_compat::LeftColumnWidth + 1;
    playlist_editor_compat::RightColumnWidth =
        COLS - playlist_editor_compat::LeftColumnWidth - 1;

    Playlists = NC::Menu<MPD::Playlist>(
        0,
        static_cast<size_t>(ui_state_main_start_y()),
        playlist_editor_compat::LeftColumnWidth,
        static_cast<size_t>(ui_state_main_height()),
        Config.titles_visibility ? "Playlists" : "",
        Config.main_color,
        NC::Border());
    setHighlightFixes(Playlists);
    Playlists.cyclicScrolling(Config.use_cyclic_scrolling);
    Playlists.centeredCursor(Config.centered_cursor);
    Playlists.setSelectedPrefix(Config.selected_item_prefix);
    Playlists.setSelectedSuffix(Config.selected_item_suffix);
    Playlists.setItemDisplayer([](NC::Menu<MPD::Playlist> &menu) {
        menu << Charset::utf8ToLocale(menu.drawn()->value().path());
    });

    Content = NC::Menu<MPD::Song>(
        playlist_editor_compat::RightColumnStartX,
        static_cast<size_t>(ui_state_main_start_y()),
        playlist_editor_compat::RightColumnWidth,
        static_cast<size_t>(ui_state_main_height()),
        Config.titles_visibility ? "Content" : "",
        Config.main_color,
        NC::Border());
    setHighlightInactiveColumnFixes(Content);
    Content.cyclicScrolling(Config.use_cyclic_scrolling);
    Content.centeredCursor(Config.centered_cursor);
    Content.setSelectedPrefix(Config.selected_item_prefix);
    Content.setSelectedSuffix(Config.selected_item_suffix);
    switch (Config.playlist_editor_display_mode)
    {
    case NCM_DISPLAY_MODE_CLASSIC:
        Content.setItemDisplayer(std::bind(
            Display::Songs, ph::_1, std::cref(Content),
            std::cref(Config.song_list_format)));
        break;
    case NCM_DISPLAY_MODE_COLUMNS:
        Content.setItemDisplayer(std::bind(
            Display::SongsInColumns, ph::_1, std::cref(Content)));
        break;
    }

    w = &Playlists;

    native_c_screen_playlist_editor_init();
    {
        NativePlaylistEditorBridge bridge = {};

        bridge.active_window = nativeActiveWindowCallback;
        bridge.refresh = nativeRefreshCallback;
        bridge.refresh_window = nativeRefreshWindowCallback;
        bridge.scroll = nativeScrollCallback;
        bridge.switch_to = nativeSwitchToCallback;
        bridge.resize = nativeResizeCallback;
        bridge.window_timeout = nativeWindowTimeoutCallback;
        bridge.title = nativeTitleCallback;
        bridge.update = nativeUpdateCallback;
        bridge.mouse_button_pressed = nativeMouseButtonPressedCallback;
        bridge.user = this;
        native_playlist_editor_screen_set_bridge(
            native_c_screen_playlist_editor(), bridge);
    }
    screen_compat::bind_legacy_owner(nativeScreen(), this);
}

inline PlaylistEditor::~PlaylistEditor()
{
    NativePlaylistEditorBridge bridge = {};

    native_playlist_editor_screen_set_bridge(native_c_screen_playlist_editor(),
                                             bridge);
    screen_compat::bind_legacy_owner(nativeScreen(), nullptr);
    if (app_controller_is_screen_registered(nativeScreen()))
        app_controller_unregister_screen(nativeScreen());
}

inline void PlaylistEditor::resizeLegacyColumns()
{
    size_t x_offset;
    size_t width;
    size_t ra;
    size_t rb;

    getWindowResizeParams(x_offset, width);

    ra = Config.playlist_editor_column_width_ratio[0];
    rb = Config.playlist_editor_column_width_ratio[1];

    playlist_editor_compat::LeftColumnStartX = x_offset;
    playlist_editor_compat::LeftColumnWidth = width*ra/(ra+rb)-1;
    playlist_editor_compat::RightColumnStartX =
        playlist_editor_compat::LeftColumnStartX
        + playlist_editor_compat::LeftColumnWidth + 1;
    playlist_editor_compat::RightColumnWidth =
        width - playlist_editor_compat::LeftColumnWidth - 1;

    Playlists.resize(playlist_editor_compat::LeftColumnWidth,
                     static_cast<size_t>(ui_state_main_height()));
    Content.resize(playlist_editor_compat::RightColumnWidth,
                   static_cast<size_t>(ui_state_main_height()));

    Playlists.moveTo(playlist_editor_compat::LeftColumnStartX,
                     static_cast<size_t>(ui_state_main_start_y()));
    Content.moveTo(playlist_editor_compat::RightColumnStartX,
                   static_cast<size_t>(ui_state_main_start_y()));

    hasToBeResized = 0;
}

inline void PlaylistEditor::resize()
{
    syncNative();
    nc_screen_resize(nativeScreen());
}

inline std::string PlaylistEditor::title()
{
    return "Playlist editor";
}

inline void PlaylistEditor::refresh()
{
    syncNative();
    nc_screen_refresh(nativeScreen());
}

inline void PlaylistEditor::refreshWindow()
{
    syncNative();
    nc_screen_refresh_window(nativeScreen());
}

inline void PlaylistEditor::scroll(enum NcScroll where)
{
    w->scroll(where);
    syncNative();
}

inline void PlaylistEditor::switchTo()
{
    syncNative();
    nc_screen_set_has_to_be_resized(nativeScreen(), hasToBeResized);
    app_controller_switch_to_screen(nativeScreen());
    drawHeader();
    refresh();
}

inline NcScreen *PlaylistEditor::nativeScreen()
{
    return native_c_screen_playlist_editor_native();
}

inline const NcScreen *PlaylistEditor::nativeScreen() const
{
    return native_c_screen_playlist_editor_native();
}

inline void PlaylistEditor::update()
{
    {
        ScopedUnfilteredMenu<MPD::Playlist> sunfilter_playlists(
            ReapplyFilter::No, Playlists);
        if (Playlists.empty() || m_playlists_update_requested)
        {
            m_playlists_update_requested = false;
            sunfilter_playlists.set(ReapplyFilter::Yes, true);
            size_t idx = 0;
            NcmError error = {};
            NcmMpdPlaylistList playlists;

            ncm_mpd_playlist_list_init(&playlists);
            if (!ncm_mpd_client_get_playlists(&global_mpd, &playlists,
                                              &error))
            {
                Statusbar::printf("Could not fetch playlists: %s",
                                  error.message);
            }
            else
            {
                for (int32 i = 0; i < playlists.count; i += 1, idx += 1)
                {
                    MPD::Playlist playlist(&playlists.items[i]);

                    if (idx < Playlists.size())
                        Playlists[idx].value() = std::move(playlist);
                    else
                        Playlists.addItem(std::move(playlist));
                }
            }
            ncm_mpd_playlist_list_destroy(&playlists);
            if (idx < Playlists.size())
                Playlists.resizeList(idx);
            std::sort(Playlists.beginV(), Playlists.endV(),
                      LocaleBasedSorting(std::locale(),
                                         Config.ignore_leading_the));
        }
    }

    {
        ScopedUnfilteredMenu<MPD::Song> sunfilter_content(
            ReapplyFilter::No, Content);
        if (!Playlists.empty()
            && ((Content.empty()
                 && global_timer_elapsed_ms(m_timer) > m_fetching_delay_ms)
                || m_content_update_requested))
        {
            m_content_update_requested = false;
            sunfilter_content.set(ReapplyFilter::Yes, true);
            size_t idx = 0;
            MPD::SongIterator s = Mpd.GetPlaylistContent(
                Playlists.current()->value().path());
            MPD::SongIterator end;
            for (; s != end; ++s, ++idx)
            {
                if (idx < Content.size())
                    Content[idx].value() = std::move(*s);
                else
                    Content.addItem(std::move(*s));
            }
            if (idx < Content.size())
                Content.resizeList(idx);
            std::string wtitle;
            if (Config.titles_visibility)
            {
                wtitle = stringFormat("Content (%1% %2%)",
                                      Content.size(),
                                      Content.size() == 1
                                      ? "item"
                                      : "items");
                wtitle.resize(Content.getWidth());
            }
            Content.setTitle(wtitle);
            Content.refreshBorder();
        }
    }
    syncNative();
}

inline int PlaylistEditor::windowTimeout()
{
    ScopedUnfilteredMenu<MPD::Song> sunfilter_content(ReapplyFilter::No,
                                                      Content);
    if (Content.empty())
        return m_window_timeout;
    return Screen<WindowType>::windowTimeout();
}

inline void PlaylistEditor::mouseButtonPressed(MEVENT me)
{
    if (Playlists.hasCoords(me.x, me.y))
    {
        if (!isActiveWindow(Playlists))
        {
            if (previousColumnAvailable())
                previousColumn();
            else
                return;
        }
        if (size_t(me.y) < Playlists.size()
            && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
        {
            Playlists.Goto(me.y);
            if (me.bstate & BUTTON3_PRESSED)
                addItemToPlaylist(false);
        }
        else
            Screen<WindowType>::mouseButtonPressed(me);
        Content.clear();
    }
    else if (Content.hasCoords(me.x, me.y))
    {
        if (!isActiveWindow(Content))
        {
            if (nextColumnAvailable())
                nextColumn();
            else
                return;
        }
        if (size_t(me.y) < Content.size()
            && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
        {
            Content.Goto(me.y);
            bool play = me.bstate & BUTTON3_PRESSED;
            addItemToPlaylist(play);
        }
        else
            Screen<WindowType>::mouseButtonPressed(me);
    }
    syncNative();
}

inline bool PlaylistEditor::allowsSearching()
{
    return true;
}

inline const std::string &PlaylistEditor::searchConstraint()
{
    if (isActiveWindow(Playlists))
        return m_playlists_search_predicate.constraint();
    if (isActiveWindow(Content))
        return m_content_search_predicate.constraint();
    throw std::runtime_error("no active window");
}

inline void PlaylistEditor::setSearchConstraint(const std::string &constraint)
{
    if (isActiveWindow(Playlists))
    {
        m_playlists_search_predicate = Regex::Filter<MPD::Playlist>(
            constraint,
            Config.regex_type,
            playlist_editor_compat::PlaylistEntryMatcher);
    }
    else if (isActiveWindow(Content))
    {
        m_content_search_predicate = Regex::Filter<MPD::Song>(
            constraint,
            Config.regex_type,
            playlist_editor_compat::SongEntryMatcher);
    }
}

inline void PlaylistEditor::clearSearchConstraint()
{
    if (isActiveWindow(Playlists))
        m_playlists_search_predicate.clear();
    else if (isActiveWindow(Content))
        m_content_search_predicate.clear();
}

inline bool PlaylistEditor::search(SearchDirection direction, bool wrap,
                                   bool skip_current)
{
    bool result = false;
    if (isActiveWindow(Playlists))
        result = ::search(Playlists, m_playlists_search_predicate,
                          direction, wrap, skip_current);
    else if (isActiveWindow(Content))
        result = ::search(Content, m_content_search_predicate,
                          direction, wrap, skip_current);
    if (result)
        syncNative();
    return result;
}

inline bool PlaylistEditor::allowsFiltering()
{
    return allowsSearching();
}

inline std::string PlaylistEditor::currentFilter()
{
    std::string result;
    if (isActiveWindow(Playlists))
    {
        if (auto pred = Playlists.filterPredicate<
            Regex::Filter<MPD::Playlist>>())
            result = pred->constraint();
    }
    else if (isActiveWindow(Content))
    {
        if (auto pred = Content.filterPredicate<Regex::Filter<MPD::Song>>())
            result = pred->constraint();
    }
    return result;
}

inline void PlaylistEditor::applyFilter(const std::string &constraint)
{
    if (isActiveWindow(Playlists))
    {
        if (!constraint.empty())
        {
            Playlists.applyFilter(Regex::Filter<MPD::Playlist>(
                constraint,
                Config.regex_type,
                playlist_editor_compat::PlaylistEntryMatcher));
        }
        else
            Playlists.clearFilter();
    }
    else if (isActiveWindow(Content))
    {
        if (!constraint.empty())
        {
            Content.applyFilter(Regex::Filter<MPD::Song>(
                constraint,
                Config.regex_type,
                playlist_editor_compat::SongEntryMatcher));
        }
        else
            Content.clearFilter();
    }
    syncNative();
}

inline bool PlaylistEditor::itemAvailable()
{
    if (isActiveWindow(Playlists))
        return !Playlists.empty();
    if (isActiveWindow(Content))
        return !Content.empty();
    return false;
}

inline bool PlaylistEditor::addItemToPlaylist(bool play)
{
    bool success = false;
    if (isActiveWindow(Playlists))
    {
        const auto &playlist = Playlists.current()->value();
        success = Mpd.LoadPlaylist(playlist.path());
        if (play)
        {
            try
            {
                Mpd.Play(Status::State::playlistLength());
            }
            catch (MPD::ServerError &e)
            {
                if (e.code() != MPD_SERVER_ERROR_ARG)
                    throw;
            }
        }
        if (success)
            Statusbar::printf("Playlist \"%1%\" loaded", playlist.path());
    }
    else if (isActiveWindow(Content))
        success = addSongToPlaylist(Content.current()->value(), play);
    return success;
}

inline std::vector<MPD::Song> PlaylistEditor::getSelectedSongs()
{
    std::vector<MPD::Song> result;
    if (isActiveWindow(Playlists))
    {
        bool any_selected = false;
        for (auto &e : Playlists)
        {
            if (e.isSelected())
            {
                any_selected = true;
                std::copy(
                    std::make_move_iterator(
                        Mpd.GetPlaylistContent(e.value().path())),
                    std::make_move_iterator(MPD::SongIterator()),
                    std::back_inserter(result));
            }
        }
        ScopedUnfilteredMenu<MPD::Song> sunfilter_content(ReapplyFilter::No,
                                                          Content);
        if (!any_selected && !Playlists.empty())
            std::copy(Content.beginV(), Content.endV(),
                      std::back_inserter(result));
    }
    else if (isActiveWindow(Content))
        result = Content.getSelectedSongs();
    return result;
}

inline bool PlaylistEditor::previousColumnAvailable()
{
    if (isActiveWindow(Content))
    {
        ScopedUnfilteredMenu<MPD::Playlist> sunfilter_playlists(
            ReapplyFilter::No, Playlists);
        if (!Playlists.empty())
            return true;
    }
    return false;
}

inline void PlaylistEditor::previousColumn()
{
    if (isActiveWindow(Content))
    {
        setHighlightInactiveColumnFixes(Content);
        w->refresh();
        w = &Playlists;
        setHighlightFixes(Playlists);
        syncNative();
    }
}

inline bool PlaylistEditor::nextColumnAvailable()
{
    if (isActiveWindow(Playlists))
    {
        ScopedUnfilteredMenu<MPD::Song> sunfilter_content(ReapplyFilter::No,
                                                          Content);
        if (!Content.empty())
            return true;
    }
    return false;
}

inline void PlaylistEditor::nextColumn()
{
    if (isActiveWindow(Playlists))
    {
        setHighlightInactiveColumnFixes(Playlists);
        w->refresh();
        w = &Content;
        setHighlightFixes(Content);
        syncNative();
    }
}

inline void PlaylistEditor::updateTimer()
{
    m_timer = global_timer;
}

inline void PlaylistEditor::locatePlaylist(const MPD::Playlist &playlist)
{
    update();
    Playlists.clearFilter();
    auto first = Playlists.beginV();
    auto last = Playlists.endV();
    auto it = std::find(first, last, playlist);
    if (it != last)
    {
        Playlists.highlight(it - first);
        Content.clear();
        Content.clearFilter();
        switchTo();
    }
    syncNative();
}

inline void PlaylistEditor::locateSong(const MPD::Song &s)
{
    if (Playlists.empty())
        return;

    Content.clearFilter();
    Playlists.clearFilter();

    auto locate_song_in_current_playlist = [this, &s](auto front,
                                                      auto back) {
        if (!Content.empty())
        {
            auto it = std::find(front, back, s);
            if (it != back)
            {
                Content.highlight(it - Content.beginV());
                nextColumn();
                return true;
            }
        }
        return false;
    };
    auto locate_song_in_playlists = [this, &s](auto front, auto back) {
        for (auto it = front; it != back; ++it)
        {
            if (auto song_index =
                playlist_editor_compat::GetSongIndexInPlaylist(*it, s))
            {
                Playlists.highlight(it - Playlists.beginV());
                Playlists.refresh();

                requestContentUpdate();
                update();
                Content.highlight(*song_index);
                nextColumn();

                return true;
            }
        }
        return false;
    };

    if (locate_song_in_current_playlist(Content.currentV() + 1,
                                        Content.endV()))
        return;
    Statusbar::print("Jumping to song...");
    if (locate_song_in_playlists(Playlists.currentV() + 1,
                                 Playlists.endV()))
        return;
    if (locate_song_in_playlists(Playlists.beginV(), Playlists.currentV()))
        return;
    if (locate_song_in_current_playlist(Content.beginV(), Content.currentV()))
        return;

    if (Content.empty() || *Content.currentV() != s)
        Statusbar::print("Song was not found in playlists");
    syncNative();
}

inline void PlaylistEditor::syncNative()
{
    NativePlaylistEditorScreen *native;
    NcMenu *native_playlists;
    NcMenu *native_content;
    bool playlists_filtered;
    bool content_filtered;

    native = native_c_screen_playlist_editor();
    native_playlists = nc_playlist_entry_menu_base(
        native_playlist_editor_screen_playlists(native));
    native_content = nc_song_menu_base(
        native_playlist_editor_screen_content(native));
    playlists_filtered = Playlists.isFiltered();
    content_filtered = Content.isFiltered();

    if (playlists_filtered)
        Playlists.showAllItems();
    if (content_filtered)
        Content.showAllItems();

    nc_menu_clear_items(native_playlists);
    for (auto it = Playlists.begin(); it != Playlists.end(); ++it)
        playlist_editor_compat::addNativePlaylist(native, *it);
    if (!Playlists.empty())
        nc_menu_highlight_position(
            native_playlists,
            static_cast<int64>(Playlists.choice()),
            nc_window_height(&native->playlists_window));

    nc_menu_clear_items(native_content);
    for (auto it = Content.begin(); it != Content.end(); ++it)
        playlist_editor_compat::addNativeSong(native, *it);
    if (!Content.empty())
        nc_menu_highlight_position(native_content,
                                   static_cast<int64>(Content.choice()),
                                   nc_window_height(&native->content_window));

    if (w == &Content)
        native->active_column = NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT;
    else
        native->active_column = NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS;

    if (playlists_filtered)
        Playlists.showFilteredItems();
    if (content_filtered)
        Content.showFilteredItems();
}

inline NcWindow *PlaylistEditor::nativeActiveWindowCallback(void *user)
{
    PlaylistEditor *editor = static_cast<PlaylistEditor *>(user);

    if (editor == nullptr || editor->w == nullptr)
        return nullptr;
    return editor->w->nativeWindow();
}

inline void PlaylistEditor::nativeRefreshCallback(void *user)
{
    PlaylistEditor *editor = static_cast<PlaylistEditor *>(user);

    if (editor == nullptr)
        return;
    editor->Playlists.display();
    drawSeparator(playlist_editor_compat::RightColumnStartX - 1);
    editor->Content.display();
}

inline void PlaylistEditor::nativeRefreshWindowCallback(void *user)
{
    PlaylistEditor *editor = static_cast<PlaylistEditor *>(user);

    if (editor == nullptr || editor->w == nullptr)
        return;
    editor->w->display();
}

inline void PlaylistEditor::nativeScrollCallback(void *user,
                                                 enum NcScroll where)
{
    PlaylistEditor *editor = static_cast<PlaylistEditor *>(user);

    if (editor == nullptr)
        return;
    editor->scroll(where);
}

inline void PlaylistEditor::nativeSwitchToCallback(void *user)
{
    PlaylistEditor *editor = static_cast<PlaylistEditor *>(user);

    if (editor == nullptr)
        return;
    editor->syncNative();
}

inline void PlaylistEditor::nativeResizeCallback(void *user)
{
    PlaylistEditor *editor = static_cast<PlaylistEditor *>(user);

    if (editor == nullptr)
        return;
    editor->resizeLegacyColumns();
}

inline int32 PlaylistEditor::nativeWindowTimeoutCallback(void *user)
{
    PlaylistEditor *editor = static_cast<PlaylistEditor *>(user);

    if (editor == nullptr)
        return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
    return editor->windowTimeout();
}

inline char *PlaylistEditor::nativeTitleCallback(void *user)
{
    PlaylistEditor *editor = static_cast<PlaylistEditor *>(user);

    if (editor == nullptr)
        return nullptr;
    editor->m_title_cache = editor->title();
    return const_cast<char *>(editor->m_title_cache.c_str());
}

inline void PlaylistEditor::nativeUpdateCallback(void *user)
{
    PlaylistEditor *editor = static_cast<PlaylistEditor *>(user);

    if (editor == nullptr)
        return;
    editor->update();
}

inline void PlaylistEditor::nativeMouseButtonPressedCallback(void *user,
                                                            MEVENT event)
{
    PlaylistEditor *editor = static_cast<PlaylistEditor *>(user);

    if (editor == nullptr)
        return;
    editor->mouseButtonPressed(event);
}

namespace playlist_editor_compat {

inline std::string SongToString(const MPD::Song &s)
{
    std::string result;
    switch (Config.playlist_display_mode)
    {
    case NCM_DISPLAY_MODE_CLASSIC:
        result = Format::stringify<char>(Config.song_list_format, &s);
        break;
    case NCM_DISPLAY_MODE_COLUMNS:
        result = Format::stringify<char>(Config.song_columns_mode_format, &s);
        break;
    }
    return result;
}

inline bool PlaylistEntryMatcher(const Regex::Regex &rx,
                                 const MPD::Playlist &playlist)
{
    return Regex::search(playlist.path(), rx);
}

inline bool SongEntryMatcher(const Regex::Regex &rx, const MPD::Song &s)
{
    return Regex::search(SongToString(s), rx);
}

inline std::optional<size_t> GetSongIndexInPlaylist(
    MPD::Playlist playlist, const MPD::Song &song)
{
    size_t index = 0;
    MPD::SongIterator it = Mpd.GetPlaylistContentNoInfo(playlist.path());
    MPD::SongIterator end;

    for (;;)
    {
        if (it == end)
            return std::nullopt;
        if (*it == song)
            return index;

        ++it;
        ++index;
    }
}

}

#endif // NCMPCPP_PLAYLIST_EDITOR_H
