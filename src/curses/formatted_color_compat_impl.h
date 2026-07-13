#if !defined(SRC_CURSES_FORMATTED_COLOR_COMPAT_IMPL_H)
#define SRC_CURSES_FORMATTED_COLOR_COMPAT_IMPL_H

namespace {

void verifyFormats(const NC::FormattedColor::Formats &formats)
{
	for (auto &fmt : formats)
	{
		if (fmt == NC_FORMAT_NO_BOLD
		    || fmt == NC_FORMAT_NO_UNDERLINE
		    || fmt == NC_FORMAT_NO_REVERSE
		    || fmt == NC_FORMAT_NO_ALT_CHARSET
		    || fmt == NC_FORMAT_NO_ITALIC)
			throw std::logic_error("FormattedColor can't hold disabling formats");
	}
}

}

inline NC::FormattedColor::FormattedColor()
{
	nc_formatted_color_init(&m_impl);
}

inline NC::FormattedColor::FormattedColor(Color color_, Formats formats_)
{
	if (color_ == NC::Color::End)
		throw std::logic_error("FormattedColor can't hold Color::End");
	verifyFormats(formats_);
	nc_formatted_color_init_color(&m_impl, toNcColor(color_));
	for (auto &format : formats_)
		nc_formatted_color_add_format(&m_impl, toNcFormat(format));
}

inline NC::FormattedColor::FormattedColor(NcFormattedColor *formatted_color)
{
	nc_formatted_color_copy(&m_impl, formatted_color);
}

inline NC::FormattedColor::FormattedColor(const FormattedColor &rhs)
{
	nc_formatted_color_copy(&m_impl,
	                        const_cast<NcFormattedColor *>(rhs.cFormattedColor()));
}

inline NC::FormattedColor::FormattedColor(FormattedColor &&rhs) noexcept
{
	nc_formatted_color_move(&m_impl, rhs.cFormattedColor());
}

inline NC::FormattedColor &NC::FormattedColor::operator=(const FormattedColor &rhs)
{
	if (this != &rhs)
	{
		nc_formatted_color_destroy(&m_impl);
		nc_formatted_color_copy(
			&m_impl, const_cast<NcFormattedColor *>(rhs.cFormattedColor()));
	}
	return *this;
}

inline NC::FormattedColor &NC::FormattedColor::operator=(FormattedColor &&rhs) noexcept
{
	if (this != &rhs)
	{
		nc_formatted_color_destroy(&m_impl);
		nc_formatted_color_move(&m_impl, rhs.cFormattedColor());
	}
	return *this;
}

inline NC::FormattedColor::~FormattedColor()
{
	nc_formatted_color_destroy(&m_impl);
}

inline const NC::Color &NC::FormattedColor::color() const
{
	m_color_cache = fromNcColor(m_impl.color);
	return m_color_cache;
}

inline NC::FormattedColor::Formats NC::FormattedColor::formats() const
{
	Formats result;
	auto formats = nc_formatted_color_formats(
		const_cast<NcFormattedColor *>(&m_impl));
	auto count = nc_formatted_color_format_count(
		const_cast<NcFormattedColor *>(&m_impl));
	for (int32 i = 0; i < count; i += 1)
		result.push_back(fromNcFormat(formats[i]));
	return result;
}

inline NcFormattedColor *NC::FormattedColor::cFormattedColor()
{
	return &m_impl;
}

const inline NcFormattedColor *NC::FormattedColor::cFormattedColor() const
{
	return &m_impl;
}

inline NcColor NC::toNcColor(Color color)
{
	if (color.isDefault())
		return nc_color_default();
	if (color.isEnd())
		return nc_color_end();
	return nc_color_make(color.foreground(), color.background(), false, false);
}

inline NC::Color NC::fromNcColor(NcColor color)
{
	if (nc_color_is_default(color))
		return Color::Default;
	if (nc_color_is_end(color))
		return Color::End;
	return Color(color.foreground, color.background, false, false);
}

inline NcFormat NC::toNcFormat(enum NcFormat format)
{
	return format;
}


inline enum NcFormat NC::fromNcFormat(NcFormat format)
{
	return format;
}


#endif /* SRC_CURSES_FORMATTED_COLOR_COMPAT_IMPL_H */
