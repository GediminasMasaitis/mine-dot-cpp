#include <string>
#include <sstream>

#include "text_map_parser.h"

using namespace std;
using namespace minedotcpp::common;
using namespace minedotcpp::mapio;

void remove_chars_from_string(string& str, char* chars_to_remove)
{
	for(unsigned int i = 0; i < strlen(chars_to_remove); ++i)
	{
		str.erase(remove(str.begin(), str.end(), chars_to_remove[i]), str.end());
	}
}

void try_skip_utf8_bom(istream& input_stream)
{
	const uint8_t a = input_stream.get();
	const uint8_t b = input_stream.get();
	const uint8_t c = input_stream.get();
	if (a != static_cast<unsigned char>(0xEF) || b != static_cast<unsigned char>(0xBB) || c != static_cast<unsigned char>(0xBF))
	{
		input_stream.seekg(0);
	}
}

void parse_cell(cell& cell, const char input_char)
{
	switch (input_char)
	{
	case '.':
		cell.state = cell_state_empty;
		cell.hint = 0;
		break;
	case '#':
		cell.state = cell_state_filled;
		cell.hint = 0;
		break;
	case '?':
		cell.state = cell_state_filled;
		cell.hint = 0;
		break;
	case '!':
		cell.state = cell_state_filled | cell_flag_has_mine;
		cell.hint = 0;
		break;
	case 'v':
		cell.state = cell_state_filled | cell_flag_doesnt_have_mine;
		cell.hint = 0;
		break;
	case 'X':
		cell.state = cell_state_wall;
		cell.hint = 0;
		break;
	default:
		cell.state = cell_state_empty;
		cell.hint = input_char - '0';
		break;
	}
}

void read_map_lines(istream& input_stream, vector<string>& lines, int32_t& remaining_mine_count)
{
	uint32_t height = 0;
	for (string line; getline(input_stream, line);)
	{
		remove_chars_from_string(line, "\r");
		const auto line_length = static_cast<uint32_t>(line.length());

		if (line_length == 0)
		{
			continue;
		}

		if (line[0] == 'm')
		{
			remaining_mine_count = stoi(line.substr(1));
			continue;
		}

		if (height != 0 && line_length != height)
		{
			throw invalid_argument("Invalid non-rectangle map");
		}

		height = line_length;
		lines.push_back(line);
	}

	if (lines.empty())
	{
		throw invalid_argument("Invalid empty map");
	}
}

void text_map_parser::parse(char* str, map& target_map) const
{
	istringstream iss(str);
	parse(iss, target_map);
}

void text_map_parser::parse(const string& str, map& target_map) const
{
	istringstream iss(str);
	parse(iss, target_map);
}

void text_map_parser::parse(istream& input_stream, map& target_map) const
{
	try_skip_utf8_bom(input_stream);

	int32_t remaining_mine_count = -1;
	vector<string> lines;
	read_map_lines(input_stream, lines, remaining_mine_count);

	const auto width = lines.size();
	const auto height = lines[0].size();
	target_map.init(width, height, remaining_mine_count, cell_state_empty);

	auto i = 0;
	for(auto& line : lines)
	{
		for (size_t j = 0; j < height; ++j)
		{
			auto& cell = target_map.cell_get(i, j);
			const auto input_char = line[j];
			parse_cell(cell, input_char);
		}
		++i;
	}
}
