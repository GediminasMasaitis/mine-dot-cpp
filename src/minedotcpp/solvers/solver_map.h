#pragma once
#include "../common/map.h"

namespace minedotcpp
{
	namespace solvers
	{
		class MINE_API solver_map : public minedotcpp::common::map
		{
		public:
			int filled_count;
			int flagged_count;
			int anti_flagged_count;
			int undecided_count;

			solver_map(const map& base_map);
		};
	}
}
	