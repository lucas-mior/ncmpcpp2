#ifndef NCMPCPP_SEARCH_ENGINE_H
#define NCMPCPP_SEARCH_ENGINE_H

#include <algorithm>
#include <cassert>
#include <functional>
#include <locale>
#include <string>
#include <vector>

#include "actions.h"
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
#include "statusbar.h"
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

private:
    void Prepare();
    void Search();
    void syncNative();
    void syncLegacyStaticRows();
    void syncLegacyRow(int32 row);
    void syncLegacyFindConstraint();
    void resizeNativeWindow();
    static NcWindow *nativeActiveWindowCallback(void *user);
    static void nativeRefreshCallback(void *user);
    static void nativeRefreshWindowCallback(void *user);
    static void nativeScrollCallback(void *user, enum NcScroll where);
    static void nativeSwitchToCallback(void *user);
    static void nativeResizeCallback(void *user);
    static void nativeUpdateCallback(void *user);
    static void nativeMouseButtonPressedCallback(void *user, MEVENT event);
    static bool nativeCanRunCurrentCallback(void *user);
    static bool nativeRunCurrentCallback(void *user) noexcept;
    static void nativeStaticRowChangedCallback(void *user, int32 row);

    // Temporary matcher for the legacy C++ menu. The native screen owns
    // the persisted search constraint.
    Regex::ItemFilter<SEItem> m_search_predicate;

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
    native_c_screen_search_engine_init();
    {
        NativeSearchEngineBridge bridge = {};

        bridge.active_window = nativeActiveWindowCallback;
        bridge.refresh = nativeRefreshCallback;
        bridge.refresh_window = nativeRefreshWindowCallback;
        bridge.scroll = nativeScrollCallback;
        bridge.switch_to = nativeSwitchToCallback;
        bridge.resize = nativeResizeCallback;
        bridge.update = nativeUpdateCallback;
        bridge.mouse_button_pressed = nativeMouseButtonPressedCallback;
        bridge.can_run_current = nativeCanRunCurrentCallback;
        bridge.run_current = nativeRunCurrentCallback;
        bridge.static_row_changed = nativeStaticRowChangedCallback;
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
    NativeSearchEngineScreen *native;

    native = native_c_screen_search_engine();
    if (!native_search_engine_screen_is_prepared(native) || w.empty())
        Prepare();
    else
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
        if ((me.bstate & BUTTON3_PRESSED) && w.choice() < NATIVE_SEARCH_ENGINE_STATIC_ROW_COUNT)
            runAction();
        else if (w.choice() >= NATIVE_SEARCH_ENGINE_STATIC_ROW_COUNT)
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
    NativeSearchEngineScreen *native;

    native = native_c_screen_search_engine();
    native_search_engine_screen_set_find_constraint(
        native, const_cast<char *>(constraint.c_str()),
        static_cast<int32>(constraint.size()));
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
    native_search_engine_screen_clear_find_constraint(
        native_c_screen_search_engine());
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
    NativeSearchEngineScreen *native;
    size_t option;

    native = native_c_screen_search_engine();
    option = w.choice();
    if (option < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT)
    {
        NcmStringView view;
        Statusbar::ScopedLock slock;
        char *name;
        std::string constraint;
        std::string value;

        view = native_search_engine_screen_constraint(
            native, static_cast<int32>(option));
        value.assign(view.data == nullptr ? "" : view.data,
                     static_cast<size_t>(view.len));
        name = native_search_engine_constraint_name(
            static_cast<int32>(option));
        constraint = name;
        Statusbar::put() << NC_FORMAT_BOLD << constraint
                         << NC_FORMAT_NO_BOLD << ": ";
        value = static_cast<NC::Window *>(
            ui_state_footer_legacy_window())->prompt(value);
        native_search_engine_screen_set_constraint(
            native, static_cast<int32>(option),
            const_cast<char *>(value.c_str()),
            static_cast<int32>(value.size()));

    }
    else if (option == NATIVE_SEARCH_ENGINE_SEARCH_SOURCE_ROW)
    {
        bool search_in_database;

        search_in_database =
            !native_search_engine_screen_searches_database(native);
        native_search_engine_screen_set_search_source(
            native, search_in_database);
        Config.search_in_db = search_in_database;
    }
    else if (option == NATIVE_SEARCH_ENGINE_SEARCH_MODE_ROW)
    {
        enum NativeSearchEngineSearchMode mode;
        uint32 next_mode;

        mode = native_search_engine_screen_search_mode(native);
        next_mode = static_cast<uint32>(mode) + 1;
        if (next_mode >= NATIVE_SEARCH_ENGINE_SEARCH_MODE_LAST)
            next_mode = NATIVE_SEARCH_ENGINE_SEARCH_MODE_LITERAL;
        mode = static_cast<enum NativeSearchEngineSearchMode>(next_mode);
        native_search_engine_screen_set_search_mode(native, mode);
    }
    else if (option == NATIVE_SEARCH_ENGINE_SEARCH_BUTTON_ROW)
    {
        w.clearFilter();
        Statusbar::print("Searching...");
        if (w.size() > NATIVE_SEARCH_ENGINE_STATIC_ROW_COUNT)
            Prepare();
        Search();
        if (w.rbegin()->value().isSong())
        {
            if (Config.search_engine_display_mode == NCM_DISPLAY_MODE_COLUMNS)
                w.setTitle(Config.titles_visibility
                    ? Display::Columns(w.getWidth())
                    : "");
            size_t found = w.size()-NATIVE_SEARCH_ENGINE_STATIC_ROW_COUNT;
            found += 3;
            w.insertSeparator(NATIVE_SEARCH_ENGINE_RESET_BUTTON_ROW+1);
            w.insertItem(NATIVE_SEARCH_ENGINE_RESET_BUTTON_ROW+2,
                         SEItem(), NC::List::Properties::Inactive);
            w.at(NATIVE_SEARCH_ENGINE_RESET_BUTTON_ROW+2).value().mkBuffer()
                << NC_FORMAT_BOLD
                << Config.color1
                << "Search results: "
                << NC::FormattedColor::End<>(Config.color1)
                << Config.color2
                << "Found " << found << (found > 1 ? " songs" : " song")
                << NC::FormattedColor::End<>(Config.color2)
                << NC_FORMAT_NO_BOLD;
            w.insertSeparator(NATIVE_SEARCH_ENGINE_RESET_BUTTON_ROW+3);
            Statusbar::print("Searching finished");
            if (Config.block_search_constraints_change)
                for (size_t i = 0;
                     i < NATIVE_SEARCH_ENGINE_STATIC_ROW_COUNT-4; ++i)
                    w.at(i).setInactive(true);
            w.scroll(NC_SCROLL_DOWN);
            w.scroll(NC_SCROLL_DOWN);
        }
        else
            Statusbar::print("No results found");
    }
    else if (option == NATIVE_SEARCH_ENGINE_RESET_BUTTON_ROW)
    {
        reset();
    }
    else
        ncm_action_add_song_to_playlist(
            w.current()->value().song().cSong(), true, -1);
    syncNative();
}

inline bool SearchEngine::itemAvailable()
{
    return !w.empty() && w.current()->value().isSong();
}

inline bool SearchEngine::addItemToPlaylist(bool play)
{
    return ncm_action_add_song_to_playlist(
        w.current()->value().song().cSong(), play, -1);
}

inline std::vector<MPD::Song> SearchEngine::getSelectedSongs()
{
    return w.getSelectedSongs();
}

inline void SearchEngine::Prepare()
{
    native_search_engine_screen_prepare_static_rows(
        native_c_screen_search_engine());
}

inline void SearchEngine::reset()
{
    native_search_engine_screen_reset(native_c_screen_search_engine());
}

inline void SearchEngine::Search()
{
    NativeSearchEngineScreen *native;
    enum NativeSearchEngineSearchMode mode;
    std::string constraints[NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT];
    bool search_in_database;
    bool constraints_empty = 1;

    native = native_c_screen_search_engine();
    mode = native_search_engine_screen_search_mode(native);
    search_in_database =
        native_search_engine_screen_searches_database(native);
    for (int32 i = 0; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; ++i)
    {
        NcmStringView view;

        view = native_search_engine_screen_constraint(native, i);
        constraints[i].assign(view.data == nullptr ? "" : view.data,
                              static_cast<size_t>(view.len));
    }
    for (size_t i = 0; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; ++i)
    {
        if (!constraints[i].empty())
        {
            constraints_empty = 0;
            break;
        }
    }
    if (constraints_empty)
        return;

    if (search_in_database && (mode == NATIVE_SEARCH_ENGINE_SEARCH_MODE_LITERAL
        || mode == NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT))
    {
        Mpd.StartSearch(mode == NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT);
        if (!constraints[0].empty())
            Mpd.AddSearchAny(constraints[0]);
        if (!constraints[1].empty())
            Mpd.AddSearch(MPD_TAG_ARTIST, constraints[1]);
        if (!constraints[2].empty())
            Mpd.AddSearch(MPD_TAG_ALBUM_ARTIST, constraints[2]);
        if (!constraints[3].empty())
            Mpd.AddSearch(MPD_TAG_TITLE, constraints[3]);
        if (!constraints[4].empty())
            Mpd.AddSearch(MPD_TAG_ALBUM, constraints[4]);
        if (!constraints[5].empty())
            Mpd.AddSearchURI(constraints[5]);
        if (!constraints[6].empty())
            Mpd.AddSearch(MPD_TAG_COMPOSER, constraints[6]);
        if (!constraints[7].empty())
            Mpd.AddSearch(MPD_TAG_PERFORMER, constraints[7]);
        if (!constraints[8].empty())
            Mpd.AddSearch(MPD_TAG_GENRE, constraints[8]);
        if (!constraints[9].empty())
            Mpd.AddSearch(MPD_TAG_DATE, constraints[9]);
        if (!constraints[10].empty())
            Mpd.AddSearch(MPD_TAG_COMMENT, constraints[10]);
        for (MPD::SongIterator s = Mpd.CommitSearchSongs(), end; s != end; ++s)
            w.addItem(std::move(*s));
        return;
    }

    Regex::Regex rx[NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT];
    if (mode != NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT)
    {
        for (size_t i = 0; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; ++i)
        {
            if (!constraints[i].empty())
            {
                try
                {
                    rx[i] = Regex::make(constraints[i], Config.regex_type);
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

            if (mode != NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT)
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
                if (!constraints[0].empty())
                    any_found =
                       !cmp(s->getArtist(), constraints[0])
                    || !cmp(s->getAlbumArtist(), constraints[0])
                    || !cmp(s->getTitle(), constraints[0])
                    || !cmp(s->getAlbum(), constraints[0])
                    || !cmp(s->getName(), constraints[0])
                    || !cmp(s->getComposer(), constraints[0])
                    || !cmp(s->getPerformer(), constraints[0])
                    || !cmp(s->getGenre(), constraints[0])
                    || !cmp(s->getDate(), constraints[0])
                    || !cmp(s->getComment(), constraints[0]);

                if (found && !constraints[1].empty())
                    found = !cmp(s->getArtist(), constraints[1]);
                if (found && !constraints[2].empty())
                    found = !cmp(s->getAlbumArtist(), constraints[2]);
                if (found && !constraints[3].empty())
                    found = !cmp(s->getTitle(), constraints[3]);
                if (found && !constraints[4].empty())
                    found = !cmp(s->getAlbum(), constraints[4]);
                if (found && !constraints[5].empty())
                    found = !cmp(s->getName(), constraints[5]);
                if (found && !constraints[6].empty())
                    found = !cmp(s->getComposer(), constraints[6]);
                if (found && !constraints[7].empty())
                    found = !cmp(s->getPerformer(), constraints[7]);
                if (found && !constraints[8].empty())
                    found = !cmp(s->getGenre(), constraints[8]);
                if (found && !constraints[9].empty())
                    found = !cmp(s->getDate(), constraints[9]);
                if (found && !constraints[10].empty())
                    found = !cmp(s->getComment(), constraints[10]);
            }

            if (any_found && found)
                w.addItem(*s);
        }
    };

    if (search_in_database)
        searchSongs(Mpd.GetDirectoryRecursive("/"), MPD::SongIterator());
    else
        searchSongs(myPlaylist->main().beginV(), myPlaylist->main().endV());
}

inline void SearchEngine::syncNative()
{
    NativeSearchEngineScreen *native;
    NcMenu *native_menu;
    const std::string *column_title;
    int32 result_count;
    bool constraints_locked;
    bool was_filtered;

    native = native_c_screen_search_engine();
    native_menu = native_search_engine_screen_menu(native);
    was_filtered = w.isFiltered();
    if (was_filtered)
        w.showAllItems();

    native_search_engine_screen_clear(native);
    result_count = 0;
    for (auto it = w.begin(); it != w.end(); ++it)
    {
        search_engine_compat::addNativeItem(native, *it);
        if (it->value().isSong())
            result_count += 1;
    }
    if (!w.empty())
        nc_menu_highlight_position(
            native_menu,
            static_cast<int64>(w.choice()),
            nc_window_height(native_search_engine_screen_window(native)));

    native_search_engine_screen_set_prepared(native, !w.empty());
    native_search_engine_screen_set_result_state(
        native, result_count > 0, result_count);
    constraints_locked = !w.empty() && w.at(0).isInactive();
    native_search_engine_screen_set_constraints_locked(
        native, constraints_locked);
    column_title = &w.getTitle();
    native_search_engine_screen_set_column_title(
        native, const_cast<char *>(column_title->c_str()),
        static_cast<int32>(column_title->size()));

    if (was_filtered)
        w.showFilteredItems();
}

inline void SearchEngine::syncLegacyStaticRows()
{
    w.clearFilter();
    w.clear();
    w.resizeList(NATIVE_SEARCH_ENGINE_RESET_BUTTON_ROW+1);
    for (int32 i = 0; i <= NATIVE_SEARCH_ENGINE_RESET_BUTTON_ROW; ++i)
        syncLegacyRow(i);
    w.setTitle("");
    w.reset();
    syncLegacyFindConstraint();
}

inline void SearchEngine::syncLegacyRow(int32 row)
{
    NativeSearchEngineScreen *native;
    NcSearchRow *native_row;
    uint32 flags;
    auto *legacy_row = static_cast<SearchEngineWindow::Item *>(nullptr);

    if (row < 0 || static_cast<size_t>(row) >= w.size())
        return;

    native = native_c_screen_search_engine();
    native_row = nc_search_row_menu_item_at(
        native_search_engine_screen_rows(native), NC_MENU_ITEMS_ALL, row);
    if (native_row == nullptr || native_row->is_song)
        return;

    legacy_row = &w.at(static_cast<size_t>(row));
    flags = nc_menu_item_flags_at(
        native_search_engine_screen_menu(native), NC_MENU_ITEMS_ALL, row);
    if (!(flags & NC_MENU_ITEM_SEPARATOR))
    {
        NC::Buffer &buffer = legacy_row->value().mkBuffer();

        nc_buffer_destroy(buffer.cBuffer());
        nc_buffer_copy(buffer.cBuffer(), &native_row->buffer);
    }
    legacy_row->setSelectable(flags & NC_MENU_ITEM_SELECTABLE);
    legacy_row->setSelected(flags & NC_MENU_ITEM_SELECTED);
    legacy_row->setInactive(flags & NC_MENU_ITEM_INACTIVE);
    legacy_row->setSeparator(flags & NC_MENU_ITEM_SEPARATOR);
}

inline void SearchEngine::syncLegacyFindConstraint()
{
    NativeSearchEngineScreen *native;
    NcmStringView view;
    std::string constraint;

    native = native_c_screen_search_engine();
    view = native_search_engine_screen_find_constraint(native);
    constraint.assign(view.data == nullptr ? "" : view.data,
                      static_cast<size_t>(view.len));
    if (constraint.empty())
    {
        m_search_predicate.clear();
        return;
    }
    m_search_predicate = Regex::ItemFilter<SEItem>(
        constraint,
        Config.regex_type,
        std::bind(search_engine_compat::SEItemEntryMatcher,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  false));
}

inline void SearchEngine::resizeNativeWindow()
{
    NativeSearchEngineScreen *native;
    size_t x_offset;
    size_t width;
    std::string column_title;

    native = native_c_screen_search_engine();
    getWindowResizeParams(x_offset, width);
    w.resize(width, static_cast<size_t>(ui_state_main_height()));
    w.moveTo(x_offset, static_cast<size_t>(ui_state_main_start_y()));
    switch (Config.search_engine_display_mode)
    {
    case NCM_DISPLAY_MODE_COLUMNS:
        if (Config.titles_visibility)
            column_title = Display::Columns(w.getWidth());
        w.setTitle(column_title);
        break;
    case NCM_DISPLAY_MODE_CLASSIC:
        w.setTitle("");
        break;
    }
    native_search_engine_screen_set_column_title(
        native, const_cast<char *>(column_title.c_str()),
        static_cast<int32>(column_title.size()));
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
    NativeSearchEngineScreen *native;

    if (searcher == nullptr)
        return;
    native = native_c_screen_search_engine();
    if (!native_search_engine_screen_is_prepared(native)
        || searcher->w.empty())
        searcher->Prepare();
    else
        searcher->syncNative();
}

inline void SearchEngine::nativeResizeCallback(void *user)
{
    SearchEngine *searcher = static_cast<SearchEngine *>(user);

    if (searcher == nullptr)
        return;
    searcher->resizeNativeWindow();
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

inline bool SearchEngine::nativeCanRunCurrentCallback(void *user)
{
    SearchEngine *searcher = static_cast<SearchEngine *>(user);

    return searcher != nullptr && searcher->actionRunnable();
}

inline bool SearchEngine::nativeRunCurrentCallback(void *user) noexcept
{
    SearchEngine *searcher = static_cast<SearchEngine *>(user);
    native_search_engine_compat::RunCurrentResult result;

    if (searcher == nullptr)
        return false;

    try
    {
        result = native_search_engine_compat::invoke_run_current<
            NC::PromptAborted>(
            [searcher]() { return searcher->actionRunnable(); },
            [searcher]() { searcher->runAction(); });
    }
    catch (MPD::ClientError &error)
    {
        ncm_status_handle_client_error_value(
            &global_mpd, const_cast<char *>(error.what()), -1,
            error.clearable());
        return false;
    }
    catch (MPD::ServerError &error)
    {
        ncm_status_handle_server_error_value(
            &global_mpd, static_cast<int32>(error.code()),
            const_cast<char *>(error.what()), -1);
        return false;
    }
    catch (std::exception &error)
    {
        NcmStringFormatArg arg;

        arg = ncm_string_format_arg_cstring(
            const_cast<char *>(error.what()));
        ncm_statusbar_format(
            ncm_statusbar_message_delay_time(),
            const_cast<char *>("Unexpected error: %1%"),
            STRLIT_LEN("Unexpected error: %1%"), &arg, 1);
        return false;
    }
    catch (...)
    {
        ncm_statusbar_print_cstring(
            ncm_statusbar_message_delay_time(),
            const_cast<char *>("Unexpected error"));
        return false;
    }

    switch (result)
    {
    case native_search_engine_compat::RunCurrentResult::NotRunnable:
        return false;
    case native_search_engine_compat::RunCurrentResult::Completed:
        return true;
    case native_search_engine_compat::RunCurrentResult::Aborted:
        ncm_statusbar_print_cstring(
            ncm_statusbar_message_delay_time(),
            const_cast<char *>("Action aborted"));
        return false;
    }
    return false;
}

inline void SearchEngine::nativeStaticRowChangedCallback(void *user,
                                                         int32 row)
{
    SearchEngine *searcher = static_cast<SearchEngine *>(user);

    if (searcher == nullptr)
        return;
    if (row == NATIVE_SEARCH_ENGINE_ALL_STATIC_ROWS)
        searcher->syncLegacyStaticRows();
    else
        searcher->syncLegacyRow(row);
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
