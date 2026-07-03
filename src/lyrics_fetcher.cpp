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

#include "config.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <regex>

#ifdef HAVE_TAGLIB_H
#include "c/ncm_taglib.h"
#endif // HAVE_TAGLIB_H

#include "charset.h"
#include "curl_handle.h"
#include "lyrics_fetcher.h"
#include "settings.h"
#include "utility/html.h"
#include "utility/string.h"

namespace {

void replaceAll(std::string &s, const std::string &from, const std::string &to)
{
	for (size_t pos = 0; (pos = s.find(from, pos)) != std::string::npos; pos += to.size())
		s.replace(pos, from.size(), to);
}

void trim(std::string &s)
{
	auto first = s.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
	{
		s.clear();
		return;
	}
	auto last = s.find_last_not_of(" \t\r\n");
	s = s.substr(first, last-first+1);
}

std::vector<std::string> splitLines(const std::string &s)
{
	std::vector<std::string> result;
	std::string current;
	for (char c : s)
	{
		if (c == '\r' || c == '\n')
		{
			result.push_back(std::move(current));
			current.clear();
		}
		else
			current += c;
	}
	result.push_back(std::move(current));
	return result;
}

std::string joinLines(const std::vector<std::string> &lines)
{
	std::string result;
	for (auto it = lines.begin(); it != lines.end(); ++it)
	{
		if (it != lines.begin())
			result += "\n";
		result += *it;
	}
	return result;
}

#ifdef HAVE_TAGLIB_H
void appendLyricsValue(char *value, void *user)
{
	auto result = static_cast<std::string *>(user);
	if (!result->empty())
		*result += "\n\n";
	*result += value;
}

std::string readLyricsFromTags(NcmTaglibFile &file)
{
	std::string result;
	ncm_taglib_read_property(&file, const_cast<char *>("LYRICS"), appendLyricsValue, &result);
	if (result.empty())
		ncm_taglib_read_property(&file, const_cast<char *>("UNSYNCEDLYRICS"), appendLyricsValue, &result);
	return result;
}
#endif // HAVE_TAGLIB_H

}

std::istream &operator>>(std::istream &is, LyricsFetcher_ &fetcher)
{
	std::string s;
	is >> s;
	if (s == "justsomelyrics")
		fetcher = std::make_unique<JustSomeLyricsFetcher>();
	else if (s == "jahlyrics")
		fetcher = std::make_unique<JahLyricsFetcher>();
	else if (s == "plyrics")
		fetcher = std::make_unique<PLyricsFetcher>();
	else if (s == "tekstowo")
		fetcher = std::make_unique<TekstowoFetcher>();
	else if (s == "zeneszoveg")
		fetcher = std::make_unique<ZeneszovegFetcher>();
	else if (s == "internet")
		fetcher = std::make_unique<InternetLyricsFetcher>();
#ifdef HAVE_TAGLIB_H
	else if (s == "tags")
		fetcher = std::make_unique<TagsLyricsFetcher>();
#endif // HAVE_TAGLIB_H
	else
		is.setstate(std::ios::failbit);
	return is;
}

const char LyricsFetcher::msgNotFound[] = "Not found";

LyricsFetcher::Result LyricsFetcher::fetch(const std::string &artist,
                                           const std::string &title,
                                           [[maybe_unused]] const MPD::Song &song)
{
	Result result;
	result.first = false;
	
	std::string url = urlTemplate();
	replaceAll(url, "%artist%", Curl::escape(artist));
	replaceAll(url, "%title%", Curl::escape(title));

	std::string data;
	CURLcode code = Curl::perform(data, url, "", true);
	
	if (code != CURLE_OK)
	{
		result.second = curl_easy_strerror(code);
		return result;
	}

	auto lyrics = getContent(regex(), data);

	//std::cerr << "URL: " << url << "\n";
	//std::cerr << "Data: " << data << "\n";

	if (lyrics.empty() || notLyrics(data))
	{
		//std::cerr << "Empty: " << lyrics.empty() << "\n";
		//std::cerr << "Not Lyrics: " << notLyrics(data) << "\n";
		result.second = msgNotFound;
		return result;
	}

	data.clear();
	for (auto it = lyrics.begin(); it != lyrics.end(); ++it)
	{
		postProcess(*it);
		if (!it->empty())
		{
			data += *it;
			if (it != lyrics.end() - 1)
				data += "\n\n";
		}
	}
	
	result.second = data;
	result.first = true;
	return result;
}

std::vector<std::string> LyricsFetcher::getContent(const char *regex_,
                                                   const std::string &data)
{
	std::vector<std::string> result;
	std::regex rx(regex_);
	auto first = std::sregex_iterator(data.begin(), data.end(), rx);
	auto last = std::sregex_iterator();
	for (; first != last; ++first)
	{
		std::string content;
		for (size_t i = 1; i < first->size(); ++i)
			content += first->str(i);
		result.push_back(std::move(content));
	}
	return result;
}

void LyricsFetcher::postProcess(std::string &data) const
{
	data = unescapeHtmlUtf8(data);
	stripHtmlTags(data);
	// Remove indentation from each line and collapse multiple newlines into one.
	auto lines = splitLines(data);
	for (auto &line : lines)
		trim(line);
	auto last = std::unique(
		lines.begin(),
		lines.end(),
		[](std::string &a, std::string &b) { return a.empty() && b.empty(); });
	lines.erase(last, lines.end());
	data = joinLines(lines);
	trim(data);
}

/**********************************************************************/

LyricsFetcher::Result GoogleLyricsFetcher::fetch(const std::string &artist,
                                                 const std::string &title,
                                                 const MPD::Song &song)
{
	Result result;
	result.first = false;
	
	std::string search_str;
	if (siteKeyword() != nullptr)
	{
		search_str += "site:";
		search_str += Curl::escape(siteKeyword());
	}
	else
		search_str = "lyrics";
	search_str += "+";
	search_str += Curl::escape(artist);
	search_str += "+";
	search_str += Curl::escape(title);
	
	std::string google_url = "http://www.google.com/search?hl=en&ie=UTF-8&oe=UTF-8&q=";
	google_url += search_str;
	google_url += "&btnI=I%27m+Feeling+Lucky";
	
	std::string data;
	CURLcode code = Curl::perform(data, google_url, google_url);
	
	if (code != CURLE_OK)
	{
		result.second = curl_easy_strerror(code);
		return result;
	}

	auto urls = getContent("<A HREF=\"http://www.google.com/url\\?q=(.*?)\">here</A>", data);

	if (urls.empty() || !isURLOk(urls[0]))
	{
		result.second = msgNotFound;
		return result;
	}

	data = unescapeHtmlUtf8(urls[0]);

	URL = data.c_str();
	return LyricsFetcher::fetch("", "", song);
}

bool GoogleLyricsFetcher::isURLOk(const std::string &url)
{
	return url.find(siteKeyword()) != std::string::npos;
}

/**********************************************************************/

LyricsFetcher::Result InternetLyricsFetcher::fetch(const std::string &artist,
                                                   const std::string &title,
                                                   const MPD::Song &song)
{
	URL.clear();
	LyricsFetcher::Result google_result = GoogleLyricsFetcher::fetch(artist, title, song);
	LyricsFetcher::Result result;
	result.first = false;
	if (!URL.empty())
	{
		result.second = "The following site may contain lyrics for this song: ";
		result.second += URL;
	}
	else
		result.second = google_result.second;
	return result;
}

bool InternetLyricsFetcher::isURLOk(const std::string &url)
{
	URL = url;
	return false;
}

#ifdef HAVE_TAGLIB_H
LyricsFetcher::Result TagsLyricsFetcher::fetch([[maybe_unused]] const std::string &artist,
                                               [[maybe_unused]] const std::string &title,
                                               const MPD::Song &song)
{
	LyricsFetcher::Result result;
	result.first = false;

	std::string path;
	if (song.isFromDatabase())
		path += Config.mpd_music_dir;
	path += song.getURI();

	NcmTaglibFile file;
	ncm_taglib_file_init(&file);
	if (!ncm_taglib_file_open(&file, const_cast<char *>(path.c_str())))
	{
		result.second = "Could not open file";
		return result;
	}
	result.second = readLyricsFromTags(file);
	ncm_taglib_file_close(&file);

	if (!result.second.empty())
	{
		result.first = true;
	}
	else
		result.second = "No lyrics in tags";

	return result;
}
#endif // HAVE_TAGLIB_H
