#pragma once

#include <vector>

namespace minedotcpp
{
	namespace benchmarking
	{
		class benchmark_entry
		{
		public:
			std::vector<int> solving_durations = std::vector<int>();
			int total_duration = 0;
			bool solved = false;
		};
	}
}
