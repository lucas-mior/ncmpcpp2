#ifndef NCMPCPP_BROWSER_H
#define NCMPCPP_BROWSER_H

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <time.h>
#include <vector>

#include "app_controller.h"
#include "charset.h"
#include "configuration_legacy.h"
#include "curses/nc_cyclic_buffer.h"
#include "format_impl.h"
#include "actions.h"
#include "global.h"
#include "helpers.h"
#include "interfaces.h"
#include "mpdpp.h"
#include "regex_filter.h"
#include "screens/native_c_screens.h"
#include "screens/nc_browser.h"
#include "screens/screen_cpp_compat.h"
#include "screens/screen_cpp_legacy.h"
#include "settings_legacy.h"
#include "song_list.h"
#include "status.h"
#include "statusbar.h"
#include "title_legacy.h"
#include "ui_state.h"
#include "utility/comparators.h"
#include "utility/string.h"
#include "utility/utf8.h"

#include "c/ncm_tags.h"


struct BrowserWindow: NC::Menu<MPD::Item>, SongList
{
    BrowserWindow() { }
    BrowserWindow(NC::Menu<MPD::Item> &&base)
    : NC::Menu<MPD::Item>(std::move(base)) { }

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
void Items(NC::Menu<MPD::Item> &menu, const SongList &list);
}

template <>
struct SongPropertiesExtractor<MPD::Item>
{
    template <typename ItemT>
    auto &operator()(ItemT &item) const
    {
        auto s = item.value().type() == MPD::Item::Type::Song
            ? &item.value().song()
            : nullptr;
        m_cache.assign(&item.properties(), s);
        return m_cache;
    }

private:
    mutable SongProperties m_cache;
};

struct Browser: Screen<BrowserWindow>, Filterable, HasSongs, Searchable, Tabbable
{
    Browser();
    virtual ~Browser();

    virtual void refresh() override;
    virtual void refreshWindow() override;
    virtual void scroll(enum NcScroll where) override;
    virtual void switchTo() override;
    virtual void resize() override;

    virtual std::string title() override;
    virtual ScreenType type() override { return NCM_SCREEN_TYPE_BROWSER; }
    virtual NcScreen *nativeScreen() override;
    virtual const NcScreen *nativeScreen() const override;

    virtual void update() override;

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

    void requestUpdate() { m_update_request = true; }

    bool inRootDirectory();
    bool isParentDirectory(const MPD::Item &item);
    const std::string &currentDirectory();

    bool isLocal() { return m_local_browser; }
    void locateSong(const MPD::Song &s);
    bool enterDirectory();
    void getDirectory(std::string directory);
    void changeBrowseMode();
    void remove(const MPD::Item &item);

    static void fetchSupportedExtensions();

private:
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
    static void nativeRequestUpdateCallback(void *user);
    static void nativeMouseButtonPressedCallback(void *user, MEVENT event);

    std::string itemToString(const MPD::Item &item);
    bool browserEntryMatcher(const Regex::Regex &rx, const MPD::Item &item,
                             bool filter);

    bool m_redraw_header;
    bool m_update_request;
    bool m_local_browser;
    size_t m_scroll_beginning;
    std::string m_current_directory;
    std::string m_title_cache;
    std::string m_search_constraint;
    Regex::Filter<MPD::Item> m_search_predicate;
};

inline Browser *myBrowser = nullptr;

namespace browser_compat {

inline std::set<std::string> lm_supported_extensions;

inline std::string realPath(bool local_browser, std::string path)
{
    if (!local_browser)
        path = Config.mpd_music_dir + path;
    return path;
}

inline bool isStringParentDirectory(const std::string &directory)
{
    return directory.size() >= 3
        && directory.compare(directory.size()-3, 3, "/..") == 0;
}

inline bool isItemParentDirectory(const MPD::Item &item)
{
    return item.type() == MPD::Item::Type::Directory
        && isStringParentDirectory(item.directory().path());
}

inline bool isRootDirectory(const std::string &directory)
{
    return directory == "/";
}

inline bool isHidden(const std::filesystem::directory_iterator &entry)
{
    return entry->path().filename().string()[0] == '.';
}

inline bool hasSupportedExtension(const std::filesystem::directory_entry &entry)
{
    return lm_supported_extensions.find(entry.path().extension().string())
        != lm_supported_extensions.end();
}

inline time_t lastWriteTime(const std::filesystem::path &path)
{
    auto file_time = std::filesystem::last_write_time(path);
    auto system_time = std::chrono::time_point_cast<
        std::chrono::system_clock::duration>(
            file_time - std::filesystem::file_time_type::clock::now()
            + std::chrono::system_clock::now());
    return std::chrono::system_clock::to_time_t(system_time);
}

inline MPD::Song getLocalSong(const std::filesystem::directory_entry &entry,
                              bool read_tags)
{
    auto path = entry.path().string();
    mpd_pair pair = { "file", path.c_str() };
    mpd_song *s = mpd_song_begin(&pair);
    if (s == nullptr)
        throw std::runtime_error("invalid path: " + path);
    if (read_tags)
    {
#ifdef HAVE_TAGLIB_H
        char last_modified_format[] = "%Y-%m-%dT%H:%M:%SZ";
        NcmBuffer last_modified_buffer;
        std::string last_modified;

        ncm_buffer_init(&last_modified_buffer);
        if (ncm_helpers_time_format(&last_modified_buffer,
                                    last_modified_format,
                                    lastWriteTime(entry.path()))) {
            last_modified.assign(last_modified_buffer.data,
                                 last_modified_buffer.len);
        }
        ncm_buffer_destroy(&last_modified_buffer);
        ncm_tags_set_attribute(
            s,
            const_cast<char *>("Last-Modified"),
            const_cast<char *>(last_modified.c_str()));
        ncm_tags_read_song(s);
#endif // HAVE_TAGLIB_H
    }
    return s;
}

inline void getLocalDirectory(NC::Menu<MPD::Item> &menu,
                              const std::string &directory)
{
    for (std::filesystem::directory_iterator entry(directory), end; entry != end; ++entry)
    {
        if (!Config.local_browser_show_hidden_files && isHidden(entry))
            continue;

        if (entry->is_directory())
            menu.addItem(MPD::Directory(entry->path().string(),
                                        lastWriteTime(entry->path())));
        else if (hasSupportedExtension(*entry))
            menu.addItem(getLocalSong(*entry, true));
    }
}

inline void getLocalDirectoryRecursively(std::vector<MPD::Song> &songs,
                                         const std::string &directory)
{
    size_t sort_offset = songs.size();
    for (std::filesystem::directory_iterator entry(directory), end; entry != end; ++entry)
    {
        if (!Config.local_browser_show_hidden_files && isHidden(entry))
            continue;

        if (entry->is_directory())
        {
            getLocalDirectoryRecursively(songs, entry->path().string());
            sort_offset = songs.size();
        }
        else if (hasSupportedExtension(*entry))
            songs.push_back(getLocalSong(*entry, false));
    }

    if (Config.browser_sort_mode != NCM_SORT_MODE_NONE)
    {
        std::stable_sort(songs.begin()+sort_offset, songs.end(),
                         LocaleBasedSorting(std::locale(),
                                            Config.ignore_leading_the));
    }
}

inline void clearDirectory(const std::string &directory)
{
    for (std::filesystem::directory_iterator entry(directory), end; entry != end; ++entry)
    {
        if (!entry->is_symlink() && entry->is_directory())
            clearDirectory(entry->path().string());
        const char msg[] = "Deleting \"%1%\"...";
        Statusbar::printf(msg,
                          Utf8::shorten(entry->path().string(),
                                        COLS-const_strlen(msg)));
        std::filesystem::remove(entry->path());
    }
}

inline bool addSongsToPlaylist(std::vector<MPD::Song> &songs, bool play)
{
    bool success = true;
    bool first = true;

    for (auto &song : songs)
    {
        if (!ncm_action_add_song_to_playlist(song.cSong(), play && first, -1))
            success = false;
        first = false;
    }
    return success;
}

inline void addNativeItem(NativeBrowserScreen *native,
                          const MPD::Item &item)
{
    NcmMpdItem native_item;
    MPD::Item &legacy_item = const_cast<MPD::Item &>(item);

    ncm_mpd_item_init(&native_item);
    switch (legacy_item.type())
    {
    case MPD::Item::Type::Directory:
        ncm_mpd_item_set_directory(&native_item,
                                   legacy_item.directory().cDirectory());
        break;
    case MPD::Item::Type::Song:
        ncm_mpd_item_set_song(&native_item, legacy_item.song().cSong());
        break;
    case MPD::Item::Type::Playlist:
        ncm_mpd_item_set_playlist(&native_item,
                                  legacy_item.playlist().cPlaylist());
        break;
    }
    native_browser_screen_add_item_move(native, &native_item);
    ncm_mpd_item_destroy(&native_item);
}

}

inline SongIterator BrowserWindow::currentS()
{
    return makeSongIterator(current());
}

inline ConstSongIterator BrowserWindow::currentS() const
{
    return makeConstSongIterator(current());
}

inline SongIterator BrowserWindow::beginS()
{
    return makeSongIterator(begin());
}

inline ConstSongIterator BrowserWindow::beginS() const
{
    return makeConstSongIterator(begin());
}

inline SongIterator BrowserWindow::endS()
{
    return makeSongIterator(end());
}

inline ConstSongIterator BrowserWindow::endS() const
{
    return makeConstSongIterator(end());
}

inline std::vector<MPD::Song> BrowserWindow::getSelectedSongs()
{
    return {};
}

inline Browser::Browser()
: m_redraw_header(false)
, m_update_request(true)
, m_local_browser(false)
, m_scroll_beginning(0)
, m_current_directory("/")
{
    w = NC::Menu<MPD::Item>(0,
                            static_cast<size_t>(ui_state_main_start_y()),
                            COLS,
                            static_cast<size_t>(ui_state_main_height()),
                            Config.browser_display_mode
                                == NCM_DISPLAY_MODE_COLUMNS
                                && Config.titles_visibility
                                ? Display::Columns(COLS)
                                : "",
                            Config.main_color,
                            NC::Border());
    w.setHighlightPrefix(Config.current_item_prefix);
    w.setHighlightSuffix(Config.current_item_suffix);
    w.cyclicScrolling(Config.use_cyclic_scrolling);
    w.centeredCursor(Config.centered_cursor);
    w.setSelectedPrefix(Config.selected_item_prefix);
    w.setSelectedSuffix(Config.selected_item_suffix);
    w.setItemDisplayer(std::bind(Display::Items, std::placeholders::_1, std::cref(w)));

    native_c_screen_browser_init();
    native_browser_screen_set_current_directory(
        native_c_screen_browser(),
        const_cast<char *>(m_current_directory.c_str()),
        static_cast<int32>(m_current_directory.size()));
    {
        NativeBrowserBridge bridge = {};

        bridge.active_window = nativeActiveWindowCallback;
        bridge.refresh = nativeRefreshCallback;
        bridge.refresh_window = nativeRefreshWindowCallback;
        bridge.scroll = nativeScrollCallback;
        bridge.switch_to = nativeSwitchToCallback;
        bridge.resize = nativeResizeCallback;
        bridge.title = nativeTitleCallback;
        bridge.update = nativeUpdateCallback;
        bridge.request_update = nativeRequestUpdateCallback;
        bridge.mouse_button_pressed = nativeMouseButtonPressedCallback;
        bridge.user = this;
        native_browser_screen_set_bridge(native_c_screen_browser(), bridge);
    }
    screen_compat::bind_legacy_owner(nativeScreen(), this);
}

inline Browser::~Browser()
{
    NativeBrowserBridge bridge = {};

    native_browser_screen_set_bridge(native_c_screen_browser(), bridge);
    screen_compat::bind_legacy_owner(nativeScreen(), nullptr);
    if (app_controller_is_screen_registered(nativeScreen()))
        app_controller_unregister_screen(nativeScreen());
}

inline void Browser::refresh()
{
    syncNative();
    nc_screen_refresh(nativeScreen());
}

inline void Browser::refreshWindow()
{
    syncNative();
    nc_screen_refresh_window(nativeScreen());
}

inline void Browser::scroll(enum NcScroll where)
{
    w.scroll(where);
    syncNative();
}

inline void Browser::switchTo()
{
    syncNative();
    nc_screen_set_has_to_be_resized(nativeScreen(), hasToBeResized);
    app_controller_switch_to_screen(nativeScreen());
    m_redraw_header = true;
}

inline void Browser::resize()
{
    nc_screen_resize(nativeScreen());
    hasToBeResized = false;
}

inline std::string Browser::title()
{
    NcmBuffer scroll_buffer;
    int64 scroll_beginning;
    std::string result = "Browse: ";
    size_t width = COLS-Utf8::width(result);
    char separator[] = " ** ";

    if (Config.design == NCM_DESIGN_ALTERNATIVE)
        width -= 2;
    else
        width -= static_cast<size_t>(global_volume_state_len());

    ncm_buffer_init(&scroll_buffer);
    scroll_beginning = static_cast<int64>(m_scroll_beginning);
    nc_cyclic_text_write(&scroll_buffer, m_current_directory.data(),
                         static_cast<int32>(m_current_directory.size()),
                         &scroll_beginning, static_cast<int32>(width),
                         separator, sizeof(separator) - 1,
                         Config.header_text_scrolling);
    m_scroll_beginning = static_cast<size_t>(scroll_beginning);
    if (scroll_buffer.len > 0)
        result.append(scroll_buffer.data,
                      static_cast<size_t>(scroll_buffer.len));
    ncm_buffer_destroy(&scroll_buffer);
    return result;
}

inline NcScreen *Browser::nativeScreen()
{
    return native_c_screen_browser_native();
}

inline const NcScreen *Browser::nativeScreen() const
{
    return native_c_screen_browser_native();
}

inline void Browser::update()
{
    if (m_update_request)
    {
        m_update_request = false;
        do
        {
            try
            {
                getDirectory(m_current_directory);
                refreshWindow();
            }
            catch (MPD::ServerError &err)
            {
                if (err.code() == MPD_SERVER_ERROR_NO_EXIST)
                    m_current_directory = getParentDirectory(
                        m_current_directory);
                else
                    throw;
            }
        }
        while (w.empty() && !inRootDirectory());
    }
    if (m_redraw_header)
    {
        drawHeader();
        m_redraw_header = false;
    }
}

inline void Browser::mouseButtonPressed(MEVENT me)
{
    if (w.empty() || !w.hasCoords(me.x, me.y) || size_t(me.y) >= w.size())
        return;
    if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
    {
        w.Goto(me.y);
        switch (w.current()->value().type())
        {
        case MPD::Item::Type::Directory:
            if (me.bstate & BUTTON1_PRESSED)
                enterDirectory();
            else
                addItemToPlaylist(false);
            break;
        case MPD::Item::Type::Playlist:
        case MPD::Item::Type::Song:
        {
            bool play = me.bstate & BUTTON3_PRESSED;
            addItemToPlaylist(play);
            break;
        }
        }
    }
    else
        Screen<WindowType>::mouseButtonPressed(me);
}

inline bool Browser::allowsSearching()
{
    return true;
}

inline const std::string &Browser::searchConstraint()
{
    return m_search_constraint;
}

inline void Browser::setSearchConstraint(const std::string &constraint)
{
    m_search_predicate = Regex::Filter<MPD::Item>(
        constraint,
        Config.regex_type,
        [this](const Regex::Regex &rx, const MPD::Item &item) {
            return browserEntryMatcher(rx, item, false);
        });
    m_search_constraint = constraint;
}

inline void Browser::clearSearchConstraint()
{
    m_search_predicate.clear();
    m_search_constraint.clear();
}

inline bool Browser::search(SearchDirection direction, bool wrap,
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

inline bool Browser::allowsFiltering()
{
    return allowsSearching();
}

inline std::string Browser::currentFilter()
{
    std::string result;
    if (auto pred = w.filterPredicate<Regex::Filter<MPD::Item>>())
        result = pred->constraint();
    return result;
}

inline void Browser::applyFilter(const std::string &constraint)
{
    if (!constraint.empty())
    {
        w.applyFilter(Regex::Filter<MPD::Item>(
                          constraint,
                          Config.regex_type,
                          [this](const Regex::Regex &rx,
                                 const MPD::Item &item) {
                              return browserEntryMatcher(rx, item, true);
                          }));
    }
    else
        w.clearFilter();
    syncNative();
}

inline bool Browser::itemAvailable()
{
    return !w.empty() && !isParentDirectory(w.current()->value());
}

inline bool Browser::addItemToPlaylist(bool play)
{
    bool success = false;

    auto tryToPlay = [] {
        try
        {
            Mpd.Play(ncm_status_state_playlist_length());
        }
        catch (MPD::ServerError &e)
        {
            if (e.code() != MPD_SERVER_ERROR_ARG)
                throw;
        }
    };

    const MPD::Item &item = w.current()->value();
    switch (item.type())
    {
    case MPD::Item::Type::Directory:
    {
        if (m_local_browser)
        {
            std::vector<MPD::Song> songs;
            browser_compat::getLocalDirectoryRecursively(
                songs, item.directory().path());
            success = browser_compat::addSongsToPlaylist(songs, play);
        }
        else
        {
            success = Mpd.Add(item.directory().path());
            if (play)
                tryToPlay();
        }
        Statusbar::printf("Directory \"%1%\" added%2%",
                          item.directory().path(),
                          ncm_helpers_with_errors(success));
        break;
    }
    case MPD::Item::Type::Song:
        success = ncm_action_add_song_to_playlist(
            item.song().cSong(), play, -1);
        break;
    case MPD::Item::Type::Playlist:
        success = Mpd.LoadPlaylist(item.playlist().path());
        if (play)
            tryToPlay();
        if (success)
            Statusbar::printf("Playlist \"%1%\" loaded",
                              item.playlist().path());
        break;
    }
    return success;
}

inline std::vector<MPD::Song> Browser::getSelectedSongs()
{
    std::vector<MPD::Song> songs;
    auto item_handler = [this, &songs](const MPD::Item &item) {
        switch (item.type())
        {
        case MPD::Item::Type::Directory:
            if (m_local_browser)
                browser_compat::getLocalDirectoryRecursively(
                    songs, item.directory().path());
            else
            {
                std::copy(
                    std::make_move_iterator(
                        Mpd.GetDirectoryRecursive(item.directory().path())),
                    std::make_move_iterator(MPD::SongIterator()),
                    std::back_inserter(songs));
            }
            break;
        case MPD::Item::Type::Song:
            songs.push_back(item.song());
            break;
        case MPD::Item::Type::Playlist:
            std::copy(
                std::make_move_iterator(
                    Mpd.GetPlaylistContent(item.playlist().path())),
                std::make_move_iterator(MPD::SongIterator()),
                std::back_inserter(songs));
            break;
        }
    };
    for (const auto &item : w)
        if (item.isSelected())
            item_handler(item.value());
    if (songs.empty() && !w.empty())
        item_handler(w.current()->value());
    return songs;
}

inline bool Browser::inRootDirectory()
{
    return browser_compat::isRootDirectory(m_current_directory);
}

inline bool Browser::isParentDirectory(const MPD::Item &item)
{
    return browser_compat::isItemParentDirectory(item);
}

inline const std::string &Browser::currentDirectory()
{
    return m_current_directory;
}

inline void Browser::locateSong(const MPD::Song &s)
{
    if (s.getDirectory().empty())
        throw std::runtime_error("Song's directory is empty");

    m_local_browser = !s.isFromDatabase();

    if (screenLegacyCurrent() != this)
        switchTo();

    w.clearFilter();

    try
    {
        if (m_current_directory != s.getDirectory())
            getDirectory(s.getDirectory());

        auto begin = w.beginV(), end = w.endV();
        auto it = std::find(begin, end, MPD::Item(s));
        if (it != end)
            w.highlight(it-begin);
    }
    catch (MPD::ServerError &err)
    {
        if (err.code() == MPD_SERVER_ERROR_NO_EXIST)
            requestUpdate();
        else
            throw;
    }
    syncNative();
}

inline bool Browser::enterDirectory()
{
    bool result = false;
    if (!w.empty())
    {
        const auto &item = w.current()->value();
        if (item.type() == MPD::Item::Type::Directory)
        {
            getDirectory(item.directory().path());
            result = true;
        }
    }
    return result;
}

inline void Browser::getDirectory(std::string directory)
{
    bool was_filtered = w.isFiltered();

    if (was_filtered)
        w.showAllItems();

    m_scroll_beginning = 0;
    w.clear();

    if (m_current_directory != directory)
    {
        w.reset();
        m_redraw_header = true;
    }

    if (browser_compat::isStringParentDirectory(directory))
    {
        directory.resize(directory.length()-3);
        directory = getParentDirectory(directory);
    }
    if (directory.empty())
        directory = "/";

    bool is_root = browser_compat::isRootDirectory(directory);
    if (!is_root)
        w.addItem(MPD::Directory(directory + "/.."),
                  NC::List::Properties::None);

    if (m_local_browser)
    {
        browser_compat::getLocalDirectory(w, directory);
        if (Config.browser_sort_mode == NCM_SORT_MODE_NONE
            || Config.browser_sort_mode == NCM_SORT_MODE_TYPE)
        {
            Statusbar::print(
                "Switching to sorting songs by name for the local browser");
            Config.browser_sort_mode = NCM_SORT_MODE_NAME;
        }
    }
    else
    {
        MPD::ItemIterator end;
        for (auto dir = Mpd.GetDirectory(directory); dir != end; ++dir)
            w.addItem(std::move(*dir));
    }

    if (Config.browser_sort_mode != NCM_SORT_MODE_NONE)
    {
        std::stable_sort(
            w.begin() + (is_root ? 0 : 1), w.end(),
            LocaleBasedItemSorting(std::locale(), Config.ignore_leading_the,
                                   Config.browser_sort_mode));
    }

    if (was_filtered)
        w.showFilteredItems();

    for (size_t i = 0; i < w.size(); ++i)
    {
        if (w[i].value().type() == MPD::Item::Type::Directory
            && w[i].value().directory().path() == m_current_directory)
        {
            w.highlight(i);
            break;
        }
    }
    m_current_directory = directory;
    syncNative();
}

inline void Browser::changeBrowseMode()
{
    if (Mpd.GetHostname()[0] != '/')
    {
        Statusbar::print(
            "For browsing local filesystem connection to MPD via UNIX Socket "
            "is required");
        return;
    }

    m_local_browser = !m_local_browser;
    Statusbar::printf("Browse mode: %1%",
                      m_local_browser ? "local filesystem" : "MPD database");
    if (m_local_browser)
    {
        m_current_directory = "~";
        expand_home(m_current_directory);
    }
    else
        m_current_directory = "/";
    w.reset();
    getDirectory(m_current_directory);
}

inline void Browser::remove(const MPD::Item &item)
{
    if (!Config.allow_for_physical_item_deletion)
        throw std::runtime_error("physical deletion is forbidden");
    if (isParentDirectory(item))
        throw std::runtime_error("deletion of parent directory is forbidden");

    std::string path;
    switch (item.type())
    {
    case MPD::Item::Type::Directory:
        path = browser_compat::realPath(m_local_browser,
                                        item.directory().path());
        browser_compat::clearDirectory(path);
        std::filesystem::remove(path);
        break;
    case MPD::Item::Type::Song:
        path = browser_compat::realPath(m_local_browser, item.song().getURI());
        std::filesystem::remove(path);
        break;
    case MPD::Item::Type::Playlist:
        path = item.playlist().path();
        try
        {
            Mpd.DeletePlaylist(path);
        }
        catch (MPD::ServerError &e)
        {
            if (e.code() == MPD_SERVER_ERROR_NO_EXIST)
            {
                path = browser_compat::realPath(m_local_browser,
                                                std::move(path));
                std::filesystem::remove(path);
            }
            else
                throw;
        }
        break;
    }
}

inline void Browser::fetchSupportedExtensions()
{
    browser_compat::lm_supported_extensions.clear();
    MPD::StringIterator extension = Mpd.GetSupportedExtensions(), end;
    for (; extension != end; ++extension)
        browser_compat::lm_supported_extensions.insert("." + std::move(*extension));
}

inline void Browser::syncNative()
{
    NativeBrowserScreen *native;
    NcMenu *native_menu;
    bool was_filtered;

    native = native_c_screen_browser();
    native_menu = native_browser_screen_menu(native);
    was_filtered = w.isFiltered();
    if (was_filtered)
        w.showAllItems();

    native_browser_screen_clear(native);
    for (auto it = w.begin(); it != w.end(); ++it)
        browser_compat::addNativeItem(native, it->value());
    if (!w.empty())
        nc_menu_highlight_position(
            native_menu,
            static_cast<int64>(w.choice()),
            nc_window_height(native_browser_screen_window(native)));
    native_browser_screen_set_current_directory(
        native,
        const_cast<char *>(m_current_directory.c_str()),
        static_cast<int32>(m_current_directory.size()));

    if (was_filtered)
        w.showFilteredItems();
}

inline void Browser::resizeNativeWindow()
{
    size_t x_offset;
    size_t width;

    getWindowResizeParams(x_offset, width);
    w.resize(width, static_cast<size_t>(ui_state_main_height()));
    w.moveTo(x_offset, static_cast<size_t>(ui_state_main_start_y()));
    switch (Config.browser_display_mode)
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

inline NcWindow *Browser::nativeActiveWindowCallback(void *user)
{
    Browser *browser = static_cast<Browser *>(user);

    if (browser == nullptr)
        return nullptr;
    return browser->w.nativeWindow();
}

inline void Browser::nativeRefreshCallback(void *user)
{
    Browser *browser = static_cast<Browser *>(user);

    if (browser == nullptr)
        return;
    browser->w.display();
}

inline void Browser::nativeRefreshWindowCallback(void *user)
{
    nativeRefreshCallback(user);
}

inline void Browser::nativeScrollCallback(void *user, enum NcScroll where)
{
    Browser *browser = static_cast<Browser *>(user);

    if (browser == nullptr)
        return;
    browser->scroll(where);
}

inline void Browser::nativeSwitchToCallback(void *user)
{
    Browser *browser = static_cast<Browser *>(user);

    if (browser == nullptr)
        return;
    browser->m_redraw_header = true;
}

inline void Browser::nativeResizeCallback(void *user)
{
    Browser *browser = static_cast<Browser *>(user);

    if (browser == nullptr)
        return;
    browser->resizeNativeWindow();
}

inline char *Browser::nativeTitleCallback(void *user)
{
    Browser *browser = static_cast<Browser *>(user);

    if (browser == nullptr)
        return nullptr;
    browser->m_title_cache = browser->title();
    return const_cast<char *>(browser->m_title_cache.c_str());
}

inline void Browser::nativeUpdateCallback(void *user)
{
    Browser *browser = static_cast<Browser *>(user);

    if (browser == nullptr)
        return;
    browser->update();
}

inline void Browser::nativeRequestUpdateCallback(void *user)
{
    Browser *browser = static_cast<Browser *>(user);

    if (browser == nullptr)
        return;
    browser->requestUpdate();
}

inline void Browser::nativeMouseButtonPressedCallback(void *user,
                                                      MEVENT event)
{
    Browser *browser = static_cast<Browser *>(user);

    if (browser == nullptr)
        return;
    browser->mouseButtonPressed(event);
}

inline std::string Browser::itemToString(const MPD::Item &item)
{
    std::string result;
    switch (item.type())
    {
    case MPD::Item::Type::Directory:
        result = "[" + getBasename(item.directory().path()) + "]";
        break;
    case MPD::Item::Type::Song:
        switch (Config.browser_display_mode)
        {
        case NCM_DISPLAY_MODE_CLASSIC:
            result = Format::stringify<char>(Config.song_list_format,
                                             &item.song());
            break;
        case NCM_DISPLAY_MODE_COLUMNS:
            result = Format::stringify<char>(Config.song_columns_mode_format,
                                             &item.song());
            break;
        }
        break;
    case MPD::Item::Type::Playlist:
        result = Config.browser_playlist_prefix.str();
        result += getBasename(item.playlist().path());
        break;
    }
    return result;
}

inline bool Browser::browserEntryMatcher(const Regex::Regex &rx,
                                         const MPD::Item &item,
                                         bool filter)
{
    if (browser_compat::isItemParentDirectory(item))
        return filter;
    return Regex::search(itemToString(item), rx);
}

#endif // NCMPCPP_BROWSER_H
