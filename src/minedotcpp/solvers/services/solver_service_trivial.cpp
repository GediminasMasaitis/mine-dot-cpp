#include "solver_service_trivial.h"

using namespace minedotcpp;
using namespace solvers;
using namespace services;
using namespace common;

void solver_service_trivial::solve_trivial(solver_map& m, point_map<bool>& verdicts) const
{
	while (true)
	{
		point_map<bool> current_round_verdicts;

		for (auto& cell : m.cells)
		{
			if (cell.state != cell_state_empty)
			{
				continue;
			}
			auto& neighbour_entry = m.neighbour_cache_get(cell.pt);
			auto& filled_neighbours = neighbour_entry.by_state[cell_state_filled];
			auto& flagged_neighbours = neighbour_entry.by_flag[cell_flag_has_mine];
			auto& antiflagged_neighbours = neighbour_entry.by_flag[cell_flag_doesnt_have_mine];
			if (filled_neighbours.size() == flagged_neighbours.size() + antiflagged_neighbours.size())
			{
				continue;
			}

			auto flagging = filled_neighbours.size() == cell.hint;
			auto clicking = flagged_neighbours.size() == cell.hint;
			if (clicking || flagging)
			{
				for (auto& neighbour : filled_neighbours)
				{
					if (neighbour.state == (cell_state_filled | cell_flag_has_mine))
					{
						continue;
					}
					if (verdicts.find(neighbour.pt) != verdicts.end())
					{
						continue;
					}
					current_round_verdicts[neighbour.pt] = flagging;
					verdicts[neighbour.pt] = flagging;
				}
			}
		}

		if (current_round_verdicts.size() == 0)
		{
			return;
		}
		m.set_cells_by_verdicts(current_round_verdicts);
	}
}
