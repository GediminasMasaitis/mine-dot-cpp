#include "solver.h"
#include "solver_map.h"
#include "border.h"
#include <thread>
#include <mutex>
#include <queue>
#include "../debug/debugging.h"

using namespace minedotcpp::solvers;
using namespace minedotcpp::common;

solver::solver(solver_settings& settings)
{
	this->settings = settings;
}

void solver::solve(const map& base_map, point_map<solver_result>& results) const
{
	solver_map m;
	m.init_from(base_map);
	if(settings.mine_count_ignore_completely)
	{
		m.remaining_mine_count = -1;
	}

	point_map<double> all_probabilities;
	point_map<bool> all_verdicts;


	if(settings.trivial_solve)
	{
		solve_trivial(m, all_verdicts);
		if(should_stop_solving(all_verdicts, settings.trivial_stop_on_no_mine_verdict, settings.trivial_stop_on_any_verdict, settings.trivial_stop_always))
		{
			get_final_results(all_probabilities, all_verdicts, results);
			return;
		}
	}

	// TODO: Gaussian solving



	solve_separation(m, all_probabilities, all_verdicts);

	get_final_results(all_probabilities, all_verdicts, results);
}

void solver::solve_trivial(solver_map& m, point_map<bool>& verdicts) const
{
	while(true)
	{
		point_map<bool> currentRoundVerdicts;

		for(auto& cell : m.cells)
		{
			if(cell.state != cell_state_empty)
			{
				continue;
			}
			auto& neighbourEntry = m.neighbour_cache_get(cell.pt);
			auto& filledNeighbours = neighbourEntry.by_state[cell_state_filled];
			auto& flaggedNeighbours = neighbourEntry.by_flag[cell_flag_has_mine];
			auto& antiflaggedNeighbours = neighbourEntry.by_flag[cell_flag_doesnt_have_mine];
			if(filledNeighbours.size() == flaggedNeighbours.size() + antiflaggedNeighbours.size())
			{
				continue;
			}

			auto flagging = filledNeighbours.size() == cell.hint;
			auto clicking = flaggedNeighbours.size() == cell.hint;
			if(clicking || flagging)
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

		if(currentRoundVerdicts.size() == 0)
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

	for(auto& b : original_borders)
	{
		solve_border(m, b, true, borders);
		for(auto& borderVerdict : b.verdicts)
		{
			verdicts[borderVerdict.first] = borderVerdict.second;
		}
		for(auto& probability : b.probabilities)
		{
			probabilities[probability.first] = probability.second;
		}
	}

	if(settings.mine_count_solve)
	{
		solve_mine_counts(m, common_border, borders, probabilities, verdicts);
	}
}

void solver::solve_border(solver_map& m, border& b, bool allow_partial_border_solving, std::vector<border>& borders) const
{
	if(settings.partial_solve)
	{
		if(allow_partial_border_solving && b.cells.size() > settings.partial_solve_from_size)
		{
			try_solve_border_by_partial_borders(m, b);
			if(should_stop_solving(b.verdicts, settings.partial_all_stop_on_no_mine_verdict, settings.partial_all_stop_on_any_verdict, settings.partial_stop_always))
			{
				borders.push_back(b);
				return;
			}

			if(settings.resplit_on_partial_verdict && b.verdicts.size() > 0)
			{
				reseparate_border(m, b, borders, true);
				return;
			}
		}
	}

	if(b.cells.size() > settings.give_up_from_size)
	{
		b.solved_fully = false;
		borders.push_back(b);
		return;
	}

	find_valid_border_cell_combinations(m, b);
	if(b.valid_combinations.size() == 0)
	{
		// TODO: Must be invalid map... Handle somehow
	}

	calculate_min_max_mine_counts(b);

	auto verdicts_before = b.verdicts.size();
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
		if(c_index >= 0)
		{
			if(c_index != b.cells.size() - 1)
			{
				b.cells[c_index] = std::move(b.cells.back());
			}
			b.cells.pop_back();
			//b.cells.erase(b.cells.begin() + c_index);
			for(auto& valid_combination : b.valid_combinations)
			{
				valid_combination.erase(whole_border_result.first);
			}
		}
	}

	b.solved_fully = true;
	if(settings.resplit_on_complete_verdict && b.verdicts.size() > verdicts_before)
	{
		reseparate_border(m, b, borders, false);
	}
	else
	{
		borders.push_back(b);
	}
}

void solver::try_solve_border_by_partial_borders(solver_map& m, border& b) const
{
	std::vector<partial_border_data> checked_partial_borders;
	for(auto i = 0; i < b.cells.size(); i++)
	{
		auto target_coordinate = b.cells[i].pt;
		partial_border_data border_data;
		get_partial_border(b, m, target_coordinate, border_data);
		//visualize(border_data.partial_map, { border_data.partial_border }, true);
		partial_border_data previous_border_data;
		auto superset = false;
		for(auto& checked : checked_partial_borders)
		{
			superset = true;
			for(auto& c : border_data.partial_border.cells)
			{
				if(!checked.partial_border_points.count(c.pt))
				{
					superset = false;
					break;
				}
			}
			if(superset)
			{
				auto it = checked.partial_border.probabilities.find(target_coordinate);
				if(it != checked.partial_border.probabilities.end())
				{
					b.probabilities[target_coordinate] = it->second;
				}
				break;
			}
		}
		if(superset)
		{
			continue;
		}

		std::vector<border> temp;
		solve_border(border_data.partial_map, border_data.partial_border, false, temp);
		checked_partial_borders.push_back(border_data);
		auto& verdicts = border_data.partial_border.verdicts;
		if(verdicts.size() > 0)
		{
			m.set_cells_by_verdicts(verdicts);
			for(auto& verdict : verdicts)
			{
				auto& cell = m[verdict.first];
				b.verdicts[verdict.first] = verdict.second;
				int cell_index;
				for(cell_index = 0; cell_index < b.cells.size(); cell_index++)
				{
					if(b.cells[cell_index].pt == cell.pt)
					{
						break;
					}
				}
				if(cell_index <= i)
				{
					i--;
				}
				b.cells.erase(b.cells.begin() + cell_index);
			}
			if(should_stop_solving(b.verdicts, settings.partial_single_stop_on_no_mine_verdict, settings.partial_single_stop_on_any_verdict, false))
			{
				return;
			}
		}
		if(settings.partial_set_probability_guesses && verdicts.find(target_coordinate) == verdicts.end())
		{
			auto it = border_data.partial_border.probabilities.find(target_coordinate);
			if(it != border_data.partial_border.probabilities.end())
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
	for(auto& c : partial_border.cells)
	{
		border_coordinate_set.insert(c.pt);
		auto& empty_cells = parent_map.neighbour_cache_get(c.pt).by_state[cell_state_empty];
		for(auto& empty_c : empty_cells)
		{
			allSurroundingEmpty.insert(empty_c.pt);
		}
	}

	//std::vector<cell> only_influencing_border;
	point_set only_influencing_border_set;
	for(auto& empty_pt : allSurroundingEmpty)
	{
		auto& filled_enoighbours = parent_map.neighbour_cache_get(empty_pt).by_state[cell_state_filled];
		auto all = true;
		for(auto& neighbour : filled_enoighbours)
		{
			if(border_coordinate_set.find(neighbour.pt) == border_coordinate_set.end() && all_flagged_coordinates.find(neighbour.pt) == all_flagged_coordinates.end())
			{
				//only_influencing_border.push_back(parent_map.cell_get(empty_pt));
				all = false;
				break;
			}
		}
		if(all)
		{
			only_influencing_border_set.insert(empty_pt);
		}
	}

	std::vector<cell> new_non_border_cells;
	for(auto& c : partial_border.cells)
	{
		auto& empty_cells = parent_map.neighbour_cache_get(c.pt).by_state[cell_state_empty];
		for(auto& empty_c : empty_cells)
		{
			if(only_influencing_border_set.find(empty_c.pt) != only_influencing_border_set.end())
			{
				new_non_border_cells.push_back(c);
				break;
			}
		}
	}
	partial_border.cells = new_non_border_cells;

	if(partial_border.cells.size() == 0)
	{
		// TODO: Handle this
		return;
	}

	partial_map.width = parent_map.width;
	partial_map.height = parent_map.height;
	partial_map.remaining_mine_count = -1;
	partial_map.cells.resize(partial_map.width * partial_map.height);

	for(auto i = 0; i < partial_map.width; i++)
	{
		for(auto j = 0; j < partial_map.height; j++)
		{
			auto& c = partial_map.cell_get(i, j);
			c.pt.x = i;
			c.pt.y = j;
			c.state = cell_state_wall;
		}
	}
	//var partialMap = new Map(newWidth, newHeight, null, true, CellState.Wall);
	for(auto& c : partial_border.cells)
	{
		partial_map[c.pt] = c;
	}
	for(auto& pt : only_influencing_border_set)
	{
		partial_map[pt] = parent_map.cell_get(pt);
	}
	for(auto& pt : all_flagged_coordinates)
	{
		partial_map[pt] = parent_map[pt];
	}
}

void solver::get_partial_border(border& border, solver_map& map, point target_pt, partial_border_data& border_data) const
{
	std::vector<cell> partial_border_sequence;
	std::vector<cell> partial_border_cells;
	point_set border_pts;
	point_set all_flagged_coordinates;
	for(auto& cell : border.cells)
	{
		border_pts.insert(cell.pt);
	}
	for(auto& cell : map.cells)
	{
		if(cell.state == (cell_state_filled | cell_flag_has_mine) || cell.state == (cell_state_filled | cell_flag_doesnt_have_mine))
		{
			all_flagged_coordinates.insert(cell.pt);
		}
	}
	breadth_search_border(map, border_pts, target_pt, partial_border_sequence);
	auto found = false;
	for(auto cell : partial_border_sequence)
	{
		partial_border_cells.push_back(cell);
		if(partial_border_cells.size() < settings.partial_optimal_size)
		{
			continue;
		}
		solvers::border partial_border_candidate;
		partial_border_candidate.cells = partial_border_cells;
		solvers::solver_map partial_map_candidate;
		calculate_partial_map_and_trim_partial_border(partial_border_candidate, partial_map_candidate, map, all_flagged_coordinates);
		if(partial_border_candidate.cells.size() > settings.partial_optimal_size)
		{
			break;
		}
		found = true;
		border_data.partial_border = partial_border_candidate;
		border_data.partial_map = partial_map_candidate;
	}
	//Debugging.Visualize(map, border, partialBorder);
	if(!found)
	{
		border_data.partial_border.cells = partial_border_cells;
		calculate_partial_map_and_trim_partial_border(border_data.partial_border, border_data.partial_map, map, all_flagged_coordinates);
	}
	for(auto& c : border_data.partial_border.cells)
	{
		border_data.partial_border_points.insert(c.pt);
	}
	border_data.partial_map.calculate_additional_data();
}

void solver::reseparate_border(solver_map& m, border& parent_border, std::vector<border>& borders, bool solve) const
{
	auto resplit_borders = std::vector<border>();
	auto border_count = separate_borders(m, parent_border, resplit_borders);
	/*if(border_count == 0)
	{
	return;
	}*/
	/*if(border_count == 1)
	{
	borders.push_back(resplit_borders[0]);
	return;
	}*/
	for(auto& b : resplit_borders)
	{
		if(solve)
		{
			solve_border(m, b, false, borders);
		}
		else
		{
			borders.push_back(b);
		}
		for(auto& verdict : b.verdicts)
		{
			parent_border.verdicts[verdict.first] = verdict.second;
		}
		if(b.solved_fully)
		{
			for(auto& probability : b.probabilities)
			{
				parent_border.probabilities[probability.first] = probability.second;
			}
		}
	}
}

void solver::thr_find_combos(const solver_map& map, border& border, unsigned int min, unsigned int max, const std::vector<cell>& empty_cells, const CELL_INDICES_T& cell_indices, std::mutex& sync) const
{
	auto border_length = border.cells.size();
	auto all_remaining_cells_in_border = map.undecided_count == border_length;
	// TODO: Macro because calling the thread function here directly causes it to slow down about 50% for some reason. Figure out why
#define FIND_COMBOS_BODY(lck) \
for(unsigned int combo = min; combo < max; combo++)\
{\
	if(map.remaining_mine_count > 0)\
	{\
		auto bits_set = SWAR(combo);\
		if(bits_set > map.remaining_mine_count)\
		{\
			continue;\
		}\
		if(all_remaining_cells_in_border && bits_set != map.remaining_mine_count)\
		{\
			continue;\
		}\
	}\
\
	auto prediction_valid = is_prediction_valid(map, border, combo, empty_cells, cell_indices);\
	if(prediction_valid)\
	{\
		point_map<bool> predictions;\
		predictions.resize(border_length);\
		for(unsigned int j = 0; j < border_length; j++)\
		{\
			auto& pt = border.cells[j].pt;\
			auto has_mine = (combo & (1 << j)) > 0;\
			predictions[pt] = has_mine;\
		}\
		lck;\
		border.valid_combinations.push_back(predictions);\
	}\
}
	FIND_COMBOS_BODY(std::lock_guard<std::mutex> block(sync))		
}

void solver::find_valid_border_cell_combinations(solver_map& map, border& border) const
{
	auto border_length = border.cells.size();

	const int max_size = 31;
	if(border_length > max_size)
	{
		// TODO: handle too big border
		//throw new InvalidDataException($"Border with {borderLength} cells is too large, maximum {maxSize} cells allowed");
	}
	unsigned int total_combos = 1 << border_length;


	point_set empty_pts;
	//border.cell_indices.resize(border.cells.size());
	CELL_INDICES_T cell_indices;
	CELL_INDICES_RESIZE(cell_indices, border, map);
	//border.cell_indices.resize(map.width * map.height);
	for(auto i = 0; i < border.cells.size(); i++)
	{
		auto& c = border.cells[i];
		CELL_INDICES_ELEMENT(cell_indices, c.pt, map) = i;
		//border.cell_indices[c.pt] = i;
		auto& entry = map.neighbour_cache_get(c.pt).by_state[cell_state_empty];
		for(auto& cell : entry)
		{
			empty_pts.insert(cell.pt);
		}
	}

	auto all_remaining_cells_in_border = map.undecided_count == border_length;
	std::vector<cell> empty_cells;
	empty_cells.reserve(empty_pts.size());
	for(auto& pt : empty_pts)
	{
		empty_cells.push_back(map.cell_get(pt));
	}

	auto thread_count = std::thread::hardware_concurrency();
	if(border_length > settings.multithread_valid_combination_search_from_size && thread_count > 1)
	{
		std::mutex sync;
		auto thread_load = total_combos / thread_count;
		std::vector<std::thread> threads;
		for(unsigned i = 0; i < thread_count; i++)
		{
			unsigned int min = thread_load * i;
			unsigned int max = min + thread_load;
			if(i == thread_count - 1)
			{
				max = total_combos;
			}
			threads.emplace_back([this, &map, &border, min, max, &empty_cells, &cell_indices, &sync]()
			{
				thr_find_combos(map, border, min, max, empty_cells, cell_indices, sync);
			});
			//thr_find_combos(map, border, min, max, empty_cells, cell_indices, sync);
		}

		for(auto& thr : threads)
		{
			thr.join();
		}
	}
	else
	{
		unsigned int min = 0;
		unsigned int max = total_combos;
		//thr_find_combos(map, border, 0, total_combos, empty_cells, cell_indices, sync);
		FIND_COMBOS_BODY()
	}
}

int solver::SWAR(int i) const
{
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

bool solver::is_prediction_valid(const solver_map& map, const border& b, unsigned int prediction, const std::vector<cell>& empty_cells, const CELL_INDICES_T& cell_indices) const
{
	for(auto& cell : empty_cells)
	{
		auto neighbours_with_mine = 0;
		//auto neighbours_without_mine = 0;
		auto& filled_neighbours = map.neighbour_cache[cell.pt.x * map.height + cell.pt.y].by_state[cell_state_filled];
		for(auto& neighbour : filled_neighbours)
		{
			auto flag = neighbour.state & cell_flags;
			switch(flag)
			{	
			case cell_flag_has_mine:
				++neighbours_with_mine;
				break;
			case cell_flag_doesnt_have_mine:
				//	++neighbours_without_mine;
				break;
			default:
				/*unsigned int i;
				for(i = 0; i < b.cells.size(); i++)
				{
				if(neighbour.pt == b.cells[i].pt)
				{
				break;
				}
				}*/
				unsigned int i = CELL_INDICES_ELEMENT(cell_indices, neighbour.pt, map);
				//unsigned int i = b.cell_indices[neighbour.pt];
				auto verdict = (prediction & (1 << i)) > 0;
				if(verdict)
				{
					++neighbours_with_mine;
				}
				//else
				//{
				//	++neighbours_without_mine;
				//}
				break;
			}
		}

		if(neighbours_with_mine != cell.hint)
			return false;

		// TODO: What does this do?
		//if (filled_neighbours.size() - neighbours_without_mine < cell.hint)
		//	return false;

	}
	return true;
}

void solver::calculate_min_max_mine_counts(border& b) const
{
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
}

void solver::calculate_border_probabilities(border& b) const
{
	if(b.valid_combinations.size() == 0)
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
		if(fabs(probability.second) < 0.0000001)
		{
			verdict = false;
		}
		else if(fabs(probability.second - 1) < 0.0000001)
		{
			verdict = true;
		}
		else
		{
			continue;
		}
		target_verdicts[probability.first] = verdict;
	}
}

void solver::breadth_search_border(solver_map& m, point_set& allowed_coordinates, point target_coordinate, std::vector<cell>& target_cells) const
{
	point_set common_coords(allowed_coordinates);
	std::queue<point> coord_queue;
	coord_queue.push(target_coordinate);
	point_set visited;

	while(coord_queue.size() > 0)
	{
		auto& coord = coord_queue.front();
		coord_queue.pop();
		auto& cell = m[coord];
		if(common_coords.find(coord) != common_coords.end())
		{
			common_coords.erase(coord);
			target_cells.push_back(cell);
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

int solver::separate_borders(solver_map& m, border& common_border, std::vector<border>& target_borders) const
{
	point_set common_coords;
	for(auto c : common_border.cells)
	{
		common_coords.insert(c.pt);
	}
	auto index = target_borders.size();
	auto border_count = 0;
	while(common_coords.size() > 0)
	{
		border_count++;
		auto& initial_point = *common_coords.begin();
		target_borders.resize(++index);
		auto& brd = target_borders[index - 1];
		breadth_search_border(m, common_coords, initial_point, brd.cells);
		for(auto& c : brd.cells)
		{
			common_coords.erase(c.pt);
		}
	}
	return border_count;
}

bool solver::is_cell_border(solver_map& m, cell& c) const
{
	if(c.state != cell_state_filled)
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

void solver::solve_mine_counts(solver_map& m, border& common_border, std::vector<border>& borders, point_map<double>& all_probabilities, point_map<bool>& all_verdicts) const
{
	if (m.remaining_mine_count == -1)
	{
		return;
	}

	auto original_common_border_pts = point_set();
	original_common_border_pts.resize(common_border.cells.size());
	for(auto& c : common_border.cells)
	{
		original_common_border_pts.insert(c.pt);
	}

	auto non_border_cells = std::vector<cell>();
	for(auto& c : m.cells)
	{
		if(c.state == cell_state_filled && original_common_border_pts.find(c.pt) == original_common_border_pts.end())
		{
			non_border_cells.push_back(c);
		}
	}

	auto common_border_coords = original_common_border_pts;
	for(auto& v : all_verdicts)
	{
		common_border_coords.erase(v.first);
	}

	auto fully_solved_borders = std::vector<border>();
	auto unsolved_borders = std::vector<border>();
	for(auto& b : borders)
	{
		if(b.solved_fully)
		{
			fully_solved_borders.push_back(b);
		}
		else
		{
			unsolved_borders.push_back(b);
			for(auto& c : b.cells)
			{
				common_border_coords.erase(c.pt);
			}
		}
	}

	auto borders_with_exact_mine_count = std::vector<border>();
	auto borders_with_variable_mine_count = std::vector<border>();
	auto exact_mine_count = 0;
	auto exect_border_size = 0;
	for(auto i = 0; i < fully_solved_borders.size(); i++)
	{
		auto& b = fully_solved_borders[i];
		auto guaranteed_mines = 0;
		auto guaranteed_empty = 0;
		for (auto j = 0; j < fully_solved_borders.size(); j++)
		{
			if(i == j)
			{
				continue;
			}
			auto& other = fully_solved_borders[j];
			guaranteed_mines += other.min_mine_count;
			guaranteed_empty += other.cells.size() - other.max_mine_count;
		}
		trim_valid_combinations_by_mine_count(b, m.remaining_mine_count, m.undecided_count, guaranteed_mines, guaranteed_empty);
		if(b.min_mine_count == b.max_mine_count)
		{
			borders_with_exact_mine_count.push_back(b);
			for (auto& c : b.cells)
			{
				common_border_coords.erase(c.pt);
			}
			exact_mine_count += b.max_mine_count;
			exect_border_size += b.cells.size();
		}
		else
		{
			borders_with_variable_mine_count.push_back(b);
		}
	}

	auto non_border_mine_count_probabilities = google::dense_hash_map<int, double>();
	non_border_mine_count_probabilities.set_empty_key(-1);
	non_border_mine_count_probabilities.set_deleted_key(-2);
	if (borders_with_variable_mine_count.size() > 0)
	{
		auto exact_non_mine_count = exect_border_size - exact_mine_count;
		get_variable_mine_count_borders_probabilities(borders_with_variable_mine_count, m.remaining_mine_count, m.undecided_count, non_border_cells.size(), exact_mine_count, exact_non_mine_count, all_probabilities, non_border_mine_count_probabilities);
	}

	// If requested, we calculate the probabilities of mines in non-border cells, and copy them over.
	if (settings.mine_count_solve_non_border)
	{

		get_non_border_probabilities_by_mine_count(m, all_probabilities, non_border_cells, all_probabilities);
	}

	// Lastly, we find guaranteed verdicts from the probabilties, and copy them over.
	get_verdicts_from_probabilities(all_probabilities, all_verdicts);
}

void solver::trim_valid_combinations_by_mine_count(border& b, int minesRemaining, int undecidedCellsRemaining, int minesElsewhere, int nonMineCountElsewhere) const
{
	for (auto i = 0; i < b.valid_combinations.size(); i++)
	{
		auto& combination = b.valid_combinations[i];
		auto mine_prediction_count = 0;
		for(auto& p : combination)
		{
			if(p.second)
			{
				++mine_prediction_count;
			}
		}
		auto isValid = is_prediction_valid_by_mine_count(mine_prediction_count, combination.size(), minesRemaining, undecidedCellsRemaining, minesElsewhere, nonMineCountElsewhere);
		if (!isValid)
		{
			b.valid_combinations.erase(b.valid_combinations.begin() + i);
			i--;
		}
	}
}

bool solver::is_prediction_valid_by_mine_count(int minePredictionCount, int totalCombinationLength, int minesRemaining, int undecidedCellsRemaining, int minesElsewhere, int nonMineCountElsewhere) const
{
	if (minePredictionCount + minesElsewhere > minesRemaining)
	{
		return false;
	}
	if (minesRemaining - minePredictionCount > undecidedCellsRemaining - totalCombinationLength - nonMineCountElsewhere)
	{
		return false;
	}
	return true;
}

static std::vector<std::vector<double>> combination_ratios = std::vector<std::vector<double>>();

static void initialize_combination_ratios()
{
	const auto max_size = 1000;
	combination_ratios.resize(max_size);
	for (auto n = 0; n < max_size; n++)
	{
		auto& ratios = combination_ratios[n];
		ratios.resize(max_size);
		//CombinationRatios[n] = ratios;
		ratios[0] = 1;
		for (auto k = 1; k <= n; k++)
		{
			auto ratio = (n + 1 - k) / double(k);
			ratios[k] = ratio;
		}
	}
}

inline static double combination_ratio(int from, int count)
{
	if(combination_ratios.size() == 0)
	{
		initialize_combination_ratios();
	}
	assert(count <= from);
	return combination_ratios[from][count];
}

void solver::thr_mine_counts(std::vector<border>& variable_borders, int min, int max, int mines_remaining, int mines_elsewhere, int non_border_cell_count, int total_combination_length, int undecided_cells_remaining, int non_mine_count_elsewhere, google::dense_hash_map<int, double>& ratios, google::dense_hash_map<int, double>& non_border_mine_counts, point_map<double>& counts, std::mutex& common_lock, point_map<std::mutex*>& count_locks) const
{
	for (auto i = min; i < max; i++)
	{
		auto combinationArr = std::vector<point_map<bool>>();
		auto minePredictionCount = 0;
		for (auto j = 0; j < variable_borders.size(); j++)
		{
			lldiv_t res{ i, 0 };
			res = div(res.quot, variable_borders[j].valid_combinations.size());
			auto& combo = variable_borders[j].valid_combinations[res.rem];
			for (auto& p : combo)
			{
				if (p.second)
				{
					++minePredictionCount;
				}
			}
			combinationArr.push_back(combo);
		}

		auto mines_in_non_border = mines_remaining - mines_elsewhere - minePredictionCount;
		if (mines_in_non_border < 0)
		{
			return;
		}
		if (mines_in_non_border > non_border_cell_count)
		{
			return;
		}
		auto& ratio = ratios[mines_in_non_border];
		{
			std::lock_guard<std::mutex> guard(common_lock);
			non_border_mine_counts[mines_in_non_border] += ratio;
		}
		auto isValid = is_prediction_valid_by_mine_count(minePredictionCount, total_combination_length, mines_remaining, undecided_cells_remaining, mines_elsewhere, non_mine_count_elsewhere);
		if (!isValid)
		{
			//throw new Exception("temp");
			throw "temp";
		}
		for (auto& combo : combinationArr)
		{
			for (auto& verdict : combo)
			{
				if (verdict.second)
				{
					auto& mutex = count_locks[verdict.first];
					std::lock_guard<std::mutex> guard(*mutex);
					++counts[verdict.first];
				}
			}
		}
	}
}

void solver::get_variable_mine_count_borders_probabilities(std::vector<border>& variable_borders, int mines_remaining, int undecided_cells_remaining, int non_border_cell_count, int mines_elsewhere, int non_mine_count_elsewhere, point_map<double>& target_probabilities, google::dense_hash_map<int, double>& non_border_mine_count_probabilities) const
{
	auto max_mines = 0;
	auto min_mines = 0;
	auto alreadyFoundMines = 0;
	auto total_combination_length = 0;
	auto total_combos = 1;
	auto counts = point_map<double>();
	auto count_locks = point_map<std::mutex*>();
	std::mutex common_lock;;
	for(auto& b : variable_borders)
	{
		for(auto& v : b.verdicts)
		{
			if(v.second)
			{
				++alreadyFoundMines;
			}
		}
		for(auto& c : b.cells)
		{
			counts[c.pt] = 0;
			count_locks[c.pt] = new std::mutex();
		}
		max_mines += b.max_mine_count;
		min_mines += b.min_mine_count;
		total_combination_length += b.cells.size();
		total_combos *= b.valid_combinations.size();
	}
	
	auto min_mines_in_non_border = mines_remaining - mines_elsewhere - max_mines + alreadyFoundMines;
	if (min_mines_in_non_border < 0)
	{
		min_mines_in_non_border = 0;
	}
	auto max_mines_in_non_border = mines_remaining - mines_elsewhere - min_mines + alreadyFoundMines;
	if (max_mines_in_non_border > non_border_cell_count)
	{
		max_mines_in_non_border = non_border_cell_count;
	}

	auto ratios = google::dense_hash_map<int, double>();
	ratios.set_empty_key(-1);
	ratios.set_deleted_key(-2);
	auto non_border_mine_counts = google::dense_hash_map<int, double>();
	non_border_mine_counts.set_empty_key(-1);
	non_border_mine_counts.set_deleted_key(-2);
	double currentRatio = 1;
	for (auto i = min_mines_in_non_border; i <= max_mines_in_non_border; i++)
	{
		currentRatio *= combination_ratio(non_border_cell_count, i);
		ratios[i] = currentRatio;
		non_border_mine_counts[i] = 0;
	}


	auto thread_count = std::thread::hardware_concurrency();
	if (total_combos > settings.multithread_variable_mine_count_borders_probabilities && thread_count > 1)
	{
		auto thread_load = total_combos / thread_count;
		std::vector<std::thread> threads;
		for (unsigned i = 0; i < thread_count; i++)
		{
			unsigned int min = thread_load * i;
			unsigned int max = min + thread_load;
			if (i == thread_count - 1)
			{
				max = total_combos;
			}
 			threads.emplace_back([this, min, max, mines_remaining, mines_elsewhere, non_border_cell_count, total_combination_length, undecided_cells_remaining, non_mine_count_elsewhere, &variable_borders, &ratios, &non_border_mine_counts, &counts, &common_lock, &count_locks]()
			{
				thr_mine_counts(variable_borders, min, max, mines_remaining, mines_elsewhere, non_border_cell_count, total_combination_length, undecided_cells_remaining, non_mine_count_elsewhere, ratios, non_border_mine_counts, counts, common_lock, count_locks);
			});
		}

		for (auto& thr : threads)
		{
			thr.join();
		}
	}
	else
	{
		thr_mine_counts(variable_borders, 0, total_combos, mines_remaining, mines_elsewhere, non_border_cell_count, total_combination_length, undecided_cells_remaining, non_mine_count_elsewhere, ratios, non_border_mine_counts, counts, common_lock, count_locks);
	}

	for(auto& mutex_ptr : count_locks)
	{
		delete mutex_ptr.second;
	}

	auto total_valid_combinations = 0;
	for(auto& mc : non_border_mine_counts)
	{
		total_valid_combinations += mc.second;
	}
	for(auto& mc : non_border_mine_counts)
	{
		if(mc.second < 0.0000000001)
		{
			continue;
		}
		non_border_mine_count_probabilities[mc.first] = mc.second / total_valid_combinations;
	}
	for(auto& count : counts)
	{
		auto probability = count.second / total_valid_combinations;
		target_probabilities[count.first] = probability;
	}
}

void solver::get_non_border_probabilities_by_mine_count(solver_map& map, point_map<double>& common_border_probabilities, std::vector<cell>& non_border_undecided_cells, point_map<double>& probabilities) const
{
	if (map.remaining_mine_count == 0)
	{
		return;
	}

	double probabilitySum = 0;
	for (auto& p : common_border_probabilities)
	{
		probabilitySum += p.second;
	}
	if (non_border_undecided_cells.size() == 0)
	{
		return;
	}
	auto nonBorderProbability = (map.remaining_mine_count - probabilitySum) / non_border_undecided_cells.size();
	for(auto& non_border_filled_cell : non_border_undecided_cells)
	{
		probabilities[non_border_filled_cell.pt] = nonBorderProbability;
	}
}

bool solver::should_stop_solving(point_map<bool>& verdicts, bool stop_on_no_mine_verdict, bool stop_on_any_verdict, bool stop_always) const
{
	if(stop_always)
	{
		return true;
	}
	if(verdicts.size() == 0)
	{
		return false;
	}
	if(stop_on_any_verdict)
	{
		return true;
	}
	if(stop_on_no_mine_verdict)
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

void solver::get_final_results(point_map<double>& probabilities, point_map<bool>& verdicts, point_map<solver_result>& results) const
{
	for(auto& probability : probabilities)
	{
		auto& result = results[probability.first];
		result.pt = probability.first;
		result.probability = probability.second;
		result.verdict = verdict_none;
	}
	for(auto& verdict : verdicts)
	{
		auto& result = results[verdict.first];
		result.pt = verdict.first;
		result.probability = verdict.second ? 1 : 0;
		result.verdict = verdict.second ? verdict_has_mine : verdict_doesnt_have_mine;
	}
}