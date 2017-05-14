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
	dump_gaussian_matrix(matrix);
	for(auto& p : parameters)
	{
		auto local_matrix = matrix;
		auto local_points = points;
		auto results = point_map<bool>();
		reduce_matrix(local_matrix, local_points, results, p);
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

void solver_service_gaussian::reduce_matrix(vector<vector<int>>& matrix, vector<point>& coordinates, point_map<bool>& allVerdicts, const matrix_reduction_parameters& parameters) const
{
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
				non_zero_index = 0;
				break;
			}
		}
		if(non_zero_index != -1)
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

	/*matrix = matrix.Where(x = > Array.FindIndex(x, y = > y != 0) != -1).Distinct(new ArrayEqualityComparer<int>()).OrderBy(x = > Array.FindIndex(x, y = > y != 0)).ToList();
	if(matrix.size() == 0)
	{
		return;
	}
	var splitsMade = false;
	parameters = parameters ? ? new MatrixReductionParameters();
	if(!parameters.SkipReduction)
	{
#if DEBUG
		Debug.WriteLine(MatrixToString(matrix));
#endif
		var rows = matrix.size();
		var cols = matrix[0].size();

		var rowsRemaining = new HashSet<int>(Enumerable.Range(0, rows));
		var m = matrix;
		var columnsData = Enumerable.Range(0, cols - 1).Select(x = > new {Index = x, Cnt = m.size()(y = > y[x] != 0)}).Where(x = > x.Cnt > 1);
		IEnumerable<int> columns;
		if(parameters.ReverseColumns)
		{
			columns = columnsData.OrderByDescending(x = > parameters.OrderColumns ? x.Cnt : x.Index).Select(x = > x.Index);
		}
		else
		{
			columns = columnsData.OrderBy(x = > parameters.OrderColumns ? x.Cnt : x.Index).Select(x = > x.Index);
		}

		//for (var col = 0; col < cols - 1; col++)
		foreach(var col in columns)
		{
			var row = -1;
			if(parameters.ReverseRows)
			{

				for(var i = rows - 1; i >= 0; i--)
				{
					var candidateNum = matrix[i][col];
					if((candidateNum == 1 || candidateNum == -1) && (!parameters.UseUniqueRows || rowsRemaining.Remove(i)))
					{
						row = i;
						break;
					}
				}
			}
			else
			{
				for(var i = 0; i < rows; i++)
				{
					var candidateNum = matrix[i][col];
					if((candidateNum == 1 || candidateNum == -1) && (!parameters.UseUniqueRows || rowsRemaining.Remove(i)))
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

			var targetNum = matrix[row][col];
			if(targetNum == -1)
			{
				for(int i = 0; i < cols; i++)
				{
					matrix[row][i] = -matrix[row][i];
				}
			}
			targetNum = matrix[row][col];
			//#if DEBUG
			//                Debug.WriteLine($"Row: {row}");
			//                Debug.WriteLine($"Col: {col}");
			//                Debug.WriteLine(MatrixToString(matrix));
			//#endif
			if(targetNum != 1)
			{
				File.WriteAllText("badmatrix.txt", MatrixToString(matrix));
				throw new Exception("coef!=1");
				for(int i = 0; i < cols; i++)
				{
					matrix[row][i] = matrix[row][i] / targetNum;
				}
			}

			for(var i = 0; i < rows; i++)
			{
				//var newRows = SeparateRow(matrix[i]).ToList();
				//if (newRows.size() > 1)
				//{
				//    splitsMade = true;
				//    matrix.RemoveAt(i);
				//    foreach (var newRow in newRows)
				//    {
				//        matrix.Insert(i, newRow);
				//    }
				//    rows += newRows.size() - 1;
				//    i += newRows.size() - 1;
				//}
				if(i == row)
				{
					continue;
				}
				var num = matrix[i][col];
				if(num != 0)
				{
					for(var j = 0; j < cols; j++)
					{
						matrix[i][j] -= matrix[row][j] * num;
					}
					//var newRowsAgain = SeparateRow(matrix[i]).ToList();
					//if (newRowsAgain.size() > 1)
					//{
					//    splitsMade = true;
					//    matrix.RemoveAt(i);
					//    foreach (var newRow in newRowsAgain)
					//    {
					//        matrix.Insert(i, newRow);
					//    }
					//    rows += newRowsAgain.size() - 1;
					//    i += newRowsAgain.size() - 1;
					//}
				}
			}
		}

		//matrix = matrix.Where(x => Array.FindIndex(x, y => y != 0) != -1).OrderBy(x => Array.FindIndex(x, y => y != 0)).ToList();
	}
	var cellsToRemove = new List<RowSeparationResult>();
	var rowList = new List<int[]>();
	for(int i = 0; i < matrix.size(); i++)
	{
		var row = matrix[i];
		var rowResults = SeparateRow(row).ToList();
		if(rowResults.size() > 0)
		{
			//splitsMade = true;
			cellsToRemove.AddRange(rowResults);
		}
		else
		{
			rowList.Add(row);
		}
	}
	var columnsToRemove = new HashSet<int>(cellsToRemove.Select(x = > x.ColumnIndex));
	foreach(var separationResult in cellsToRemove)
	{
		var col = separationResult.ColumnIndex;
		for(int i = 0; i < matrix.size(); i++)
		{
			var num = matrix[i][col];
			if(num != 0)
			{
				//matrix[i][col] -= matrix[row][col]*num;
				matrix[i][matrix[0].size() - 1] -= separationResult.Constant * num;
			}
		}
		allVerdicts[coordinates[col]] = separationResult.Constant == 1;
	}
	for(int i = 0; i < rowList.size(); i++)
	{
		rowList[i] = rowList[i].Where((x, index) = > !columnsToRemove.Contains(index)).ToArray();
	}
	matrix = rowList.Where(x = > Array.FindIndex(x, y = > y != 0) != -1).OrderBy(x = > Array.FindIndex(x, y = > y != 0)).ToList();
	coordinates = coordinates.Where((x, i) = > !columnsToRemove.Contains(i)).ToList();
	//#if DEBUG
	//            Debug.WriteLine(MatrixToString(matrix));
	//#endif
	if(matrix.size() == 0)
	{
		return;
	}
	if(splitsMade)
	{
		ReduceMatrix(ref coordinates, ref matrix, allVerdicts, parameters);
	}*/
}
