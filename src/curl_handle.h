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

#if !defined(NCMPCPP_CURL_HANDLE_H)
#define NCMPCPP_CURL_HANDLE_H

#include "config.h"

#include <stdbool.h>

#include <curl/curl.h>

#include "c/ncm_defs.h"

NCM_EXTERN_C_BEGIN

typedef struct NcmCurlResponseWriter {
    NcmBuffer *buffer;
} NcmCurlResponseWriter;

void ncm_curl_response_writer_init(NcmCurlResponseWriter *writer,
                                   NcmBuffer *buffer);
void ncm_curl_response_writer_destroy(NcmCurlResponseWriter *writer);

CURLcode ncm_curl_perform(NcmBuffer *data, char *url, int32 url_len,
                          char *referer, int32 referer_len,
                          bool follow_redirect,
                          int32 timeout_seconds);
CURLcode ncm_curl_escape(NcmBuffer *out, char *string, int32 string_len);

NCM_EXTERN_C_END

#endif /* NCMPCPP_CURL_HANDLE_H */
