#pragma once
#include "../common/cell.h"
#include "../common/map_base.h"
#include "game_cell.h"

namespace minedotcpp
{
	namespace game
	{
		class game_map : public common::map_base<game_cell>
		{
		};
	}
}
