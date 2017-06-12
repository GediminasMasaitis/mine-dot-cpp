#pragma once
#include "../common/point.h"

namespace minedotcpp
{
	class combination
	{
	public:
		combination(int mine_count, const common::point_map<bool>& pts)
			: mine_count(mine_count),
			  pts(pts)
		{
		}

		int mine_count;
		common::point_map<bool> pts;
	};
}
