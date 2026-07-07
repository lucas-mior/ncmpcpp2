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

#if !defined(NCMPCPP_CONFIGURATION_H)
#define NCMPCPP_CONFIGURATION_H

#include <stdbool.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_error.h"

#if defined(__cplusplus)
extern "C" {
#endif

bool expand_home(char **path, int32 *path_len, NcmError *error);
bool configuration_discover_default_paths(NcmBufferArray *config_paths,
                                          NcmBufferArray *bindings_paths,
                                          NcmError *error);
bool configure(int32 argc, char **argv);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_CONFIGURATION_H */
