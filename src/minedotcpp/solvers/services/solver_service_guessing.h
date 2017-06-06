#pragma once
#include "../solver_map.h"
#include "../solver_settings.h"
#include "solver_service_base.h"

namespace minedotcpp
{
	namespace solvers
	{
		namespace services
		{
			class solver_service_guessing : private solver_service_base
			{
			public:
				explicit solver_service_guessing(const solver_settings& settings, ctpl::thread_pool* thr_pool)
					: solver_service_base(settings, thr_pool)
				{
				}

				bool guess_verdict(solver_map& m, common::point_map<double>& probabilities, common::point_map<bool>& verdicts, common::point& guessed_point) const;
			private:
				bool should_guess(solver_map& m, common::point_map<bool>& verdicts) const;
			};
		}
	}
}
