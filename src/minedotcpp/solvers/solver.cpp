#include "solver.h"
#include "solver_map.h"
#include <thread>
#include "services/solver_service_trivial.h"
#include "services/solver_service_separation.h"
#include "services/solver_service_guessing.h"

using namespace minedotcpp;
using namespace solvers;
using namespace services;
using namespace common;
using namespace std;

void solver::solve(const map& base_map, point_map<solver_result>& results) const
{
	solver_map m;
	m.init_from(base_map);
	if(settings.mine_count_ignore_completely)
	{
		m.remaining_mine_count = -1;
	}

	point_map<double> all_probabilities;
	point_map<bool> all_verdicts;


	if(settings.trivial_solve)
	{
		auto trivial_service = solver_service_trivial(settings);
		trivial_service.solve_trivial(m, all_verdicts);
		if(should_stop_solving(all_verdicts, settings.trivial_stop_on_no_mine_verdict, settings.trivial_stop_on_any_verdict, settings.trivial_stop_always))
		{
			get_final_results(m, all_probabilities, all_verdicts, results);
			return;
		}
	}

	// TODO: Gaussian solving

	if (settings.separation_solve)
	{
		auto separation_service = solver_service_separation(settings);
		separation_service.solve_separation(m, all_probabilities, all_verdicts);
	}

	get_final_results(m, all_probabilities, all_verdicts, results);
}

void solver::get_final_results(solver_map& m, point_map<double>& probabilities, point_map<bool>& verdicts, point_map<solver_result>& results) const
{
	auto guessing_service = solver_service_guessing(settings);
	point guessed_pt;
	auto guessed = guessing_service.guess_verdict(m, probabilities, verdicts, guessed_pt);
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
		if (!guessed || guessed_pt != result.pt)
		{
			result.probability = verdict.second ? 1 : 0;
		}
		result.verdict = verdict.second ? verdict_has_mine : verdict_doesnt_have_mine;
	}
}