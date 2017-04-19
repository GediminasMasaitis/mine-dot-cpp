#include "map.h"
#include "stdafx.h"

map::map(unsigned short width, unsigned short height)
{
	width = width;
	height = height;
	cells = new cell[width*height];
	for (auto i = 0; i < width; ++i)
	{
		for (auto j = 0; j < height; ++j)
		{
			auto cell = cell_at(i, j);
			cell->point.x = i;
		}
	}
}

map::~map()
{
	delete[] cells;
}

inline cell* map::cell_at(int x, int y)
{
	return &cells[x*height + y];
}