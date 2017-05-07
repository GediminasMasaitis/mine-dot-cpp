#include "solver_map.h"
#include <unordered_map>
#include <unordered_set>

using namespace minedotcpp::common;
using namespace minedotcpp::solvers;

void solver_map::init_from(const map& base_map)
{
	width = base_map.width;
	height = base_map.height;
	remaining_mine_count = base_map.remaining_mine_count;
	cells.resize(width*height);
	auto size = width*height;
	for(auto i = 0; i < size; ++i)
	{
		cells[i] = base_map.cells[i];
	}
	calculate_additional_data();
}

void solver_map::calculate_additional_data()
{
	filled_count = 0;
	flagged_count = 0;
	anti_flagged_count = 0;
	undecided_count = 0;
	auto size = width*height;
	for(auto i = 0; i < size; ++i)
	{
		
		auto state = cells[i].state & cell_states;
		if(state == cell_state_filled)
		{
			if(cells[i].state == (cell_state_filled | cell_flag_has_mine))
			{
				filled_count++;
				flagged_count++;
			}
			else if(cells[i].state == (cell_state_filled | cell_flag_doesnt_have_mine))
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
	build_neighbour_cache();
}

void solver_map::build_neighbour_cache()
{
	neighbour_cache.resize(width * height);
	for (short i = 0; i < width; ++i)
	{
		for (short j = 0; j < height; ++j)
		{
			auto& entry = neighbour_cache[i*height + j];
			auto cell = cell_get(i, j);
			if(entry.initialized)
			{
				entry.all_neighbours.clear();
				for(auto k = 0; k < 4; k++)
				{
					entry.by_state[k].clear();
				}
				for(auto k = 0; k < 16; k++)
				{
					entry.by_flag[k].clear();
					entry.by_param[k].clear();
				}

			}
			calculate_neighbours_of(cell.pt, entry.all_neighbours, false);
			build_additional_neighbour_lists(entry);
			entry.initialized = true;
		}
	}
}

void solver_map::set_cells_by_verdicts(point_map<bool>& verdicts)
{
	std::unordered_set<point, point_hash> points_to_update;
	for(auto& result : verdicts)
	{
		auto& cell = cell_get(result.first);
		if (result.second)
		{
			cell.state = cell_state_filled | cell_flag_has_mine;
			flagged_count++;
			undecided_count--;
			if (remaining_mine_count > 0)
			{
				remaining_mine_count--;
			}
		}
		else
		{
			cell.state = cell_state_filled | cell_flag_doesnt_have_mine;
			anti_flagged_count++;
			undecided_count--;
		}
		auto& entry = neighbour_cache_get(result.first);
		for(auto& neighbour : entry.all_neighbours)
		{
			points_to_update.insert(neighbour.pt);
		}
	}
	for(auto pt : points_to_update)
	{
		update_neighbour_cache(pt);
	}
}

void solver_map::update_neighbour_cache(point pt)
{
	auto& entry = neighbour_cache_get(pt);
	entry.all_neighbours.clear();
	calculate_neighbours_of(pt, entry.all_neighbours, false);
	for(auto i = 0; i < 4; i++)
	{
		entry.by_state[i].clear();
		entry.by_flag[i << 2].clear();
		entry.by_param[i].clear();
		entry.by_param[(cell_state_filled | i << 2)].clear();
	}
	/*for(auto i = 0; i < 16; i++)
	{
		entry.by_flag[i].clear();
		entry.by_param[i].clear();
	}*/
	build_additional_neighbour_lists(entry);
}

void solver_map::build_additional_neighbour_lists(neighbour_cache_entry& entry)
{
	for (auto& neighbour : entry.all_neighbours)
	{
		auto param = neighbour.state;
		entry.by_param[param].push_back(neighbour);

		auto state = param & cell_states;
		entry.by_state[state].push_back(neighbour);

		auto flag = param & cell_flags;
		entry.by_flag[flag].push_back(neighbour);
	}
}

static const point neighbour_offsets[8] = {
	{ -1,-1 },
	{ -1, 0 },
	{ -1, 1 },
	{ 0,-1 },
	{ 0, 1 },
	{ 1,-1 },
	{ 1, 0 },
	{ 1, 1 }
};

static const int neighbour_offset_count = 8;

void solver_map::calculate_neighbours_of(point pt, std::vector<cell> &cells, bool include_self = false)
{
	for (auto i = 0; i < neighbour_offset_count; i++)
	{
		auto cell_ptr = cell_try_get(pt + neighbour_offsets[i]);
		if (cell_ptr != nullptr)
		{
			auto& c = *cell_ptr;
			cells.push_back(c);
		}
	}
	if (include_self)
	{
		cells.push_back(cell_get(pt));
	}
}