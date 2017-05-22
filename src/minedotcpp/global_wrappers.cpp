#include "global_wrappers.h"
#include <cstdio>
#include "solvers/solver.h"
#include "mapio/text_map_parser.h"

using namespace minedotcpp;
using namespace common;
using namespace solvers;
using namespace mapio;
using namespace std;

static text_map_parser parser;
static solver_settings g_settings;
static solver* g_solver;

extern "C" void MINE_API init_solver(solver_settings settings)
{
	if(g_solver != nullptr)
	{
		delete g_solver;
	}
	g_settings = settings;
	g_solver = new solver(g_settings);
	printf("%i; %i; %i\n", g_settings.give_up_from_size, g_settings.valid_combination_search_open_cl_use_from_size, g_settings.separation_solve);
}

extern "C" int MINE_API solve(const char* map_str, solver_result* results_buffer, int* buffer_size)
{
	//puts(map_str);
	auto m = map();
	parser.parse(map_str, m);
	auto results = point_map<solver_result>();
	g_solver->solve(m, results);
	if(results.size() > *buffer_size)
	{
		return -1;
	}
	*buffer_size = results.size();
	int i = 0;
	for(auto res : results)
	{
		results_buffer[i++] = res.second;
	}
	return 1;
}