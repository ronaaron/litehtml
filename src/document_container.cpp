#include "utf8_strings.h"
#include "document_container.h"
#include <cstring>

namespace
{

inline bool is_horizontal_space(char32_t ch)
{
	return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\f';
}

inline bool is_cjk_codepoint(char32_t ch)
{
	return ch >= 0x4E00 && ch <= 0x9FCC;
}

template<typename Callback>
void emit_ascii_segment(const char* start, size_t len, std::string& scratch, const Callback& callback)
{
	if (!len) return;
	scratch.assign(start, len);
	callback(scratch.c_str());
}

template<typename Callback>
void emit_utf32_segment(const std::u32string& text, std::string& scratch, const Callback& callback)
{
	if (text.empty()) return;
	scratch.clear();
	scratch.reserve(text.size());
	for (char32_t ch : text)
	{
		litehtml::append_char(scratch, ch);
	}
	callback(scratch.c_str());
}

bool is_ascii_only(const char* text)
{
	for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p; ++p)
	{
		if (*p & 0x80) return false;
	}
	return true;
}

}

void litehtml::document_container::split_text(const char* text, const std::function<void(const char*)>& on_word, const std::function<void(const char*)>& on_space)
{
	if (!text || !text[0]) return;

	std::string scratch;
	if (is_ascii_only(text))
	{
		const char* segment_start = text;
		const char* p = text;
		while (*p)
		{
			unsigned char ch = static_cast<unsigned char>(*p);
			if (ch == '\n')
			{
				emit_ascii_segment(segment_start, static_cast<size_t>(p - segment_start), scratch, on_word);
				emit_ascii_segment(p, 1, scratch, on_space);
				++p;
				segment_start = p;
				continue;
			}

			if (is_horizontal_space(ch))
			{
				emit_ascii_segment(segment_start, static_cast<size_t>(p - segment_start), scratch, on_word);
				const char* space_start = p;
				do
				{
					++p;
					ch = static_cast<unsigned char>(*p);
				} while (*p && is_horizontal_space(ch));
				emit_ascii_segment(space_start, static_cast<size_t>(p - space_start), scratch, on_space);
				segment_start = p;
				continue;
			}

			++p;
		}
		emit_ascii_segment(segment_start, static_cast<size_t>(p - segment_start), scratch, on_word);
		return;
	}

	std::u32string word;
	std::u32string space;
	std::u32string input = (const char32_t*) utf8_to_utf32(text);
	auto flush_word = [&]()
	{
		emit_utf32_segment(word, scratch, on_word);
		word.clear();
	};
	auto flush_space = [&]()
	{
		emit_utf32_segment(space, scratch, on_space);
		space.clear();
	};

	for (char32_t ch : input)
	{
		if (ch == '\n')
		{
			flush_word();
			flush_space();
			space.push_back(ch);
			flush_space();
			continue;
		}

		if (is_horizontal_space(ch))
		{
			flush_word();
			space.push_back(ch);
			continue;
		}

		if (is_cjk_codepoint(ch))
		{
			flush_word();
			flush_space();
			word.push_back(ch);
			flush_word();
			continue;
		}

		flush_space();
		word.push_back(ch);
	}

	flush_word();
	flush_space();
}
