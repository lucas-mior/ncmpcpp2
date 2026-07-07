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

#include "screens/tiny_tag_editor.h"

#ifdef HAVE_TAGLIB_H

#include <cstring>
#include <string>

#include "c/ncm_taglib.h"
#include "c/ncm_type_conversions.h"

#include "curses/menu_impl.h"
#include "screens/browser.h"
#include "charset.h"
#include "display.h"
#include "helpers_legacy.h"
#include "global.h"
#include "ui_state_legacy.h"
#include "screens/song_info.h"
#include "screens/playlist.h"
#include "screens/search_engine.h"
#include "statusbar_legacy.h"
#include "screens/tag_editor.h"
#include "title_legacy.h"
#include "tags.h"
#include "screens/screen_switcher.h"
#include "screens/screen_legacy.h"
#include "utility/string.h"


TinyTagEditor *myTinyTagEditor;

namespace {

std::string channelsString(int channels)
{
	char buffer[32];
	int32 len = ncm_channels_to_string(
		static_cast<int32>(channels),
		buffer,
		static_cast<int32>(sizeof(buffer)));
	return std::string(buffer, static_cast<size_t>(len));
}

}

TinyTagEditor::TinyTagEditor()
: Screen(NC::Menu<NC::Buffer>(0, ui_state_legacy_main_start_y(), COLS, ui_state_legacy_main_height(), "", Config.main_color, NC::Border()))
{
	w.setHighlightPrefix(Config.current_item_prefix);
	w.setHighlightSuffix(Config.current_item_suffix);
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	w.setItemDisplayer([](NC::Menu<NC::Buffer> &menu) {
		menu << menu.drawn()->value();
	});
}

void TinyTagEditor::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, ui_state_legacy_main_height());
	w.moveTo(x_offset, ui_state_legacy_main_start_y());
	hasToBeResized = 0;
}

void TinyTagEditor::switchTo()
{
		if (itsEdited.isStream())
	{
		Statusbar::print("Streams can't be edited");
	}
	else if (getTags())
	{
		m_previous_screen = screenLegacyCurrent();
		SwitchTo::execute(this);
		drawHeader();
	}
	else
	{
		std::string full_path;
		if (itsEdited.isFromDatabase())
			full_path += Config.mpd_music_dir;
		full_path += itsEdited.getURI();
		
		const char msg[] = "Couldn't read file \"%1%\"";
		Statusbar::printf(msg, Utf8::shorten(full_path, COLS-const_strlen(msg)));
	}
}

std::string TinyTagEditor::title()
{
	return "Tiny tag editor";
}

void TinyTagEditor::mouseButtonPressed(MEVENT me)
{
	if (w.empty() || !w.hasCoords(me.x, me.y) || size_t(me.y) >= w.size())
		return;
	if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
	{
		if (!w.Goto(me.y))
			return;
		if (me.bstate & BUTTON3_PRESSED)
		{
			w.refresh();
			runAction();
		}
	}
	else
		Screen<WindowType>::mouseButtonPressed(me);
}

/**********************************************************************/

bool TinyTagEditor::actionRunnable()
{
	return !w.empty();
}

void TinyTagEditor::runAction()
{
	size_t option = w.choice();
	if (option < 19) // separator after comment
	{
		Statusbar::ScopedLock slock;
		size_t pos = option-8;
		Statusbar::put() << NC_FORMAT_BOLD << SongInfo::Tags[pos].Name << ": " << NC_FORMAT_NO_BOLD;
		itsEdited.setTags(SongInfo::Tags[pos].Field, ui_state_legacy_footer_window()->prompt(
			itsEdited.getTags(SongInfo::Tags[pos].Get)));
		w.at(option).value().clear();
		w.at(option).value() << NC_FORMAT_BOLD << SongInfo::Tags[pos].Name << ':' << NC_FORMAT_NO_BOLD << ' ';
		ShowTag(w.at(option).value(), itsEdited.getTags(SongInfo::Tags[pos].Get));
	}
	else if (option == 20)
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << NC_FORMAT_BOLD << "Filename: " << NC_FORMAT_NO_BOLD;
		std::string filename = itsEdited.getNewName().empty() ? itsEdited.getName() : itsEdited.getNewName();
		size_t dot = filename.rfind(".");
		std::string extension;
		if (dot != std::string::npos)
		{
			extension = filename.substr(dot);
			filename = filename.substr(0, dot);
		}
		std::string new_name = ui_state_legacy_footer_window()->prompt(filename);
		if (!new_name.empty())
		{
			itsEdited.setNewName(new_name + extension);
			w.at(option).value().clear();
			w.at(option).value() << NC_FORMAT_BOLD << "Filename:" << NC_FORMAT_NO_BOLD << ' ' << (itsEdited.getNewName().empty() ? itsEdited.getName() : itsEdited.getNewName());
		}
	}

	if (option == 22)
	{
		Statusbar::print("Updating tags...");
		if (ncm_tags_write_mutable_song(itsEdited))
		{
			Statusbar::print("Tags updated");
			if (itsEdited.isFromDatabase())
				Mpd.UpdateDirectory(itsEdited.getDirectory());
			else
			{
				if (m_previous_screen == myPlaylist)
					myPlaylist->main().current()->value() = itsEdited;
				else if (m_previous_screen == myBrowser)
					myBrowser->requestUpdate();
			}
		}
		else
			Statusbar::printf("Error while writing tags: %1%", strerror(errno));
	}
	if (option > 21)
		m_previous_screen->switchTo();
}

/**********************************************************************/

void TinyTagEditor::SetEdited(const MPD::Song &s)
{
	if (auto ms = dynamic_cast<const MPD::MutableSong *>(&s))
		itsEdited = *ms;
	else
		itsEdited = s;
}

bool TinyTagEditor::getTags()
{
	std::string path_to_file;
	if (itsEdited.isFromDatabase())
		path_to_file += Config.mpd_music_dir;
	path_to_file += itsEdited.getURI();
	
	NcmTaglibFile file;
	NcmTaglibAudioProperties properties;
	ncm_taglib_file_init(&file);
	if (!ncm_taglib_file_open(&file, const_cast<char *>(path_to_file.c_str())))
		return false;
	ncm_taglib_file_audio_properties(&file, &properties);
	
	std::string ext = itsEdited.getURI();
	ext = lowercaseAscii(ext.substr(ext.rfind(".")+1));
	
	w.clear();
	w.reset();
	
	w.resizeList(24);
	
	for (size_t i = 0; i < 7; ++i)
		w[i].setInactive(true);
	
	w[7].setSeparator(true);
	w[19].setSeparator(true);
	w[21].setSeparator(true);
	
	if (!ncm_taglib_extended_set_supported(&file))
	{
		w[10].setInactive(true);
		for (size_t i = 15; i <= 17; ++i)
			w[i].setInactive(true);
	}
	
	ncm_taglib_file_close(&file);
	w.highlight(8);

	auto print_key_value = [](NC::Buffer &buf, const char *key, const auto &value) {
		buf << NC_FORMAT_BOLD
		    << Config.color1
		    << key
		    << ":"
		    << NC::FormattedColor::End<>(Config.color1)
		    << NC_FORMAT_NO_BOLD
		    << " "
		    << Config.color2
		    << value
		    << NC::FormattedColor::End<>(Config.color2);
	};

	print_key_value(w[0].value(), "Filename", itsEdited.getName());
	print_key_value(w[1].value(), "Directory", ShowTag(itsEdited.getDirectory()));
	print_key_value(w[3].value(), "Length", itsEdited.getLength());
	print_key_value(
		w[4].value(),
		"Bitrate",
		std::to_string(properties.bitrate) + " kbps");
	print_key_value(
		w[5].value(),
		"Sample rate",
		std::to_string(properties.sample_rate) + " Hz");
	print_key_value(
		w[6].value(),
		"Channels",
		channelsString(properties.channels));
	
	unsigned pos = 8;
	for (const SongInfo::Metadata *m = SongInfo::Tags; m->Name; ++m, ++pos)
	{
		w[pos].value() << NC_FORMAT_BOLD
		                  << m->Name
		                  << ":"
		                  << NC_FORMAT_NO_BOLD
		                  << " ";
		ShowTag(w[pos].value(), itsEdited.getTags(m->Get));
	}
	
	w[20].value() << NC_FORMAT_BOLD
	                 << "Filename:"
	                 << NC_FORMAT_NO_BOLD
	                 << " "
	                 << itsEdited.getName();
	
	w[22].value() << "Save";
	w[23].value() << "Cancel";
	return true;
}

#endif // HAVE_TAGLIB_H

