#include <chrono>
#include "html.h"
#include "document.h"
#include "stylesheet.h"
#include "html_tag.h"
#include "el_text.h"
#include "el_para.h"
#include "el_space.h"
#include "el_body.h"
#include "el_image.h"
#include "el_table.h"
#include "el_td.h"
#include "el_link.h"
#include "el_title.h"
#include "el_style.h"
#include "el_script.h"
#include "el_comment.h"
#include "el_cdata.h"
#include "el_base.h"
#include "el_anchor.h"
#include "el_break.h"
#include "el_div.h"
#include "el_font.h"
#include "el_tr.h"
#include "gumbo.h"
#include "render_item.h"
#include "render_table.h"
#include "render_block.h"
#include "document_container.h"
#include "types.h"

namespace litehtml
{

// Static master CSS cache — shared across all documents for a given master_styles string.
// The master CSS is the browser default stylesheet — semantically identical across all
// documents that pass the same string. User CSS (.selected, .track, etc.) lives in each
// document's own m_styles, never here. This cache maps master_styles hash → parsed css.
// Multiple distinct master_styles strings (e.g. different document categories) are all
// cached simultaneously — no thrashing. Call invalidate_master_css_cache() only when you
// need to force a re-parse of a specific or all master stylesheets.
std::mutex        document::s_master_css_mutex;
document::css_lru document::s_master_css_lru;

std::mutex        document::s_doc_css_mutex;
document::css_lru document::s_doc_css_lru;

	document::document(document_container* container)
	{
		m_container = container;
	}

	std::shared_ptr<css_properties> document::get_flyweight_css(std::shared_ptr<css_properties> css)
	{
		auto it = m_css_pool.find(css);
		if (it != m_css_pool.end())
		{
			return *it;
		}
		m_css_pool.insert(css);
		return css;
	}

	document::~document()
	{
		m_over_element = m_active_element = nullptr;
		if(m_container)
		{
			for(auto& font : m_fonts)
			{
				m_container->delete_font(font.second.font);
			}
		}
	}

	/*static*/ void document::invalidate_master_css_cache()
	{
		std::lock_guard<std::mutex> lock(s_master_css_mutex);
		s_master_css_lru.clear();
	}

	document::ptr document::createFromString(const estring& str, document_container* container,
											 const string& master_styles, const string& user_styles)
	{
		// Performance instrumentation — uncomment to re-enable:
		// #define LITEHTML_PERF
#ifdef LITEHTML_PERF
		using clock = std::chrono::steady_clock;
		auto t_start = clock::now();
#endif

		// Create litehtml::document
		document::ptr doc	= make_shared<document>(container);

		// Parse document into GumboOutput
#ifdef LITEHTML_PERF
		auto t0 = clock::now();
#endif
		GumboOutput* output = doc->parse_html(str);

		// mode must be set before doc->create_node because it is used in html_tag::set_attr
		switch(output->document->v.document.doc_type_quirks_mode)
		{
		case GUMBO_DOCTYPE_NO_QUIRKS:
			doc->m_mode = no_quirks_mode;
			break;
		case GUMBO_DOCTYPE_QUIRKS:
			doc->m_mode = quirks_mode;
			break;
		case GUMBO_DOCTYPE_LIMITED_QUIRKS:
			doc->m_mode = limited_quirks_mode;
			break;
		}
#ifdef LITEHTML_PERF
		auto t1 = clock::now();
#endif

		// Create litehtml::elements.
		elements_list root_elements;
		doc->create_node(output->root, root_elements, true, true);
		if(!root_elements.empty())
		{
			doc->m_root = root_elements.back();
		}

		// Destroy GumboOutput
		gumbo_destroy_output(&kGumboDefaultOptions, output);
#ifdef LITEHTML_PERF
		auto t2 = clock::now();
#endif

		if(master_styles != "")
		{
			const size_t h = std::hash<string>{}(master_styles);
			std::lock_guard<std::mutex> lock(s_master_css_mutex);
			if (const litehtml::css* cached = s_master_css_lru.find(h))
			{
				doc->m_master_css = *cached;
			} else {
				doc->m_master_css.parse_css_stylesheet(master_styles, "", doc);
				doc->m_master_css.sort_selectors();
				s_master_css_lru.put(h, doc->m_master_css);
			}
		}
#ifdef LITEHTML_PERF
		auto t3 = clock::now();
#endif

		if(user_styles != "")
		{
			doc->m_user_css.parse_css_stylesheet(user_styles, "", doc);
			doc->m_user_css.sort_selectors();
		}

		// Let's process created elements tree
		if(doc->m_root)
		{
			doc->container()->get_media_features(doc->m_media);

			doc->m_root->set_pseudo_class(_root_, true);

			// apply master CSS
			doc->m_root->apply_stylesheet(doc->m_master_css);
	#ifdef LITEHTML_PERF
		auto t_ms1 = clock::now();
#endif

			// parse elements attributes
			doc->m_root->parse_attributes();
	#ifdef LITEHTML_PERF
		auto t_pa = clock::now();
#endif

			// Parse style sheets linked in document (from <style> tags).
			// Hash all CSS text blocks to look up the LRU cache first.
			// This avoids re-parsing the same inline CSS on every document
			// creation (e.g. opening the same view repeatedly).
			{
				// Compute combined hash of all css_text entries.
				size_t doc_css_hash = 0;
				for (const auto& c : doc->m_css)
				{
					// Boost-style hash combine to avoid trivial XOR collisions.
					auto combine = [&](const string& s)
					{
						doc_css_hash ^= std::hash<string>{}(s)
							+ 0x9e3779b9u + (doc_css_hash << 6) + (doc_css_hash >> 2);
					};
					combine(c.text);
					combine(c.media);
					combine(c.baseurl);
				}

				bool cache_hit = false;
				if (!doc->m_css.empty())
				{
					std::lock_guard<std::mutex> lock(s_doc_css_mutex);
					if (const litehtml::css* cached = s_doc_css_lru.find(doc_css_hash))
					{
						// Cache hit — copy pre-parsed css into this document's m_styles.
						doc->m_styles = *cached;
						cache_hit = true;
					}
				}

				if (!cache_hit)
				{
					for (const auto& css : doc->m_css)
					{
						media_query_list_list::ptr media;
						if (css.media != "")
						{
							auto mq_list = parse_media_query_list(css.media, doc);
							media        = make_shared<media_query_list_list>();
							media->add(mq_list);
						}
						doc->m_styles.parse_css_stylesheet(css.text, css.baseurl, doc, media);
					}
					// Sort css selectors using CSS rules.
					doc->m_styles.sort_selectors();

					if (!doc->m_css.empty())
					{
						std::lock_guard<std::mutex> lock(s_doc_css_mutex);
						s_doc_css_lru.put(doc_css_hash, doc->m_styles);
					}
				}
			}
	#ifdef LITEHTML_PERF
		auto t_dp = clock::now();
#endif

			// Apply media features.
			doc->update_media_lists(doc->m_media);

			// Apply parsed styles.
			doc->m_root->apply_stylesheet(doc->m_styles);
	#ifdef LITEHTML_PERF
		auto t_ds = clock::now();
#endif

			// Apply user styles if any
			doc->m_root->apply_stylesheet(doc->m_user_css);
	#ifdef LITEHTML_PERF
		auto t_us = clock::now();
#endif

			// Initialize element::m_css
			doc->m_root->compute_styles();
	#ifdef LITEHTML_PERF
		auto t_cs = clock::now();
#endif

			// Create rendering tree
			doc->m_root_render = doc->m_root->create_render_item(nullptr);

			// Now the m_tabular_elements is filled with tabular elements.
			// We have to check the tabular elements for missing table elements
			// and create the anonymous boxes in visual table layout
			doc->fix_tables_layout();

			// Finally initialize elements
			// init() returns pointer to the render_init element because it can change its type
			if(doc->m_root_render)
			{
				doc->m_root_render = doc->m_root_render->init();
			}
	#ifdef LITEHTML_PERF
		auto t_ri = clock::now();
#endif

#ifdef LITEHTML_PERF
			auto us = [](auto a, auto b) { return std::chrono::duration_cast<std::chrono::microseconds>(b - a).count(); };
			fprintf(stderr,
				"[litehtml-phases] html_parse=%lldµs  elements=%lldµs  master_css_parse=%lldµs  "
				"apply_master=%lldµs  parse_attrs=%lldµs  doc_css_parse=%lldµs  "
				"apply_doc=%lldµs  apply_user=%lldµs  compute_styles=%lldµs  "
				"render_tree=%lldµs  TOTAL=%lldµs  selectors: master=%zu doc=%zu user=%zu\n",
				(long long)us(t0, t1),     // html_parse (gumbo)
				(long long)us(t1, t2),     // elements (create_node + gumbo_destroy)
				(long long)us(t2, t3),     // master_css_parse
				(long long)us(t3, t_ms1),  // apply_master
				(long long)us(t_ms1, t_pa), // parse_attributes
				(long long)us(t_pa, t_dp),  // doc_css_parse
				(long long)us(t_dp, t_ds),  // apply_doc
				(long long)us(t_ds, t_us),  // apply_user
				(long long)us(t_us, t_cs),  // compute_styles
				(long long)us(t_cs, t_ri),  // render_tree (create + fix_tables + init)
				(long long)us(t_start, t_ri),
				doc->m_master_css.selectors().size(),
				doc->m_styles.selectors().size(),
				doc->m_user_css.selectors().size()
			);
#endif
		} // if(doc->m_root)

		return doc;
	}

	// https://html.spec.whatwg.org/multipage/parsing.html#change-the-encoding
	encoding adjust_meta_encoding(encoding meta_encoding, encoding current_encoding)
	{
		// 1.
		if(current_encoding == encoding::utf_16le || current_encoding == encoding::utf_16be)
			return current_encoding;

		// 2.
		if(meta_encoding == encoding::utf_16le || meta_encoding == encoding::utf_16be)
			return encoding::utf_8;

		// 3.
		if(meta_encoding == encoding::x_user_defined)
			return encoding::windows_1252;

		// 4,5,6.
		return meta_encoding;
	}

	// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inhead:change-the-encoding
	encoding get_meta_encoding(GumboNode* root)
	{
		// find <head>
		GumboNode* head = nullptr;
		for(size_t i = 0; i < root->v.element.children.length; i++)
		{
			GumboNode* node = (GumboNode*) root->v.element.children.data[i];
			if(node->type == GUMBO_NODE_ELEMENT && node->v.element.tag == GUMBO_TAG_HEAD)
			{
				head = node;
				break;
			}
		}
		if(!head)
			return encoding::null;

		// go through <meta> tags in <head>
		for(size_t i = 0; i < head->v.element.children.length; i++)
		{
			GumboNode* node = (GumboNode*) head->v.element.children.data[i];
			if(node->type != GUMBO_NODE_ELEMENT || node->v.element.tag != GUMBO_TAG_META)
				continue;

			auto charset	= gumbo_get_attribute(&node->v.element.attributes, "charset");
			auto http_equiv = gumbo_get_attribute(&node->v.element.attributes, "http-equiv");
			auto content	= gumbo_get_attribute(&node->v.element.attributes, "content");
			// 1. If the element has a charset attribute...
			if(charset)
			{
				auto encoding = get_encoding(charset->value);
				if(encoding != encoding::null)
					return encoding;
			}
			// 2. Otherwise, if the element has an http-equiv attribute...
			else if(http_equiv && t_strcasecmp(http_equiv->value, "content-type") == 0 && content)
			{
				auto encoding = extract_encoding_from_meta_element(content->value);
				if(encoding != encoding::null)
					return encoding;
			}
		}

		return encoding::null;
	}

	// substitute for gumbo_parse that handles encodings
	GumboOutput* document::parse_html(estring str)
	{
		// https://html.spec.whatwg.org/multipage/parsing.html#the-input-byte-stream
		encoding_sniffing_algorithm(str);
		// cannot store output in local variable because gumbo keeps pointers into it,
		// which will be accessed later in gumbo_tag_from_original_text
		if(str.encoding == encoding::utf_8)
			m_text = str;
		else
			decode(str, str.encoding, m_text);

		// Gumbo does not support callbacks on node creation, so we cannot change encoding while parsing.
		// Instead, we parse entire file and then handle <meta> tags.

		// Using gumbo_parse_with_options to pass string length (m_text may contain NUL chars).
		GumboOutput* output = gumbo_parse_with_options(&kGumboDefaultOptions, m_text.data(), m_text.size());

		if(str.confidence == confidence::certain)
			return output;

		// Otherwise: confidence is tentative.

		// If valid HTML encoding is specified in <meta> tag...
		encoding meta_encoding = get_meta_encoding(output->root);
		if(meta_encoding != encoding::null)
		{
			// ...and it is different from currently used encoding...
			encoding new_encoding = adjust_meta_encoding(meta_encoding, str.encoding);
			if(new_encoding != str.encoding)
			{
				// ...reparse with the new encoding.
				gumbo_destroy_output(&kGumboDefaultOptions, output);
				m_text.clear();

				if(new_encoding == encoding::utf_8)
					m_text = str;
				else
					decode(str, new_encoding, m_text);
				output = gumbo_parse_with_options(&kGumboDefaultOptions, m_text.data(), m_text.size());
			}
		}

		return output;
	}

	void document::create_node(void* gnode, elements_list& elements, bool parseTextNode, bool process_root)
	{
		auto* node = (GumboNode*) gnode;
		switch(node->type)
		{
		case GUMBO_NODE_ELEMENT:
			{
				if(process_root)
				{
					string_map		attrs;
					GumboAttribute* attr;
					for(unsigned int i = 0; i < node->v.element.attributes.length; i++)
					{
						attr			  = (GumboAttribute*) node->v.element.attributes.data[i];
						attrs[attr->name] = attr->value;
					}

					element::ptr ret;
					const char*	 tag = gumbo_normalized_tagname(node->v.element.tag);
					if(tag[0])
					{
						ret = create_element(tag, attrs);
					} else
					{
						if(node->v.element.original_tag.data && node->v.element.original_tag.length)
						{
							string str;
							gumbo_tag_from_original_text(&node->v.element.original_tag);
							str.append(node->v.element.original_tag.data, node->v.element.original_tag.length);
							ret = create_element(str.c_str(), attrs);
						}
					}
					if(!strcmp(tag, "script"))
					{
						parseTextNode = false;
					}
					if(ret)
					{
						elements_list child;
						for(unsigned int i = 0; i < node->v.element.children.length; i++)
						{
							child.clear();
							create_node(static_cast<GumboNode*>(node->v.element.children.data[i]), child, parseTextNode,
										true);
							std::for_each(child.begin(), child.end(),
										  [&ret](element::ptr& el) { ret->appendChild(el); });
						}
						elements.push_back(ret);
					}
				} else
				{
					for(unsigned int i = 0; i < node->v.element.children.length; i++)
					{
						create_node(static_cast<GumboNode*>(node->v.element.children.data[i]), elements, parseTextNode,
									true);
					}
				}
			}
			break;
		case GUMBO_NODE_TEXT:
			{
				if(!parseTextNode)
				{
					elements.push_back(std::make_shared<el_text>(node->v.text.text, shared_from_this()));
				} else
				{
					m_container->split_text(
						node->v.text.text,
						[this, &elements](const char* text) {
							elements.push_back(std::make_shared<el_text>(text, shared_from_this()));
						},
						[this, &elements](const char* text) {
							elements.push_back(std::make_shared<el_space>(text, shared_from_this()));
						});
				}
			}
			break;
		case GUMBO_NODE_CDATA:
			{
				element::ptr ret = std::make_shared<el_cdata>(shared_from_this());
				ret->set_data(node->v.text.text);
				elements.push_back(ret);
			}
			break;
		case GUMBO_NODE_COMMENT:
			{
				element::ptr ret = std::make_shared<el_comment>(shared_from_this());
				ret->set_data(node->v.text.text);
				elements.push_back(ret);
			}
			break;
		case GUMBO_NODE_WHITESPACE:
			{
				// The original code split every character into a separate el_space, which
				// causes massive element explosion for indented HTML (e.g. "\n    " → 5 elements).
				// For white-space:normal (the common case) all whitespace collapses to " " anyway,
				// so the only character el_space needs to distinguish is '\n' (for is_break() in
				// pre/pre-line/pre-wrap modes). We therefore split only on newlines:
				//   "\n    "  → el_space("\n") + el_space("    ")   [2 elements]
				//   "  "      → el_space("  ")                      [1 element]
				//   "\n"      → el_space("\n")                      [1 element]
				const char* p     = node->v.text.text;
				const char* start = p;
				while (*p)
				{
					if (*p == '\n')
					{
						if (p > start)
							elements.push_back(std::make_shared<el_space>(string(start, p - start).c_str(), shared_from_this()));
						elements.push_back(std::make_shared<el_space>("\n", shared_from_this()));
						start = p + 1;
					}
					++p;
				}
				if (p > start)
					elements.push_back(std::make_shared<el_space>(start, shared_from_this()));
			}
			break;
		default:
			break;
		}
	}

	element::ptr document::create_element(const char* tag_name, const string_map& attributes)
	{
		element::ptr  newTag;
		document::ptr this_doc = shared_from_this();
		if(m_container)
		{
			newTag = m_container->create_element(tag_name, attributes, this_doc);
		}
		if(!newTag)
		{
			if(!strcmp(tag_name, "br"))
			{
				newTag = std::make_shared<el_break>(this_doc);
			} else if(!strcmp(tag_name, "p"))
			{
				newTag = std::make_shared<el_para>(this_doc);
			} else if(!strcmp(tag_name, "img"))
			{
				newTag = std::make_shared<el_image>(this_doc);
			} else if(!strcmp(tag_name, "table"))
			{
				newTag = std::make_shared<el_table>(this_doc);
			} else if(!strcmp(tag_name, "td") || !strcmp(tag_name, "th"))
			{
				newTag = std::make_shared<el_td>(this_doc);
			} else if(!strcmp(tag_name, "link"))
			{
				newTag = std::make_shared<el_link>(this_doc);
			} else if(!strcmp(tag_name, "title"))
			{
				newTag = std::make_shared<el_title>(this_doc);
			} else if(!strcmp(tag_name, "a"))
			{
				newTag = std::make_shared<el_anchor>(this_doc);
			} else if(!strcmp(tag_name, "tr"))
			{
				newTag = std::make_shared<el_tr>(this_doc);
			} else if(!strcmp(tag_name, "style"))
			{
				newTag = std::make_shared<el_style>(this_doc);
			} else if(!strcmp(tag_name, "base"))
			{
				newTag = std::make_shared<el_base>(this_doc);
			} else if(!strcmp(tag_name, "body"))
			{
				newTag = std::make_shared<el_body>(this_doc);
			} else if(!strcmp(tag_name, "div"))
			{
				newTag = std::make_shared<el_div>(this_doc);
			} else if(!strcmp(tag_name, "script"))
			{
				newTag = std::make_shared<el_script>(this_doc);
			} else if(!strcmp(tag_name, "font"))
			{
				newTag = std::make_shared<el_font>(this_doc);
			} else
			{
				newTag = std::make_shared<html_tag>(this_doc);
			}
		}

		if(newTag)
		{
			newTag->set_tagName(tag_name);
			for(const auto& attribute : attributes)
			{
				newTag->set_attr(attribute.first.c_str(), attribute.second.c_str());
			}
		}

		return newTag;
	}

	uint_ptr document::add_font(const font_description& descr, font_metrics* fm)
	{
		uint_ptr ret	= 0;

		std::string key = descr.hash();

		if(m_fonts.find(key) == m_fonts.end())
		{
			font_item fi = {0, {}};

			fi.font		 = m_container->create_font(descr, this, &fi.metrics);
			m_fonts[key] = fi;
			ret			 = fi.font;
			if(fm)
			{
				*fm = fi.metrics;
			}
		}
		return ret;
	}

	uint_ptr document::get_font(const font_description& descr, font_metrics* fm)
	{
		if(descr.size == 0)
		{
			return 0;
		}

		auto key = descr.hash();

		auto el	 = m_fonts.find(key);

		if(el != m_fonts.end())
		{
			if(fm)
			{
				*fm = el->second.metrics;
			}
			return el->second.font;
		}
		return add_font(descr, fm);
	}

	pixel_t document::render(pixel_t max_width, render_type rt)
	{
		pixel_t ret = 0;
		if(m_root && m_root_render)
		{
			// Skip re-render if width is unchanged and no style changes pending.
			// This avoids the expensive tree walk for hover-only changes (background, color).
			if(rt != render_fixed_only && max_width == m_last_render_width && m_root && !m_root->m_style_dirty)
			{
				return m_size.width;
			}

			position viewport;
			m_container->get_viewport(viewport);
			containing_block_context cb_context;
			cb_context.width	   = max_width;
			cb_context.width.type  = containing_block_context::cbc_value_type_absolute;
			cb_context.height	   = viewport.height;
			cb_context.height.type = containing_block_context::cbc_value_type_absolute;

			if(rt == render_fixed_only)
			{
				m_fixed_boxes.clear();
				m_root_render->render_positioned(rt);
			} else
			{
				ret = m_root_render->render(0, 0, cb_context, nullptr);
				if(m_root_render->fetch_positioned())
				{
					m_fixed_boxes.clear();
					m_root_render->render_positioned(rt);
				}
				m_size.width  = 0;
				m_size.height = 0;
				m_root_render->calc_document_size(m_size);
				m_last_render_width = max_width;
			}
		}
		return ret;
	}

	void document::draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip)
	{
		if(m_root && m_root_render)
		{
			m_root->draw(hdc, x, y, clip, m_root_render);
			m_root_render->draw_stacking_context(hdc, x, y, clip, true);
		}
	}

	pixel_t document::to_pixels(const css_length& val, const font_metrics& metrics, pixel_t size) const
	{
		if(val.is_predefined())
		{
			return 0;
		}
		pixel_t ret;
		switch(val.units())
		{
		case css_units_percentage:
			ret = val.calc_percent(size);
			break;
		case css_units_em:
			ret = val.val() * metrics.font_size;
			break;

		// https://drafts.csswg.org/css-values-4/#absolute-lengths
		case css_units_pt:
			ret = m_container->pt_to_px(val.val());
			break;
		case css_units_in:
			ret = m_container->pt_to_px(val.val() * 72); // 1in = 72pt
			break;
		case css_units_pc:
			ret = m_container->pt_to_px(val.val() * 12); // 1pc = (1/6)in = 12pt
			break;
		case css_units_cm:
			ret = m_container->pt_to_px(val.val() * 72 / 2.54f); // 1cm = (1/2.54)in = (72/2.54)pt
			break;
		case css_units_mm:
			ret = m_container->pt_to_px(val.val() * 72 / 2.54f / 10);
			break;

		case css_units_vw:
			ret = (pixel_t) (m_media.width * val.val() / 100.0);
			break;
		case css_units_vh:
			ret = (pixel_t) (m_media.height * val.val() / 100.0);
			break;
		case css_units_vmin:
			ret = (pixel_t) (std::min(m_media.height, m_media.width) * val.val() / 100.0);
			break;
		case css_units_vmax:
			ret = (pixel_t) (std::max(m_media.height, m_media.width) * val.val() / 100.0);
			break;
		case css_units_rem:
			ret = (pixel_t) (m_root->css().get_font_size() * val.val());
			break;
		case css_units_ex:
			ret = (pixel_t) (metrics.x_height * val.val());
			break;
		case css_units_ch:
			ret = (pixel_t) (metrics.ch_width * val.val());
			break;
		default:
			ret = (pixel_t) val.val();
			break;
		}
		return ret;
	}

	void document::cvt_units(css_length& val, const font_metrics& metrics, pixel_t size) const
	{
		if(val.is_predefined())
		{
			return;
		}
		if(val.units() != css_units_percentage)
		{
			val.set_value((float) to_pixels(val, metrics, size), css_units_px);
		}
	}

	pixel_t document::width() const
	{
		return m_size.width;
	}

	pixel_t document::height() const
	{
		return m_size.height;
	}

	void document::add_stylesheet(const char* str, const char* baseurl, const char* media)
	{
		if(str && str[0])
		{
			m_css.emplace_back(str, baseurl, media);
		}
	}

	bool document::on_mouse_over(pixel_t x, pixel_t y, pixel_t client_x, pixel_t client_y,
								 position::vector& redraw_boxes)
	{
		if(!m_root || !m_root_render)
		{
			return false;
		}

		element::ptr over_el   = m_root_render->get_element_by_point(x, y, client_x, client_y, nullptr);

		bool state_was_changed = false;

		if(over_el != m_over_element)
		{
			if(m_over_element)
			{
				if(m_over_element->on_mouse_leave())
				{
					m_container->on_mouse_event(m_over_element, mouse_event_leave);
					state_was_changed = true;
				}
			}
			m_over_element = over_el;
		}

		string cursor;

		if(m_over_element)
		{
			if(m_over_element->on_mouse_over())
			{
				state_was_changed = true;
			}
			cursor = m_over_element->css().get_cursor();
		}

		m_container->set_cursor(cursor.c_str());

		if(state_was_changed)
		{
			m_container->on_mouse_event(m_over_element, mouse_event_enter);
			return m_root->find_styles_changes(redraw_boxes);
		}
		return false;
	}

	std::vector<scroll_values> document::on_scroll(pixel_t dx, pixel_t dy, pixel_t x, pixel_t y, pixel_t client_x,
												   pixel_t client_y) const
	{
		if(dy == 0 && dx == 0)
			return {};

		element::ptr vscroll_el;
		element::ptr hscroll_el;

		if(dy != 0.f)
		{
			vscroll_el = m_root_render->get_element_by_point(
				x, y, client_x, client_y,
				[dy](const shared_ptr<render_item>& el) -> bool { return el->is_v_scrollable(dy); });
		}

		if(dx != 0.f)
		{
			hscroll_el = m_root_render->get_element_by_point(
				x, y, client_x, client_y,
				[dx](const shared_ptr<render_item>& el) -> bool { return el->is_h_scrollable(dx); });
		}

		if(!vscroll_el && !hscroll_el)
			return {};

		if(vscroll_el == hscroll_el)
		{
			scroll_values sv;
			sv.dx		  = hscroll_el->h_scroll(dx);
			sv.dy		  = vscroll_el->v_scroll(dy);
			sv.scroll_box = hscroll_el->get_placement();
			return {sv};
		}

		std::vector<scroll_values> ret;
		ret.reserve(2);

		if(vscroll_el)
		{
			scroll_values sv;
			sv.dy		  = vscroll_el->v_scroll(dy);
			sv.scroll_box = vscroll_el->get_placement();
			ret.push_back(sv);
		}

		if(hscroll_el)
		{
			scroll_values sv;
			sv.dx		  = hscroll_el->h_scroll(dx);
			sv.scroll_box = hscroll_el->get_placement();
			ret.push_back(sv);
		}

		return ret;
	}

	bool document::on_mouse_leave(position::vector& redraw_boxes)
	{
		if(!m_root || !m_root_render)
		{
			return false;
		}
		if(m_over_element)
		{
			auto el		   = m_over_element;
			m_over_element = nullptr;
			if(el->on_mouse_leave())
			{
				m_container->on_mouse_event(el, mouse_event_leave);
				return m_root->find_styles_changes(redraw_boxes);
			}
		}
		return false;
	}

	bool document::on_lbutton_down(pixel_t x, pixel_t y, pixel_t client_x, pixel_t client_y,
								   position::vector& redraw_boxes)
	{
		if(!m_root || !m_root_render)
		{
			return false;
		}

		element::ptr over_el   = m_root_render->get_element_by_point(x, y, client_x, client_y, nullptr);
		m_active_element	   = over_el;

		bool state_was_changed = false;

		if(over_el != m_over_element)
		{
			if(m_over_element)
			{
				if(m_over_element->on_mouse_leave())
				{
					m_container->on_mouse_event(m_over_element, mouse_event_leave);
					state_was_changed = true;
				}
			}
			m_over_element = over_el;
			if(m_over_element)
			{
				if(m_over_element->on_mouse_over())
				{
					state_was_changed = true;
				}
			}
		}

		string cursor;

		if(m_over_element)
		{
			if(m_over_element->on_lbutton_down())
			{
				state_was_changed = true;
			}
			cursor = m_over_element->css().get_cursor();
		}

		m_container->set_cursor(cursor.c_str());

		if(state_was_changed)
		{
			m_container->on_mouse_event(m_over_element, mouse_event_enter);
			return m_root->find_styles_changes(redraw_boxes);
		}

		return false;
	}

	bool document::on_lbutton_up(pixel_t /*x*/, pixel_t /*y*/, pixel_t /*client_x*/, pixel_t /*client_y*/,
								 position::vector& redraw_boxes)
	{
		if(!m_root || !m_root_render)
		{
			return false;
		}
		if(m_over_element)
		{
			if(m_over_element->on_lbutton_up(m_active_element == m_over_element))
			{
				return m_root->find_styles_changes(redraw_boxes);
			}
		}
		return false;
	}

	bool document::on_button_cancel(position::vector& redraw_boxes)
	{
		m_active_element = nullptr;
		return on_mouse_leave(redraw_boxes);
	}

	void document::get_fixed_boxes(position::vector& fixed_boxes)
	{
		fixed_boxes = m_fixed_boxes;
	}

	void document::add_fixed_box(const position& pos)
	{
		m_fixed_boxes.push_back(pos);
	}

	bool document::media_changed()
	{
		container()->get_media_features(m_media);
		if(update_media_lists(m_media))
		{
			m_root->refresh_styles();
			m_root->compute_styles();
			return true;
		}
		return false;
	}

	bool document::lang_changed()
	{
		if(!m_media_lists.empty())
		{
			string culture;
			container()->get_language(m_lang, culture);
			if(!culture.empty())
			{
				m_culture = m_lang + '-' + culture;
			} else
			{
				m_culture.clear();
			}
			m_root->refresh_styles();
			m_root->compute_styles();
			return true;
		}
		return false;
	}

	// Apply media features (determine which selectors are active).
	bool document::update_media_lists(const media_features& features)
	{
		bool update_styles = false;
		for(auto& media_list : m_media_lists)
		{
			if(media_list->apply_media_features(features))
			{
				update_styles = true;
			}
		}
		return update_styles;
	}

	void document::add_media_list(media_query_list_list::ptr list)
	{
		if(list && !contains(m_media_lists, list))
			m_media_lists.push_back(list);
	}

	void document::fix_tables_layout()
	{
		for(const auto& el_ptr : m_tabular_elements)
		{
			switch(el_ptr->src_el()->css().get_display())
			{
			case display_inline_table:
			case display_table:
				fix_table_children(el_ptr, display_table_row_group, "table-row-group");
				break;
			case display_table_footer_group:
			case display_table_row_group:
			case display_table_header_group:
				{
					auto parent = el_ptr->parent();
					if(parent)
					{
						if(parent->src_el()->css().get_display() != display_inline_table)
							fix_table_parent(el_ptr, display_table, "table");
					}
					fix_table_children(el_ptr, display_table_row, "table-row");
				}
				break;
			case display_table_row:
				fix_table_parent(el_ptr, display_table_row_group, "table-row-group");
				fix_table_children(el_ptr, display_table_cell, "table-cell");
				break;
			case display_table_cell:
				fix_table_parent(el_ptr, display_table_row, "table-row");
				break;
			// TODO: make table layout fix for table-caption, table-column etc. elements
			case display_table_caption:
			case display_table_column:
			case display_table_column_group:
			default:
				break;
			}
		}
	}

	void document::fix_table_children(const std::shared_ptr<render_item>& el_ptr, style_display disp,
									  const char* disp_str)
	{
		std::list<std::shared_ptr<render_item>> tmp;
		auto									first_iter = el_ptr->children().begin();
		auto									cur_iter   = el_ptr->children().begin();

		auto flush_elements								   = [&]() {
			   element::ptr annon_tag = std::make_shared<html_tag>(el_ptr->src_el(), string("display:") + disp_str);
			   std::shared_ptr<render_item> annon_ri;
			   if(annon_tag->css().get_display() == display_table_cell)
			   {
				   annon_tag->set_tagName("table_cell");
				   annon_ri = std::make_shared<render_item_block>(annon_tag);
			   } else if(annon_tag->css().get_display() == display_table_row)
			   {
				   annon_ri = std::make_shared<render_item_table_row>(annon_tag);
			   } else
			   {
				   annon_ri = std::make_shared<render_item_table_part>(annon_tag);
			   }
			   for(const auto& el : tmp)
			   {
				   annon_ri->add_child(el);
			   }
			   // add annon item as tabular for future processing
			   add_tabular(annon_ri);
			   annon_ri->parent(el_ptr);
			   first_iter = el_ptr->children().insert(first_iter, annon_ri);
			   cur_iter	  = std::next(first_iter);
			   while(cur_iter != el_ptr->children().end() && (*cur_iter)->parent() != el_ptr)
			   {
				   cur_iter = el_ptr->children().erase(cur_iter);
			   }
			   first_iter = cur_iter;
			   tmp.clear();
		};

		while(cur_iter != el_ptr->children().end())
		{
			if((*cur_iter)->src_el()->css().get_display() != disp)
			{
				if(!(*cur_iter)->src_el()->is_table_skip() || ((*cur_iter)->src_el()->is_table_skip() && !tmp.empty()))
				{
					if(disp != display_table_row_group ||
					   (*cur_iter)->src_el()->css().get_display() != display_table_caption)
					{
						if(tmp.empty())
						{
							first_iter = cur_iter;
						}
						tmp.push_back((*cur_iter));
					}
				}
				cur_iter++;
			} else if(!tmp.empty())
			{
				flush_elements();
			} else
			{
				cur_iter++;
			}
		}
		if(!tmp.empty())
		{
			flush_elements();
		}
	}

	void document::fix_table_parent(const std::shared_ptr<render_item>& el_ptr, style_display disp,
									const char* disp_str)
	{
		auto parent = el_ptr->parent();

		if(parent->src_el()->css().get_display() != disp)
		{
			auto this_element = std::find_if(parent->children().begin(), parent->children().end(),
											 [&](const std::shared_ptr<render_item>& el) {
												 if(el == el_ptr)
												 {
													 return true;
												 }
												 return false;
											 });
			if(this_element != parent->children().end())
			{
				style_display el_disp = el_ptr->src_el()->css().get_display();
				auto		  first	  = this_element;
				auto		  last	  = this_element;
				auto		  cur	  = this_element;

				// find first element with same display
				while(true)
				{
					if(cur == parent->children().begin())
						break;
					cur--;
					if((*cur)->src_el()->is_table_skip() || (*cur)->src_el()->css().get_display() == el_disp)
					{
						first = cur;
					} else
					{
						break;
					}
				}

				// find last element with same display
				cur = this_element;
				while(true)
				{
					cur++;
					if(cur == parent->children().end())
						break;

					if((*cur)->src_el()->is_table_skip() || (*cur)->src_el()->css().get_display() == el_disp)
					{
						last = cur;
					} else
					{
						break;
					}
				}

				// extract elements with the same display and wrap them with anonymous object
				element::ptr annon_tag = std::make_shared<html_tag>(parent->src_el(), string("display:") + disp_str);
				std::shared_ptr<render_item> annon_ri;
				if(annon_tag->css().get_display() == display_table ||
				   annon_tag->css().get_display() == display_inline_table)
				{
					annon_ri = std::make_shared<render_item_table>(annon_tag);
				} else if(annon_tag->css().get_display() == display_table_row)
				{
					annon_ri = std::make_shared<render_item_table_row>(annon_tag);
				} else
				{
					annon_ri = std::make_shared<render_item_table_part>(annon_tag);
				}
				std::for_each(first, std::next(last, 1),
							  [&annon_ri](std::shared_ptr<render_item>& el) { annon_ri->add_child(el); });
				first = parent->children().erase(first, std::next(last));
				parent->children().insert(first, annon_ri);
				add_tabular(annon_ri);
				annon_ri->parent(parent);
			}
		}
	}

	void document::append_children_from_string(element& parent, const char* str, bool replace_existing)
	{
		// parent must belong to this document
		if(parent.get_document().get() != this)
		{
			return;
		}

		GumboOptions opts	  = kGumboDefaultOptions;
		// This is require to prevent creating html, head, body tags around the fragment
		// Although Gumbo always creates html tag anyway. We have to ignore it in create_node.
		opts.fragment_context = GUMBO_TAG_BODY;
		// parse document into GumboOutput
		GumboOutput* output	  = gumbo_parse_with_options(&opts, str, strlen(str));

		// Create litehtml::elements.
		elements_list child_elements;
		// Create elements excluding the root node
		create_node(output->root, child_elements, true, false);

		// Destroy GumboOutput
		gumbo_destroy_output(&kGumboDefaultOptions, output);

		auto parent_render = parent.get_render_item();

		if(replace_existing)
		{
			parent.clearRecursive();
			parent_render->children().clear();
		}

		// Let's process created elements tree
		for(const auto& child : child_elements)
		{
			// Add the child element to parent
			parent.appendChild(child);

			// apply master CSS
			child->apply_stylesheet(m_master_css);

			// parse elements attributes
			child->parse_attributes();

			// Apply parsed styles.
			child->apply_stylesheet(m_styles);

			// Apply user styles if any
			child->apply_stylesheet(m_user_css);

			// Initialize m_css
			child->compute_styles();

			// Finally initialize elements
			if(parent_render)
			{
				auto child_render = child->create_render_item(parent_render);
				if(child_render)
				{
					child_render = child_render->init();
					parent_render->add_child(child_render);
				}
			}
		}
		// Now the m_tabular_elements is filled with tabular elements.
		// We have to check the tabular elements for missing table elements
		// and create the anonymous boxes in visual table layout
		fix_tables_layout();
	}

	void document::dump(dumper& cout)
	{
		if(m_root_render)
		{
			m_root_render->dump(cout);
		}
	}

} // namespace litehtml
