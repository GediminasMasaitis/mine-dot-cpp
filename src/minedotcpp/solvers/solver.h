#pragma once
#include "../mine_api.h"
#include "solver_settings.h"
#include "../common/point.h"
#include "solver_result.h"
#include "../common/map.h"
#include "border.h"

#include <unordered_map>

namespace minedotcpp
{
	namespace solvers
	{
		class solver_map;

		class MINE_API solver
		{
		public:
			solver_settings settings;

			explicit solver(solver_settings& settings);
			common::point_map<solver_result>* solve(const common::map& map) const;

		private:

			void solve_trivial(solver_map& m, common::point_map<bool>& verdicts) const;
			void solve_separation(solver_map& m, common::point_map<double>& probabilities, common::point_map<bool>& verdicts) const;

			void solve_border(border& b, solver_map& m, bool allow_partial_border_solving, std::vector<border>& borders) const;
			void find_valid_border_cell_combinations(solver_map& map, border& border) const;
			bool is_prediction_valid(solver_map& map, border& b, unsigned prediction, std::vector<common::cell>& empty_cells) const;
			int SWAR(int i) const;
			void calculate_border_probabilities(border& b) const;
			void get_verdicts_from_probabilities(common::point_map<double>& probabilities, common::point_map<bool>& target_verdicts) const;
			void get_partial_border(solver_map& m, common::point_set& allowed_coordinates, common::point target_coordinate, border& target_border) const;
			void separate_borders(solver_map& m, border& common_border, std::vector<border>& target_borders) const;
			bool is_cell_border(solver_map& m, common::cell& c) const;
			void find_common_border(solver_map& m, border& common_border) const;

			bool should_stop_solving(common::point_map<bool>& verdicts) const;
			common::point_map<solver_result>* get_final_results(common::point_map<double>& probabilities, common::point_map<bool>& verdicts) const;
		};
	}
}
