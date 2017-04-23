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
			unsigned short width;
			unsigned short height;
			int remaining_mine_count;
			cell* cells;
			neighbour_cache_entry* neighbour_cache;

			map(unsigned short width, unsigned short height, int remaining_mine_count = -1);
			~map();

			cell* cell_get(unsigned short x, unsigned short y);
			cell* cell_get(point pt);
			cell* cell_try_get(unsigned short x, unsigned short y);
			cell* cell_try_get(point pt);
			bool cell_exists(unsigned short x, unsigned short y);
			bool cell_exists(point pt);
			neighbour_cache_entry* neighbour_cache_get(int x, int y);
			neighbour_cache_entry* neighbour_cache_get(point pt);
		private:
			void build_neighbour_cache();
			void calculate_neighbours_of(point pt, std::vector<cell*>& cells, bool include_self);
		};
	}
}
