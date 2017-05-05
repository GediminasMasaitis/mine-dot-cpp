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
	std::vector<border> original_borders;
	std::vector<border> borders;

	find_common_border(m, common_border);
	separate_borders(m, common_border, original_borders);
	
	for(auto& border : original_borders)
	{
		solve_border(border, m, true, borders);
		for(auto& borderVerdict : border.verdicts)
		{
			verdicts[borderVerdict.first] = borderVerdict.second;
		}
		for(auto& probability : border.probabilities)
		{
			probabilities[probability.first] = probability.second;
		}
		if (should_stop_solving(verdicts))
		{
			return;
		}
	}
}

void solver::solve_border(border& b, solver_map& m, bool allow_partial_border_solving, std::vector<border>& borders) const
{
	if (settings.partial_border_solving)
	{
		if (allow_partial_border_solving && b.cells.size() > settings.partial_border_solve_from)
		{
			//TrySolveBorderByPartialBorders(b, m);
		}
		if (should_stop_solving(b.verdicts))
		{
			borders.push_back(b);
			return;
		}

		if (settings.border_resplitting && b.verdicts.size() > 0)
		{
			//var borders = TrySolveBorderByReseparating(b, m);
			//if (borders != null)
			//{
			//	return borders;
			//}
		}
	}

	if (b.cells.size() > settings.give_up_from)
	{
		b.solved_fully = false;
		borders.push_back(b);
		return;
	}

	find_valid_border_cell_combinations(m, b);
	if (b.valid_combinations.size() == 0)
	{
		// TODO: Must be invalid map... Handle somehow
	}


	auto current_mine_verdicts = 0;
	for(auto& verdict : b.verdicts)
	{
		if(verdict.second)
		{
			current_mine_verdicts++;
		}
	}

	b.min_mine_count = 32;
	b.max_mine_count = 0;

	for(auto& valid_combination : b.valid_combinations)
	{
		auto mine_count = 0;
		for(auto& verdict : valid_combination)
		{
			if(verdict.second)
			{
				mine_count++;
			}
		}
		if(mine_count < b.min_mine_count)
		{
			b.min_mine_count = mine_count;
		}
		if(mine_count > b.max_mine_count)
		{
			b.max_mine_count = mine_count;
		}
	}

	b.min_mine_count += current_mine_verdicts;
	b.max_mine_count += current_mine_verdicts;

	calculate_border_probabilities(b);
	get_verdicts_from_probabilities(b.probabilities, b.verdicts);
	m.set_cells_by_verdicts(b.verdicts);
	for(auto& whole_border_result : b.verdicts)
	{
		b.probabilities.erase(whole_border_result.first);
		auto& c = m[whole_border_result.first];
		auto c_index = -1;
		for(auto i = 0; i < b.cells.size(); ++i)
		{
			if(c.pt == b.cells[i].pt)
			{
				c_index = i;
				break;
			}
		}
		b.cells.erase(b.cells.begin() + c_index);
		for(auto& valid_combination : b.valid_combinations)
		{
			valid_combination.erase(whole_border_result.first);
		}
	}

	b.solved_fully = true;
	// TODO: try resplitting borders anyway after solving?
	borders.push_back(b);
}

void solver::find_valid_border_cell_combinations(solver_map& map, border& border) const
{
	auto border_length = border.cells.size();
	const int maxSize = 31;
	if (border_length > maxSize)
	{
		// TODO: handle too big border
		//throw new InvalidDataException($"Border with {borderLength} cells is too large, maximum {maxSize} cells allowed");
	}
	auto totalCombinations = 1 << border_length;
	auto allRemainingCellsInBorder = map.undecided_count == border_length;

	point_set empty_pts;
	for(auto& c : border.cells)
	{
		auto& entry = map.neighbour_cache_get(c.pt).by_state[cell_state_empty];
		for (auto& cell : entry)
		{
			empty_pts.insert(cell.pt);
		}
	}
	

	std::vector<cell> empty_cells;
	empty_cells.reserve(empty_pts.size());
	for(auto& pt : empty_pts)
	{
		empty_cells.push_back(map.cell_get(pt));
	}
	//auto prediction_index = border.valid_combinations.size();
	
	for(int combo = 0; combo < totalCombinations; combo++)
	{
		if (map.remaining_mine_count > 0)
		{
			auto bits_set = SWAR(combo);
			if (bits_set > map.remaining_mine_count)
			{
				continue;
			}
			if (allRemainingCellsInBorder && bits_set != map.remaining_mine_count)
			{
				continue;
			}
		}
		
		auto prediction_valid = is_prediction_valid(map, border, combo, empty_cells);
		if (prediction_valid)
		{
			point_map<bool> predictions;
			predictions.resize(border_length);
			for (unsigned int j = 0; j < border_length; j++)
			{
				auto& pt = border.cells[j].pt;
				auto has_mine = (combo & (1 << j)) > 0;
				predictions[pt] = has_mine;
			}
			border.valid_combinations.push_back(predictions);
		}
	}
}

bool solver::is_prediction_valid(solver_map& map, border& b, unsigned int prediction, std::vector<cell>& empty_cells) const
{
	for (auto& cell : empty_cells)
	{
		auto neighbours_with_mine = 0;
		auto neighbours_without_mine = 0;
		auto& filled_neighbours = map.neighbour_cache_get(cell.pt).by_state[cell_state_filled];
		for (auto& neighbour : filled_neighbours)
		{
			auto flag = neighbour.state & cell_flags;
			switch (flag)
			{
			case cell_flag_has_mine:
				++neighbours_with_mine;
				break;
			case cell_flag_doesnt_have_mine:
				++neighbours_without_mine;
				break;
			default:
				unsigned int i;
				for(i = 0; i < b.cells.size(); i++)
				{
					if(neighbour.pt == b.cells[i].pt)
					{
						break;
					}
				}
				auto verdict = (prediction & (1 << i)) > 0;
				if (verdict)
				{
					++neighbours_with_mine;
				}
				else
				{
					++neighbours_without_mine;
				}
				break;
			}
		}

		if (neighbours_with_mine != cell.hint)
			return false;
		
		// TODO: What does this do?
		//if (filled_neighbours.size() - neighbours_without_mine < cell.hint)
		//	return false;

	}
	return true;
}

int solver::SWAR(int i) const
{
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

void solver::calculate_border_probabilities(border& b) const
{
	if (b.valid_combinations.size() == 0)
	{
		return;
	}
	for(auto& c : b.cells)
	{
		auto mine_count = 0;
		for(auto& combination : b.valid_combinations)
		{
			if(combination[c.pt])
			{
				++mine_count;
			}
		}
		auto probability = static_cast<double>(mine_count) / b.valid_combinations.size();
		b.probabilities[c.pt] = probability;
	}
}

void solver::get_verdicts_from_probabilities(point_map<double>& probabilities, point_map<bool>& target_verdicts) const
{
	for(auto& probability : probabilities)
	{
		bool verdict;
		if (abs(probability.second) < 0.0000001)
		{
			verdict = false;
		}
		else if (abs(probability.second - 1) < 0.0000001)
		{
			verdict = true;
		}
		else
		{
			// TODO: Invalid probability, handle somehow
			continue;
		}
		target_verdicts[probability.first] = verdict;
	}
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

void solver::separate_borders(solver_map& m, border& common_border, std::vector<border>& target_borders) const
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