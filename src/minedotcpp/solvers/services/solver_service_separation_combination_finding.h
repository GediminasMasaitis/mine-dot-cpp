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
				explicit solver_service_separation_combination_finding(const solver_settings& settings)
					: solver_service_base(settings)
#ifdef ENABLE_OPEN_CL
					, cl_find_combination_program(cl_build_find_combination_program())
#endif
				{
				}
				
				void find_valid_border_cell_combinations(solver_map& map, border& border) const;
			private:

#ifdef ENABLE_OPEN_CL
				const cl::Program cl_find_combination_program;
				cl::Program cl_build_find_combination_program();
				void cl_validate_predictions(std::vector<unsigned char>& map, std::vector<int>& results, unsigned max_prediction) const;
#endif

				int SWAR(int i) const;
				void get_combination_search_map(solver_map& original_map, border& border, std::vector<unsigned char>& m) const;
				void thr_validate_predictions(std::vector<unsigned char>& map, std::vector<int>& results, unsigned total) const;
				void validate_predictions(std::vector<unsigned char>& map, std::vector<int>& results, unsigned min, unsigned max, std::mutex* sync) const;
			};
		}
	}
}
