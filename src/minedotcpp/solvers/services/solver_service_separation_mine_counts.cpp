#include "solver_service_separation_mine_counts.h"

using namespace minedotcpp;
using namespace solvers;
using namespace services;
using namespace common;
using namespace std;

void minedotcpp::solvers::services::solver_service_separation_mine_counts::solve_mine_counts(solver_map& m, border& common_border, vector<border>& borders, point_map<double>& all_probabilities, point_map<bool>& all_verdicts) const
{
	if (m.remaining_mine_count == -1)
	{
		return;
	}

	auto original_common_border_pts = point_set();
	original_common_border_pts.resize(common_border.cells.size());
	for (auto& c : common_border.cells)
	{
		original_common_border_pts.insert(c.pt);
	}

	auto non_border_cells = vector<cell>();
	for (auto& c : m.cells)
	{
		if (c.state == cell_state_filled && original_common_border_pts.find(c.pt) == original_common_border_pts.end())
		{
			non_border_cells.push_back(c);
		}
	}

	auto common_border_coords = original_common_border_pts;
	for (auto& v : all_verdicts)
	{
		common_border_coords.erase(v.first);
	}

	auto fully_solved_borders = vector<border>();
	auto unsolved_borders = vector<border>();
	for (auto& b : borders)
	{
		if (b.solved_fully)
		{
			fully_solved_borders.push_back(b);
		}
		else
		{
			unsolved_borders.push_back(b);
			for (auto& c : b.cells)
			{
				common_border_coords.erase(c.pt);
			}
		}
	}

	auto borders_with_exact_mine_count = vector<border>();
	auto borders_with_variable_mine_count = vector<border>();
	auto exact_mine_count = 0;
	auto exact_border_size = 0;
	for (auto i = 0; i < fully_solved_borders.size(); i++)
	{
		auto& b = fully_solved_borders[i];
		auto guaranteed_mines = 0;
		auto guaranteed_empty = 0;
		for (auto j = 0; j < fully_solved_borders.size(); j++)
		{
			if (i == j)
			{
				continue;
			}
			auto& other = fully_solved_borders[j];
			guaranteed_mines += other.min_mine_count;
			guaranteed_empty += static_cast<int>(other.cells.size()) - other.max_mine_count;
		}
		trim_valid_combinations_by_mine_count(b, m.remaining_mine_count, m.undecided_count, guaranteed_mines, guaranteed_empty);
		if (b.min_mine_count == b.max_mine_count)
		{
			borders_with_exact_mine_count.push_back(b);
			for (auto& c : b.cells)
			{
				common_border_coords.erase(c.pt);
			}
			exact_mine_count += b.max_mine_count;
			exact_border_size += static_cast<int>(b.cells.size());
		}
		else
		{
			borders_with_variable_mine_count.push_back(b);
		}
	}

	auto non_border_mine_count_probabilities = google::dense_hash_map<int, double>();
	non_border_mine_count_probabilities.set_empty_key(-1);
	non_border_mine_count_probabilities.set_deleted_key(-2);
	if (borders_with_variable_mine_count.size() > 0)
	{
		auto exact_non_mine_count = exact_border_size - exact_mine_count;
		auto non_border_cell_count = static_cast<int>(non_border_cells.size());
		get_variable_mine_count_borders_probabilities(borders_with_variable_mine_count, m.remaining_mine_count, m.undecided_count, non_border_cell_count, exact_mine_count, exact_non_mine_count, all_probabilities, non_border_mine_count_probabilities);
	}

	// If requested, we calculate the probabilities of mines in non-border cells, and copy them over.
	if (settings.mine_count_solve_non_border)
	{

		get_non_border_probabilities_by_mine_count(m, all_probabilities, non_border_cells, all_probabilities);
	}

	// Lastly, we find guaranteed verdicts from the probabilties, and copy them over.
	get_verdicts_from_probabilities(all_probabilities, all_verdicts);
}

void solver_service_separation_mine_counts::trim_valid_combinations_by_mine_count(border& b, int minesRemaining, int undecidedCellsRemaining, int minesElsewhere, int nonMineCountElsewhere) const
{
	for (auto i = 0; i < b.valid_combinations.size(); i++)
	{
		auto& combination = b.valid_combinations[i];
		auto mine_prediction_count = 0;
		for (auto& p : combination)
		{
			if (p.second)
			{
				++mine_prediction_count;
			}
		}
		auto combination_size = static_cast<int>(combination.size());
		auto isValid = is_prediction_valid_by_mine_count(mine_prediction_count, combination_size, minesRemaining, undecidedCellsRemaining, minesElsewhere, nonMineCountElsewhere);
		if (!isValid)
		{
			b.valid_combinations.erase(b.valid_combinations.begin() + i);
			i--;
		}
	}
}

inline bool solver_service_separation_mine_counts::is_prediction_valid_by_mine_count(int mine_prediction_count, int total_combination_length, int mines_remaining, int undecided_cells_remaining, int mines_elsewhere, int non_mine_count_elsewhere) const
{
	if (mine_prediction_count + mines_elsewhere > mines_remaining)
	{
		return false;
	}
	if (mines_remaining - mine_prediction_count > undecided_cells_remaining - total_combination_length - non_mine_count_elsewhere)
	{
		return false;
	}
	return true;
}

static vector<vector<double>> combination_ratios = vector<vector<double>>();

static void initialize_combination_ratios()
{
	const auto max_size = 1600;
	combination_ratios.resize(max_size);
	for (auto n = 0; n < max_size; n++)
	{
		auto& ratios = combination_ratios[n];
		ratios.resize(max_size);
		//CombinationRatios[n] = ratios;
		ratios[0] = 1;
		for (auto k = 1; k <= n; k++)
		{
			auto ratio = (n + 1 - k) / double(k);
			ratios[k] = ratio;
		}
	}
}

inline static double combination_ratio(int from, int count)
{
	if (combination_ratios.size() == 0)
	{
		initialize_combination_ratios();
	}
	assert(count <= from);
	return combination_ratios[from][count];
}

void solver_service_separation_mine_counts::thr_mine_counts(vector<border>& variable_borders, int min, int max, int mines_remaining, int mines_elsewhere, int non_border_cell_count, int total_combination_length, int undecided_cells_remaining, int non_mine_count_elsewhere, google::dense_hash_map<int, double>& ratios, google::dense_hash_map<int, double>& non_border_mine_counts, point_map<double>& counts, mutex& common_lock, point_map<mutex*>& count_locks) const
{
	for (auto i = min; i < max; i++)
	{
		auto combinationArr = vector<point_map<bool>>();
		auto mine_prediction_count = 0;
		for (auto j = 0; j < variable_borders.size(); j++)
		{
			lldiv_t res{ i, 0 };
			res = div(res.quot, variable_borders[j].valid_combinations.size());
			auto& combo = variable_borders[j].valid_combinations[res.rem];
			for (auto& p : combo)
			{
				if (p.second)
				{
					++mine_prediction_count;
				}
			}
			combinationArr.push_back(combo);
		}

		auto mines_in_non_border = mines_remaining - mines_elsewhere - mine_prediction_count;
		if (mines_in_non_border < 0)
		{
			return;
		}
		if (mines_in_non_border > non_border_cell_count)
		{
			return;
		}
		auto& ratio = ratios[mines_in_non_border];
		{
			lock_guard<mutex> guard(common_lock);
			non_border_mine_counts[mines_in_non_border] += ratio;
		}
		auto is_valid = is_prediction_valid_by_mine_count(mine_prediction_count, total_combination_length, mines_remaining, undecided_cells_remaining, mines_elsewhere, non_mine_count_elsewhere);
		if (!is_valid)
		{
			//throw new Exception("temp");
			throw "temp";
		}
		for (auto& combo : combinationArr)
		{
			for (auto& verdict : combo)
			{
				if (verdict.second)
				{
					auto& mutex = count_locks[verdict.first];
					lock_guard<std::mutex> guard(*mutex);
					counts[verdict.first] += ratio;
				}
			}
		}
	}
}

void solver_service_separation_mine_counts::get_variable_mine_count_borders_probabilities(vector<border>& variable_borders, int mines_remaining, int undecided_cells_remaining, int non_border_cell_count, int mines_elsewhere, int non_mine_count_elsewhere, point_map<double>& target_probabilities, google::dense_hash_map<int, double>& non_border_mine_count_probabilities) const
{
	auto max_mines = 0;
	auto min_mines = 0;
	auto alreadyFoundMines = 0;
	auto total_combination_length = 0;
	auto total_combos = 1;
	auto counts = point_map<double>();
	auto count_locks = point_map<mutex*>();
	mutex common_lock;;
	for (auto& b : variable_borders)
	{
		for (auto& v : b.verdicts)
		{
			if (v.second)
			{
				++alreadyFoundMines;
			}
		}
		for (auto& c : b.cells)
		{
			counts[c.pt] = 0;
			count_locks[c.pt] = new mutex();
		}
		max_mines += b.max_mine_count;
		min_mines += b.min_mine_count;
		total_combination_length += static_cast<int>(b.cells.size());
		total_combos *= static_cast<int>(b.valid_combinations.size());
	}

	auto min_mines_in_non_border = mines_remaining - mines_elsewhere - max_mines + alreadyFoundMines;
	if (min_mines_in_non_border < 0)
	{
		min_mines_in_non_border = 0;
	}
	auto max_mines_in_non_border = mines_remaining - mines_elsewhere - min_mines + alreadyFoundMines;
	if (max_mines_in_non_border > non_border_cell_count)
	{
		max_mines_in_non_border = non_border_cell_count;
	}

	auto ratios = google::dense_hash_map<int, double>();
	ratios.set_empty_key(-1);
	ratios.set_deleted_key(-2);
	auto non_border_mine_counts = google::dense_hash_map<int, double>();
	non_border_mine_counts.set_empty_key(-1);
	non_border_mine_counts.set_deleted_key(-2);
	double currentRatio = 1;
	for (auto i = min_mines_in_non_border; i <= max_mines_in_non_border; i++)
	{
		currentRatio *= combination_ratio(non_border_cell_count, i);
		ratios[i] = currentRatio;
		non_border_mine_counts[i] = 0;
	}

	auto thread_count = thread::hardware_concurrency();
	if (total_combos > settings.variable_mine_count_borders_probabilities_multithread_use_from && thread_count > 1)
	{
		auto thread_load = total_combos / thread_count;
		vector<thread> threads;
		for (unsigned i = 0; i < thread_count; i++)
		{
			unsigned int min = thread_load * i;
			unsigned int max = min + thread_load;
			if (i == thread_count - 1)
			{
				max = total_combos;
			}
			threads.emplace_back([this, min, max, mines_remaining, mines_elsewhere, non_border_cell_count, total_combination_length, undecided_cells_remaining, non_mine_count_elsewhere, &variable_borders, &ratios, &non_border_mine_counts, &counts, &common_lock, &count_locks]()
			{
				thr_mine_counts(variable_borders, min, max, mines_remaining, mines_elsewhere, non_border_cell_count, total_combination_length, undecided_cells_remaining, non_mine_count_elsewhere, ratios, non_border_mine_counts, counts, common_lock, count_locks);
			});
		}

		for (auto& thr : threads)
		{
			thr.join();
		}
	}
	else
	{
		thr_mine_counts(variable_borders, 0, total_combos, mines_remaining, mines_elsewhere, non_border_cell_count, total_combination_length, undecided_cells_remaining, non_mine_count_elsewhere, ratios, non_border_mine_counts, counts, common_lock, count_locks);
	}

	for (auto& mutex_ptr : count_locks)
	{
		delete mutex_ptr.second;
	}

	double total_valid_combinations = 0;
	for (auto& mc : non_border_mine_counts)
	{
		total_valid_combinations += mc.second;
	}
	for (auto& mc : non_border_mine_counts)
	{
		if (mc.second < 0.0000000001)
		{
			continue;
		}
		non_border_mine_count_probabilities[mc.first] = mc.second / total_valid_combinations;
	}
	for (auto& count : counts)
	{
		auto probability = count.second / total_valid_combinations;
		target_probabilities[count.first] = probability;
	}
}

void solver_service_separation_mine_counts::get_non_border_probabilities_by_mine_count(solver_map& map, point_map<double>& common_border_probabilities, vector<cell>& non_border_undecided_cells, point_map<double>& probabilities) const
{
	if (map.remaining_mine_count == -1)
	{
		return;
	}

	double probabilitySum = 0;
	for (auto& p : common_border_probabilities)
	{
		probabilitySum += p.second;
	}
	if (non_border_undecided_cells.size() == 0)
	{
		return;
	}
	auto non_border_probability = (map.remaining_mine_count - probabilitySum) / non_border_undecided_cells.size();
	for (auto& non_border_filled_cell : non_border_undecided_cells)
	{
		probabilities[non_border_filled_cell.pt] = non_border_probability;
	}
}