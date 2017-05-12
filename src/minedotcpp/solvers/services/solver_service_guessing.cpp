#include "../../common/point.h"
#include "../solver_map.h"
#include "solver_service_guessing.h"

using namespace minedotcpp;
using namespace solvers;
using namespace services;
using namespace common;
using namespace std;

bool solver_service_guessing::guess_verdict(solver_map& m, point_map<double>& probabilities, point_map<bool>& verdicts, point& guessed_point) const
{
	if(!should_guess(m,verdicts))
	{
		return false;
	}
	if(probabilities.size() == 0)
	{
		for(auto& c : m.cells)
		{
			if(c.state == cell_state_filled)
			{
				verdicts[c.pt] = false;
				guessed_point = c.pt;
				return true;
			}
		}
		return false;
	}
	auto least_risky_ptr = probabilities.begin();
	auto least_risky_pt = least_risky_ptr->first;
	auto least_risky_probability = least_risky_ptr->second;
	for(auto& probability : probabilities)
	{
		if (probability.second < least_risky_probability)
		{
			least_risky_pt = probability.first;
			least_risky_probability = probability.second;
		}
	}
	verdicts[least_risky_pt] = true;
	guessed_point = least_risky_pt;
	return true;
}

bool solver_service_guessing::should_guess(solver_map& m, point_map<bool>& verdicts) const
{
	if (verdicts.size() == 0 && settings.guess_if_no_verdict)
	{
		return true;
	}
	if (settings.guess_if_no_no_mine_verdict)
	{
		for (auto& verdict : verdicts)
		{
			if (!verdict.second)
			{
				return false;
			}
		}
	}
	return settings.guess_if_no_no_mine_verdict;
}