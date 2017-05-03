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

			border* get_partial_border(solver_map& m, common::point_set& allowed_coordinates, common::point target_coordinate) const;
			std::vector<border*>* separate_borders(solver_map& m, border& common_border) const;
			bool is_cell_border(solver_map& m, common::cell& c) const;
			border* find_common_border(solver_map& m) const;

			bool should_stop_solving(std::unordered_map<common::point, bool, common::point_hash>& verdicts) const;
			std::unordered_map<common::point, solver_result, common::point_hash>* get_final_results(std::unordered_map<common::point, double, common::point_hash>& probabilities, std::unordered_map<common::point, bool, common::point_hash>& verdicts) const;
		};
	}
}
