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
#include <cstring>
#include <functional>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

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

std::string formattedNumericTag(NcmStringView tag)
{
	int len = ncm_song_numeric_tag_len(tag.data, tag.len);
	std::string result(len+1, '\0');
	int written = ncm_song_format_numeric_tag(result.data(), result.size(),
	                                        tag.data, tag.len);
	result.resize(written);
	return result;
}

std::string formattedTrackNumber(NcmStringView tag)
{
	int len = ncm_song_track_number_len(tag.data, tag.len);
	std::string result(len+1, '\0');
	int written = ncm_song_format_track_number(result.data(), result.size(),
	                                          tag.data, tag.len);
	result.resize(written);
	return result;
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

const char *checked_uri(const mpd_song *s)
{
	NcmStringView uri;

	if (s == nullptr)
		throw std::bad_alloc();

	if (!ncm_mpd_song_uri_view(const_cast<mpd_song *>(s), 0, &uri))
		throw std::runtime_error("song without uri");
	return uri.data;
}

}
namespace MPD {

std::string Song::TagsSeparator = " | ";

bool Song::ShowDuplicateTags = true;

std::string Song::get(mpd_tag_type type, unsigned idx) const
{
	NcmStringView tag;

	assert(m_song);
	if (!ncm_mpd_song_tag_view(m_song.get(), type, idx, &tag))
		return "";
	return stringFromView(tag);
}

Song::Song(mpd_song *s)
{
	assert(s);
	m_song = std::shared_ptr<mpd_song>(s, mpd_song_free);
	m_hash = calc_hash(checked_uri(s));
}

Song::Song(const mpd_song *s, std::shared_ptr<mpd_entity> owner)
{
	assert(s);
	assert(owner);
	m_song = std::shared_ptr<mpd_song>(std::move(owner),
	                                  const_cast<mpd_song *>(s));
	m_hash = calc_hash(checked_uri(s));
}

std::string Song::getURI(unsigned idx) const
{
	NcmStringView uri;

	assert(m_song);
	if (!ncm_mpd_song_uri_view(m_song.get(), idx, &uri))
		return "";
	return stringFromView(uri);
}

std::string Song::getName(unsigned idx) const
{
	NcmStringView name;

	assert(m_song);
	if (!ncm_mpd_song_name_view(m_song.get(), idx, &name))
		return "";
	return stringFromView(name);
}

std::string Song::getDirectory(unsigned idx) const
{
	NcmStringView directory;

	assert(m_song);
	if (!ncm_mpd_song_directory_view(m_song.get(), idx, &directory))
		return "";
	return stringFromView(directory);
}

std::string Song::getArtist(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_ARTIST, idx);
}

std::string Song::getTitle(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_TITLE, idx);
}

std::string Song::getAlbum(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_ALBUM, idx);
}

std::string Song::getAlbumArtist(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_ALBUM_ARTIST, idx);
}

std::string Song::getTrack(unsigned idx) const
{
	NcmStringView track;

	assert(m_song);
	if (!ncm_mpd_song_tag_view(m_song.get(), MPD_TAG_TRACK, idx, &track))
		return "";
	return formattedNumericTag(track);
}

std::string Song::getTrackNumber(unsigned idx) const
{
	NcmStringView track;

	assert(m_song);
	if (!ncm_mpd_song_tag_view(m_song.get(), MPD_TAG_TRACK, idx, &track))
		return "";
	return formattedTrackNumber(track);
}

std::string Song::getDate(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_DATE, idx);
}

std::string Song::getGenre(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_GENRE, idx);
}

std::string Song::getComposer(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_COMPOSER, idx);
}

std::string Song::getPerformer(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_PERFORMER, idx);
}

std::string Song::getDisc(unsigned idx) const
{
	NcmStringView disc;

	assert(m_song);
	if (!ncm_mpd_song_tag_view(m_song.get(), MPD_TAG_DISC, idx, &disc))
		return "";
	return formattedNumericTag(disc);
}

std::string Song::getComment(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_COMMENT, idx);
}

std::string Song::getLength(unsigned idx) const
{
	assert(m_song);
	if (idx > 0)
		return "";
	unsigned len = getDuration();
	if (len > 0)
		return ShowTime(len);
	else
		return "-:--";
}

std::string Song::getPriority(unsigned idx) const
{
	assert(m_song);
	if (idx > 0)
		return "";
	return std::to_string(getPrio());
}

std::string MPD::Song::getTags(GetFunction f) const
{
	assert(m_song);
	unsigned idx = 0;
	std::string result;
	if (ShowDuplicateTags)
	{
		for (std::string tag; !(tag = (this->*f)(idx)).empty(); ++idx)
		{
			if (!result.empty())
				result += TagsSeparator;
			result += tag;
		}
	}
	else
	{
		bool already_present;
		// This is O(n^2), but it doesn't really matter as a list of tags will have
		// at most 2 or 3 items the vast majority of time.
		for (std::string tag; !(tag = (this->*f)(idx)).empty(); ++idx)
		{
			already_present = false;
			for (unsigned i = 0; i < idx; ++i)
			{
				if ((this->*f)(i) == tag)
				{
					already_present = true;
					break;
				}
			}
			if (!already_present)
			{
				if (idx > 0)
					result += TagsSeparator;
				result += tag;
			}
		}
	}
	return result;
}

unsigned Song::getDuration() const
{
	assert(m_song);
	return mpd_song_get_duration(m_song.get());
}

unsigned Song::getPosition() const
{
	assert(m_song);
	return mpd_song_get_pos(m_song.get());
}

unsigned Song::getID() const
{
	assert(m_song);
	return mpd_song_get_id(m_song.get());
}

unsigned Song::getPrio() const
{
	assert(m_song);
	return mpd_song_get_prio(m_song.get());
}

time_t Song::getMTime() const
{
	assert(m_song);
	return mpd_song_get_last_modified(m_song.get());
}

bool Song::isFromDatabase() const
{
	assert(m_song);
	return ncm_mpd_song_is_from_database(m_song.get());
}

bool Song::isStream() const
{
	assert(m_song);
	return ncm_mpd_song_is_stream(m_song.get());
}

bool Song::empty() const
{
	return m_song.get() == 0;
}

std::string Song::ShowTime(unsigned length)
{
	char buffer[32];
	int len = ncm_song_show_time(length, buffer, sizeof(buffer));

	return std::string(buffer, len);
}

}
