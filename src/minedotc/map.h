#pragma once
#include <vector>
#include <unordered_map>

using namespace std;

enum cell_param
{
	// State
	cell_state_empty = 0,
	cell_state_filled = 1 << 0,
	cell_state_wall = 1 << 1,

	// Flags
	cell_flag_has_mine = 1 << 2,
	cell_flag_doesnt_have_mine = 1 << 3,
	cell_flag_not_sure = 1 << 4
};

struct point
{
	unsigned short x;
	unsigned short y;
};

class cell
{
public:
	point point;
	cell_param state;
};

class neighbour_cache_entry
{
public:
	vector<point,cell> all_neighbours;
	unordered_map<cell_param, vector<cell>> by_state;
	unordered_map<cell_param, vector<cell>> by_flag;
	unordered_map<cell_param, vector<cell>> by_param;
};

class map
{
public:
	unsigned short width;
	unsigned short height;
	cell* cells;
	map(unsigned short width, unsigned short height);
	~map();
	inline cell* cell_at(int x, int y);
	void build_neighbour_cache();
};