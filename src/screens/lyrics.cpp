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
#include <filesystem>
#include <cassert>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <functional>
#include <future>
#include <thread>

#include "curses/scrollpad.h"
#include "screens/browser.h"
#include "charset.h"
#include "curl_handle.h"
#include "format_impl.h"
#include "app_controller.h"
#include "global.h"
#include "ui_state_legacy.h"
#include "helpers.h"
#include "macro_utilities.h"
#include "screens/lyrics.h"
#include "screens/playlist.h"
#include "settings.h"
#include "song.h"
#include "statusbar.h"
#include "title.h"
#include "screens/screen_switcher.h"
#include "screens/screen_legacy.h"
#include "utility/string.h"


Lyrics *myLyrics;

namespace {

NC::Scroll to_cpp_scroll(enum NcScroll where)
{
	switch (where)
	{
		case NC_SCROLL_UP:
			return NC::Scroll::Up;
		case NC_SCROLL_DOWN:
			return NC::Scroll::Down;
		case NC_SCROLL_PAGE_UP:
			return NC::Scroll::PageUp;
		case NC_SCROLL_PAGE_DOWN:
			return NC::Scroll::PageDown;
		case NC_SCROLL_HOME:
			return NC::Scroll::Home;
		case NC_SCROLL_END:
			return NC::Scroll::End;
	}
	return NC::Scroll::Up;
}

enum NcScroll to_nc_scroll(NC::Scroll where)
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

std::string removeExtension(std::string filename)
{
	size_t dot = filename.rfind('.');
	if (dot != std::string::npos)
		filename.resize(dot);
	return filename;
}

std::string lyricsFilename(const MPD::Song &s)
{
	std::string filename;
	if (Config.store_lyrics_in_song_dir && !s.isStream())
	{
		if (s.isFromDatabase())
			filename = Config.mpd_music_dir + "/";
		filename += removeExtension(s.getURI());
		removeExtension(filename);
	}
	else
	{
		std::string artist = s.getArtist();
		std::string title  = s.getTitle();
		if (artist.empty() || title.empty())
			filename = removeExtension(s.getName());
		else
			filename = artist + " - " + title;
		removeInvalidCharsFromFilename(filename, Config.generate_win32_compatible_filenames);
		filename = Config.lyrics_directory + "/" + filename;
	}
	filename += ".txt";
	return filename;
}

bool loadLyrics(NC::Scrollpad &w, const std::string &filename)
{
	std::ifstream input(filename);
	if (input.is_open())
	{
		std::string line;
		bool first_line = true;
		while (std::getline(input, line))
		{
			// Remove carriage returns as they mess up the display.
			line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
			if (!first_line)
				w << '\n';
			w << Charset::utf8ToLocale(line);
			first_line = false;
		}
		return true;
	}
	else
		return false;
}

bool saveLyrics(const std::string &filename, const std::string &lyrics)
{
	std::ofstream output(filename);
	if (output.is_open())
	{
		output << lyrics;
		output.close();
		return true;
	}
	else
		return false;
}

LyricsFetcher::Result downloadLyrics(
	const MPD::Song &s,
	std::shared_ptr<Shared<NC::Buffer>> shared_buffer,
	std::shared_ptr<std::atomic<bool>> download_stopper,
	LyricsFetcher *current_fetcher)
{
	std::string s_artist = s.getArtist();
	std::string s_title  = s.getTitle();
	// If artist or title is empty, use filename. This should give reasonable
	// results for google search based lyrics fetchers.
	if (s_artist.empty() || s_title.empty())
	{
		s_artist.clear();
		s_title = s.getName();
		// Get rid of underscores to improve search results.
		std::replace_if(
			s_title.begin(), s_title.end(),
			[](char c) { return c == '-' || c == '_'; }, ' ');
		size_t dot = s_title.rfind('.');
		if (dot != std::string::npos)
			s_title.resize(dot);
	}

	auto fetch_lyrics = [&](auto &fetcher_) {
		{
			if (shared_buffer)
			{
				auto buf = shared_buffer->acquire();
				*buf << "Fetching lyrics from "
				     << NC::Format::Bold
				     << fetcher_->name()
				     << NC::Format::NoBold << "... ";
			}
		}
		auto result_ = fetcher_->fetch(s_artist, s_title, s);
		if (result_.first == false)
		{
			if (shared_buffer)
			{
				auto buf = shared_buffer->acquire();
				*buf << NC::Color::Red
				     << result_.second
				     << NC::Color::End
				     << '\n';
			}
		}
		return result_;
	};

	LyricsFetcher::Result fetcher_result;
	if (current_fetcher == nullptr)
	{
		for (auto &fetcher : Config.lyrics_fetchers)
		{
			if (download_stopper && download_stopper->load())
				return LyricsFetcher::Result(false, "");
			fetcher_result = fetch_lyrics(fetcher);
			if (fetcher_result.first)
				break;
		}
	}
	else
		fetcher_result = fetch_lyrics(current_fetcher);

	return fetcher_result;
}

}

Lyrics::Lyrics()
	: w(0, ui_state_legacy_main_start_y(), COLS, ui_state_legacy_main_height(), "", Config.main_color, NC::Border())
	, m_fetcher(nullptr)
{
	NcScreenCallbacks callbacks = makeCallbacks();

	nc_lyrics_screen_init(&m_screen,
	                      callbacks,
	                      this,
	                      0,
	                      COLS,
	                      ui_state_legacy_main_start_y(),
	                      ui_state_legacy_main_height());

	bool register_success = app_controller_register_screen(nativeScreen());
	assert(register_success);
	(void)register_success;
}

Lyrics::~Lyrics()
{
	stopDownload();
	if (app_controller_is_screen_registered(nativeScreen()))
	{
		app_controller_unregister_screen(nativeScreen());
	}
}

bool Lyrics::isActiveWindow(const NC::Window &w_) const
{
	return &w == &w_;
}

NC::Window *Lyrics::activeWindow()
{
	return &w;
}

const NC::Window *Lyrics::activeWindow() const
{
	return &w;
}

void Lyrics::refresh()
{
	nc_screen_refresh(nativeScreen());
}

void Lyrics::refreshWindow()
{
	nc_screen_refresh_window(nativeScreen());
}

void Lyrics::scroll(NC::Scroll where)
{
	nc_screen_scroll(nativeScreen(), to_nc_scroll(where));
}

void Lyrics::resize()
{
	nc_screen_resize(nativeScreen());
}

int Lyrics::windowTimeout()
{
	return nc_screen_window_timeout(nativeScreen());
}

void Lyrics::mouseButtonPressed(MEVENT me)
{
	nc_screen_mouse_button_pressed(nativeScreen(), me);
}

void Lyrics::update()
{
	nc_screen_update(nativeScreen());
}

void Lyrics::switchTo()
{
	nc_screen_set_has_to_be_resized(nativeScreen(), hasToBeResized);
	app_controller_switch_to_screen(nativeScreen());
}

std::string Lyrics::title()
{
	char *result = nc_screen_title(nativeScreen());
	if (result == nullptr)
		return "";
	return result;
}

bool Lyrics::isLockable()
{
	return nc_screen_is_lockable(nativeScreen());
}

bool Lyrics::isMergable()
{
	return nc_screen_is_mergable(nativeScreen());
}

NcScreen *Lyrics::nativeScreen()
{
	return nc_lyrics_screen_base(&m_screen);
}

const NcScreen *Lyrics::nativeScreen() const
{
	return nc_lyrics_screen_base(const_cast<NcLyricsScreen *>(&m_screen));
}

NcScreenCallbacks Lyrics::makeCallbacks()
{
	NcScreenCallbacks callbacks = {0};

	callbacks.active_window = activeWindowCallback;
	callbacks.refresh = refreshCallback;
	callbacks.refresh_window = refreshWindowCallback;
	callbacks.scroll = scrollCallback;
	callbacks.switch_to = switchToCallback;
	callbacks.resize = resizeCallback;
	callbacks.window_timeout = windowTimeoutCallback;
	callbacks.title = titleCallback;
	callbacks.update = updateCallback;
	callbacks.mouse_button_pressed = mouseButtonPressedCallback;
	callbacks.is_lockable = isLockableCallback;
	callbacks.is_mergable = isMergableCallback;
	callbacks.destroy = destroyCallback;
	return callbacks;
}

Lyrics *Lyrics::fromScreen(NcScreen *screen)
{
	return static_cast<Lyrics *>(nc_screen_user(screen));
}

NcWindow *Lyrics::activeWindowCallback(NcScreen *screen)
{
	return fromScreen(screen)->w.nativeWindow();
}

void Lyrics::refreshCallback(NcScreen *screen)
{
	fromScreen(screen)->w.display();
}

void Lyrics::refreshWindowCallback(NcScreen *screen)
{
	fromScreen(screen)->w.display();
}

void Lyrics::scrollCallback(NcScreen *screen, enum NcScroll where)
{
	fromScreen(screen)->w.scroll(to_cpp_scroll(where));
}

void Lyrics::switchToCallback(NcScreen *screen)
{
	Lyrics *lyrics = fromScreen(screen);
	
	if (screenLegacySwitchChanged())
	{
		SwitchTo::finishNativeSwitch(lyrics);
		nc_lyrics_screen_reset_scroll_begin(&lyrics->m_screen);
		drawHeader();
	}
	else
		lyrics->switchToPreviousScreen();
}

void Lyrics::resizeCallback(NcScreen *screen)
{
	Lyrics *lyrics = fromScreen(screen);
	size_t x_offset;
	size_t width;

	lyrics->getWindowResizeParams(x_offset, width);
	nc_lyrics_screen_set_geometry(&lyrics->m_screen,
	                              static_cast<int64>(x_offset),
	                              static_cast<int64>(width),
	                              ui_state_legacy_main_start_y(),
	                              ui_state_legacy_main_height());
	lyrics->w.resize(nc_lyrics_screen_width(&lyrics->m_screen),
	                 nc_lyrics_screen_height(&lyrics->m_screen));
	lyrics->w.moveTo(nc_lyrics_screen_start_x(&lyrics->m_screen),
	                 nc_lyrics_screen_start_y(&lyrics->m_screen));
	lyrics->hasToBeResized = false;
}

int32 Lyrics::windowTimeoutCallback(NcScreen *screen)
{
	(void)screen;
	return defaultWindowTimeout;
}

char *Lyrics::titleCallback(NcScreen *screen)
{
	static std::string result;
	Lyrics *lyrics = fromScreen(screen);

	result = "Lyrics";
	if (!lyrics->m_song.empty())
	{
		result += ": ";
		size_t scroll_begin = static_cast<size_t>(
			nc_lyrics_screen_scroll_begin(&lyrics->m_screen));
		result += Scroller(
			Format::stringify<char>(Format::parse("{%a - %t}|{%f}"),
			                       &lyrics->m_song),
			scroll_begin,
			COLS - Utf8::width(result) - (Config.design == NCM_DESIGN_ALTERNATIVE
			                          ? 2
			                          : global_volume_state_len()));
		nc_lyrics_screen_set_scroll_begin(&lyrics->m_screen,
		                                  static_cast<int64>(scroll_begin));
	}
	return const_cast<char *>(result.c_str());
}

void Lyrics::updateCallback(NcScreen *screen)
{
	Lyrics *lyrics = fromScreen(screen);

	if (lyrics->m_worker.valid())
	{
		auto buffer = lyrics->m_shared_buffer->acquire();
		if (!buffer->empty())
		{
			lyrics->w << *buffer;
			buffer->clear();
			nc_lyrics_screen_request_refresh(&lyrics->m_screen);
		}

		if (lyrics->m_worker.wait_for(std::chrono::seconds(0))
		 == std::future_status::ready)
		{
			auto fetched_lyrics = lyrics->m_worker.get();
			if (fetched_lyrics.first)
			{
				lyrics->w.clear();
				lyrics->w << Charset::utf8ToLocale(fetched_lyrics.second);
				std::string filename = lyricsFilename(lyrics->m_song);
				if (!saveLyrics(filename, fetched_lyrics.second))
					Statusbar::printf("Couldn't save lyrics as \"%1%\": %2%",
					                  filename, strerror(errno));
			}
			else
				lyrics->w << "\nLyrics were not found.\n";
			lyrics->clearWorker();
			nc_lyrics_screen_request_refresh(&lyrics->m_screen);
		}
	}

	if (nc_lyrics_screen_take_refresh_request(&lyrics->m_screen))
	{
		lyrics->w.flush();
		lyrics->w.refresh();
	}
}

void Lyrics::mouseButtonPressedCallback(NcScreen *screen, MEVENT event)
{
	scrollpadMouseButtonPressed(fromScreen(screen)->w, event);
}

bool Lyrics::isLockableCallback(NcScreen *screen)
{
	(void)screen;
	return true;
}

bool Lyrics::isMergableCallback(NcScreen *screen)
{
	(void)screen;
	return true;
}

void Lyrics::destroyCallback(NcScreen *screen)
{
	Lyrics *lyrics = fromScreen(screen);

	if (myLyrics == lyrics)
		myLyrics = nullptr;
	delete lyrics;
}

void Lyrics::fetch(const MPD::Song &s)
{
	if (!m_worker.valid() || s != m_song)
	{
		stopDownload();
		w.clear();
		w.reset();
		m_song = s;
		if (loadLyrics(w, lyricsFilename(m_song)))
		{
			clearWorker();
			nc_lyrics_screen_request_refresh(&m_screen);
		}
		else
		{
			m_download_stopper = std::make_shared<std::atomic<bool>>(false);
			m_shared_buffer = std::make_shared<Shared<NC::Buffer>>();
			m_worker = std::async(
				std::launch::async,
				std::bind(downloadLyrics,
				          m_song, m_shared_buffer, m_download_stopper, m_fetcher));
		}
	}
}

void Lyrics::refetchCurrent()
{
	std::string filename = lyricsFilename(m_song);
	if (std::remove(filename.c_str()) == -1 && errno != ENOENT)
	{
		const char msg[] = "Couldn't remove \"%1%\": %2%";
		Statusbar::printf(msg, Utf8::shorten(filename, COLS - const_strlen(msg) - 25),
		                  strerror(errno));
	}
	else
	{
		clearWorker();
		fetch(m_song);
	}
}

void Lyrics::edit()
{
	if (Config.external_editor.empty())
	{
		Statusbar::print("external_editor variable has to be set in configuration file");
		return;
	}

	Statusbar::print("Opening lyrics in external editor...");

	std::string filename = lyricsFilename(m_song);
	escapeSingleQuotes(filename);
	if (Config.use_console_editor)
	{
		runExternalConsoleCommand(Config.external_editor + " '" + filename + "'");
		fetch(m_song);
	}
	else
		runExternalCommand(Config.external_editor + " '" + filename + "'", false);
}

void Lyrics::toggleFetcher()
{
	if (m_fetcher != nullptr)
	{
		auto fetcher = std::find_if(Config.lyrics_fetchers.begin(),
		                            Config.lyrics_fetchers.end(),
		                            [this](auto &f) { return f.get() == m_fetcher; });
		assert(fetcher != Config.lyrics_fetchers.end());
		++fetcher;
		if (fetcher != Config.lyrics_fetchers.end())
			m_fetcher = fetcher->get();
		else
			m_fetcher = nullptr;
	}
	else
	{
		assert(!Config.lyrics_fetchers.empty());
		m_fetcher = Config.lyrics_fetchers[0].get();
	}

	if (m_fetcher != nullptr)
		Statusbar::printf("Using lyrics fetcher: %s", m_fetcher->name());
	else
		Statusbar::print("Using all lyrics fetchers");
}

/* For HTTP(S) streams, fetchInBackground makes ncmpcpp crash: the lyrics_file
 * gets set to the stream URL, which is too long, hence
 * std::filesystem::exists() fails.
 * Possible solutions:
 * - truncate the URL and use that as a filename. Problem: resulting filename
 *   might not be unique.
 * - generate filenames in a different way for streams. Problem: what is a good
 *   method for this? Perhaps hashing -- but then the lyrics filenames are not
 *   informative.
 * - skip fetching lyrics for streams with URLs that are too long. Problem:
 *   this leads to inconsistent behavior since lyrics will be fetched for some
 *   streams but not others.
 * - avoid fetching lyrics for streams altogether.
 *
 * We choose the last solution, and ignore streams when fetching lyrics. This
 * is because fetching lyrics for a stream may not be accurate (streams do not
 * always provide the necessary metadata to look up lyrics reliably).
 * Furthermore, fetching lyrics for streams is not necessarily desirable.
 */
void Lyrics::fetchInBackground(const MPD::Song &s, bool notify_)
{
	auto consumer_impl = [this] {
		std::string lyrics_file;
		while (true)
		{
			ConsumerState::Song cs;
			{
				auto consumer = m_consumer_state.acquire();
				assert(consumer->running);
				if (consumer->songs.empty())
				{
					consumer->running = false;
					break;
				}
				lyrics_file = lyricsFilename(consumer->songs.front().song());

				// For long filenames (e.g. http(s) stream URLs), std::filesystem::exists() fails.
				// This if condition is fine, because evaluation order is guaranteed.
				if (!consumer->songs.front().song().isStream() && !std::filesystem::exists(lyrics_file))
				{
					cs = consumer->songs.front();
					if (cs.notify())
					{
						consumer->message = "Fetching lyrics for \""
							+ Format::stringify<char>(Config.song_status_format, &cs.song())
							+ "\"...";
					}
				}
				consumer->songs.pop();
			}
			if (!cs.song().empty() && !cs.song().isStream())
			{
				auto lyrics = downloadLyrics(cs.song(), nullptr, nullptr, m_fetcher);
				if (lyrics.first)
					saveLyrics(lyrics_file, lyrics.second);
			}
		}
	};

	auto consumer = m_consumer_state.acquire();
	consumer->songs.emplace(s, notify_);
	// Start the consumer if it's not running.
	if (!consumer->running)
	{
		std::thread t(consumer_impl);
		t.detach();
		consumer->running = true;
	}
}

std::optional<std::string> Lyrics::tryTakeConsumerMessage()
{
	std::optional<std::string> result;
	auto consumer = m_consumer_state.acquire();
	if (consumer->message)
	{
		result = std::move(consumer->message);
		consumer->message = std::nullopt;
	}
	return result;
}

void Lyrics::clearWorker()
{
	m_shared_buffer.reset();
	m_worker = std::future<LyricsFetcher::Result>();
}

void Lyrics::stopDownload()
{
	if (m_download_stopper)
		m_download_stopper->store(true);
}
