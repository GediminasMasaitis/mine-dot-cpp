#pragma once
#include "border.h"
#include "solver_map.h"

class partial_border_data
{
public:
	minedotcpp::common::point_set partial_border_points;
	minedotcpp::solvers::solver_map partial_map;
	minedotcpp::solvers::border partial_border;
};
