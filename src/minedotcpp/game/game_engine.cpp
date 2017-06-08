#include "game_engine.h"

inline void minedotcpp::game::game_engine::start_with_mine_density(int width, int height, minedotcpp::common::point starting_position, bool guarantee_opening, double mine_density)
{
	gm = minedotcpp::game::game_map();
	gm.init(width, height, -1, common::cell_state_filled);
	generator->generate_with_mine_density(gm, starting_position, guarantee_opening, mine_density);
	if(starting_position.x > -1 && starting_position.y > -1)
	{
		open_cell(starting_position);
	}
}

void minedotcpp::game::game_engine::start_with_mine_count(int width, int height, common::point starting_position, bool guarantee_opening, int mine_count)
{
	gm = game_map();
	gm.init(width, height, -1, common::cell_state_filled);
	generator->generate_with_mine_count(gm, starting_position, guarantee_opening, mine_count);
	if (starting_position.x > -1 && starting_position.y > -1)
	{
		open_cell(starting_position);
	}
}

void minedotcpp::game::game_engine::set_flag(common::point pt, common::cell_param flag)
{
	auto& cell = gm.cell_get(pt);
	auto old_flag = cell.state & common::cell_flags;
	if (gm.remaining_mine_count != -1)
	{
		if (old_flag != common::cell_flag_has_mine && flag == common::cell_flag_has_mine)
		{
			--gm.remaining_mine_count;
		}
		else if (old_flag == common::cell_flag_has_mine && flag != common::cell_flag_has_mine)
		{
			++gm.remaining_mine_count;
		}
	}
	cell.state = (cell.state & ~common::cell_flags) | flag;
}

void minedotcpp::game::game_engine::toggle_flag(common::point pt)
{
	auto& cell = gm[pt];
	auto old_flag = cell.state & common::cell_flags;
	switch (old_flag)
	{
	case common::cell_flag_none:
		set_flag(pt, common::cell_flag_has_mine);
		break;
	case common::cell_flag_has_mine:
		set_flag(pt, common::cell_flag_none);
		break;
	default:
		throw "wut";
	}
}

minedotcpp::game::game_result minedotcpp::game::game_engine::open_cell(common::point pt)
{
	auto& initialCell = gm[pt];
	if (initialCell.has_mine)
	{
		return game_lost;
	}

	auto visited = common::point_set();
	auto to_open = common::point_set();
	to_open.insert(pt);
	visited.insert(pt);
	while (to_open.size() > 0)
	{
		auto& coord = *to_open.begin();
		auto& c = gm.cell_get(coord);
		c.state = common::cell_state_empty;
		to_open.erase(coord);
		if (c.hint == 0)
		{
			auto neighbours = std::vector<game_cell*>();
			gm.calculate_neighbours_of(c.pt, neighbours);
			for (auto& neighbour : neighbours)
			{
				if (neighbour->state == common::cell_state_filled)
				{
					if (visited.find(neighbour->pt) == visited.end())
					{
						to_open.insert(neighbour->pt);
						visited.insert(neighbour->pt);
					}
				}
			}
		}
	}
	for (auto& p : visited)
	{
		gm.cell_get(p).state = common::cell_state_empty;
	}
	/*if(gm.remaining_mine_count > 0)
	{
		return move_correct;
	}*/
	for (auto& c : gm.cells)
	{
		if (c.state == common::cell_state_filled && !c.has_mine)
		{
			return move_correct;
		}
	}
	return game_won;
}
