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
			bool gaussian_stop_on_no_mine_verdict = true;
			bool gaussian_stop_on_any_verdict = false;
			bool gaussian_stop_always = false;

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
			int partial_optimal_size = 14;
			bool partial_set_probability_guesses = true;

			bool resplit_on_partial_verdict = true;
			bool resplit_on_complete_verdict = false;

			bool mine_count_ignore_completely = false;
			bool mine_count_solve = true;
			bool mine_count_solve_non_border = true;

			int give_up_from_size = 28;

			bool valid_combination_search_open_cl = true;
			bool valid_combination_search_open_cl_allow_loop_break = true;
			int valid_combination_search_open_cl_use_from_size = 16;
			int valid_combination_search_open_cl_max_batch_size = 20;
			int valid_combination_search_open_cl_platform_id = 1;
			int valid_combination_search_open_cl_device_id = 0;

			bool valid_combination_search_multithread = true;
			int valid_combination_search_multithread_use_from_size = 8;

			int variable_mine_count_borders_probabilities_multithread_use_from = 128;
			int variable_mine_count_borders_probabilities_give_up_from = 131072;

			bool guess_if_no_no_mine_verdict = true;
			bool guess_if_no_verdict = false;
		};
	}
}
