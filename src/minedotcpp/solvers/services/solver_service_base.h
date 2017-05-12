#pragma once
#include "../solver_map.h"
#include "../solver_settings.h"
#include "../solver_result.h"

namespace minedotcpp
{
	namespace solvers
	{
		namespace services
		{
			class solver_service_base
			{
			public:
				const solver_settings settings;

				explicit solver_service_base(const solver_settings& settings) : settings(settings)
				{
				}

			protected:
				void get_verdicts_from_probabilities(common::point_map<double>& probabilities, common::point_map<bool>& target_verdicts) const;
				bool should_stop_solving(common::point_map<bool>& verdicts, bool stop_on_no_mine_verdict, bool stop_on_any_verdict, bool stop_always) const;
				
			};
		}
	}
}
