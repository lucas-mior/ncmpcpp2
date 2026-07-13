#ifndef NCMPCPP_DISPLAY_H
#define NCMPCPP_DISPLAY_H

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

#include "c/ncm_display.h"
#include "charset.h"
#include "curses/menu_impl.h"
#include "format.h"
#include "global.h"
#include "helpers_legacy.h"
#include "status.h"
#include "interfaces.h"
#include "curses/menu.h"
#include "mutable_song.h"
#include "screens/playlist.h"
#include "screens/song_info.h"
#include "screens/tag_editor.h"
#include "settings_legacy.h"
#include "song_list.h"
#include "utility/string.h"
#include "utility/utf8.h"

namespace Display {

namespace Detail {

inline const char *toColumnName(char c)
{
	switch (c)
	{
		case 'l':
			return "Time";
		case 'f':
			return "Filename";
		case 'D':
			return "Directory";
		case 'F':
			return "Filepath";
		case 'a':
			return "Artist";
		case 'A':
			return "Album Artist";
		case 't':
			return "Title";
		case 'b':
			return "Album";
		case 'y':
			return "Date";
		case 'n': case 'N':
			return "Track";
		case 'g':
			return "Genre";
		case 'c':
			return "Composer";
		case 'p':
			return "Performer";
		case 'd':
			return "Disc";
		case 'C':
			return "Comment";
		case 'P':
			return "Priority";
		default:
			return "?";
	}
}

template <typename T>
void setProperties(NC::Menu<T> &menu, const MPD::Song &s,
                   const SongList &list, bool &separate_albums,
                   bool &is_now_playing, bool &is_selected,
                   bool &is_in_playlist, bool &discard_colors)
{
	size_t drawn_pos = menu.drawn() - menu.begin();
	separate_albums = false;
	if (Config.playlist_separate_albums)
	{
		auto next = list.beginS() + drawn_pos + 1;
		if (next != list.endS())
		{
			if (next->song() != nullptr)
			{
				separate_albums = next->song()->getAlbum() != s.getAlbum()
				               || next->song()->getAlbumArtist()
				               != s.getAlbumArtist();
			}
		}
	}
	if (separate_albums)
	{
		menu << NC_FORMAT_UNDERLINE;
		mvwhline(menu.raw(), menu.getY(), 0,
		        NC_KEY_SPACE, menu.getWidth());
	}

	int song_pos = s.getPosition();
	is_now_playing = ncm_status_state_player() != NCM_STATUS_PLAYER_STOP
		&& myPlaylist->isActiveWindow(menu)
		&& song_pos == ncm_status_state_current_song_position();
	if (is_now_playing)
		menu << Config.now_playing_prefix;

	is_in_playlist = !myPlaylist->isActiveWindow(menu)
		&& myPlaylist->checkForSong(s);
	if (is_in_playlist)
		menu << NC_FORMAT_BOLD;

	is_selected = menu.drawn()->isSelected();
	discard_colors = Config.discard_colors_if_item_is_selected && is_selected;
}

template <typename T>
void unsetProperties(NC::Menu<T> &menu, bool separate_albums,
                     bool is_now_playing, bool is_in_playlist)
{
	if (is_in_playlist)
		menu << NC_FORMAT_NO_BOLD;

	if (is_now_playing)
		menu << Config.now_playing_suffix;

	if (separate_albums)
		menu << NC_FORMAT_NO_UNDERLINE;
}

template <typename T>
void showSongs(NC::Menu<T> &menu, const MPD::Song &s,
               const SongList &list, const Format::AST<char> &ast)
{
	bool separate_albums;
	bool is_now_playing;
	bool is_selected;
	bool is_in_playlist;
	bool discard_colors;
	setProperties(menu, s, list, separate_albums, is_now_playing,
	              is_selected, is_in_playlist, discard_colors);

	const size_t y = menu.getY();
	NC::Buffer right_aligned;
	Format::print(
		ast, menu, &s, &right_aligned,
		discard_colors
		? Format::Flags::Tag | Format::Flags::OutputSwitch
		: Format::Flags::All);
	if (!right_aligned.str().empty())
	{
		size_t x_off = menu.getWidth() - Utf8::width(right_aligned.str());
		if (menu.isHighlighted() && list.currentS()->song() == &s)
		{
			if (menu.highlightSuffix() == Config.current_item_suffix)
				x_off -= Config.current_item_suffix_length;
			else
				x_off -= Config.current_item_inactive_column_suffix_length;
		}
		if (is_now_playing)
			x_off -= Config.now_playing_suffix_length;
		if (is_selected)
			x_off -= Config.selected_item_suffix_length;
		menu << NC::TermManip::ClearToEOL << NC::XY(x_off, y)
		     << right_aligned;
	}

	unsetProperties(menu, separate_albums, is_now_playing, is_in_playlist);
}

template <typename T>
void showSongsInColumns(NC::Menu<T> &menu, const MPD::Song &s,
                        const SongList &list)
{
	if (Config.columns.empty())
		return;

	bool separate_albums;
	bool is_now_playing;
	bool is_selected;
	bool is_in_playlist;
	bool discard_colors;
	setProperties(menu, s, list, separate_albums, is_now_playing,
	              is_selected, is_in_playlist, discard_colors);

	int menu_width = menu.getWidth();
	if (menu.isHighlighted() && list.currentS()->song() == &s)
	{
		if (menu.highlightPrefix() == Config.current_item_prefix)
			menu_width -= Config.current_item_prefix_length;
		else
			menu_width -= Config.current_item_inactive_column_prefix_length;

		if (menu.highlightSuffix() == Config.current_item_suffix)
			menu_width -= Config.current_item_suffix_length;
		else
			menu_width -= Config.current_item_inactive_column_suffix_length;
	}
	if (is_now_playing)
	{
		menu_width -= Config.now_playing_prefix_length;
		menu_width -= Config.now_playing_suffix_length;
	}
	if (is_selected)
	{
		menu_width -= Config.selected_item_prefix_length;
		menu_width -= Config.selected_item_suffix_length;
	}

	int width;
	int y = menu.getY();
	int remained_width = menu_width;

	std::vector<LegacyColumn>::const_iterator it;
	std::vector<LegacyColumn>::const_iterator last = Config.columns.end() - 1;
	for (it = Config.columns.begin(); it != Config.columns.end(); ++it)
	{
		int x = menu.getX();
		if (it->stretch_limit >= 0)
			width = remained_width - it->stretch_limit;
		else
			width = it->fixed
			      ? it->width
			      : it->width * menu_width * 0.01;
		if (width == 0)
			continue;
		if (it != last)
			--width;

		if (remained_width-width < 0 || width < 0)
			break;

		std::string tag;
		for (size_t i = 0; i < it->type.length(); ++i)
		{
			enum NcmSongGetter getter = ncm_song_getter_from_char(it->type[i]);
			assert(getter != NCM_SONG_GETTER_NONE);
			tag = Charset::utf8ToLocale(s.getTags(getter));
			if (!tag.empty())
				break;
		}
		if (tag.empty() && it->display_empty_tag)
			tag = Config.empty_tag;
		Utf8::cut(tag, width);

		if (!discard_colors && it->color != NC::Color::Default)
			menu << it->color;

		int x_off = 0;
		if (it->right_alignment)
			x_off = std::max(0, width - int(Utf8::width(tag)));

		whline(menu.raw(), NC_KEY_SPACE, width);
		menu.goToXY(x + x_off, y);
		menu << tag;
		menu.goToXY(x + width, y);
		if (it != last)
		{
			menu << ' ';
			remained_width -= width+1;
		}

		if (!discard_colors && it->color != NC::Color::Default)
			menu << NC::Color::End;
	}

	unsetProperties(menu, separate_albums, is_now_playing, is_in_playlist);
}

}

inline std::string Columns(size_t list_width)
{
	std::string result;
	if (Config.columns.empty())
		return result;

	int width;
	int remained_width = list_width;
	std::vector<LegacyColumn>::const_iterator it;
	std::vector<LegacyColumn>::const_iterator last = Config.columns.end() - 1;
	for (it = Config.columns.begin(); it != Config.columns.end(); ++it)
	{
		if (it->stretch_limit >= 0)
			width = remained_width - it->stretch_limit;
		else
			width = it->fixed ? it->width : it->width * list_width * 0.01;
		if (width == 0)
			continue;
		if (it != last)
			--width;

		if (remained_width-width < 0 || width < 0)
			break;

		std::string name;
		if (it->name.empty())
		{
			size_t j = 0;
			while (true)
			{
				name += Detail::toColumnName(it->type[j]);
				++j;
				if (j < it->type.length())
					name += '/';
				else
					break;
			}
		}
		else
			name = it->name;
		Utf8::cut(name, width);

		int x_off = std::max(0, width - int(Utf8::width(name)));
		if (it->right_alignment)
		{
			result += std::string(x_off, NC_KEY_SPACE);
			result += Charset::utf8ToLocale(name);
		}
		else
		{
			result += Charset::utf8ToLocale(name);
			result += std::string(x_off, NC_KEY_SPACE);
		}

		if (it != last)
		{
			remained_width -= width+1;
			result += ' ';
		}
	}

	return result;
}

inline void SongsInColumns(NC::Menu<MPD::Song> &menu, const SongList &list)
{
	Detail::showSongsInColumns(menu, menu.drawn()->value(), list);
}

inline void Songs(NC::Menu<MPD::Song> &menu, const SongList &list,
                  const Format::AST<char> &ast)
{
	Detail::showSongs(menu, menu.drawn()->value(), list, ast);
}

#if HAVE_TAGLIB_H
inline void Tags(NC::Menu<MPD::MutableSong> &menu)
{
	const MPD::MutableSong &s = menu.drawn()->value();
	if (s.isModified())
		menu << Config.modified_item_prefix;
	size_t i = myTagEditor->TagTypes->choice();
	if (i < 11)
	{
		ShowTag(menu, Charset::utf8ToLocale(s.getTags(ncm_song_info_tags[i].get)));
	}
	else if (i == 12)
	{
		if (s.getNewName().empty())
			menu << Charset::utf8ToLocale(s.getName());
		else
			menu << Charset::utf8ToLocale(s.getName())
			     << Config.color2
			     << " -> "
			     << NC::FormattedColor::End<>(Config.color2)
			     << Charset::utf8ToLocale(s.getNewName());
	}
}
#endif // HAVE_TAGLIB_H

inline void Items(NC::Menu<MPD::Item> &menu, const SongList &list)
{
	const MPD::Item &item = menu.drawn()->value();
	switch (item.type())
	{
		case MPD::Item::Type::Directory:
			menu << "["
			     << Charset::utf8ToLocale(getBasename(item.directory().path()))
			     << "]";
			break;
		case MPD::Item::Type::Song:
			switch (Config.browser_display_mode)
			{
				case NCM_DISPLAY_MODE_CLASSIC:
					Detail::showSongs(menu, item.song(), list,
					                  Config.song_list_format);
					break;
				case NCM_DISPLAY_MODE_COLUMNS:
					Detail::showSongsInColumns(menu, item.song(), list);
					break;
			}
			break;
		case MPD::Item::Type::Playlist:
			menu << Config.browser_playlist_prefix
			     << Charset::utf8ToLocale(getBasename(item.playlist().path()));
			break;
	}
}

}

#endif // NCMPCPP_DISPLAY_H
