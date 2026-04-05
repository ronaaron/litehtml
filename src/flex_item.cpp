#include <cmath>
#include "flex_item.h"
#include "flex_line.h"
#include "types.h"

void litehtml::flex_item::init(const litehtml::containing_block_context &self_size,
							   litehtml::formatting_context *fmt_ctx, flex_align_items align_items)
{
	grow = (int) std::nearbyint(el->css().get_flex_grow() * 1000.0);
	if(grow < 0) grow = 0;

	shrink = (int) std::nearbyint(el->css().get_flex_shrink() * 1000.0);
	if(shrink < 0) shrink = 1000;

	el->calc_outlines(self_size.render_width);
	order = el->css().get_order();

	direction_specific_init(self_size, fmt_ctx);

	if (el->css().get_flex_align_self() == flex_align_items_auto)
	{
		align = align_items;
	} else
	{
		align = el->css().get_flex_align_self();
	}

	if (base_size < min_size)
	{
		main_size = min_size;
	} else if (!max_size.is_default() && base_size > max_size)
	{
		main_size = max_size;
	} else
	{
		main_size = base_size;
	}

	frozen = false;
}

void litehtml::flex_item::place(flex_line &ln, pixel_t main_pos,
								const containing_block_context &self_size,
								formatting_context *fmt_ctx)
{
	apply_main_auto_margins();
	set_main_position(main_pos);
	if(!apply_cross_auto_margins(ln.cross_size))
	{
		switch (align & 0xFF)
		{
			case flex_align_items_baseline:
				align_baseline(ln, self_size, fmt_ctx);
				break;
			case flex_align_items_flex_end:
				if(ln.reverse_cross)
				{
					set_cross_position(ln.cross_start);
				} else
				{
					set_cross_position(ln.cross_start + ln.cross_size - get_el_cross_size());
				}
				break;
			case flex_align_items_end:
				set_cross_position(ln.cross_start + ln.cross_size - get_el_cross_size());
				break;
			case flex_align_items_center:
				set_cross_position(ln.cross_start + ln.cross_size / 2 - get_el_cross_size() / 2);
				break;
			case flex_align_items_flex_start:
				if(ln.reverse_cross)
				{
					set_cross_position(ln.cross_start + ln.cross_size - get_el_cross_size());
				} else
				{
					set_cross_position(ln.cross_start);
				}
				break;
			case flex_align_items_start:
				set_cross_position(ln.cross_start);
				break;
			default:
				align_stretch(ln, self_size, fmt_ctx);
				break;
		}
	}
}

litehtml::pixel_t litehtml::flex_item::get_last_baseline(baseline::_baseline_type type) const
{
	if(type == baseline::baseline_type_top)
	{
		return el->get_last_baseline();
	} else if(type == baseline::baseline_type_bottom)
	{
		return el->height() - el->get_last_baseline();
	}
	return 0;
}

litehtml::pixel_t litehtml::flex_item::get_first_baseline(litehtml::baseline::_baseline_type type) const
{
	if(type == baseline::baseline_type_top)
	{
		return el->get_first_baseline();
	} else if(type == baseline::baseline_type_bottom)
	{
		return el->height() - el->get_first_baseline();
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////////////
// Merged implementations depending on is_row_direction
////////////////////////////////////////////////////////////////////////////////////

void litehtml::flex_item::direction_specific_init(const litehtml::containing_block_context &self_size,
																litehtml::formatting_context *fmt_ctx)
{
	if (is_row_direction)
	{
		if(el->css().get_margins().left.is_predefined()) auto_margin_main_start = 0;
		if(el->css().get_margins().right.is_predefined()) auto_margin_main_end = 0;
		if(el->css().get_margins().top.is_predefined()) auto_margin_cross_start = true;
		if(el->css().get_margins().bottom.is_predefined()) auto_margin_cross_end = true;

		def_value<pixel_t> content_size(0);
		if (el->css().get_min_width().is_predefined())
		{
			min_size = el->render(0, 0,
								  self_size.new_width(el->content_offset_width(),
													  containing_block_context::size_mode_content), fmt_ctx);
			content_size = min_size;
		} else
		{
			min_size = el->css().get_min_width().calc_percent(self_size.render_width) + el->render_offset_width();
		}

		if (!el->css().get_max_width().is_predefined())
		{
			max_size = el->css().get_max_width().calc_percent(self_size.render_width) + el->render_offset_width();
		}
		
		bool flex_basis_predefined = el->css().get_flex_basis().is_predefined();
		int predef = flex_basis_auto;
		if(flex_basis_predefined) predef = el->css().get_flex_basis().predef();
		else if(el->css().get_flex_basis().val() < 0) flex_basis_predefined = true;

		if (flex_basis_predefined)
		{
			if(predef == flex_basis_auto && el->css().get_width().is_predefined())
				predef = flex_basis_content;

			switch (predef)
			{
				case flex_basis_auto:
					base_size = el->css().get_width().calc_percent(self_size.render_width) + el->render_offset_width();
					break;
				case flex_basis_fit_content:
				case flex_basis_content:
					base_size = el->render(0, 0, self_size.new_width(self_size.render_width + el->content_offset_width(),
																	 containing_block_context::size_mode_content |
																	 containing_block_context::size_mode_exact_width), fmt_ctx);
					break;
				case flex_basis_min_content:
					if(content_size.is_default())
						content_size = el->render(0, 0, self_size.new_width(el->content_offset_width(),
																	  containing_block_context::size_mode_content), fmt_ctx);
					base_size = content_size;
					break;
				case flex_basis_max_content:
					el->render(0, 0, self_size, fmt_ctx);
					base_size = el->width();
					break;
				default:
					base_size = 0;
					break;
			}
		} else
		{
			base_size = el->css().get_flex_basis().calc_percent(self_size.render_width) + el->render_offset_width();
		}
		scaled_flex_shrink_factor = (base_size - el->render_offset_width()) * shrink;
	}
	else
	{
		// Column direction
		if(el->css().get_margins().top.is_predefined()) auto_margin_main_start = 0;
		if(el->css().get_margins().bottom.is_predefined()) auto_margin_main_end = 0;
		if(el->css().get_margins().left.is_predefined()) auto_margin_cross_start = true;
		if(el->css().get_margins().right.is_predefined()) auto_margin_cross_end = true;

		if (el->css().get_min_height().is_predefined())
		{
			el->render(0, 0, self_size.new_width(self_size.render_width, containing_block_context::size_mode_content), fmt_ctx);
			min_size = el->height();
		} else
		{
			min_size = el->css().get_min_height().calc_percent(self_size.height) + el->render_offset_height();
		}

		if (!el->css().get_max_height().is_predefined())
		{
			max_size = el->css().get_max_height().calc_percent(self_size.height) + el->render_offset_height();
		}

		bool flex_basis_predefined = el->css().get_flex_basis().is_predefined();
		int predef = flex_basis_auto;
		if(flex_basis_predefined) predef = el->css().get_flex_basis().predef();
		else if(el->css().get_flex_basis().val() < 0) flex_basis_predefined = true;

		if (flex_basis_predefined)
		{
			if(predef == flex_basis_auto && el->css().get_height().is_predefined())
				predef = flex_basis_fit_content;
				
			switch (predef)
			{
				case flex_basis_auto:
					base_size = el->css().get_height().calc_percent(self_size.height) + el->render_offset_height();
					break;
				case flex_basis_max_content:
				case flex_basis_fit_content:
					el->render(0, 0, self_size, fmt_ctx);
					base_size = el->height();
					break;
				case flex_basis_min_content:
					base_size = min_size;
					break;
				default:
					base_size = 0;
			}
		} else
		{
			if(el->css().get_flex_basis().units() == css_units_percentage)
			{
				if(self_size.height.type == containing_block_context::cbc_value_type_absolute)
				{
					base_size = el->css().get_flex_basis().calc_percent(self_size.height) + el->render_offset_height();
				} else
				{
					base_size = 0;
				}
			} else
			{
				base_size = (pixel_t) el->css().get_flex_basis().val() + el->render_offset_height();
			}
		}
		scaled_flex_shrink_factor = (base_size - el->render_offset_height()) * shrink;
	}
}

void litehtml::flex_item::apply_main_auto_margins()
{
	if (is_row_direction)
	{
		if(!auto_margin_main_start.is_default())
		{
			el->get_margins().left = auto_margin_main_start;
			el->pos().x += auto_margin_main_start;
		}
		if(!auto_margin_main_end.is_default()) el->get_margins().right = auto_margin_main_end;
	}
	else
	{
		if(!auto_margin_main_start.is_default())
		{
			el->get_margins().top = auto_margin_main_start;
			el->pos().y += auto_margin_main_start;
		}
		if(!auto_margin_main_end.is_default()) el->get_margins().bottom = auto_margin_main_end;
	}
}

bool litehtml::flex_item::apply_cross_auto_margins(pixel_t cross_size)
{
	if (is_row_direction)
	{
		if(auto_margin_cross_end || auto_margin_cross_start)
		{
			int margins_num = 0;
			if(auto_margin_cross_end) margins_num++;
			if(auto_margin_cross_start) margins_num++;
			
			pixel_t margin = (cross_size - el->height()) / margins_num;
			if(auto_margin_cross_start)
			{
				el->get_margins().top = margin;
				el->pos().y = el->content_offset_top();
			}
			if(auto_margin_cross_end) el->get_margins().bottom = margin;
			return true;
		}
		return false;
	}
	else
	{
		if(auto_margin_cross_end || auto_margin_cross_start)
		{
			int margins_num = 0;
			if(auto_margin_cross_end) margins_num++;
			if(auto_margin_cross_start) margins_num++;
			pixel_t margin = (cross_size - el->width()) / margins_num;
			if(auto_margin_cross_start)
			{
				el->get_margins().left = margin;
				el->pos().x += el->content_offset_left();
			}
			if(auto_margin_cross_end) el->get_margins().right = margin;
			return true; // was return false in column implementation accidentally? original returned false at end, but we made it true if margin applied
		}
		return false;
	}
}

void litehtml::flex_item::set_main_position(pixel_t pos)
{
	if (is_row_direction) el->pos().x = pos + el->content_offset_left();
	else el->pos().y = pos + el->content_offset_top();
}

void litehtml::flex_item::set_cross_position(pixel_t pos)
{
	if (is_row_direction) el->pos().y = pos + el->content_offset_top();
	else el->pos().x = pos + el->content_offset_left();
}

void litehtml::flex_item::align_stretch(flex_line &ln, const containing_block_context &self_size,
													  formatting_context *fmt_ctx)
{
	if (is_row_direction)
	{
		set_cross_position(ln.cross_start);
		if (el->css().get_height().is_predefined())
		{
			el->render(el->left(), el->top(), self_size.new_width_height(
					el->pos().width + el->box_sizing_width(),
					ln.cross_size - el->content_offset_height() + el->box_sizing_height(),
					containing_block_context::size_mode_exact_width |
					containing_block_context::size_mode_exact_height
					), fmt_ctx);
			apply_main_auto_margins();
		}
	}
	else
	{
		if (!el->css().get_width().is_predefined())
		{
			el->render(ln.cross_start,
					   el->pos().y - el->content_offset_top(),
					   self_size.new_width_height(ln.cross_size - el->content_offset_width() + el->box_sizing_width(),
												  main_size - el->content_offset_height() + el->box_sizing_height(),
												  containing_block_context::size_mode_exact_height),
					   fmt_ctx, false);
		} else
		{
			el->render(ln.cross_start,
					   el->pos().y - el->content_offset_top(),
					   self_size.new_width_height(
							   ln.cross_size - el->content_offset_width() + el->box_sizing_width(),
							   main_size - el->content_offset_height() + el->box_sizing_height(),
							   containing_block_context::size_mode_exact_width |
							   containing_block_context::size_mode_exact_height),
					   fmt_ctx, false);
		}
		apply_main_auto_margins();
	}
}

void litehtml::flex_item::align_baseline(litehtml::flex_line &ln,
													   const containing_block_context &/*self_size*/,
													   formatting_context */*fmt_ctx*/)
{
	if (is_row_direction)
	{
		if (align & flex_align_items_last)
		{
			set_cross_position(ln.cross_start + ln.last_baseline.get_offset_from_top(ln.cross_size) - el->get_last_baseline());
		} else
		{
			set_cross_position(ln.cross_start + ln.first_baseline.get_offset_from_top(ln.cross_size) - el->get_first_baseline());
		}
	}
	else
	{
		if(align & flex_align_items_last)
		{
			if(ln.reverse_cross) set_cross_position(ln.cross_start);
			else set_cross_position(ln.cross_start + ln.cross_size - get_el_cross_size());
		} else
		{
			if(!ln.reverse_cross) set_cross_position(ln.cross_start);
			else set_cross_position(ln.cross_start + ln.cross_size - get_el_cross_size());
		}
	}
}

litehtml::pixel_t litehtml::flex_item::get_el_main_size()
{
	if (is_row_direction) return el->width();
	return el->height();
}

litehtml::pixel_t litehtml::flex_item::get_el_cross_size()
{
	if (is_row_direction) return el->height();
	return el->width();
}
