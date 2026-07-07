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

#if !defined(NCMPCPP_LASTFM_SERVICE_H)
#define NCMPCPP_LASTFM_SERVICE_H

#include "config.h"

#include <stdbool.h>

#include "c/ncm_defs.h"

NCM_EXTERN_C_BEGIN

enum NcmLastfmServiceType {
    NCM_LASTFM_SERVICE_NONE,
    NCM_LASTFM_SERVICE_ARTIST_INFO,
};

typedef struct NcmLastfmResult {
    char *text;
    int32 text_len;
    int32 text_cap;
    bool success;
} NcmLastfmResult;

typedef struct NcmLastfmService {
    char *artist;
    char *lang;

    int32 artist_len;
    int32 artist_cap;
    int32 lang_len;
    int32 lang_cap;

    enum NcmLastfmServiceType type;
} NcmLastfmService;

void ncm_lastfm_result_init(NcmLastfmResult *result);
void ncm_lastfm_result_destroy(NcmLastfmResult *result);
void ncm_lastfm_result_clear(NcmLastfmResult *result);
bool ncm_lastfm_result_set(NcmLastfmResult *result, bool success,
                           char *text, int32 text_len);

void ncm_lastfm_service_init(NcmLastfmService *service);
void ncm_lastfm_service_destroy(NcmLastfmService *service);
bool ncm_lastfm_artist_info_init(NcmLastfmService *service,
                                 char *artist,
                                 int32 artist_len,
                                 char *lang,
                                 int32 lang_len);
bool ncm_lastfm_service_equal(NcmLastfmService *left,
                              NcmLastfmService *right);
char *ncm_lastfm_service_name(NcmLastfmService *service);
enum NcmLastfmServiceType ncm_lastfm_service_type(
    NcmLastfmService *service);
bool ncm_lastfm_service_fetch(NcmLastfmService *service,
                              NcmLastfmResult *result);

NCM_EXTERN_C_END

#endif /* NCMPCPP_LASTFM_SERVICE_H */
