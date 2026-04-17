#pragma once
#include <cstdint>
#include "../common/point.h"

namespace minedotcpp
{
	class combination
	{
	public:
		combination(int mine_count, std::uint64_t bitmask)
			: mine_count(mine_count),
			  bitmask(bitmask)
		{
		}

		int mine_count;
		// Bit j is set iff the cell at border.cells[j] has a mine in this combination.
		// Interpreted relative to the enclosing border's cells vector at the time
		// this combination was produced. Verdict processing keeps this invariant by
		// updating the bitmask whenever b.cells is swap-popped.
		std::uint64_t bitmask;
	};
}
