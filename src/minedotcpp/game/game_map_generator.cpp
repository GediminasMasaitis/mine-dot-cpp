#include "../common/common_macros.h"
#include "../common/point.h"
#include "game_map.h"
#include "game_map_generator.h"

inline void minedotcpp::game::game_map_generator::generate_with_mine_density(minedotcpp::game::game_map& m, minedotcpp::common::point starting_position, bool guarantee_opening, double mine_density)
{
	if (mine_density < 0 || mine_density > 1)
	{
		throw nameof(mine_density);
	}
	auto area = m.width * m.height;
	auto count = static_cast<int>(area * mine_density);
	generate_with_mine_count(m, starting_position, guarantee_opening, count);
}

void minedotcpp::game::game_map_generator::generate_with_mine_count(game_map& m, common::point starting_position, bool guarantee_opening, int mine_count)
{
	auto area = m.width * m.height;
	auto coordinates = std::vector<common::point>();
	coordinates.reserve(area);
	for(auto i = 0; i < m.width; i++)
	{
		for(auto j = 0; j < m.height; j++)
		{
			coordinates.push_back({i,j});
		}
	}

	m.remaining_mine_count = mine_count;
	//var map = new GameMap(width, height, mine_count, true, CellState.Filled);

	if(starting_position.x > -1 && starting_position.y > -1)
	{
		common::vector_erase_value_quick<common::point>(coordinates, starting_position);
		if(guarantee_opening)
		{
			auto starting_neighbours = std::vector<common::cell>();
			m.calculate_neighbours_of(starting_position, starting_neighbours);
			for(auto& starting_neighbour : starting_neighbours)
			{
				common::vector_erase_value_quick(coordinates, starting_neighbour.pt);
			}
		}
		//map[startingPosition].State = CellState.Empty;
	}
	std::shuffle(coordinates.begin(), coordinates.end(), random_engine);

	for(auto i = 0; i < mine_count; i++)
	{
		auto& coord = coordinates[i];
		m.cell_get(coord).has_mine = true;
	}

	for(auto& c : m.cells)
	{
		if(c.has_mine)
		{
			continue;
		}
		auto neighbours = std::vector<common::cell>();
		m.calculate_neighbours_of(c.pt, neighbours);
		c.hint = 0;
		for(auto& neighbour : neighbours)
		{
			++c.hint;
		}
	}
}
