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

#ifndef NCMPCPP_EDITABLE_SONG_H
#define NCMPCPP_EDITABLE_SONG_H

#include "config.h"
#include "c/ncm_mutable_song.h"
#include "song.h"

namespace MPD {

struct MutableSong : public Song
{
	typedef void (MutableSong::*SetFunction)(const std::string &, unsigned);
	
	MutableSong();
	MutableSong(Song s);
	MutableSong(const MutableSong &rhs);
	MutableSong(MutableSong &&rhs) noexcept;
	MutableSong &operator=(const MutableSong &rhs);
	MutableSong &operator=(MutableSong &&rhs) noexcept;
	virtual ~MutableSong() override;
	
	virtual std::string getArtist(unsigned idx = 0) const override;
	virtual std::string getTitle(unsigned idx = 0) const override;
	virtual std::string getAlbum(unsigned idx = 0) const override;
	virtual std::string getAlbumArtist(unsigned idx = 0) const override;
	virtual std::string getTrack(unsigned idx = 0) const override;
	virtual std::string getDate(unsigned idx = 0) const override;
	virtual std::string getGenre(unsigned idx = 0) const override;
	virtual std::string getComposer(unsigned idx = 0) const override;
	virtual std::string getPerformer(unsigned idx = 0) const override;
	virtual std::string getDisc(unsigned idx = 0) const override;
	virtual std::string getComment(unsigned idx = 0) const override;
	
	void setArtist(const std::string &value, unsigned idx = 0);
	void setTitle(const std::string &value, unsigned idx = 0);
	void setAlbum(const std::string &value, unsigned idx = 0);
	void setAlbumArtist(const std::string &value, unsigned idx = 0);
	void setTrack(const std::string &value, unsigned idx = 0);
	void setDate(const std::string &value, unsigned idx = 0);
	void setGenre(const std::string &value, unsigned idx = 0);
	void setComposer(const std::string &value, unsigned idx = 0);
	void setPerformer(const std::string &value, unsigned idx = 0);
	void setDisc(const std::string &value, unsigned idx = 0);
	void setComment(const std::string &value, unsigned idx = 0);
	
	std::string getNewName() const;
	void setNewName(const std::string &value);
	
	virtual unsigned getDuration() const override;
	virtual time_t getMTime() const override;
	void setDuration(unsigned duration);
	void setMTime(time_t mtime);
	
	void setTags(SetFunction set, const std::string &value);
	
	bool isModified() const;
	void clearModifications();
	
	NcmMutableSong *cMutableSong();

private:
	std::string getTag(enum NcmTagsField field, unsigned idx) const;
	void setTag(enum NcmTagsField field, const std::string &value, unsigned idx);
	void loadOriginals();
	void loadOriginalTag(enum NcmTagsField field, unsigned idx,
	                     const std::string &value);
	void loadOriginalTags(enum NcmTagsField field,
	                      std::string (*getter)(const Song *, unsigned));
	static enum NcmTagsField fieldForSetFunction(SetFunction set);
	
	NcmMutableSong m_mutable;
};

MutableSong::SetFunction setFunctionFromTagType(mpd_tag_type tag);

}

#endif // NCMPCPP_EDITABLE_SONG_H
