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

#include <filesystem>
#include <string>
#include <vector>

#include "c/ncm_taglib.h"
#include "global.h"
#include "settings.h"

namespace {

void setAttributeCallback(char *name, char *value, void *user)
{
	Tags::setAttribute(static_cast<mpd_song *>(user), name, value);
}

void appendStringCallback(char *value, void *user)
{
	auto strings = static_cast<std::vector<std::string> *>(user);
	strings->emplace_back(value);
}

std::vector<std::string> readProperty(NcmTaglibFile &file, char *property)
{
	std::vector<std::string> values;
	ncm_taglib_read_property(&file, property, appendStringCallback, &values);
	return values;
}

std::string readFirstProperty(NcmTaglibFile &file, char *property)
{
	auto values = readProperty(file, property);
	if (values.empty())
		return std::string();
	return values.front();
}

std::string joinProperty(NcmTaglibFile &file, char *property,
                         const std::string &separator)
{
	auto values = readProperty(file, property);
	std::string result;
	for (size_t i = 0; i < values.size(); ++i)
	{
		if (i != 0)
			result += separator;
		result += values[i];
	}
	return result;
}

void clearAlias(NcmTaglibFile &file, char *property)
{
	ncm_taglib_clear_property(&file, property);
}

void writeProperty(NcmTaglibFile &file, char *property,
                   const MPD::MutableSong &s, MPD::Song::GetFunction f)
{
	ncm_taglib_clear_property(&file, property);
	for (unsigned idx = 0; ; ++idx)
	{
		auto value = (s.*f)(idx);
		if (value.empty())
			break;
		ncm_taglib_append_property(&file, property,
		                           const_cast<char *>(value.c_str()));
	}
}

void writeTags(const MPD::MutableSong &s, NcmTaglibFile &file)
{
	clearAlias(file, const_cast<char *>("ALBUM ARTIST"));
	clearAlias(file, const_cast<char *>("TRACK"));
	clearAlias(file, const_cast<char *>("DISC"));
	clearAlias(file, const_cast<char *>("DESCRIPTION"));

	writeProperty(file, const_cast<char *>("TITLE"), s, &MPD::Song::getTitle);
	writeProperty(file, const_cast<char *>("ARTIST"), s, &MPD::Song::getArtist);
	writeProperty(file, const_cast<char *>("ALBUMARTIST"), s,
	              &MPD::Song::getAlbumArtist);
	writeProperty(file, const_cast<char *>("ALBUM"), s, &MPD::Song::getAlbum);
	writeProperty(file, const_cast<char *>("DATE"), s, &MPD::Song::getDate);
	writeProperty(file, const_cast<char *>("TRACKNUMBER"), s,
	              &MPD::Song::getTrack);
	writeProperty(file, const_cast<char *>("GENRE"), s, &MPD::Song::getGenre);
	writeProperty(file, const_cast<char *>("COMPOSER"), s,
	              &MPD::Song::getComposer);
	writeProperty(file, const_cast<char *>("PERFORMER"), s,
	              &MPD::Song::getPerformer);
	writeProperty(file, const_cast<char *>("DISCNUMBER"), s,
	              &MPD::Song::getDisc);
	writeProperty(file, const_cast<char *>("COMMENT"), s,
	              &MPD::Song::getComment);
}

}

namespace Tags {

void setAttribute(mpd_song *s, const char *name, const std::string &value)
{
	mpd_pair pair = { name, value.c_str() };
	mpd_song_feed(s, &pair);
}

bool extendedSetSupported(const char *path)
{
	NcmTaglibFile file;
	bool result;

	ncm_taglib_file_init(&file);
	if (!ncm_taglib_file_open(&file, const_cast<char *>(path)))
		return false;

	result = ncm_taglib_extended_set_supported(&file);
	ncm_taglib_file_close(&file);
	return result;
}

ReplayGainInfo readReplayGain(const char *path)
{
	NcmTaglibFile file;
	ReplayGainInfo result;

	ncm_taglib_file_init(&file);
	if (!ncm_taglib_file_open(&file, const_cast<char *>(path)))
		return result;

	result = ReplayGainInfo(
		readFirstProperty(file, const_cast<char *>("REPLAYGAIN_REFERENCE_LOUDNESS")),
		readFirstProperty(file, const_cast<char *>("REPLAYGAIN_TRACK_GAIN")),
		readFirstProperty(file, const_cast<char *>("REPLAYGAIN_TRACK_PEAK")),
		readFirstProperty(file, const_cast<char *>("REPLAYGAIN_ALBUM_GAIN")),
		readFirstProperty(file, const_cast<char *>("REPLAYGAIN_ALBUM_PEAK"))
	);
	ncm_taglib_file_close(&file);
	return result;
}

std::string readLyrics(const char *path)
{
	NcmTaglibFile file;
	std::string result;

	ncm_taglib_file_init(&file);
	if (!ncm_taglib_file_open(&file, const_cast<char *>(path)))
		return result;

	result = joinProperty(file, const_cast<char *>("LYRICS"), "\n\n");
	if (result.empty())
		result = joinProperty(file, const_cast<char *>("UNSYNCEDLYRICS"), "\n\n");

	ncm_taglib_file_close(&file);
	return result;
}

void read(mpd_song *s)
{
	NcmTaglibFile file;
	NcmTaglibAudioProperties properties;

	ncm_taglib_file_init(&file);
	if (!ncm_taglib_file_open(&file,
	                          const_cast<char *>(mpd_song_get_uri(s))))
		return;

	if (ncm_taglib_file_audio_properties(&file, &properties))
		setAttribute(s, "Time", std::to_string(properties.length));

	ncm_taglib_read_mapped_properties(&file, setAttributeCallback, s);
	ncm_taglib_file_close(&file);
	ncm_taglib_clear_strings();
}

bool write(MPD::MutableSong &s)
{
	NcmTaglibFile file;
	std::string old_name;
	bool saved;

	if (s.isFromDatabase())
		old_name += Config.mpd_music_dir;
	old_name += s.getURI();

	ncm_taglib_file_init(&file);
	if (!ncm_taglib_file_open(&file, const_cast<char *>(old_name.c_str())))
		return false;

	writeTags(s, file);
	saved = ncm_taglib_file_save(&file);
	ncm_taglib_file_close(&file);
	if (!saved)
		return false;

	// TODO: move this somewhere else
	if (!s.getNewName().empty())
	{
		std::string new_name;
		if (s.isFromDatabase())
			new_name += Config.mpd_music_dir;
		new_name += s.getDirectory();
		new_name += "/";
		new_name += s.getNewName();
		std::filesystem::rename(old_name, new_name);
	}
	return true;
}

}

#endif // HAVE_TAGLIB_H
