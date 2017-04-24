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
		class MINE_API solver
		{
		public:
			solver_settings settings;

			explicit solver(solver_settings settings);

			std::unordered_map<common::point, solver_result>* solve(const common::map& map) const;
		};
	}
}
