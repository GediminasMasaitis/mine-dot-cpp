#pragma once
#include "../../common/map.h"
#include "../solver_map.h"
#include "../solver_settings.h"
#include "../border.h"
#include "../partial_border_data.h"
#include "solver_service_base.h"
#include <mutex>


// TODO: For testing purposes, an easy way to switch between the two. Should be removed in final version
//#define CELL_INDICES_MAP
#ifdef CELL_INDICES_MAP
typedef minedotcpp::common::point_map<int> CELL_INDICES_T;
#define CELL_INDICES_ELEMENT(ci, pt, m) ci[pt]
#define CELL_INDICES_RESIZE(ci, b, m) ci.resize(b.cells.size())
#else
typedef std::vector<int> CELL_INDICES_T;
#define CELL_INDICES_ELEMENT(ci, pt, m) ci[pt.x * m.height + pt.y]
#define CELL_INDICES_RESIZE(ci, b, m) ci.resize(m.width * m.height)
#endif

namespace minedotcpp
{
	namespace solvers
	{
		namespace services
		{
			class solver_service_separation : private solver_service_base
			{
			public:
				explicit solver_service_separation(const solver_settings& settings)	: solver_service_base(settings)
				{
				}

				void solve_separation(solver_map& m, common::point_map<double>& probabilities, common::point_map<bool>& verdicts) const;

			private:
				void solve_border(solver_map& m, border& b, bool allow_partial_border_solving, std::vector<border>& borders) const;
				void try_solve_border_by_partial_borders(solver_map& map, border& border) const;
				void get_partial_border(border& border, solver_map& map, common::point pt, partial_border_data& border_data) const;
				void reseparate_border(solver_map& m, border& parent_border, std::vector<border>& borders, bool solve) const;
				void find_valid_border_cell_combinations(solver_map& map, border& border) const;
				bool is_prediction_valid(const solver_map& map, const border& b, unsigned prediction, const std::vector<common::cell>& empty_cells, const CELL_INDICES_T& cell_indices) const;
				void calculate_min_max_mine_counts(border& b) const;
				int SWAR(int i) const;
				void thr_find_combos(const solver_map& map, border& border, unsigned min, unsigned max, const std::vector<common::cell>& empty_cells, const CELL_INDICES_T& cell_indices, std::mutex& sync) const;
				void calculate_border_probabilities(border& b) const;
				void breadth_search_border(solver_map& m, common::point_set& allowed_coordinates, common::point target_coordinate, std::vector<common::cell>& target_cells) const;
				int separate_borders(solver_map& m, border& common_border, std::vector<border>& target_borders) const;
				bool is_cell_border(solver_map& m, common::cell& c) const;
				void find_common_border(solver_map& m, border& common_border) const;
			};
		}
	}
}
