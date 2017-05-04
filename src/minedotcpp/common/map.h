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

			inline cell& cell_get(int x, int y) { return cells[x*height + y]; }
			inline cell& cell_get(point pt) { return cell_get(pt.x, pt.y);	}
			inline cell& operator[] (point pt) { return cell_get(pt); }
			cell* cell_try_get(int x, int y);
			inline cell* cell_try_get(point pt) { return cell_try_get(pt.x, pt.y); }
			bool cell_exists(int x, int y);
			bool cell_exists(point pt);

		protected:
			map();
		};
	}
}
