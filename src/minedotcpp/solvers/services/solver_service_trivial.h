#pragma once
#include "../solver_map.h"
#include "../solver_settings.h"

namespace minedotcpp
{
	namespace solvers
	{
		namespace services
		{
			class solver_service_trivial
			{
			public:
				solver_settings settings;

				explicit solver_service_trivial(const solver_settings& settings)
				{
					this->settings = settings;
				}

				void solve_trivial(solver_map& m, common::point_map<bool>& verdicts) const;
			};
		}
	}
}
