#include "../solver_map.h"
#include "../border.h"
#include "../../debug/debugging.h"
#include "solver_service_separation.h"
#include <thread>
#include <mutex>
#include <queue>
#include "solver_service_separation_mine_counts.h"
#include "solver_service_separation_combination_finding.h"

using namespace minedotcpp;
using namespace solvers;
using namespace services;
using namespace common;
using namespace std;

void solver_service_separation::solve_separation(solver_map& m, point_map<double>& probabilities, point_map<bool>& verdicts) const
{
	border common_border;
	vector<border> original_borders;
	vector<border> borders;

	find_common_border(m, common_border);
	//visualize(m, { common_border }, false);
	separate_borders(m, common_border, original_borders);

	if(settings.separation_order_borders_by_size)
	{
		std::sort(original_borders.begin(), original_borders.end(), [](const border& lhs, const border& rhs)
		{
			return lhs.cells.size() < rhs.cells.size();
		});
	}
	else if(settings.separation_order_borders_by_size_descending)
	{
		std::sort(original_borders.begin(), original_borders.end(), [](const border& lhs, const border& rhs)
		{
			return lhs.cells.size() > rhs.cells.size();
		});
	}


	for (auto& b : original_borders)
	{
		solve_border(m, b, true, borders);
		for (auto& borderVerdict : b.verdicts)
		{
			verdicts[borderVerdict.first] = borderVerdict.second;
		}
		for (auto& probability : b.probabilities)
		{
			probabilities[probability.first] = probability.second;
		}
		if(should_stop_solving(verdicts, settings.separation_single_border_stop_on_no_mine_verdict, settings.separation_single_border_stop_on_any_verdict, settings.separation_single_border_stop_always))
		{
			return;
		}
	}

	if (settings.mine_count_solve)
	{
		auto mine_count_srvc = solver_service_separation_mine_counts(settings);
		mine_count_srvc.solve_mine_counts(m, common_border, borders, probabilities, verdicts);
	}
}

void solver_service_separation::solve_border(solver_map& m, border& b, bool allow_partial_border_solving, vector<border>& borders) const
{
	if (settings.partial_solve)
	{
		if (allow_partial_border_solving && b.cells.size() > settings.partial_solve_from_size)
		{
			try_solve_border_by_partial_borders(m, b);
			if (should_stop_solving(b.verdicts, settings.partial_all_stop_on_no_mine_verdict, settings.partial_all_stop_on_any_verdict, settings.partial_stop_always))
			{
				borders.push_back(b);
				return;
			}

			if (settings.resplit_on_partial_verdict && b.verdicts.size() > 0)
			{
				reseparate_border(m, b, borders, true);
				return;
			}
		}
	}

	if (b.cells.size() > settings.give_up_from_size)
	{
		b.solved_fully = false;
		borders.push_back(b);
		return;
	}

	combination_service.find_valid_border_cell_combinations(m, b);
	//dump_predictions(b.valid_combinations);

	if (b.valid_combinations.size() == 0)
	{
		// TODO: Must be invalid map... Handle somehow
	}

	calculate_min_max_mine_counts(b);

	auto verdicts_before = b.verdicts.size();
	calculate_border_probabilities(b);
	get_verdicts_from_probabilities(b.probabilities, b.verdicts);
	m.set_cells_by_verdicts(b.verdicts);
	for (auto& whole_border_result : b.verdicts)
	{
		b.probabilities.erase(whole_border_result.first);
		auto& c = m[whole_border_result.first];
		auto c_index = -1;
		for (auto i = 0; i < b.cells.size(); ++i)
		{
			if (c.pt == b.cells[i].pt)
			{
				c_index = i;
				break;
			}
		}
		if (c_index >= 0)
		{
			if (c_index != b.cells.size() - 1)
			{
				b.cells[c_index] = move(b.cells.back());
			}
			b.cells.pop_back();
			//b.cells.erase(b.cells.begin() + c_index);
			for (auto& valid_combination : b.valid_combinations)
			{
				valid_combination.erase(whole_border_result.first);
			}
		}
	}

	b.solved_fully = true;
	if (settings.resplit_on_complete_verdict && b.verdicts.size() > verdicts_before)
	{
		reseparate_border(m, b, borders, false);
	}
	else
	{
		borders.push_back(b);
	}
}

void solver_service_separation::try_solve_border_by_partial_borders(solver_map& m, border& b) const
{
	vector<partial_border_data> checked_partial_borders;
	for (auto i = 0; i < b.cells.size(); i++)
	{
		auto target_coordinate = b.cells[i].pt;
		partial_border_data border_data;
		get_partial_border(b, m, target_coordinate, border_data);
		//visualize(border_data.partial_map, { border_data.partial_border }, true);
		partial_border_data previous_border_data;
		auto superset = false;
		for (auto& checked : checked_partial_borders)
		{
			superset = true;
			for (auto& c : border_data.partial_border.cells)
			{
				if (!checked.partial_border_points.count(c.pt))
				{
					superset = false;
					break;
				}
			}
			if (superset)
			{
				auto it = checked.partial_border.probabilities.find(target_coordinate);
				if (it != checked.partial_border.probabilities.end())
				{
					b.probabilities[target_coordinate] = it->second;
				}
				break;
			}
		}
		if (superset)
		{
			continue;
		}

		vector<border> temp;
		solve_border(border_data.partial_map, border_data.partial_border, false, temp);
		checked_partial_borders.push_back(border_data);
		auto& verdicts = border_data.partial_border.verdicts;
		if (verdicts.size() > 0)
		{
			m.set_cells_by_verdicts(verdicts);
			for (auto& verdict : verdicts)
			{
				auto& cell = m[verdict.first];
				b.verdicts[verdict.first] = verdict.second;
				int cell_index;
				for (cell_index = 0; cell_index < b.cells.size(); cell_index++)
				{
					if (b.cells[cell_index].pt == cell.pt)
					{
						break;
					}
				}
				if (cell_index <= i)
				{
					i--;
				}
				b.cells.erase(b.cells.begin() + cell_index);
			}
			if (should_stop_solving(b.verdicts, settings.partial_single_stop_on_no_mine_verdict, settings.partial_single_stop_on_any_verdict, false))
			{
				return;
			}
		}
		if (settings.partial_set_probability_guesses && verdicts.find(target_coordinate) == verdicts.end())
		{
			auto it = border_data.partial_border.probabilities.find(target_coordinate);
			if (it != border_data.partial_border.probabilities.end())
			{
				b.probabilities[target_coordinate] = it->second;
			}
		}
	}
}

void calculate_partial_map_and_trim_partial_border(border& partial_border, solver_map& partial_map, solver_map& parent_map, point_set& all_flagged_coordinates)
{
	point_set border_coordinate_set;
	point_set allSurroundingEmpty;
	border_coordinate_set.resize(partial_border.cells.size());
	for (auto& c : partial_border.cells)
	{
		border_coordinate_set.insert(c.pt);
		auto& empty_cells = parent_map.neighbour_cache_get(c.pt).by_state[cell_state_empty];
		for (auto& empty_c : empty_cells)
		{
			allSurroundingEmpty.insert(empty_c.pt);
		}
	}

	//std::vector<cell> only_influencing_border;
	point_set only_influencing_border_set;
	for (auto& empty_pt : allSurroundingEmpty)
	{
		auto& filled_enoighbours = parent_map.neighbour_cache_get(empty_pt).by_state[cell_state_filled];
		auto all = true;
		for (auto& neighbour : filled_enoighbours)
		{
			if (border_coordinate_set.find(neighbour.pt) == border_coordinate_set.end() && all_flagged_coordinates.find(neighbour.pt) == all_flagged_coordinates.end())
			{
				//only_influencing_border.push_back(parent_map.cell_get(empty_pt));
				all = false;
				break;
			}
		}
		if (all)
		{
			only_influencing_border_set.insert(empty_pt);
		}
	}

	vector<cell> new_non_border_cells;
	for (auto& c : partial_border.cells)
	{
		auto& empty_cells = parent_map.neighbour_cache_get(c.pt).by_state[cell_state_empty];
		for (auto& empty_c : empty_cells)
		{
			if (only_influencing_border_set.find(empty_c.pt) != only_influencing_border_set.end())
			{
				new_non_border_cells.push_back(c);
				break;
			}
		}
	}
	partial_border.cells = new_non_border_cells;

	if (partial_border.cells.size() == 0)
	{
		// TODO: Handle this
		return;
	}

	partial_map.width = parent_map.width;
	partial_map.height = parent_map.height;
	partial_map.remaining_mine_count = -1;
	partial_map.cells.resize(partial_map.width * partial_map.height);

	for (auto i = 0; i < partial_map.width; i++)
	{
		for (auto j = 0; j < partial_map.height; j++)
		{
			auto& c = partial_map.cell_get(i, j);
			c.pt.x = i;
			c.pt.y = j;
			c.state = cell_state_wall;
		}
	}
	//var partialMap = new Map(newWidth, newHeight, null, true, CellState.Wall);
	for (auto& c : partial_border.cells)
	{
		partial_map[c.pt] = c;
	}
	for (auto& pt : only_influencing_border_set)
	{
		partial_map[pt] = parent_map.cell_get(pt);
	}
	for (auto& pt : all_flagged_coordinates)
	{
		partial_map[pt] = parent_map[pt];
	}
}

void solver_service_separation::get_partial_border(border& border, solver_map& map, point target_pt, partial_border_data& border_data) const
{
	vector<cell> partial_border_sequence;
	vector<cell> partial_border_cells;
	point_set border_pts;
	point_set all_flagged_coordinates;
	for (auto& cell : border.cells)
	{
		border_pts.insert(cell.pt);
	}
	for (auto& cell : map.cells)
	{
		if (cell.state == (cell_state_filled | cell_flag_has_mine) || cell.state == (cell_state_filled | cell_flag_doesnt_have_mine))
		{
			all_flagged_coordinates.insert(cell.pt);
		}
	}
	breadth_search_border(map, border_pts, target_pt, partial_border_sequence);
	auto found = false;
	for (auto cell : partial_border_sequence)
	{
		partial_border_cells.push_back(cell);
		if (partial_border_cells.size() < settings.partial_optimal_size)
		{
			continue;
		}
		solvers::border partial_border_candidate;
		partial_border_candidate.cells = partial_border_cells;
		solver_map partial_map_candidate;
		calculate_partial_map_and_trim_partial_border(partial_border_candidate, partial_map_candidate, map, all_flagged_coordinates);
		if (partial_border_candidate.cells.size() > settings.partial_optimal_size)
		{
			break;
		}
		found = true;
		border_data.partial_border = partial_border_candidate;
		border_data.partial_map = partial_map_candidate;
	}
	//Debugging.Visualize(map, border, partialBorder);
	if (!found)
	{
		border_data.partial_border.cells = partial_border_cells;
		calculate_partial_map_and_trim_partial_border(border_data.partial_border, border_data.partial_map, map, all_flagged_coordinates);
	}
	for (auto& c : border_data.partial_border.cells)
	{
		border_data.partial_border_points.insert(c.pt);
	}
	border_data.partial_map.calculate_additional_data();
}

void solver_service_separation::reseparate_border(solver_map& m, border& parent_border, vector<border>& borders, bool solve) const
{
	auto resplit_borders = vector<border>();
	separate_borders(m, parent_border, resplit_borders);
	for (auto& b : resplit_borders)
	{
		if (solve)
		{
			solve_border(m, b, false, borders);
		}
		else
		{
			borders.push_back(b);
		}
		for (auto& verdict : b.verdicts)
		{
			parent_border.verdicts[verdict.first] = verdict.second;
		}
		if (b.solved_fully)
		{
			for (auto& probability : b.probabilities)
			{
				parent_border.probabilities[probability.first] = probability.second;
			}
		}
	}
}


void solver_service_separation::calculate_min_max_mine_counts(border& b) const
{
	auto current_mine_verdicts = 0;
	for (auto& verdict : b.verdicts)
	{
		if (verdict.second)
		{
			current_mine_verdicts++;
		}
	}

	b.min_mine_count = 32;
	b.max_mine_count = 0;

	for (auto& valid_combination : b.valid_combinations)
	{
		auto mine_count = 0;
		for (auto& verdict : valid_combination)
		{
			if (verdict.second)
			{
				mine_count++;
			}
		}
		if (mine_count < b.min_mine_count)
		{
			b.min_mine_count = mine_count;
		}
		if (mine_count > b.max_mine_count)
		{
			b.max_mine_count = mine_count;
		}
	}

	b.min_mine_count += current_mine_verdicts;
	b.max_mine_count += current_mine_verdicts;
}

void solver_service_separation::calculate_border_probabilities(border& b) const
{
	if (b.valid_combinations.size() == 0)
	{
		return;
	}
	for (auto& c : b.cells)
	{
		auto mine_count = 0;
		for (auto& combination : b.valid_combinations)
		{
			if (combination[c.pt])
			{
				++mine_count;
			}
		}
		auto probability = static_cast<double>(mine_count) / b.valid_combinations.size();
		b.probabilities[c.pt] = probability;
	}
}

void solver_service_separation::breadth_search_border(solver_map& m, point_set& allowed_coordinates, point target_coordinate, vector<cell>& target_cells) const
{
	point_set common_coords(allowed_coordinates);
	queue<point> coord_queue;
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
			target_cells.push_back(cell);
		}
		visited.insert(coord);
		auto unflagged_neighbours = m.neighbour_cache_get(coord).by_flag[cell_flag_none];
		auto cell_state = cell.state & cell_states;
		for (auto& x : unflagged_neighbours)
		{
			auto state = x.state & cell_states;
			if ((cell_state == cell_state_filled && state == cell_state_empty) || (cell_state == cell_state_empty && state == cell_state_filled))
			{
				if (visited.find(x.pt) == visited.end())
				{
					visited.insert(x.pt);
					coord_queue.push(x.pt);
				}
			}
		}
	}
}

int solver_service_separation::separate_borders(solver_map& m, border& common_border, vector<border>& target_borders) const
{
	point_set common_coords;
	for (auto c : common_border.cells)
	{
		common_coords.insert(c.pt);
	}
	auto index = target_borders.size();
	auto border_count = 0;
	while (common_coords.size() > 0)
	{
		border_count++;
		auto& initial_point = *common_coords.begin();
		target_borders.resize(++index);
		auto& brd = target_borders[index - 1];
		breadth_search_border(m, common_coords, initial_point, brd.cells);
		for (auto& c : brd.cells)
		{
			common_coords.erase(c.pt);
		}
	}
	return border_count;
}

bool solver_service_separation::is_cell_border(solver_map& m, cell& c) const
{
	if (c.state != cell_state_filled)
	{
		return false;
	}
	auto has_empty_neighbour = m.neighbour_cache_get(c.pt).by_state[cell_state_empty].size();
	return has_empty_neighbour;
}

void solver_service_separation::find_common_border(solver_map& m, border& common_border) const
{
	for (auto& cell : m.cells)
	{
		if (is_cell_border(m, cell))
		{
			common_border.cells.push_back(cell);
		}
	}
}

