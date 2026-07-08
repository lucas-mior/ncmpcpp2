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

#ifndef NCMPCPP_SEARCH_ENGINE_H
#define NCMPCPP_SEARCH_ENGINE_H

#include <algorithm>
#include <cassert>
#include <functional>
#include <locale>
#include <string>
#include <vector>

#include "app_controller.h"
#include "curses/menu_impl.h"
#include "format_impl.h"
#include "global.h"
#include "helpers/song_iterator_maker.h"
#include "helpers_legacy.h"
#include "interfaces.h"
#include "mpdpp.h"
#include "regex_filter.h"
#include "screens/native_c_screens.h"
#include "screens/nc_search_engine.h"
#include "screens/playlist.h"
#include "screens/screen_cpp_compat.h"
#include "settings_legacy.h"
#include "song_list.h"
#include "status_legacy.h"
#include "statusbar_legacy.h"
#include "title_legacy.h"
#include "ui_state.h"
#include "utility/comparators.h"

struct SEItem
{
    SEItem() : m_is_song(false), m_buffer(0) { }
    SEItem(NC::Buffer *buf) : m_is_song(false), m_buffer(buf) { }
    SEItem(const MPD::Song &s) : m_is_song(true), m_song(s), m_buffer(0) { }
    SEItem(const SEItem &ei) : m_is_song(false), m_buffer(0) { *this = ei; }
    ~SEItem() {
        if (!m_is_song)
            delete m_buffer;
    }

    NC::Buffer &mkBuffer() {
        assert(!m_is_song);
        delete m_buffer;
        m_buffer = new NC::Buffer();
        return *m_buffer;
    }

    bool isSong() const { return m_is_song; }

    NC::Buffer &buffer() { assert(!m_is_song && m_buffer); return *m_buffer; }
    MPD::Song &song() { assert(m_is_song); return m_song; }

    const NC::Buffer &buffer() const { assert(!m_is_song && m_buffer); return *m_buffer; }
    const MPD::Song &song() const { assert(m_is_song); return m_song; }

    SEItem &operator=(const SEItem &se) {
        if (this == &se)
            return *this;
        if (!m_is_song)
            delete m_buffer;
        m_buffer = 0;
        m_is_song = se.m_is_song;
        if (se.m_is_song)
            m_song = se.m_song;
        else if (se.m_buffer)
            m_buffer = new NC::Buffer(*se.m_buffer);
        return *this;
    }

private:
    bool m_is_song;

    MPD::Song m_song;
    NC::Buffer *m_buffer;
};

struct SearchEngineWindow: NC::Menu<SEItem>, SongList
{
    SearchEngineWindow() { }
    SearchEngineWindow(NC::Menu<SEItem> &&base)
    : NC::Menu<SEItem>(std::move(base)) { }

    virtual SongIterator currentS() override;
    virtual ConstSongIterator currentS() const override;
    virtual SongIterator beginS() override;
    virtual ConstSongIterator beginS() const override;
    virtual SongIterator endS() override;
    virtual ConstSongIterator endS() const override;

    virtual std::vector<MPD::Song> getSelectedSongs() override;
};

namespace Display {
std::string Columns(size_t width);
void SEItems(NC::Menu<SEItem> &menu, const SongList &list);
}

struct SearchEngine: Screen<SearchEngineWindow>, Filterable, HasActions, HasSongs, Searchable, Tabbable
{
    SearchEngine();
    virtual ~SearchEngine();

    virtual void refresh() override;
    virtual void refreshWindow() override;
    virtual void scroll(enum NcScroll where) override;
    virtual void resize() override;
    virtual void switchTo() override;

    virtual std::string title() override;
    virtual ScreenType type() override { return NCM_SCREEN_TYPE_SEARCH_ENGINE; }
    virtual NcScreen *nativeScreen() override;
    virtual const NcScreen *nativeScreen() const override;

    virtual void update() override { }

    virtual void mouseButtonPressed(MEVENT me) override;

    virtual bool isLockable() override { return true; }
    virtual bool isMergable() override { return true; }

    virtual bool allowsSearching() override;
    virtual const std::string &searchConstraint() override;
    virtual void setSearchConstraint(const std::string &constraint) override;
    virtual void clearSearchConstraint() override;
    virtual bool search(SearchDirection direction, bool wrap, bool skip_current) override;

    virtual bool allowsFiltering() override;
    virtual std::string currentFilter() override;
    virtual void applyFilter(const std::string &filter) override;

    virtual bool actionRunnable() override;
    virtual void runAction() override;

    virtual bool itemAvailable() override;
    virtual bool addItemToPlaylist(bool play) override;
    virtual std::vector<MPD::Song> getSelectedSongs() override;

    void reset();

    static inline size_t StaticOptions = 20;
    static inline size_t SearchButton = 15;
    static inline size_t ResetButton = 16;

private:
    void Prepare();
    void Search();
    void syncNative();
    void resizeNativeWindow();
    static NcWindow *nativeActiveWindowCallback(void *user);
    static void nativeRefreshCallback(void *user);
    static void nativeRefreshWindowCallback(void *user);
    static void nativeScrollCallback(void *user, enum NcScroll where);
    static void nativeSwitchToCallback(void *user);
    static void nativeResizeCallback(void *user);
    static char *nativeTitleCallback(void *user);
    static void nativeUpdateCallback(void *user);
    static void nativeMouseButtonPressedCallback(void *user, MEVENT event);

    Regex::ItemFilter<SEItem> m_search_predicate;

    const char **SearchMode;

    static inline const char *SearchModes[] =
    {
        "Match if tag contains searched phrase (no regexes)",
        "Match if tag contains searched phrase (regexes supported)",
        "Match only if both values are the same",
        0,
    };

    static const size_t ConstraintsNumber = 11;
    static inline const char *ConstraintsNames[] =
    {
        "Any",
        "Artist",
        "Album Artist",
        "Title",
        "Album",
        "Filename",
        "Composer",
        "Performer",
        "Genre",
        "Date",
        "Comment",
    };
    std::string itsConstraints[ConstraintsNumber];
    std::string m_title_cache;
};

inline SearchEngine *mySearcher = nullptr;

namespace search_engine_compat {

inline std::string SEItemToString(const SEItem &ei);
inline bool SEItemEntryMatcher(const Regex::Regex &rx,
                               const NC::Menu<SEItem>::Item &item,
                               bool filter);
inline uint32 nativeFlags(const NC::Menu<SEItem>::Item &item);
inline bool addNativeItem(NativeSearchEngineScreen *native,
                          const NC::Menu<SEItem>::Item &item);

}

template <>
struct SongPropertiesExtractor<SEItem>
{
    template <typename ItemT>
    auto &operator()(ItemT &item) const
    {
        auto s = !item.isSeparator() && item.value().isSong()
            ? &item.value().song()
            : nullptr;
        return m_cache.assign(&item.properties(), s);
    }

private:
    mutable SongProperties m_cache;
};

inline SongIterator SearchEngineWindow::currentS()
{
    return makeSongIterator(current());
}

inline ConstSongIterator SearchEngineWindow::currentS() const
{
    return makeConstSongIterator(current());
}

inline SongIterator SearchEngineWindow::beginS()
{
    return makeSongIterator(begin());
}

inline ConstSongIterator SearchEngineWindow::beginS() const
{
    return makeConstSongIterator(begin());
}

inline SongIterator SearchEngineWindow::endS()
{
    return makeSongIterator(end());
}

inline ConstSongIterator SearchEngineWindow::endS() const
{
    return makeConstSongIterator(end());
}

inline std::vector<MPD::Song> SearchEngineWindow::getSelectedSongs()
{
    std::vector<MPD::Song> result;
    for (auto &item : *this)
    {
        if (item.isSelected())
        {
            assert(item.value().isSong());
            result.push_back(item.value().song());
        }
    }
    if (result.empty() && !empty() && current()->value().isSong())
        result.push_back(current()->value().song());
    return result;
}

inline SearchEngine::SearchEngine()
: Screen(NC::Menu<SEItem>(0,
                          static_cast<size_t>(ui_state_main_start_y()),
                          COLS,
                          static_cast<size_t>(ui_state_main_height()),
                          "",
                          Config.main_color,
                          NC::Border()))
{
    w.setHighlightPrefix(Config.current_item_prefix);
    w.setHighlightSuffix(Config.current_item_suffix);
    w.cyclicScrolling(Config.use_cyclic_scrolling);
    w.centeredCursor(Config.centered_cursor);
    w.setItemDisplayer(std::bind(Display::SEItems,
                                 std::placeholders::_1,
                                 std::cref(w)));
    w.setSelectedPrefix(Config.selected_item_prefix);
    w.setSelectedSuffix(Config.selected_item_suffix);
    SearchMode = &SearchModes[Config.search_engine_default_search_mode];

    native_c_screen_search_engine_init();
    {
        NativeSearchEngineBridge bridge = {};

        bridge.active_window = nativeActiveWindowCallback;
        bridge.refresh = nativeRefreshCallback;
        bridge.refresh_window = nativeRefreshWindowCallback;
        bridge.scroll = nativeScrollCallback;
        bridge.switch_to = nativeSwitchToCallback;
        bridge.resize = nativeResizeCallback;
        bridge.title = nativeTitleCallback;
        bridge.update = nativeUpdateCallback;
        bridge.mouse_button_pressed = nativeMouseButtonPressedCallback;
        bridge.user = this;
        native_search_engine_screen_set_bridge(native_c_screen_search_engine(),
                                               bridge);
    }
    screen_compat::bind_legacy_owner(nativeScreen(), this);
}

inline SearchEngine::~SearchEngine()
{
    NativeSearchEngineBridge bridge = {};

    native_search_engine_screen_set_bridge(native_c_screen_search_engine(),
                                           bridge);
    screen_compat::bind_legacy_owner(nativeScreen(), nullptr);
    if (app_controller_is_screen_registered(nativeScreen()))
        app_controller_unregister_screen(nativeScreen());
}

inline void SearchEngine::refresh()
{
    syncNative();
    nc_screen_refresh(nativeScreen());
}

inline void SearchEngine::refreshWindow()
{
    syncNative();
    nc_screen_refresh_window(nativeScreen());
}

inline void SearchEngine::scroll(enum NcScroll where)
{
    w.scroll(where);
    syncNative();
}

inline void SearchEngine::resize()
{
    nc_screen_resize(nativeScreen());
}

inline void SearchEngine::switchTo()
{
    if (w.empty())
        Prepare();
    syncNative();
    nc_screen_set_has_to_be_resized(nativeScreen(), hasToBeResized);
    app_controller_switch_to_screen(nativeScreen());
    drawHeader();
}

inline std::string SearchEngine::title()
{
    return "Search engine";
}

inline NcScreen *SearchEngine::nativeScreen()
{
    return native_c_screen_search_engine_native();
}

inline const NcScreen *SearchEngine::nativeScreen() const
{
    return native_c_screen_search_engine_native();
}

inline void SearchEngine::mouseButtonPressed(MEVENT me)
{
    if (w.empty() || !w.hasCoords(me.x, me.y) || size_t(me.y) >= w.size())
        return;
    if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
    {
        if (!w.Goto(me.y))
            return;
        w.refresh();
        if ((me.bstate & BUTTON3_PRESSED) && w.choice() < StaticOptions)
            runAction();
        else if (w.choice() >= StaticOptions)
        {
            bool play = me.bstate & BUTTON3_PRESSED;
            addItemToPlaylist(play);
        }
        syncNative();
    }
    else
        Screen<WindowType>::mouseButtonPressed(me);
}

inline bool SearchEngine::allowsSearching()
{
    ScopedUnfilteredMenu<SEItem> sunfilter(ReapplyFilter::Yes, w);
    return !w.empty() && w.rbegin()->value().isSong();
}

inline const std::string &SearchEngine::searchConstraint()
{
    return m_search_predicate.constraint();
}

inline void SearchEngine::setSearchConstraint(const std::string &constraint)
{
    m_search_predicate = Regex::ItemFilter<SEItem>(
        constraint,
        Config.regex_type,
        std::bind(search_engine_compat::SEItemEntryMatcher,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  false));
}

inline void SearchEngine::clearSearchConstraint()
{
    m_search_predicate.clear();
}

inline bool SearchEngine::search(SearchDirection direction, bool wrap,
                                 bool skip_current)
{
    bool found;

    found = ::search(w, m_search_predicate, direction, wrap, skip_current);
    if (found)
        syncNative();
    return found;
}

inline bool SearchEngine::allowsFiltering()
{
    return allowsSearching();
}

inline std::string SearchEngine::currentFilter()
{
    std::string result;
    if (auto pred = w.filterPredicate<Regex::ItemFilter<SEItem>>())
        result = pred->constraint();
    return result;
}

inline void SearchEngine::applyFilter(const std::string &constraint)
{
    if (!constraint.empty())
    {
        w.applyFilter(Regex::ItemFilter<SEItem>(
                          constraint,
                          Config.regex_type,
                          std::bind(search_engine_compat::SEItemEntryMatcher,
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    true)));
    }
    else
        w.clearFilter();
    syncNative();
}

inline bool SearchEngine::actionRunnable()
{
    return !w.empty() && !w.current()->value().isSong();
}

inline void SearchEngine::runAction()
{
    size_t option = w.choice();
    if (option > ConstraintsNumber && option < SearchButton)
        w.current()->value().buffer().clear();

    if (option < ConstraintsNumber)
    {
        Statusbar::ScopedLock slock;
        std::string constraint = ConstraintsNames[option];
        Statusbar::put() << NC_FORMAT_BOLD << constraint << NC_FORMAT_NO_BOLD << ": ";
        itsConstraints[option] = static_cast<NC::Window *>(ui_state_footer_window())->prompt(itsConstraints[option]);
        w.current()->value().buffer().clear();
        constraint.resize(13, ' ');
        w.current()->value().buffer() << NC_FORMAT_BOLD << constraint << NC_FORMAT_NO_BOLD << ": ";
        ShowTag(w.current()->value().buffer(), itsConstraints[option]);
    }
    else if (option == ConstraintsNumber+1)
    {
        Config.search_in_db = !Config.search_in_db;
        w.current()->value().buffer() << NC_FORMAT_BOLD << "Search in:" << NC_FORMAT_NO_BOLD << ' ' << (Config.search_in_db ? "Database" : "Current playlist");
    }
    else if (option == ConstraintsNumber+2)
    {
        if (!*++SearchMode)
            SearchMode = &SearchModes[0];
        w.current()->value().buffer() << NC_FORMAT_BOLD << "Search mode:" << NC_FORMAT_NO_BOLD << ' ' << *SearchMode;
    }
    else if (option == SearchButton)
    {
        w.clearFilter();
        Statusbar::print("Searching...");
        if (w.size() > StaticOptions)
            Prepare();
        Search();
        if (w.rbegin()->value().isSong())
        {
            if (Config.search_engine_display_mode == NCM_DISPLAY_MODE_COLUMNS)
                w.setTitle(Config.titles_visibility ? Display::Columns(w.getWidth()) : "");
            size_t found = w.size()-SearchEngine::StaticOptions;
            found += 3;
            w.insertSeparator(ResetButton+1);
            w.insertItem(ResetButton+2, SEItem(), NC::List::Properties::Inactive);
            w.at(ResetButton+2).value().mkBuffer()
                << NC_FORMAT_BOLD
                << Config.color1
                << "Search results: "
                << NC::FormattedColor::End<>(Config.color1)
                << Config.color2
                << "Found " << found << (found > 1 ? " songs" : " song")
                << NC::FormattedColor::End<>(Config.color2)
                << NC_FORMAT_NO_BOLD;
            w.insertSeparator(ResetButton+3);
            Statusbar::print("Searching finished");
            if (Config.block_search_constraints_change)
                for (size_t i = 0; i < StaticOptions-4; ++i)
                    w.at(i).setInactive(true);
            w.scroll(NC_SCROLL_DOWN);
            w.scroll(NC_SCROLL_DOWN);
        }
        else
            Statusbar::print("No results found");
    }
    else if (option == ResetButton)
    {
        reset();
    }
    else
        addSongToPlaylist(w.current()->value().song(), true);
    syncNative();
}

inline bool SearchEngine::itemAvailable()
{
    return !w.empty() && w.current()->value().isSong();
}

inline bool SearchEngine::addItemToPlaylist(bool play)
{
    return addSongToPlaylist(w.current()->value().song(), play);
}

inline std::vector<MPD::Song> SearchEngine::getSelectedSongs()
{
    return w.getSelectedSongs();
}

inline void SearchEngine::Prepare()
{
    w.setTitle("");
    w.clear();
    w.resizeList(StaticOptions-3);

    for (auto &item : w)
        item.setSelectable(false);

    w.at(ConstraintsNumber).setSeparator(true);
    w.at(SearchButton-1).setSeparator(true);

    for (size_t i = 0; i < ConstraintsNumber; ++i)
    {
        std::string constraint = ConstraintsNames[i];
        constraint.resize(13, ' ');
        w[i].value().mkBuffer() << NC_FORMAT_BOLD << constraint << NC_FORMAT_NO_BOLD << ": ";
        ShowTag(w[i].value().buffer(), itsConstraints[i]);
    }

    w.at(ConstraintsNumber+1).value().mkBuffer() << NC_FORMAT_BOLD << "Search in:" << NC_FORMAT_NO_BOLD << ' ' << (Config.search_in_db ? "Database" : "Current playlist");
    w.at(ConstraintsNumber+2).value().mkBuffer() << NC_FORMAT_BOLD << "Search mode:" << NC_FORMAT_NO_BOLD << ' ' << *SearchMode;

    w.at(SearchButton).value().mkBuffer() << "Search";
    w.at(ResetButton).value().mkBuffer() << "Reset";
    syncNative();
}

inline void SearchEngine::reset()
{
    for (size_t i = 0; i < ConstraintsNumber; ++i)
        itsConstraints[i].clear();
    w.clearFilter();
    w.reset();
    Prepare();
    Statusbar::print("Search state reset");
    syncNative();
}

inline void SearchEngine::Search()
{
    bool constraints_empty = 1;
    for (size_t i = 0; i < ConstraintsNumber; ++i)
    {
        if (!itsConstraints[i].empty())
        {
            constraints_empty = 0;
            break;
        }
    }
    if (constraints_empty)
        return;

    if (Config.search_in_db && (SearchMode == &SearchModes[0] || SearchMode == &SearchModes[2]))
    {
        Mpd.StartSearch(SearchMode == &SearchModes[2]);
        if (!itsConstraints[0].empty())
            Mpd.AddSearchAny(itsConstraints[0]);
        if (!itsConstraints[1].empty())
            Mpd.AddSearch(MPD_TAG_ARTIST, itsConstraints[1]);
        if (!itsConstraints[2].empty())
            Mpd.AddSearch(MPD_TAG_ALBUM_ARTIST, itsConstraints[2]);
        if (!itsConstraints[3].empty())
            Mpd.AddSearch(MPD_TAG_TITLE, itsConstraints[3]);
        if (!itsConstraints[4].empty())
            Mpd.AddSearch(MPD_TAG_ALBUM, itsConstraints[4]);
        if (!itsConstraints[5].empty())
            Mpd.AddSearchURI(itsConstraints[5]);
        if (!itsConstraints[6].empty())
            Mpd.AddSearch(MPD_TAG_COMPOSER, itsConstraints[6]);
        if (!itsConstraints[7].empty())
            Mpd.AddSearch(MPD_TAG_PERFORMER, itsConstraints[7]);
        if (!itsConstraints[8].empty())
            Mpd.AddSearch(MPD_TAG_GENRE, itsConstraints[8]);
        if (!itsConstraints[9].empty())
            Mpd.AddSearch(MPD_TAG_DATE, itsConstraints[9]);
        if (!itsConstraints[10].empty())
            Mpd.AddSearch(MPD_TAG_COMMENT, itsConstraints[10]);
        for (MPD::SongIterator s = Mpd.CommitSearchSongs(), end; s != end; ++s)
            w.addItem(std::move(*s));
        return;
    }

    Regex::Regex rx[ConstraintsNumber];
    if (SearchMode != &SearchModes[2])
    {
        for (size_t i = 0; i < ConstraintsNumber; ++i)
        {
            if (!itsConstraints[i].empty())
            {
                try
                {
                    rx[i] = Regex::make(itsConstraints[i], Config.regex_type);
                }
                catch (Regex::Error &) { }
            }
        }
    }

    LocaleStringComparison cmp(std::locale(), Config.ignore_leading_the);
    auto searchSongs = [&](auto s, auto end) {
        for (; s != end; ++s)
        {
            bool any_found = true, found = true;

            if (SearchMode != &SearchModes[2])
            {
                if (!rx[0].empty())
                    any_found =
                           Regex::search(s->getArtist(), rx[0])
                        || Regex::search(s->getAlbumArtist(), rx[0])
                        || Regex::search(s->getTitle(), rx[0])
                        || Regex::search(s->getAlbum(), rx[0])
                        || Regex::search(s->getName(), rx[0])
                        || Regex::search(s->getComposer(), rx[0])
                        || Regex::search(s->getPerformer(), rx[0])
                        || Regex::search(s->getGenre(), rx[0])
                        || Regex::search(s->getDate(), rx[0])
                        || Regex::search(s->getComment(), rx[0]);
                if (found && !rx[1].empty())
                    found = Regex::search(s->getArtist(), rx[1]);
                if (found && !rx[2].empty())
                    found = Regex::search(s->getAlbumArtist(), rx[2]);
                if (found && !rx[3].empty())
                    found = Regex::search(s->getTitle(), rx[3]);
                if (found && !rx[4].empty())
                    found = Regex::search(s->getAlbum(), rx[4]);
                if (found && !rx[5].empty())
                    found = Regex::search(s->getName(), rx[5]);
                if (found && !rx[6].empty())
                    found = Regex::search(s->getComposer(), rx[6]);
                if (found && !rx[7].empty())
                    found = Regex::search(s->getPerformer(), rx[7]);
                if (found && !rx[8].empty())
                    found = Regex::search(s->getGenre(), rx[8]);
                if (found && !rx[9].empty())
                    found = Regex::search(s->getDate(), rx[9]);
                if (found && !rx[10].empty())
                    found = Regex::search(s->getComment(), rx[10]);
            }
            else
            {
                if (!itsConstraints[0].empty())
                    any_found =
                       !cmp(s->getArtist(), itsConstraints[0])
                    || !cmp(s->getAlbumArtist(), itsConstraints[0])
                    || !cmp(s->getTitle(), itsConstraints[0])
                    || !cmp(s->getAlbum(), itsConstraints[0])
                    || !cmp(s->getName(), itsConstraints[0])
                    || !cmp(s->getComposer(), itsConstraints[0])
                    || !cmp(s->getPerformer(), itsConstraints[0])
                    || !cmp(s->getGenre(), itsConstraints[0])
                    || !cmp(s->getDate(), itsConstraints[0])
                    || !cmp(s->getComment(), itsConstraints[0]);

                if (found && !itsConstraints[1].empty())
                    found = !cmp(s->getArtist(), itsConstraints[1]);
                if (found && !itsConstraints[2].empty())
                    found = !cmp(s->getAlbumArtist(), itsConstraints[2]);
                if (found && !itsConstraints[3].empty())
                    found = !cmp(s->getTitle(), itsConstraints[3]);
                if (found && !itsConstraints[4].empty())
                    found = !cmp(s->getAlbum(), itsConstraints[4]);
                if (found && !itsConstraints[5].empty())
                    found = !cmp(s->getName(), itsConstraints[5]);
                if (found && !itsConstraints[6].empty())
                    found = !cmp(s->getComposer(), itsConstraints[6]);
                if (found && !itsConstraints[7].empty())
                    found = !cmp(s->getPerformer(), itsConstraints[7]);
                if (found && !itsConstraints[8].empty())
                    found = !cmp(s->getGenre(), itsConstraints[8]);
                if (found && !itsConstraints[9].empty())
                    found = !cmp(s->getDate(), itsConstraints[9]);
                if (found && !itsConstraints[10].empty())
                    found = !cmp(s->getComment(), itsConstraints[10]);
            }

            if (any_found && found)
                w.addItem(*s);
        }
    };

    if (Config.search_in_db)
        searchSongs(Mpd.GetDirectoryRecursive("/"), MPD::SongIterator());
    else
        searchSongs(myPlaylist->main().beginV(), myPlaylist->main().endV());
}

inline void SearchEngine::syncNative()
{
    NativeSearchEngineScreen *native;
    NcMenu *native_menu;
    bool was_filtered;

    native = native_c_screen_search_engine();
    native_menu = native_search_engine_screen_menu(native);
    was_filtered = w.isFiltered();
    if (was_filtered)
        w.showAllItems();

    native_search_engine_screen_clear(native);
    for (auto it = w.begin(); it != w.end(); ++it)
        search_engine_compat::addNativeItem(native, *it);
    if (!w.empty())
        nc_menu_highlight_position(
            native_menu,
            static_cast<int64>(w.choice()),
            nc_window_height(native_search_engine_screen_window(native)));
    for (size_t i = 0; i < ConstraintsNumber; ++i)
        native_search_engine_screen_set_constraint(
            native,
            static_cast<int32>(i),
            const_cast<char *>(itsConstraints[i].c_str()),
            static_cast<int32>(itsConstraints[i].size()));

    if (was_filtered)
        w.showFilteredItems();
}

inline void SearchEngine::resizeNativeWindow()
{
    size_t x_offset;
    size_t width;

    getWindowResizeParams(x_offset, width);
    w.resize(width, static_cast<size_t>(ui_state_main_height()));
    w.moveTo(x_offset, static_cast<size_t>(ui_state_main_start_y()));
    switch (Config.search_engine_display_mode)
    {
    case NCM_DISPLAY_MODE_COLUMNS:
        if (Config.titles_visibility)
            w.setTitle(Display::Columns(w.getWidth()));
        break;
    case NCM_DISPLAY_MODE_CLASSIC:
        w.setTitle("");
        break;
    }
    hasToBeResized = false;
}

inline NcWindow *SearchEngine::nativeActiveWindowCallback(void *user)
{
    SearchEngine *searcher = static_cast<SearchEngine *>(user);

    if (searcher == nullptr)
        return nullptr;
    return searcher->w.nativeWindow();
}

inline void SearchEngine::nativeRefreshCallback(void *user)
{
    SearchEngine *searcher = static_cast<SearchEngine *>(user);

    if (searcher == nullptr)
        return;
    searcher->w.display();
}

inline void SearchEngine::nativeRefreshWindowCallback(void *user)
{
    nativeRefreshCallback(user);
}

inline void SearchEngine::nativeScrollCallback(void *user, enum NcScroll where)
{
    SearchEngine *searcher = static_cast<SearchEngine *>(user);

    if (searcher == nullptr)
        return;
    searcher->scroll(where);
}

inline void SearchEngine::nativeSwitchToCallback(void *user)
{
    SearchEngine *searcher = static_cast<SearchEngine *>(user);

    if (searcher == nullptr)
        return;
    if (searcher->w.empty())
        searcher->Prepare();
    searcher->syncNative();
}

inline void SearchEngine::nativeResizeCallback(void *user)
{
    SearchEngine *searcher = static_cast<SearchEngine *>(user);

    if (searcher == nullptr)
        return;
    searcher->resizeNativeWindow();
}

inline char *SearchEngine::nativeTitleCallback(void *user)
{
    SearchEngine *searcher = static_cast<SearchEngine *>(user);

    if (searcher == nullptr)
        return nullptr;
    searcher->m_title_cache = searcher->title();
    return const_cast<char *>(searcher->m_title_cache.c_str());
}

inline void SearchEngine::nativeUpdateCallback(void *user)
{
    SearchEngine *searcher = static_cast<SearchEngine *>(user);

    if (searcher == nullptr)
        return;
    searcher->update();
}

inline void SearchEngine::nativeMouseButtonPressedCallback(void *user,
                                                          MEVENT event)
{
    SearchEngine *searcher = static_cast<SearchEngine *>(user);

    if (searcher == nullptr)
        return;
    searcher->mouseButtonPressed(event);
}

namespace search_engine_compat {

inline std::string SEItemToString(const SEItem &ei)
{
    std::string result;
    if (ei.isSong())
    {
        switch (Config.search_engine_display_mode)
        {
        case NCM_DISPLAY_MODE_CLASSIC:
            result = Format::stringify<char>(Config.song_list_format,
                                             &ei.song());
            break;
        case NCM_DISPLAY_MODE_COLUMNS:
            result = Format::stringify<char>(
                Config.song_columns_mode_format,
                &ei.song());
            break;
        }
    }
    else
        result = ei.buffer().str();
    return result;
}

inline bool SEItemEntryMatcher(const Regex::Regex &rx,
                               const NC::Menu<SEItem>::Item &item,
                               bool filter)
{
    if (item.isSeparator() || !item.value().isSong())
        return filter;
    return Regex::search(SEItemToString(item.value()), rx);
}

inline uint32 nativeFlags(const NC::Menu<SEItem>::Item &item)
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

inline bool addNativeItem(NativeSearchEngineScreen *native,
                          const NC::Menu<SEItem>::Item &item)
{
    NC::Buffer empty;
    uint32 flags;

    flags = nativeFlags(item);
    if (item.isSeparator())
        return native_search_engine_screen_add_buffer_with_flags(
            native,
            const_cast<NcBuffer *>(empty.cBuffer()),
            flags);
    if (item.value().isSong())
        return native_search_engine_screen_add_song_copy_with_flags(
            native,
            item.value().song().cSong(),
            flags);
    return native_search_engine_screen_add_buffer_with_flags(
        native,
        const_cast<NcBuffer *>(item.value().buffer().cBuffer()),
        flags);
}

}

#endif // NCMPCPP_SEARCH_ENGINE_H
