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

#include "tags.h"

#ifdef HAVE_TAGLIB_H

#include <string>

#include "c/ncm_tags.h"
#include "settings.h"

namespace {

struct MutableSongTagsContext
{
	MPD::MutableSong *song;
	std::string value;
};

bool getMutableSongTagValue(enum NcmTagsField field, int32 idx,
                            NcmStringView *value, void *user)
{
	auto context = static_cast<MutableSongTagsContext *>(user);
	if (context == nullptr || value == nullptr)
		return false;

	auto index = static_cast<unsigned>(idx);
	switch (field)
	{
	case NCM_TAGS_FIELD_TITLE:
		context->value = context->song->getTitle(index);
		break;
	case NCM_TAGS_FIELD_ARTIST:
		context->value = context->song->getArtist(index);
		break;
	case NCM_TAGS_FIELD_ALBUM_ARTIST:
		context->value = context->song->getAlbumArtist(index);
		break;
	case NCM_TAGS_FIELD_ALBUM:
		context->value = context->song->getAlbum(index);
		break;
	case NCM_TAGS_FIELD_DATE:
		context->value = context->song->getDate(index);
		break;
	case NCM_TAGS_FIELD_TRACK:
		context->value = context->song->getTrack(index);
		break;
	case NCM_TAGS_FIELD_GENRE:
		context->value = context->song->getGenre(index);
		break;
	case NCM_TAGS_FIELD_COMPOSER:
		context->value = context->song->getComposer(index);
		break;
	case NCM_TAGS_FIELD_PERFORMER:
		context->value = context->song->getPerformer(index);
		break;
	case NCM_TAGS_FIELD_DISC:
		context->value = context->song->getDisc(index);
		break;
	case NCM_TAGS_FIELD_COMMENT:
		context->value = context->song->getComment(index);
		break;
	case NCM_TAGS_FIELD_LAST:
		return false;
	}

	value->data = const_cast<char *>(context->value.c_str());
	value->len = static_cast<int32>(context->value.size());
	return true;
}

}

bool ncm_tags_write_mutable_song(MPD::MutableSong &song)
{
	MutableSongTagsContext context;
	std::string music_dir = Config.mpd_music_dir;
	std::string uri = song.getURI();
	std::string directory = song.getDirectory();
	std::string new_name = song.getNewName();

	context.song = &song;
	return ncm_tags_write(
		const_cast<char *>(music_dir.c_str()),
		const_cast<char *>(uri.c_str()),
		song.isFromDatabase(),
		const_cast<char *>(directory.c_str()),
		const_cast<char *>(new_name.c_str()),
		getMutableSongTagValue,
		&context);
}

#endif // HAVE_TAGLIB_H
