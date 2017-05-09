#pragma once
#include "../mine_api.h"
#include "solver_settings.h"
#include "../common/point.h"
#include "solver_result.h"
#include "../common/map.h"
#include "border.h"
#include "partial_border_data.h"

#include <mutex>


// TODO: For testing purposes, an easy way to switch between the two. Should be removed in final version
//#define CELL_INDICES_MAP
#ifdef CELL_INDICES_MAP
typedef minedotcpp::common::point_map<int> CELL_INDICES_T;
#define CELL_INDICES_ELEMENT(ci, pt, m) ci[pt]
#define CELL_INDICES_RESIZE(ci, b, m) ci.resize(b.cells.size())
#else
typedef std::vector<int> CELL_INDICES_T;
#define CELL_INDICES_ELEMENT(ci, pt, m) ci[pt.x * m.width + pt.y]
#define CELL_INDICES_RESIZE(ci, b, m) ci.resize(m.width * m.height)
#endif

namespace minedotcpp
{
	namespace solvers
	{
		class solver_map;

		class MINE_API solver
		{
		public:
			solver_settings settings;

			explicit solver(solver_settings& settings);
			void solve(const common::map& base_map, common::point_map<solver_result>& results) const;

		private:

			void solve_trivial(solver_map& m, common::point_map<bool>& verdicts) const;
			void solve_separation(solver_map& m, common::point_map<double>& probabilities, common::point_map<bool>& verdicts) const;

			void solve_border(solver_map& m, border& b, bool allow_partial_border_solving, std::vector<border>& borders) const;
			void try_solve_border_by_partial_borders(solver_map& map, border& border) const;
			void get_partial_border(border& border, solver_map& map, common::point pt, partial_border_data& border_data) const;
			void reseparate_border(solver_map& m, border& parent_border, std::vector<border>& borders, bool solve) const;
			void find_valid_border_cell_combinations(solver_map& map, border& border) const;
			bool is_prediction_valid(const solver_map& map, const border& b, unsigned prediction, const std::vector<common::cell>& empty_cells, const CELL_INDICES_T& cell_indices) const;
			int SWAR(int i) const;
			void thr_find_combos(const solver_map& map, border& border, unsigned min, unsigned max, const std::vector<common::cell>& empty_cells, const CELL_INDICES_T& cell_indices, std::mutex& sync) const;
			void calculate_border_probabilities(border& b) const;
			void get_verdicts_from_probabilities(common::point_map<double>& probabilities, common::point_map<bool>& target_verdicts) const;
			void breadth_search_border(solver_map& m, common::point_set& allowed_coordinates, common::point target_coordinate, std::vector<common::cell>& target_cells) const;
			int separate_borders(solver_map& m, border& common_border, std::vector<border>& target_borders) const;
			bool is_cell_border(solver_map& m, common::cell& c) const;
			void find_common_border(solver_map& m, border& common_border) const;
			void solve_mine_counts(solver_map& m, border& common_border, std::vector<border>& borders, common::point_map<double>& all_probabilities, common::point_map<bool>& all_verdicts) const;
			void trim_valid_combinations_by_mine_count(border& b, int minesRemaining, int undecidedCellsRemaining, int minesElsewhere, int nonMineCountElsewhere) const;
			bool is_prediction_valid_by_mine_count(int minePredictionCount, int totalCombinationLength, int minesRemaining, int undecidedCellsRemaining, int minesElsewhere, int nonMineCountElsewhere) const;
			void get_variable_mine_count_borders_probabilities(std::vector<border>& variable_borders, int minesRemaining, int undecided_cells_remaining, int non_border_cell_count, int minesElsewhere, int non_mine_count_elsewhere, common::point_map<double>& targetProbabilities, google::dense_hash_map<int, double>& nonBorderMineCountProbabilities) const;
			void get_non_border_probabilities_by_mine_count(solver_map& map, common::point_map<double>& common_border_probabilities, std::vector<common::cell>& non_border_undecided_cells, common::point_map<double>& probabilities) const;
			bool should_stop_solving(common::point_map<bool>& verdicts, bool stop_on_no_mine_verdict, bool stop_on_any_verdict, bool stop_always) const;
			void get_final_results(common::point_map<double>& probabilities, common::point_map<bool>& verdicts, common::point_map<solver_result>& results) const;
		};
	}
}
