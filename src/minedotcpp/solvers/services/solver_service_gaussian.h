#pragma once
#include "../solver_map.h"
#include "../solver_settings.h"
#include "solver_service_base.h"
#include "../matrix_reduction_parameters.h"
#include <vector>

namespace minedotcpp
{
	namespace solvers
	{
		namespace services
		{
			class solver_service_gaussian : private solver_service_base
			{
			public:
				explicit solver_service_gaussian(const solver_settings& settings) : solver_service_base(settings)
				{
				}

				void reduce_matrix(std::vector<std::vector<int>>& matrix, std::vector<common::point>& points, common::point_map<bool>& verdicts, const matrix_reduction_parameters& parameters) const;
				void solve_gaussian(solver_map& m, common::point_map<bool>& verdicts) const;
				void get_matrix_from_map(solver_map& m, std::vector<common::point>& points, bool all_undecided_coordinates_provided, std::vector<std::vector<int>>& matrix) const;
			};
		}
	}
}
