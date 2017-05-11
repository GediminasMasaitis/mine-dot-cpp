#pragma once

#include "../mine_api.h"
#include "cell.h"
#include "map_base.h"

namespace minedotcpp
{
	namespace common
	{
		class MINE_API map : public map_base<cell>
		{
		};
	}
}
