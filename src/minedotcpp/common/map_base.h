#pragma once

#include <vector>

#include "point.h"
#include "cell.h"

namespace minedotcpp
{
	namespace common
	{
		template <typename TCell>
		class map_base
		{
		public:
			int width;
			int height;
			int remaining_mine_count;
			std::vector<TCell> cells;

			map_base()
			{
				width = -1;
				height = -1;
				remaining_mine_count = -1;
			}


			TCell& cell_get(int x, int y) { return cells[x*height + y]; }
			TCell& cell_get(point pt) { return cell_get(pt.x, pt.y); }
			TCell& operator[] (point pt) { return cell_get(pt); }
			TCell* cell_try_get(int x, int y)
			{
				if(x >= 0 && y >= 0 && x < width && y < height)
				{
					auto& cell = cell_get(x, y);
					if(cell.state != cell_state_wall) return &cell;
				}
				return nullptr;
			}
			TCell* cell_try_get(point pt) { return cell_try_get(pt.x, pt.y); }
			bool cell_exists(int x, int y) { return x >= 0 && y >= 0 && x < width && y < height && cell_get(x, y).state != cell_state_wall; }
			bool cell_exists(point pt) { return cell_exists(pt.x, pt.y); }
		};
	}
}