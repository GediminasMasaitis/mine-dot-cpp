#pragma once

#include "../mine_api.h"

namespace minedotcpp
{
	namespace solvers
	{
		struct MINE_API solver_settings
		{
		public:
			bool trivial_solve = true;
			bool trivial_stop_on_no_mine_verdict = false;
			bool trivial_stop_on_any_verdict = false;
			bool trivial_stop_always = false;

			bool gaussian_solve = true;
			bool gaussian_stop_on_no_mine_verdict = false;
			bool gaussian_stop_on_any_verdict = false;
			bool gaussian_stop_always = false;

			bool partial_solve = true;
			bool partial_single_stop_on_no_mine_verdict = false;
			bool partial_single_stop_on_any_verdict = false;
			bool partial_all_stop_on_no_mine_verdict = false;
			bool partial_all_stop_on_any_verdict = false;
			bool partial_stop_always = false;
			int partial_solve_from_size = 22;
			int partial_optimal_size = 17;
			bool partial_set_probability_guesses = true;

			bool resplit_on_partial_verdict = false;
			bool resplit_on_complete_verdict = false;

			bool mine_count_ignore_completely = false;
			bool mine_count_solve = true;
			bool mine_count_solve_non_border = true;

			int give_up_from_size = 28;
			int multithread_valid_combination_search_from_size = 21; //2097152
			int multithread_variable_mine_count_borders_probabilities = 21;
		};
	}
}
