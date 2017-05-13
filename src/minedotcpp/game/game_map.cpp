#include "../common/map.h"
#include "game_map.h"

void minedotcpp::game::game_map::to_regular_map(minedotcpp::common::map& m)
{
	m.width = width;
	m.height = height;
	m.remaining_mine_count = remaining_mine_count;
	m.cells.resize(width*height);
	auto size = width*height;
	for(auto i = 0; i < size; ++i)
	{
		auto& source = cells[i];
		auto& target = m.cells[i];
		target.pt = source.pt;
		target.state = source.state;
		target.hint = source.hint;
	}
}
