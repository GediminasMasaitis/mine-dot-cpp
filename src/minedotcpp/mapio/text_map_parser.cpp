#include <string>
#include <sstream>

#include "text_map_parser.h"

using namespace std;
using namespace minedotcpp::common;
using namespace minedotcpp::mapio;


void text_map_parser::parse(string str, map& target_map) const
{
	istringstream iss(str);
	parse(iss, target_map);
}

void text_map_parser::parse(istream& is, map& m) const
{
	int remaining_mine_count = -1;
	vector<string> lines;
	unsigned int height = 0;
	unsigned char a = is.get();
	unsigned char b = is.get();
	unsigned char c = is.get();
	if (a != static_cast<unsigned char>(0xEF) || b != static_cast<unsigned char>(0xBB) || c != static_cast<unsigned char>(0xBF)) {
		is.seekg(0);
	}
	for (string line; getline(is, line);)
	{
		auto line_len = static_cast<unsigned int>(line.length());
		if(line_len == 0)
		{
			continue;
		}
		if(line_len > height)
		{
			height = line_len;
		}
		if(line[0] == 'm')
		{
			remaining_mine_count = stoi(line.substr(1));
			continue;
		}
		lines.push_back(line);
	}
	auto width = static_cast<int>(lines.size());
	m.init(width, height, remaining_mine_count, cell_state_empty);
	int i = 0;
	for(auto& line : lines)
	{
		for (unsigned int j = 0; j < height; ++j)
		{
			auto& c = m.cell_get(i, j);
			switch (line[j])
			{
			case '.':
				c.state = cell_state_empty;
				c.hint = 0;
				break;
			case '#':
				c.state = cell_state_filled;
				c.hint = 0;
				break;
			case '?':
				c.state = cell_state_filled | cell_flag_not_sure;
				c.hint = 0;
				break;
			case '!':
				c.state = cell_state_filled | cell_flag_has_mine;
				c.hint = 0;
				break;
			case 'v':
				c.state = cell_state_filled | cell_flag_doesnt_have_mine;
				c.hint = 0;
				break;
			case 'X':
				c.state = cell_state_wall;
				c.hint = 0;
				break;
			default:
				c.state = cell_state_empty;
				c.hint = line[j] - '0';
				break;
			}
		}
		++i;
	}
}
