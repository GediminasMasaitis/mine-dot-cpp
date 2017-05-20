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
		matrix_reduction_parameters(true, false, false,false, false),
		matrix_reduction_parameters(false, true, false, true, true),
		matrix_reduction_parameters(false, true, true, true, true),
		matrix_reduction_parameters(false, false, false, true, true),
		matrix_reduction_parameters(false, false, true, true, true),
		matrix_reduction_parameters(false, false, true, false, true),
		matrix_reduction_parameters(false, true, false, true, false),
		matrix_reduction_parameters(false, true, true, true, false)
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
	for(auto& p : parameters)
	{
		auto local_matrix = matrix;
		auto local_points = points;
		auto round_verdicts = point_map<bool>();
		reduce_matrix(local_matrix, local_points, round_verdicts, p);

		
		for(auto& verdict : round_verdicts)
		{
			verdicts[verdict.first] = verdict.second;
		}
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
			hint_points.insert(c.pt);
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
		auto remaining_hint = static_cast<int>(hint_cell.hint - entry.by_flag[cell_flag_has_mine].size());

		auto row = vector<int>(points.size() + 1);
		auto& undecided_neighbours = m.neighbour_cache_get(hint_cell.pt).by_param[cell_state_filled | cell_flag_none];
		for(auto& undecided_neighbour : undecided_neighbours)
		{
			auto& index = indices[undecided_neighbour.pt];
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

struct target_less
{
	template<class It>
	bool operator()(It const &a, It const &b) const { return *a < *b; }
};
struct target_equal
{
	template<class It>
	bool operator()(It const &a, It const &b) const { return *a == *b; }
};
template<class It> It uniquify(It begin, It const end)
{
	std::vector<It> v;
	v.reserve(static_cast<size_t>(std::distance(begin, end)));
	for(It i = begin; i != end; ++i)
	{
		v.push_back(i);
	}
	std::sort(v.begin(), v.end(), target_less());
	v.erase(std::unique(v.begin(), v.end(), target_equal()), v.end());
	std::sort(v.begin(), v.end());
	size_t j = 0;
	for(It i = begin; i != end && j != v.size(); ++i)
	{
		if(i == v[j])
		{
			using std::iter_swap; iter_swap(i, begin);
			++j;
			++begin;
		}
	}
	return begin;
}

class column_data
{
public:
	column_data(int index, int cnt): index(index), cnt(cnt) {}
	int index;
	int cnt;
};

class row_separation_result
{
public:
	row_separation_result(int column_index, int constant) : column_index(column_index), constant(constant) {}
	int column_index;
	int constant;
};

void separate_row(vector<int>& row, vector<row_separation_result>& results)
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

	if(constant == positiveSum || constant == negativeSum)
	{
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
				results.emplace_back(i, newConstant);
			}
		}
		//splitsMade = true;
	}
}

void solver_service_gaussian::reduce_matrix(vector<vector<int>>& matrix, vector<point>& coordinates, point_map<bool>& allVerdicts, const matrix_reduction_parameters& parameters) const
{
	//dump_gaussian_matrix(matrix);
	if(matrix.size() == 0)
	{
		return;
	}

	auto non_zero_indices = vector<int>();

	for(auto i = 0; i < matrix.size(); i++)
	{
		auto& row = matrix[i];
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
			vector_erase_index_safe(matrix, i);
			--i;
			continue;
		}
		auto duplicate = false;
		for(auto j = i+1; j < matrix.size(); j++)
		{
			if(row == matrix[j])
			{
				duplicate = true;
				break;
			}
		}
		if(duplicate)
		{
			vector_erase_index_safe(matrix, i);
			--i;
			continue;
		}
		non_zero_indices.push_back(non_zero_index);
	}

	std::sort(matrix.begin(), matrix.end(), [](const vector<int>& lhs, const vector<int>& rhs)
	{
		auto index_lhs = static_cast<int>(std::find_if(lhs.begin(), lhs.end(), [](const int& i) {return i != 0; }) - lhs.begin());
		auto index_rhs = static_cast<int>(std::find_if(rhs.begin(), rhs.end(), [](const int& i) {return i != 0; }) - rhs.begin());
		return index_lhs < index_rhs;
	});

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
				for(int i = 0; i < cols; i++)
				{
					matrix[row][i] = -matrix[row][i];
				}
			}
			targetNum = matrix[row][col];

			assert(targetNum == 1);

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
	for(auto& c : cells_to_remove)
	{
		columns_to_remove.insert(c.column_index);
	}
	for(auto& separation_result : cells_to_remove)
	{
		auto col = separation_result.column_index;
		for(auto i = 0; i < matrix.size(); i++)
		{
			auto num = matrix[i][col];
			if(num != 0)
			{
				matrix[i][matrix[0].size() - 1] -= separation_result.constant * num;
			}
		}
		allVerdicts[coordinates[col]] = separation_result.constant == 1;
	}
	for(int i = 0; i < row_list.size(); i++)
	{
		for(auto& row : row_list)
		{
			auto new_row = vector<int>();
			for(auto j = 0; j < row.size(); j++)
			{
				if(columns_to_remove.find(j) != columns_to_remove.end())
				{
					new_row.push_back(row[j]);
				}
			}
			row_list[i] = new_row;
		}
	}
	
	matrix = vector<vector<int>>();
	for(auto i = 0; i < matrix.size(); i++)
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
			break;
		}
		matrix.push_back(row);
	}
	std::sort(matrix.begin(), matrix.end(), [](const vector<int>& lhs, const vector<int>& rhs)
	{
		auto index_lhs = static_cast<int>(std::find_if(lhs.begin(), lhs.end(), [](const int& i) {return i != 0; }) - lhs.begin());
		auto index_rhs = static_cast<int>(std::find_if(rhs.begin(), rhs.end(), [](const int& i) {return i != 0; }) - rhs.begin());
		return index_lhs < index_rhs;
	});
	for(auto i = 0; i < coordinates.size(); i++)
	{
		if(columns_to_remove.find(i) != columns_to_remove.end())
		{
			vector_erase_index_safe(coordinates, i);
		}
	}
	if(matrix.size() == 0)
	{
		return;
	}
	if(splitsMade)
	{
		reduce_matrix(matrix, coordinates, allVerdicts, parameters);
	}
}