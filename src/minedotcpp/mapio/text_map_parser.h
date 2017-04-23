#pragma once
#include "../common/map.h"

namespace minedotcpp
{
	namespace mapio
	{
		class text_map_parser
		{
		public:
			common::map* parse(std::string str) const;
			common::map* parse(std::istream* is) const;
		};
	}
}
