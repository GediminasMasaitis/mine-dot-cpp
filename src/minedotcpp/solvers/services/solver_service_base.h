#pragma once
#include "../solver_map.h"
#include "../solver_settings.h"
#include "../solver_result.h"
#include <ctpl_stl.h>

namespace minedotcpp
{
	namespace solvers
	{
		namespace services
		{
			class solver_service_base
			{
			public:
				solver_settings settings;

				explicit solver_service_base(const solver_settings& settings, ctpl::thread_pool* thr_pool)
					: settings(settings)
					, thr_pool(thr_pool)
				{
				}

			protected:
				ctpl::thread_pool* thr_pool;

				void get_verdicts_from_probabilities(common::point_map<double>& probabilities, common::point_map<bool>& target_verdicts) const;
				bool should_stop_solving(common::point_map<bool>& verdicts, bool stop_on_no_mine_verdict, bool stop_on_any_verdict, bool stop_always) const;
				
			};
		}
	}
}
