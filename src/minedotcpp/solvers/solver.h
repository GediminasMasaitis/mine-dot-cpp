#pragma once
#include "../mine_api.h"
#include "../common/point.h"
#include "../common/map.h"
#include "solver_result.h"
#include "solver_settings.h"
#include "services/solver_service_base.h"
#include "services/separation/solver_service_separation.h"


namespace minedotcpp
{
	namespace solvers
	{
		class solver_map;

		class MINE_API solver : private services::solver_service_base
		{
		public:
			// thread_count sizes the internal thread pool. 0 (default) = one
			// thread per hardware core — sensible for a standalone solver.
			// Benchmarks running N solvers in parallel should pass
			// hw_conc / N so the N pools together don't oversubscribe.
			explicit solver(solver_settings& settings, int thread_count = 0)
				: solver_service_base(settings, new ctpl::thread_pool(thread_count > 0 ? thread_count : std::thread::hardware_concurrency()))
				, separation_service(settings, thr_pool)
			{
			}

			~solver()
			{
				delete thr_pool;
			}

			void solve(const common::map& base_map, common::point_map<solver_result>& results) const;
		private:
			services::solver_service_separation separation_service;

			void get_final_results(solver_map& m, common::point_map<double>& probabilities, common::point_map<bool>& verdicts, common::point_map<solver_result>& results) const;
		};
	}
}
