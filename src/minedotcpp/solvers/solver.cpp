#include "solver.h"
#include "solver_map.h"
#include "border.h"

using namespace minedotcpp::solvers;
using namespace minedotcpp::common;

solver::solver(solver_settings& settings)
{
	this->settings = settings;
}

point_map<solver_result>* solver::solve(const map& base_map) const
{
	solver_map m(base_map);
	if(settings.ignore_mine_count_completely)
	{
		m.remaining_mine_count = -1;
	}

	point_map<double> all_probabilities;
	point_map<bool> all_verdicts;
	

	if(settings.solve_trivial)
	{
		solve_trivial(m, all_verdicts);
		if (settings.stop_after_trivial_solving || should_stop_solving(all_verdicts))
		{
			return get_final_results(all_probabilities, all_verdicts);
		}
	}

	// TODO: Gaussian solving



	solve_separation(m, all_probabilities, all_verdicts);

	return get_final_results(all_probabilities, all_verdicts);
}

void solver::solve_trivial(solver_map& m, point_map<bool>& verdicts) const
{
	while (true)
	{
		point_map<bool> currentRoundVerdicts;

		for (auto& cell : m.cells)
		{
			if (cell.state != cell_state_empty)
			{
				continue;
			}
			auto& neighbourEntry = m.neighbour_cache_get(cell.pt);
			auto& filledNeighbours = neighbourEntry.by_state[cell_state_filled];
			auto& flaggedNeighbours = neighbourEntry.by_flag[cell_flag_has_mine];
			auto& antiflaggedNeighbours = neighbourEntry.by_flag[cell_flag_doesnt_have_mine];
			if (filledNeighbours.size() == flaggedNeighbours.size() + antiflaggedNeighbours.size())
			{
				continue;
			}

			auto flagging = filledNeighbours.size() == cell.hint;
			auto clicking = flaggedNeighbours.size() == cell.hint;
			if (clicking || flagging)
			{
				for(auto& neighbour : filledNeighbours)
				{
					if(neighbour.state == (cell_state_filled | cell_flag_has_mine))
					{
						continue;
					}
					if(verdicts.find(neighbour.pt) != verdicts.end())
					{
						continue;
					}
					currentRoundVerdicts[neighbour.pt] = flagging;
					verdicts[neighbour.pt] = flagging;
				}
			}
		}

		if (currentRoundVerdicts.size() == 0)
		{
			return;
		}
		m.set_cells_by_verdicts(currentRoundVerdicts);
		
	}
}

void solver::solve_separation(solver_map& m, point_map<double>& probabilities, point_map<bool>& verdicts) const
{
	border common_border;
	std::vector<border> original_border_sequence;

	find_common_border(m, common_border);
	separate_borders(m, common_border, original_border_sequence);
	

}

void solver::get_partial_border(solver_map& m, point_set& allowed_coordinates, point target_coordinate, border& target_border) const
{
	point_set common_coords(allowed_coordinates);
	std::queue<point> coord_queue;
	coord_queue.push(target_coordinate);
	point_set visited;
	
	while (coord_queue.size() > 0)
	{
		auto& coord = coord_queue.front();
		coord_queue.pop();
		auto& cell = m[coord];
		if (common_coords.find(coord) != common_coords.end())
		{
			common_coords.erase(coord);
			target_border.cells.push_back(cell);
		}
		visited.insert(coord);
		auto unflagged_neighbours = m.neighbour_cache_get(coord).by_flag[cell_flag_none];
		auto cell_state = cell.state & cell_states;
		for(auto& x : unflagged_neighbours)
		{
			auto state = x.state & cell_states;
			if((cell_state == cell_state_filled && state == cell_state_empty) || (cell_state == cell_state_empty && state == cell_state_filled))
			{
				if(visited.find(x.pt) == visited.end())
				{
					visited.insert(x.pt);
					coord_queue.push(x.pt);
				}
			}
		}
	}
}

void solver::separate_borders(solver_map& m, border& common_border, std::vector<border> target_borders) const
{
	point_set common_coords;
	for(auto c : common_border.cells)
	{
		common_coords.insert(c.pt);
	}
	auto i = target_borders.size();
	while (common_coords.size() > 0)
	{
		auto& initial_point = *common_coords.begin();
		target_borders.resize(++i);
		auto& brd = target_borders[i-1];
		get_partial_border(m, common_coords, initial_point, brd);
		for(auto& c : brd.cells)
		{
			common_coords.erase(c.pt);
		}
	}
}

bool solver::is_cell_border(solver_map& m, cell& c) const
{
	if (c.state != cell_state_filled)
	{
		return false;
	}
	auto has_empty_neighbour = m.neighbour_cache_get(c.pt).by_state[cell_state_empty].size();
	return has_empty_neighbour;
}

void solver::find_common_border(solver_map& m, border& common_border) const
{
	for(auto& cell : m.cells)
	{
		if(is_cell_border(m, cell))
		{
			common_border.cells.push_back(cell);
		}
	}
}

bool solver::should_stop_solving(point_map<bool>& verdicts) const
{
	if(verdicts.size() == 0)
	{
		return false;
	}
	if (settings.stop_on_any_verdict)
	{
		return true;
	}
	if (settings.stop_on_no_mine_verdict)
	{
		for(auto& verdict : verdicts)
		{
			if(!verdict.second)
			{
				return true;
			}
		}
	}
	return false;
}

point_map<solver_result>* solver::get_final_results(point_map<double>& probabilities, point_map<bool>& verdicts) const
{
	auto results_ptr = new point_map<solver_result>();
	auto& results = *results_ptr;
	for(auto& probability : probabilities)
	{
		auto& result = results[probability.first];
		result.pt = probability.first;
		result.probability = probability.second;
		result.verdict = verdict_none;
	}
	for (auto& verdict : verdicts)
	{
		auto& result = results[verdict.first];
		result.pt = verdict.first;
		result.probability = verdict.second ? 1 : 0;
		result.verdict = verdict.second ? verdict_has_mine : verdict_doesnt_have_mine;
	}
	return results_ptr;
}