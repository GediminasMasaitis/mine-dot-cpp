#pragma once

#include <vector>

namespace minedotcpp
{
	namespace benchmarking
	{
		class benchmark_entry
		{
		public:
			int map_index;
			std::vector<size_t> solving_durations = std::vector<size_t>();
			size_t total_duration = 0;
			bool solved = false;
		};
	}
}
