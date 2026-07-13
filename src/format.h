#ifndef NCMPCPP_HAVE_FORMAT_H
#define NCMPCPP_HAVE_FORMAT_H

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

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

}

#endif // NCMPCPP_HAVE_FORMAT_H
