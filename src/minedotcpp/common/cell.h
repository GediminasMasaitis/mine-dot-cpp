#pragma once

#include "../mine_api.h"
#include "point.h"
#include "cell_param.h"

namespace minedotcpp
{
	namespace common
	{
		class MINE_API cell
		{
		public:
			point pt;
			cell_param state;
			int hint;

			cell();
			cell(point pt, cell_param param, int hint);
		};
	}
}
