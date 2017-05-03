#pragma once

#include <unordered_map>

#include "../common/map.h"
#include "../common/neighbour_cache_entry.h"

namespace minedotcpp
{
	namespace solvers
	{
		class MINE_API solver_map : public common::map
		{
		public:
			int filled_count;
			int flagged_count;
			int anti_flagged_count;
			int undecided_count;
			std::vector<common::neighbour_cache_entry> neighbour_cache;

			solver_map(const map& base_map);
			common::neighbour_cache_entry& neighbour_cache_get(int x, int y);
			common::neighbour_cache_entry& neighbour_cache_get(common::point pt);

			void set_cells_by_verdicts(std::unordered_map<common::point, bool, common::point_hash> verdicts);
		private:
			void calculate_neighbours_of(common::point pt, std::vector<common::cell>& cells, bool include_self);
			void build_additional_neighbour_lists(common::neighbour_cache_entry& cache_entry);
			void build_neighbour_cache();
			void update_neighbour_cache(common::point p);
		};
	}
}
	