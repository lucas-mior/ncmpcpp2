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

std::string songArtist(const MPD::Song *song, unsigned idx)
{
	return song->MPD::Song::getArtist(idx);
}

std::string songTitle(const MPD::Song *song, unsigned idx)
{
	return song->MPD::Song::getTitle(idx);
}

std::string songAlbum(const MPD::Song *song, unsigned idx)
{
	return song->MPD::Song::getAlbum(idx);
}

std::string songAlbumArtist(const MPD::Song *song, unsigned idx)
{
	return song->MPD::Song::getAlbumArtist(idx);
}

std::string songTrack(const MPD::Song *song, unsigned idx)
{
	return song->MPD::Song::getTrack(idx);
}

std::string songDate(const MPD::Song *song, unsigned idx)
{
	return song->MPD::Song::getDate(idx);
}

std::string songGenre(const MPD::Song *song, unsigned idx)
{
	return song->MPD::Song::getGenre(idx);
}

std::string songComposer(const MPD::Song *song, unsigned idx)
{
	return song->MPD::Song::getComposer(idx);
}

std::string songPerformer(const MPD::Song *song, unsigned idx)
{
	return song->MPD::Song::getPerformer(idx);
}

std::string songDisc(const MPD::Song *song, unsigned idx)
{
	return song->MPD::Song::getDisc(idx);
}

std::string songComment(const MPD::Song *song, unsigned idx)
{
	return song->MPD::Song::getComment(idx);
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

std::string MutableSong::getArtist(unsigned idx) const
{
	return getTag(NCM_TAGS_FIELD_ARTIST, idx);
}

std::string MutableSong::getTitle(unsigned idx) const
{
	return getTag(NCM_TAGS_FIELD_TITLE, idx);
}

std::string MutableSong::getAlbum(unsigned idx) const
{
	return getTag(NCM_TAGS_FIELD_ALBUM, idx);
}

std::string MutableSong::getAlbumArtist(unsigned idx) const
{
	return getTag(NCM_TAGS_FIELD_ALBUM_ARTIST, idx);
}

std::string MutableSong::getTrack(unsigned idx) const
{
	std::string track = getTag(NCM_TAGS_FIELD_TRACK, idx);
	if ((track.length() == 1 && track[0] != '0')
	||  (track.length() > 3  && track[1] == '/'))
		return "0"+track;
	else
		return track;
}

std::string MutableSong::getDate(unsigned idx) const
{
	return getTag(NCM_TAGS_FIELD_DATE, idx);
}

std::string MutableSong::getGenre(unsigned idx) const
{
	return getTag(NCM_TAGS_FIELD_GENRE, idx);
}

std::string MutableSong::getComposer(unsigned idx) const
{
	return getTag(NCM_TAGS_FIELD_COMPOSER, idx);
}

std::string MutableSong::getPerformer(unsigned idx) const
{
	return getTag(NCM_TAGS_FIELD_PERFORMER, idx);
}

std::string MutableSong::getDisc(unsigned idx) const
{
	return getTag(NCM_TAGS_FIELD_DISC, idx);
}

std::string MutableSong::getComment(unsigned idx) const
{
	return getTag(NCM_TAGS_FIELD_COMMENT, idx);
}

void MutableSong::setArtist(const std::string &value, unsigned idx)
{
	setTag(NCM_TAGS_FIELD_ARTIST, value, idx);
}

void MutableSong::setTitle(const std::string &value, unsigned idx)
{
	setTag(NCM_TAGS_FIELD_TITLE, value, idx);
}

void MutableSong::setAlbum(const std::string &value, unsigned idx)
{
	setTag(NCM_TAGS_FIELD_ALBUM, value, idx);
}

void MutableSong::setAlbumArtist(const std::string &value, unsigned idx)
{
	setTag(NCM_TAGS_FIELD_ALBUM_ARTIST, value, idx);
}

void MutableSong::setTrack(const std::string &value, unsigned idx)
{
	setTag(NCM_TAGS_FIELD_TRACK, value, idx);
}

void MutableSong::setDate(const std::string &value, unsigned idx)
{
	setTag(NCM_TAGS_FIELD_DATE, value, idx);
}

void MutableSong::setGenre(const std::string &value, unsigned idx)
{
	setTag(NCM_TAGS_FIELD_GENRE, value, idx);
}

void MutableSong::setComposer(const std::string &value, unsigned idx)
{
	setTag(NCM_TAGS_FIELD_COMPOSER, value, idx);
}

void MutableSong::setPerformer(const std::string &value, unsigned idx)
{
	setTag(NCM_TAGS_FIELD_PERFORMER, value, idx);
}

void MutableSong::setDisc(const std::string &value, unsigned idx)
{
	setTag(NCM_TAGS_FIELD_DISC, value, idx);
}

void MutableSong::setComment(const std::string &value, unsigned idx)
{
	setTag(NCM_TAGS_FIELD_COMMENT, value, idx);
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

void MutableSong::setTags(SetFunction set, const std::string &value)
{
	enum NcmTagsField field = fieldForSetFunction(set);
	if (field == NCM_TAGS_FIELD_LAST)
		return;

	ncm_mutable_song_set_tags(&m_mutable, field,
	                          const_cast<char *>(value.c_str()),
	                          stringLength(value),
	                          const_cast<char *>(Song::TagsSeparator.c_str()),
	                          stringLength(Song::TagsSeparator));
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

std::string MutableSong::getTag(enum NcmTagsField field, unsigned idx) const
{
	NcmStringView value;

	if (!ncm_mutable_song_get_tag(const_cast<NcmMutableSong *>(&m_mutable),
	                              field, static_cast<int32>(idx), &value))
		return "";
	return stringFromView(value);
}

void MutableSong::setTag(enum NcmTagsField field, const std::string &value,
                         unsigned idx)
{
	ncm_mutable_song_set_tag(&m_mutable, field, static_cast<int32>(idx),
	                         const_cast<char *>(value.c_str()),
	                         stringLength(value));
}

void MutableSong::loadOriginals()
{
	std::string uri = Song::getURI();
	std::string directory = Song::getDirectory();
	std::string name = Song::getName();

	ncm_mutable_song_set_uri(&m_mutable, const_cast<char *>(uri.c_str()),
	                         stringLength(uri));
	ncm_mutable_song_set_directory(&m_mutable,
	                               const_cast<char *>(directory.c_str()),
	                               stringLength(directory));
	ncm_mutable_song_set_name(&m_mutable, const_cast<char *>(name.c_str()),
	                          stringLength(name));
	ncm_mutable_song_set_from_database(&m_mutable, Song::isFromDatabase());

	loadOriginalTags(NCM_TAGS_FIELD_ARTIST, songArtist);
	loadOriginalTags(NCM_TAGS_FIELD_TITLE, songTitle);
	loadOriginalTags(NCM_TAGS_FIELD_ALBUM, songAlbum);
	loadOriginalTags(NCM_TAGS_FIELD_ALBUM_ARTIST, songAlbumArtist);
	loadOriginalTags(NCM_TAGS_FIELD_TRACK, songTrack);
	loadOriginalTags(NCM_TAGS_FIELD_DATE, songDate);
	loadOriginalTags(NCM_TAGS_FIELD_GENRE, songGenre);
	loadOriginalTags(NCM_TAGS_FIELD_COMPOSER, songComposer);
	loadOriginalTags(NCM_TAGS_FIELD_PERFORMER, songPerformer);
	loadOriginalTags(NCM_TAGS_FIELD_DISC, songDisc);
	loadOriginalTags(NCM_TAGS_FIELD_COMMENT, songComment);
}

void MutableSong::loadOriginalTag(enum NcmTagsField field, unsigned idx,
                                  const std::string &value)
{
	ncm_mutable_song_set_original_tag(&m_mutable, field,
	                                  static_cast<int32>(idx),
	                                  const_cast<char *>(value.c_str()),
	                                  stringLength(value));
}

void MutableSong::loadOriginalTags(enum NcmTagsField field,
                                   std::string (*getter)(const Song *,
                                                         unsigned))
{
	for (unsigned i = 0; ; i += 1)
	{
		std::string value = getter(this, i);
		if (value.empty())
			break;
		loadOriginalTag(field, i, value);
	}
}

enum NcmTagsField MutableSong::fieldForSetFunction(SetFunction set)
{
	if (set == &MutableSong::setArtist)
		return NCM_TAGS_FIELD_ARTIST;
	if (set == &MutableSong::setTitle)
		return NCM_TAGS_FIELD_TITLE;
	if (set == &MutableSong::setAlbum)
		return NCM_TAGS_FIELD_ALBUM;
	if (set == &MutableSong::setAlbumArtist)
		return NCM_TAGS_FIELD_ALBUM_ARTIST;
	if (set == &MutableSong::setTrack)
		return NCM_TAGS_FIELD_TRACK;
	if (set == &MutableSong::setDate)
		return NCM_TAGS_FIELD_DATE;
	if (set == &MutableSong::setGenre)
		return NCM_TAGS_FIELD_GENRE;
	if (set == &MutableSong::setComposer)
		return NCM_TAGS_FIELD_COMPOSER;
	if (set == &MutableSong::setPerformer)
		return NCM_TAGS_FIELD_PERFORMER;
	if (set == &MutableSong::setDisc)
		return NCM_TAGS_FIELD_DISC;
	if (set == &MutableSong::setComment)
		return NCM_TAGS_FIELD_COMMENT;
	return NCM_TAGS_FIELD_LAST;
}

MutableSong::SetFunction setFunctionFromTagType(mpd_tag_type tag)
{
	switch (tag)
	{
		case MPD_TAG_ARTIST:
			return &MutableSong::setArtist;
		case MPD_TAG_ALBUM:
			return &MutableSong::setAlbum;
		case MPD_TAG_ALBUM_ARTIST:
			return &MutableSong::setAlbumArtist;
		case MPD_TAG_TITLE:
			return &MutableSong::setTitle;
		case MPD_TAG_TRACK:
			return &MutableSong::setTrack;
		case MPD_TAG_GENRE:
			return &MutableSong::setGenre;
		case MPD_TAG_DATE:
			return &MutableSong::setDate;
		case MPD_TAG_COMPOSER:
			return &MutableSong::setComposer;
		case MPD_TAG_PERFORMER:
			return &MutableSong::setPerformer;
		case MPD_TAG_COMMENT:
			return &MutableSong::setComment;
		case MPD_TAG_DISC:
			return &MutableSong::setDisc;
		default:
			return nullptr;
	}
}

}
