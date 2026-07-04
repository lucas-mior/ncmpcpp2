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

MPD::MutableSong::SetFunction songGetterToSetFunction(NcmSongGetter getter)
{
	switch (getter)
	{
		case NCM_SONG_GETTER_ARTIST:
			return &MPD::MutableSong::setArtist;
		case NCM_SONG_GETTER_ALBUM:
			return &MPD::MutableSong::setAlbum;
		case NCM_SONG_GETTER_ALBUM_ARTIST:
			return &MPD::MutableSong::setAlbumArtist;
		case NCM_SONG_GETTER_TITLE:
			return &MPD::MutableSong::setTitle;
		case NCM_SONG_GETTER_TRACK:
			return &MPD::MutableSong::setTrack;
		case NCM_SONG_GETTER_GENRE:
			return &MPD::MutableSong::setGenre;
		case NCM_SONG_GETTER_DATE:
			return &MPD::MutableSong::setDate;
		case NCM_SONG_GETTER_COMPOSER:
			return &MPD::MutableSong::setComposer;
		case NCM_SONG_GETTER_PERFORMER:
			return &MPD::MutableSong::setPerformer;
		case NCM_SONG_GETTER_COMMENT:
			return &MPD::MutableSong::setComment;
		case NCM_SONG_GETTER_DISC:
			return &MPD::MutableSong::setDisc;
		default:
			return nullptr;
	}
}

MPD::Song::GetFunction songGetterToGetFunction(NcmSongGetter getter)
{
	switch (getter)
	{
		case NCM_SONG_GETTER_LENGTH:
			return &MPD::Song::getLength;
		case NCM_SONG_GETTER_DIRECTORY:
			return &MPD::Song::getDirectory;
		case NCM_SONG_GETTER_NAME:
			return &MPD::Song::getName;
		case NCM_SONG_GETTER_URI:
			return &MPD::Song::getURI;
		case NCM_SONG_GETTER_ARTIST:
			return &MPD::Song::getArtist;
		case NCM_SONG_GETTER_ALBUM_ARTIST:
			return &MPD::Song::getAlbumArtist;
		case NCM_SONG_GETTER_TITLE:
			return &MPD::Song::getTitle;
		case NCM_SONG_GETTER_ALBUM:
			return &MPD::Song::getAlbum;
		case NCM_SONG_GETTER_DATE:
			return &MPD::Song::getDate;
		case NCM_SONG_GETTER_TRACK_NUMBER:
			return &MPD::Song::getTrackNumber;
		case NCM_SONG_GETTER_TRACK:
			return &MPD::Song::getTrack;
		case NCM_SONG_GETTER_GENRE:
			return &MPD::Song::getGenre;
		case NCM_SONG_GETTER_COMPOSER:
			return &MPD::Song::getComposer;
		case NCM_SONG_GETTER_PERFORMER:
			return &MPD::Song::getPerformer;
		case NCM_SONG_GETTER_DISC:
			return &MPD::Song::getDisc;
		case NCM_SONG_GETTER_COMMENT:
			return &MPD::Song::getComment;
		case NCM_SONG_GETTER_PRIORITY:
			return &MPD::Song::getPriority;
		default:
			return nullptr;
	}
}

std::optional<NcmSongGetter> getFunctionToSongGetter(MPD::Song::GetFunction f)
{
	if (f == &MPD::Song::getArtist)
		return NCM_SONG_GETTER_ARTIST;
	else if (f == &MPD::Song::getTitle)
		return NCM_SONG_GETTER_TITLE;
	else if (f == &MPD::Song::getAlbum)
		return NCM_SONG_GETTER_ALBUM;
	else if (f == &MPD::Song::getAlbumArtist)
		return NCM_SONG_GETTER_ALBUM_ARTIST;
	else if (f == &MPD::Song::getTrackNumber)
		return NCM_SONG_GETTER_TRACK_NUMBER;
	else if (f == &MPD::Song::getTrack)
		return NCM_SONG_GETTER_TRACK;
	else if (f == &MPD::Song::getDate)
		return NCM_SONG_GETTER_DATE;
	else if (f == &MPD::Song::getGenre)
		return NCM_SONG_GETTER_GENRE;
	else if (f == &MPD::Song::getComposer)
		return NCM_SONG_GETTER_COMPOSER;
	else if (f == &MPD::Song::getPerformer)
		return NCM_SONG_GETTER_PERFORMER;
	else if (f == &MPD::Song::getComment)
		return NCM_SONG_GETTER_COMMENT;
	else if (f == &MPD::Song::getDisc)
		return NCM_SONG_GETTER_DISC;
	else
		return std::nullopt;
}

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

MPD::MutableSong::SetFunction tagTypeToSetFunction(mpd_tag_type tag)
{
	return songGetterToSetFunction(ncm_song_getter_from_tag_type(tag));
}

mpd_tag_type charToTagType(char c)
{
	assert(ncm_char_is_tag_type(c));
	return ncm_char_to_tag_type(c);
}

MPD::Song::GetFunction charToGetFunction(char c)
{
	return songGetterToGetFunction(ncm_song_getter_from_char(c));
}

std::optional<mpd_tag_type> getFunctionToTagType(MPD::Song::GetFunction f)
{
	std::optional<NcmSongGetter> getter = getFunctionToSongGetter(f);

	if (!getter)
		return std::nullopt;

	mpd_tag_type tag = ncm_song_getter_to_tag_type(*getter);
	if (tag == MPD_TAG_UNKNOWN)
		return std::nullopt;

	return tag;
}

std::string itemTypeToString(MPD::Item::Type type)
{
	return ncm_item_type_name(itemTypeToNcmItemType(type));
}
