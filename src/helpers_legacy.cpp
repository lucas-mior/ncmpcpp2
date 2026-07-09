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

#include "actions.h"
#include "helpers_legacy.h"
#include "screens/playlist.h"

const MPD::Song *currentSong(const BaseScreen *screen)
{
	const MPD::Song *ptr = nullptr;
	const auto *list = dynamic_cast<const SongList *>(screen->activeWindow());
	if (list != nullptr)
	{
		const auto it = list->currentS();
		if (it != list->endS())
			ptr = it->song();
	}
	return ptr;
}

void deleteSelectedSongsFromPlaylist(NC::Menu<MPD::Song> &playlist)
{
	selectCurrentIfNoneSelected(playlist);
	bool in_range = false;
	int range_end = 0;
	Mpd.StartCommandsList();
	for (auto it = playlist.rbegin(); it != playlist.rend(); ++it)
	{
		auto &s = *it;
		if (s.isSelected())
		{
			s.setSelected(false);
			if (!in_range)
			{
				range_end = s.value().getPosition() + 1;
				in_range = true;
			}
		}
		else if (in_range)
		{
			Mpd.DeleteRange(s.value().getPosition() + 1, range_end);
			in_range = false;
		}
	}
	if (in_range)
		Mpd.DeleteRange(0, range_end);
	Mpd.CommitCommandsList();
}

bool addSongToPlaylist(const MPD::Song &s, bool play, int position)
{
	return ncm_action_add_song_to_playlist_with_mode(
		s.cSong(), play, position, Config.space_add_mode);
}

