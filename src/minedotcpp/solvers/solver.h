#pragma once
#include "solver_settings.h"
#include <unordered_map>
#include "../common/point.h"
#include "solver_result.h"
#include "../common/map.h"
#include "border.h"

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

			void solve_trivial(solver_map& m, std::unordered_map<common::point, bool, common::point_hash>& verdicts) const;
			void solve_separation(solver_map& m, std::unordered_map<common::point, double, common::point_hash>& probabilities, std::unordered_map<common::point, bool, common::point_hash>& verdicts) const;

			void solve_border(border& b, solver_map& m, bool allow_partial_border_solving, std::vector<border> borders);
			void find_valid_border_cell_combinations(solver_map& map, border& border);
			bool is_prediction_valid(solver_map& map, std::unordered_map<common::point, bool, common::point_hash>& prediction, std::vector<common::cell>& empty_cells);
			int SWAR(int i);
			void get_partial_border(solver_map& m, common::point_set& allowed_coordinates, common::point target_coordinate, border& target_border) const;
			void separate_borders(solver_map& m, border& common_border, std::vector<border> target_borders) const;
			bool is_cell_border(solver_map& m, common::cell& c) const;
			void find_common_border(solver_map& m, border& common_border) const;

			bool should_stop_solving(std::unordered_map<common::point, bool, common::point_hash>& verdicts) const;
			std::unordered_map<common::point, solver_result, common::point_hash>* get_final_results(std::unordered_map<common::point, double, common::point_hash>& probabilities, std::unordered_map<common::point, bool, common::point_hash>& verdicts) const;
		};
	}
}
