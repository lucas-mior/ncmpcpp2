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

#include <utility>

#include "c/ncm_base.h"
#include "c/ncm_type_conversions.h"
#include "mutable_song.h"

namespace {

int32 stringLength(const std::string &value)
{
	return static_cast<int32>(value.size());
}

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

}

namespace MPD {

MutableSong::MutableSong()
{
	ncm_mutable_song_init(&m_mutable);
}

MutableSong::MutableSong(Song s)
: Song(std::move(s))
{
	ncm_mutable_song_init(&m_mutable);
	loadOriginals();
}

MutableSong::MutableSong(const MutableSong &rhs)
: Song(static_cast<const Song &>(rhs))
{
	ncm_mutable_song_init(&m_mutable);
	ncm_mutable_song_copy(&m_mutable,
	                      const_cast<NcmMutableSong *>(&rhs.m_mutable));
}

MutableSong::MutableSong(MutableSong &&rhs) noexcept
: Song(static_cast<Song &&>(rhs))
{
	ncm_mutable_song_init(&m_mutable);
	ncm_mutable_song_move(&m_mutable, &rhs.m_mutable);
}

MutableSong &MutableSong::operator=(const MutableSong &rhs)
{
	if (this != &rhs)
	{
		Song::operator=(static_cast<const Song &>(rhs));
		ncm_mutable_song_copy(&m_mutable,
		                      const_cast<NcmMutableSong *>(&rhs.m_mutable));
	}
	return *this;
}

MutableSong &MutableSong::operator=(MutableSong &&rhs) noexcept
{
	if (this != &rhs)
	{
		Song::operator=(static_cast<Song &&>(rhs));
		ncm_mutable_song_move(&m_mutable, &rhs.m_mutable);
	}
	return *this;
}

MutableSong::~MutableSong()
{
	ncm_mutable_song_destroy(&m_mutable);
}

std::string MutableSong::get(enum NcmSongGetter getter, unsigned idx) const
{
	enum NcmTagsField field = ncm_song_getter_to_tags_field(getter);

	if (field == NCM_TAGS_FIELD_LAST
	    || getter == NCM_SONG_GETTER_TRACK_NUMBER)
		return Song::get(getter, idx);
	return get(field, idx);
}

std::string MutableSong::get(enum NcmTagsField field, unsigned idx) const
{
	return stringFromBuffer(ncm_mutable_song_get_tag_buffer(
		const_cast<NcmMutableSong *>(&m_mutable), field,
		static_cast<int32>(idx)));
}

void MutableSong::set(enum NcmTagsField field, const std::string &value,
                      unsigned idx)
{
	if (field == NCM_TAGS_FIELD_LAST)
		return;
	ncm_mutable_song_set_tag(&m_mutable, field, static_cast<int32>(idx),
	                         const_cast<char *>(value.c_str()),
	                         stringLength(value));
}

void MutableSong::setTags(enum NcmTagsField field, const std::string &value)
{
	if (field == NCM_TAGS_FIELD_LAST)
		return;

	ncm_mutable_song_set_tags(&m_mutable, field,
	                          const_cast<char *>(value.c_str()),
	                          stringLength(value),
	                          const_cast<char *>(Song::TagsSeparator.c_str()),
	                          stringLength(Song::TagsSeparator));
}

std::string MutableSong::getArtist(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_ARTIST, idx);
}

std::string MutableSong::getTitle(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_TITLE, idx);
}

std::string MutableSong::getAlbum(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_ALBUM, idx);
}

std::string MutableSong::getAlbumArtist(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_ALBUM_ARTIST, idx);
}

std::string MutableSong::getTrack(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_TRACK, idx);
}

std::string MutableSong::getDate(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_DATE, idx);
}

std::string MutableSong::getGenre(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_GENRE, idx);
}

std::string MutableSong::getComposer(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_COMPOSER, idx);
}

std::string MutableSong::getPerformer(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_PERFORMER, idx);
}

std::string MutableSong::getDisc(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_DISC, idx);
}

std::string MutableSong::getComment(unsigned idx) const
{
	return get(NCM_TAGS_FIELD_COMMENT, idx);
}

void MutableSong::setArtist(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_ARTIST, value, idx);
}

void MutableSong::setTitle(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_TITLE, value, idx);
}

void MutableSong::setAlbum(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_ALBUM, value, idx);
}

void MutableSong::setAlbumArtist(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_ALBUM_ARTIST, value, idx);
}

void MutableSong::setTrack(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_TRACK, value, idx);
}

void MutableSong::setDate(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_DATE, value, idx);
}

void MutableSong::setGenre(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_GENRE, value, idx);
}

void MutableSong::setComposer(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_COMPOSER, value, idx);
}

void MutableSong::setPerformer(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_PERFORMER, value, idx);
}

void MutableSong::setDisc(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_DISC, value, idx);
}

void MutableSong::setComment(const std::string &value, unsigned idx)
{
	set(NCM_TAGS_FIELD_COMMENT, value, idx);
}

std::string MutableSong::getTags(enum NcmSongGetter getter) const
{
	enum NcmTagsField field = ncm_song_getter_to_tags_field(getter);

	if (field == NCM_TAGS_FIELD_LAST
	    || getter == NCM_SONG_GETTER_TRACK_NUMBER)
		return Song::getTags(getter);
	return stringFromBuffer(ncm_mutable_song_tags_buffer(
		const_cast<NcmMutableSong *>(&m_mutable), field,
		const_cast<char *>(Song::TagsSeparator.c_str()),
		static_cast<int32>(Song::TagsSeparator.size()),
		Song::ShowDuplicateTags));
}

std::string MutableSong::getNewName() const
{
	NcmStringView view;

	if (!ncm_mutable_song_get_new_name(
		    const_cast<NcmMutableSong *>(&m_mutable), &view))
		return "";
	return stringFromView(view);
}

void MutableSong::setNewName(const std::string &value)
{
	ncm_mutable_song_set_new_name(&m_mutable,
	                              const_cast<char *>(value.c_str()),
	                              stringLength(value));
}

unsigned MutableSong::getDuration() const
{
	uint32 duration;

	duration = ncm_mutable_song_duration(
		const_cast<NcmMutableSong *>(&m_mutable));
	if (duration > 0)
		return duration;
	else
		return Song::getDuration();
}

time_t MutableSong::getMTime() const
{
	int64 mtime;

	mtime = ncm_mutable_song_mtime(const_cast<NcmMutableSong *>(&m_mutable));
	if (mtime > 0)
		return static_cast<time_t>(mtime);
	else
		return Song::getMTime();
}

void MutableSong::setDuration(unsigned int duration)
{
	ncm_mutable_song_set_duration(&m_mutable, static_cast<uint32>(duration));
}

void MutableSong::setMTime(time_t mtime)
{
	ncm_mutable_song_set_mtime(&m_mutable, static_cast<int64>(mtime));
}

bool MutableSong::isModified() const
{
	return ncm_mutable_song_is_modified(
		const_cast<NcmMutableSong *>(&m_mutable));
}

void MutableSong::clearModifications()
{
	ncm_mutable_song_clear_modifications(&m_mutable);
}

NcmMutableSong *MutableSong::cMutableSong()
{
	return &m_mutable;
}

void MutableSong::loadOriginals()
{
	ncm_mutable_song_load_originals_from_song(&m_mutable, cSong());
}


}
