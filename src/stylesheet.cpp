#include "html.h"
#include "stylesheet.h"
#include "css_parser.h"
#include "document.h"
#include "document_container.h"

namespace litehtml
{

namespace
{

const css_attribute_selector* find_rightmost_attr(const css_element_selector& selector,
												  attr_select_type type)
{
	for (const auto& attr : selector.m_attrs)
	{
		if (attr.type == type) return &attr;
	}
	return nullptr;
}

}

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
	rebuild_selector_index();
}

void css::collect_selector_lists(string_id tag,
								 string_id id,
								 const std::vector<string_id>& classes,
								 std::vector<const css_selector::vector*>& out) const
{
	out.clear();
	out.reserve(classes.size() + 3);

	auto push_bucket = [&](const css_selector::vector* bucket)
	{
		if (!bucket || bucket->empty()) return;
		if (std::find(out.begin(), out.end(), bucket) != out.end()) return;
		out.push_back(bucket);
	};

	push_bucket(&m_universal_selectors);
	if (id != empty_id)
	{
		push_bucket(find_bucket(m_id_selectors, id));
	}
	for (auto cls : classes)
	{
		push_bucket(find_bucket(m_class_selectors, cls));
	}
	if (tag != empty_id)
	{
		push_bucket(find_bucket(m_tag_selectors, tag));
	}
}

void css::rebuild_selector_index()
{
	m_universal_selectors.clear();
	m_tag_selectors.clear();
	m_class_selectors.clear();
	m_id_selectors.clear();

	for (const auto& selector : m_selectors)
	{
		const css_attribute_selector* id_attr = find_rightmost_attr(selector->m_right, select_id);
		if (id_attr)
		{
			m_id_selectors[id_attr->name].push_back(selector);
			continue;
		}

		const css_attribute_selector* class_attr = find_rightmost_attr(selector->m_right, select_class);
		if (class_attr)
		{
			m_class_selectors[class_attr->name].push_back(selector);
			continue;
		}

		if (selector->m_right.m_tag != star_id)
		{
			m_tag_selectors[selector->m_right.m_tag].push_back(selector);
			continue;
		}

		m_universal_selectors.push_back(selector);
	}
}

const css_selector::vector* css::find_bucket(const std::map<string_id, css_selector::vector>& buckets,
											 string_id key) const
{
	auto it = buckets.find(key);
	if (it == buckets.end()) return nullptr;
	return &it->second;
}

} // namespace litehtml
