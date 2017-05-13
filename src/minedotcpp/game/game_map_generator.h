#pragma once
#include "../common/cell.h"
#include "../common/map_base.h"
#include "game_cell.h"
#include "game_map.h"
#include "../common/common_macros.h"
#include "../common/common_functions.h"
#include "../solvers/solver_map.h"
#include <algorithm>
#include <random>


namespace minedotcpp
{
	namespace game
	{
		class game_map_generator
		{
		public:
			std::mt19937* random_engine;

			game_map_generator()
			{
				
			}

			void generate_with_mine_density(game_map& m, common::point starting_position, bool guarantee_opening, double mine_density);
			void generate_with_mine_count(game_map& m, common::point starting_position, bool guarantee_opening, int mine_count);
		};
	}
}
