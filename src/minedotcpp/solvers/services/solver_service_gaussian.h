#pragma once
#include "../solver_map.h"
#include "../solver_settings.h"
#include "solver_service_base.h"
#include "../matrix_reduction_parameters.h"
#include <vector>

namespace minedotcpp
{
	namespace solvers
	{
		namespace services
		{
			class solver_service_gaussian : private solver_service_base
			{
			public:
				explicit solver_service_gaussian(const solver_settings& settings, ctpl::thread_pool* thr_pool)
					: solver_service_base(settings, thr_pool)
				{
				}

				void solve_gaussian(solver_map& m, common::point_map<bool>& verdicts) const;
			private:

				class column_data
				{
				public:
					column_data(int index, int cnt) : index(index), cnt(cnt) {}
					int index;
					int cnt;
				};

				class row_separation_result
				{
				public:
					row_separation_result(int column_index, int constant) : column_index(column_index), constant(constant) {}
					int column_index;
					int constant;
				};

				void get_matrix_from_map(solver_map& m, std::vector<common::point>& points, bool all_undecided_coordinates_provided, std::vector<std::vector<int>>& matrix) const;
				void solve_gaussian_with_parameters(std::vector<std::vector<int>>& matrix, std::vector<common::point>& points, common::point_map<bool>& verdicts, const matrix_reduction_parameters& parameters) const;
				void prepare_matrix(std::vector<std::vector<int>>& matrix) const;
				void reduce_matrix(std::vector<std::vector<int>>& matrix, std::vector<common::point>& coordinates, const matrix_reduction_parameters& parameters) const;
				void separate_row(std::vector<int>& row, std::vector<row_separation_result>& results) const;
				bool find_results(std::vector<std::vector<int>>& matrix, std::vector<common::point>& coordinates, common::point_map<bool>& verdicts) const;
			};
		}
	}
}
