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

			bool gaussian_solve = false;
			bool gaussian_stop_on_no_mine_verdict = true;
			bool gaussian_stop_on_any_verdict = false;
			bool gaussian_stop_always = false;

			bool separation_solve = true;
			bool separation_single_border_stop_on_no_mine_verdict = true;
			bool separation_single_border_stop_on_any_verdict = false;
			bool separation_single_border_stop_always = false;

			bool partial_solve = false;
			bool partial_single_stop_on_no_mine_verdict = false;
			bool partial_single_stop_on_any_verdict = false;
			bool partial_all_stop_on_no_mine_verdict = true;
			bool partial_all_stop_on_any_verdict = false;
			bool partial_stop_always = true;
			int partial_solve_from_size = 20;
			int partial_optimal_size = 14;
			bool partial_set_probability_guesses = true;

			bool resplit_on_partial_verdict = true;
			bool resplit_on_complete_verdict = false;

			bool mine_count_ignore_completely = false;
			bool mine_count_solve = false;
			bool mine_count_solve_non_border = true;

			int give_up_from_size = 31;

			bool valid_combination_search_open_cl = true;
			bool valid_combination_search_open_cl_allow_loop_break = true;
			int valid_combination_search_open_cl_use_from_size = 18;
			int valid_combination_search_open_cl_max_batch_size = 20;
			int valid_combination_search_open_cl_platform_id = 1;
			int valid_combination_search_open_cl_device_id = 0;

			bool valid_combination_search_multithread = true;
			int valid_combination_search_multithread_use_from_size = 22; //2097152

			int variable_mine_count_borders_probabilities_multithread_use_from = 65536;

			bool guess_if_no_no_mine_verdict = false;
			bool guess_if_no_verdict = false;
		};
	}
}
