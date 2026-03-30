#ifndef LH_EL_TEXT_H
#define LH_EL_TEXT_H

#include "element.h"
#include "document.h"

namespace litehtml
{
	class el_text : public element
	{
		protected:
			string			m_text;
			string			m_transformed_text;
			string			m_trailing_space;
			string			m_transformed_trailing_space;
			size			m_size;
			pixel_t			m_text_width = 0;
			pixel_t			m_trailing_space_width = 0;
			bool			m_use_transformed;
			bool			m_draw_spaces;
			bool			m_draw_trailing_space = true;
		public:
				el_text(const char* text, const document::ptr& doc);
				el_text(const char* text, size_t len, const document::ptr& doc);
			void				append_trailing_space(const char* text);
			void				append_trailing_space(const char* text, size_t len);

				void				get_text(string& text) const override;
			void				compute_styles(bool recursive) override;
			bool				is_text() const override { return true; }
			bool				has_trailing_white_space() const override;
			pixel_t				trim_trailing_white_space() override;
			void				restore_trailing_white_space() override;

			void draw(uint_ptr hdc, pixel_t x, pixel_t y, const position *clip, const std::shared_ptr<render_item> &ri) override;
			string				dump_get_name() override;
			std::vector<std::tuple<string, string>> dump_get_attrs() override;
		protected:
			void				get_content_size(size& sz, pixel_t max_width) override;
			const string&		visible_text() const;
			const string&		visible_trailing_space() const;
		};
}

#endif  // LH_EL_TEXT_H
