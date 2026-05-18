#include "html.h"
#include "el_text.h"
#include "render_item.h"
#include "document_container.h"

namespace
{

bool needs_preserved_whitespace_normalization(const litehtml::string& input)
{
	for (char ch : input)
	{
		switch (ch)
		{
		case '\t':
		case '\n':
		case '\r':
		case '\f':
			return true;
		default:
			break;
		}
	}

	return false;
}

void normalize_preserved_whitespace(const litehtml::string& input, litehtml::string& output)
{
	output.clear();
	output.reserve(input.size());

	for (char ch : input)
	{
		switch (ch)
		{
		case '\t':
			output += "    ";
			break;
		case '\n':
		case '\r':
		case '\f':
			break;
		default:
			output.push_back(ch);
			break;
		}
	}
}

}

litehtml::el_text::el_text(const char* text, const document::ptr& doc) : element(doc)
{
	if(text)
	{
		m_text = text;
	}
	m_use_transformed	= false;
	m_draw_spaces		= true;
	css_w().set_display(display_inline_text);
}

litehtml::el_text::el_text(const char* text, size_t len, const document::ptr& doc) : element(doc)
{
	if(text && len)
	{
		m_text.assign(text, len);
	}
	m_use_transformed	= false;
	m_draw_spaces		= true;
	css_w().set_display(display_inline_text);
}

void litehtml::el_text::append_trailing_space(const char* text)
{
	if (text)
	{
		m_trailing_space += text;
	}
}

void litehtml::el_text::append_trailing_space(const char* text, size_t len)
{
	if (text && len)
	{
		m_trailing_space.append(text, len);
	}
}

void litehtml::el_text::get_content_size( size& sz, pixel_t /*max_width*/ )
{
	sz = m_size;
}

void litehtml::el_text::get_text( string& text ) const
{
	text += m_text;
	text += m_trailing_space;
}

void litehtml::el_text::compute_styles(bool /*recursive*/)
{
	element::ptr el_parent = parent();
	if (el_parent)
	{
		css_w().line_height_w() = el_parent->css().line_height();
		css_w().set_font(el_parent->css().get_font());
		css_w().set_font_metrics(el_parent->css().get_font_metrics());
		css_w().set_white_space(el_parent->css().get_white_space());
		css_w().set_text_transform(el_parent->css().get_text_transform());
	}
	css_w().set_display(display_inline_text);
	css_w().set_float(float_none);

	if(m_css.get_text_transform() != text_transform_none)
	{
		m_transformed_text	= m_text;
		m_use_transformed = true;
		get_document()->container()->transform_text(m_transformed_text, m_css.get_text_transform());
	} else
	{
		m_use_transformed = false;
	}

	element::ptr p = parent();
	while(p && p->css().get_display() == display_inline)
	{
		if(p->css().get_position() == element_position_relative)
		{
			css_w().set_offsets(p->css().get_offsets());
			css_w().set_position(element_position_relative);
			break;
		}
		p = p->parent();
	}
	if(p)
	{
		css_w().set_position(element_position_static);
	}

	if(is_white_space())
	{
		m_transformed_text = " ";
		m_use_transformed = true;
	} else
	{
		const litehtml::string& preserved =
			m_use_transformed ? m_transformed_text : m_text;
		if (needs_preserved_whitespace_normalization(preserved))
		{
			litehtml::string normalized;
			normalize_preserved_whitespace(preserved, normalized);
			m_transformed_text.swap(normalized);
			m_use_transformed = true;
		}
	}
	m_transformed_trailing_space.clear();
	if(!m_trailing_space.empty() &&
		!is_one_of(m_css.get_white_space(), white_space_normal, white_space_nowrap, white_space_pre_line))
	{
		if (needs_preserved_whitespace_normalization(m_trailing_space))
		{
			normalize_preserved_whitespace(m_trailing_space, m_transformed_trailing_space);
		}
	}
	m_draw_trailing_space = true;

	font_metrics fm;
	uint_ptr font = 0;
	if (el_parent)
	{
		font = el_parent->css().get_font();
		fm = el_parent->css().get_font_metrics();
	}
	if(is_break() || !font)
	{
		m_size.height	= 0;
		m_size.width	= 0;
		m_text_width = 0;
		m_trailing_space_width = 0;
	} else
	{
		m_size.height = fm.height;
		m_text_width =
			get_document()->container()->text_width(visible_text().c_str(), font);
		m_trailing_space_width = 0;
		if (!m_trailing_space.empty())
		{
			m_trailing_space_width =
				get_document()->container()->text_width(visible_trailing_space().c_str(), font);
		}
		m_size.width = m_text_width + m_trailing_space_width;
	}
	m_draw_spaces = fm.draw_spaces;
}

bool litehtml::el_text::has_trailing_white_space() const
{
	return m_draw_trailing_space &&
		!m_trailing_space.empty() &&
		is_one_of(m_css.get_white_space(), white_space_normal, white_space_nowrap, white_space_pre_line);
}

litehtml::pixel_t litehtml::el_text::trim_trailing_white_space()
{
	if (!has_trailing_white_space()) return 0;
	m_draw_trailing_space = false;
	m_size.width -= m_trailing_space_width;
	if (m_size.width < 0) m_size.width = 0;
	return m_trailing_space_width;
}

void litehtml::el_text::restore_trailing_white_space()
{
	if (m_draw_trailing_space || m_trailing_space.empty()) return;
	m_draw_trailing_space = true;
	m_size.width += m_trailing_space_width;
}

void litehtml::el_text::draw(uint_ptr hdc, pixel_t x, pixel_t y, const position *clip, const std::shared_ptr<render_item> &ri)
{
	if(is_white_space() && !m_draw_spaces)
	{
		return;
	}

	position pos = ri->pos();
	pos.x	+= x;
	pos.y	+= y;
	pos.round();

	if(pos.does_intersect(clip))
	{
		element::ptr el_parent = parent();
		if (el_parent)
		{
			document::ptr doc = get_document();

			uint_ptr font = el_parent->css().get_font();
			if(font)
			{
				web_color color = el_parent->css().get_color();
				if (m_draw_trailing_space && m_draw_spaces && !m_trailing_space.empty())
				{
					litehtml::string text = visible_text();
					text += visible_trailing_space();
					doc->container()->draw_text(hdc, text.c_str(), font, color, pos);
				} else
				{
					doc->container()->draw_text(hdc, visible_text().c_str(), font, color, pos);
				}
			}
		}
	}
}

const litehtml::string& litehtml::el_text::visible_text() const
{
	return m_use_transformed ? m_transformed_text : m_text;
}

const litehtml::string& litehtml::el_text::visible_trailing_space() const
{
	return m_transformed_trailing_space.empty() ? m_trailing_space : m_transformed_trailing_space;
}

litehtml::string litehtml::el_text::dump_get_name()
{
	return "text: \"" + get_escaped_string(m_text) + "\"";
}

std::vector<std::tuple<litehtml::string, litehtml::string>> litehtml::el_text::dump_get_attrs()
{
	return {};
}
