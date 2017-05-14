#pragma once
#include "../solver_map.h"
#include "../solver_settings.h"
#include "../border.h"
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
			class solver_service_separation_combination_finding : private solver_service_base
			{
			public:
				explicit solver_service_separation_combination_finding(const solver_settings& settings) : solver_service_base(settings)
				{
				}

				void find_valid_border_cell_combinations(solver_map& map, border& border) const;

			private:
				bool is_prediction_valid(const solver_map& map, const border& b, unsigned prediction, const std::vector<common::cell>& empty_cells, const CELL_INDICES_T& cell_indices) const;
				int SWAR(int i) const;
				void thr_find_combos(const solver_map& map, border& border, unsigned min, unsigned max, const std::vector<common::cell>& empty_cells, const CELL_INDICES_T& cell_indices, std::mutex& sync) const;
			};
		}
	}
}
