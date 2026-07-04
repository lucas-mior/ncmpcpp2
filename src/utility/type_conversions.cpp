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
#include <stdexcept>
#include <string>
#include "c/ncm_type_conversions.h"
#include "utility/type_conversions.h"

namespace {

NcmItemType itemTypeToNcmItemType(MPD::Item::Type type)
{
	switch (type)
	{
		case MPD::Item::Type::Directory:
			return NCM_ITEM_DIRECTORY;
		case MPD::Item::Type::Song:
			return NCM_ITEM_SONG;
		case MPD::Item::Type::Playlist:
			return NCM_ITEM_PLAYLIST;
	}
	return NCM_ITEM_UNKNOWN;
}

}

std::string channelsToString(int channels)
{
	char buffer[32];
	int32 len = ncm_channels_to_string(
		static_cast<int32>(channels),
		buffer, static_cast<int32>(sizeof(buffer)));
	return std::string(buffer, static_cast<size_t>(len));
}

NC::Color charToColor(char c)
{
	switch (ncm_color_index_from_char(c))
	{
		case 0:
			return NC::Color::Default;
		case 1:
			return NC::Color::Black;
		case 2:
			return NC::Color::Red;
		case 3:
			return NC::Color::Green;
		case 4:
			return NC::Color::Yellow;
		case 5:
			return NC::Color::Blue;
		case 6:
			return NC::Color::Magenta;
		case 7:
			return NC::Color::Cyan;
		case 8:
			return NC::Color::White;
		case 9:
			return NC::Color::End;
		default:
			throw std::runtime_error("invalid character");
	}
}

std::string tagTypeToString(mpd_tag_type tag)
{
	return ncm_tag_type_name(tag);
}

mpd_tag_type charToTagType(char c)
{
	assert(ncm_char_is_tag_type(c));
	return ncm_char_to_tag_type(c);
}

std::string itemTypeToString(MPD::Item::Type type)
{
	return ncm_item_type_name(itemTypeToNcmItemType(type));
}
