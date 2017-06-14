#pragma once
#include "../common/map.h"
#include "../common/map_base.h"
#include "game_cell.h"

namespace minedotcpp
{
	namespace game
	{
		class game_map : public common::map_base<game_cell>
		{
		public:
			void to_regular_map(common::map& m);
			void from_regular_map(minedotcpp::common::map& m);
		};
	}
}
