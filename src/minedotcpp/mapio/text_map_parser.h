#pragma once
#include "../mine_api.h"
#include "../common/map.h"

namespace minedotcpp
{
	namespace mapio
	{
		class MINE_API text_map_parser
		{
		public:
			void parse(std::string str, common::map& target_map) const;
			void parse(std::istream& is, common::map& target_map) const;
		};
	}
}
