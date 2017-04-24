#include "map.h"
#include "cell.h"
#include "neighbour_cache_entry.h"

using namespace minedotcpp::common;

map::map()
{
	
}

map::map(int width, int height, int remaining_mine_count)
{
	this->width = width;
	this->height = height;
	this->remaining_mine_count = remaining_mine_count;
	cells = new cell[width*height];
	for (int i = 0; i < width; ++i)
	{
		for (int j = 0; j < height; ++j)
		{
			auto cell = cell_get(i, j);
			cell->pt.x = i;
			cell->pt.y = j;
		}
	}
	neighbour_cache = nullptr;
}

map::~map()
{
	delete[] cells;
	if(neighbour_cache)
	{
		delete[] neighbour_cache;
	}
}

inline cell* map::cell_get(int x, int y)
{
	return &cells[x*height + y];
}

inline cell* map::cell_get(point pt)
{
	return cell_get(pt.x, pt.y);
}

cell* map::cell_try_get(int x, int y)
{
	if(x >= 0 && y >= 0 && x < width && y < height)
	{
		auto cell = cell_get(x,y);
		if(cell->state != cell_state_wall)
		{
			return cell;
		}
	}
	return nullptr;
}

inline cell* map::cell_try_get(point pt)
{
	return cell_try_get(pt.x, pt.y);
}

inline bool map::cell_exists(int x, int y)
{
	return x >= 0 && y >= 0 && x < width && y < height && cells[x, y].state != cell_state_wall;
}

inline bool map::cell_exists(point pt)
{
	return cell_exists(pt.x, pt.y);
}

inline neighbour_cache_entry* map::neighbour_cache_get(int x, int y)
{
	return &neighbour_cache[x*height + y];
}

inline neighbour_cache_entry* map::neighbour_cache_get(point pt)
{
	return neighbour_cache_get(pt.x, pt.y);
}

void map::build_neighbour_cache()
{
	if(neighbour_cache)
	{
		delete[] neighbour_cache;
	}
	neighbour_cache = new neighbour_cache_entry[width * height];
	for (short i = 0; i < width; ++i)
	{
		for (short j = 0; j < height; ++j)
		{
			auto& cache_entry = neighbour_cache[i*height + j];
			auto cell = cell_get(i, j);
			calculate_neighbours_of(cell->pt, cache_entry.all_neighbours, false);
			for(auto& neighbour : cache_entry.all_neighbours)
			{
				auto param = neighbour->state;
				cache_entry.by_param[param].push_back(neighbour);

				auto state = param & cell_states;
				cache_entry.by_state[state].push_back(neighbour);

				auto flag = param & cell_flags;
				cache_entry.by_flag[flag].push_back(neighbour);
			}
		}
	}
}

static const point neighbour_offsets[8] = {
	{-1,-1 },
	{-1, 0 },
	{-1, 1 },
	{ 0,-1 },
	{ 0, 1 },
	{ 1,-1 },
	{ 1, 0 },
	{ 1, 1 }
};

static const int neighbour_offset_count = 8;

void map::calculate_neighbours_of(point pt, std::vector<cell*> &cells, bool include_self = false)
{
	for(auto i = 0; i < neighbour_offset_count; i++)
	{
		if(auto c = cell_try_get(pt + neighbour_offsets[i]))
		{
			cells.push_back(c);
		}
	}
	if (include_self)
	{
		cells.push_back(cell_get(pt));
	}
}
