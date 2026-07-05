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

#include <cassert>
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

NC::Scroll to_cpp_scroll(enum NcScroll where)
{
	switch (where)
	{
		case NC_SCROLL_UP:
			return NC::Scroll::Up;
		case NC_SCROLL_DOWN:
			return NC::Scroll::Down;
		case NC_SCROLL_PAGE_UP:
			return NC::Scroll::PageUp;
		case NC_SCROLL_PAGE_DOWN:
			return NC::Scroll::PageDown;
		case NC_SCROLL_HOME:
			return NC::Scroll::Home;
		case NC_SCROLL_END:
			return NC::Scroll::End;
	}
	return NC::Scroll::Up;
}

enum NcScroll to_nc_scroll(NC::Scroll where)
{
	switch (where)
	{
		case NC::Scroll::Up:
			return NC_SCROLL_UP;
		case NC::Scroll::Down:
			return NC_SCROLL_DOWN;
		case NC::Scroll::PageUp:
			return NC_SCROLL_PAGE_UP;
		case NC::Scroll::PageDown:
			return NC_SCROLL_PAGE_DOWN;
		case NC::Scroll::Home:
			return NC_SCROLL_HOME;
		case NC::Scroll::End:
			return NC_SCROLL_END;
	}
	return NC_SCROLL_UP;
}

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
	NcScreenCallbacks callbacks = makeCallbacks();

	nc_song_info_screen_init(&m_screen,
	                         callbacks,
	                         this,
	                         0,
	                         COLS,
	                         MainStartY,
	                         MainHeight);

	bool register_success = nc_screen_registry_register(
		&Global::myScreenRegistry, nativeScreen());
	assert(register_success);
	(void)register_success;
}

SongInfo::~SongInfo()
{
	if (nc_screen_registry_is_registered(&Global::myScreenRegistry,
	                                     nativeScreen()))
	{
		nc_screen_registry_unregister(&Global::myScreenRegistry,
		                              nativeScreen());
	}
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
	nc_screen_refresh(nativeScreen());
}

void SongInfo::refreshWindow()
{
	nc_screen_refresh_window(nativeScreen());
}

void SongInfo::scroll(NC::Scroll where)
{
	nc_screen_scroll(nativeScreen(), to_nc_scroll(where));
}

void SongInfo::resize()
{
	nc_screen_resize(nativeScreen());
}

int SongInfo::windowTimeout()
{
	return nc_screen_window_timeout(nativeScreen());
}

std::string SongInfo::title()
{
	char *result = nc_screen_title(nativeScreen());
	if (result == nullptr)
		return "";
	return result;
}

void SongInfo::switchTo()
{
	using Global::myScreen;

	if ((myScreen != this) && !currentSong(myScreen))
		return;
	if (myScreen == this)
	{
		nc_screen_switch_to(nativeScreen());
		return;
	}
	nc_screen_set_has_to_be_resized(nativeScreen(), hasToBeResized);
	nc_screen_registry_switch_to(&Global::myScreenRegistry, nativeScreen());
}

void SongInfo::mouseButtonPressed(MEVENT me)
{
	nc_screen_mouse_button_pressed(nativeScreen(), me);
}

void SongInfo::update()
{
	nc_screen_update(nativeScreen());
}

bool SongInfo::isLockable()
{
	return nc_screen_is_lockable(nativeScreen());
}

bool SongInfo::isMergable()
{
	return nc_screen_is_mergable(nativeScreen());
}

NcScreen *SongInfo::nativeScreen()
{
	return nc_song_info_screen_base(&m_screen);
}

const NcScreen *SongInfo::nativeScreen() const
{
	return nc_song_info_screen_base(const_cast<NcSongInfoScreen *>(&m_screen));
}

NcScreenCallbacks SongInfo::makeCallbacks()
{
	NcScreenCallbacks callbacks = {0};

	callbacks.active_window = activeWindowCallback;
	callbacks.refresh = refreshCallback;
	callbacks.refresh_window = refreshWindowCallback;
	callbacks.scroll = scrollCallback;
	callbacks.switch_to = switchToCallback;
	callbacks.resize = resizeCallback;
	callbacks.window_timeout = windowTimeoutCallback;
	callbacks.title = titleCallback;
	callbacks.update = updateCallback;
	callbacks.mouse_button_pressed = mouseButtonPressedCallback;
	callbacks.is_lockable = isLockableCallback;
	callbacks.is_mergable = isMergableCallback;
	callbacks.destroy = destroyCallback;
	return callbacks;
}

SongInfo *SongInfo::fromScreen(NcScreen *screen)
{
	return static_cast<SongInfo *>(nc_screen_user(screen));
}

NcWindow *SongInfo::activeWindowCallback(NcScreen *screen)
{
	return fromScreen(screen)->w.nativeWindow();
}

void SongInfo::refreshCallback(NcScreen *screen)
{
	fromScreen(screen)->w.display();
}

void SongInfo::refreshWindowCallback(NcScreen *screen)
{
	fromScreen(screen)->w.display();
}

void SongInfo::scrollCallback(NcScreen *screen, enum NcScroll where)
{
	fromScreen(screen)->w.scroll(to_cpp_scroll(where));
}

void SongInfo::switchToCallback(NcScreen *screen)
{
	SongInfo *song_info = fromScreen(screen);
	using Global::myScreen;

	if (myScreen != song_info)
	{
		auto s = currentSong(myScreen);
		if (!s)
			return;
		SwitchTo::finishNativeSwitch(song_info);
		song_info->w.clear();
		song_info->w.reset();
		song_info->PrepareSong(*s);
		song_info->w.flush();
		// redraw header after we're done with the file, since reading it from disk
		// takes a bit of time and having header updated before content of a window
		// is displayed doesn't look nice.
		drawHeader();
	}
	else
		song_info->switchToPreviousScreen();
}

void SongInfo::resizeCallback(NcScreen *screen)
{
	SongInfo *song_info = fromScreen(screen);
	size_t x_offset;
	size_t width;

	song_info->getWindowResizeParams(x_offset, width);
	nc_song_info_screen_set_geometry(&song_info->m_screen,
	                                 static_cast<int64>(x_offset),
	                                 static_cast<int64>(width),
	                                 MainStartY,
	                                 MainHeight);
	song_info->w.resize(nc_song_info_screen_width(&song_info->m_screen),
	                    nc_song_info_screen_height(&song_info->m_screen));
	song_info->w.moveTo(nc_song_info_screen_start_x(&song_info->m_screen),
	                    nc_song_info_screen_start_y(&song_info->m_screen));
	song_info->hasToBeResized = false;
}

int32 SongInfo::windowTimeoutCallback(NcScreen *screen)
{
	(void)screen;
	return defaultWindowTimeout;
}

char *SongInfo::titleCallback(NcScreen *screen)
{
	static char title[] = "Song info";

	(void)screen;
	return title;
}

void SongInfo::updateCallback(NcScreen *screen)
{
	(void)screen;
}

void SongInfo::mouseButtonPressedCallback(NcScreen *screen, MEVENT event)
{
	scrollpadMouseButtonPressed(fromScreen(screen)->w, event);
}

bool SongInfo::isLockableCallback(NcScreen *screen)
{
	(void)screen;
	return false;
}

bool SongInfo::isMergableCallback(NcScreen *screen)
{
	(void)screen;
	return true;
}

void SongInfo::destroyCallback(NcScreen *screen)
{
	SongInfo *song_info = fromScreen(screen);

	if (mySongInfo == song_info)
		mySongInfo = nullptr;
	delete song_info;
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
