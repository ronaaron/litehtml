#ifndef LITEHTML_FLEX_ITEM_H
#define LITEHTML_FLEX_ITEM_H

#include "formatting_context.h"
#include "render_item.h"

namespace litehtml
{
	class flex_line;

	enum flex_clamp_state
	{
		flex_clamp_state_unclamped,
		flex_clamp_state_inflexible,
		flex_clamp_state_min_violation,
		flex_clamp_state_max_violation
	};

	/**
	 * Concrete class for flex item (data-oriented, no inheritance)
	 */
	class flex_item
	{
	public:
		render_item* el;

		// All sizes should be interpreted as outer/margin-box sizes.
		pixel_t base_size;
		pixel_t min_size;
		def_value<pixel_t> max_size;
		pixel_t main_size; // Holds the outer hypothetical main size before distribute_free_space, and the used outer main size after.

		int grow;
		int shrink;
		pixel_t scaled_flex_shrink_factor;

		bool frozen;
		flex_clamp_state clamp_state;

		int order;
		int src_order;

		def_value<pixel_t> auto_margin_main_start;
		def_value<pixel_t> auto_margin_main_end;
		bool auto_margin_cross_start;
		bool auto_margin_cross_end;

		flex_align_items align;

		bool is_row_direction;

		flex_item() : 
				el(nullptr),
				base_size(0),
				min_size(0),
				max_size(0),
				main_size(0),
				grow(0),
				shrink(0),
				scaled_flex_shrink_factor(0),
				frozen(false),
				clamp_state(flex_clamp_state_unclamped),
				order(0),
				src_order(0),
				auto_margin_main_start(0),
				auto_margin_main_end(0),
				auto_margin_cross_start(false),
				auto_margin_cross_end(false),
				align(flex_align_items_auto),
				is_row_direction(true)
		{}

		flex_item(render_item* _el, bool _is_row) :
				el(_el),
				base_size(0),
				min_size(0),
				max_size(0),
				main_size(0),
				grow(0),
				shrink(0),
				scaled_flex_shrink_factor(0),
				frozen(false),
				clamp_state(flex_clamp_state_unclamped),
				order(0),
				src_order(0),
				auto_margin_main_start(0),
				auto_margin_main_end(0),
				auto_margin_cross_start(false),
				auto_margin_cross_end(false),
				align(flex_align_items_auto),
				is_row_direction(_is_row)
		{}

		bool operator<(const flex_item& b) const
		{
			if(order < b.order) return true;
			if(order == b.order) return src_order < b.src_order;
			return false;
		}

		void init(const litehtml::containing_block_context &self_size,
				  litehtml::formatting_context *fmt_ctx, flex_align_items align_items);
		
		void apply_main_auto_margins();
		bool apply_cross_auto_margins(pixel_t cross_size);
		void set_main_position(pixel_t pos);
		void set_cross_position(pixel_t pos);
		pixel_t get_el_main_size();
		pixel_t get_el_cross_size();

		void place(flex_line &ln, pixel_t main_pos,
				   const containing_block_context &self_size,
				   formatting_context *fmt_ctx);
		pixel_t get_last_baseline(baseline::_baseline_type type) const;
		pixel_t get_first_baseline(baseline::_baseline_type type) const;

	protected:
		void direction_specific_init(const litehtml::containing_block_context &self_size,
											 litehtml::formatting_context *fmt_ctx);
		void align_stretch(flex_line &ln, const containing_block_context &self_size,
								   formatting_context *fmt_ctx);
		void align_baseline(flex_line &ln,
									const containing_block_context &self_size,
									formatting_context *fmt_ctx);
	};
}

#endif //LITEHTML_FLEX_ITEM_H
