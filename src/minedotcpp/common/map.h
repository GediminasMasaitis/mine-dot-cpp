#pragma once

#include <vector>

#include "../mine_api.h"
#include "point.h"
#include "cell.h"
#include "neighbour_cache_entry.h"

namespace minedotcpp
{
	namespace common
	{
		class MINE_API map
		{
		public:
			int width;
			int height;
			int remaining_mine_count;
			cell* cells;
			neighbour_cache_entry* neighbour_cache;

			map(int width, int height, int remaining_mine_count = -1);
			~map();

			cell* cell_get(int x, int y);
			cell* cell_get(point pt);
			cell* cell_try_get(int x, int y);
			cell* cell_try_get(point pt);
			bool cell_exists(int x, int y);
			bool cell_exists(point pt);
			neighbour_cache_entry* neighbour_cache_get(int x, int y);
			neighbour_cache_entry* neighbour_cache_get(point pt);
		private:
			void build_neighbour_cache();
			void calculate_neighbours_of(point pt, std::vector<cell*>& cells, bool include_self);
		};
	}
}
