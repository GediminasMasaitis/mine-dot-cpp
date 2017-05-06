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
			std::string visualize_to_string(common::map& m) const;
			void visualize(common::map& m, std::ostream& os) const;
		};

		
	}
}
