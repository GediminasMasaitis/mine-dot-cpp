#include "map.h"
#include "cell.h"
#include "neighbour_cache_entry.h"

using namespace minedotcpp::common;

template<typename TCell>
map_base<TCell>::map_base()
{
	width = -1;
	height = -1;
}

template<typename TCell>
void map_base<TCell>::init(int width, int height, int remaining_mine_count, cell_param state)
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
			cell.state = state;
			cell.hint = 0;
		}
	}
}

template<typename TCell>
TCell* map_base<TCell>::cell_try_get(int x, int y)
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

template<typename TCell>
inline bool map_base<TCell>::cell_exists(int x, int y)
{
	return x >= 0 && y >= 0 && x < width && y < height && cell_get(x, y).state != cell_state_wall;
}

template<typename TCell>
inline bool map_base<TCell>::cell_exists(point pt)
{
	return cell_exists(pt.x, pt.y);
}



