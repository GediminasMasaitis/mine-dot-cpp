#pragma once
#include "../common/cell.h"

class game_cell : public minedotcpp::common::cell
{
public:
	bool has_mine;
};
