#pragma once
#include "solver_settings.h"

namespace monedotcpp
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
