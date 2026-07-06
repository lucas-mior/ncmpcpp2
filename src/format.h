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

#ifndef NCMPCPP_HAVE_FORMAT_H
#define NCMPCPP_HAVE_FORMAT_H

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "c/ncm_format.h"
#include "curses/formatted_color.h"
#include "curses/menu.h"
#include "curses/strbuffer.h"
#include "song.h"

namespace Format {

namespace Flags {
const unsigned None = NCM_FORMAT_FLAG_NONE;
const unsigned Color = NCM_FORMAT_FLAG_COLOR;
const unsigned Format = NCM_FORMAT_FLAG_FORMAT;
const unsigned OutputSwitch = NCM_FORMAT_FLAG_OUTPUT_SWITCH;
const unsigned Tag = NCM_FORMAT_FLAG_TAG;
const unsigned All = NCM_FORMAT_FLAG_ALL;
}

struct OutputSwitch { };

struct SongTag
{
	SongTag(enum NcmSongGetter getter_, unsigned delimiter_ = 0)
	: m_getter(getter_), m_delimiter(delimiter_)
	{ }

	enum NcmSongGetter getter() const { return m_getter; }
	unsigned delimiter() const { return m_delimiter; }

private:
	enum NcmSongGetter m_getter;
	unsigned m_delimiter;
};

inline bool operator==(const SongTag &lhs, const SongTag &rhs)
{
	return lhs.getter() == rhs.getter()
	    && lhs.delimiter() == rhs.delimiter();
}

inline bool operator!=(const SongTag &lhs, const SongTag &rhs)
{
	return !(lhs == rhs);
}

template <typename CharT>
using TagVector = std::vector<
	std::pair<
		std::optional<SongTag>,
		std::basic_string<CharT>
		>
	>;

enum class Result { Empty, Missing, Ok };

template <typename CharT>
class AST
{
public:
	AST()
	{
		ncm_format_ast_init(&m_ast);
	}

	explicit AST(NcmFormatAst *ast)
	{
		ncm_format_ast_init(&m_ast);
		ncm_format_ast_move(&m_ast, ast);
	}

	AST(const AST &rhs)
	{
		ncm_format_ast_init(&m_ast);
		if (!ncm_format_ast_copy(&m_ast,
		                         const_cast<NcmFormatAst *>(rhs.cAst())))
			throw std::bad_alloc();
	}

	AST(AST &&rhs) noexcept
	{
		ncm_format_ast_init(&m_ast);
		ncm_format_ast_move(&m_ast, rhs.cAst());
	}

	AST &operator=(const AST &rhs)
	{
		if (this != &rhs)
		{
			if (!ncm_format_ast_copy(&m_ast,
			                         const_cast<NcmFormatAst *>(rhs.cAst())))
				throw std::bad_alloc();
		}
		return *this;
	}

	AST &operator=(AST &&rhs) noexcept
	{
		if (this != &rhs)
			ncm_format_ast_move(&m_ast, rhs.cAst());
		return *this;
	}

	~AST()
	{
		ncm_format_ast_destroy(&m_ast);
	}

	NcmFormatAst *cAst()
	{
		return &m_ast;
	}

	const NcmFormatAst *cAst() const
	{
		return &m_ast;
	}

private:
	NcmFormatAst m_ast;
};

namespace Detail {

inline std::string takeBuffer(NcmBuffer &buffer)
{
	std::string result;
	if (buffer.data != nullptr)
		result.assign(buffer.data, static_cast<size_t>(buffer.len));
	ncm_buffer_destroy(&buffer);
	return result;
}

struct OutputTarget
{
	void (*text)(OutputTarget *, char *, int32);
	void (*color)(OutputTarget *, NcColor);
	void (*format)(OutputTarget *, enum NcFormat);
};

template <typename StreamT>
struct StreamTarget : OutputTarget
{
	explicit StreamTarget(StreamT &stream_)
	: stream(&stream_)
	{
		text = appendText;
		color = appendColor;
		format = appendFormat;
	}

	static void appendText(OutputTarget *target, char *data, int32 data_len)
	{
		auto self = static_cast<StreamTarget *>(target);
		*self->stream << std::string(data, static_cast<size_t>(data_len));
	}

	static void appendColor(OutputTarget *target, NcColor color)
	{
		auto self = static_cast<StreamTarget *>(target);
		*self->stream << NC::fromNcColor(color);
	}

	static void appendFormat(OutputTarget *target, enum NcFormat format)
	{
		auto self = static_cast<StreamTarget *>(target);
		*self->stream << NC::fromNcFormat(format);
	}

	StreamT *stream;
};

inline void targetText(void *user, char *data, int32 data_len,
                       NcmFormatSongTag *tag)
{
	(void)tag;
	auto target = static_cast<OutputTarget *>(user);
	if (target != nullptr)
		target->text(target, data, data_len);
}

inline void targetColor(void *user, NcColor color)
{
	auto target = static_cast<OutputTarget *>(user);
	if (target != nullptr)
		target->color(target, color);
}

inline void targetFormat(void *user, enum NcFormat format)
{
	auto target = static_cast<OutputTarget *>(user);
	if (target != nullptr)
		target->format(target, format);
}

inline NcmSong *songImpl(const MPD::Song *song)
{
	if (song == nullptr)
		return nullptr;
	return const_cast<MPD::Song *>(song)->cSong();
}

}

template <typename CharT, typename VisitorT>
void visit(VisitorT &, const AST<CharT> &)
{
}

template <typename CharT, typename ItemT>
void print(const AST<CharT> &ast, NC::Menu<ItemT> &menu,
           const MPD::Song *song, NC::BasicBuffer<CharT> *buffer,
           const unsigned flags = Flags::All)
{
	Detail::StreamTarget<NC::Menu<ItemT>> menu_target(menu);
	NcmFormatCallbacks callbacks;

	callbacks.text = Detail::targetText;
	callbacks.color = Detail::targetColor;
	callbacks.format = Detail::targetFormat;
	if (buffer == nullptr)
	{
		ncm_format_render(const_cast<NcmFormatAst *>(ast.cAst()),
		                  Detail::songImpl(song), &callbacks, &menu_target,
		                  nullptr, flags);
	}
	else
	{
		Detail::StreamTarget<NC::BasicBuffer<CharT>> buffer_target(*buffer);
		ncm_format_render(const_cast<NcmFormatAst *>(ast.cAst()),
		                  Detail::songImpl(song), &callbacks, &menu_target,
		                  &buffer_target, flags);
	}
}

template <typename CharT>
void print(const AST<CharT> &ast, NC::BasicBuffer<CharT> &buffer,
           const MPD::Song *song, const unsigned flags = Flags::All)
{
	ncm_format_render_buffer(const_cast<NcmFormatAst *>(ast.cAst()),
	                         Detail::songImpl(song), buffer.cBuffer(),
	                         buffer.cBuffer(), flags);
}

template <typename CharT>
std::basic_string<CharT> stringify(const AST<CharT> &ast,
                                   const MPD::Song *song)
{
	NcmBuffer result = ncm_format_render_string(
		const_cast<NcmFormatAst *>(ast.cAst()), Detail::songImpl(song));
	return Detail::takeBuffer(result);
}

template <typename CharT>
TagVector<CharT> flatten(const AST<CharT> &, const MPD::Song &)
{
	return TagVector<CharT>();
}

inline AST<char> parse(const std::string &s, const unsigned flags = Flags::All)
{
	NcmFormatAst ast;
	NcmError error;

	ncm_format_ast_init(&ast);
	ncm_error_clear(&error);
	if (!ncm_format_parse(&ast, const_cast<char *>(s.data()),
	                      static_cast<int32>(s.size()), flags, &error))
	{
		std::string message(error.message);
		ncm_format_ast_destroy(&ast);
		throw std::runtime_error(message);
	}
	return AST<char>(&ast);
}

inline AST<char> makeColumnsFormat(const std::vector<std::string> &columns)
{
	NcmFormatAst ast;

	ncm_format_ast_init(&ast);
	for (auto column = columns.begin(); column != columns.end(); ++column)
	{
		enum NcmSongGetter getters[32];
		int32 getters_len = 0;

		for (char type : *column)
		{
			enum NcmSongGetter getter = ncm_song_getter_from_char(type);
			if (getter != NCM_SONG_GETTER_NONE && getters_len < 32)
				getters[getters_len++] = getter;
		}
		ncm_format_ast_append_first_of_getters(&ast, getters, getters_len);
		if (column + 1 != columns.end())
			ncm_format_ast_append_text(&ast, const_cast<char *>(" "), 1);
	}
	return AST<char>(&ast);
}

}

#endif // NCMPCPP_HAVE_FORMAT_H
