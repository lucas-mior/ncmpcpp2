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

typedef struct NcmConfigurationOptions {
    NcmBuffer host;
    NcmBuffer current_song_format;
    NcmBuffer screen_name;
    NcmBuffer slave_screen_name;
    NcmBufferArray config_paths;
    NcmBufferArray bindings_paths;

    uint32 port;

    bool host_provided;
    bool port_provided;
    bool current_song;
    bool ignore_config_errors;
    bool test_lyrics_fetchers;
    bool screen;
    bool slave_screen;
    bool help;
    bool version;
    bool quiet;
} NcmConfigurationOptions;

void ncm_configuration_options_init(NcmConfigurationOptions *options);
void ncm_configuration_options_destroy(NcmConfigurationOptions *options);
bool ncm_configuration_options_parse(NcmConfigurationOptions *options,
                                     int32 argc, char **argv,
                                     NcmError *error);
bool ncm_configuration_options_apply(NcmConfigurationOptions *options,
                                     NcmError *error);

bool expand_home(char **path, int32 *path_len, NcmError *error);
bool configuration_discover_default_paths(NcmBufferArray *config_paths,
                                          NcmBufferArray *bindings_paths,
                                          NcmError *error);
bool configure(int32 argc, char **argv);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_CONFIGURATION_H */
