#include <string>
#include <sstream>

#include "text_map_visualizer.h"

using namespace std;
using namespace minedotcpp::common;
using namespace minedotcpp::mapio;


string text_map_visualizer::visualize_to_string(minedotcpp::common::map& m) const
{
	ostringstream oss;
	visualize(m, oss);
	return oss.str();
}

void text_map_visualizer::visualize(minedotcpp::common::map& m, ostream& os) const
{
	for (int i = 0; i < m.width; i++)
	{
		for (int j = 0; j < m.height; j++)
		{
			auto& cell = m.cell_get(i, j);
			auto state = cell.state & cell_states;
			switch(state)
			{
			case cell_state_empty:
				if(cell.hint == 0)
				{
					os << '.';
				}
				else
				{
					os << cell.hint;
				}
				break;
			case cell_state_filled:
			{
				auto flag = cell.state & cell_flags;
				switch (flag)
				{
				case cell_flag_has_mine:
					os << '!';
					break;
				case cell_flag_doesnt_have_mine:
					os << 'v';
					break;
				case cell_flag_not_sure:
					os << '?';
					break;
				default:
					os << '#';
					break;
				}
				
			}
			break;
			case cell_state_wall:
				os << 'X';
				break;
			default:
				break;
			}
		}
		os << endl;
	}
}
