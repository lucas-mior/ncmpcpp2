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

MPD::MutableSong::SetFunction tagTypeToSetFunction(mpd_tag_type tag)
{
	switch (tag)
	{
		case MPD_TAG_ARTIST:
			return &MPD::MutableSong::setArtist;
		case MPD_TAG_ALBUM:
			return &MPD::MutableSong::setAlbum;
		case MPD_TAG_ALBUM_ARTIST:
			return &MPD::MutableSong::setAlbumArtist;
		case MPD_TAG_TITLE:
			return &MPD::MutableSong::setTitle;
		case MPD_TAG_TRACK:
			return &MPD::MutableSong::setTrack;
		case MPD_TAG_GENRE:
			return &MPD::MutableSong::setGenre;
		case MPD_TAG_DATE:
			return &MPD::MutableSong::setDate;
		case MPD_TAG_COMPOSER:
			return &MPD::MutableSong::setComposer;
		case MPD_TAG_PERFORMER:
			return &MPD::MutableSong::setPerformer;
		case MPD_TAG_COMMENT:
			return &MPD::MutableSong::setComment;
		case MPD_TAG_DISC:
			return &MPD::MutableSong::setDisc;
		default:
			return nullptr;
	}
}

mpd_tag_type charToTagType(char c)
{
	assert(ncm_char_is_tag_type(c));
	return ncm_char_to_tag_type(c);
}

MPD::Song::GetFunction charToGetFunction(char c)
{
	switch (c)
	{
		case 'l':
			return &MPD::Song::getLength;
		case 'D':
			return &MPD::Song::getDirectory;
		case 'f':
			return &MPD::Song::getName;
		case 'F':
			return &MPD::Song::getURI;
		case 'a':
			return &MPD::Song::getArtist;
		case 'A':
			return &MPD::Song::getAlbumArtist;
		case 't':
			return &MPD::Song::getTitle;
		case 'b':
			return &MPD::Song::getAlbum;
		case 'y':
			return &MPD::Song::getDate;
		case 'n':
			return &MPD::Song::getTrackNumber;
		case 'N':
			return &MPD::Song::getTrack;
		case 'g':
			return &MPD::Song::getGenre;
		case 'c':
			return &MPD::Song::getComposer;
		case 'p':
			return &MPD::Song::getPerformer;
		case 'd':
			return &MPD::Song::getDisc;
		case 'C':
			return &MPD::Song::getComment;
		case 'P':
			return &MPD::Song::getPriority;
		default:
			return nullptr;
	}
}

std::optional<mpd_tag_type> getFunctionToTagType(MPD::Song::GetFunction f)
{
	if (f == &MPD::Song::getArtist)
		return MPD_TAG_ARTIST;
	else if (f == &MPD::Song::getTitle)
		return MPD_TAG_TITLE;
	else if (f == &MPD::Song::getAlbum)
		return MPD_TAG_ALBUM;
	else if (f == &MPD::Song::getAlbumArtist)
		return MPD_TAG_ALBUM_ARTIST;
	else if (f == &MPD::Song::getTrack)
		return MPD_TAG_TRACK;
	else if (f == &MPD::Song::getDate)
		return MPD_TAG_DATE;
	else if (f == &MPD::Song::getGenre)
		return MPD_TAG_GENRE;
	else if (f == &MPD::Song::getComposer)
		return MPD_TAG_COMPOSER;
	else if (f == &MPD::Song::getPerformer)
		return MPD_TAG_PERFORMER;
	else if (f == &MPD::Song::getComment)
		return MPD_TAG_COMMENT;
	else if (f == &MPD::Song::getDisc)
		return MPD_TAG_DISC;
	else if (f == &MPD::Song::getComment)
		return MPD_TAG_COMMENT;
	else
		return std::nullopt;
}

std::string itemTypeToString(MPD::Item::Type type)
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
