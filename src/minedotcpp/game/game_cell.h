#pragma once
#include "../common/cell.h"

namespace minedotcpp
{
	namespace game
	{
		class game_cell : public minedotcpp::common::cell
		{
		public:
			bool has_mine;
		};
	}
}
