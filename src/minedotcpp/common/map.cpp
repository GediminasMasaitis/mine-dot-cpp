#include "map.h"
#include "cell.h"
#include "neighbour_cache_entry.h"

using namespace minedotcpp::common;

void map::init(int width, int height, int remaining_mine_count, cell_param state)
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