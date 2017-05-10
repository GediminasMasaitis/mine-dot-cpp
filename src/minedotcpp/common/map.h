#pragma once

#include <vector>

#include "../mine_api.h"
#include "point.h"
#include "cell.h"

namespace minedotcpp
{
	namespace common
	{
		template <typename TCell>
		class MINE_API map_base
		{
		public:
			int width;
			int height;
			int remaining_mine_count;
			std::vector<TCell> cells;

			map_base();
			void init(int width, int height, int remaining_mine_count = -1, cell_param state = cell_state_empty);

			inline TCell& cell_get(int x, int y) { return cells[x*height + y]; }
			inline TCell& cell_get(point pt) { return cell_get(pt.x, pt.y); }
			inline TCell& operator[] (point pt) { return cell_get(pt); }
			TCell* cell_try_get(int x, int y);
			inline TCell* cell_try_get(point pt) { return cell_try_get(pt.x, pt.y); }
			bool cell_exists(int x, int y);
			bool cell_exists(point pt);
		};

		class MINE_API map : public map_base<cell>
		{
		public:
			

		protected:
			
		};
	}
}
