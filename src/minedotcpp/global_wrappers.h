#pragma once
#include "mine_api.h"
#include "solvers/solver_settings.h"
#include "solvers/solver_result.h"

//int MINE_API solve(const common::map& base_map, common::point_map<solver_result>& results) const;

extern "C" void MINE_API init_solver(minedotcpp::solvers::solver_settings settings);
extern "C" int MINE_API solve(const char* map_str, minedotcpp::solvers::solver_result* results_buffer, int* buffer_size);