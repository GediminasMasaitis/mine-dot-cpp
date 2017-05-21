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
	printf("%i; %i\n", settings.give_up_from_size, settings.valid_combination_search_open_cl_use_from_size);
}