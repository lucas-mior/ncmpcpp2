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

#ifndef NCMPCPP_LYRICS_H
#define NCMPCPP_LYRICS_H

#include <atomic>
#include <future>
#include <memory>
#include <optional>
#include <queue>
#include <string>

#include "interfaces.h"
#include "lyrics_fetcher.h"
#include "screens/nc_lyrics.h"
#include "screens/screen_cpp_compat.h"
#include "song.h"
#include "utility/shared_resource.h"

struct LyricsDownloadResult
{
	bool success;
	std::string text;
};

struct Lyrics: BaseScreen, Tabbable
{
	Lyrics();
	virtual ~Lyrics();
	
	// BaseScreen implementation
	virtual bool isActiveWindow(const NC::Window &w_) const override;
	virtual NC::Window *activeWindow() override;
	virtual const NC::Window *activeWindow() const override;
	virtual void refresh() override;
	virtual void refreshWindow() override;
	virtual void scroll(enum NcScroll where) override;
	virtual void resize() override;
	virtual void switchTo() override;
	virtual int windowTimeout() override;
	virtual std::string title() override;
	virtual ScreenType type() override { return NCM_SCREEN_TYPE_LYRICS; }
	virtual void update() override;
	virtual void mouseButtonPressed(MEVENT me) override;
	virtual bool isLockable() override;
	virtual bool isMergable() override;
	virtual NcScreen *nativeScreen() override;
	virtual const NcScreen *nativeScreen() const override;

	// other members
	void fetch(const MPD::Song &s);
	void refetchCurrent();
	void edit();
	void toggleFetcher();

	void fetchInBackground(const MPD::Song &s, bool notify_);
	std::optional<std::string> tryTakeConsumerMessage();

private:
	struct ConsumerState
	{
		struct Song
		{
			Song()
				: m_notify(false)
			{ }

			Song(const MPD::Song &s, bool notify_)
				: m_song(s), m_notify(notify_)
			{ }

			const MPD::Song &song() const { return m_song; }
			bool notify() const { return m_notify; }

		private:
			MPD::Song m_song;
			bool m_notify;
		};

		ConsumerState()
			: running(false)
		{ }

		bool running;
		std::queue<Song> songs;
		std::optional<std::string> message;
	};

	void clearWorker();
	void stopDownload();
	NcScreenCallbacks makeCallbacks();

	static Lyrics *fromScreen(NcScreen *screen);
	static NcWindow *activeWindowCallback(NcScreen *screen);
	static void refreshCallback(NcScreen *screen);
	static void refreshWindowCallback(NcScreen *screen);
	static void scrollCallback(NcScreen *screen, enum NcScroll where);
	static void switchToCallback(NcScreen *screen);
	static void resizeCallback(NcScreen *screen);
	static int32 windowTimeoutCallback(NcScreen *screen);
	static char *titleCallback(NcScreen *screen);
	static void updateCallback(NcScreen *screen);
	static void mouseButtonPressedCallback(NcScreen *screen, MEVENT event);
	static bool isLockableCallback(NcScreen *screen);
	static bool isMergableCallback(NcScreen *screen);
	static void destroyCallback(NcScreen *screen);

	NC::Scrollpad w;
	NcLyricsScreen m_screen;

	std::shared_ptr<Shared<NC::Buffer>> m_shared_buffer;
	std::shared_ptr<std::atomic<bool>> m_download_stopper;

	MPD::Song m_song;
	NcmLyricsFetcherDef *m_fetcher;
	std::future<LyricsDownloadResult> m_worker;

	Shared<ConsumerState> m_consumer_state;
};

extern Lyrics *myLyrics;

#endif // NCMPCPP_LYRICS_H
