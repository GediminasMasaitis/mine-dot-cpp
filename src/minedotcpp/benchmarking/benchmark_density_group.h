#pragma once

#include <vector>
#include "benchmark_entry.h"

namespace minedotcpp
{
	namespace benchmarking
	{
		class benchmark_density_group
		{
		public:
			double density = -1;
			std::vector<benchmark_entry> entries = std::vector<benchmark_entry>();
		};
	}
}
