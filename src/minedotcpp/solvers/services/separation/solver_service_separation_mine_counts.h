#pragma once
#include "../../../common/map.h"
#include "../../solver_map.h"
#include "../../solver_settings.h"
#include "../../border.h"
#include "../../partial_border_data.h"
#include "../solver_service_base.h"
#include <mutex>


namespace minedotcpp
{
	namespace solvers
	{
		namespace services
		{
			class solver_service_separation_mine_counts : private solver_service_base
			{
			public:
				explicit solver_service_separation_mine_counts(const solver_settings& settings, ctpl::thread_pool* thr_pool)
					: solver_service_base(settings, thr_pool)
				{
				}

				void solve_mine_counts(solver_map& m, border& common_border, std::vector<border>& borders, common::point_map<double>& all_probabilities, common::point_map<bool>& all_verdicts) const;

			private:
				void trim_valid_combinations_by_mine_count(border& b, int minesRemaining, int undecidedCellsRemaining, int minesElsewhere, int nonMineCountElsewhere) const;
				bool is_prediction_valid_by_mine_count(int mine_prediction_count, int total_combination_length, int mines_remaining, int undecided_cells_remaining, int mines_elsewhere, int non_mine_count_elsewhere) const;
				void thr_mine_counts(std::vector<border>& variable_borders, int min, int max, int mines_remaining, int mines_elsewhere, int non_border_cell_count, int total_combination_length, int undecided_cells_remaining, int non_mine_count_elsewhere, google::dense_hash_map<int, double>& ratios, google::dense_hash_map<int, double>& non_border_mine_counts, common::point_map<double>& counts, std::mutex& common_lock, common::point_map<std::mutex*>& count_locks) const;
				bool get_variable_mine_count_borders_probabilities(std::vector<border>& variable_borders, int minesRemaining, int undecided_cells_remaining, int non_border_cell_count, int minesElsewhere, int non_mine_count_elsewhere, common::point_map<double>& targetProbabilities, google::dense_hash_map<int, double>& nonBorderMineCountProbabilities) const;
				void get_non_border_probabilities_by_mine_count(solver_map& map, common::point_map<double>& common_border_probabilities, std::vector<common::cell>& non_border_undecided_cells, common::point_map<double>& probabilities) const;
			};
		}
	}
}
