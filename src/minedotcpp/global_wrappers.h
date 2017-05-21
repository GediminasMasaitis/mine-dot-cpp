#pragma once
#include "mine_api.h"
#include "solvers/solver_settings.h"

//int MINE_API solve(const common::map& base_map, common::point_map<solver_result>& results) const;

extern "C" void MINE_API init_solver(minedotcpp::solvers::solver_settings settings);
