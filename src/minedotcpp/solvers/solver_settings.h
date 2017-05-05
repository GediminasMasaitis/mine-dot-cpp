#pragma once

#include "../mine_api.h"

namespace minedotcpp
{
	namespace solvers
	{
		struct MINE_API solver_settings
		{
		public:
			bool stop_on_no_mine_verdict = false;
			bool stop_on_any_verdict = false;

			bool solve_trivial = true;
			bool stop_after_trivial_solving = false;

			bool solve_gaussian = true;
			bool stop_after_gaussian_solving = false;

			bool ignore_mine_count_completely = false;
			bool solve_by_mine_count = true;
			bool solve_non_border_cells = true;

			bool partial_border_solving = true;
			bool border_resplitting = true;
			int partial_border_solve_from = 18;
			int multithread_from = 22;
			int give_up_from = 28;
			int max_partial_border_size = 14;
			bool set_partially_calculated_probabilities = true;
		};
	}
}
