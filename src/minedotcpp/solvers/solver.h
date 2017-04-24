#pragma once
#include "solver_settings.h"

namespace minedotcpp
{
	namespace solvers
	{
		class solver
		{
		public:
			solver_settings settings;

			explicit solver(solver_settings settings);
		};
	}
}
