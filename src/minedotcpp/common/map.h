#pragma once

#include "../mine_api.h"
#include "cell.h"
#include "map_base.h"

namespace minedotcpp
{
	namespace common
	{
		class MINE_API map : public map_base<cell>
		{
		public:
			void init(int width, int height, int remaining_mine_count = -1, cell_param state = cell_state_empty);
		};
	}
}
