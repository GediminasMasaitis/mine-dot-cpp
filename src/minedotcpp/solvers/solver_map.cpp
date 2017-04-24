#include "solver_map.h"

using namespace minedotcpp::common;

minedotcpp::solvers::solver_map::solver_map(const map& base_map)
{
	width = base_map.width;
	height = base_map.height;
	remaining_mine_count = base_map.remaining_mine_count;
	auto size = width*height;
	cells = new cell[size];
	neighbour_cache = nullptr;

	filled_count = 0;
	flagged_count = 0;
	anti_flagged_count = 0;
	undecided_count = 0;
	for (auto i = 0; i < size; ++i)
	{
		cells[i] = base_map.cells[i];
		auto state = cells[i].state & cell_states;
		if (state == cell_state_filled)
		{
			if (cells[i].state == (cell_state_filled | cell_flag_has_mine))
			{
				filled_count++;
				flagged_count++;
			}
			else if (cells[i].state == (cell_state_filled | cell_flag_doesnt_have_mine))
			{
				filled_count++;
				anti_flagged_count++;
			}
			else
			{
				filled_count++;
				undecided_count++;
			}
		}
	}
}
