#ifndef NCMPCPP_TAGS_H
#define NCMPCPP_TAGS_H

#include "config.h"

#ifdef HAVE_TAGLIB_H

#include "mutable_song.h"
#include "settings_legacy.h"

inline bool ncm_tags_write_mutable_song(MPD::MutableSong &song)
{
	std::string music_dir = Config.mpd_music_dir;
	return ncm_mutable_song_write(song.cMutableSong(),
	                              const_cast<char *>(music_dir.c_str()));
}

#endif // HAVE_TAGLIB_H

#endif // NCMPCPP_TAGS_H
