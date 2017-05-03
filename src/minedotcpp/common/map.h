#pragma once

#include <vector>

#include "../mine_api.h"
#include "point.h"
#include "cell.h"

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
			std::vector<cell> cells;
			

			map(int width, int height, int remaining_mine_count = -1);

			cell& cell_get(int x, int y);
			cell& cell_get(point pt);
			cell* cell_try_get(int x, int y);
			cell* cell_try_get(point pt);
			bool cell_exists(int x, int y);
			bool cell_exists(point pt);

		protected:
			map();
		};
	}
}
