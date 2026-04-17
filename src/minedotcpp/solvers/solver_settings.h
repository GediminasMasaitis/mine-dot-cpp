#pragma once

#include "../mine_api.h"

namespace minedotcpp
{
	namespace solvers
	{
		struct solver_settings
		{
		public:
			bool trivial_solve = true;
			bool trivial_stop_on_no_mine_verdict = true;
			bool trivial_stop_on_any_verdict = false;
			bool trivial_stop_always = false;

			bool gaussian_solve = true;
			bool gaussian_resolve_on_success = true;
			bool gaussian_single_stop_on_no_mine_verdict = true;
			bool gaussian_single_stop_on_any_verdict = false;
			bool gaussian_single_stop_always = false;
			bool gaussian_all_stop_on_no_mine_verdict = true;
			bool gaussian_all_stop_on_any_verdict = false;
			bool gaussian_all_stop_always = false;

			bool separation_solve = true;
			bool separation_order_borders_by_size = true;
			bool separation_order_borders_by_size_descending = false;
			bool separation_single_border_stop_on_no_mine_verdict = true;
			bool separation_single_border_stop_on_any_verdict = false;
			bool separation_single_border_stop_always = false;

			bool partial_solve = true;
			bool partial_single_stop_on_no_mine_verdict = false;
			bool partial_single_stop_on_any_verdict = false;
			bool partial_all_stop_on_no_mine_verdict = true;
			bool partial_all_stop_on_any_verdict = false;
			bool partial_stop_always = false;
			int partial_solve_from_size = 24;
			int partial_optimal_size = 20;
			bool partial_set_probability_guesses = true;

			bool resplit_on_partial_verdict = true;
			bool resplit_on_complete_verdict = false;

			bool mine_count_ignore_completely = false;
			bool mine_count_solve = true;
			bool mine_count_solve_non_border = true;

			int give_up_from_size = 28;

			bool valid_combination_search_open_cl = true;
			bool valid_combination_search_open_cl_allow_loop_break = false;
			int valid_combination_search_open_cl_use_from_size = 16;
			int valid_combination_search_open_cl_max_batch_size = 31;
			int valid_combination_search_open_cl_platform_id = 0;
			int valid_combination_search_open_cl_device_id = 0;

			bool valid_combination_search_multithread = true;
			int valid_combination_search_multithread_use_from_size = 8;
			int valid_combination_search_multithread_thread_count = 16;

			bool combination_search_gaussian_reduction = true;
			bool combination_search_gaussian_backtracking = true;

			// When true, partial_solve is skipped for borders that the full
			// enumeration will handle anyway (effective_size <= give_up_from_size).
			// In theory partial's verdicts are a subset of what full enum
			// produces, but in practice partial-found verdicts can shrink the
			// border (via resplit_on_partial_verdict or direct cell removal)
			// and make the subsequent full enum cheaper. Empirically on expert
			// boards this is a small net positive, so the default keeps the
			// legacy "always run partial" behavior. Flip to true if you want to
			// skip partial for tractable borders.
			bool partial_solve_only_when_giving_up = false;

			// Master switch for [trace] output. When false, no traces are emitted and
			// the (cheap) timing measurements are skipped entirely.
			bool print_trace = false;
			// Threshold for emitting per-border traces. Borders whose effective_size
			// (free variable count after RREF, or raw cell count when reduction fails)
			// is at least this value emit full downstream traces. Set to 0 to trace
			// every border.
			int print_trace_min_effective_size = 20;
			// Threshold in microseconds for emitting the top-level solve() summary.
			// solve() calls faster than this are not printed. Set to 0 to print every
			// solve().
			long long print_trace_min_solve_us = 100000;

			int variable_mine_count_borders_probabilities_multithread_use_from = 128;
			int variable_mine_count_borders_probabilities_give_up_from = 131072;

			bool guess_if_no_no_mine_verdict = true;
			bool guess_if_no_verdict = false;

			int debug_setting_1 = 0;
			int debug_setting_2 = 0;
			int debug_setting_3 = 0;
		};
	}
}
