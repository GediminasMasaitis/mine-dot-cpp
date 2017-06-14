#include "solver_service_gaussian.h"
#include "../../debug/debugging.h"
#include "../matrix_reduction_parameters.h"
#include "../../common/common_functions.h"

using namespace minedotcpp;
using namespace common;
using namespace solvers;
using namespace services;
using namespace std;

void solver_service_gaussian::
solve_gaussian(solver_map& m, point_map<bool>& verdicts) const
{
	auto parameters = vector<matrix_reduction_parameters>
	{
		//matrix_reduction_parameters(true, false, false,false, false), // 0.4
		matrix_reduction_parameters(false, true, false, true, true), // 20.8
		//matrix_reduction_parameters(false, true, true, true, true), // 11.7
		//matrix_reduction_parameters(false, false, false, true, true), // 9.9
		//matrix_reduction_parameters(false, false, true, true, true), // 16.2
		//matrix_reduction_parameters(false, false, true, false, true), // 16.1
		//matrix_reduction_parameters(false, true, false, true, false), // 5.5
		//matrix_reduction_parameters(false, true, true, true, false) // 12.3

		// everything everything = 34.3
		/*matrix_reduction_parameters(false, false, false, false, false),
		matrix_reduction_parameters(false, false, false, false, true),
		matrix_reduction_parameters(false, false, false, true, false),
		matrix_reduction_parameters(false, false, false, true, true),
		matrix_reduction_parameters(false, false, true, false, false),
		matrix_reduction_parameters(false, false, true, false, true),
		matrix_reduction_parameters(false, false, true, true, false),
		matrix_reduction_parameters(false, false, true, true, true),
		matrix_reduction_parameters(false, true, false, false, false),
		matrix_reduction_parameters(false, true, false, false, true),
		matrix_reduction_parameters(false, true, false, true, false),
		matrix_reduction_parameters(false, true, false, true, true),
		matrix_reduction_parameters(false, true, true, false, false),
		matrix_reduction_parameters(false, true, true, false, true),
		matrix_reduction_parameters(false, true, true, true, false),
		matrix_reduction_parameters(false, true, true, true, true),*/
	};
	auto points = vector<point>();
	for(auto&c : m.cells)
	{
		if(c.state == cell_state_filled)
		{
			points.push_back(c.pt);
		}
	}
	auto matrix = vector<vector<int>>();
	get_matrix_from_map(m, points, true, matrix);
	std::mutex sync;
	auto futures = vector<future<void>>();
	for(auto& p : parameters)
	{
		//futures.emplace_back(thr_pool->push([this, &matrix, &points, &verdicts, &sync, &p](int thr_num)
		//{
			auto local_matrix = matrix;
			auto local_points = points;
			auto round_verdicts = point_map<bool>();
			reduce_matrix(local_matrix, local_points, round_verdicts, p);

			lock_guard<mutex> guard(sync);
			for(auto& verdict : round_verdicts)
			{
				assert([&]()
				{
					auto old_iter = verdicts.find(verdict.first);
					return old_iter == verdicts.end() || old_iter->second == verdict.second;
				}());
				verdicts[verdict.first] = verdict.second;
			}
		//}));
	}

	for(auto& future : futures)
	{
		future.get();
	}
}

void solver_service_gaussian::get_matrix_from_map(solver_map& m, vector<point>& points, bool all_undecided_coordinates_provided, vector<vector<int>>& matrix) const
{
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

void solver_service_gaussian::reduce_matrix(vector<vector<int>>& matrix, vector<point>& coordinates, point_map<bool>& allVerdicts, const matrix_reduction_parameters& parameters) const
{
	//dump_gaussian_matrix(matrix);
	if(matrix.size() == 0)
	{
		return;
	}

	prepare_matrix(matrix);

	if(matrix.size() == 0)
	{
		return;
	}
	auto splitsMade = false;
	if(!parameters.skip_reduction)
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
		
		std::sort(columns_data.begin(), columns_data.end(), [&parameters](const column_data& lhs, const column_data& rhs)
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
		});

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
					auto candidateNum = matrix[i][col];
					if((candidateNum == 1 || candidateNum == -1) && (!parameters.use_unique_rows || rows_remaining.erase(i)))
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
					auto candidateNum = matrix[i][col];
					if((candidateNum == 1 || candidateNum == -1) && (!parameters.use_unique_rows || rows_remaining.erase(i)))
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

			auto targetNum = matrix[row][col];
			if(targetNum == -1)
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
	auto cells_to_remove = vector<row_separation_result>();
	auto row_list = vector<vector<int>>();
	for(auto i = 0; i < matrix.size(); i++)
	{
		auto& row = matrix[i];
		auto rowResults = vector<row_separation_result>();
		separate_row(row, rowResults);
		if(rowResults.size() > 0)
		{
			//splitsMade = true;
			for(auto& row_result: rowResults)
			{
				cells_to_remove.push_back(row_result);
			}
		}
		else
		{
			row_list.push_back(row);
		}
	}
	auto columns_to_remove = google::dense_hash_set<int>();
	columns_to_remove.set_empty_key(-1);
	columns_to_remove.set_deleted_key(-2);
	auto last_col = matrix[0].size() - 1;
	for(auto& separation_result : cells_to_remove)
	{
		auto col = separation_result.column_index;
		columns_to_remove.insert(col);
		for(auto i = 0; i < matrix.size(); i++)
		{
			auto num = matrix[i][col];
			if(num != 0)
			{
				matrix[i][last_col] -= separation_result.constant * num;
			}
		}
		auto& pt = coordinates[col];
		auto verdict = separation_result.constant == 1;
		allVerdicts[pt] = verdict;
	}
	for(auto i = 0; i < row_list.size(); i++)
	{
		auto& row = row_list[i];
		auto new_row = vector<int>();
		new_row.reserve(row.size() - columns_to_remove.size());
		for(auto j = 0; j < row.size(); j++)
		{
			if(columns_to_remove.find(j) != columns_to_remove.end())
			{
				new_row.push_back(row[j]);
			}
		}
		row_list[i] = new_row;
	}
	
	matrix = vector<vector<int>>();
	for(auto i = 0; i < row_list.size(); i++)
	{
		auto& row = row_list[i];
		auto non_zero_index = -1;
		for(auto j = 0; j < row.size(); ++j)
		{
			if(row[j] != 0)
			{
				non_zero_index = j;
				break;
			}
		}
		if(non_zero_index == -1)
		{
			continue;
		}
		matrix.push_back(row);
	}
	vector_sort_by<vector<int>, int>(matrix, [](const vector<int>& row)
	{
		return static_cast<int>(std::find_if(row.begin(), row.end(), [](const int& i) {return i != 0; }) - row.begin());
	}, [](const int& lhs, const int& rhs)
	{
		return lhs < rhs;
	});
	auto new_coordinates = vector<point>();
	new_coordinates.reserve(coordinates.size() - columns_to_remove.size());
	for(auto i = 0; i < coordinates.size(); i++)
	{
		if(columns_to_remove.find(i) == columns_to_remove.end())
		{
			new_coordinates.push_back(coordinates[i]);
		}
	}
	if(matrix.size() == 0)
	{
		return;
	}
	if(splitsMade)
	{
		reduce_matrix(matrix, new_coordinates, allVerdicts, parameters);
	}
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

void solver_service_gaussian::separate_row(vector<int>& row, vector<row_separation_result>& results) const
{
	auto constantIndex = row.size() - 1;
	auto constant = row[constantIndex];

	auto positiveSum = 0;
	auto negativeSum = 0;
	for (int i = 0; i < row.size() - 1; i++)
	{
		auto num = row[i];
		if (num == 0)
		{
			continue;
		}
		if (num > 0)
		{
			positiveSum += num;
		}
		else
		{
			negativeSum += num;
		}
	}

	if (constant == positiveSum || constant == negativeSum)
	{
		int forPositive;
		int forNegative;
		if (constant == positiveSum)
		{
			forPositive = 1;
			forNegative = 0;
		}
		else
		{
			forPositive = 0;
			forNegative = 1;
		}
		for (auto i = 0; i < row.size() - 1; i++)
		{
			if (row[i] != 0)
			{
				auto newConstant = row[i] > 0 ? forPositive : forNegative;
				results.emplace_back(i, newConstant);
			}
		}
		//splitsMade = true;
	}
}