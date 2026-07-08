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
#include <cerrno>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <regex>
#include <string>

#include "actions_legacy.h"
#include "app_legacy_bridge.h"
#include "charset.h"
#include "config.h"
#include "configuration_legacy.h"
#include "display.h"
#include "app_controller.h"
#include "global.h"
#include "ui_state.h"
#include "screens/screen_cpp_legacy.h"
#include "mpdpp.h"
#include "helpers_legacy.h"
#include "statusbar_legacy.h"
#include "status_legacy.h"
#include "utility/comparators.h"
#include "utility/conversion.h"
#include "utility/scoped_value.h"

#include "curses/menu_impl.h"
#include "bindings_legacy.h"
#include "screens/browser.h"
#include "screens/native_c_screens.h"
#include "screens/media_library.h"
#include "screens/lastfm.h"
#include "screens/lyrics.h"
#include "screens/playlist.h"
#include "screens/playlist_editor.h"
#include "screens/sort_playlist.h"
#include "screens/search_engine.h"
#include "screens/sel_items_adder.h"
#include "utility/readline.h"
#include "utility/string.h"
#include "utility/string_format.h"
#include "c/ncm_type_conversions.h"
#include "screens/tag_editor.h"
#include "screens/tiny_tag_editor.h"
#include "screens/visualizer.h"
#include "title_legacy.h"
#include "tags.h"

#ifdef HAVE_TAGLIB_H
# include "c/ncm_taglib.h"
#endif // HAVE_TAGLIB_H


namespace ph = std::placeholders;

namespace {

void legacy_noidle_status_update(int32 flags, void *)
{
	Status::update(flags);
}

std::string legacy_mpd_error_message(const NcmError &error)
{
	if (error.message[0] != '\0')
		return error.message;

	char *message = ncm_mpd_client_error_message(&global_mpd);
	if (message != nullptr && message[0] != '\0')
		return message;

	return "MPD command failed";
}

void legacy_report_mpd_error(const NcmError &error)
{
	if (ncm_mpd_client_error_code(&global_mpd) == MPD_ERROR_SERVER
	    || error.code == MPD_ERROR_SERVER)
	{
		MPD::ServerError server_error(
		    ncm_mpd_client_server_error_code(&global_mpd),
		    legacy_mpd_error_message(error),
		    ncm_mpd_client_error_clearable(&global_mpd));
		Status::handleServerError(server_error);
	}
	else
	{
		MPD::ClientError client_error(
		    static_cast<mpd_error>(error.code),
		    legacy_mpd_error_message(error),
		    ncm_mpd_client_error_clearable(&global_mpd));
		Status::handleClientError(client_error);
	}
}

std::vector<std::shared_ptr<Actions::BaseAction>> AvailableActions;

char *itemTypeName(MPD::Item::Type type)
{
	switch (type)
	{
		case MPD::Item::Type::Directory:
			return ncm_item_type_name(NCM_ITEM_DIRECTORY);
		case MPD::Item::Type::Song:
			return ncm_item_type_name(NCM_ITEM_SONG);
		case MPD::Item::Type::Playlist:
			return ncm_item_type_name(NCM_ITEM_PLAYLIST);
	}
	return ncm_item_type_name(NCM_ITEM_UNKNOWN);
}

std::string storedPlaylistPath(const MPD::Playlist &playlist)
{
	return playlist.path();
}

std::string currentStoredPlaylistPath()
{
	return storedPlaylistPath(myPlaylistEditor->Playlists.current()->value());
}

void populateActions();

bool scrollTagCanBeRun(NC::List *&list, const SongList *&songs);
void scrollTagUpRun(NC::List *list, const SongList *songs, enum NcmSongGetter getter);
void scrollTagDownRun(NC::List *list, const SongList *songs, enum NcmSongGetter getter);

void seek(SearchDirection sd);
void findItem(const SearchDirection direction);
void listsChangeFinisher();

template <typename Iterator>
bool findSelectedRangeAndPrintInfoIfNot(Iterator &first, Iterator &last)
{
	bool success = findSelectedRange(first, last);
	if (!success)
		Statusbar::print("No range selected");
	return success;
}

template <typename Iterator>
Iterator nextScreenTypeInSequence(Iterator first, Iterator last, ScreenType type)
{
	auto it = std::find(first, last, type);
	if (it == last)
		return first;
	else
	{
		++it;
		if (it == last)
			return first;
		else
			return it;
	}
}

}

namespace Actions {

bool OriginalStatusbarVisibility;
bool ExitMainLoop = false;

size_t HeaderHeight;
size_t FooterHeight;
size_t FooterStartY;

void initializeScreens()
{
	app_controller_init();
	native_c_screens_init_all();

	myPlaylist = new Playlist;
	myBrowser = new Browser;
	mySearcher = new SearchEngine;
	myLibrary = new MediaLibrary;
	myPlaylistEditor = new PlaylistEditor;
	myLyrics = new Lyrics;
	mySelectedItemsAdder = new SelectedItemsAdder;
	mySortPlaylistDialog = new SortPlaylistDialog;
	myLastfm = new Lastfm;

#	ifdef HAVE_TAGLIB_H
	myTinyTagEditor = new TinyTagEditor;
	myTagEditor = new TagEditor;
#	endif // HAVE_TAGLIB_H

#	ifdef ENABLE_VISUALIZER
	myVisualizer = new Visualizer;
#	endif // ENABLE_VISUALIZER

	native_c_screens_register_native_only();
	myPlaylist->registerNativeScreen();
	myBrowser->registerNativeScreen();
	mySearcher->registerNativeScreen();
	myLibrary->registerNativeScreen();
	myPlaylistEditor->registerNativeScreen();
	myLyrics->registerNativeScreen();
	mySelectedItemsAdder->registerNativeScreen();

	mySortPlaylistDialog->registerNativeScreen();
	myLastfm->registerNativeScreen();

#	ifdef HAVE_TAGLIB_H
	myTinyTagEditor->registerNativeScreen();
	myTagEditor->registerNativeScreen();
#	endif // HAVE_TAGLIB_H

#	ifdef ENABLE_VISUALIZER
	myVisualizer->registerNativeScreen();
#	endif // ENABLE_VISUALIZER


}

void setResizeFlags()
{
	native_c_screens_request_registered_resize();
	myPlaylist->hasToBeResized = 1;
	myBrowser->hasToBeResized = 1;
	mySearcher->hasToBeResized = 1;
	myLibrary->hasToBeResized = 1;
	myPlaylistEditor->hasToBeResized = 1;
	myLyrics->hasToBeResized = 1;
	mySelectedItemsAdder->hasToBeResized = 1;
	mySortPlaylistDialog->hasToBeResized = 1;
	myLastfm->hasToBeResized = 1;

#	ifdef HAVE_TAGLIB_H
	myTinyTagEditor->hasToBeResized = 1;
	myTagEditor->hasToBeResized = 1;
#	endif // HAVE_TAGLIB_H

#	ifdef ENABLE_VISUALIZER
	myVisualizer->hasToBeResized = 1;
#	endif // ENABLE_VISUALIZER


}

void resizeScreen(bool reload_main_window)
{
	// update internal screen dimensions
	if (reload_main_window)
	{
		rl_resize_terminal();
		endwin();
		refresh();
		// Remove KEY_RESIZE from input queue, I'm not sure how these make it in.
		getch();
	}

	ui_state_set_screen_size(COLS, LINES);

	std::size_t main_height = static_cast<std::size_t>(std::max(
	    LINES-(Config.design == NCM_DESIGN_ALTERNATIVE ? 7 : 4),
	    0));

	if (!Config.header_visibility)
		main_height += 2;
	if (!Config.statusbar_visibility)
		main_height += 1;
	ui_state_set_main_height(main_height);

	setResizeFlags();

	app_controller_resize_visible_screens();

	if (Config.header_visibility || Config.design == NCM_DESIGN_ALTERNATIVE)
		static_cast<NC::Window *>(ui_state_header_window())->resize(COLS, HeaderHeight);

	FooterStartY = LINES-(Config.statusbar_visibility ? 2 : 1);
	static_cast<NC::Window *>(ui_state_footer_window())->moveTo(0, FooterStartY);
	static_cast<NC::Window *>(ui_state_footer_window())->resize(COLS, Config.statusbar_visibility ? 2 : 1);

	app_controller_refresh_visible_screens();

	Status::Changes::elapsedTime(false);
	Status::Changes::playerState();
	// Note: routines for drawing separator if alternative user
	// interface is active and header is hidden are placed in
	// NcmpcppStatusChanges.StatusFlags
	Status::Changes::flags();
	drawHeader();
	static_cast<NC::Window *>(ui_state_footer_window())->refresh();
	refresh();
}

void setWindowsDimensions()
{
	ui_state_set_screen_size(COLS, LINES);

	std::size_t main_start_y = Config.design == NCM_DESIGN_ALTERNATIVE ? 5 : 2;
	std::size_t main_height = static_cast<std::size_t>(std::max(
	    LINES-(Config.design == NCM_DESIGN_ALTERNATIVE ? 7 : 4),
	    0));

	if (!Config.header_visibility)
	{
		main_start_y -= 2;
		main_height += 2;
	}
	if (!Config.statusbar_visibility)
		main_height += 1;
	ui_state_set_main_geometry(main_start_y, main_height);

	HeaderHeight = Config.design == NCM_DESIGN_ALTERNATIVE ? (Config.header_visibility ? 5 : 3) : 2;
	FooterStartY = LINES-(Config.statusbar_visibility ? 2 : 1);
	FooterHeight = Config.statusbar_visibility ? 2 : 1;
}

bool confirmAction(const std::string &description)
{
	Statusbar::ScopedLock slock;
	Statusbar::put() << description
	<< " [" << NC_FORMAT_BOLD << 'y' << NC_FORMAT_NO_BOLD
	<< '/' << NC_FORMAT_BOLD << 'n' << NC_FORMAT_NO_BOLD
	<< "] ";
	char answer = Statusbar::Helpers::promptReturnOneOf({'y', 'n'});
	if (answer == 'n')
	{
		Statusbar::print("Action cancelled");
		return false;
	}
	return true;
}

bool isMPDMusicDirSet()
{
	if (Config.mpd_music_dir.empty())
	{
		Statusbar::print("Proper mpd_music_dir variable has to be set in configuration file");
		return false;
	}
	return true;
}

BaseAction *runtimeAction(enum NcmActionType type)
{
	NcmActionDef *definition = ncm_action_get(type);
	if (definition == nullptr)
	{
		Statusbar::printf("Unknown action type: %1%", static_cast<int>(type));
		return nullptr;
	}

	if (AvailableActions.empty())
		populateActions();

	auto index = static_cast<size_t>(type);
	if (index >= AvailableActions.size() || AvailableActions[index] == nullptr)
	{
		std::cerr << "fatal: action not implemented: "
		          << definition->name << std::endl;
		std::exit(EXIT_FAILURE);
	}

	return AvailableActions[index].get();
}

BaseAction *runtimeActionByName(char *name, int32 name_len)
{
	NcmActionDef *definition = ncm_action_find(name, name_len);
	if (definition == nullptr)
	{
		Statusbar::printf("Unknown action: %1%", std::string(name, name_len));
		return nullptr;
	}
	return runtimeAction(definition->type);
}

bool canRun(enum NcmActionType type)
{
	BaseAction *action = runtimeAction(type);
	return action != nullptr && action->canBeRun();
}

bool execute(enum NcmActionType type)
{
	BaseAction *action = runtimeAction(type);
	return action != nullptr && action->execute();
}

UpdateEnvironment::UpdateEnvironment()
: BaseAction(Type::UpdateEnvironment, "update_environment")
, m_past()
{ }

void UpdateEnvironment::run(bool update_timer, bool refresh_window, bool mpd_sync)
{

	// update timer, status if necessary etc.
	Status::trace(update_timer, true);

	// show lyrics consumer notification if appropriate
	if (auto message = myLyrics->tryTakeConsumerMessage())
		Statusbar::print(*message);

	// header stuff
	if ((screenLegacyCurrent() == myPlaylist || screenLegacyCurrent() == myBrowser || screenLegacyCurrent() == myLyrics)
	&&  (global_timer_elapsed_ms(m_past) > 500)
	)
	{
		drawHeader();
		m_past = global_timer;
	}

	if (refresh_window)
		app_controller_refresh_current_window();

	// We want to synchronize with MPD during execution of an action chain.
	if (mpd_sync)
	{
		int flags = Mpd.noidle();
		if (flags)
			Status::update(flags);
	}
}

void UpdateEnvironment::run()
{
	run(true, true, true);
}

bool MouseEvent::canBeRun()
{
	return Config.mouse_support;
}

void MouseEvent::run()
{

	m_old_mouse_event = m_mouse_event;
	m_mouse_event = static_cast<NC::Window *>(ui_state_footer_window())->getMouseEvent();

	//Statusbar::printf("(%1%, %2%, %3%)", m_mouse_event.bstate, m_mouse_event.x, m_mouse_event.y);

	if (m_mouse_event.bstate & BUTTON1_PRESSED
	&&  m_mouse_event.y == LINES-(Config.statusbar_visibility ? 2 : 1)
	   ) // progressbar
	{
		if (Status::State::player() == MPD::psStop)
			return;
		Mpd.Seek(Status::State::currentSongPosition(),
			Status::State::totalTime()*m_mouse_event.x/double(COLS));
	}
	else if (m_mouse_event.bstate & BUTTON1_PRESSED
	     &&  (Config.statusbar_visibility || Config.design == NCM_DESIGN_ALTERNATIVE)
	     &&  Status::State::player() != MPD::psStop
	     &&  m_mouse_event.y == (Config.design == NCM_DESIGN_ALTERNATIVE ? 1 : LINES-1)
			 &&  m_mouse_event.x < 9
		) // playing/paused
	{
		Mpd.Toggle();
	}
	else if ((m_mouse_event.bstate & BUTTON5_PRESSED || m_mouse_event.bstate & BUTTON4_PRESSED)
	     &&	 (Config.header_visibility || Config.design == NCM_DESIGN_ALTERNATIVE)
	     &&	 m_mouse_event.y == 0 && size_t(m_mouse_event.x) > COLS-global_volume_state_len()
	) // volume
	{
		if (m_mouse_event.bstate & BUTTON5_PRESSED)
			Actions::execute(NCM_ACTION_VOLUME_DOWN);
		else
			Actions::execute(NCM_ACTION_VOLUME_UP);
	}
	else if (m_mouse_event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED | BUTTON4_PRESSED | BUTTON5_PRESSED))
		app_controller_mouse_button_pressed_current(m_mouse_event);
}

void ScrollUp::run()
{
	app_controller_scroll_current_screen(NC_SCROLL_UP);
	listsChangeFinisher();
}

void ScrollDown::run()
{
	app_controller_scroll_current_screen(NC_SCROLL_DOWN);
	listsChangeFinisher();
}

bool ScrollUpArtist::canBeRun()
{
	return scrollTagCanBeRun(m_list, m_songs);
}

void ScrollUpArtist::run()
{
	scrollTagUpRun(m_list, m_songs, NCM_SONG_GETTER_ARTIST);
}

bool ScrollUpAlbum::canBeRun()
{
	return scrollTagCanBeRun(m_list, m_songs);
}

void ScrollUpAlbum::run()
{
	scrollTagUpRun(m_list, m_songs, NCM_SONG_GETTER_ALBUM);
}

bool ScrollDownArtist::canBeRun()
{
	return scrollTagCanBeRun(m_list, m_songs);
}

void ScrollDownArtist::run()
{
	scrollTagDownRun(m_list, m_songs, NCM_SONG_GETTER_ARTIST);
}

bool ScrollDownAlbum::canBeRun()
{
	return scrollTagCanBeRun(m_list, m_songs);
}

void ScrollDownAlbum::run()
{
	scrollTagDownRun(m_list, m_songs, NCM_SONG_GETTER_ALBUM);
}

void PageUp::run()
{
	app_controller_scroll_current_screen(NC_SCROLL_PAGE_UP);
	listsChangeFinisher();
}

void PageDown::run()
{
	app_controller_scroll_current_screen(NC_SCROLL_PAGE_DOWN);
	listsChangeFinisher();
}

void MoveHome::run()
{
	app_controller_scroll_current_screen(NC_SCROLL_HOME);
	listsChangeFinisher();
}

void MoveEnd::run()
{
	app_controller_scroll_current_screen(NC_SCROLL_END);
	listsChangeFinisher();
}

void ToggleInterface::run()
{
	switch (Config.design)
	{
		case NCM_DESIGN_CLASSIC:
			Config.design = NCM_DESIGN_ALTERNATIVE;
			Config.statusbar_visibility = false;
			break;
		case NCM_DESIGN_ALTERNATIVE:
			Config.design = NCM_DESIGN_CLASSIC;
			Config.statusbar_visibility = OriginalStatusbarVisibility;
			break;
	}
	setWindowsDimensions();
	resizeScreen(false);
	// unlock progressbar
	Progressbar::ScopedLock();
	Status::Changes::mixer();
	Status::Changes::elapsedTime(false);
	Statusbar::printf("User interface: %1%", Config.design);
}

bool JumpToParentDirectory::canBeRun()
{
	return (screenLegacyCurrent() == myBrowser)
#	ifdef HAVE_TAGLIB_H
	    || (screenLegacyCurrent()->activeWindow() == myTagEditor->Dirs)
#	endif // HAVE_TAGLIB_H
	;
}

void JumpToParentDirectory::run()
{
	if (screenLegacyCurrent() == myBrowser)
	{
		if (!myBrowser->inRootDirectory())
		{
			myBrowser->main().reset();
			myBrowser->enterDirectory();
		}
	}
#	ifdef HAVE_TAGLIB_H
	else if (screenLegacyCurrent() == myTagEditor)
	{
		if (myTagEditor->CurrentDir() != "/")
		{
			myTagEditor->Dirs->reset();
			myTagEditor->enterDirectory();
		}
	}
#	endif // HAVE_TAGLIB_H
}

bool RunAction::canBeRun()
{
	m_ha = dynamic_cast<HasActions *>(screenLegacyCurrent());
	return m_ha != nullptr
		&& m_ha->actionRunnable();
}

void RunAction::run()
{
	m_ha->runAction();
}

bool PreviousColumn::canBeRun()
{
	m_hc = dynamic_cast<HasColumns *>(screenLegacyCurrent());
	return m_hc != nullptr
		&& m_hc->previousColumnAvailable();
}

void PreviousColumn::run()
{
	m_hc->previousColumn();
}

bool NextColumn::canBeRun()
{
	m_hc = dynamic_cast<HasColumns *>(screenLegacyCurrent());
	return m_hc != nullptr
		&& m_hc->nextColumnAvailable();
}

void NextColumn::run()
{
	m_hc->nextColumn();
}

bool MasterScreen::canBeRun()
{
	return app_controller_can_show_locked_screen();
}

void MasterScreen::run()
{
	if (app_controller_show_locked_screen())
	{
		syncLegacyScreenPointers();
		drawHeader();
	}
}

bool SlaveScreen::canBeRun()
{
	return app_controller_can_show_inactive_screen();
}

void SlaveScreen::run()
{
	if (app_controller_show_inactive_screen())
	{
		syncLegacyScreenPointers();
		drawHeader();
	}
}

bool VolumeUp::canBeRun()
{
	return Status::State::volume() >= 0;
}

void VolumeUp::run()
{
	Mpd.ChangeVolume(static_cast<int>(Config.volume_change_step));
}

bool VolumeDown::canBeRun()
{
	return Status::State::volume() >= 0;
}

void VolumeDown::run()
{
	Mpd.ChangeVolume(-static_cast<int>(Config.volume_change_step));
}

bool AddItemToPlaylist::canBeRun()
{
	m_hs = dynamic_cast<HasSongs *>(screenLegacyCurrent());
	return m_hs != nullptr && m_hs->itemAvailable();
}

void AddItemToPlaylist::run()
{
	bool success = m_hs->addItemToPlaylist(false);
	if (success)
	{
		app_controller_scroll_current_screen(NC_SCROLL_DOWN);
		listsChangeFinisher();
	}
}

bool PlayItem::canBeRun()
{
	m_hs = dynamic_cast<HasSongs *>(screenLegacyCurrent());
	return m_hs != nullptr && m_hs->itemAvailable();
}

void PlayItem::run()
{
	bool success = m_hs->addItemToPlaylist(true);
	if (success)
		listsChangeFinisher();
}

bool DeletePlaylistItems::canBeRun()
{
	return (screenLegacyCurrent() == myPlaylist && !myPlaylist->main().empty())
	    || (screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Content) && !myPlaylistEditor->Content.empty());
}

void DeletePlaylistItems::run()
{
	if (screenLegacyCurrent() == myPlaylist)
	{
		Statusbar::print("Deleting items...");
		deleteSelectedSongsFromPlaylist(myPlaylist->main());
		Statusbar::print("Item(s) deleted");
	}
	else if (screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Content))
	{
		std::string playlist = currentStoredPlaylistPath();
		auto delete_fun = [playlist](auto &, unsigned pos) {
			Mpd.PlaylistDelete(playlist, pos);
		};
		Statusbar::print("Deleting items...");
		deleteSelectedSongs(myPlaylistEditor->Content, delete_fun);
		Statusbar::print("Item(s) deleted");
	}
}

bool DeleteBrowserItems::canBeRun()
{
	auto check_if_deletion_allowed = []() {
		if (Config.allow_for_physical_item_deletion)
			return true;
		else
		{
			Statusbar::print("Flag \"allow_for_physical_item_deletion\" needs to be enabled in configuration file");
			return false;
		}
	};
	return screenLegacyCurrent() == myBrowser
	    && !myBrowser->main().empty()
	    && isMPDMusicDirSet()
	    && check_if_deletion_allowed();
}

void DeleteBrowserItems::run()
{
	auto get_name = [](const MPD::Item &item) -> std::string {
		std::string iname;
		switch (item.type())
		{
			case MPD::Item::Type::Directory:
				iname = getBasename(item.directory().path());
				break;
			case MPD::Item::Type::Song:
				iname = item.song().getName();
				break;
			case MPD::Item::Type::Playlist:
				iname = getBasename(item.playlist().path());
				break;
		}
		return iname;
	};

	std::string question;
	if (hasSelected(myBrowser->main().begin(), myBrowser->main().end()))
		question = "Delete selected items?";
	else
	{
		const auto &item = myBrowser->main().current()->value();
		// parent directories are not accepted (and they
		// can't be selected, so in other cases it's fine).
		if (myBrowser->isParentDirectory(item))
			return;
		const char msg[] = "Delete \"%1%\"?";
		question = stringFormat(msg,
			Utf8::shorten(get_name(item), COLS-const_strlen(msg)-5)
		);
	}
	if (!confirmAction(question))
		return;

	auto items = getSelectedOrCurrent(
		myBrowser->main().begin(),
		myBrowser->main().end(),
		myBrowser->main().current()
	);
	for (const auto &item : items)
	{
		myBrowser->remove(item->value());
		const char msg[] = "Deleted %1% \"%2%\"";
		Statusbar::printf(msg,
			itemTypeName(item->value().type()),
			Utf8::shorten(get_name(item->value()), COLS-const_strlen(msg))
		);
	}

	if (!myBrowser->isLocal())
		Mpd.UpdateDirectory(myBrowser->currentDirectory());
	myBrowser->requestUpdate();
}

bool DeleteStoredPlaylist::canBeRun()
{
	return screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Playlists);
}

void DeleteStoredPlaylist::run()
{
	if (myPlaylistEditor->Playlists.empty())
		return;
	std::string question;
	if (hasSelected(myPlaylistEditor->Playlists.begin(), myPlaylistEditor->Playlists.end()))
		question = "Delete selected playlists?";
	else
	{
		const char msg[] = "Delete playlist \"%1%\"?";
		question = stringFormat(msg,
			Utf8::shorten(currentStoredPlaylistPath(),
			            COLS-const_strlen(msg)-10));
	}
	if (!confirmAction(question))
		return;

	auto list = getSelectedOrCurrent(
		myPlaylistEditor->Playlists.begin(),
		myPlaylistEditor->Playlists.end(),
		myPlaylistEditor->Playlists.current()
	);
	for (const auto &item : list)
		Mpd.DeletePlaylist(storedPlaylistPath(item->value()));
	Statusbar::printf("%1% deleted", list.size() == 1 ? "Playlist" : "Playlists");
	// force playlists update. this happens automatically, but only after call
	// to Key::read, therefore when we call PlaylistEditor::Update, it won't
	// yet see it, so let's point that it needs to update it.
	myPlaylistEditor->requestPlaylistsUpdate();
}

bool ReplaySong::canBeRun()
{
	return Status::State::currentSongPosition() >= 0;
}

void ReplaySong::run()
{
	Mpd.Play(Status::State::currentSongPosition());
}

void PreviousSong::run()
{
	Mpd.Prev();
}

void NextSong::run()
{
	Mpd.Next();
}

bool Pause::canBeRun()
{
	return Status::State::player() != MPD::psStop;
}

void Pause::run()
{
	Mpd.Toggle();
}

void SavePlaylist::run()
{

	std::string playlist_name;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Save playlist as: ";
		playlist_name = static_cast<NC::Window *>(ui_state_footer_window())->prompt();
	}
	try
	{
		Mpd.SavePlaylist(playlist_name);
		Statusbar::printf("Playlist saved as \"%1%\"", playlist_name);
	}
	catch (MPD::ServerError &e)
	{
		if (e.code() == MPD_SERVER_ERROR_EXIST)
		{
			if (!confirmAction(
				stringFormat("Playlist \"%1%\" already exists, overwrite?", playlist_name)
			))
				return;
			Mpd.DeletePlaylist(playlist_name);
			Mpd.SavePlaylist(playlist_name);
			Statusbar::print("Playlist overwritten");
		}
		else
		{
			Statusbar::printf("Error while saving playlist: %1%", e.what());
			return;
		}
	}
}

void Stop::run()
{
	Mpd.Stop();
}

void Play::run()
{
	Mpd.Play();
}

void ExecuteCommand::run()
{

	std::string cmd_name;
	{
		Statusbar::ScopedLock slock;
		NC::Window::ScopedPromptHook helper(*static_cast<NC::Window *>(ui_state_footer_window()),
			Statusbar::Helpers::TryExecuteImmediateCommand()
		);
		Statusbar::put() << NC_FORMAT_BOLD << ":" << NC_FORMAT_NO_BOLD;
		cmd_name = static_cast<NC::Window *>(ui_state_footer_window())->prompt();
	}

	auto cmd = ncm_bindings_configuration_find_command(
		&Bindings, const_cast<char *>(cmd_name.data()),
		static_cast<int32>(cmd_name.size()));
	if (cmd)
	{
		Statusbar::printf(1, "Executing %1%...", cmd_name);
		bool res = bindings_legacy_execute(&cmd->binding);
		Statusbar::printf("Execution of command \"%1%\" %2%.",
			cmd_name, res ? "successful" : "unsuccessful"
		);
	}
	else
		Statusbar::printf("No command named \"%1%\"", cmd_name);
}

bool MoveSortOrderUp::canBeRun()
{
	return screenLegacyCurrent() == mySortPlaylistDialog;
}

void MoveSortOrderUp::run()
{
	mySortPlaylistDialog->moveSortOrderUp();
}

bool MoveSortOrderDown::canBeRun()
{
	return screenLegacyCurrent() == mySortPlaylistDialog;
}

void MoveSortOrderDown::run()
{
	mySortPlaylistDialog->moveSortOrderDown();
}

bool MoveSelectedItemsUp::canBeRun()
{
	return ((screenLegacyCurrent() == myPlaylist
	    &&  !myPlaylist->main().empty())
	 ||    (screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Content)
	    &&  !myPlaylistEditor->Content.empty()));
}

void MoveSelectedItemsUp::run()
{
	const char *filteredMsg = "Moving items up is disabled in filtered playlist";
	if (screenLegacyCurrent() == myPlaylist)
	{
		if (myPlaylist->main().isFiltered())
			Statusbar::print(filteredMsg);
		else
			moveSelectedItemsUp(
				myPlaylist->main(),
				[](auto &, unsigned from, unsigned to) { Mpd.Move(from, to); });
	}
	else if (screenLegacyCurrent() == myPlaylistEditor)
	{
		if (myPlaylistEditor->Content.isFiltered())
			Statusbar::print(filteredMsg);
		else
		{
			auto playlist = currentStoredPlaylistPath();
			moveSelectedItemsUp(
				myPlaylistEditor->Content,
				[playlist](auto &, unsigned from, unsigned to) {
					Mpd.PlaylistMove(playlist, from, to);
				});
		}
	}
}

bool MoveSelectedItemsDown::canBeRun()
{
	return ((screenLegacyCurrent() == myPlaylist
	    &&  !myPlaylist->main().empty())
	 ||    (screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Content)
	    &&  !myPlaylistEditor->Content.empty()));
}

void MoveSelectedItemsDown::run()
{
	const char *filteredMsg = "Moving items down is disabled in filtered playlist";
	if (screenLegacyCurrent() == myPlaylist)
	{
		if (myPlaylist->main().isFiltered())
			Statusbar::print(filteredMsg);
		else
			moveSelectedItemsDown(
				myPlaylist->main(),
				[](auto &, unsigned from, unsigned to) { Mpd.Move(from, to); });
	}
	else if (screenLegacyCurrent() == myPlaylistEditor)
	{
		if (myPlaylistEditor->Content.isFiltered())
			Statusbar::print(filteredMsg);
		else
		{
			auto playlist = currentStoredPlaylistPath();
			moveSelectedItemsDown(
				myPlaylistEditor->Content,
				[playlist](auto &, unsigned from, unsigned to) {
					Mpd.PlaylistMove(playlist, from, to);
				});
		}
	}
}

bool MoveSelectedItemsTo::canBeRun()
{
	return screenLegacyCurrent() == myPlaylist
	    || screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Content);
}

void MoveSelectedItemsTo::run()
{
	if (screenLegacyCurrent() == myPlaylist)
	{
		if (!myPlaylist->main().empty())
			moveSelectedItemsTo(myPlaylist->main(), [](auto &, unsigned from, unsigned to) { Mpd.Move(from, to); });
	}
	else
	{
		assert(!myPlaylistEditor->Playlists.empty());
		std::string playlist = currentStoredPlaylistPath();
		auto move_fun = [playlist](auto &, unsigned from, unsigned to) {
			Mpd.PlaylistMove(playlist, from, to);
		};
		moveSelectedItemsTo(myPlaylistEditor->Content, move_fun);
	}
}

bool Add::canBeRun()
{
	return screenLegacyCurrent() != myPlaylistEditor
	   || !myPlaylistEditor->Playlists.empty();
}

void Add::run()
{

	std::string path;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << (screenLegacyCurrent() == myPlaylistEditor ? "Add to playlist: " : "Add: ");
		path = static_cast<NC::Window *>(ui_state_footer_window())->prompt();
	}

	// confirm when one wants to add the whole database
	if (path.empty())
		if (!confirmAction("Are you sure you want to add the whole database?"))
			return;

	Statusbar::put() << "Adding...";
	static_cast<NC::Window *>(ui_state_footer_window())->refresh();
	if (screenLegacyCurrent() == myPlaylistEditor)
		Mpd.AddToPlaylist(currentStoredPlaylistPath(), path);
	else
	{
		try
		{
			Mpd.Add(path);
		}
		catch (MPD::ServerError &err)
		{
			// If a path is not a file or directory, assume it is a playlist.
			if (err.code() == MPD_SERVER_ERROR_NO_EXIST)
				Mpd.LoadPlaylist(path);
			else
			{
				Statusbar::printf("Error while adding item: %1%", err.what());
				return;
			}
		}
	}
}

bool Load::canBeRun()
{
	return screenLegacyCurrent() != myPlaylistEditor;
}

void Load::run()
{

	std::string path;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Load playlist: ";
		path = static_cast<NC::Window *>(ui_state_footer_window())->prompt();
	}

	Statusbar::put() << "Loading...";
	static_cast<NC::Window *>(ui_state_footer_window())->refresh();
	Mpd.LoadPlaylist(path);
}

bool SeekForward::canBeRun()
{
	return Status::State::player() != MPD::psStop && Status::State::totalTime() > 0;
}

void SeekForward::run()
{
	seek(NCM_SEARCH_DIRECTION_FORWARD);
}

bool SeekBackward::canBeRun()
{
	return Status::State::player() != MPD::psStop && Status::State::totalTime() > 0;
}

void SeekBackward::run()
{
	seek(NCM_SEARCH_DIRECTION_BACKWARD);
}

bool ToggleDisplayMode::canBeRun()
{
	return screenLegacyCurrent() == myPlaylist
	    || screenLegacyCurrent() == myBrowser
	    || screenLegacyCurrent() == mySearcher
	    || screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Content);
}

void ToggleDisplayMode::run()
{
	if (screenLegacyCurrent() == myPlaylist)
	{
		switch (Config.playlist_display_mode)
		{
			case NCM_DISPLAY_MODE_CLASSIC:
				Config.playlist_display_mode = NCM_DISPLAY_MODE_COLUMNS;
				myPlaylist->main().setItemDisplayer(std::bind(
					Display::SongsInColumns, ph::_1, std::cref(myPlaylist->main())
				));
				if (Config.titles_visibility)
					myPlaylist->main().setTitle(Display::Columns(myPlaylist->main().getWidth()));
				else
					myPlaylist->main().setTitle("");
				break;
			case NCM_DISPLAY_MODE_COLUMNS:
				Config.playlist_display_mode = NCM_DISPLAY_MODE_CLASSIC;
				myPlaylist->main().setItemDisplayer(std::bind(
					Display::Songs, ph::_1, std::cref(myPlaylist->main()), std::cref(Config.song_list_format)
				));
				myPlaylist->main().setTitle("");
		}
		Statusbar::printf("Playlist display mode: %1%", Config.playlist_display_mode);
	}
	else if (screenLegacyCurrent() == myBrowser)
	{
		switch (Config.browser_display_mode)
		{
			case NCM_DISPLAY_MODE_CLASSIC:
				Config.browser_display_mode = NCM_DISPLAY_MODE_COLUMNS;
				if (Config.titles_visibility)
					myBrowser->main().setTitle(Display::Columns(myBrowser->main().getWidth()));
				else
					myBrowser->main().setTitle("");
				break;
			case NCM_DISPLAY_MODE_COLUMNS:
				Config.browser_display_mode = NCM_DISPLAY_MODE_CLASSIC;
				myBrowser->main().setTitle("");
				break;
		}
		Statusbar::printf("Browser display mode: %1%", Config.browser_display_mode);
	}
	else if (screenLegacyCurrent() == mySearcher)
	{
		switch (Config.search_engine_display_mode)
		{
			case NCM_DISPLAY_MODE_CLASSIC:
				Config.search_engine_display_mode = NCM_DISPLAY_MODE_COLUMNS;
				break;
			case NCM_DISPLAY_MODE_COLUMNS:
				Config.search_engine_display_mode = NCM_DISPLAY_MODE_CLASSIC;
				break;
		}
		Statusbar::printf("Search engine display mode: %1%", Config.search_engine_display_mode);
		if (mySearcher->main().size() > SearchEngine::StaticOptions)
			mySearcher->main().setTitle(
				   Config.search_engine_display_mode == NCM_DISPLAY_MODE_COLUMNS
				&& Config.titles_visibility
				? Display::Columns(mySearcher->main().getWidth())
				: ""
			);
	}
	else if (screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Content))
	{
		switch (Config.playlist_editor_display_mode)
		{
			case NCM_DISPLAY_MODE_CLASSIC:
				Config.playlist_editor_display_mode = NCM_DISPLAY_MODE_COLUMNS;
				myPlaylistEditor->Content.setItemDisplayer(std::bind(
					Display::SongsInColumns, ph::_1, std::cref(myPlaylistEditor->Content)
				));
				break;
			case NCM_DISPLAY_MODE_COLUMNS:
				Config.playlist_editor_display_mode = NCM_DISPLAY_MODE_CLASSIC;
				myPlaylistEditor->Content.setItemDisplayer(std::bind(
					Display::Songs, ph::_1, std::cref(myPlaylistEditor->Content), std::cref(Config.song_list_format)
				));
				break;
		}
		Statusbar::printf("Playlist editor display mode: %1%", Config.playlist_editor_display_mode);
	}
}

bool ToggleSeparatorsBetweenAlbums::canBeRun()
{
	return true;
}

void ToggleSeparatorsBetweenAlbums::run()
{
	Config.playlist_separate_albums = !Config.playlist_separate_albums;
	Statusbar::printf("Separators between albums: %1%",
		Config.playlist_separate_albums ? "on" : "off"
	);
}

bool ToggleLyricsUpdateOnSongChange::canBeRun()
{
	return screenLegacyCurrent() == myLyrics;
}

void ToggleLyricsUpdateOnSongChange::run()
{
	Config.now_playing_lyrics = !Config.now_playing_lyrics;
	Statusbar::printf("Update lyrics if song changes: %1%",
		Config.now_playing_lyrics ? "on" : "off"
	);
}

void ToggleLyricsFetcher::run()
{
	myLyrics->toggleFetcher();
}

void ToggleFetchingLyricsInBackground::run()
{
	Config.fetch_lyrics_in_background = !Config.fetch_lyrics_in_background;
	Statusbar::printf("Fetching lyrics for playing songs in background: %1%",
	                  Config.fetch_lyrics_in_background ? "on" : "off");
}

void TogglePlayingSongCentering::run()
{
	Config.autocenter_mode = !Config.autocenter_mode;
	Statusbar::printf("Centering playing song: %1%",
		Config.autocenter_mode ? "on" : "off"
	);
	if (Config.autocenter_mode)
	{
		auto s = myPlaylist->nowPlayingSong();
		if (!s.empty())
			myPlaylist->locateSong(s);
	}
}

void UpdateDatabase::run()
{
	if (screenLegacyCurrent() == myBrowser)
		Mpd.UpdateDirectory(myBrowser->currentDirectory());
#	ifdef HAVE_TAGLIB_H
	else if (screenLegacyCurrent() == myTagEditor)
		Mpd.UpdateDirectory(myTagEditor->CurrentDir());
#	endif // HAVE_TAGLIB_H
	else
		Mpd.UpdateDirectory("/");
}

bool JumpToPlayingSong::canBeRun()
{
	m_song = myPlaylist->nowPlayingSong();
	return !m_song.empty()
		&& (screenLegacyCurrent() == myPlaylist
		    || screenLegacyCurrent() == myPlaylistEditor
		    || screenLegacyCurrent() == myBrowser
		    || screenLegacyCurrent() == myLibrary);
}

void JumpToPlayingSong::run()
{
	if (screenLegacyCurrent() == myPlaylist)
	{
		myPlaylist->locateSong(m_song);
	}
	else if (screenLegacyCurrent() == myPlaylistEditor)
	{
		myPlaylistEditor->locateSong(m_song);
	}
	else if (screenLegacyCurrent() == myBrowser)
	{
		myBrowser->locateSong(m_song);
	}
	else if (screenLegacyCurrent() == myLibrary)
	{
		myLibrary->locateSong(m_song);
	}
}

void ToggleRepeat::run()
{
	Mpd.SetRepeat(!Status::State::repeat());
}

bool Shuffle::canBeRun()
{
	if (screenLegacyCurrent() != myPlaylist)
		return false;
	m_begin = myPlaylist->main().begin();
	m_end = myPlaylist->main().end();
	return findSelectedRangeAndPrintInfoIfNot(m_begin, m_end);
}

void Shuffle::run()
{
	if (Config.ask_before_shuffling_playlists)
		if (!confirmAction("Do you really want to shuffle selected range?"))
			return;
	auto begin = myPlaylist->main().begin();
	Mpd.ShuffleRange(m_begin-begin, m_end-begin);
	Statusbar::print("Range shuffled");
}

void ToggleRandom::run()
{
	Mpd.SetRandom(!Status::State::random());
}

bool StartSearching::canBeRun()
{
	return screenLegacyCurrent() == mySearcher && !mySearcher->main()[0].isInactive();
}

void StartSearching::run()
{
	mySearcher->main().highlight(SearchEngine::SearchButton);
	mySearcher->main().setHighlighting(0);
	mySearcher->main().refresh();
	mySearcher->main().setHighlighting(1);
	mySearcher->runAction();
}

bool SaveTagChanges::canBeRun()
{
#	ifdef HAVE_TAGLIB_H
	return screenLegacyCurrent() == myTinyTagEditor
	    || screenLegacyCurrent()->activeWindow() == myTagEditor->TagTypes;
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void SaveTagChanges::run()
{
#	ifdef HAVE_TAGLIB_H
	if (screenLegacyCurrent() == myTinyTagEditor)
	{
		myTinyTagEditor->main().highlight(myTinyTagEditor->main().size()-2); // Save
		myTinyTagEditor->runAction();
	}
	else if (screenLegacyCurrent()->activeWindow() == myTagEditor->TagTypes)
	{
		myTagEditor->TagTypes->highlight(myTagEditor->TagTypes->size()-1); // Save
		myTagEditor->runAction();
	}
#	endif // HAVE_TAGLIB_H
}

void ToggleSingle::run()
{
	Mpd.SetSingle(!Status::State::single());
}

void ToggleConsume::run()
{
	Mpd.SetConsume(!Status::State::consume());
}

void ToggleCrossfade::run()
{
	Mpd.SetCrossfade(Status::State::crossfade() ? 0 : Config.crossfade_time);
}

void SetCrossfade::run()
{

	Statusbar::ScopedLock slock;
	Statusbar::put() << "Set crossfade to: ";
	auto crossfade = fromString<unsigned>(static_cast<NC::Window *>(ui_state_footer_window())->prompt());
	lowerBoundCheck(crossfade, 0u);
	Config.crossfade_time = crossfade;
	Mpd.SetCrossfade(crossfade);
}

bool SetVolume::canBeRun()
{
	return Status::State::volume() >= 0;
}

void SetVolume::run()
{

	unsigned volume;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Set volume to: ";
		volume = fromString<unsigned>(static_cast<NC::Window *>(ui_state_footer_window())->prompt());
		boundsCheck(volume, 0u, 100u);
		Mpd.SetVolume(volume);
	}
	Statusbar::printf("Volume set to %1%%%", volume);
}

bool EnterDirectory::canBeRun()
{
	bool result = false;
	if (screenLegacyCurrent() == myBrowser && !myBrowser->main().empty())
	{
		result = myBrowser->main().current()->value().type()
			== MPD::Item::Type::Directory;
	}
#ifdef HAVE_TAGLIB_H
	else if (screenLegacyCurrent()->activeWindow() == myTagEditor->Dirs)
		result = true;
#endif // HAVE_TAGLIB_H
	return result;
}

void EnterDirectory::run()
{
	if (screenLegacyCurrent() == myBrowser)
		myBrowser->enterDirectory();
#ifdef HAVE_TAGLIB_H
	else if (screenLegacyCurrent()->activeWindow() == myTagEditor->Dirs)
	{
		if (!myTagEditor->enterDirectory())
			Statusbar::print("No subdirectories found");
	}
#endif // HAVE_TAGLIB_H
}

bool EditSong::canBeRun()
{
#	ifdef HAVE_TAGLIB_H
	m_song = currentSong(screenLegacyCurrent());
	return m_song != nullptr && isMPDMusicDirSet();
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void EditSong::run()
{
#	ifdef HAVE_TAGLIB_H
	myTinyTagEditor->SetEdited(*m_song);
	myTinyTagEditor->switchTo();
#	endif // HAVE_TAGLIB_H
}

bool EditLibraryTag::canBeRun()
{
#	ifdef HAVE_TAGLIB_H
	return screenLegacyCurrent()->isActiveWindow(myLibrary->Tags)
	    && !myLibrary->Tags.empty()
	    && isMPDMusicDirSet();
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void EditLibraryTag::run()
{
#	ifdef HAVE_TAGLIB_H

	std::string new_tag;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << NC_FORMAT_BOLD << ncm_tag_type_name(Config.media_lib_primary_tag) << NC_FORMAT_NO_BOLD << ": ";
		new_tag = static_cast<NC::Window *>(ui_state_footer_window())->prompt(myLibrary->Tags.current()->value().tag());
	}
	if (!new_tag.empty() && new_tag != myLibrary->Tags.current()->value().tag())
	{
		Statusbar::print("Updating tags...");
		Mpd.StartSearch(true);
		Mpd.AddSearch(Config.media_lib_primary_tag, myLibrary->Tags.current()->value().tag());
		enum NcmTagsField field = ncm_tags_field_from_tag_type(Config.media_lib_primary_tag);
		assert(field != NCM_TAGS_FIELD_LAST);
		bool success = true;
		std::string dir_to_update;
		for (MPD::SongIterator s = Mpd.CommitSearchSongs(), end; s != end; ++s)
		{
			MPD::MutableSong ms = std::move(*s);
			ms.setTags(field, new_tag);
			Statusbar::printf("Updating tags in \"%1%\"...", ms.getName());
			std::string path = Config.mpd_music_dir + ms.getURI();
			if (!ncm_tags_write_mutable_song(ms))
			{
				success = false;
				Statusbar::printf("Error while writing tags to \"%1%\": %2%",
				                  ms.getName(), strerror(errno));
				s.finish();
				break;
			}
			if (dir_to_update.empty())
				dir_to_update = ms.getURI();
			else
				dir_to_update = getSharedDirectory(dir_to_update, ms.getURI());
		};
		if (success)
		{
			Mpd.UpdateDirectory(dir_to_update);
			Statusbar::print("Tags updated successfully");
		}
	}
#	endif // HAVE_TAGLIB_H
}

bool EditLibraryAlbum::canBeRun()
{
#	ifdef HAVE_TAGLIB_H
	return screenLegacyCurrent()->isActiveWindow(myLibrary->Albums)
	    && !myLibrary->Albums.empty()
		&& isMPDMusicDirSet();
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void EditLibraryAlbum::run()
{
#	ifdef HAVE_TAGLIB_H
		// FIXME: merge this and EditLibraryTag. also, prompt on failure if user wants to continue
	std::string new_album;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << NC_FORMAT_BOLD << "Album: " << NC_FORMAT_NO_BOLD;
		new_album = static_cast<NC::Window *>(ui_state_footer_window())->prompt(myLibrary->Albums.current()->value().entry().album());
	}
	if (!new_album.empty() && new_album != myLibrary->Albums.current()->value().entry().album())
	{
		bool success = 1;
		Statusbar::print("Updating tags...");
		for (size_t i = 0;  i < myLibrary->Songs.size(); ++i)
		{
			Statusbar::printf("Updating tags in \"%1%\"...", myLibrary->Songs[i].value().getName());
			std::string path = Config.mpd_music_dir + myLibrary->Songs[i].value().getURI();
			NcmTaglibFile file;
			ncm_taglib_file_init(&file);
			if (!ncm_taglib_file_open(&file, const_cast<char *>(path.c_str())))
			{
				const char msg[] = "Error while opening file \"%1%\"";
				Statusbar::printf(msg, Utf8::shorten(myLibrary->Songs[i].value().getURI(), COLS-const_strlen(msg)));
				success = 0;
				break;
			}
			ncm_taglib_clear_property(&file, const_cast<char *>("ALBUM"));
			ncm_taglib_append_property(&file, const_cast<char *>("ALBUM"),
			                           const_cast<char *>(new_album.c_str()));
			if (!ncm_taglib_file_save(&file))
			{
				const char msg[] = "Error while writing tags in \"%1%\"";
				Statusbar::printf(msg, Utf8::shorten(myLibrary->Songs[i].value().getURI(), COLS-const_strlen(msg)));
				ncm_taglib_file_close(&file);
				success = 0;
				break;
			}
			ncm_taglib_file_close(&file);
		}
		if (success)
		{
			Mpd.UpdateDirectory(getSharedDirectory(myLibrary->Songs.beginV(), myLibrary->Songs.endV()));
			Statusbar::print("Tags updated successfully");
		}
	}
#	endif // HAVE_TAGLIB_H
}

bool EditDirectoryName::canBeRun()
{
	return  ((screenLegacyCurrent() == myBrowser
	      && !myBrowser->main().empty()
	      && myBrowser->main().current()->value().type() == MPD::Item::Type::Directory)
#	ifdef HAVE_TAGLIB_H
	    ||   (screenLegacyCurrent()->activeWindow() == myTagEditor->Dirs
	      && !myTagEditor->Dirs->empty()
	      && myTagEditor->Dirs->choice() > 0)
#	endif // HAVE_TAGLIB_H
	) &&     isMPDMusicDirSet();
}

void EditDirectoryName::run()
{
		if (screenLegacyCurrent() == myBrowser)
	{
		std::string old_dir = myBrowser->main().current()->value().directory().path();
		std::string new_dir;
		{
			Statusbar::ScopedLock slock;
			Statusbar::put() << NC_FORMAT_BOLD << "Directory: " << NC_FORMAT_NO_BOLD;
			new_dir = static_cast<NC::Window *>(ui_state_footer_window())->prompt(old_dir);
		}
		if (!new_dir.empty() && new_dir != old_dir)
		{
			std::string full_old_dir;
			if (!myBrowser->isLocal())
				full_old_dir += Config.mpd_music_dir;
			full_old_dir += old_dir;
			std::string full_new_dir;
			if (!myBrowser->isLocal())
				full_new_dir += Config.mpd_music_dir;
			full_new_dir += new_dir;
			std::filesystem::rename(full_old_dir, full_new_dir);
			const char msg[] = "Directory renamed to \"%1%\"";
			Statusbar::printf(msg, Utf8::shorten(new_dir, COLS-const_strlen(msg)));
			if (!myBrowser->isLocal())
				Mpd.UpdateDirectory(getSharedDirectory(old_dir, new_dir));
			myBrowser->requestUpdate();
		}
	}
#	ifdef HAVE_TAGLIB_H
	else if (screenLegacyCurrent()->activeWindow() == myTagEditor->Dirs)
	{
		std::string old_dir = myTagEditor->Dirs->current()->value().first, new_dir;
		{
			Statusbar::ScopedLock slock;
			Statusbar::put() << NC_FORMAT_BOLD << "Directory: " << NC_FORMAT_NO_BOLD;
			new_dir = static_cast<NC::Window *>(ui_state_footer_window())->prompt(old_dir);
		}
		if (!new_dir.empty() && new_dir != old_dir)
		{
			std::string full_old_dir = Config.mpd_music_dir + myTagEditor->CurrentDir() + "/" + old_dir;
			std::string full_new_dir = Config.mpd_music_dir + myTagEditor->CurrentDir() + "/" + new_dir;
			if (rename(full_old_dir.c_str(), full_new_dir.c_str()) == 0)
			{
				const char msg[] = "Directory renamed to \"%1%\"";
				Statusbar::printf(msg, Utf8::shorten(new_dir, COLS-const_strlen(msg)));
				Mpd.UpdateDirectory(myTagEditor->CurrentDir());
			}
			else
			{
				const char msg[] = "Couldn't rename \"%1%\": %2%";
				Statusbar::printf(msg, Utf8::shorten(old_dir, COLS-const_strlen(msg)-25), strerror(errno));
			}
		}
	}
#	endif // HAVE_TAGLIB_H
}

bool EditPlaylistName::canBeRun()
{
	return   (screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Playlists)
	      && !myPlaylistEditor->Playlists.empty())
	    ||   (screenLegacyCurrent() == myBrowser
	      && !myBrowser->main().empty()
		  && myBrowser->main().current()->value().type() == MPD::Item::Type::Playlist);
}

void EditPlaylistName::run()
{
		std::string old_name, new_name;
	if (screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Playlists))
		old_name = currentStoredPlaylistPath();
	else
		old_name = myBrowser->main().current()->value().playlist().path();
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << NC_FORMAT_BOLD << "Playlist: " << NC_FORMAT_NO_BOLD;
		new_name = static_cast<NC::Window *>(ui_state_footer_window())->prompt(old_name);
	}
	if (!new_name.empty() && new_name != old_name)
	{
		Mpd.Rename(old_name, new_name);
		const char msg[] = "Playlist renamed to \"%1%\"";
		Statusbar::printf(msg, Utf8::shorten(new_name, COLS-const_strlen(msg)));
	}
}

bool EditLyrics::canBeRun()
{
	return screenLegacyCurrent() == myLyrics;
}

void EditLyrics::run()
{
	myLyrics->edit();
}

bool JumpToBrowser::canBeRun()
{
	m_song = currentSong(screenLegacyCurrent());
	return m_song != nullptr;
}

void JumpToBrowser::run()
{
	myBrowser->locateSong(*m_song);
}

bool JumpToMediaLibrary::canBeRun()
{
	m_song = currentSong(screenLegacyCurrent());
	return m_song != nullptr;
}

void JumpToMediaLibrary::run()
{
	myLibrary->locateSong(*m_song);
}

bool JumpToPlaylistEditor::canBeRun()
{
	return screenLegacyCurrent() == myBrowser
	    && myBrowser->main().current()->value().type() == MPD::Item::Type::Playlist;
}

void JumpToPlaylistEditor::run()
{
	myPlaylistEditor->locatePlaylist(
		myBrowser->main().current()->value().playlist());
}

void ToggleScreenLock::run()
{
		const char *msg_unlockable_screen = "Current screen can't be locked";
	if (screenLegacyLocked() != nullptr)
	{
		BaseScreen::unlock();
		Actions::setResizeFlags();
		app_controller_resize_current_screen();
		Statusbar::print("Screen unlocked");
	}
	else if (!screenLegacyCurrent()->isLockable())
	{
		Statusbar::print(msg_unlockable_screen);
	}
	else
	{
		unsigned part = Config.locked_screen_width_part*100;
		if (Config.ask_for_locked_screen_width_part)
		{
			Statusbar::ScopedLock slock;
			Statusbar::put() << "% of the locked screen's width to be reserved (20-80): ";
			part = fromString<unsigned>(static_cast<NC::Window *>(ui_state_footer_window())->prompt(std::to_string(part)));
		}
		boundsCheck(part, 20u, 80u);
		Config.locked_screen_width_part = part/100.0;
		if (screenLegacyCurrent()->lock())
			Statusbar::printf("Screen locked (with %1%%% width)", part);
		else
			Statusbar::print(msg_unlockable_screen);
	}
}

bool JumpToTagEditor::canBeRun()
{
#	ifdef HAVE_TAGLIB_H
	m_song = currentSong(screenLegacyCurrent());
	return m_song != nullptr && isMPDMusicDirSet();
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void JumpToTagEditor::run()
{
#	ifdef HAVE_TAGLIB_H
	myTagEditor->LocateSong(*m_song);
#	endif // HAVE_TAGLIB_H
}

bool JumpToPositionInSong::canBeRun()
{
	return Status::State::player() != MPD::psStop && Status::State::totalTime() > 0;
}

void JumpToPositionInSong::run()
{

	const MPD::Song s = myPlaylist->nowPlayingSong();

	std::string spos;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Position to go (in %/h:m:ss/m:ss/seconds(s)): ";
		spos = static_cast<NC::Window *>(ui_state_footer_window())->prompt();
	}

	std::regex rx;
	std::smatch what;
	// mm:ss
	if (std::regex_match(spos, what, rx.assign("([0-9]+):([0-9]{2})")))
	{
		auto mins = fromString<unsigned>(what[1]);
		auto secs = fromString<unsigned>(what[2]);
		boundsCheck(secs, 0u, 60u);
		Mpd.Seek(s.getPosition(), mins * 60 + secs);
	}
	// position in seconds
	else if (std::regex_match(spos, what, rx.assign("([0-9]+)s")))
	{
		auto secs = fromString<unsigned>(what[1]);
		Mpd.Seek(s.getPosition(), secs);
	}
	// position in%
	else if (std::regex_match(spos, what, rx.assign("([0-9]+)[%]{0,1}")))
	{
		auto percent = fromString<unsigned>(what[1]);
		boundsCheck(percent, 0u, 100u);
		int secs = (percent * s.getDuration()) / 100.0;
		Mpd.Seek(s.getPosition(), secs);
	}
	// position in hh:mm:ss
	else if (std::regex_match(spos, what, rx.assign("([0-9]+):([0-9]{2}):([0-9]{2})")))
	{
		auto hours = fromString<unsigned>(what[1]);
		auto mins  = fromString<unsigned>(what[2]);
		auto secs  = fromString<unsigned>(what[3]);
		boundsCheck(mins, 0u, 60u);
		boundsCheck(secs, 0u, 60u);
		Mpd.Seek(s.getPosition(), hours * 3600 + mins * 60 + secs);
	}
	else
		Statusbar::print("Invalid format ([h]:[mm]:[ss], [m]:[ss], [s]s, [%]%, [%] accepted)");
}

bool SelectItem::canBeRun()
{
	m_list = dynamic_cast<NC::List *>(screenLegacyCurrent()->activeWindow());
	return m_list != nullptr
	    && !m_list->empty()
	    && m_list->currentP()->isSelectable();
}

void SelectItem::run()
{
	auto current = m_list->currentP();
	current->setSelected(!current->isSelected());
}

bool SelectRange::canBeRun()
{
	m_list = dynamic_cast<NC::List *>(screenLegacyCurrent()->activeWindow());
	if (m_list == nullptr)
		return false;
	m_begin = m_list->beginP();
	m_end = m_list->endP();
	return findRange(m_begin, m_end);
}

void SelectRange::run()
{
	for (; m_begin != m_end; ++m_begin)
		m_begin->setSelected(true);
	Statusbar::print("Range selected");
}

bool ReverseSelection::canBeRun()
{
	m_list = dynamic_cast<NC::List *>(screenLegacyCurrent()->activeWindow());
	return m_list != nullptr;
}

void ReverseSelection::run()
{
	for (auto &p : *m_list)
		p.setSelected(!p.isSelected());
	Statusbar::print("Selection reversed");
}

bool RemoveSelection::canBeRun()
{
	m_list = dynamic_cast<NC::List *>(screenLegacyCurrent()->activeWindow());
	return m_list != nullptr;
}

void RemoveSelection::run()
{
	for (auto &p : *m_list)
		p.setSelected(false);
	Statusbar::print("Selection removed");
}

bool SelectAlbum::canBeRun()
{
	auto *w = screenLegacyCurrent()->activeWindow();
	if (m_list != static_cast<void *>(w))
		m_list = dynamic_cast<NC::List *>(w);
	if (m_songs != static_cast<void *>(w))
		m_songs = dynamic_cast<SongList *>(w);
	return m_list != nullptr && !m_list->empty()
	    && m_songs != nullptr;
}

void SelectAlbum::run()
{
	const auto front = m_songs->beginS(), current = m_songs->currentS(), end = m_songs->endS();
	if (current->song() == nullptr)
		return;
	enum NcmSongGetter getter = NCM_SONG_GETTER_ALBUM;
	const std::string tag = current->song()->getTags(getter);
	// go up
	for (auto it = current; it != front;)
	{
		--it;
		if (it->song() == nullptr || it->song()->getTags(getter) != tag)
			break;
		it->properties().setSelected(true);
	}
	// go down
	for (auto it = current;;)
	{
		it->properties().setSelected(true);
		if (++it == end)
			break;
		if (it->song() == nullptr || it->song()->getTags(getter) != tag)
			break;
	}
	Statusbar::print("Album around cursor position selected");
}

bool SelectFoundItems::canBeRun()
{
	m_list = dynamic_cast<NC::List *>(screenLegacyCurrent()->activeWindow());
	if (m_list == nullptr || m_list->empty())
		return false;
	m_searchable = dynamic_cast<Searchable *>(screenLegacyCurrent());
	return m_searchable != nullptr && m_searchable->allowsSearching();
}

void SelectFoundItems::run()
{
	auto current_pos = m_list->choice();
	screenLegacyCurrent()->activeWindow()->scroll(NC_SCROLL_HOME);
	bool found = m_searchable->search(NCM_SEARCH_DIRECTION_FORWARD, false, false);
	if (found)
	{
		Statusbar::print("Searching for items...");
		m_list->currentP()->setSelected(true);
		while (m_searchable->search(NCM_SEARCH_DIRECTION_FORWARD, false, true))
			m_list->currentP()->setSelected(true);
		Statusbar::print("Found items selected");
	}
	m_list->highlight(current_pos);
}

bool AddSelectedItems::canBeRun()
{
	return screenLegacyCurrent() != mySelectedItemsAdder;
}

void AddSelectedItems::run()
{
	mySelectedItemsAdder->switchTo();
}

void CropMainPlaylist::run()
{
	auto &w = myPlaylist->main();
	// cropping doesn't make sense in this case
	if (w.size() <= 1)
		return;
	if (Config.ask_before_clearing_playlists)
		if (!confirmAction("Do you really want to crop main playlist?"))
			return;
	Statusbar::print("Cropping playlist...");
	selectCurrentIfNoneSelected(w);
	reverseSelectionHelper(w.begin(), w.end());
	deleteSelectedSongsFromPlaylist(w);
	Statusbar::print("Playlist cropped");
}

bool CropPlaylist::canBeRun()
{
	return screenLegacyCurrent() == myPlaylistEditor;
}

void CropPlaylist::run()
{
	auto &w = myPlaylistEditor->Content;
	// cropping doesn't make sense in this case
	if (w.size() <= 1)
		return;
	assert(!myPlaylistEditor->Playlists.empty());
	std::string playlist = currentStoredPlaylistPath();
	if (Config.ask_before_clearing_playlists
	    && !confirmAction(stringFormat("Do you really want to crop playlist \"%1%\"?",
	                                  playlist)))
		return;
	selectCurrentIfNoneSelected(w);
	Statusbar::printf("Cropping playlist \"%1%\"...", playlist);
	cropPlaylist(w, [playlist](auto &, unsigned pos) { Mpd.PlaylistDelete(playlist, pos); });
	Statusbar::printf("Playlist \"%1%\" cropped", playlist);
}

void ClearMainPlaylist::run()
{
	if (!myPlaylist->main().empty() && Config.ask_before_clearing_playlists)
		if (!confirmAction("Do you really want to clear main playlist?"))
			return;
	Mpd.ClearMainPlaylist();
	Statusbar::print("Playlist cleared");
	myPlaylist->main().reset();
}

bool ClearPlaylist::canBeRun()
{
	return screenLegacyCurrent() == myPlaylistEditor;
}

void ClearPlaylist::run()
{
	if (myPlaylistEditor->Playlists.empty())
		return;
	std::string playlist = currentStoredPlaylistPath();
	if (Config.ask_before_clearing_playlists
	    && !confirmAction(stringFormat("Do you really want to clear playlist \"%1%\"?",
	                                  playlist)))
		return;
	Mpd.ClearPlaylist(playlist);
	Statusbar::printf("Playlist \"%1%\" cleared", playlist);
}

bool SortPlaylist::canBeRun()
{
	if (screenLegacyCurrent() != myPlaylist)
		return false;
	auto first = myPlaylist->main().begin(), last = myPlaylist->main().end();
	return findSelectedRangeAndPrintInfoIfNot(first, last);
}

void SortPlaylist::run()
{
	mySortPlaylistDialog->switchTo();
}

bool ReversePlaylist::canBeRun()
{
	if (screenLegacyCurrent() != myPlaylist)
		return false;
	m_begin = myPlaylist->main().begin();
	m_end = myPlaylist->main().end();
	if (m_begin == m_end)
		return false;
	else
		return findSelectedRangeAndPrintInfoIfNot(m_begin, m_end);
}

void ReversePlaylist::run()
{
	Statusbar::print("Reversing range...");
	Mpd.StartCommandsList();
	for (--m_end; m_begin < m_end; ++m_begin, --m_end)
		Mpd.Swap(m_begin->value().getPosition(), m_end->value().getPosition());
	Mpd.CommitCommandsList();
	Statusbar::print("Range reversed");
}

bool ApplyFilter::canBeRun()
{
	m_filterable = dynamic_cast<Filterable *>(screenLegacyCurrent());
	return m_filterable != nullptr
		&& m_filterable->allowsFiltering();
}

void ApplyFilter::run()
{

	std::string filter = m_filterable->currentFilter();
	if (!filter.empty())
	{
		m_filterable->applyFilter(filter);
		app_controller_refresh_current_window();
	}

	try
	{
		ScopedValue<bool> disabled_autocenter_mode(Config.autocenter_mode, false);
		Statusbar::ScopedLock slock;
		NC::Window::ScopedPromptHook helper(
			*static_cast<NC::Window *>(ui_state_footer_window()),
			Statusbar::Helpers::ApplyFilterImmediately(m_filterable));
		Statusbar::put() << "Apply filter: ";
		filter = static_cast<NC::Window *>(ui_state_footer_window())->prompt(filter);
	}
	catch (NC::PromptAborted &)
	{
		m_filterable->applyFilter(filter);
		Statusbar::print("Action cancelled");
		return;
	}

	if (filter.empty())
		Statusbar::printf("Filtering disabled");
	else
		Statusbar::printf("Using filter \"%1%\"", filter);

	if (screenLegacyCurrent() == myPlaylist)
		myPlaylist->reloadTotalLength();

	listsChangeFinisher();
}

bool Find::canBeRun()
{
	return native_c_screen_help_is_current()
		|| screenLegacyCurrent() == myLyrics
		|| screenLegacyCurrent() == myLastfm;
}

void Find::run()
{

	std::string token;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Find: ";
		token = static_cast<NC::Window *>(ui_state_footer_window())->prompt();
	}

	Statusbar::print("Searching...");
	auto s = static_cast<Screen<NC::Scrollpad> *>(screenLegacyCurrent());
	s->main().removeProperties();
	if (token.empty() || s->main().setProperties(NC_FORMAT_REVERSE, token, NC_FORMAT_NO_REVERSE, Config.regex_type))
		Statusbar::print("Done");
	else
		Statusbar::print("No matching patterns found");
	s->main().flush();
}

bool FindItemBackward::canBeRun()
{
	auto w = dynamic_cast<Searchable *>(screenLegacyCurrent());
	return w && w->allowsSearching();
}

void FindItemForward::run()
{
	findItem(NCM_SEARCH_DIRECTION_FORWARD);
	listsChangeFinisher();
}

bool FindItemForward::canBeRun()
{
	auto w = dynamic_cast<Searchable *>(screenLegacyCurrent());
	return w && w->allowsSearching();
}

void FindItemBackward::run()
{
	findItem(NCM_SEARCH_DIRECTION_BACKWARD);
	listsChangeFinisher();
}

bool NextFoundItem::canBeRun()
{
	return dynamic_cast<Searchable *>(screenLegacyCurrent());
}

void NextFoundItem::run()
{
	Searchable *w = dynamic_cast<Searchable *>(screenLegacyCurrent());
	assert(w != nullptr);
	w->search(NCM_SEARCH_DIRECTION_FORWARD, Config.wrapped_search, true);
	listsChangeFinisher();
}

bool PreviousFoundItem::canBeRun()
{
	return dynamic_cast<Searchable *>(screenLegacyCurrent());
}

void PreviousFoundItem::run()
{
	Searchable *w = dynamic_cast<Searchable *>(screenLegacyCurrent());
	assert(w != nullptr);
	w->search(NCM_SEARCH_DIRECTION_BACKWARD, Config.wrapped_search, true);
	listsChangeFinisher();
}

void ToggleFindMode::run()
{
	Config.wrapped_search = !Config.wrapped_search;
	Statusbar::printf("Search mode: %1%",
		Config.wrapped_search ? "Wrapped" : "Normal"
	);
}

void ToggleReplayGainMode::run()
{

	char rgm = 0;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Replay gain mode? "
		<< "[" << NC_FORMAT_BOLD << 'o' << NC_FORMAT_NO_BOLD << "ff"
		<< "/" << NC_FORMAT_BOLD << 't' << NC_FORMAT_NO_BOLD << "rack"
		<< "/" << NC_FORMAT_BOLD << 'a' << NC_FORMAT_NO_BOLD << "lbum"
		<< "] ";
		rgm = Statusbar::Helpers::promptReturnOneOf({'t', 'a', 'o'});
	}
	switch (rgm)
	{
		case 't':
			Mpd.SetReplayGainMode(MPD::rgmTrack);
			break;
		case 'a':
			Mpd.SetReplayGainMode(MPD::rgmAlbum);
			break;
		case 'o':
			Mpd.SetReplayGainMode(MPD::rgmOff);
			break;
		default: // impossible
			Statusbar::printf("Internal error: invalid replay gain mode: %1%",
			                  rgm);
			return;
	}
	Statusbar::printf("Replay gain mode: %1%", Mpd.GetReplayGainMode());
}

void ToggleAddMode::run()
{
	std::string mode_desc;
	switch (Config.space_add_mode)
	{
		case NCM_SPACE_ADD_MODE_ADD_REMOVE:
			Config.space_add_mode = NCM_SPACE_ADD_MODE_ALWAYS_ADD;
			mode_desc = "always add an item to playlist";
			break;
		case NCM_SPACE_ADD_MODE_ALWAYS_ADD:
			Config.space_add_mode = NCM_SPACE_ADD_MODE_ADD_REMOVE;
			mode_desc = "add an item to playlist or remove if already added";
			break;
	}
	Statusbar::printf("Add mode: %1%", mode_desc);
}

void ToggleMouse::run()
{
	Config.mouse_support = !Config.mouse_support;
	if (Config.mouse_support)
		nc_mouse_enable();
	else
		nc_mouse_disable();
	Statusbar::printf("Mouse support %1%",
		Config.mouse_support ? "enabled" : "disabled"
	);
}

void ToggleBitrateVisibility::run()
{
	Config.display_bitrate = !Config.display_bitrate;
	Statusbar::printf("Bitrate visibility %1%",
		Config.display_bitrate ? "enabled" : "disabled"
	);
}

void AddRandomItems::run()
{
		char rnd_type = 0;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Add random? "
		<< "[" << NC_FORMAT_BOLD << 's' << NC_FORMAT_NO_BOLD << "ongs"
		<< "/" << NC_FORMAT_BOLD << 'a' << NC_FORMAT_NO_BOLD << "rtists"
		<< "/" << "album" << NC_FORMAT_BOLD << 'A' << NC_FORMAT_NO_BOLD << "rtists"
		<< "/" << "al" << NC_FORMAT_BOLD << 'b' << NC_FORMAT_NO_BOLD << "ums"
		<< "] ";
		rnd_type = Statusbar::Helpers::promptReturnOneOf({'s', 'a', 'A', 'b'});
	}

	mpd_tag_type tag_type = MPD_TAG_ARTIST;
	std::string tag_type_str ;
	if (rnd_type != 's')
	{
		tag_type = ncm_char_to_tag_type(rnd_type);
		tag_type_str = lowercaseAscii(ncm_tag_type_name(tag_type));
	}
	else
		tag_type_str = "song";

	unsigned number;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Number of random " << tag_type_str << "s: ";
		number = fromString<unsigned>(static_cast<NC::Window *>(ui_state_footer_window())->prompt());
	}
	if (number > 0)
	{
		bool success;
		if (rnd_type == 's')
			success = Mpd.AddRandomSongs(number, Config.random_exclude_pattern,
			                          &global_random);
		else
			success = Mpd.AddRandomTag(tag_type, number, &global_random);
		if (success)
			Statusbar::printf("%1% random %2%%3% added to playlist", number, tag_type_str, number == 1 ? "" : "s");
	}
}

bool ToggleBrowserSortMode::canBeRun()
{
	return screenLegacyCurrent() == myBrowser;
}

void ToggleBrowserSortMode::run()
{
	switch (Config.browser_sort_mode)
	{
		case NCM_SORT_MODE_TYPE:
			Config.browser_sort_mode = NCM_SORT_MODE_NAME;
			Statusbar::print("Sort songs by: name");
			break;
		case NCM_SORT_MODE_NAME:
			Config.browser_sort_mode = NCM_SORT_MODE_MODIFICATION_TIME;
			Statusbar::print("Sort songs by: modification time");
			break;
		case NCM_SORT_MODE_MODIFICATION_TIME:
			Config.browser_sort_mode = NCM_SORT_MODE_CUSTOM_FORMAT;
			Statusbar::print("Sort songs by: custom format");
			break;
		case NCM_SORT_MODE_CUSTOM_FORMAT:
			Config.browser_sort_mode = NCM_SORT_MODE_NONE;
			Statusbar::print("Do not sort songs");
			break;
		case NCM_SORT_MODE_NONE:
			Config.browser_sort_mode = NCM_SORT_MODE_TYPE;
			Statusbar::print("Sort songs by: type");
	}
	if (Config.browser_sort_mode != NCM_SORT_MODE_NONE)
	{
		size_t sort_offset = myBrowser->inRootDirectory() ? 0 : 1;
		std::stable_sort(
			myBrowser->main().begin()+sort_offset, myBrowser->main().end(),
			LocaleBasedItemSorting(std::locale(), Config.ignore_leading_the,
			                       Config.browser_sort_mode));
	}
}

bool ToggleLibraryTagType::canBeRun()
{
	return (screenLegacyCurrent()->isActiveWindow(myLibrary->Tags))
	    || (myLibrary->columns() == 2 && screenLegacyCurrent()->isActiveWindow(myLibrary->Albums));
}

void ToggleLibraryTagType::run()
{

	char tag_type = 0;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Tag type? "
		<< "[" << NC_FORMAT_BOLD << 'a' << NC_FORMAT_NO_BOLD << "rtist"
		<< "/" << "album" << NC_FORMAT_BOLD << 'A' << NC_FORMAT_NO_BOLD << "rtist"
		<< "/" << NC_FORMAT_BOLD << 'y' << NC_FORMAT_NO_BOLD << "ear"
		<< "/" << NC_FORMAT_BOLD << 'g' << NC_FORMAT_NO_BOLD << "enre"
		<< "/" << NC_FORMAT_BOLD << 'c' << NC_FORMAT_NO_BOLD << "omposer"
		<< "/" << NC_FORMAT_BOLD << 'p' << NC_FORMAT_NO_BOLD << "erformer"
		<< "] ";
		tag_type = Statusbar::Helpers::promptReturnOneOf({'a', 'A', 'y', 'g', 'c', 'p'});
	}
	mpd_tag_type new_tagitem = ncm_char_to_tag_type(tag_type);
	if (new_tagitem != Config.media_lib_primary_tag)
	{
		Config.media_lib_primary_tag = new_tagitem;
		std::string item_type = ncm_tag_type_name(Config.media_lib_primary_tag);
		myLibrary->Tags.setTitle(Config.titles_visibility ? item_type + "s" : "");
		myLibrary->Tags.reset();
		item_type = lowercaseAscii(item_type);
		std::string and_mtime = Config.media_library_sort_by_mtime ?
		                        " and mtime" :
		                        "";
		if (myLibrary->columns() == 2)
		{
			myLibrary->Songs.clear();
			myLibrary->Albums.reset();
			myLibrary->Albums.clear();
			myLibrary->Albums.setTitle(Config.titles_visibility ? "Albums (sorted by " + item_type + and_mtime + ")" : "");
			myLibrary->Albums.display();
		}
		else
		{
			myLibrary->Tags.clear();
			myLibrary->Tags.display();
		}
		Statusbar::printf("Switched to the list of %1%s", item_type);
	}
}

bool ToggleMediaLibrarySortMode::canBeRun()
{
	return screenLegacyCurrent() == myLibrary;
}

void ToggleMediaLibrarySortMode::run()
{
	myLibrary->toggleSortMode();
}

bool FetchLyricsInBackground::canBeRun()
{
	m_hs = dynamic_cast<HasSongs *>(screenLegacyCurrent());
	return m_hs != nullptr && m_hs->itemAvailable();
}

void FetchLyricsInBackground::run()
{
	auto songs = m_hs->getSelectedSongs();
	for (const auto &s : songs)
		myLyrics->fetchInBackground(s, true);
	Statusbar::print("Selected songs queued for lyrics fetching");
}

bool RefetchLyrics::canBeRun()
{
	return screenLegacyCurrent() == myLyrics;
}

void RefetchLyrics::run()
{
	myLyrics->refetchCurrent();
}

bool SetSelectedItemsPriority::canBeRun()
{
	if (Mpd.Version() < 17)
	{
		Statusbar::print("Priorities are supported in MPD >= 0.17.0");
		return false;
	}
	return screenLegacyCurrent() == myPlaylist && !myPlaylist->main().empty();
}

void SetSelectedItemsPriority::run()
{

	unsigned prio;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Set priority [0-255]: ";
		prio = fromString<unsigned>(static_cast<NC::Window *>(ui_state_footer_window())->prompt());
		boundsCheck(prio, 0u, 255u);
	}
	myPlaylist->setSelectedItemsPriority(prio);
}

bool ToggleOutput::canBeRun()
{
#ifdef ENABLE_OUTPUTS
	return native_c_screen_outputs_is_current();
#else
	return false;
#endif // ENABLE_OUTPUTS
}

void ToggleOutput::run()
{
#ifdef ENABLE_OUTPUTS
	native_c_screen_outputs_toggle();
#endif // ENABLE_OUTPUTS
}

bool ToggleVisualizationType::canBeRun()
{
#	ifdef ENABLE_VISUALIZER
	return screenLegacyCurrent() == myVisualizer;
#	else
	return false;
#	endif // ENABLE_VISUALIZER
}

void ToggleVisualizationType::run()
{
#	ifdef ENABLE_VISUALIZER
	myVisualizer->ToggleVisualizationType();
#	endif // ENABLE_VISUALIZER
}

void ShowSongInfo::run()
{
	native_c_screen_song_info_switch_to();
}

bool ShowArtistInfo::canBeRun()
{
	return screenLegacyCurrent() == myLastfm
		|| (screenLegacyCurrent()->isActiveWindow(myLibrary->Tags)
		    && !myLibrary->Tags.empty()
		    && Config.media_lib_primary_tag == MPD_TAG_ARTIST)
		|| currentSong(screenLegacyCurrent());
}

void ShowArtistInfo::run()
{
	if (screenLegacyCurrent() == myLastfm)
	{
		myLastfm->switchTo();
		return;
	}

	std::string artist;
	if (screenLegacyCurrent()->isActiveWindow(myLibrary->Tags))
	{
		assert(!myLibrary->Tags.empty());
		assert(Config.media_lib_primary_tag == MPD_TAG_ARTIST);
		artist = myLibrary->Tags.current()->value().tag();
	}
	else
	{
		auto s = currentSong(screenLegacyCurrent());
		assert(s);
		artist = s->getArtist();
	}

	if (!artist.empty())
	{
		myLastfm->queueArtistInfo(artist, Config.lastfm_preferred_language);
		if (!isVisible(myLastfm))
			myLastfm->switchTo();
	}
}

bool ShowLyrics::canBeRun()
{
	if (screenLegacyCurrent() == myLyrics)
	{
		m_song = nullptr;
		return true;
	}
	else
	{
		m_song = currentSong(screenLegacyCurrent());
		return m_song != nullptr;
	}
}

void ShowLyrics::run()
{
	if (m_song != nullptr)
		myLyrics->fetch(*m_song);
	if (screenLegacyCurrent() == myLyrics || !isVisible(myLyrics))
		myLyrics->switchTo();
}

void Quit::run()
{
	ExitMainLoop = true;
}

void NextScreen::run()
{
	if (Config.screen_switcher_previous)
	{
		BaseScreen *current = screenLegacyCurrent();

		if (auto tababble = dynamic_cast<Tabbable *>(current))
			tababble->switchToPreviousScreen();
		else if (current == nullptr)
			(void)app_controller_switch_to_screen(
				app_controller_previous_screen());
	}
	else if (!Config.screen_sequence.empty())
	{
		auto screen = nextScreenTypeInSequence(
			Config.screen_sequence.begin(),
			Config.screen_sequence.end(),
			native_c_screens_current_type());
		(void)native_c_screens_switch_to_type(*screen);
	}
}

void PreviousScreen::run()
{
	if (Config.screen_switcher_previous)
	{
		BaseScreen *current = screenLegacyCurrent();

		if (auto tababble = dynamic_cast<Tabbable *>(current))
			tababble->switchToPreviousScreen();
		else if (current == nullptr)
			(void)app_controller_switch_to_screen(
				app_controller_previous_screen());
	}
	else if (!Config.screen_sequence.empty())
	{
		auto screen = nextScreenTypeInSequence(
			Config.screen_sequence.rbegin(),
			Config.screen_sequence.rend(),
			native_c_screens_current_type());
		(void)native_c_screens_switch_to_type(*screen);
	}
}

bool ShowHelp::canBeRun()
{
	return !native_c_screen_help_is_current()
#	ifdef HAVE_TAGLIB_H
	    && screenLegacyCurrent() != myTinyTagEditor
#	endif // HAVE_TAGLIB_H
	;
}

void ShowHelp::run()
{
	native_c_screen_help_switch_to();
}

bool ShowPlaylist::canBeRun()
{
	return screenLegacyCurrent() != myPlaylist
#	ifdef HAVE_TAGLIB_H
	    && screenLegacyCurrent() != myTinyTagEditor
#	endif // HAVE_TAGLIB_H
	;
}

void ShowPlaylist::run()
{
	myPlaylist->switchTo();
}

bool ShowBrowser::canBeRun()
{
	return screenLegacyCurrent() != myBrowser
#	ifdef HAVE_TAGLIB_H
	    && screenLegacyCurrent() != myTinyTagEditor
#	endif // HAVE_TAGLIB_H
	;
}

void ShowBrowser::run()
{
	myBrowser->switchTo();
}

bool ChangeBrowseMode::canBeRun()
{
	return screenLegacyCurrent() == myBrowser;
}

void ChangeBrowseMode::run()
{
	myBrowser->changeBrowseMode();
}

bool ShowSearchEngine::canBeRun()
{
	return screenLegacyCurrent() != mySearcher
#	ifdef HAVE_TAGLIB_H
	    && screenLegacyCurrent() != myTinyTagEditor
#	endif // HAVE_TAGLIB_H
	;
}

void ShowSearchEngine::run()
{
	mySearcher->switchTo();
}

bool ResetSearchEngine::canBeRun()
{
	return screenLegacyCurrent() == mySearcher;
}

void ResetSearchEngine::run()
{
	mySearcher->reset();
}

bool ShowMediaLibrary::canBeRun()
{
	return screenLegacyCurrent() != myLibrary
#	ifdef HAVE_TAGLIB_H
	    && screenLegacyCurrent() != myTinyTagEditor
#	endif // HAVE_TAGLIB_H
	;
}

void ShowMediaLibrary::run()
{
	myLibrary->switchTo();
}

bool ToggleMediaLibraryColumnsMode::canBeRun()
{
	return screenLegacyCurrent() == myLibrary;
}

void ToggleMediaLibraryColumnsMode::run()
{
	myLibrary->toggleColumnsMode();
	myLibrary->refresh();
}

bool ShowPlaylistEditor::canBeRun()
{
	return screenLegacyCurrent() != myPlaylistEditor
#	ifdef HAVE_TAGLIB_H
	    && screenLegacyCurrent() != myTinyTagEditor
#	endif // HAVE_TAGLIB_H
	;
}

void ShowPlaylistEditor::run()
{
	myPlaylistEditor->switchTo();
}

bool ShowTagEditor::canBeRun()
{
#	ifdef HAVE_TAGLIB_H
	return screenLegacyCurrent() != myTagEditor
	    && screenLegacyCurrent() != myTinyTagEditor;
#	else
	return false;
#	endif // HAVE_TAGLIB_H
}

void ShowTagEditor::run()
{
#	ifdef HAVE_TAGLIB_H
	if (isMPDMusicDirSet())
		myTagEditor->switchTo();
#	endif // HAVE_TAGLIB_H
}

bool ShowOutputs::canBeRun()
{
#	ifdef ENABLE_OUTPUTS
	return !native_c_screen_outputs_is_current()
#	ifdef HAVE_TAGLIB_H
	    && screenLegacyCurrent() != myTinyTagEditor
#	endif // HAVE_TAGLIB_H
	;
#	else
	return false;
#	endif // ENABLE_OUTPUTS
}

void ShowOutputs::run()
{
#	ifdef ENABLE_OUTPUTS
	native_c_screen_outputs_switch_to();
#	endif // ENABLE_OUTPUTS
}

bool ShowVisualizer::canBeRun()
{
#	ifdef ENABLE_VISUALIZER
	return screenLegacyCurrent() != myVisualizer
#	ifdef HAVE_TAGLIB_H
	    && screenLegacyCurrent() != myTinyTagEditor
#	endif // HAVE_TAGLIB_H
	;
#	else
	return false;
#	endif // ENABLE_VISUALIZER
}

void ShowVisualizer::run()
{
#	ifdef ENABLE_VISUALIZER
	myVisualizer->switchTo();
#	endif // ENABLE_VISUALIZER
}


#ifdef HAVE_TAGLIB_H
bool ShowServerInfo::canBeRun()
{
	return screenLegacyCurrent() != myTinyTagEditor;
}
#endif // HAVE_TAGLIB_H

void ShowServerInfo::run()
{
	native_c_screen_server_info_switch_to();
}

}

namespace {

void populateActions()
{
	AvailableActions.resize(static_cast<size_t>(Actions::Type::_numberOfActions));
	auto insert_action = [](Actions::BaseAction *a) {
		AvailableActions.at(static_cast<size_t>(a->type())).reset(a);
	};
	insert_action(new Actions::Dummy());
	insert_action(new Actions::UpdateEnvironment());
	insert_action(new Actions::MouseEvent());
	insert_action(new Actions::ScrollUp());
	insert_action(new Actions::ScrollDown());
	insert_action(new Actions::ScrollUpArtist());
	insert_action(new Actions::ScrollUpAlbum());
	insert_action(new Actions::ScrollDownArtist());
	insert_action(new Actions::ScrollDownAlbum());
	insert_action(new Actions::PageUp());
	insert_action(new Actions::PageDown());
	insert_action(new Actions::MoveHome());
	insert_action(new Actions::MoveEnd());
	insert_action(new Actions::ToggleInterface());
	insert_action(new Actions::JumpToParentDirectory());
	insert_action(new Actions::RunAction());
	insert_action(new Actions::SelectItem());
	insert_action(new Actions::SelectRange());
	insert_action(new Actions::PreviousColumn());
	insert_action(new Actions::NextColumn());
	insert_action(new Actions::MasterScreen());
	insert_action(new Actions::SlaveScreen());
	insert_action(new Actions::VolumeUp());
	insert_action(new Actions::VolumeDown());
	insert_action(new Actions::AddItemToPlaylist());
	insert_action(new Actions::DeletePlaylistItems());
	insert_action(new Actions::DeleteStoredPlaylist());
	insert_action(new Actions::DeleteBrowserItems());
	insert_action(new Actions::ReplaySong());
	insert_action(new Actions::PreviousSong());
	insert_action(new Actions::NextSong());
	insert_action(new Actions::Pause());
	insert_action(new Actions::Stop());
	insert_action(new Actions::Play());
	insert_action(new Actions::ExecuteCommand());
	insert_action(new Actions::SavePlaylist());
	insert_action(new Actions::MoveSortOrderUp());
	insert_action(new Actions::MoveSortOrderDown());
	insert_action(new Actions::MoveSelectedItemsUp());
	insert_action(new Actions::MoveSelectedItemsDown());
	insert_action(new Actions::MoveSelectedItemsTo());
	insert_action(new Actions::Add());
	insert_action(new Actions::Load());
	insert_action(new Actions::PlayItem());
	insert_action(new Actions::SeekForward());
	insert_action(new Actions::SeekBackward());
	insert_action(new Actions::ToggleDisplayMode());
	insert_action(new Actions::ToggleSeparatorsBetweenAlbums());
	insert_action(new Actions::ToggleLyricsUpdateOnSongChange());
	insert_action(new Actions::ToggleLyricsFetcher());
	insert_action(new Actions::ToggleFetchingLyricsInBackground());
	insert_action(new Actions::TogglePlayingSongCentering());
	insert_action(new Actions::UpdateDatabase());
	insert_action(new Actions::JumpToPlayingSong());
	insert_action(new Actions::ToggleRepeat());
	insert_action(new Actions::Shuffle());
	insert_action(new Actions::ToggleRandom());
	insert_action(new Actions::StartSearching());
	insert_action(new Actions::SaveTagChanges());
	insert_action(new Actions::ToggleSingle());
	insert_action(new Actions::ToggleConsume());
	insert_action(new Actions::ToggleCrossfade());
	insert_action(new Actions::SetCrossfade());
	insert_action(new Actions::SetVolume());
	insert_action(new Actions::EnterDirectory());
	insert_action(new Actions::EditSong());
	insert_action(new Actions::EditLibraryTag());
	insert_action(new Actions::EditLibraryAlbum());
	insert_action(new Actions::EditDirectoryName());
	insert_action(new Actions::EditPlaylistName());
	insert_action(new Actions::EditLyrics());
	insert_action(new Actions::JumpToBrowser());
	insert_action(new Actions::JumpToMediaLibrary());
	insert_action(new Actions::JumpToPlaylistEditor());
	insert_action(new Actions::ToggleScreenLock());
	insert_action(new Actions::JumpToTagEditor());
	insert_action(new Actions::JumpToPositionInSong());
	insert_action(new Actions::ReverseSelection());
	insert_action(new Actions::RemoveSelection());
	insert_action(new Actions::SelectAlbum());
	insert_action(new Actions::SelectFoundItems());
	insert_action(new Actions::AddSelectedItems());
	insert_action(new Actions::CropMainPlaylist());
	insert_action(new Actions::CropPlaylist());
	insert_action(new Actions::ClearMainPlaylist());
	insert_action(new Actions::ClearPlaylist());
	insert_action(new Actions::SortPlaylist());
	insert_action(new Actions::ReversePlaylist());
	insert_action(new Actions::ApplyFilter());
	insert_action(new Actions::Find());
	insert_action(new Actions::FindItemForward());
	insert_action(new Actions::FindItemBackward());
	insert_action(new Actions::NextFoundItem());
	insert_action(new Actions::PreviousFoundItem());
	insert_action(new Actions::ToggleFindMode());
	insert_action(new Actions::ToggleReplayGainMode());
	insert_action(new Actions::ToggleAddMode());
	insert_action(new Actions::ToggleMouse());
	insert_action(new Actions::ToggleBitrateVisibility());
	insert_action(new Actions::AddRandomItems());
	insert_action(new Actions::ToggleBrowserSortMode());
	insert_action(new Actions::ToggleLibraryTagType());
	insert_action(new Actions::ToggleMediaLibrarySortMode());
	insert_action(new Actions::FetchLyricsInBackground());
	insert_action(new Actions::RefetchLyrics());
	insert_action(new Actions::SetSelectedItemsPriority());
	insert_action(new Actions::ToggleOutput());
	insert_action(new Actions::ToggleVisualizationType());
	insert_action(new Actions::ShowSongInfo());
	insert_action(new Actions::ShowArtistInfo());
	insert_action(new Actions::ShowLyrics());
	insert_action(new Actions::Quit());
	insert_action(new Actions::NextScreen());
	insert_action(new Actions::PreviousScreen());
	insert_action(new Actions::ShowHelp());
	insert_action(new Actions::ShowPlaylist());
	insert_action(new Actions::ShowBrowser());
	insert_action(new Actions::ChangeBrowseMode());
	insert_action(new Actions::ShowSearchEngine());
	insert_action(new Actions::ResetSearchEngine());
	insert_action(new Actions::ShowMediaLibrary());
	insert_action(new Actions::ToggleMediaLibraryColumnsMode());
	insert_action(new Actions::ShowPlaylistEditor());
	insert_action(new Actions::ShowTagEditor());
	insert_action(new Actions::ShowOutputs());
	insert_action(new Actions::ShowVisualizer());
	insert_action(new Actions::ShowServerInfo());
	NcmError action_error;
	ncm_error_clear(&action_error);
	if (!ncm_action_validate(&action_error))
	{
		std::cerr << "fatal: invalid action table: "
		          << action_error.message << std::endl;
		std::exit(EXIT_FAILURE);
	}

	for (int32 i = 0; i < NCM_ACTION_LAST; i += 1)
	{
		if (AvailableActions[static_cast<size_t>(i)] == nullptr)
		{
			std::cerr << "fatal: undefined action: "
			          << ncm_action_type_name(static_cast<NcmActionType>(i))
			          << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}
}

bool scrollTagCanBeRun(NC::List *&list, const SongList *&songs)
{
	auto w = screenLegacyCurrent()->activeWindow();
	if (list != static_cast<void *>(w))
		list = dynamic_cast<NC::List *>(w);
	if (songs != static_cast<void *>(w))
		songs = dynamic_cast<SongList *>(w);
	return list != nullptr && !list->empty()
	    && songs != nullptr;
}

void scrollTagUpRun(NC::List *list, const SongList *songs, enum NcmSongGetter getter)
{
	const auto front = songs->beginS();
	auto it = songs->currentS();
	if (it->song() != nullptr)
	{
		const std::string tag = it->song()->getTags(getter);
		while (it != front)
		{
			--it;
			if (it->song() == nullptr || it->song()->getTags(getter) != tag)
				break;
		}
		list->highlight(it-front);
	}
}

void scrollTagDownRun(NC::List *list, const SongList *songs, enum NcmSongGetter getter)
{
	const auto front = songs->beginS(), back = --songs->endS();
	auto it = songs->currentS();
	if (it->song() != nullptr)
	{
		const std::string tag = it->song()->getTags(getter);
		while (it != back)
		{
			++it;
			if (it->song() == nullptr || it->song()->getTags(getter) != tag)
				break;
		}
		list->highlight(it-front);
	}
}

void seek(SearchDirection sd)
{

	if (!Status::State::totalTime())
	{
		Statusbar::print("Unknown item length");
		return;
	}

	Progressbar::ScopedLock progressbar_lock;
	Statusbar::ScopedLock statusbar_lock;

	unsigned songpos = Status::State::elapsedTime();
	auto t = global_timer;

	NC::Window::ScopedTimeout stimeout{*static_cast<NC::Window *>(ui_state_footer_window()), BaseScreen::defaultWindowTimeout};

	// Accept single action of a given type or action chain for which all actions
	// can be run and one of them is of the given type. This will still not work
	// in some contrived cases, but allows for more flexibility than accepting
	// single actions only.
	auto hasRunnableAction = [](NcmBindingSlice bindings,
	                            Actions::Type type) {
		for (int32 i = 0; i < bindings.len; i += 1)
		{
			if (bindings_legacy_has_runnable_action(bindings.data + i, type))
				return true;
		}
		return false;
	};

	global_seeking_in_progress = true;
	while (true)
	{
	Status::trace();

		int64 elapsed_seconds = global_timer_elapsed_seconds(t);
		unsigned howmuch = Config.incremental_seeking
		                 ? static_cast<unsigned>(elapsed_seconds/2)+Config.seek_time
		                 : Config.seek_time;

		NcKey input = ncm_read_key(static_cast<NC::Window *>(ui_state_footer_window())->nativeWindow());

		switch (sd)
		{
		case NCM_SEARCH_DIRECTION_BACKWARD:
			if (songpos > 0)
			{
				if (songpos < howmuch)
					songpos = 0;
				else
					songpos -= howmuch;
			}
			break;
		case NCM_SEARCH_DIRECTION_FORWARD:
			if (songpos < Status::State::totalTime())
				songpos = std::min(songpos + howmuch, Status::State::totalTime());
			break;
		};

		std::string tracklength;
		// FIXME: merge this with the code in status.cpp
		switch (Config.design)
		{
			case NCM_DESIGN_CLASSIC:
				tracklength = " [";
				if (Config.display_remaining_time)
				{
					tracklength += "-";
					tracklength += showSongTime(Status::State::totalTime()-songpos);
				}
				else
					tracklength += showSongTime(songpos);
				tracklength += "/";
				tracklength += showSongTime(Status::State::totalTime());
				tracklength += "]";
				*static_cast<NC::Window *>(ui_state_footer_window()) << NC::XY(static_cast<NC::Window *>(ui_state_footer_window())->getWidth()-tracklength.length(), 1)
				         << Config.statusbar_time_color
				         << tracklength
				         << NC::FormattedColor::End<>(Config.statusbar_time_color);
				break;
			case NCM_DESIGN_ALTERNATIVE:
				if (Config.display_remaining_time)
				{
					tracklength = "-";
					tracklength += showSongTime(Status::State::totalTime()-songpos);
				}
				else
					tracklength = showSongTime(songpos);
				tracklength += "/";
				tracklength += showSongTime(Status::State::totalTime());
				*static_cast<NC::Window *>(ui_state_header_window()) << NC::XY(0, 0)
				         << Config.statusbar_time_color
				         << tracklength
				         << NC::FormattedColor::End<>(Config.statusbar_time_color)
				         << " ";
				static_cast<NC::Window *>(ui_state_header_window())->refresh();
				break;
		}
		Progressbar::draw(songpos, Status::State::totalTime());
		static_cast<NC::Window *>(ui_state_footer_window())->refresh();

		auto k = ncm_bindings_configuration_get(&Bindings, input);
		if (hasRunnableAction(k, Actions::Type::SeekBackward))
			sd = NCM_SEARCH_DIRECTION_BACKWARD;
		else if (hasRunnableAction(k, Actions::Type::SeekForward))
			sd = NCM_SEARCH_DIRECTION_FORWARD;
		else
			break;
	}
	global_seeking_in_progress = false;
	Mpd.Seek(Status::State::currentSongPosition(), songpos);
}

void findItem(const SearchDirection direction)
{

	Searchable *w = dynamic_cast<Searchable *>(screenLegacyCurrent());
	assert(w != nullptr);
	assert(w->allowsSearching());

	std::string constraint;
	try
	{
		ScopedValue<bool> disabled_autocenter_mode(Config.autocenter_mode, false);
		Statusbar::ScopedLock slock;
		NC::Window::ScopedPromptHook prompt_hook(
			*static_cast<NC::Window *>(ui_state_footer_window()),
			Statusbar::Helpers::FindImmediately(w, direction));
		Statusbar::put() << stringFormat("Find %1%: ", direction);
		constraint = static_cast<NC::Window *>(ui_state_footer_window())->prompt(constraint);
	}
	catch (NC::PromptAborted &)
	{
		w->setSearchConstraint(constraint);
		w->search(direction, Config.wrapped_search, false);
		Statusbar::print("Action cancelled");
		return;
	}

	if (constraint.empty())
	{
		Statusbar::printf("Constraint unset");
		w->clearSearchConstraint();
	}
	else
		Statusbar::printf("Using constraint \"%1%\"", constraint);
}

void listsChangeFinisher()
{
	if (screenLegacyCurrent() == myLibrary
	||  screenLegacyCurrent() == myPlaylistEditor
#	ifdef HAVE_TAGLIB_H
	||  screenLegacyCurrent() == myTagEditor
#	endif // HAVE_TAGLIB_H
	   )
	{
		if (screenLegacyCurrent()->activeWindow() == &myLibrary->Tags)
		{
			myLibrary->Albums.clear();
			myLibrary->Albums.refresh();
			myLibrary->Songs.clear();
			myLibrary->Songs.refresh();
			myLibrary->updateTimer();
		}
		else if (screenLegacyCurrent()->activeWindow() == &myLibrary->Albums)
		{
			myLibrary->Songs.clear();
			myLibrary->Songs.refresh();
			myLibrary->updateTimer();
		}
		else if (screenLegacyCurrent()->isActiveWindow(myPlaylistEditor->Playlists))
		{
			myPlaylistEditor->Content.clear();
			myPlaylistEditor->Content.refresh();
			myPlaylistEditor->updateTimer();
		}
#		ifdef HAVE_TAGLIB_H
		else if (screenLegacyCurrent()->activeWindow() == myTagEditor->Dirs)
		{
			myTagEditor->Tags->clear();
		}
#		endif // HAVE_TAGLIB_H
	}
}

}


extern "C" {

bool ncmpcpp_legacy_configure(int32 argc, char **argv)
{
	return configure(argc, argv);
}

void ncmpcpp_legacy_init_screen(bool enable_colors, bool enable_mouse)
{
	NC::initScreen(enable_colors, enable_mouse);
}

void ncmpcpp_legacy_destroy_screen(void)
{
	NC::destroyScreen();
}

void ncmpcpp_legacy_set_statusbar_visibility_baseline(bool visible)
{
	Actions::OriginalStatusbarVisibility = visible;
}

void ncmpcpp_legacy_set_windows_dimensions(void)
{
	Actions::setWindowsDimensions();
}

int64 ncmpcpp_legacy_header_height(void)
{
	return static_cast<int64>(Actions::HeaderHeight);
}

int64 ncmpcpp_legacy_footer_height(void)
{
	return static_cast<int64>(Actions::FooterHeight);
}

int64 ncmpcpp_legacy_footer_start_y(void)
{
	return static_cast<int64>(Actions::FooterStartY);
}

void *ncmpcpp_legacy_window_create(int64 start_x, int64 start_y,
                                   int64 width, int64 height,
                                   NcColor color)
{
	NC::Color window_color = NC::fromNcColor(color);

	return new NC::Window(static_cast<size_t>(start_x),
	                      static_cast<size_t>(start_y),
	                      static_cast<size_t>(width),
	                      static_cast<size_t>(height), "",
	                      window_color, NC::Border());
}

void ncmpcpp_legacy_window_display(void *window)
{
	if (window == nullptr)
		return;
	static_cast<NC::Window *>(window)->display();
}

void ncmpcpp_legacy_window_destroy(void *window)
{
	delete static_cast<NC::Window *>(window);
}

void ncmpcpp_legacy_window_set_main_hook(void *window)
{
	if (window == nullptr)
		return;
	static_cast<NC::Window *>(window)->setPromptHook(
	    Statusbar::Helpers::mainHook);
}

void ncmpcpp_legacy_window_clear_fd_callbacks(void *window)
{
	if (window == nullptr)
		return;
	static_cast<NC::Window *>(window)->clearFDCallbacksList();
}

NcWindow *ncmpcpp_legacy_window_native(void *window)
{
	if (window == nullptr)
		return nullptr;
	return static_cast<NC::Window *>(window)->nativeWindow();
}

void ncmpcpp_legacy_initialize_screens(void)
{
	Actions::initializeScreens();
}

void ncmpcpp_legacy_resize_screen(bool reload_main_window)
{
	try
	{
		Actions::resizeScreen(reload_main_window);
	}
	catch (MPD::ClientError &e)
	{
		Status::handleClientError(e);
	}
	catch (MPD::ServerError &e)
	{
		Status::handleServerError(e);
	}
	catch (std::exception &e)
	{
		Statusbar::printf("Unexpected error: %1%", e.what());
	}
}

void ncmpcpp_legacy_playlist_switch_to(void)
{
	(void)native_c_screens_switch_to_type(NCM_SCREEN_TYPE_PLAYLIST);
}

void ncmpcpp_legacy_playlist_enable_highlighting_if_current(void)
{
	if (screenLegacyCurrent() == myPlaylist)
		myPlaylist->enableHighlighting();
}

bool ncmpcpp_legacy_switch_to_screen_type(enum ScreenType screen_type)
{
	return native_c_screens_switch_to_type(screen_type);
}

bool ncmpcpp_legacy_lock_current_screen(void)
{
	return native_c_screens_lock_current();
}

enum ScreenType ncmpcpp_legacy_current_screen_type(void)
{
	return native_c_screens_current_type();
}

void ncmpcpp_legacy_set_noidle_status_callback(void)
{
	ncm_mpd_client_set_noidle_callback(
	    &global_mpd, legacy_noidle_status_update, nullptr);
}

bool ncmpcpp_legacy_mpd_connected(void)
{
	return ncm_mpd_client_connected(&global_mpd);
}

void ncmpcpp_legacy_connect_or_report(void)
{
	NcmError error{};

	if (!ncm_mpd_client_connect(&global_mpd, &error))
	{
		legacy_report_mpd_error(error);
		return;
	}

	if (ncm_mpd_client_version(&global_mpd) < 16)
	{
		ncm_mpd_client_disconnect(&global_mpd);
		ncm_error_set(&error, MPD_ERROR_STATE,
		              const_cast<char *>("MPD < 0.16.0 is not supported"),
		              static_cast<int32>(
		                  strlen("MPD < 0.16.0 is not supported")));
		legacy_report_mpd_error(error);
	}
}

void ncmpcpp_legacy_status_clear(void)
{
	Status::clear();
}

bool ncmpcpp_legacy_update_environment(bool update_timer,
                                        bool refresh_window,
                                        bool mpd_sync)
{
	try
	{
		auto action = Actions::runtimeAction(
		    NCM_ACTION_UPDATE_ENVIRONMENT);
		auto update_environment = static_cast<Actions::UpdateEnvironment *>(
		    action);

		if (update_environment == nullptr)
			return false;
		update_environment->run(update_timer, refresh_window, mpd_sync);
		return true;
	}
	catch (MPD::ClientError &e)
	{
		Status::handleClientError(e);
	}
	catch (MPD::ServerError &e)
	{
		Status::handleServerError(e);
	}
	catch (std::exception &e)
	{
		Statusbar::printf("Unexpected error: %1%", e.what());
	}
	return false;
}

bool ncmpcpp_legacy_execute_binding(NcmBinding *binding)
{
	try
	{
		return bindings_legacy_execute(binding);
	}
	catch (ConversionError &e)
	{
		Statusbar::printf("Invalid value: %1%", e.value());
	}
	catch (OutOfBounds &e)
	{
		Statusbar::printf("Error: %1%", e.errorMessage());
	}
	catch (NC::PromptAborted &)
	{
		Statusbar::printf("Action aborted");
	}
	catch (MPD::ClientError &e)
	{
		Status::handleClientError(e);
	}
	catch (MPD::ServerError &e)
	{
		Status::handleServerError(e);
	}
	catch (std::exception &e)
	{
		Statusbar::printf("Unexpected error: %1%", e.what());
	}
	return false;
}

bool ncmpcpp_legacy_execute_action(enum NcmActionType type)
{
	try
	{
		return Actions::execute(type);
	}
	catch (MPD::ClientError &e)
	{
		Status::handleClientError(e);
	}
	catch (MPD::ServerError &e)
	{
		Status::handleServerError(e);
	}
	catch (std::exception &e)
	{
		Statusbar::printf("Unexpected error: %1%", e.what());
	}
	return false;
}

bool ncmpcpp_legacy_exit_requested(void)
{
	return Actions::ExitMainLoop;
}

}
