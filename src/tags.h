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

#ifndef NCMPCPP_TAGS_H
#define NCMPCPP_TAGS_H

#include "config.h"

#ifdef HAVE_TAGLIB_H

#include "mutable_song.h"
#include "settings.h"

inline bool ncm_tags_write_mutable_song(MPD::MutableSong &song)
{
	std::string music_dir = Config.mpd_music_dir;
	return ncm_mutable_song_write(song.cMutableSong(),
	                              const_cast<char *>(music_dir.c_str()));
}

#endif // HAVE_TAGLIB_H

#endif // NCMPCPP_TAGS_H
