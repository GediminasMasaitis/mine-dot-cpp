#pragma once
#include "../mine_api.h"
#include "../common/map.h"

namespace minedotcpp
{
	namespace mapio
	{
		class MINE_API text_map_visualizer
		{
		public:
			std::string visualize_to_string(common::map* map) const;
			void visualize(common::map* map, std::ostream& os) const;
		};

		
	}
}
