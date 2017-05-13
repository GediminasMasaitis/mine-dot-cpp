#include "solver_service_gaussian.h"
#include "../../debug/debugging.h"
#include "../matrix_reduction_parameters.h"

using namespace minedotcpp;
using namespace common;
using namespace solvers;
using namespace services;
using namespace std;

void solver_service_gaussian::solve_gaussian(solver_map& m, point_map<bool>& verdicts) const
{
	auto parameters = vector<matrix_reduction_parameters>
	{
		matrix_reduction_parameters(true, false, false,false, false),
		matrix_reduction_parameters(false, true, false, true, true),
		matrix_reduction_parameters(false, true, true, true, true),
		matrix_reduction_parameters(false, false, false, true, true),
		matrix_reduction_parameters(false, false, true, true, true),
		matrix_reduction_parameters(false, false, true, false, true),
		matrix_reduction_parameters(false, true, false, true, false),
		matrix_reduction_parameters(false, true, true, true, false)
	};
	auto points = vector<point>();
	for(auto&c : m.cells)
	{
		if(c.state == cell_state_filled)
		{
			points.push_back(c.pt);
		}
	}
	auto matrix = vector<vector<int>>();
	get_matrix_from_map(m, points, true, matrix);
	dump_gaussian_matrix(matrix);
	for(auto& p : parameters)
	{
		auto local_matrix = matrix;
		auto local_points = points;
		auto results = point_map<bool>();
		reduce_matrix(local_matrix, local_points, results, p);
	}
}

void solver_service_gaussian::get_matrix_from_map(solver_map& m, vector<point>& points, bool all_undecided_coordinates_provided, vector<vector<int>>& matrix) const
{
	auto hint_points = point_set();
	auto indices = point_map<int>();
	for(auto i = 0; i < points.size(); i++)
	{
		auto& pt = points[i];
		auto& entry = m.neighbour_cache_get(pt);
		auto& empty = entry.by_state[cell_state_empty];
		for(auto& c : empty)
		{
			hint_points.insert(c.pt);
		}
		indices[pt] = i;
	}

	auto hint_cells = vector<cell>();
	hint_cells.reserve(hint_points.size());
	for(auto& pt : hint_points)
	{
		hint_cells.push_back(m.cell_get(pt));
	}

	for(auto i = 0; i < hint_cells.size(); i++)
	{
		auto& hint_cell = hint_cells[i];
		auto& entry = m.neighbour_cache_get(hint_cell.pt);
		auto remaining_hint = hint_cell.hint - entry.by_flag[cell_flag_has_mine].size();

		auto row = vector<int>(points.size() + 1);
		auto& undecided_neighbours = m.neighbour_cache_get(hint_cell.pt).by_param[cell_state_filled | cell_flag_none];
		for(auto& undecided_neighbour : undecided_neighbours)
		{
			auto& index = indices[undecided_neighbour.pt];
			row[index] = 1;
		}
		row[row.size() - 1] = remaining_hint;
		matrix.push_back(row);
	}
	if(all_undecided_coordinates_provided && m.remaining_mine_count != -1)
	{
		auto row = vector<int>(points.size() + 1);
		for(auto i = 0; i < points.size(); i++)
		{
			row[i] = 1;
		}
		row[row.size() - 1] = m.remaining_mine_count;
		matrix.push_back(row);
	}
}

void solver_service_gaussian::reduce_matrix(vector<vector<int>>& matrix, vector<point>& coordinates, point_map<bool>& allVerdicts, const matrix_reduction_parameters& parameters) const
{
	
}
