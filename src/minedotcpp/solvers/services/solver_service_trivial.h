#pragma once
#include "../solver_map.h"
#include "../solver_settings.h"
#include "solver_service_base.h"

namespace minedotcpp
{
	namespace solvers
	{
		namespace services
		{
			class solver_service_trivial : private solver_service_base
			{
			public:
				explicit solver_service_trivial(const solver_settings& settings) : solver_service_base(settings)
				{
				}

				void solve_trivial(solver_map& m, common::point_map<bool>& verdicts) const;
			};
		}
	}
}
