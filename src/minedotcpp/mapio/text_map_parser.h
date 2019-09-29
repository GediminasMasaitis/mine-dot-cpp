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
			void parse(char* str, common::map& target_map) const;
			void parse(const std::string& str, common::map& target_map) const;
			void parse(std::istream& input_stream, common::map& target_map) const;
		};
	}
}
