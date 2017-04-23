#pragma once
#include "../common/point.h"
#include "solver_verdict.h"

namespace monedotcpp
{
	namespace solvers
	{
		struct solver_result
		{
		public:
			minedotcpp::common::point pt;
			double probability;
			solver_verdict verdict;
		};
	}
}
