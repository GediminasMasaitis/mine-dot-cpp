#pragma once
#include <vector>
#include "cell.h"

namespace minedotcpp
{
	namespace common
	{
		class neighbour_cache_entry
		{
		public:
			std::vector<cell*> all_neighbours;
			std::vector<cell*> by_state[16];
			std::vector<cell*> by_flag[16];
			std::vector<cell*> by_param[16];
		};
	}
}