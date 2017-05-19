#pragma once
#include "../solver_map.h"
#include "../solver_settings.h"
#include "../border.h"
#include "solver_service_base.h"
#include <mutex>
#ifdef ENABLE_OPEN_CL
#include <CL/cl.hpp>
#endif

namespace minedotcpp
{
	namespace solvers
	{
		namespace services
		{
			class solver_service_separation_combination_finding : private solver_service_base
			{
			public:
				
				void find_valid_border_cell_combinations(solver_map& map, border& border) const;
			private:

#ifdef ENABLE_OPEN_CL
				cl::Context cl_context;
				cl::Program cl_find_combination_program;

				void cl_build_find_combination_program();
				void cl_validate_predictions(unsigned char map_size, std::vector<unsigned char>& map, std::vector<int>& results, unsigned total) const;
#endif

				int find_hamming_weight(int i) const;
				void get_combination_search_map(solver_map& solver_map, border& border, std::vector<unsigned char>& m, unsigned char& map_size) const;
				void thr_validate_predictions(unsigned char map_size, std::vector<unsigned char>& m, std::vector<int>& results, unsigned total) const;
				void validate_predictions(unsigned char map_size, std::vector<unsigned char>& m, std::vector<int>& results, unsigned min, unsigned max, std::mutex* sync) const;

			public:
				explicit solver_service_separation_combination_finding(const solver_settings& settings)
					: solver_service_base(settings)
				{
#ifdef ENABLE_OPEN_CL
					cl_build_find_combination_program();
#endif
				}
			};
		}
	}
}
