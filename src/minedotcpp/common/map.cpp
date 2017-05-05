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
	//cells = new std::vector<cell>(width*height);
	cells.resize(width*height);
	for (int i = 0; i < width; ++i)
	{
		for (int j = 0; j < height; ++j)
		{
			auto& cell = cell_get(i, j);
			cell.pt.x = i;
			cell.pt.y = j;
		}
	}
}

cell* map::cell_try_get(int x, int y)
{
	if(x >= 0 && y >= 0 && x < width && y < height)
	{
		auto& cell = cell_get(x,y);
		if(cell.state != cell_state_wall)
		{
			return &cell;
		}
	}
	return nullptr;
}

inline bool map::cell_exists(int x, int y)
{
	return x >= 0 && y >= 0 && x < width && y < height && cell_get(x, y).state != cell_state_wall;
}

inline bool map::cell_exists(point pt)
{
	return cell_exists(pt.x, pt.y);
}



