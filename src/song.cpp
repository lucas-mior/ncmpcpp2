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
#include <functional>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

#include "c/ncm_base.h"
#include "c/ncm_song.h"
#include "song.h"

namespace {

std::string stringFromView(NcmStringView view)
{
	if (view.data == nullptr)
		return "";
	else
		return std::string(view.data, view.len);
}

std::string stringFromBuffer(NcmBuffer buffer)
{
	std::string result;
	if (buffer.data != nullptr)
		result.assign(buffer.data, buffer.len);
	ncm_buffer_destroy(&buffer);
	return result;
}

std::string songString(const NcmSong *song, NcmSongGetter getter,
                       unsigned idx)
{
	return stringFromBuffer(ncm_song_getter_buffer(
		const_cast<NcmSong *>(song), getter, idx));
}

MPD::Song::GetFunction functionFromGetter(NcmSongGetter getter)
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
		case NCM_SONG_GETTER_NONE:
		default:
			return nullptr;
	}
}

NcmSongGetter getterFromFunction(MPD::Song::GetFunction f)
{
	if (f == &MPD::Song::getLength)
		return NCM_SONG_GETTER_LENGTH;
	if (f == &MPD::Song::getDirectory)
		return NCM_SONG_GETTER_DIRECTORY;
	if (f == &MPD::Song::getName)
		return NCM_SONG_GETTER_NAME;
	if (f == &MPD::Song::getURI)
		return NCM_SONG_GETTER_URI;
	if (f == &MPD::Song::getArtist)
		return NCM_SONG_GETTER_ARTIST;
	if (f == &MPD::Song::getAlbumArtist)
		return NCM_SONG_GETTER_ALBUM_ARTIST;
	if (f == &MPD::Song::getTitle)
		return NCM_SONG_GETTER_TITLE;
	if (f == &MPD::Song::getAlbum)
		return NCM_SONG_GETTER_ALBUM;
	if (f == &MPD::Song::getDate)
		return NCM_SONG_GETTER_DATE;
	if (f == &MPD::Song::getTrackNumber)
		return NCM_SONG_GETTER_TRACK_NUMBER;
	if (f == &MPD::Song::getTrack)
		return NCM_SONG_GETTER_TRACK;
	if (f == &MPD::Song::getGenre)
		return NCM_SONG_GETTER_GENRE;
	if (f == &MPD::Song::getComposer)
		return NCM_SONG_GETTER_COMPOSER;
	if (f == &MPD::Song::getPerformer)
		return NCM_SONG_GETTER_PERFORMER;
	if (f == &MPD::Song::getDisc)
		return NCM_SONG_GETTER_DISC;
	if (f == &MPD::Song::getComment)
		return NCM_SONG_GETTER_COMMENT;
	if (f == &MPD::Song::getPriority)
		return NCM_SONG_GETTER_PRIORITY;
	return NCM_SONG_GETTER_NONE;
}

void hash_combine(size_t &seed, char c)
{
	seed ^= std::hash<char>{}(c) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

size_t calc_hash(const char *s, size_t seed = 0)
{
	for (; *s != '\0'; ++s)
		hash_combine(seed, *s);
	return seed;
}

void checkedLoadMpdSong(NcmSong *dest, const mpd_song *source)
{
	if (source == nullptr)
		throw std::bad_alloc();
	if (!ncm_song_from_mpd_song_copy(dest, const_cast<mpd_song *>(source)))
		throw std::runtime_error("invalid mpd song");
}

}
namespace MPD {

std::string Song::TagsSeparator = " | ";

bool Song::ShowDuplicateTags = true;

Song::Song()
: m_hash(0)
{
	ncm_song_init(&m_song);
}

Song::~Song()
{
	ncm_song_destroy(&m_song);
}

Song::Song(mpd_song *s)
: m_hash(0)
{
	ncm_song_init(&m_song);
	try
	{
		checkedLoadMpdSong(&m_song, s);
		m_hash = calc_hash(c_uri());
	}
	catch (...)
	{
		if (s != nullptr)
			mpd_song_free(s);
		throw;
	}
	mpd_song_free(s);
}

Song::Song(const mpd_song *s, std::shared_ptr<mpd_entity> owner)
: m_hash(0)
{
	(void)owner;
	ncm_song_init(&m_song);
	checkedLoadMpdSong(&m_song, s);
	m_hash = calc_hash(c_uri());
}

Song::Song(NcmSong *song)
: m_hash(0)
{
	ncm_song_init(&m_song);
	if (!ncm_song_copy(&m_song, song))
		throw std::runtime_error("invalid song");
	m_hash = calc_hash(c_uri());
}

Song::Song(const Song &rhs)
: m_hash(rhs.m_hash)
{
	ncm_song_init(&m_song);
	if (!ncm_song_copy(&m_song, const_cast<NcmSong *>(&rhs.m_song)))
		throw std::bad_alloc();
}

Song::Song(Song &&rhs) noexcept
: m_hash(rhs.m_hash)
{
	ncm_song_init(&m_song);
	ncm_song_move(&m_song, &rhs.m_song);
	rhs.m_hash = 0;
}

Song &Song::operator=(const Song &rhs)
{
	if (this != &rhs)
	{
		if (!ncm_song_copy(&m_song, const_cast<NcmSong *>(&rhs.m_song)))
			throw std::bad_alloc();
		m_hash = rhs.m_hash;
	}
	return *this;
}

Song &Song::operator=(Song &&rhs) noexcept
{
	if (this != &rhs)
	{
		ncm_song_move(&m_song, &rhs.m_song);
		m_hash = rhs.m_hash;
		rhs.m_hash = 0;
	}
	return *this;
}

std::string Song::get(mpd_tag_type type, unsigned idx) const
{
	NcmStringView tag;

	assert(!empty());
	if (!ncm_song_tag_view(const_cast<NcmSong *>(&m_song), type, idx, &tag))
		return "";
	return stringFromView(tag);
}

std::string Song::getURI(unsigned idx) const
{
	assert(!empty());
	return songString(&m_song, NCM_SONG_GETTER_URI, idx);
}

std::string Song::getName(unsigned idx) const
{
	assert(!empty());
	return songString(&m_song, NCM_SONG_GETTER_NAME, idx);
}

std::string Song::getDirectory(unsigned idx) const
{
	assert(!empty());
	return songString(&m_song, NCM_SONG_GETTER_DIRECTORY, idx);
}

std::string Song::getArtist(unsigned idx) const
{
	assert(!empty());
	return get(MPD_TAG_ARTIST, idx);
}

std::string Song::getTitle(unsigned idx) const
{
	assert(!empty());
	return get(MPD_TAG_TITLE, idx);
}

std::string Song::getAlbum(unsigned idx) const
{
	assert(!empty());
	return get(MPD_TAG_ALBUM, idx);
}

std::string Song::getAlbumArtist(unsigned idx) const
{
	assert(!empty());
	return get(MPD_TAG_ALBUM_ARTIST, idx);
}

std::string Song::getTrack(unsigned idx) const
{
	assert(!empty());
	return songString(&m_song, NCM_SONG_GETTER_TRACK, idx);
}

std::string Song::getTrackNumber(unsigned idx) const
{
	assert(!empty());
	return songString(&m_song, NCM_SONG_GETTER_TRACK_NUMBER, idx);
}

std::string Song::getDate(unsigned idx) const
{
	assert(!empty());
	return get(MPD_TAG_DATE, idx);
}

std::string Song::getGenre(unsigned idx) const
{
	assert(!empty());
	return get(MPD_TAG_GENRE, idx);
}

std::string Song::getComposer(unsigned idx) const
{
	assert(!empty());
	return get(MPD_TAG_COMPOSER, idx);
}

std::string Song::getPerformer(unsigned idx) const
{
	assert(!empty());
	return get(MPD_TAG_PERFORMER, idx);
}

std::string Song::getDisc(unsigned idx) const
{
	assert(!empty());
	return songString(&m_song, NCM_SONG_GETTER_DISC, idx);
}

std::string Song::getComment(unsigned idx) const
{
	assert(!empty());
	return get(MPD_TAG_COMMENT, idx);
}

std::string Song::getLength(unsigned idx) const
{
	assert(!empty());
	return songString(&m_song, NCM_SONG_GETTER_LENGTH, idx);
}

std::string Song::getPriority(unsigned idx) const
{
	assert(!empty());
	return songString(&m_song, NCM_SONG_GETTER_PRIORITY, idx);
}

std::string MPD::Song::getTags(GetFunction f) const
{
	assert(!empty());
	return stringFromBuffer(ncm_song_tags_buffer(
		const_cast<NcmSong *>(&m_song), getterFromFunction(f),
		const_cast<char *>(TagsSeparator.c_str()),
		static_cast<int32>(TagsSeparator.size()), ShowDuplicateTags));
}

unsigned Song::getDuration() const
{
	assert(!empty());
	return ncm_song_duration(const_cast<NcmSong *>(&m_song));
}

unsigned Song::getPosition() const
{
	assert(!empty());
	return ncm_song_position(const_cast<NcmSong *>(&m_song));
}

unsigned Song::getID() const
{
	assert(!empty());
	return ncm_song_id(const_cast<NcmSong *>(&m_song));
}

unsigned Song::getPrio() const
{
	assert(!empty());
	return ncm_song_priority(const_cast<NcmSong *>(&m_song));
}

time_t Song::getMTime() const
{
	assert(!empty());
	return ncm_song_mtime(const_cast<NcmSong *>(&m_song));
}

bool Song::isFromDatabase() const
{
	assert(!empty());
	return ncm_song_is_from_database(const_cast<NcmSong *>(&m_song));
}

bool Song::isStream() const
{
	assert(!empty());
	return ncm_song_is_stream(const_cast<NcmSong *>(&m_song));
}

bool Song::empty() const
{
	return ncm_song_empty(const_cast<NcmSong *>(&m_song));
}

std::string Song::ShowTime(unsigned length)
{
	char buffer[32];
	int len = ncm_song_show_time(length, buffer, sizeof(buffer));

	return std::string(buffer, len);
}

Song::GetFunction getFunctionFromChar(char c)
{
	return functionFromGetter(ncm_song_getter_from_char(c));
}

}
