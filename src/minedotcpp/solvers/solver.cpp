#include "solver.h"
#include "solver_map.h"

using namespace minedotcpp::solvers;
using namespace minedotcpp::common;

solver::solver(solver_settings& settings)
{
	this->settings = settings;
}

point_map<solver_result>* solver::solve(const map& base_map) const
{
	solver_map m(base_map);
	if(settings.ignore_mine_count_completely)
	{
		m.remaining_mine_count = -1;
	}

	point_map<double> all_probabilities;
	point_map<bool> all_verdicts;
	

	if(settings.solve_trivial)
	{
		solve_trivial(m, all_verdicts);
		if (settings.stop_after_trivial_solving || should_stop_solving(all_verdicts))
		{
			return get_final_results(all_probabilities, all_verdicts);
		}
	}

	return get_final_results(all_probabilities, all_verdicts);
}

void solver::solve_trivial(solver_map& m, point_map<bool>& verdicts) const
{
	while (true)
	{
		point_map<bool> currentRoundVerdicts;

		for (auto& cell : m.cells)
		{
			if (cell.state != cell_state_empty)
			{
				continue;
			}
			auto& neighbourEntry = m.neighbour_cache_get(cell.pt);
			auto& filledNeighbours = neighbourEntry.by_state[cell_state_filled];
			auto& flaggedNeighbours = neighbourEntry.by_flag[cell_flag_has_mine];
			auto& antiflaggedNeighbours = neighbourEntry.by_flag[cell_flag_doesnt_have_mine];
			if (filledNeighbours.size() == flaggedNeighbours.size() + antiflaggedNeighbours.size())
			{
				continue;
			}

			auto flagging = filledNeighbours.size() == cell.hint;
			auto clicking = flaggedNeighbours.size() == cell.hint;
			if (clicking || flagging)
			{
				for(auto& neighbour : filledNeighbours)
				{
					if(neighbour.state == (cell_state_filled | cell_flag_has_mine))
					{
						continue;
					}
					if(verdicts.find(neighbour.pt) != verdicts.end())
					{
						continue;
					}
					currentRoundVerdicts[neighbour.pt] = flagging;
					verdicts[neighbour.pt] = flagging;
				}
			}
		}

		if (currentRoundVerdicts.size() == 0)
		{
			return;
		}
		m.set_cells_by_verdicts(currentRoundVerdicts);
		
	}
}

bool solver::should_stop_solving(point_map<bool>& verdicts) const
{
	if(verdicts.size() == 0)
	{
		return false;
	}
	if (settings.stop_on_any_verdict)
	{
		return true;
	}
	if (settings.stop_on_no_mine_verdict)
	{
		for(auto& verdict : verdicts)
		{
			if(!verdict.second)
			{
				return true;
			}
		}
	}
	return false;
}

point_map<solver_result>* solver::get_final_results(point_map<double>& probabilities, point_map<bool>& verdicts) const
{
	auto results_ptr = new point_map<solver_result>();
	auto& results = *results_ptr;
	for(auto& probability : probabilities)
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
	return results_ptr;
}
