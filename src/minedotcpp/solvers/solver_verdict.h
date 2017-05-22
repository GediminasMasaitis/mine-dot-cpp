#pragma once

namespace minedotcpp
{
	namespace solvers
	{
		enum solver_verdict
		{
			verdict_none = 0,
			verdict_has_mine = 1,
			verdict_doesnt_have_mine = 2
		};
	}
}
