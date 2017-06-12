#pragma once
#include "../common/cell.h"
#include "combination.h"

namespace minedotcpp
{
	namespace solvers
	{
		class border
		{
		public:
			std::vector<common::cell> cells;
			std::vector<combination> valid_combinations;
			int min_mine_count;
			int max_mine_count;
			common::point_map<double> probabilities;
			common::point_map<bool> verdicts;
			bool solved_fully;
		};
	}
}
