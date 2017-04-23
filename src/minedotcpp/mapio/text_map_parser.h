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
			common::map* parse(std::string str) const;
			common::map* parse(std::istream* is) const;
		};
	}
}
