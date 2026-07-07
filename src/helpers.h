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

#if !defined(NCMPCPP_HELPERS_H)
#define NCMPCPP_HELPERS_H

#include <stdbool.h>
#include <time.h>

#include "c/ncm_base.h"
#include "c/ncm_enums.h"
#include "c/ncm_mpd_connection.h"
#include "c/ncm_song_list.h"
#include "curses/nc_menu.h"

NCM_EXTERN_C_BEGIN

enum NcmReapplyFilter {
    NCM_REAPPLY_FILTER_YES,
    NCM_REAPPLY_FILTER_NO,
};

typedef bool (*NcmSongListIterFunc)(NcmSongListItem *item, int32 idx,
                                    void *user);
typedef bool (*NcmMpdSongListIterFunc)(NcmSong *song, int32 idx,
                                       void *user);
typedef bool (*NcmMenuIterFunc)(void *item, int64 pos, void *user);

char *ncm_helpers_with_errors(bool success);
int32 ncm_helpers_show_song_time(uint32 length, char *buffer,
                                 int32 buffer_cap);
bool ncm_helpers_time_format(NcmBuffer *buffer, char *format,
                             time_t value);
bool ncm_helpers_timestamp(NcmBuffer *buffer, time_t value);

bool ncm_song_list_each_item(NcmSongList *list, NcmSongListIterFunc func,
                             void *user);
bool ncm_song_list_each_item_reverse(NcmSongList *list,
                                     NcmSongListIterFunc func, void *user);
bool ncm_song_list_each_selected(NcmSongList *list,
                                 NcmSongListIterFunc func, void *user);
bool ncm_song_list_each_selected_or_current(NcmSongList *list,
                                            NcmSongListIterFunc func,
                                            void *user);
bool ncm_song_list_has_selected_item(NcmSongList *list);
void ncm_song_list_select_current_if_none_selected(NcmSongList *list);
void ncm_song_list_reverse_selection(NcmSongList *list);
bool ncm_song_list_find_selected_range(NcmSongList *list, int32 *first,
                                       int32 *last);
bool ncm_song_list_find_full_selected_range(NcmSongList *list,
                                            int32 *first, int32 *last);
int32 ncm_song_list_wrapped_search(NcmSongList *list, int32 current,
                                   enum SearchDirection direction,
                                   bool wrap, bool skip_current,
                                   NcmSongListIterFunc predicate,
                                   void *user);

bool ncm_mpd_song_list_each_song(NcmMpdSongList *list,
                                 NcmMpdSongListIterFunc func, void *user);
bool ncm_mpd_song_list_each_song_reverse(NcmMpdSongList *list,
                                         NcmMpdSongListIterFunc func,
                                         void *user);
int32 ncm_mpd_song_list_wrapped_search(NcmMpdSongList *list,
                                       int32 current,
                                       enum SearchDirection direction,
                                       bool wrap, bool skip_current,
                                       NcmMpdSongListIterFunc predicate,
                                       void *user);

bool ncm_menu_each_item(NcMenu *menu, enum NcMenuItemSource source,
                        NcmMenuIterFunc func, void *user);
bool ncm_menu_each_item_reverse(NcMenu *menu, enum NcMenuItemSource source,
                                NcmMenuIterFunc func, void *user);
bool ncm_menu_each_selected(NcMenu *menu, enum NcMenuItemSource source,
                            NcmMenuIterFunc func, void *user);
bool ncm_menu_each_selected_or_current(NcMenu *menu,
                                       enum NcMenuItemSource source,
                                       NcmMenuIterFunc func, void *user);
bool ncm_menu_has_selected_item(NcMenu *menu, enum NcMenuItemSource source);
void ncm_menu_select_current_if_none_selected(NcMenu *menu);
void ncm_menu_reverse_selection(NcMenu *menu, enum NcMenuItemSource source);
bool ncm_menu_find_selected_range(NcMenu *menu, enum NcMenuItemSource source,
                                  int64 *first, int64 *last);
bool ncm_menu_find_full_selected_range(NcMenu *menu,
                                       enum NcMenuItemSource source,
                                       int64 *first, int64 *last);
int64 ncm_menu_wrapped_search(NcMenu *menu, enum NcMenuItemSource source,
                              int64 current,
                              enum SearchDirection direction, bool wrap,
                              bool skip_current, NcmMenuIterFunc predicate,
                              void *user);

NCM_EXTERN_C_END

#endif /* NCMPCPP_HELPERS_H */
