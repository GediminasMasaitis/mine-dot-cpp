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



