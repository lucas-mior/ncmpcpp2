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

#ifndef NCMPCPP_SONG_INFO_H
#define NCMPCPP_SONG_INFO_H

#include "c/ncm_defs.h"
#include "c/ncm_tags.h"
#include "c/ncm_type_conversions.h"
#include "screens/native_c_screens.h"

NCM_EXTERN_C_BEGIN

typedef struct NcmSongInfoMetadata {
    char *name;
    enum NcmSongGetter get;
    enum NcmTagsField field;
} NcmSongInfoMetadata;

extern NcmSongInfoMetadata ncm_song_info_tags[];

NCM_EXTERN_C_END

#endif /* NCMPCPP_SONG_INFO_H */
