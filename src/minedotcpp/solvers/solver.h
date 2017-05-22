#pragma once
#include "../mine_api.h"
#include "../common/point.h"
#include "../common/map.h"
#include "solver_result.h"
#include "solver_settings.h"
#include "services/solver_service_base.h"
#include "services/solver_service_separation.h"


namespace minedotcpp
{
	namespace solvers
	{
		class solver_map;

		class MINE_API solver : private services::solver_service_base
		{
		public:
			explicit solver(solver_settings& settings) : solver_service_base(settings), separation_service(settings)
			{
			}

			void solve(const common::map& base_map, common::point_map<solver_result>& results) const;
		private:
			services::solver_service_separation separation_service;

			void get_final_results(solver_map& m, common::point_map<double>& probabilities, common::point_map<bool>& verdicts, common::point_map<solver_result>& results) const;
		};
	}
}
