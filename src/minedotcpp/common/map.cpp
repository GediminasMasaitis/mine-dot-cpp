#include "map.h"

using namespace minedotcpp::common;

neighbour_cache_entry::neighbour_cache_entry()
{

}

neighbour_cache_entry::~neighbour_cache_entry()
{

}

map::map(unsigned short width, unsigned short height)
{
	this->width = width;
	this->height = height;
	cells = new cell[width*height];
	for (unsigned short i = 0; i < width; ++i)
	{
		for (unsigned short j = 0; j < height; ++j)
		{
			auto cell = cell_get(i, j);
			cell->point.x = i;
			cell->point.y = j;
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

inline cell* map::cell_get(unsigned short x, unsigned short y)
{
	return &cells[x*height + y];
}

inline cell* map::cell_get(point pt)
{
	return cell_get(pt.x, pt.y);
}

cell* map::cell_try_get(unsigned short x, unsigned short y)
{
	if(x >= 0 && y >= 0 && x < width && y < height)
	{
		auto cell = cell_get(x,y);
		if(cell->state != cell_state_empty)
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

inline bool map::cell_exists(unsigned short x, unsigned short y)
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
	for (unsigned short i = 0; i < width; ++i)
	{
		for (unsigned short j = 0; j < height; ++j)
		{
			auto& cache_entry = neighbour_cache[i*height + j];
			auto cell = cell_get(i, j);
			calculate_neighbours_of(cell->point, cache_entry.all_neighbours);
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

static const point* neighbour_offsets = new point[8] {
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
