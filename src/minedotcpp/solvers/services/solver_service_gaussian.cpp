#include "solver_service_gaussian.h"
#include "../../debug/debugging.h"
#include "../matrix_reduction_parameters.h"
#include "../../common/common_functions.h"

using namespace minedotcpp;
using namespace common;
using namespace solvers;
using namespace services;
using namespace std;

void solver_service_gaussian::solve_gaussian(solver_map& m, point_map<bool>& verdicts) const
{
	auto parameters = vector<matrix_reduction_parameters>
	{
		//matrix_reduction_parameters(true, false, false),  // 0.4   3.4

		matrix_reduction_parameters(false, false, true),  // 9.9   30.5
		matrix_reduction_parameters(false, true, true),   // 9.9   29.3
		matrix_reduction_parameters(false, true, false),  // 11.3  27.7
		matrix_reduction_parameters(false, false, false), // 1.2   7.4

		/*matrix_reduction_parameters(false, false, true, false, false),  // 0.6   5.9
		matrix_reduction_parameters(false, false, true, false, true),   // 16.1  31.8
		matrix_reduction_parameters(false, false, true, true, false),   // 5.9   23.1
		matrix_reduction_parameters(false, false, true, true, true),    // 16.2  32*/

		/*matrix_reduction_parameters(false, true, false, false, false),  // 0.7   7.7
		matrix_reduction_parameters(false, true, false, false, true),   // 20.7  31.4
		matrix_reduction_parameters(false, true, false, true, false),   // 5.5   23.7
		matrix_reduction_parameters(false, true, false, true, true),    // 20.8  30.7*/

		/*matrix_reduction_parameters(false, true, true, false, false),   // 1.3   6.8
		matrix_reduction_parameters(false, true, true, false, true),    // 11.2  29.1
		matrix_reduction_parameters(false, true, true, true, false),    // 12.3  27.6
		matrix_reduction_parameters(false, true, true, true, true),     // 11.7  28.8*/
	};
	auto points = vector<point>();
	auto matrix = vector<vector<int>>();
	get_matrix_from_map(m, points, true, matrix);
	//mutex sync;
	//auto futures = vector<future<void>>();
	for(auto& p : parameters)
	{
		//futures.emplace_back(thr_pool->push([this, &matrix, &points, &verdicts, &sync, &p](int thr_num)
		//{
			//auto local_matrix = matrix;
			//auto local_points = points;
			//auto round_verdicts = point_map<bool>();
			solve_gaussian_with_parameters(matrix, points, verdicts, p);

			//lock_guard<mutex> guard(sync);
			/*for(auto& verdict : round_verdicts)
			{
				assert([&]()
				{
					auto old_iter = verdicts.find(verdict.first);
					return old_iter == verdicts.end() || old_iter->second == verdict.second;
				}());
				verdicts[verdict.first] = verdict.second;
			}*/
		if(matrix.size() == 0)
		{
			break;
		}
		if(should_stop_solving(verdicts, settings.gaussian_single_stop_on_no_mine_verdict, settings.gaussian_single_stop_on_any_verdict, settings.gaussian_single_stop_always))
		{
			break;
		}
		//}));
	}

	//for(auto& future : futures)
	//{
	//	future.get();
	//}
}

void solver_service_gaussian::get_matrix_from_map(solver_map& m, vector<point>& points, bool all_undecided_coordinates_provided, vector<vector<int>>& matrix) const
{
	for(auto&c : m.cells)
	{
		if(c.state == cell_state_filled)
		{
			points.push_back(c.pt);
		}
	}
	auto hint_points = point_set();
	auto indices = point_map<int>();
	for(auto i = 0; i < points.size(); i++)
	{
		auto& pt = points[i];
		auto& entry = m.neighbour_cache_get(pt);
		auto& empty = entry.by_state[cell_state_empty];
		for(auto& c : empty)
		{
			hint_points.insert(c->pt);
		}
		indices[pt] = i;
	}

	auto hint_cells = vector<cell>();
	hint_cells.reserve(hint_points.size());
	for(auto& pt : hint_points)
	{
		hint_cells.push_back(m.cell_get(pt));
	}

	for(auto i = 0; i < hint_cells.size(); i++)
	{
		auto& hint_cell = hint_cells[i];
		auto& entry = m.neighbour_cache_get(hint_cell.pt);
		auto remaining_hint = static_cast<int>(hint_cell.hint - entry.by_flag[cell_flag_has_mine >> 2].size());

		auto row = vector<int>(points.size() + 1);
		auto& undecided_neighbours = m.neighbour_cache_get(hint_cell.pt).by_state[cell_state_filled];
		for(auto& undecided_neighbour : undecided_neighbours)
		{
			auto flag = undecided_neighbour->state & cell_flags;
			if(flag != cell_flag_none)
			{
				continue;
			}
			auto& index = indices[undecided_neighbour->pt];
			row[index] = 1;
		}
		row[row.size() - 1] = remaining_hint;
		matrix.push_back(row);
	}
	if(all_undecided_coordinates_provided && m.remaining_mine_count != -1)
	{
		auto row = vector<int>(points.size() + 1);
		for(auto i = 0; i < points.size(); i++)
		{
			row[i] = 1;
		}
		row[row.size() - 1] = m.remaining_mine_count;
		matrix.push_back(row);
	}
}

void solver_service_gaussian::solve_gaussian_with_parameters(vector<vector<int>>& matrix, vector<point>& coordinates, point_map<bool>& verdicts, const matrix_reduction_parameters& parameters) const
{
	bool found_results;
	do
	{
		prepare_matrix(matrix);
		if(matrix.size() == 0)
		{
			return;
		}

		if(!parameters.skip_reduction)
		{
			reduce_matrix(matrix, coordinates, parameters);
		}

		found_results = find_results(matrix, coordinates, verdicts);
	} while(settings.gaussian_resolve_on_success && found_results);
}

void solver_service_gaussian::prepare_matrix(vector<vector<int>>& matrix) const
{
	auto non_zero_indices = vector<int>();

	auto hashes = vector<unsigned int>();
	hashes.reserve(matrix.size());
	for (auto i = 0; i < matrix.size(); i++)
	{
		auto& row = matrix[i];
		unsigned int hash = 0;
		for (auto& num : row)
		{
			hash = (hash * 17) ^ num;
		}
		hashes.push_back(hash);
	}

	for (auto i = 0; i < matrix.size(); i++)
	{
		auto& row = matrix[i];
		auto non_zero_index = -1;
		for (auto j = 0; j < row.size(); ++j)
		{
			if (row[j] != 0)
			{
				non_zero_index = j;
				break;
			}
		}
		if (non_zero_index == -1)
		{
			vector_erase_index_safe(matrix, i);
			vector_erase_index_safe(hashes, i);
			--i;
			continue;
		}
		auto duplicate = false;
		for (auto j = i + 1; j < matrix.size(); j++)
		{
			if (hashes[i] != hashes[j])
			{
				continue;
			}
			duplicate = true;
			for (int k = row.size() - 1; k >= 0; --k)
			{
				if (row[k] != matrix[j][k])
				{
					duplicate = false;
					break;
				}
			}
			if (duplicate)
			{
				break;
			}
		}
		if (duplicate)
		{
			vector_erase_index_safe(matrix, i);
			vector_erase_index_safe(hashes, i);
			--i;
			continue;
		}
		non_zero_indices.push_back(non_zero_index);
	}

	vector_sort_by<vector<int>, int>(matrix, [](const vector<int>& row)
	{
		return static_cast<int>(std::find_if(row.begin(), row.end(), [](const int& i) {return i != 0; }) - row.begin());
	}, [](const int& lhs, const int& rhs)
	{
		return lhs < rhs;
	});
}

void solver_service_gaussian::reduce_matrix(vector<vector<int>>& matrix, vector<point>& coordinates, const matrix_reduction_parameters& parameters) const
{
	auto rows = static_cast<int>(matrix.size());
	auto cols = static_cast<int>(matrix[0].size());

	auto rows_remaining = google::dense_hash_set<int>();
	rows_remaining.set_empty_key(-1);
	rows_remaining.set_deleted_key(-2);

	for(auto i = 0; i < rows; i++)
	{
		rows_remaining.insert(i);
	}

	auto columns_data = vector<column_data>();
	for(auto i = 0; i < cols - 1; ++i)
	{
		auto cnt = 0;
		for(auto& row : matrix)
		{
			if(row[i] != 0)
			{
				cnt++;
			}
		}
		if(cnt > 1)
		{
			columns_data.emplace_back(i, cnt);
		}
	}

	/*std::sort(columns_data.begin(), columns_data.end(), [&parameters](const column_data& lhs, const column_data& rhs)
	{
		if(parameters.reverse_columns)
		{
			if(parameters.order_columns)
			{
				return lhs.cnt > rhs.cnt;
			}
			else
			{
				return lhs.index > rhs.index;
			}
		}
		else
		{
			if(parameters.order_columns)
			{
				return lhs.cnt < rhs.cnt;
			}
			else
			{
				return lhs.index < rhs.index;
			}
		}
	});*/

	auto columns = vector<int>();
	columns.reserve(columns_data.size());
	for(auto& col : columns_data)
	{
		columns.push_back(col.index);
	}

	for(auto& col : columns)
	{
		auto row = -1;
		if(parameters.reverse_rows)
		{
			for(auto i = rows - 1; i >= 0; i--)
			{
				auto candidate_num = matrix[i][col];
				if((candidate_num == 1 || candidate_num == -1) && (!parameters.use_unique_rows || rows_remaining.erase(i)))
				{
					row = i;
					break;
				}
			}
		}
		else
		{
			for(auto i = 0; i < rows; i++)
			{
				auto candidate_num = matrix[i][col];
				if((candidate_num == 1 || candidate_num == -1) && (!parameters.use_unique_rows || rows_remaining.erase(i)))
				{
					row = i;
					break;
				}
			}
		}
		if(row == -1)
		{
			continue;
		}

		auto target_num = matrix[row][col];
		if(target_num == -1)
		{
			for(auto i = 0; i < cols; i++)
			{
				matrix[row][i] = -matrix[row][i];
			}
		}

		assert(matrix[row][col] == 1);

		for(auto i = 0; i < rows; i++)
		{
			if(i == row)
			{
				continue;
			}
			auto num = matrix[i][col];
			if(num != 0)
			{
				for(auto j = 0; j < cols; j++)
				{
					matrix[i][j] -= matrix[row][j] * num;
				}
			}
		}
	}
}

bool solver_service_gaussian::find_results(vector<vector<int>>& matrix, vector<point>& coordinates, point_map<bool>& verdicts) const
{
	auto separation_results = google::dense_hash_map<int, int>();
	separation_results.set_empty_key(-1);
	separation_results.set_deleted_key(-2);
	auto unsolved_rows = vector<vector<int>>();
	for(auto i = 0; i < matrix.size(); i++)
	{
		auto& row = matrix[i];
		auto separation_success = separate_row(row, separation_results);
		if(!separation_success)
		{
			unsolved_rows.push_back(row);
		}
	}
	if(separation_results.size() == 0)
	{
		return false;
	}
	auto last_col = matrix[0].size() - 1;
	for(auto& separation_result : separation_results)
	{
		auto& col = separation_result.first;
		for(auto i = 0; i < unsolved_rows.size(); i++)
		{
			auto num = unsolved_rows[i][col];
			if(num != 0)
			{
				unsolved_rows[i][last_col] -= separation_result.second * num;
			}
		}
		auto& pt = coordinates[col];
		auto verdict = separation_result.second == 1;
		verdicts[pt] = verdict;
	}
	for(auto i = 0; i < unsolved_rows.size(); i++)
	{
		auto& row = unsolved_rows[i];
		auto new_row = vector<int>();
		new_row.reserve(row.size() - separation_results.size());
		for(auto j = 0; j < row.size(); j++)
		{
			if(separation_results.find(j) == separation_results.end())
			{
				new_row.push_back(row[j]);
			}
		}
		unsolved_rows[i] = new_row;
	}

	auto remaining_coordinates = vector<point>();
	remaining_coordinates.reserve(coordinates.size() - separation_results.size());
	for(auto i = 0; i < coordinates.size(); i++)
	{
		if(separation_results.find(i) == separation_results.end())
		{
			remaining_coordinates.push_back(coordinates[i]);
		}
	}
	matrix = unsolved_rows;
	coordinates = remaining_coordinates;
	return true;
}

bool solver_service_gaussian::separate_row(vector<int>& row, google::dense_hash_map<int, int>& results) const
{
	auto constantIndex = row.size() - 1;
	auto constant = row[constantIndex];

	auto positiveSum = 0;
	auto negativeSum = 0;
	for(int i = 0; i < row.size() - 1; i++)
	{
		auto num = row[i];
		if(num == 0)
		{
			continue;
		}
		if(num > 0)
		{
			positiveSum += num;
		}
		else
		{
			negativeSum += num;
		}
	}

	if(constant != positiveSum && constant != negativeSum)
	{
		return false;
	}

	int forPositive;
	int forNegative;
	if(constant == positiveSum)
	{
		forPositive = 1;
		forNegative = 0;
	}
	else
	{
		forPositive = 0;
		forNegative = 1;
	}
	for(auto i = 0; i < row.size() - 1; i++)
	{
		if(row[i] != 0)
		{
			auto newConstant = row[i] > 0 ? forPositive : forNegative;
			results.emplace(i, newConstant);
		}
	}
	return true;
}