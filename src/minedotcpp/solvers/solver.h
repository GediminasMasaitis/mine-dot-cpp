#pragma once
#include "solver_settings.h"
#include <unordered_map>
#include "../common/point.h"
#include "solver_result.h"
#include "../common/map.h"

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
			std::unordered_map<common::point, solver_result, common::point_hash>* solve(const common::map& map) const;
			void solve_trivial(solver_map& m, std::unordered_map<common::point, bool, common::point_hash>& verdicts) const;
		private:
			bool should_stop_solving(std::unordered_map<common::point, bool, common::point_hash>& verdicts) const;
			std::unordered_map<common::point, solver_result, common::point_hash>* get_final_results(std::unordered_map<common::point, double, common::point_hash>& probabilities, std::unordered_map<common::point, bool, common::point_hash>& verdicts) const;
		};
	}
}
