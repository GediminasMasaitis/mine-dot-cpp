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
			bool initialized = false;
			std::vector<cell*> all_neighbours;
			std::vector<cell*> by_state[3];
			std::vector<cell*> by_flag[3];
			//std::vector<cell*> by_param[16];

			neighbour_cache_entry()
			{
				all_neighbours.reserve(8);
				for(auto i = 0; i < 3; i++)
				{
					by_state[i].reserve(8);
					by_flag[i].reserve(8);
				}
			}
		};
	}
}