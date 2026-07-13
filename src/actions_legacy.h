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

#ifndef NCMPCPP_ACTIONS_LEGACY_H
#define NCMPCPP_ACTIONS_LEGACY_H

#include <map>
#include <string>
#include "actions.h"
#include "c/ncm_time.h"
#include "curses/window.h"
#include "interfaces.h"

// forward declarations
struct SongList;

namespace Actions {

enum class Type
{
	MacroUtility = -1,
	Dummy,
	UpdateEnvironment,
	MouseEvent,
	ScrollUp,
	ScrollDown,
	ScrollUpArtist,
	ScrollUpAlbum,
	ScrollDownArtist,
	ScrollDownAlbum,
	PageUp,
	PageDown,
	MoveHome,
	MoveEnd,
	ToggleInterface,
	JumpToParentDirectory,
	RunAction,
	PreviousColumn,
	NextColumn,
	MasterScreen,
	SlaveScreen,
	VolumeUp,
	VolumeDown,
	AddItemToPlaylist,
	PlayItem,
	DeletePlaylistItems,
	DeleteStoredPlaylist,
	DeleteBrowserItems,
	ReplaySong,
	Previous,
	Next,
	Pause,
	Stop,
	Play,
	ExecuteCommand,
	SavePlaylist,
	MoveSortOrderUp,
	MoveSortOrderDown,
	MoveSelectedItemsUp,
	MoveSelectedItemsDown,
	MoveSelectedItemsTo,
	Add,
	Load,
	SeekForward,
	SeekBackward,
	ToggleDisplayMode,
	ToggleSeparatorsBetweenAlbums,
	ToggleLyricsUpdateOnSongChange,
	ToggleLyricsFetcher,
	ToggleFetchingLyricsInBackground,
	TogglePlayingSongCentering,
	UpdateDatabase,
	JumpToPlayingSong,
	ToggleRepeat,
	Shuffle,
	ToggleRandom,
	StartSearching,
	SaveTagChanges,
	ToggleSingle,
	ToggleConsume,
	ToggleCrossfade,
	SetCrossfade,
	SetVolume,
	EnterDirectory,
	EditSong,
	EditLibraryTag,
	EditLibraryAlbum,
	EditDirectoryName,
	EditPlaylistName,
	EditLyrics,
	JumpToBrowser,
	JumpToMediaLibrary,
	JumpToPlaylistEditor,
	ToggleScreenLock,
	JumpToTagEditor,
	JumpToPositionInSong,
	SelectItem,
	SelectRange,
	ReverseSelection,
	RemoveSelection,
	SelectAlbum,
	SelectFoundItems,
	AddSelectedItems,
	CropMainPlaylist,
	CropPlaylist,
	ClearMainPlaylist,
	ClearPlaylist,
	SortPlaylist,
	ReversePlaylist,
	ApplyFilter,
	Find,
	FindItemForward,
	FindItemBackward,
	NextFoundItem,
	PreviousFoundItem,
	ToggleFindMode,
	ToggleReplayGainMode,
	ToggleAddMode,
	ToggleMouse,
	ToggleBitrateVisibility,
	AddRandomItems,
	ToggleBrowserSortMode,
	ToggleLibraryTagType,
	ToggleMediaLibrarySortMode,
	FetchLyricsInBackground,
	RefetchLyrics,
	SetSelectedItemsPriority,
	ToggleOutput,
	ToggleVisualizationType,
	ShowSongInfo,
	ShowArtistInfo,
	ShowLyrics,
	Quit,
	NextScreen,
	PreviousScreen,
	ShowHelp,
	ShowPlaylist,
	ShowBrowser,
	ChangeBrowseMode,
	ShowSearchEngine,
	ResetSearchEngine,
	ShowMediaLibrary,
	ToggleMediaLibraryColumnsMode,
	ShowPlaylistEditor,
	ShowTagEditor,
	ShowOutputs,
	ShowVisualizer,
	ShowServerInfo,
	_numberOfActions // needed to dynamically calculate size of action array
};

void initializeScreens();
void setResizeFlags();
void resizeScreen(bool reload_main_window);

bool confirmAction(const std::string &description);

bool isMPDMusicDirSet();

extern bool ExitMainLoop;

struct BaseAction
{
	BaseAction(Type type_): m_type(type_) { }

	virtual ~BaseAction() { }

	Type type() const { return m_type; }
	
	virtual bool canBeRun() { return true; }
	
	bool execute()
	{
		if (canBeRun())
		{
			run();
			return true;
		}
		return false;
	}
private:
	virtual void run() = 0;

	Type m_type;
};

BaseAction *runtimeAction(enum NcmActionType type);
bool execute(enum NcmActionType type);

struct UpdateEnvironment: BaseAction
{
	UpdateEnvironment();

	void run(bool update_status, bool refresh_window, bool mpd_sync);

private:
	NcmTimePoint m_past;

	virtual void run() override;
};

struct MouseEvent: BaseAction
{
	MouseEvent(): BaseAction(Type::MouseEvent)
	{
		m_old_mouse_event.bstate = 0;
		m_mouse_event.bstate = 0;
	}
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
	
	MEVENT m_mouse_event;
	MEVENT m_old_mouse_event;
};

struct ScrollUp: BaseAction
{
	ScrollUp(): BaseAction(Type::ScrollUp) { }
	
private:
	virtual void run() override;
};

struct ScrollDown: BaseAction
{
	ScrollDown(): BaseAction(Type::ScrollDown) { }
	
private:
	virtual void run() override;
};

struct ScrollUpArtist: BaseAction
{
	ScrollUpArtist(): BaseAction(Type::ScrollUpArtist) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
	const SongList *m_songs;
};

struct ScrollUpAlbum: BaseAction
{
	ScrollUpAlbum(): BaseAction(Type::ScrollUpAlbum) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
	const SongList *m_songs;
};

struct ScrollDownArtist: BaseAction
{
	ScrollDownArtist(): BaseAction(Type::ScrollDownArtist) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
	const SongList *m_songs;
};

struct ScrollDownAlbum: BaseAction
{
	ScrollDownAlbum(): BaseAction(Type::ScrollDownAlbum) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
	const SongList *m_songs;
};

struct PageUp: BaseAction
{
	PageUp(): BaseAction(Type::PageUp) { }
	
private:
	virtual void run() override;
};

struct PageDown: BaseAction
{
	PageDown(): BaseAction(Type::PageDown) { }
	
private:
	virtual void run() override;
};

struct MoveHome: BaseAction
{
	MoveHome(): BaseAction(Type::MoveHome) { }
	
private:
	virtual void run() override;
};

struct MoveEnd: BaseAction
{
	MoveEnd(): BaseAction(Type::MoveEnd) { }
	
private:
	virtual void run() override;
};

struct ToggleInterface: BaseAction
{
	ToggleInterface(): BaseAction(Type::ToggleInterface) { }
	
private:
	virtual void run() override;
};

struct JumpToParentDirectory: BaseAction
{
	JumpToParentDirectory(): BaseAction(Type::JumpToParentDirectory) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct PreviousColumn: BaseAction
{
	PreviousColumn(): BaseAction(Type::PreviousColumn) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	HasColumns *m_hc;
};

struct NextColumn: BaseAction
{
	NextColumn(): BaseAction(Type::NextColumn) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	HasColumns *m_hc;
};

struct MasterScreen: BaseAction
{
	MasterScreen(): BaseAction(Type::MasterScreen) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct SlaveScreen: BaseAction
{
	SlaveScreen(): BaseAction(Type::SlaveScreen) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct VolumeUp: BaseAction
{
	VolumeUp(): BaseAction(Type::VolumeUp) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct VolumeDown: BaseAction
{
	VolumeDown(): BaseAction(Type::VolumeDown) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct AddItemToPlaylist: BaseAction
{
	AddItemToPlaylist(): BaseAction(Type::AddItemToPlaylist) { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;

	HasSongs *m_hs;
};

struct PlayItem: BaseAction
{
	PlayItem(): BaseAction(Type::PlayItem) { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;

	HasSongs *m_hs;
};

struct DeletePlaylistItems: BaseAction
{
	DeletePlaylistItems(): BaseAction(Type::DeletePlaylistItems) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct DeleteStoredPlaylist: BaseAction
{
	DeleteStoredPlaylist(): BaseAction(Type::DeleteStoredPlaylist) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct DeleteBrowserItems: BaseAction
{
	DeleteBrowserItems(): BaseAction(Type::DeleteBrowserItems) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct SavePlaylist: BaseAction
{
	SavePlaylist(): BaseAction(Type::SavePlaylist) { }
	
private:
	virtual void run() override;
};

struct MoveSelectedItemsUp: BaseAction
{
	MoveSelectedItemsUp(): BaseAction(Type::MoveSelectedItemsUp) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct MoveSelectedItemsDown: BaseAction
{
	MoveSelectedItemsDown(): BaseAction(Type::MoveSelectedItemsDown) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct MoveSelectedItemsTo: BaseAction
{
	MoveSelectedItemsTo(): BaseAction(Type::MoveSelectedItemsTo) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct Add: BaseAction
{
	Add(): BaseAction(Type::Add) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct Load: BaseAction
{
	Load(): BaseAction(Type::Load) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleDisplayMode: BaseAction
{
	ToggleDisplayMode(): BaseAction(Type::ToggleDisplayMode) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleSeparatorsBetweenAlbums: BaseAction
{
	ToggleSeparatorsBetweenAlbums()
	: BaseAction(Type::ToggleSeparatorsBetweenAlbums) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleLyricsUpdateOnSongChange: BaseAction
{
	ToggleLyricsUpdateOnSongChange()
	: BaseAction(Type::ToggleLyricsUpdateOnSongChange) { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleLyricsFetcher: BaseAction
{
	ToggleLyricsFetcher(): BaseAction(Type::ToggleLyricsFetcher) { }
	
private:
	virtual void run() override;
};

struct ToggleFetchingLyricsInBackground: BaseAction
{
	ToggleFetchingLyricsInBackground()
	: BaseAction(Type::ToggleFetchingLyricsInBackground) { }
	
private:
	virtual void run() override;
};

struct TogglePlayingSongCentering: BaseAction
{
	TogglePlayingSongCentering()
	: BaseAction(Type::TogglePlayingSongCentering) { }
	
private:
	virtual void run() override;
};

struct JumpToPlayingSong: BaseAction
{
	JumpToPlayingSong(): BaseAction(Type::JumpToPlayingSong) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	MPD::Song m_song;
};

struct Shuffle: BaseAction
{
	Shuffle(): BaseAction(Type::Shuffle) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::Menu<MPD::Song>::ConstIterator m_begin;
	NC::Menu<MPD::Song>::ConstIterator m_end;
};

struct SaveTagChanges: BaseAction
{
	SaveTagChanges(): BaseAction(Type::SaveTagChanges) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct SetCrossfade: BaseAction
{
	SetCrossfade(): BaseAction(Type::SetCrossfade) { }
	
private:
	virtual void run() override;
};

struct SetVolume: BaseAction
{
	SetVolume(): BaseAction(Type::SetVolume) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct EnterDirectory: BaseAction
{
	EnterDirectory(): BaseAction(Type::EnterDirectory) { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;
};


struct EditSong: BaseAction
{
	EditSong(): BaseAction(Type::EditSong) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

#ifdef HAVE_TAGLIB_H
	MPD::Song m_song;
#endif // HAVE_TAGLIB_H
};

struct EditLibraryTag: BaseAction
{
	EditLibraryTag(): BaseAction(Type::EditLibraryTag) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct EditLibraryAlbum: BaseAction
{
	EditLibraryAlbum(): BaseAction(Type::EditLibraryAlbum) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct EditDirectoryName: BaseAction
{
	EditDirectoryName(): BaseAction(Type::EditDirectoryName) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct EditPlaylistName: BaseAction
{
	EditPlaylistName(): BaseAction(Type::EditPlaylistName) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct EditLyrics: BaseAction
{
	EditLyrics(): BaseAction(Type::EditLyrics) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct JumpToBrowser: BaseAction
{
	JumpToBrowser(): BaseAction(Type::JumpToBrowser) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	MPD::Song m_song;
};

struct JumpToPlaylistEditor: BaseAction
{
	JumpToPlaylistEditor(): BaseAction(Type::JumpToPlaylistEditor) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleScreenLock: BaseAction
{
	ToggleScreenLock(): BaseAction(Type::ToggleScreenLock) { }
	
private:
	virtual void run() override;
};

struct JumpToTagEditor: BaseAction
{
	JumpToTagEditor(): BaseAction(Type::JumpToTagEditor) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

#ifdef HAVE_TAGLIB_H
	MPD::Song m_song;
#endif // HAVE_TAGLIB_H
};

struct JumpToPositionInSong: BaseAction
{
	JumpToPositionInSong(): BaseAction(Type::JumpToPositionInSong) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct SelectItem: BaseAction
{
	SelectItem(): BaseAction(Type::SelectItem) { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
};

struct SelectRange: BaseAction
{
	SelectRange(): BaseAction(Type::SelectRange) { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
	NC::List::Iterator m_begin;
	NC::List::Iterator m_end;
};

struct ReverseSelection: BaseAction
{
	ReverseSelection(): BaseAction(Type::ReverseSelection) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
};

struct RemoveSelection: BaseAction
{
	RemoveSelection(): BaseAction(Type::RemoveSelection) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
};

struct SelectAlbum: BaseAction
{
	SelectAlbum(): BaseAction(Type::SelectAlbum) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
	SongList *m_songs;
};

struct SelectFoundItems: BaseAction
{
	SelectFoundItems(): BaseAction(Type::SelectFoundItems) { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
	Searchable *m_searchable;
};

struct CropMainPlaylist: BaseAction
{
	CropMainPlaylist(): BaseAction(Type::CropMainPlaylist) { }
	
private:
	virtual void run() override;
};

struct CropPlaylist: BaseAction
{
	CropPlaylist(): BaseAction(Type::CropPlaylist) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ClearMainPlaylist: BaseAction
{
	ClearMainPlaylist(): BaseAction(Type::ClearMainPlaylist) { }
	
private:
	virtual void run() override;
};

struct ClearPlaylist: BaseAction
{
	ClearPlaylist(): BaseAction(Type::ClearPlaylist) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ReversePlaylist: BaseAction
{
	ReversePlaylist(): BaseAction(Type::ReversePlaylist) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::Menu<MPD::Song>::ConstIterator m_begin;
	NC::Menu<MPD::Song>::ConstIterator m_end;
};

struct Find: BaseAction
{
	Find(): BaseAction(Type::Find) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct NextFoundItem: BaseAction
{
	NextFoundItem(): BaseAction(Type::NextFoundItem) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct PreviousFoundItem: BaseAction
{
	PreviousFoundItem(): BaseAction(Type::PreviousFoundItem) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleFindMode: BaseAction
{
	ToggleFindMode(): BaseAction(Type::ToggleFindMode) { }
	
private:
	virtual void run() override;
};

struct ToggleReplayGainMode: BaseAction
{
	ToggleReplayGainMode(): BaseAction(Type::ToggleReplayGainMode) { }
	
private:
	virtual void run() override;
};

struct ToggleAddMode: BaseAction
{
	ToggleAddMode(): BaseAction(Type::ToggleAddMode) { }
	
private:
	virtual void run() override;
};

struct ToggleMouse: BaseAction
{
	ToggleMouse(): BaseAction(Type::ToggleMouse) { }
	
private:
	virtual void run() override;
};

struct ToggleBitrateVisibility: BaseAction
{
	ToggleBitrateVisibility(): BaseAction(Type::ToggleBitrateVisibility) { }
	
private:
	virtual void run() override;
};

struct AddRandomItems: BaseAction
{
	AddRandomItems(): BaseAction(Type::AddRandomItems) { }
	
private:
	virtual void run() override;
};

struct ToggleBrowserSortMode: BaseAction
{
	ToggleBrowserSortMode(): BaseAction(Type::ToggleBrowserSortMode) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleLibraryTagType: BaseAction
{
	ToggleLibraryTagType(): BaseAction(Type::ToggleLibraryTagType) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct FetchLyricsInBackground: BaseAction
{
	FetchLyricsInBackground()
		: BaseAction(Type::FetchLyricsInBackground) { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;

};

struct RefetchLyrics: BaseAction
{
	RefetchLyrics(): BaseAction(Type::RefetchLyrics) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct SetSelectedItemsPriority: BaseAction
{
	SetSelectedItemsPriority()
	: BaseAction(Type::SetSelectedItemsPriority) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleOutput: BaseAction
{
	ToggleOutput(): BaseAction(Type::ToggleOutput) { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleVisualizationType: BaseAction
{
	ToggleVisualizationType()
	: BaseAction(Type::ToggleVisualizationType) { }

private:
	
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowSongInfo: BaseAction
{
	ShowSongInfo(): BaseAction(Type::ShowSongInfo) { }
	
private:
	virtual void run() override;
};

struct ShowArtistInfo: BaseAction
{
	ShowArtistInfo(): BaseAction(Type::ShowArtistInfo) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowLyrics: BaseAction
{
	ShowLyrics(): BaseAction(Type::ShowLyrics) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

};

struct NextScreen: BaseAction
{
	NextScreen(): BaseAction(Type::NextScreen) { }
	
private:
	virtual void run() override;
};

struct PreviousScreen: BaseAction
{
	PreviousScreen(): BaseAction(Type::PreviousScreen) { }
	
private:
	virtual void run() override;
};

struct ShowHelp: BaseAction
{
	ShowHelp(): BaseAction(Type::ShowHelp) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowPlaylist: BaseAction
{
	ShowPlaylist(): BaseAction(Type::ShowPlaylist) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowBrowser: BaseAction
{
	ShowBrowser(): BaseAction(Type::ShowBrowser) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ChangeBrowseMode: BaseAction
{
	ChangeBrowseMode(): BaseAction(Type::ChangeBrowseMode) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowPlaylistEditor: BaseAction
{
	ShowPlaylistEditor(): BaseAction(Type::ShowPlaylistEditor) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowTagEditor: BaseAction
{
	ShowTagEditor(): BaseAction(Type::ShowTagEditor) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowOutputs: BaseAction
{
	ShowOutputs(): BaseAction(Type::ShowOutputs) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowVisualizer: BaseAction
{
	ShowVisualizer(): BaseAction(Type::ShowVisualizer) { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};


struct ShowServerInfo: BaseAction
{
	ShowServerInfo(): BaseAction(Type::ShowServerInfo) { }
	
private:
#	ifdef HAVE_TAGLIB_H
	virtual bool canBeRun() override;
#	endif // HAVE_TAGLIB_H
	virtual void run() override;
};

}

#endif // NCMPCPP_ACTIONS_LEGACY_H
