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

#include <string>
#include <netinet/tcp.h>
#include <netinet/in.h>

#include "curses/menu_impl.h"
#include "curses/nc_cyclic_buffer.h"
#include "screens/browser.h"
#include "charset.h"
#include "format_impl.h"
#include "global.h"
#include "cbase/base_macros.h"
#include "ui_state.h"
#include "helpers_legacy.h"
#include "macro_utilities.h"
#include "screens/lyrics.h"
#include "screens/media_library.h"
#include "screens/native_c_screens.h"
#include "screens/playlist.h"
#include "screens/playlist_editor.h"
#include "screens/search_engine.h"
#include "screens/sel_items_adder.h"
#include "settings_legacy.h"
#include "status_legacy.h"
#include "status.h"
#include "statusbar.h"
#include "screens/tag_editor.h"
#include "screens/visualizer.h"
#include "title_legacy.h"
#include "utility/string.h"




namespace {

void initialize_status();

bool m_status_initialized;

char m_consume;
char m_crossfade;
char m_db_updating;
char m_repeat;
char m_random;
char m_single;

int m_current_song_id;
int m_current_song_pos;
unsigned m_elapsed_time;
unsigned m_kbps;
MPD::PlayerState m_player_state;
unsigned m_playlist_version;
unsigned m_playlist_length;
unsigned m_total_time;
int m_volume;

enum NcmStatusPlayerState legacy_player_state_to_c(MPD::PlayerState state)
{
	switch (state)
	{
		case MPD::psUnknown:
			return NCM_STATUS_PLAYER_UNKNOWN;
		case MPD::psStop:
			return NCM_STATUS_PLAYER_STOP;
		case MPD::psPlay:
			return NCM_STATUS_PLAYER_PLAY;
		case MPD::psPause:
			return NCM_STATUS_PLAYER_PAUSE;
	}
	throw std::logic_error("unreachable");
}

MPD::PlayerState c_player_state_to_legacy(enum NcmStatusPlayerState state)
{
	switch (state)
	{
		case NCM_STATUS_PLAYER_UNKNOWN:
			return MPD::psUnknown;
		case NCM_STATUS_PLAYER_STOP:
			return MPD::psStop;
		case NCM_STATUS_PLAYER_PLAY:
			return MPD::psPlay;
		case NCM_STATUS_PLAYER_PAUSE:
			return MPD::psPause;
	}
	throw std::logic_error("unreachable");
}

void syncLegacyStatusPrimaryStateFromC()
{
	m_current_song_id = ncm_status_state_current_song_id();
	m_current_song_pos = ncm_status_state_current_song_position();
	m_elapsed_time = ncm_status_state_elapsed_time();
	m_kbps = ncm_status_state_kbps();
	m_player_state = c_player_state_to_legacy(ncm_status_state_player());
	m_playlist_version = ncm_status_state_playlist_version();
	m_playlist_length = ncm_status_state_playlist_length();
	m_total_time = ncm_status_state_total_time();
	m_volume = ncm_status_state_volume();
}

void syncLegacyStatusFlagsFromC()
{
	m_consume = ncm_status_state_consume() ? 'c' : 0;
	m_crossfade = ncm_status_state_crossfade() ? 'x' : 0;
	m_db_updating = ncm_status_state_database_updating() ? 'U' : 0;
	m_repeat = ncm_status_state_repeat() ? 'r' : 0;
	m_random = ncm_status_state_random() ? 'z' : 0;
	m_single = ncm_status_state_single() ? 's' : 0;
}

void syncLegacyStatusStateFromC()
{
	syncLegacyStatusPrimaryStateFromC();
	syncLegacyStatusFlagsFromC();
}

void legacy_status_playlist_changed(uint32 previous_version, void *user)
{
	(void)user;
	syncLegacyStatusPrimaryStateFromC();
	Status::Changes::playlist(previous_version);
}

void legacy_status_stored_playlists_changed(void *user)
{
	(void)user;
	syncLegacyStatusPrimaryStateFromC();
	Status::Changes::storedPlaylists();
}

void legacy_status_database_changed(void *user)
{
	(void)user;
	syncLegacyStatusPrimaryStateFromC();
	Status::Changes::database();
}

void legacy_status_player_state_changed(void *user)
{
	(void)user;
	syncLegacyStatusPrimaryStateFromC();
	Status::Changes::playerState();
}

void legacy_status_song_id_changed(int32 song_id, void *user)
{
	(void)user;
	syncLegacyStatusPrimaryStateFromC();
	Status::Changes::songID(song_id);
	m_current_song_id = song_id;
}

void legacy_status_refresh_footer(void *user)
{
	(void)user;
	static_cast<NC::Window *>(ui_state_footer_legacy_window())->refresh();
}

void legacy_status_refresh_visible_screens(void *user)
{
	(void)user;
	applyToVisibleScreens(nc_screen_refresh_window);
}

void legacy_status_initialize(void *user)
{
	(void)user;
	initialize_status();
}

void registerLegacyStatusHooks()
{
	NcmStatusHooks hooks = {
		nullptr,
		legacy_status_playlist_changed,
		legacy_status_stored_playlists_changed,
		legacy_status_database_changed,
		legacy_status_player_state_changed,
		legacy_status_song_id_changed,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		legacy_status_refresh_footer,
		legacy_status_refresh_visible_screens,
	};

	ncm_status_set_hooks(&hooks);
	ncm_status_set_initialize_hook(legacy_status_initialize, nullptr);
}

void drawTitle(const MPD::Song &np)
{
	assert(!np.empty());
	windowTitle(Format::stringify<char>(Config.song_window_title_format, &np));
}

std::string playerStateToString(MPD::PlayerState ps)
{
	std::string result;
	switch (ps)
	{
		case MPD::psUnknown:
			switch (Config.design)
			{
				case NCM_DESIGN_ALTERNATIVE:
					result = "[unknown]";
					break;
				case NCM_DESIGN_CLASSIC:
					break;
			}
			break;
		case MPD::psPlay:
			switch (Config.design)
			{
				case NCM_DESIGN_ALTERNATIVE:
					result = "[playing]";
					break;
				case NCM_DESIGN_CLASSIC:
					result = "Playing:";
					break;
			}
			break;
		case MPD::psPause:
			switch (Config.design)
			{
				case NCM_DESIGN_ALTERNATIVE:
					result = "[paused]";
					break;
				case NCM_DESIGN_CLASSIC:
					result = "Paused:";
					break;
			}
			break;
		case MPD::psStop:
			switch (Config.design)
			{
				case NCM_DESIGN_ALTERNATIVE:
					result = "[stopped]";
					break;
				case NCM_DESIGN_CLASSIC:
					break;
			}
			break;
	}
	return result;
}

extern "C" void ncm_statusbar_legacy_mpd_callback(void)
{
	NcmError error;

	ncm_error_clear(&error);
	(void)ncm_status_update_from_noidle(&global_mpd, nullptr, &error);
}

void initialize_status()
{
	// get full info about new connection
	NcmError error;

	ncm_error_clear(&error);
	registerLegacyStatusHooks();
	(void)ncm_status_update_full(&global_mpd, nullptr, &error);
	syncLegacyStatusStateFromC();

	if (Config.jump_to_now_playing_song_at_start)
	{
		int curr_pos = Status::State::currentSongPosition();
		if  (curr_pos >= 0)
		{
			myPlaylist->main().highlight(curr_pos);
			if (isVisible(myPlaylist))
				myPlaylist->refresh();
		}
	}

	// Set TCP_NODELAY on the tcp socket as we are using write-write-read pattern
	// a lot (noidle - write, command - write, then read the result of command),
	// which kills the performance.
	int flag = 1;
	setsockopt(Mpd.GetFD(), IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

	myBrowser->fetchSupportedExtensions();
#	ifdef ENABLE_OUTPUTS
	native_c_screen_outputs_fetch_list();
#	endif // ENABLE_OUTPUTS
#	ifdef ENABLE_VISUALIZER
	myVisualizer->CloseDataSource();
	myVisualizer->OpenDataSource();
	myVisualizer->FindOutputID();
#	endif // ENABLE_VISUALIZER

	m_status_initialized = true;
	static_cast<NC::Window *>(ui_state_footer_legacy_window())->addFDCallback(Mpd.GetFD(), Statusbar::Helpers::mpd);
	if (Config.connected_message_on_startup)
	{
		Statusbar::printf("Connected to %1%", Mpd.GetHostname());
	}
}

}

/*************************************************************************/

void Status::handleClientError(MPD::ClientError &e)
{
	if (!e.clearable())
		Mpd.Disconnect();
	Statusbar::printf("ncmpcpp: %1%", e.what());
}

void Status::handleServerError(MPD::ServerError &e)
{
	Statusbar::printf("MPD: %1%", e.what());
	if (e.code() == MPD_SERVER_ERROR_PERMISSION)
	{
		try
		{
			NC::Window::ScopedPromptHook helper(*static_cast<NC::Window *>(ui_state_footer_legacy_window()), nullptr);
			Statusbar::put() << "Password: ";
			Mpd.SetPassword(static_cast<NC::Window *>(ui_state_footer_legacy_window())->prompt("", -1, true));
			Mpd.SendPassword();
			Statusbar::print("Password accepted");
		}
		// SendPassword might throw if connection is closed
		catch (MPD::ClientError &e_prim)
		{
			handleClientError(e_prim);
		}
		// Wrong password, we'll ask again later
		catch (MPD::ServerError &e_prim)
		{
			Statusbar::printf("MPD: %1%", e_prim.what());
		}
		// If prompt asking for a password is aborted, exit the application to
		// prevent getting stuck in the prompt indefinitely.
		catch (NC::PromptAborted &)
		{
			Actions::ExitMainLoop = true;
		}
	}
}

/*************************************************************************/

void Status::trace(bool update_timer, bool update_window_timeout)
{
	NcmError error;

	ncm_error_clear(&error);
	registerLegacyStatusHooks();
	ncm_status_trace(&global_mpd, update_timer, update_window_timeout, &error);
}

void Status::update(int event)
{
	NcmError error;

	ncm_error_clear(&error);
	registerLegacyStatusHooks();
	if (event == -1)
	{
		(void)ncm_status_update_full(&global_mpd, nullptr, &error);
	}
	else
	{
		(void)ncm_status_update(&global_mpd, event, &error);
	}
	syncLegacyStatusStateFromC();
	m_status_initialized = true;
}

void Status::clear()
{
	// reset local variables
	m_status_initialized = false;
	m_repeat = 0;
	m_random = 0;
	m_single = 0;
	m_consume = 0;
	m_crossfade = 0;
	m_db_updating = 0;
	m_current_song_id = -1;
	m_current_song_pos = -1;
	m_kbps = 0;
	m_player_state = MPD::psUnknown;
	m_playlist_length = 0;
	m_playlist_version = 0;
	m_total_time = 0;
	m_volume = -1;
	ncm_status_clear();
}

/*************************************************************************/

bool Status::State::consume()
{
	return ncm_status_state_consume();
}

bool Status::State::crossfade()
{
	return ncm_status_state_crossfade();
}

bool Status::State::repeat()
{
	return ncm_status_state_repeat();
}

bool Status::State::random()
{
	return ncm_status_state_random();
}

bool Status::State::single()
{
	return ncm_status_state_single();
}

int Status::State::currentSongID()
{
	return ncm_status_state_current_song_id();
}

int Status::State::currentSongPosition()
{
	return ncm_status_state_current_song_position();
}

unsigned Status::State::playlistLength()
{
	return ncm_status_state_playlist_length();
}

unsigned Status::State::elapsedTime()
{
	return ncm_status_state_elapsed_time();
}

MPD::PlayerState Status::State::player()
{
	return c_player_state_to_legacy(ncm_status_state_player());
}

unsigned Status::State::totalTime()
{
	return ncm_status_state_total_time();
}

int Status::State::volume()
{
	return ncm_status_state_volume();
}

/*************************************************************************/

void Status::Changes::playlist(unsigned previous_version)
{
	{
		ScopedUnfilteredMenu<MPD::Song> sunfilter(ReapplyFilter::Yes, myPlaylist->main());

		if (m_playlist_length < myPlaylist->main().size())
		{
			auto it = myPlaylist->main().begin()+m_playlist_length;
			auto end = myPlaylist->main().end();
			for (; it != end; ++it)
				myPlaylist->unregisterSong(it->value());
			myPlaylist->main().resizeList(m_playlist_length);
		}

		MPD::SongIterator s = Mpd.GetPlaylistChanges(previous_version), end;
		for (; s != end; ++s)
		{
			size_t pos = s->getPosition();
			myPlaylist->registerSong(*s);
			if (pos < myPlaylist->main().size())
			{
				// if song's already in playlist, replace it with a new one
				MPD::Song &old_s = myPlaylist->main()[pos].value();
				myPlaylist->unregisterSong(old_s);
				old_s = std::move(*s);
			}
			else // otherwise just add it to playlist
				myPlaylist->main().addItem(std::move(*s));
		}
	}

	myPlaylist->reloadTotalLength();
	myPlaylist->reloadRemaining();

	// When we're in multi-column screens, it might happen that songs visible on
	// the screen are added, but they will not be immediately marked as such
	// because the window that contains them is not the active one at the moment,
	// so we need to refresh them manually.
	if (isVisible(myLibrary)
	    && !myLibrary->isActiveWindow(myLibrary->Songs))
		myLibrary->Songs.refresh();
	if (isVisible(myPlaylistEditor)
	    && !myPlaylistEditor->isActiveWindow(myPlaylistEditor->Content))
		myPlaylistEditor->Content.refresh();
}

void Status::Changes::storedPlaylists()
{
	myPlaylistEditor->requestPlaylistsUpdate();
	myPlaylistEditor->requestContentUpdate();
	if (!myBrowser->isLocal() && myBrowser->inRootDirectory())
		myBrowser->requestUpdate();
}

void Status::Changes::database()
{
	myBrowser->requestUpdate();
#	ifdef HAVE_TAGLIB_H
	myTagEditor->Dirs->clear();
#	endif // HAVE_TAGLIB_H
	myLibrary->requestTagsUpdate();
	myLibrary->requestAlbumsUpdate();
	myLibrary->requestSongsUpdate();
}

void Status::Changes::playerState()
{
	if (!Config.execute_on_player_state_change.empty())
	{
		auto stateToEnv = [](MPD::PlayerState st) -> const char * {
			switch (st)
			{
			case MPD::psPlay:    return "play";
			case MPD::psStop:    return "stop";
			case MPD::psPause:   return "pause";
			case MPD::psUnknown: return "unknown";
			}
			throw std::logic_error("unreachable");
		};
		setenv("MPD_PLAYER_STATE", stateToEnv(m_player_state), 1);
		// Since we're setting a MPD_PLAYER_STATE, we need to block.
		runExternalCommand(Config.execute_on_player_state_change, true);
		unsetenv("MPD_PLAYER_STATE");
	}

	switch (m_player_state)
	{
		case MPD::psPlay:
		{
			auto np = myPlaylist->nowPlayingSong();
			if (!np.empty())
				drawTitle(np);
			myPlaylist->reloadRemaining();
			break;
		}
		case MPD::psStop:
			windowTitle("ncmpcpp " VERSION);
			if (ncm_progressbar_is_unlocked())
				ncm_progressbar_draw(0, 0);
			myPlaylist->reloadRemaining();
			if (Config.design == NCM_DESIGN_ALTERNATIVE)
			{
				*static_cast<NC::Window *>(ui_state_header_legacy_window()) << NC::XY(0, 0) << NC::TermManip::ClearToEOL;
				*static_cast<NC::Window *>(ui_state_header_legacy_window()) << NC::XY(0, 1) << NC::TermManip::ClearToEOL;
				mixer();
				flags();
			}
#			ifdef ENABLE_VISUALIZER
			if (isVisible(myVisualizer))
				myVisualizer->Clear();
#			endif // ENABLE_VISUALIZER
			break;
		default:
			break;
	}

	std::string state = playerStateToString(m_player_state);
	if (Config.design == NCM_DESIGN_ALTERNATIVE)
	{
		*static_cast<NC::Window *>(ui_state_header_legacy_window()) << NC::XY(0, 1) << NC_FORMAT_BOLD << state << NC_FORMAT_NO_BOLD;
		static_cast<NC::Window *>(ui_state_header_legacy_window())->refresh();
	}
	else if (ncm_statusbar_is_unlocked() && Config.statusbar_visibility)
	{
		*static_cast<NC::Window *>(ui_state_footer_legacy_window()) << NC::XY(0, 1);
		if (state.empty())
			*static_cast<NC::Window *>(ui_state_footer_legacy_window()) << NC::TermManip::ClearToEOL;
		else
			*static_cast<NC::Window *>(ui_state_footer_legacy_window()) << NC_FORMAT_BOLD << state << NC_FORMAT_NO_BOLD;
	}

	// needed for immediate display after starting
	// player from stopped state or seeking
	elapsedTime(false);
}

void Status::Changes::songID(int song_id)
{
	// update information about current song
	myPlaylist->reloadRemaining();
	ncm_status_changes_reset_song_scroll();
#	ifdef ENABLE_VISUALIZER
	myVisualizer->ResetAutoScaleMultiplier();
#	endif // ENABLE_VISUALIZER
	if (m_player_state != MPD::psStop)
	{
		auto &pl = myPlaylist->main();

		// try to find the song with new id in the playlist
		auto it = std::find_if(pl.beginV(), pl.endV(), [song_id](const MPD::Song &s) {
			return s.getID() == unsigned(song_id);
		});
		// if it's not there (playlist may be outdated), fetch it
		const auto &s = it != pl.endV() ? *it : Mpd.GetCurrentSong();
		if (!s.empty())
		{
			if (!Config.execute_on_song_change.empty())
			{
				// We need to block to allow sending output to the terminal so a script
				// can e.g. set the album art.
				runExternalCommand(Config.execute_on_song_change, true);
			}

			if (Config.fetch_lyrics_in_background)
				myLyrics->fetchInBackground(s, false);

			drawTitle(s);

			if (Config.autocenter_mode)
				myPlaylist->locateSong(s);

			if (Config.now_playing_lyrics
			    && isVisible(myLyrics)
			    && myLyrics->previousScreen() == myPlaylist)
				myLyrics->fetch(s);
		}
	}
	elapsedTime(false);
}

void Status::Changes::elapsedTime(bool update_elapsed)
{
	ncm_status_changes_elapsed_time(update_elapsed);
}


void Status::Changes::flags()
{
	ncm_status_changes_flags();
}


void Status::Changes::mixer()
{
	ncm_status_changes_mixer();
}


void Status::Changes::outputs()
{
	ncm_status_changes_outputs();
}

