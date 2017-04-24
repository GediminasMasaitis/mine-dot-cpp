#include "solver.h"
#include "solver_map.h"

using namespace minedotcpp::solvers;
using namespace minedotcpp::common;

solver::solver(solver_settings settings)
{
	this->settings = settings;
}

std::unordered_map<point, solver_result>* solver::solve(const map& base_map) const
{
	solver_map m(base_map);
	if(settings.ignore_mine_count_completely)
	{
		m.remaining_mine_count = -1;
	}
	return nullptr;
}

void solver::set_cells_by_verdicts(solver_map& map, std::unordered_map<point, bool>& verdicts)
{

}
