#pragma once
#include "point.h"
#include "cell_param.h"

namespace minedotcpp
{
	namespace common
	{
		class cell
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
