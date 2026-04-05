#include "html.h"
#include "stylesheet.h"
#include "css_parser.h"
#include "document.h"
#include "document_container.h"

namespace litehtml
{

// https://www.w3.org/TR/css-syntax-3/#parse-a-css-stylesheet
template<class Input> // Input == string or css_token_vector
void css::parse_css_stylesheet(const Input& input, string baseurl, document::ptr doc, media_query_list_list::ptr media, bool top_level)
{
	if (doc && media)
		doc->add_media_list(media);

	// To parse a CSS stylesheet, first parse a stylesheet.
	auto rules = css_parser::parse_stylesheet(input, top_level);
	bool import_allowed = top_level;

	// Interpret all of the resulting top-level qualified rules as style rules, defined below.
	// If any style rule is invalid, or any at-rule is not recognized or is invalid according
	// to its grammar or context, it's a parse error. Discard that rule.
	for (auto rule : rules)
	{
		if (rule->type == raw_rule::qualified)
		{
			if (parse_style_rule(rule, baseurl, doc, media))
				import_allowed = false;
			continue;
		}

		// Otherwise: at-rule
		switch (_id(lowcase(rule->name)))
		{
		case _charset_: // ignored  https://www.w3.org/TR/css-syntax-3/#charset-rule
			break;

		case _import_:
			if (import_allowed)
				parse_import_rule(rule, baseurl, doc, media);
			else
				css_parse_error("incorrect placement of @import rule");
			break;

		// https://www.w3.org/TR/css-conditional-3/#at-media
		// @media <media-query-list> { <stylesheet> }
		case _media_:
		{
			if (rule->block.type != CURLY_BLOCK) break;
			auto new_media = media;
			auto mq_list = parse_media_query_list(rule->prelude, doc);
			// An empty media query list evaluates to true.  https://drafts.csswg.org/mediaqueries-5/#example-6f06ee45
			if (!mq_list.empty())
			{
				new_media = make_shared<media_query_list_list>(media ? *media : media_query_list_list());
				new_media->add(mq_list);
			}
			parse_css_stylesheet(rule->block.value, baseurl, doc, new_media, false);
			import_allowed = false;
			break;
		}

		default:
			css_parse_error("unrecognized rule @" + rule->name);
		}
	}
}

// https://drafts.csswg.org/css-cascade-5/#at-import
// `layer` and `supports` are not supported
// @import [ <url> | <string> ] <media-query-list>?
void css::parse_import_rule(raw_rule::ptr rule, string baseurl, document::ptr doc, media_query_list_list::ptr media)
{
	auto tokens = rule->prelude;
	int index = 0;
	skip_whitespace(tokens, index);
	auto tok = at(tokens, index);
	string url;
	auto parse_string = [](const css_token& tok, string& str)
	{
		if (tok.type != STRING) return false;
		str = tok.str;
		return true;
	};
	bool ok = parse_url(tok, url) || parse_string(tok, url);
	if (!ok) {
		css_parse_error("invalid @import rule");
		return;
	}
	document_container* container = doc->container();
	string css_text;
	string css_baseurl = baseurl;
	container->import_css(css_text, url, css_baseurl);

	auto new_media = media;
	tokens = slice(tokens, index + 1);
	auto mq_list = parse_media_query_list(tokens, doc);
	if (!mq_list.empty())
	{
		new_media = make_shared<media_query_list_list>(media ? *media : media_query_list_list());
		new_media->add(mq_list);
	}

	parse_css_stylesheet(css_text, css_baseurl, doc, new_media, true);
}

// https://www.w3.org/TR/css-syntax-3/#style-rules
bool css::parse_style_rule(raw_rule::ptr rule, string baseurl, document::ptr doc, media_query_list_list::ptr media)
{
	// The prelude of the qualified rule is parsed as a <selector-list>. If this returns failure, the entire style rule is invalid.
	auto list = parse_selector_list(rule->prelude, strict_mode, doc->mode());
	if (list.empty())
	{
		css_parse_error("invalid selector");
		return false;
	}

	style::ptr style = make_shared<litehtml::style>(); // style block
	// The content of the qualified rule's block is parsed as a style block's contents.
	style->add(rule->block.value, baseurl, doc->container());

	for (auto sel : list)
	{
		sel->m_style = style;
		sel->m_media_query = media;
		sel->calc_specificity();
		add_selector(sel);
	}
	return true;
}

void css::sort_selectors()
{
	std::sort(m_selectors.begin(), m_selectors.end(),
		 [](const css_selector::ptr& v1, const css_selector::ptr& v2)
		 {
			 return (*v1) < (*v2);
		 }
	);
	build_index();
}

void css::build_index()
{
	m_index = {};

	for (size_t i = 0; i < m_selectors.size(); i++)
	{
		const auto& right = m_selectors[i]->m_right;

		// Classify by the right-most compound selector.
		// Each selector goes into exactly ONE bucket (highest priority wins):
		//   id > class > tag > universal
		bool has_id = false;
		bool has_class = false;

		for (const auto& attr : right.m_attrs)
		{
			if (attr.type == select_id)
			{
				has_id = true;
			}
			else if (attr.type == select_class)
			{
				has_class = true;
			}
		}

		if (has_id)
		{
			// Put into by_id under the first #id found
			for (const auto& attr : right.m_attrs)
			{
				if (attr.type == select_id)
				{
					m_index.by_id[attr.name].push_back(i);
					break;
				}
			}
		}
		else if (has_class)
		{
			// Put into by_class under EACH class in the right-most compound selector.
			// This allows selectors like ".a.b" to be found via either class.
			for (const auto& attr : right.m_attrs)
			{
				if (attr.type == select_class)
				{
					m_index.by_class[attr.name].push_back(i);
				}
			}
		}
		else if (right.m_tag != star_id)
		{
			m_index.by_tag[right.m_tag].push_back(i);
		}
		else
		{
			m_index.universal.push_back(i);
		}
	}
}

const std::vector<size_t> css::s_empty_index;

void css::get_candidates(string_id tag, string_id id,
						 const std::vector<string_id>& classes,
						 std::vector<size_t>& out) const
{
	// Gather from all applicable buckets.
	// Since each selector is in exactly one bucket, there are no duplicates.

	// 1. By ID
	if (id != empty_id)
	{
		auto it = m_index.by_id.find(id);
		if (it != m_index.by_id.end())
			out.insert(out.end(), it->second.begin(), it->second.end());
	}

	// 2. By class
	for (string_id cls : classes)
	{
		auto it = m_index.by_class.find(cls);
		if (it != m_index.by_class.end())
			out.insert(out.end(), it->second.begin(), it->second.end());
	}

	// 3. By tag
	if (tag != star_id)
	{
		auto it = m_index.by_tag.find(tag);
		if (it != m_index.by_tag.end())
			out.insert(out.end(), it->second.begin(), it->second.end());
	}

	// 4. Universal (always checked)
	out.insert(out.end(), m_index.universal.begin(), m_index.universal.end());
}

const std::vector<size_t>& css::selectors_for_class(string_id cls) const
{
	auto it = m_index.by_class.find(cls);
	if (it != m_index.by_class.end())
		return it->second;
	return s_empty_index;
}

} // namespace litehtml
