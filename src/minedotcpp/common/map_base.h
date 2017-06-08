#pragma once
#include <vector>
#include "point.h"
#include "cell.h"

namespace minedotcpp
{
	namespace common
	{
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

		template <typename TCell>
		class map_base
		{
		public:
			int width;
			int height;
			int remaining_mine_count;
			std::vector<TCell> cells;

			map_base()
			{
				width = -1;
				height = -1;
				remaining_mine_count = -1;
			}


			TCell& cell_get(int x, int y) { return cells[x*height + y]; }
			TCell& cell_get(point pt) { return cell_get(pt.x, pt.y); }
			TCell& operator[] (point pt) { return cell_get(pt); }
			TCell* cell_try_get(int x, int y)
			{
				if(x >= 0 && y >= 0 && x < width && y < height)
				{
					auto& cell = cell_get(x, y);
					if(cell.state != cell_state_wall) return &cell;
				}
				return nullptr;
			}
			TCell* cell_try_get(point pt) { return cell_try_get(pt.x, pt.y); }
			bool cell_exists(int x, int y) { return x >= 0 && y >= 0 && x < width && y < height && cell_get(x, y).state != cell_state_wall; }
			bool cell_exists(point pt) { return cell_exists(pt.x, pt.y); }

			void init(int width, int height, int remaining_mine_count, cell_param state)
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

			void calculate_neighbours_of(point pt, std::vector<TCell*>& cells, bool include_self = false)
			{
				for (auto i = 0; i < neighbour_offset_count; i++)
				{
					auto cell_ptr = cell_try_get(pt + neighbour_offsets[i]);
					if (cell_ptr != nullptr)
					{
						cells.push_back(cell_ptr);
					}
				}
				if (include_self)
				{
					cells.push_back(&cell_get(pt));
				}
			}
		};
	}
}