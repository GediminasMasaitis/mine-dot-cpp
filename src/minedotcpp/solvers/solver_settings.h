#pragma once

namespace monedotcpp
{
	namespace solvers
	{
		struct solver_settings
		{
		public:
			bool stop_on_no_mine_verdict;
			bool stop_on_any_verdict;

			bool solve_trivial;
			bool stop_after_trivial_solving;
				
			bool solve_gaussian;
			bool stop_after_gaussian_solving;

			bool ignore_mine_count_completely;
			bool solve_by_mine_count;
			bool solve_non_border_cells;

			bool solve_hint_probabilities;

			bool partial_border_solving;
			bool border_resplitting;
			int partial_border_solve_from;
			int give_up_from;
			int max_partial_border_size;
			bool set_partially_calculated_probabilities;
		};
	}
}
