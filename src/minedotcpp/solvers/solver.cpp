#include "solver.h"
#include "solver_map.h"
#include <thread>
#include <mutex>
#include "services/solver_service_trivial.h"
#include "services/solver_service_separation.h"

using namespace minedotcpp;
using namespace solvers;
using namespace services;
using namespace common;
using namespace std;

void solver::solve(const map& base_map, point_map<solver_result>& results) const
{
	auto trivial_service = solver_service_trivial(this->settings);
	auto separation_service = solver_service_separation(this->settings);
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
		trivial_service.solve_trivial(m, all_verdicts);
		if(should_stop_solving(all_verdicts, settings.trivial_stop_on_no_mine_verdict, settings.trivial_stop_on_any_verdict, settings.trivial_stop_always))
		{
			get_final_results(all_probabilities, all_verdicts, results);
			return;
		}
	}

	// TODO: Gaussian solving

	if (settings.separation_solve)
	{
		separation_service.solve_separation(m, all_probabilities, all_verdicts);
	}

	get_final_results(all_probabilities, all_verdicts, results);
}