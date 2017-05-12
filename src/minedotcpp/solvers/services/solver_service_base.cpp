#include "solver_service_base.h"

using namespace minedotcpp;
using namespace solvers;
using namespace services;
using namespace common;
using namespace std;

void solver_service_base::get_verdicts_from_probabilities(point_map<double>& probabilities, point_map<bool>& target_verdicts) const
{
	for (auto& probability : probabilities)
	{
		bool verdict;
		if (fabs(probability.second) < 0.0000001)
		{
			verdict = false;
		}
		else if (fabs(probability.second - 1) < 0.0000001)
		{
			verdict = true;
		}
		else
		{
			continue;
		}
		target_verdicts[probability.first] = verdict;
	}
}

bool solver_service_base::should_stop_solving(point_map<bool>& verdicts, bool stop_on_no_mine_verdict, bool stop_on_any_verdict, bool stop_always) const
{
	if (stop_always)
	{
		return true;
	}
	if (verdicts.size() == 0)
	{
		return false;
	}
	if (stop_on_any_verdict)
	{
		return true;
	}
	if (stop_on_no_mine_verdict)
	{
		for (auto& verdict : verdicts)
		{
			if (!verdict.second)
			{
				return true;
			}
		}
	}
	return false;
}

void solver_service_base::get_final_results(point_map<double>& probabilities, point_map<bool>& verdicts, point_map<solver_result>& results) const
{
	for (auto& probability : probabilities)
	{
		auto& result = results[probability.first];
		result.pt = probability.first;
		result.probability = probability.second;
		result.verdict = verdict_none;
	}
	for (auto& verdict : verdicts)
	{
		auto& result = results[verdict.first];
		result.pt = verdict.first;
		result.probability = verdict.second ? 1 : 0;
		result.verdict = verdict.second ? verdict_has_mine : verdict_doesnt_have_mine;
	}
}