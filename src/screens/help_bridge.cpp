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
#include <string>
#include <vector>

#include "bindings.h"
#include "curses/formatted_color.h"
#include "global.h"
#include "screens/help.h"
#include "screens/screen_switcher.h"
#include "settings.h"
#include "title.h"
#include "utility/string.h"
#include "utility/string_format.h"
#include "utility/utf8.h"

using Global::MainHeight;
using Global::MainStartY;

namespace {

struct HelpBuffer
{
    NcBuffer *buffer;
};

NcScroll toNcScroll(NC::Scroll where)
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

NcBorder toNcBorder(NC::Border border)
{
    NcBorder result = {};

    if (border)
    {
        result.enabled = true;
        result.color = NC::toNcColor(*border);
    }
    return result;
}

HelpBuffer &operator<<(HelpBuffer &output, char value)
{
    nc_buffer_append_char(output.buffer, value);
    return output;
}

HelpBuffer &operator<<(HelpBuffer &output, const char *value)
{
    nc_buffer_append_cstring(output.buffer, const_cast<char *>(value));
    return output;
}

HelpBuffer &operator<<(HelpBuffer &output, const std::string &value)
{
    nc_buffer_append_data(output.buffer,
                          const_cast<char *>(value.data()),
                          static_cast<int32>(value.size()));
    return output;
}

HelpBuffer &operator<<(HelpBuffer &output, int value)
{
    nc_buffer_append_int32(output.buffer, static_cast<int32>(value));
    return output;
}

HelpBuffer &operator<<(HelpBuffer &output, NC::Format format)
{
    nc_buffer_add_format(output.buffer,
                         nc_buffer_len(output.buffer),
                         NC::toNcFormat(format),
                         0);
    return output;
}

HelpBuffer &operator<<(HelpBuffer &output, NC::Color color)
{
    nc_buffer_add_color(output.buffer,
                        nc_buffer_len(output.buffer),
                        NC::toNcColor(color),
                        0);
    return output;
}

std::string align_key_rep(std::string keys)
{
	size_t i = 0, len = 0;
	const size_t max_len = 20;
	for (; i < keys.size(); ++i)
	{
		size_t width = Utf8::width(keys.substr(i, 1));
		if (len+width > max_len)
			break;
		else
			len += width;
	}
	keys.resize(i + max_len - len, ' ');
	return keys;
}

std::string display_keys(const Actions::Type at)
{
	std::string result, skey;
	for (auto it = Bindings.begin(); it != Bindings.end(); ++it)
	{
		for (auto j = it->second.begin(); j != it->second.end(); ++j)
		{
			if (j->isSingle() && j->action().type() == at)
			{
				skey = NC::keyToString(it->first);
				if (!skey.empty())
				{
					result += std::move(skey);
					result += ' ';
				}
			}
		}
	}
	return align_key_rep(std::move(result));
}

void section(HelpBuffer &w, const char *type_, const char *title_)
{
	w << "\n  " << NC::Format::Bold;
	if (type_[0] != '\0')
		w << type_ << " - ";
	w << title_ << NC::Format::NoBold << "\n\n";
}

/**********************************************************************/

void key_section(HelpBuffer &w, const char *title_)
{
	section(w, "Keys", title_);
}

void key(HelpBuffer &w, const Actions::Type at, const char *desc)
{
	w << "    " << display_keys(at) << " : " << desc << '\n';
}

void key(HelpBuffer &w, const Actions::Type at, const std::string &desc)
{
	w << "    " << display_keys(at) << " : " << desc << '\n';
}

void key(HelpBuffer &w, NC::Key::Type k, const std::string &desc)
{
	w << "    " << align_key_rep(NC::keyToString(k)) << " : " << desc << '\n';
}

/**********************************************************************/

void mouse_section(HelpBuffer &w, const char *title_)
{
	section(w, "Mouse", title_);
}

void mouse(HelpBuffer &w, std::string action, const char *desc, bool indent = false)
{
	action.resize(31 - (indent ? 2 : 0), ' ');
	w << "    " << (indent ? "  " : "") << action;
	w << ": " << desc << '\n';
}

void mouse_column(HelpBuffer &w, const char *column)
{
	w << NC::Format::Bold << "    " << column << " column:\n" << NC::Format::NoBold;
}

/**********************************************************************/

void write_bindings(HelpBuffer &w)
{
	using Actions::Type;

	key_section(w, "Movement");
	key(w, Type::ScrollUp, "Move cursor up");
	key(w, Type::ScrollDown, "Move cursor down");
	key(w, Type::ScrollUpAlbum, "Move cursor up one album");
	key(w, Type::ScrollDownAlbum, "Move cursor down one album");
	key(w, Type::ScrollUpArtist, "Move cursor up one artist");
	key(w, Type::ScrollDownArtist, "Move cursor down one artist");
	key(w, Type::PageUp, "Page up");
	key(w, Type::PageDown, "Page down");
	key(w, Type::MoveHome, "Home");
	key(w, Type::MoveEnd, "End");
	w << '\n';
	if (Config.screen_switcher_previous)
	{
		key(w, Type::NextScreen, "Switch between current and last screen");
		key(w, Type::PreviousScreen, "Switch between current and last screen");
	}
	else
	{
		key(w, Type::NextScreen, "Switch to next screen in sequence");
		key(w, Type::PreviousScreen, "Switch to previous screen in sequence");
	}
	key(w, Type::ShowHelp, "Show help");
	key(w, Type::ShowPlaylist, "Show playlist");
	key(w, Type::ShowBrowser, "Show browser");
	key(w, Type::ShowSearchEngine, "Show search engine");
	key(w, Type::ShowMediaLibrary, "Show media library");
	key(w, Type::ShowPlaylistEditor, "Show playlist editor");
#	ifdef HAVE_TAGLIB_H
	key(w, Type::ShowTagEditor, "Show tag editor");
#	endif // HAVE_TAGLIB_H
#	ifdef ENABLE_OUTPUTS
	key(w, Type::ShowOutputs, "Show outputs");
#	endif // ENABLE_OUTPUTS
#	ifdef ENABLE_VISUALIZER
	key(w, Type::ShowVisualizer, "Show music visualizer");
#	endif // ENABLE_VISUALIZER
	w << '\n';
	key(w, Type::ShowServerInfo, "Show server info");

	key_section(w, "Global");
	key(w, Type::Play, "Play");
	key(w, Type::Stop, "Stop");
	key(w, Type::Pause, "Pause");
	key(w, Type::Next, "Next track");
	key(w, Type::Previous, "Previous track");
	key(w, Type::ReplaySong, "Replay current song");
	key(w, Type::SeekForward, "Seek forward in playing song");
	key(w, Type::SeekBackward, "Seek backward in playing song");
	key(w, Type::VolumeDown,
		stringFormat("Decrease volume by %1%%%", Config.volume_change_step)
	);
	key(w, Type::VolumeUp,
		stringFormat("Increase volume by %1%%%", Config.volume_change_step)
	);
	w << '\n';
	key(w, Type::ToggleAddMode, "Toggle add mode (add or remove/always add)");
	key(w, Type::ToggleMouse, "Toggle mouse support");
	key(w, Type::SelectRange, "Select range");
	key(w, Type::ReverseSelection, "Reverse selection");
	key(w, Type::RemoveSelection, "Remove selection");
	key(w, Type::SelectItem, "Select current item");
	key(w, Type::SelectFoundItems, "Select found items");
	key(w, Type::SelectAlbum, "Select songs of album around the cursor");
	key(w, Type::AddSelectedItems, "Add selected items to playlist");
	key(w, Type::AddRandomItems, "Add random items to playlist");
	w << '\n';
	key(w, Type::ToggleRepeat, "Toggle repeat mode");
	key(w, Type::ToggleRandom, "Toggle random mode");
	key(w, Type::ToggleSingle, "Toggle single mode");
	key(w, Type::ToggleConsume, "Toggle consume mode");
	key(w, Type::ToggleReplayGainMode, "Toggle replay gain mode");
	key(w, Type::ToggleBitrateVisibility, "Toggle bitrate visibility");
	key(w, Type::ToggleCrossfade, "Toggle crossfade mode");
	key(w, Type::SetCrossfade, "Set crossfade");
	key(w, Type::SetVolume, "Set volume");
	key(w, Type::UpdateDatabase, "Start music database update");
	w << '\n';
	key(w, Type::ExecuteCommand, "Execute command");
	key(w, Type::ApplyFilter, "Apply filter");
	key(w, Type::FindItemForward, "Find item forward");
	key(w, Type::FindItemBackward, "Find item backward");
	key(w, Type::PreviousFoundItem, "Jump to previous found item");
	key(w, Type::NextFoundItem, "Jump to next found item");
	key(w, Type::ToggleFindMode, "Toggle find mode (normal/wrapped)");
	key(w, Type::JumpToBrowser, "Locate song in browser");
	key(w, Type::JumpToMediaLibrary, "Locate song in media library");
	key(w, Type::ToggleScreenLock, "Lock/unlock current screen");
	key(w, Type::MasterScreen, "Switch to master screen (left one)");
	key(w, Type::SlaveScreen, "Switch to slave screen (right one)");
#	ifdef HAVE_TAGLIB_H
	key(w, Type::JumpToTagEditor, "Locate song in tag editor");
#	endif // HAVE_TAGLIB_H
	key(w, Type::ToggleDisplayMode, "Toggle display mode");
	key(w, Type::ToggleInterface, "Toggle user interface");
	key(w, Type::ToggleSeparatorsBetweenAlbums, "Toggle displaying separators between albums");
	key(w, Type::JumpToPositionInSong, "Jump to given position in playing song (formats: mm:ss, x%)");
	key(w, Type::ShowSongInfo, "Show song info");
	key(w, Type::ShowArtistInfo, "Show artist info");
	key(w, Type::FetchLyricsInBackground, "Fetch lyrics for selected songs");
	key(w, Type::ToggleLyricsFetcher, "Toggle lyrics fetcher");
	key(w, Type::ToggleFetchingLyricsInBackground, "Toggle fetching lyrics for playing songs in background");
	key(w, Type::ShowLyrics, "Show/hide song lyrics");
	w << '\n';
	key(w, Type::Quit, "Quit");

	key_section(w, "Playlist");
	key(w, Type::PlayItem, "Play selected item");
	key(w, Type::DeletePlaylistItems, "Delete selected item(s) from playlist");
	key(w, Type::ClearMainPlaylist, "Clear playlist");
	key(w, Type::CropMainPlaylist, "Clear playlist except selected item(s)");
	key(w, Type::SetSelectedItemsPriority, "Set priority of selected items");
	key(w, Type::MoveSelectedItemsUp, "Move selected item(s) up");
	key(w, Type::MoveSelectedItemsDown, "Move selected item(s) down");
	key(w, Type::MoveSelectedItemsTo, "Move selected item(s) to cursor position");
	key(w, Type::Add, "Add item to playlist");
	key(w, Type::Load, "Load stored playlist");
#	ifdef HAVE_TAGLIB_H
	key(w, Type::EditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	key(w, Type::SavePlaylist, "Save playlist");
	key(w, Type::Shuffle, "Shuffle range");
	key(w, Type::SortPlaylist, "Sort range");
	key(w, Type::ReversePlaylist, "Reverse range");
	key(w, Type::JumpToPlayingSong, "Jump to current song");
	key(w, Type::TogglePlayingSongCentering, "Toggle playing song centering");

	key_section(w, "Browser");
	key(w, Type::EnterDirectory, "Enter directory");
	key(w, Type::PlayItem, "Add item to playlist and play it");
	key(w, Type::AddItemToPlaylist, "Add item to playlist");
#	ifdef HAVE_TAGLIB_H
	key(w, Type::EditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	key(w, Type::EditDirectoryName, "Edit directory name");
	key(w, Type::EditPlaylistName, "Edit playlist name");
	key(w, Type::ChangeBrowseMode, "Browse MPD database/local filesystem");
	key(w, Type::ToggleBrowserSortMode, "Toggle sort mode");
	key(w, Type::JumpToPlayingSong, "Locate current song");
	key(w, Type::JumpToParentDirectory, "Jump to parent directory");
	key(w, Type::DeleteBrowserItems, "Delete selected items from disk");
	key(w, Type::JumpToPlaylistEditor, "Jump to playlist editor (playlists only)");

	key_section(w, "Search engine");
	key(w, Type::RunAction, "Modify option / Run action");
	key(w, Type::AddItemToPlaylist, "Add item to playlist");
	key(w, Type::PlayItem, "Add item to playlist and play it");
#	ifdef HAVE_TAGLIB_H
	key(w, Type::EditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	key(w, Type::StartSearching, "Start searching");
	key(w, Type::ResetSearchEngine, "Reset search constraints and clear results");

	key_section(w, "Media library");
	key(w, Type::ToggleMediaLibraryColumnsMode, "Switch between two/three columns mode");
	key(w, Type::PreviousColumn, "Previous column");
	key(w, Type::NextColumn, "Next column");
	key(w, Type::PlayItem, "Add item to playlist and play it");
	key(w, Type::AddItemToPlaylist, "Add item to playlist");
	key(w, Type::JumpToPlayingSong, "Locate current song");
#	ifdef HAVE_TAGLIB_H
	key(w, Type::EditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	key(w, Type::EditLibraryTag, "Edit tag (left column)/album (middle/right column)");
	key(w, Type::ToggleLibraryTagType, "Toggle type of tag used in left column");
	key(w, Type::ToggleMediaLibrarySortMode, "Toggle sort mode");

	key_section(w, "Playlist editor");
	key(w, Type::PreviousColumn, "Previous column");
	key(w, Type::NextColumn, "Next column");
	key(w, Type::PlayItem, "Add item to playlist and play it");
	key(w, Type::AddItemToPlaylist, "Add item to playlist");
	key(w, Type::JumpToPlayingSong, "Locate current song");
#	ifdef HAVE_TAGLIB_H
	key(w, Type::EditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	key(w, Type::EditPlaylistName, "Edit playlist name");
	key(w, Type::MoveSelectedItemsUp, "Move selected item(s) up");
	key(w, Type::MoveSelectedItemsDown, "Move selected item(s) down");
	key(w, Type::DeleteStoredPlaylist, "Delete selected playlists (left column)");
	key(w, Type::DeletePlaylistItems, "Delete selected item(s) from playlist (right column)");
	key(w, Type::ClearPlaylist, "Clear playlist");
	key(w, Type::CropPlaylist, "Clear playlist except selected items");

	key_section(w, "Lyrics");
	key(w, Type::ToggleLyricsUpdateOnSongChange, "Toggle lyrics update on song change");
	key(w, Type::EditLyrics, "Open lyrics in external editor");
	key(w, Type::RefetchLyrics, "Refetch lyrics");

#	ifdef HAVE_TAGLIB_H
	key_section(w, "Tiny tag editor");
	key(w, Type::RunAction, "Edit tag / Run action");
	key(w, Type::SaveTagChanges, "Save");

	key_section(w, "Tag editor");
	key(w, Type::EnterDirectory, "Enter directory (right column)");
	key(w, Type::RunAction, "Perform operation on selected items (middle column)");
	key(w, Type::RunAction, "Edit item (left column)");
	key(w, Type::PreviousColumn, "Previous column");
	key(w, Type::NextColumn, "Next column");
	key(w, Type::JumpToParentDirectory, "Jump to parent directory (left column, directories view)");
#	endif // HAVE_TAGLIB_H

#	ifdef ENABLE_OUTPUTS
	key_section(w, "Outputs");
	key(w, Type::ToggleOutput, "Toggle output");
#	endif // ENABLE_OUTPUTS

#	if defined(ENABLE_VISUALIZER) && defined(HAVE_FFTW3_H)
	key_section(w, "Music visualizer");
	key(w, Type::ToggleVisualizationType, "Toggle visualization type");
#	endif // ENABLE_VISUALIZER && HAVE_FFTW3_H

	mouse_section(w, "Global");
	mouse(w, "Left click on \"Playing/Paused\"", "Play/pause");
	mouse(w, "Left click on progressbar", "Jump to pointed position in playing song");
	w << '\n';
	mouse(w, "Mouse wheel on \"Volume: xx\"", "Adjust volume");
	mouse(w, "Mouse wheel on main window", "Scroll");

	mouse_section(w, "Playlist");
	mouse(w, "Left click", "Select pointed item");
	mouse(w, "Right click", "Play");

	mouse_section(w, "Browser");
	mouse(w, "Left click on directory", "Enter pointed directory");
	mouse(w, "Right click on directory", "Add pointed directory to playlist");
	w << '\n';
	mouse(w, "Left click on song/playlist", "Add pointed item to playlist");
	mouse(w, "Right click on song/playlist", "Add pointed item to playlist and play it");

	mouse_section(w, "Search engine");
	mouse(w, "Left click", "Highlight/switch value");
	mouse(w, "Right click", "Change value");

	mouse_section(w, "Media library");
	mouse_column(w, "Left/middle");
	mouse(w, "Left click", "Select pointed item", true);
	mouse(w, "Right click", "Add item to playlist", true);
	w << '\n';
	mouse_column(w, "Right");
	mouse(w, "Left Click", "Add pointed item to playlist", true);
	mouse(w, "Right Click", "Add pointed item to playlist and play it", true);

	mouse_section(w, "Playlist editor");
	mouse_column(w, "Left");
	mouse(w, "Left click", "Select pointed item", true);
	mouse(w, "Right click", "Add item to playlist", true);
	w << '\n';
	mouse_column(w, "Right");
	mouse(w, "Left click", "Add pointed item to playlist", true);
	mouse(w, "Right click", "Add pointed item to playlist and play it", true);

#	ifdef HAVE_TAGLIB_H
	mouse_section(w, "Tiny tag editor");
	mouse(w, "Left click", "Select option");
	mouse(w, "Right click", "Set value/execute");

	mouse_section(w, "Tag editor");
	mouse_column(w, "Left");
	mouse(w, "Left click", "Enter pointed directory/select pointed album", true);
	mouse(w, "Right click", "Toggle view (directories/albums)", true);
	w << '\n';
	mouse_column(w, "Middle");
	mouse(w, "Left click", "Select option", true);
	mouse(w, "Right click", "Set value/execute", true);
	w << '\n';
	mouse_column(w, "Right");
	mouse(w, "Left click", "Select pointed item", true);
	mouse(w, "Right click", "Set value", true);
#	endif // HAVE_TAGLIB_H

#	ifdef ENABLE_OUTPUTS
	mouse_section(w, "Outputs");
	mouse(w, "Left click", "Select pointed output");
	mouse(w, "Right click", "Toggle output");
#	endif // ENABLE_OUTPUTS

	section(w, "", "Action chains");
	for (const auto &k : Bindings)
	{
		for (const auto &binding : k.second)
		{
			if (!binding.isSingle())
			{
				std::vector<std::string> commands;
				for (const auto &action : binding.actions())
					commands.push_back(action->name());
				key(w, k.first, join<std::string>(commands, ", "));
			}
		}
	}

	section(w, "", "List of available colors");
	for (int i = 0; i < NC::colorCount(); ++i)
	{
		w << NC::Color(i, NC::Color::transparent) << i+1 << NC::Color::End << " ";
	}
}


}

Help *myHelp;

Help::Help()
{
    nc_help_screen_init(&m_screen,
                        makeHooks(),
                        0,
                        COLS,
                        MainStartY,
                        MainHeight,
                        NC::toNcColor(Config.main_color),
                        toNcBorder(NC::Border()),
                        static_cast<int64>(Config.lines_scrolled));
    bool loaded = nc_help_screen_reload(&m_screen);
    assert(loaded);
    (void)loaded;

    bool register_success = nc_screen_registry_register(
        &Global::myScreenRegistry, nativeScreen());
    assert(register_success);
    (void)register_success;
}

Help::~Help()
{
    if (nc_screen_registry_is_registered(&Global::myScreenRegistry,
                                         nativeScreen()))
    {
        nc_screen_registry_unregister(&Global::myScreenRegistry,
                                      nativeScreen());
    }
    nc_help_screen_destroy(&m_screen);
}

bool Help::isActiveWindow(const NC::Window &w_) const
{
    (void)w_;
    return false;
}

NC::Window *Help::activeWindow()
{
    return nullptr;
}

const NC::Window *Help::activeWindow() const
{
    return nullptr;
}

void Help::refresh()
{
    nc_screen_refresh(nativeScreen());
}

void Help::refreshWindow()
{
    nc_screen_refresh_window(nativeScreen());
}

void Help::scroll(NC::Scroll where)
{
    nc_screen_scroll(nativeScreen(), toNcScroll(where));
}

void Help::resize()
{
    nc_screen_resize(nativeScreen());
}

void Help::switchTo()
{
    nc_screen_set_has_to_be_resized(nativeScreen(), hasToBeResized);
    nc_screen_registry_switch_to(&Global::myScreenRegistry, nativeScreen());
}

int Help::windowTimeout()
{
    return nc_screen_window_timeout(nativeScreen());
}

std::string Help::title()
{
    char *result = nc_screen_title(nativeScreen());

    if (result == nullptr)
    {
        return "";
    }
    return result;
}

void Help::update()
{
    nc_screen_update(nativeScreen());
}

void Help::mouseButtonPressed(MEVENT me)
{
    nc_screen_mouse_button_pressed(nativeScreen(), me);
}

bool Help::isLockable()
{
    return nc_screen_is_lockable(nativeScreen());
}

bool Help::isMergable()
{
    return nc_screen_is_mergable(nativeScreen());
}

NcScreen *Help::nativeScreen()
{
    return nc_help_screen_base(&m_screen);
}

const NcScreen *Help::nativeScreen() const
{
    return nc_help_screen_base(const_cast<NcHelpScreen *>(&m_screen));
}

bool Help::renderBindings(NcBuffer *buffer)
{
    HelpBuffer output = {buffer};

    write_bindings(output);
    return true;
}

void Help::setGeometry(NcHelpScreen *screen)
{
    size_t x_offset;
    size_t width;

    getWindowResizeParams(x_offset, width);
    nc_help_screen_set_geometry(screen,
                                static_cast<int64>(x_offset),
                                static_cast<int64>(width),
                                MainStartY,
                                MainHeight);
    hasToBeResized = false;
}

NcHelpHooks Help::makeHooks()
{
    NcHelpHooks hooks = {};

    hooks.render = renderHook;
    hooks.switch_to = switchToHook;
    hooks.resize_layout = resizeLayoutHook;
    hooks.resize_background = resizeBackgroundHook;
    hooks.destroy = destroyHook;
    hooks.user = this;
    return hooks;
}

bool Help::renderHook(void *user, NcBuffer *buffer)
{
    return static_cast<Help *>(user)->renderBindings(buffer);
}

void Help::switchToHook(void *user)
{
    Help *help = static_cast<Help *>(user);
    using Global::myScreen;

    if (myScreen != help)
    {
        SwitchTo::finishNativeSwitch(help);
    }
    drawHeader();
}

void Help::resizeLayoutHook(void *user, NcHelpScreen *screen)
{
    static_cast<Help *>(user)->setGeometry(screen);
}

void Help::resizeBackgroundHook(void *user)
{
    (void)user;
}

void Help::destroyHook(void *user)
{
    Help *help = static_cast<Help *>(user);

    if (myHelp == help)
    {
        myHelp = nullptr;
    }
    delete help;
}
