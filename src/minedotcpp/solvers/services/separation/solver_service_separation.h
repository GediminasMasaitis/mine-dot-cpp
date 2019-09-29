#pragma once
#include "../../../common/map.h"
#include "../../solver_map.h"
#include "../../solver_settings.h"
#include "../../border.h"
#include "../../partial_border_data.h"
#include "../solver_service_base.h"
#include <mutex>
#include "solver_service_separation_combination_finding.h"


namespace minedotcpp
{
	namespace solvers
	{
		namespace services
		{
			class solver_service_separation : private solver_service_base
			{
			public:
				explicit solver_service_separation(const solver_settings& settings, ctpl::thread_pool* thr_pool)
					: solver_service_base(settings, thr_pool)
					, combination_service(settings, thr_pool)
				{
				}

				void get_pattern(solver_map& m) const;
				void solve_separation(solver_map& m, common::point_map<double>& probabilities, common::point_map<bool>& verdicts) const;

			private:
				solver_service_separation_combination_finding combination_service;
				void solve_border(solver_map& m, border& b, bool allow_partial_border_solving, std::vector<border>& borders) const;
				void calculate_min_max_mine_counts(border& b) const;
				void try_solve_border_by_partial_borders(solver_map& map, border& border) const;
				void calculate_partial_map_and_trim_partial_border(border& partial_border, solver_map& partial_map, solver_map& parent_map, common::point_set& all_flagged_coordinates) const;
				void get_partial_border(border& border, solver_map& map, common::point pt, partial_border_data& border_data) const;
				void reseparate_border(solver_map& m, border& parent_border, std::vector<border>& borders, bool solve) const;
				
				void calculate_border_probabilities(border& b) const;
				void breadth_search_border(solver_map& m, common::point_set& allowed_coordinates, common::point target_coordinate, std::vector<common::cell>& target_cells, const bool smooth_order) const;
				int separate_borders(solver_map& m, border& common_border, std::vector<border>& target_borders) const;
				bool is_cell_border(solver_map& m, common::cell& c) const;
				void find_common_border(solver_map& m, border& common_border) const;
			};
		}
	}
}
