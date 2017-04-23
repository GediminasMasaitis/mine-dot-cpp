#pragma once

namespace minedotcpp
{
	namespace common
	{
		enum cell_param
		{
			// State
			cell_states = 3,

			cell_state_empty = 0,
			cell_state_filled = 1,
			cell_state_wall = 2,

			// Flags
			cell_flags = 3 << 2,

			cell_flag_has_mine = 1 << 2,
			cell_flag_doesnt_have_mine = 2 << 2,
			cell_flag_not_sure = 3 << 2
		};

		inline cell_param operator | (cell_param a, cell_param b)
		{
			return static_cast<cell_param>(static_cast<int>(a) | static_cast<int>(b));
		}

		inline cell_param operator & (cell_param a, cell_param b)
		{
			return static_cast<cell_param>(static_cast<int>(a) & static_cast<int>(b));
		}
	}
}