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
#include "c/ncm_base.h"
#include "curses/formatted_color.h"
#include "app_controller.h"
#include "global.h"
#include "ui_state.h"
#include "screens/help.h"
#include "screens/screen_cpp_switcher.h"
#include "screens/screen_cpp_legacy.h"
#include "settings_legacy.h"
#include "title_legacy.h"
#include "utility/string.h"
#include "utility/string_format.h"
#include "utility/utf8.h"


namespace {

struct HelpBuffer
{
    NcBuffer *buffer;
};

NcScroll toNcScroll(enum NcScroll where)
{
    switch (where)
    {
        case NC_SCROLL_UP:
            return NC_SCROLL_UP;
        case NC_SCROLL_DOWN:
            return NC_SCROLL_DOWN;
        case NC_SCROLL_PAGE_UP:
            return NC_SCROLL_PAGE_UP;
        case NC_SCROLL_PAGE_DOWN:
            return NC_SCROLL_PAGE_DOWN;
        case NC_SCROLL_HOME:
            return NC_SCROLL_HOME;
        case NC_SCROLL_END:
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

HelpBuffer &operator<<(HelpBuffer &output, enum NcFormat format)
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


std::string key_to_string(NcKey key)
{
	char name[64];
	int32 name_len = ncm_bindings_key_name(key, name, (int32)sizeof(name));

	if (name_len < 0)
		return {};
	return std::string(name, static_cast<size_t>(name_len));
}

std::string display_keys(enum NcmActionType at)
{
	std::string result, skey;
	for (int32 i = 0; i < Bindings.keys_len; i += 1)
	{
		NcmKeyBindings *key_bindings = Bindings.keys + i;
		for (int32 j = 0; j < key_bindings->bindings_len; j += 1)
		{
			NcmBinding *binding = key_bindings->bindings + j;
			if (ncm_binding_is_single_action_type(
				binding, at))
			{
				skey = key_to_string(key_bindings->key);
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
	w << "\n  " << NC_FORMAT_BOLD;
	if (type_[0] != '\0')
		w << type_ << " - ";
	w << title_ << NC_FORMAT_NO_BOLD << "\n\n";
}

/**********************************************************************/

void key_section(HelpBuffer &w, const char *title_)
{
	section(w, "Keys", title_);
}

void key(HelpBuffer &w, enum NcmActionType at, const char *desc)
{
	w << "    " << display_keys(at) << " : " << desc << '\n';
}

void key(HelpBuffer &w, enum NcmActionType at, const std::string &desc)
{
	w << "    " << display_keys(at) << " : " << desc << '\n';
}

void key(HelpBuffer &w, NcKey k, const std::string &desc)
{
	w << "    " << align_key_rep(key_to_string(k)) << " : " << desc << '\n';
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
	w << NC_FORMAT_BOLD << "    " << column << " column:\n" << NC_FORMAT_NO_BOLD;
}

/**********************************************************************/

void write_bindings(HelpBuffer &w)
{
	key_section(w, "Movement");
	key(w, NCM_ACTION_SCROLL_UP, "Move cursor up");
	key(w, NCM_ACTION_SCROLL_DOWN, "Move cursor down");
	key(w, NCM_ACTION_SCROLL_UP_ALBUM, "Move cursor up one album");
	key(w, NCM_ACTION_SCROLL_DOWN_ALBUM, "Move cursor down one album");
	key(w, NCM_ACTION_SCROLL_UP_ARTIST, "Move cursor up one artist");
	key(w, NCM_ACTION_SCROLL_DOWN_ARTIST, "Move cursor down one artist");
	key(w, NCM_ACTION_PAGE_UP, "Page up");
	key(w, NCM_ACTION_PAGE_DOWN, "Page down");
	key(w, NCM_ACTION_MOVE_HOME, "Home");
	key(w, NCM_ACTION_MOVE_END, "End");
	w << '\n';
	if (Config.screen_switcher_previous)
	{
		key(w, NCM_ACTION_NEXT_SCREEN, "Switch between current and last screen");
		key(w, NCM_ACTION_PREVIOUS_SCREEN, "Switch between current and last screen");
	}
	else
	{
		key(w, NCM_ACTION_NEXT_SCREEN, "Switch to next screen in sequence");
		key(w, NCM_ACTION_PREVIOUS_SCREEN, "Switch to previous screen in sequence");
	}
	key(w, NCM_ACTION_SHOW_HELP, "Show help");
	key(w, NCM_ACTION_SHOW_PLAYLIST, "Show playlist");
	key(w, NCM_ACTION_SHOW_BROWSER, "Show browser");
	key(w, NCM_ACTION_SHOW_SEARCH_ENGINE, "Show search engine");
	key(w, NCM_ACTION_SHOW_MEDIA_LIBRARY, "Show media library");
	key(w, NCM_ACTION_SHOW_PLAYLIST_EDITOR, "Show playlist editor");
#	ifdef HAVE_TAGLIB_H
	key(w, NCM_ACTION_SHOW_TAG_EDITOR, "Show tag editor");
#	endif // HAVE_TAGLIB_H
#	ifdef ENABLE_OUTPUTS
	key(w, NCM_ACTION_SHOW_OUTPUTS, "Show outputs");
#	endif // ENABLE_OUTPUTS
#	ifdef ENABLE_VISUALIZER
	key(w, NCM_ACTION_SHOW_VISUALIZER, "Show music visualizer");
#	endif // ENABLE_VISUALIZER
	w << '\n';
	key(w, NCM_ACTION_SHOW_SERVER_INFO, "Show server info");

	key_section(w, "Global");
	key(w, NCM_ACTION_PLAY, "Play");
	key(w, NCM_ACTION_STOP, "Stop");
	key(w, NCM_ACTION_PAUSE, "Pause");
	key(w, NCM_ACTION_NEXT, "Next track");
	key(w, NCM_ACTION_PREVIOUS, "Previous track");
	key(w, NCM_ACTION_REPLAY_SONG, "Replay current song");
	key(w, NCM_ACTION_SEEK_FORWARD, "Seek forward in playing song");
	key(w, NCM_ACTION_SEEK_BACKWARD, "Seek backward in playing song");
	key(w, NCM_ACTION_VOLUME_DOWN,
		stringFormat("Decrease volume by %1%%%", Config.volume_change_step)
	);
	key(w, NCM_ACTION_VOLUME_UP,
		stringFormat("Increase volume by %1%%%", Config.volume_change_step)
	);
	w << '\n';
	key(w, NCM_ACTION_TOGGLE_ADD_MODE, "Toggle add mode (add or remove/always add)");
	key(w, NCM_ACTION_TOGGLE_MOUSE, "Toggle mouse support");
	key(w, NCM_ACTION_SELECT_RANGE, "Select range");
	key(w, NCM_ACTION_REVERSE_SELECTION, "Reverse selection");
	key(w, NCM_ACTION_REMOVE_SELECTION, "Remove selection");
	key(w, NCM_ACTION_SELECT_ITEM, "Select current item");
	key(w, NCM_ACTION_SELECT_FOUND_ITEMS, "Select found items");
	key(w, NCM_ACTION_SELECT_ALBUM, "Select songs of album around the cursor");
	key(w, NCM_ACTION_ADD_SELECTED_ITEMS, "Add selected items to playlist");
	key(w, NCM_ACTION_ADD_RANDOM_ITEMS, "Add random items to playlist");
	w << '\n';
	key(w, NCM_ACTION_TOGGLE_REPEAT, "Toggle repeat mode");
	key(w, NCM_ACTION_TOGGLE_RANDOM, "Toggle random mode");
	key(w, NCM_ACTION_TOGGLE_SINGLE, "Toggle single mode");
	key(w, NCM_ACTION_TOGGLE_CONSUME, "Toggle consume mode");
	key(w, NCM_ACTION_TOGGLE_REPLAY_GAIN_MODE, "Toggle replay gain mode");
	key(w, NCM_ACTION_TOGGLE_BITRATE_VISIBILITY, "Toggle bitrate visibility");
	key(w, NCM_ACTION_TOGGLE_CROSSFADE, "Toggle crossfade mode");
	key(w, NCM_ACTION_SET_CROSSFADE, "Set crossfade");
	key(w, NCM_ACTION_SET_VOLUME, "Set volume");
	key(w, NCM_ACTION_UPDATE_DATABASE, "Start music database update");
	w << '\n';
	key(w, NCM_ACTION_EXECUTE_COMMAND, "Execute command");
	key(w, NCM_ACTION_APPLY_FILTER, "Apply filter");
	key(w, NCM_ACTION_FIND_ITEM_FORWARD, "Find item forward");
	key(w, NCM_ACTION_FIND_ITEM_BACKWARD, "Find item backward");
	key(w, NCM_ACTION_PREVIOUS_FOUND_ITEM, "Jump to previous found item");
	key(w, NCM_ACTION_NEXT_FOUND_ITEM, "Jump to next found item");
	key(w, NCM_ACTION_TOGGLE_FIND_MODE, "Toggle find mode (normal/wrapped)");
	key(w, NCM_ACTION_JUMP_TO_BROWSER, "Locate song in browser");
	key(w, NCM_ACTION_JUMP_TO_MEDIA_LIBRARY, "Locate song in media library");
	key(w, NCM_ACTION_TOGGLE_SCREEN_LOCK, "Lock/unlock current screen");
	key(w, NCM_ACTION_MASTER_SCREEN, "Switch to master screen (left one)");
	key(w, NCM_ACTION_SLAVE_SCREEN, "Switch to slave screen (right one)");
#	ifdef HAVE_TAGLIB_H
	key(w, NCM_ACTION_JUMP_TO_TAG_EDITOR, "Locate song in tag editor");
#	endif // HAVE_TAGLIB_H
	key(w, NCM_ACTION_TOGGLE_DISPLAY_MODE, "Toggle display mode");
	key(w, NCM_ACTION_TOGGLE_INTERFACE, "Toggle user interface");
	key(w, NCM_ACTION_TOGGLE_SEPARATORS_BETWEEN_ALBUMS, "Toggle displaying separators between albums");
	key(w, NCM_ACTION_JUMP_TO_POSITION_IN_SONG, "Jump to given position in playing song (formats: mm:ss, x%)");
	key(w, NCM_ACTION_SHOW_SONG_INFO, "Show song info");
	key(w, NCM_ACTION_SHOW_ARTIST_INFO, "Show artist info");
	key(w, NCM_ACTION_FETCH_LYRICS_IN_BACKGROUND, "Fetch lyrics for selected songs");
	key(w, NCM_ACTION_TOGGLE_LYRICS_FETCHER, "Toggle lyrics fetcher");
	key(w, NCM_ACTION_TOGGLE_FETCHING_LYRICS_IN_BACKGROUND, "Toggle fetching lyrics for playing songs in background");
	key(w, NCM_ACTION_SHOW_LYRICS, "Show/hide song lyrics");
	w << '\n';
	key(w, NCM_ACTION_QUIT, "Quit");

	key_section(w, "Playlist");
	key(w, NCM_ACTION_PLAY_ITEM, "Play selected item");
	key(w, NCM_ACTION_DELETE_PLAYLIST_ITEMS, "Delete selected item(s) from playlist");
	key(w, NCM_ACTION_CLEAR_MAIN_PLAYLIST, "Clear playlist");
	key(w, NCM_ACTION_CROP_MAIN_PLAYLIST, "Clear playlist except selected item(s)");
	key(w, NCM_ACTION_SET_SELECTED_ITEMS_PRIORITY, "Set priority of selected items");
	key(w, NCM_ACTION_MOVE_SELECTED_ITEMS_UP, "Move selected item(s) up");
	key(w, NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN, "Move selected item(s) down");
	key(w, NCM_ACTION_MOVE_SELECTED_ITEMS_TO, "Move selected item(s) to cursor position");
	key(w, NCM_ACTION_ADD, "Add item to playlist");
	key(w, NCM_ACTION_LOAD, "Load stored playlist");
#	ifdef HAVE_TAGLIB_H
	key(w, NCM_ACTION_EDIT_SONG, "Edit song");
#	endif // HAVE_TAGLIB_H
	key(w, NCM_ACTION_SAVE_PLAYLIST, "Save playlist");
	key(w, NCM_ACTION_SHUFFLE, "Shuffle range");
	key(w, NCM_ACTION_SORT_PLAYLIST, "Sort range");
	key(w, NCM_ACTION_REVERSE_PLAYLIST, "Reverse range");
	key(w, NCM_ACTION_JUMP_TO_PLAYING_SONG, "Jump to current song");
	key(w, NCM_ACTION_TOGGLE_PLAYING_SONG_CENTERING, "Toggle playing song centering");

	key_section(w, "Browser");
	key(w, NCM_ACTION_ENTER_DIRECTORY, "Enter directory");
	key(w, NCM_ACTION_PLAY_ITEM, "Add item to playlist and play it");
	key(w, NCM_ACTION_ADD_ITEM_TO_PLAYLIST, "Add item to playlist");
#	ifdef HAVE_TAGLIB_H
	key(w, NCM_ACTION_EDIT_SONG, "Edit song");
#	endif // HAVE_TAGLIB_H
	key(w, NCM_ACTION_EDIT_DIRECTORY_NAME, "Edit directory name");
	key(w, NCM_ACTION_EDIT_PLAYLIST_NAME, "Edit playlist name");
	key(w, NCM_ACTION_CHANGE_BROWSE_MODE, "Browse MPD database/local filesystem");
	key(w, NCM_ACTION_TOGGLE_BROWSER_SORT_MODE, "Toggle sort mode");
	key(w, NCM_ACTION_JUMP_TO_PLAYING_SONG, "Locate current song");
	key(w, NCM_ACTION_JUMP_TO_PARENT_DIRECTORY, "Jump to parent directory");
	key(w, NCM_ACTION_DELETE_BROWSER_ITEMS, "Delete selected items from disk");
	key(w, NCM_ACTION_JUMP_TO_PLAYLIST_EDITOR, "Jump to playlist editor (playlists only)");

	key_section(w, "Search engine");
	key(w, NCM_ACTION_RUN_ACTION, "Modify option / Run action");
	key(w, NCM_ACTION_ADD_ITEM_TO_PLAYLIST, "Add item to playlist");
	key(w, NCM_ACTION_PLAY_ITEM, "Add item to playlist and play it");
#	ifdef HAVE_TAGLIB_H
	key(w, NCM_ACTION_EDIT_SONG, "Edit song");
#	endif // HAVE_TAGLIB_H
	key(w, NCM_ACTION_START_SEARCHING, "Start searching");
	key(w, NCM_ACTION_RESET_SEARCH_ENGINE, "Reset search constraints and clear results");

	key_section(w, "Media library");
	key(w, NCM_ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE, "Switch between two/three columns mode");
	key(w, NCM_ACTION_PREVIOUS_COLUMN, "Previous column");
	key(w, NCM_ACTION_NEXT_COLUMN, "Next column");
	key(w, NCM_ACTION_PLAY_ITEM, "Add item to playlist and play it");
	key(w, NCM_ACTION_ADD_ITEM_TO_PLAYLIST, "Add item to playlist");
	key(w, NCM_ACTION_JUMP_TO_PLAYING_SONG, "Locate current song");
#	ifdef HAVE_TAGLIB_H
	key(w, NCM_ACTION_EDIT_SONG, "Edit song");
#	endif // HAVE_TAGLIB_H
	key(w, NCM_ACTION_EDIT_LIBRARY_TAG, "Edit tag (left column)/album (middle/right column)");
	key(w, NCM_ACTION_TOGGLE_LIBRARY_TAG_TYPE, "Toggle type of tag used in left column");
	key(w, NCM_ACTION_TOGGLE_MEDIA_LIBRARY_SORT_MODE, "Toggle sort mode");

	key_section(w, "Playlist editor");
	key(w, NCM_ACTION_PREVIOUS_COLUMN, "Previous column");
	key(w, NCM_ACTION_NEXT_COLUMN, "Next column");
	key(w, NCM_ACTION_PLAY_ITEM, "Add item to playlist and play it");
	key(w, NCM_ACTION_ADD_ITEM_TO_PLAYLIST, "Add item to playlist");
	key(w, NCM_ACTION_JUMP_TO_PLAYING_SONG, "Locate current song");
#	ifdef HAVE_TAGLIB_H
	key(w, NCM_ACTION_EDIT_SONG, "Edit song");
#	endif // HAVE_TAGLIB_H
	key(w, NCM_ACTION_EDIT_PLAYLIST_NAME, "Edit playlist name");
	key(w, NCM_ACTION_MOVE_SELECTED_ITEMS_UP, "Move selected item(s) up");
	key(w, NCM_ACTION_MOVE_SELECTED_ITEMS_DOWN, "Move selected item(s) down");
	key(w, NCM_ACTION_DELETE_STORED_PLAYLIST, "Delete selected playlists (left column)");
	key(w, NCM_ACTION_DELETE_PLAYLIST_ITEMS, "Delete selected item(s) from playlist (right column)");
	key(w, NCM_ACTION_CLEAR_PLAYLIST, "Clear playlist");
	key(w, NCM_ACTION_CROP_PLAYLIST, "Clear playlist except selected items");

	key_section(w, "Lyrics");
	key(w, NCM_ACTION_TOGGLE_LYRICS_UPDATE_ON_SONG_CHANGE, "Toggle lyrics update on song change");
	key(w, NCM_ACTION_EDIT_LYRICS, "Open lyrics in external editor");
	key(w, NCM_ACTION_REFETCH_LYRICS, "Refetch lyrics");

#	ifdef HAVE_TAGLIB_H
	key_section(w, "Tiny tag editor");
	key(w, NCM_ACTION_RUN_ACTION, "Edit tag / Run action");
	key(w, NCM_ACTION_SAVE_TAG_CHANGES, "Save");

	key_section(w, "Tag editor");
	key(w, NCM_ACTION_ENTER_DIRECTORY, "Enter directory (right column)");
	key(w, NCM_ACTION_RUN_ACTION, "Perform operation on selected items (middle column)");
	key(w, NCM_ACTION_RUN_ACTION, "Edit item (left column)");
	key(w, NCM_ACTION_PREVIOUS_COLUMN, "Previous column");
	key(w, NCM_ACTION_NEXT_COLUMN, "Next column");
	key(w, NCM_ACTION_JUMP_TO_PARENT_DIRECTORY, "Jump to parent directory (left column, directories view)");
#	endif // HAVE_TAGLIB_H

#	ifdef ENABLE_OUTPUTS
	key_section(w, "Outputs");
	key(w, NCM_ACTION_TOGGLE_OUTPUT, "Toggle output");
#	endif // ENABLE_OUTPUTS

#	if defined(ENABLE_VISUALIZER) && defined(HAVE_FFTW3_H)
	key_section(w, "Music visualizer");
	key(w, NCM_ACTION_TOGGLE_VISUALIZATION_TYPE, "Toggle visualization type");
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
	for (int32 i = 0; i < Bindings.keys_len; i += 1)
	{
		NcmKeyBindings *key_bindings = Bindings.keys + i;
		for (int32 j = 0; j < key_bindings->bindings_len; j += 1)
		{
			NcmBinding *binding = key_bindings->bindings + j;
			if (!ncm_binding_is_single(binding))
			{
				std::vector<std::string> commands;
				for (int32 k = 0; k < binding->actions_len; k += 1)
				{
					NcmBuffer command;

					ncm_buffer_init(&command);
					ncm_binding_action_format(&command, binding->actions + k);
					commands.emplace_back(command.data,
					                      static_cast<size_t>(command.len));
					ncm_buffer_destroy(&command);
				}
				key(w, key_bindings->key, join<std::string>(commands, ", "));
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
                        static_cast<size_t>(ui_state_main_start_y()),
                        static_cast<size_t>(ui_state_main_height()),
                        NC::toNcColor(Config.main_color),
                        toNcBorder(NC::Border()),
                        static_cast<int64>(Config.lines_scrolled));
    bool loaded = nc_help_screen_reload(&m_screen);
    assert(loaded);
    (void)loaded;

    bool register_success = app_controller_register_screen(nativeScreen());
    assert(register_success);
    (void)register_success;
}

Help::~Help()
{
    if (app_controller_is_screen_registered(nativeScreen()))
    {
        app_controller_unregister_screen(nativeScreen());
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

void Help::scroll(enum NcScroll where)
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
    app_controller_switch_to_screen(nativeScreen());
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
                                static_cast<size_t>(ui_state_main_start_y()),
                                static_cast<size_t>(ui_state_main_height()));
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
    
    if (screenLegacySwitchChanged())
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
