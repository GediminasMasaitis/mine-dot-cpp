#pragma once
#include "../../solver_map.h"
#include "../../solver_settings.h"
#include "../../border.h"
#include "../solver_service_base.h"
#include <mutex>
#include <array>
#ifdef ENABLE_OPEN_CL
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.hpp>
#endif

namespace minedotcpp
{
	namespace solvers
	{
		struct border_reduction_result
		{
			bool valid = false;
			int rank = 0;
			int free_count = 0;
			std::vector<int> pivot_columns;
			std::vector<int> free_variable_indices;
			std::vector<std::vector<int>> matrix;

			// Precomputed for backtracking: which dependent vars to check at each depth
			// check_at_depth[d] = list of pivot row indices whose dependent var
			// becomes fully determined after free variable d is assigned
			std::vector<std::vector<int>> check_at_depth;
		};

		namespace services
		{
			using ClResultArr = std::array<unsigned int, 1024 * 1024 * 64>;

			class solver_service_separation_combination_finding : private solver_service_base
			{
			public:
				explicit solver_service_separation_combination_finding(const solver_settings& settings, ctpl::thread_pool* thr_pool)
					: solver_service_base(settings, thr_pool)
				{
#ifdef ENABLE_OPEN_CL
					cl_build_find_combination_program();
#endif
				}

				void find_valid_border_cell_combinations(solver_map& map, border& border, const border_reduction_result& reduction) const;
				border_reduction_result reduce_border(solver_map& map, border& border) const;
			private:

#ifdef ENABLE_OPEN_CL
				cl::Context cl_context;
				cl::Program cl_find_combination_program;

				
				
				void cl_build_find_combination_program();
				void cl_validate_predictions(unsigned char map_size, std::vector<unsigned char>& map, ClResultArr& results, int& result_count, unsigned total) const;
#endif
				int find_hamming_weight(int i) const;
				void get_combination_search_map(solver_map& solver_map, border& border, std::vector<unsigned char>& m, unsigned char& map_size) const;
				void thr_validate_predictions(unsigned char map_size, std::vector<unsigned char>& m, std::vector<unsigned>& results, unsigned total) const;
				void thr_pool_validate_predictions(unsigned char map_size, std::vector<unsigned char>& m, std::vector<unsigned>& results, unsigned total) const;
				void validate_predictions(const unsigned char map_size, std::vector<unsigned char>& m, std::vector<unsigned>& results, const unsigned min, const unsigned max, std::mutex* sync) const;

				void build_border_constraint_matrix(solver_map& map, border& border, std::vector<std::vector<int>>& matrix) const;
				void compute_rref(std::vector<std::vector<int>>& matrix, int num_vars, border_reduction_result& result) const;
				void validate_predictions_reduced(const border_reduction_result& reduction, int border_length, std::vector<unsigned int>& results, unsigned int min_free, unsigned int max_free, std::mutex* sync) const;
				void thr_pool_validate_predictions_reduced(const border_reduction_result& reduction, int border_length, std::vector<unsigned int>& results, unsigned int total_free) const;

				void precompute_backtracking_depths(border_reduction_result& reduction) const;
				void validate_predictions_backtracking(const border_reduction_result& reduction, int border_length, std::vector<unsigned int>& results) const;
				void backtrack(const border_reduction_result& reduction, int border_length, int depth, unsigned int free_val, unsigned int prediction, std::vector<unsigned int>& results) const;
			};
		}
	}
}
