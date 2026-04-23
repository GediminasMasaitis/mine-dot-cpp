#pragma once
#include "mine_api.h"
#include "solvers/solver_settings.h"
#include "solvers/solver_result.h"

// Opaque handle to an independent solver instance. Each handle owns its own
// solver, thread pool, OpenCL context and scratch buffers, so handles can be
// used concurrently from different threads without cross-contamination. This
// is what lets the C# DirectSolver path run in parallel (one handle per
// benchmark worker).
typedef void* solver_handle;

extern "C" solver_handle MINE_API create_solver(minedotcpp::solvers::solver_settings settings);
extern "C" void MINE_API destroy_solver(solver_handle handle);
extern "C" int MINE_API solve(solver_handle handle, const char* map_str, minedotcpp::solvers::solver_result* results_buffer, int* buffer_size);
