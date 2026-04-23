#include "global_wrappers.h"
#include <cstdio>
#include "solvers/solver.h"
#include "mapio/text_map_parser.h"

using namespace minedotcpp;
using namespace common;
using namespace solvers;
using namespace mapio;
using namespace std;

// Per-handle state. The settings field must outlive the solver because solver
// stores it by reference (see solver.h); keep it alongside so both die
// together when the handle is destroyed.
struct solver_handle_impl
{
	solver_settings settings;
	solver* svc;
	text_map_parser parser;

	explicit solver_handle_impl(const solver_settings& s)
		: settings(s), svc(new solver(settings))
	{
	}

	~solver_handle_impl()
	{
		delete svc;
	}
};

extern "C" solver_handle MINE_API create_solver(solver_settings settings)
{
	return new solver_handle_impl(settings);
}

extern "C" void MINE_API destroy_solver(solver_handle handle)
{
	if (handle == nullptr) return;
	delete static_cast<solver_handle_impl*>(handle);
}

extern "C" int MINE_API solve(solver_handle handle, const char* map_str, solver_result* results_buffer, int* buffer_size)
{
	auto* h = static_cast<solver_handle_impl*>(handle);
	auto m = map();
	h->parser.parse(map_str, m);
	auto results = point_map<solver_result>();
	h->svc->solve(m, results);
	if (static_cast<int>(results.size()) > *buffer_size)
	{
		return -1;
	}
	*buffer_size = static_cast<int>(results.size());
	int i = 0;
	for (auto res : results)
	{
		results_buffer[i++] = res.second;
	}
	return 1;
}
