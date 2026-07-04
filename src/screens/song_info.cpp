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

#include "global.h"
#include "helpers.h"
#include "screens/song_info.h"
#include "screens/tag_editor.h"
#include "title.h"
#include "screens/screen_switcher.h"

#include <string>

#include "c/ncm_type_conversions.h"

#ifdef HAVE_TAGLIB_H
# include "c/ncm_taglib.h"
# include "c/ncm_tags.h"
#endif // HAVE_TAGLIB_H

using Global::MainHeight;
using Global::MainStartY;

SongInfo *mySongInfo;

namespace {

std::string channelsString(int channels)
{
	char buffer[32];
	int32 len = ncm_channels_to_string(
		static_cast<int32>(channels),
		buffer,
		static_cast<int32>(sizeof(buffer)));
	return std::string(buffer, static_cast<size_t>(len));
}

}

const SongInfo::Metadata SongInfo::Tags[] =
{
 { "Title",        NCM_SONG_GETTER_TITLE,        NCM_TAGS_FIELD_TITLE        },
 { "Artist",       NCM_SONG_GETTER_ARTIST,       NCM_TAGS_FIELD_ARTIST       },
 { "Album Artist", NCM_SONG_GETTER_ALBUM_ARTIST, NCM_TAGS_FIELD_ALBUM_ARTIST },
 { "Album",        NCM_SONG_GETTER_ALBUM,        NCM_TAGS_FIELD_ALBUM        },
 { "Date",         NCM_SONG_GETTER_DATE,         NCM_TAGS_FIELD_DATE         },
 { "Track",        NCM_SONG_GETTER_TRACK,        NCM_TAGS_FIELD_TRACK        },
 { "Genre",        NCM_SONG_GETTER_GENRE,        NCM_TAGS_FIELD_GENRE        },
 { "Composer",     NCM_SONG_GETTER_COMPOSER,     NCM_TAGS_FIELD_COMPOSER     },
 { "Performer",    NCM_SONG_GETTER_PERFORMER,    NCM_TAGS_FIELD_PERFORMER    },
 { "Disc",         NCM_SONG_GETTER_DISC,         NCM_TAGS_FIELD_DISC         },
 { "Comment",      NCM_SONG_GETTER_COMMENT,      NCM_TAGS_FIELD_COMMENT      },
 { 0,              NCM_SONG_GETTER_NONE,         NCM_TAGS_FIELD_LAST         }
};

SongInfo::SongInfo()
: w(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::Border())
{
	NcScreenCallbacks callbacks = {0};

	nc_song_info_screen_init(&m_screen,
	                         callbacks,
	                         this,
	                         0,
	                         COLS,
	                         MainStartY,
	                         MainHeight);
}

bool SongInfo::isActiveWindow(const NC::Window &w_) const
{
	return &w == &w_;
}

NC::Window *SongInfo::activeWindow()
{
	return &w;
}

const NC::Window *SongInfo::activeWindow() const
{
	return &w;
}

void SongInfo::refresh()
{
	w.display();
}

void SongInfo::refreshWindow()
{
	w.display();
}

void SongInfo::scroll(NC::Scroll where)
{
	w.scroll(where);
}

void SongInfo::resize()
{
	size_t x_offset;
	size_t width;

	getWindowResizeParams(x_offset, width);
	nc_song_info_screen_set_geometry(&m_screen,
	                                 static_cast<int64>(x_offset),
	                                 static_cast<int64>(width),
	                                 MainStartY,
	                                 MainHeight);
	w.resize(nc_song_info_screen_width(&m_screen),
	         nc_song_info_screen_height(&m_screen));
	w.moveTo(nc_song_info_screen_start_x(&m_screen),
	         nc_song_info_screen_start_y(&m_screen));
	nc_screen_set_has_to_be_resized(nc_song_info_screen_base(&m_screen), false);
	hasToBeResized = false;
}

int SongInfo::windowTimeout()
{
	return defaultWindowTimeout;
}

std::string SongInfo::title()
{
	return "Song info";
}

void SongInfo::switchTo()
{
	using Global::myScreen;
	if (myScreen != this)
	{
		auto s = currentSong(myScreen);
		if (!s)
			return;
		SwitchTo::execute(this);
		w.clear();
		w.reset();
		PrepareSong(*s);
		w.flush();
		// redraw header after we're done with the file, since reading it from disk
		// takes a bit of time and having header updated before content of a window
		// is displayed doesn't look nice.
		drawHeader();
	}
	else
		switchToPreviousScreen();
}

void SongInfo::mouseButtonPressed(MEVENT me)
{
	scrollpadMouseButtonPressed(w, me);
}

void SongInfo::PrepareSong(const MPD::Song &s)
{
	auto print_key_value = [this](const char *key, const auto &value) {
		w << NC::Format::Bold
		  << Config.color1
		  << key
		  << ":"
		  << NC::FormattedColor::End<>(Config.color1)
		  << NC::Format::NoBold
		  << " "
		  << Config.color2
		  << value
		  << NC::FormattedColor::End<>(Config.color2)
		  << "\n";
	};

	print_key_value("Filename", s.getName());
	print_key_value("Directory", ShowTag(s.getDirectory()));
	w << "\n";
	print_key_value("Length", s.getLength());

#	ifdef HAVE_TAGLIB_H
	if (!Config.mpd_music_dir.empty() && !s.isStream())
	{
		std::string path_to_file;
		if (s.isFromDatabase())
			path_to_file += Config.mpd_music_dir;
		path_to_file += s.getURI();
		NcmTaglibFile file;
		NcmTaglibAudioProperties properties;

		ncm_taglib_file_init(&file);
		if (ncm_taglib_file_open(&file, const_cast<char *>(path_to_file.c_str())))
		{
			if (ncm_taglib_file_audio_properties(&file, &properties))
			{
				print_key_value(
					"Bitrate",
					std::to_string(properties.bitrate) + " kbps");
				print_key_value(
					"Sample rate",
					std::to_string(properties.sample_rate) + " Hz");
				print_key_value("Channels",
				                channelsString(properties.channels));
			}

			NcmTagsReplayGainInfo rginfo;
			ncm_tags_replay_gain_info_init(&rginfo);
			if (ncm_tags_read_replay_gain(
					const_cast<char *>(path_to_file.c_str()),
					&rginfo)
					&& !ncm_tags_replay_gain_info_empty(&rginfo))
			{
				auto view_string = [](NcmStringView view) {
					return view.data != nullptr ? view.data : "";
				};
				w << "\n";
				print_key_value("Reference loudness",
				                view_string(rginfo.reference_loudness));
				print_key_value("Track gain",
				                view_string(rginfo.track_gain));
				print_key_value("Track peak",
				                view_string(rginfo.track_peak));
				print_key_value("Album gain",
				                view_string(rginfo.album_gain));
				print_key_value("Album peak",
				                view_string(rginfo.album_peak));
			}
			ncm_tags_replay_gain_info_destroy(&rginfo);
			ncm_taglib_file_close(&file);
		}
	}
#	endif // HAVE_TAGLIB_H
	w << NC::Color::Default;
	
	for (const Metadata *m = Tags; m->Name; ++m)
	{
		w << NC::Format::Bold << "\n" << m->Name << ":" << NC::Format::NoBold << " ";
		ShowTag(w, s.getTags(m->Get));
	}
}
