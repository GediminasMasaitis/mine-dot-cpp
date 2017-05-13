#pragma once
#include "game_map_generator.h"
#include "../common/point.h"
#include "game_result.h"

namespace minedotcpp
{
	namespace game
	{
		class game_engine
		{
		public:
			game_map_generator* generator;
			game_map gm;
			bool game_started;

			explicit game_engine(game_map_generator& generator) : generator(&generator)
			{
				game_started = false;
			}

			void start_with_mine_density(int width, int height, common::point starting_position, bool guarantee_opening, double mine_density);
			void start_with_mine_count(int width, int height, common::point starting_position, bool guarantee_opening, int mine_count);
			void set_flag(common::point pt, common::cell_param flag);
			void toggle_flag(common::point pt);
			game_result open_cell(common::point pt);
		};
	}
}
