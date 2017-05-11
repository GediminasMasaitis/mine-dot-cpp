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
			int seed;
			std::default_random_engine random_engine;;

			explicit game_map_generator(int seed = -1)
			{
				random_engine = std::default_random_engine();
				this->seed = seed;
				if (seed != -1)
				{
					random_engine.seed(seed);
				}
			}

			void generate_with_mine_density(game_map& m, common::point starting_position, bool guarantee_opening, double mine_density);
			void generate_with_mine_count(game_map& m, common::point starting_position, bool guarantee_opening, int mine_count);
		};
	}
}
