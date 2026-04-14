#include "../../solver_map.h"
#include "../../border.h"
#include "../../../debug/debugging.h"
#include <thread>
#include <mutex>
#include <queue>
#include "solver_service_separation_combination_finding.h"
#ifdef ENABLE_OPEN_CL
#endif

using namespace minedotcpp;
using namespace solvers;
using namespace services;
using namespace common;
using namespace std;

#ifdef ENABLE_OPEN_CL
void solver_service_separation_combination_finding::cl_build_find_combination_program()
{
	
	if (!settings.valid_combination_search_open_cl)
	{
		return;
	}
	auto platforms = vector<cl::Platform>();
	auto devices = vector<cl::Device>();
	cl::Platform::get(&platforms);
	auto& platform = platforms[settings.valid_combination_search_open_cl_platform_id];
	platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
	auto& device = devices[settings.valid_combination_search_open_cl_device_id];
	cout << "Selected: " << device.getInfo<CL_DEVICE_NAME>() << endl;
	string cl_code = 
	R"(
//#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable
//#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

__kernel void find_combination(const unsigned char map_size, const __constant unsigned char* map, __global unsigned int* results, __global int* increment, const unsigned int offset)
{
	const unsigned int prediction = offset + get_global_id(0);

	unsigned char prediction_valid = 1;
	for(unsigned char i = 0; i < map_size; i++)
	{
#ifndef ALLOW_LOOP_BREAK
		if(!prediction_valid)
		{
			continue;
		}
#endif
		unsigned char neighbours_with_mine = 0;
		const unsigned short header_offset = i * 9;
		const unsigned char header = map[header_offset];
		const unsigned char neighbour_count = header & 0x0F;
		const unsigned char hint = header >> 4;
		for(unsigned char j = 1; j <= neighbour_count; j++)
		{
			const unsigned char neighbour = map[header_offset + j];
			const unsigned char flag = neighbour & 0x03;
			switch(flag)
			{
			case 1:
				++neighbours_with_mine;
				break;
			case 2:
				break;
			default:
			{
				if((prediction & (1 << (neighbour >> 2))) != 0)
				{
					++neighbours_with_mine;
				}
				break;
			}
			}
		}

		if(neighbours_with_mine != hint)
		{
			prediction_valid = 0;
#ifdef ALLOW_LOOP_BREAK
			break;
#endif
		}
	}
	
	if(prediction_valid)
	{
		results[atomic_inc(increment)] = prediction;
	}
}
	
	)";

	auto sources = cl::Program::Sources(1, std::make_pair(cl_code.c_str(), cl_code.length() + 1));
	cl_context = cl::Context(device);
	cl_find_combination_program = cl::Program(cl_context, sources);
	string build_args = "-cl-std=CL1.2";
	if (settings.valid_combination_search_open_cl_allow_loop_break)
	{
		build_args += " -D ALLOW_LOOP_BREAK";
	}
	auto err = cl_find_combination_program.build(build_args.c_str());
	auto context2 = cl_find_combination_program.getInfo<CL_PROGRAM_CONTEXT>(&err);
	auto devices2 = context2.getInfo<CL_CONTEXT_DEVICES>(&err);
}

void solver_service_separation_combination_finding::cl_validate_predictions(unsigned char map_size, vector<unsigned char>& map, ClResultArr& results, int& result_count, unsigned int total) const
{
	cl_int err;
	auto map_buf = cl::Buffer(cl_context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, map.size(), map.data(), &err);
	auto results_buf = cl::Buffer(cl_context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * results.size(), results.data(), &err);
	auto count_buf = cl::Buffer(cl_context, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &result_count, &err);

	auto kernel = cl::Kernel(cl_find_combination_program, "find_combination");

	err = kernel.setArg(0, map_size);
	err = kernel.setArg(1, map_buf);
	err = kernel.setArg(2, results_buf);
	err = kernel.setArg(3, count_buf);

	auto devices = cl_context.getInfo<CL_CONTEXT_DEVICES>(&err);
	auto device = devices[0];
	auto queue = cl::CommandQueue(cl_context, device);

	auto batch_load = static_cast<unsigned int>(1 << settings.valid_combination_search_open_cl_max_batch_size);
	auto batch_count = total / batch_load;
	if (batch_load * batch_count != total)
	{
		++batch_count;
		batch_load = total / batch_count;
	}

	for (auto i = 0; i < batch_count; i++)
	{
		unsigned int offset = batch_load * i;
		if (i == batch_count - 1)
		{
			batch_load = total - offset;
		}

		err = kernel.setArg(4, offset);
		auto run_err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(batch_load), cl::NullRange);
	}
	auto read_count_err = queue.enqueueReadBuffer(count_buf, CL_TRUE, 0, sizeof(int), &result_count);
	auto read_err = queue.enqueueReadBuffer(results_buf, CL_TRUE, 0, sizeof(int) * results.size(), results.data());
}
#endif
void solver_service_separation_combination_finding::validate_predictions(const unsigned char map_size, vector<unsigned char>& map, vector<unsigned int>& results, const unsigned int min, const unsigned int max, mutex* sync) const
{
	for (unsigned int prediction = min; prediction < max; prediction++)
	{
		unsigned char prediction_valid = 1;
		for (unsigned char i = 0; i < map_size; i++)
		{
			unsigned char neighbours_with_mine = 0;
			const unsigned short header_offset = i * 9;
			const unsigned char header = map[header_offset];
			const unsigned char neighbour_count = header & 0x0F;
			const unsigned char hint = header >> 4;
			for (unsigned char j = 1; j <= neighbour_count; j++)
			{
				const unsigned char neighbour = map[header_offset + j];
				const unsigned char flag = neighbour & 0x03;
				switch (flag)
				{
				case 1:
					++neighbours_with_mine;
					break;
				case 2:
					break;
				default:
				{
					if ((prediction & (1 << (neighbour >> 2))) != 0)
					{
						++neighbours_with_mine;
					}
					break;
				}
				}
			}

			if (neighbours_with_mine != hint)
			{
				prediction_valid = 0;
				break;
			}
		}

		if (prediction_valid)
		{
			if (sync != nullptr)
			{
				lock_guard<mutex> guard(*sync);
				results.push_back(prediction);
			}
			else
			{
				results.push_back(prediction);
			}
		}
	}
}

void solver_service_separation_combination_finding::thr_validate_predictions(unsigned char map_size, vector<unsigned char>& m, vector<unsigned int>& results, unsigned int total) const
{
	auto thread_count = thread::hardware_concurrency();
	auto thread_load = total / thread_count;
	auto threads = vector<thread>();
	mutex sync;
	for (unsigned i = 0; i < thread_count; i++)
	{
		unsigned int min = thread_load * i;
		unsigned int max = min + thread_load;
		if (i == thread_count - 1)
		{
			max = total;
		}

		threads.emplace_back([this, map_size, &m, &results, min, max, &sync]()
		{
			validate_predictions(map_size, m, results, min, max, &sync);
		});
	}

	for (auto& thr : threads)
	{
		thr.join();
	}
}

void solver_service_separation_combination_finding::thr_pool_validate_predictions(unsigned char map_size, vector<unsigned char>& m, vector<unsigned int>& results, unsigned int total) const
{
	auto thread_count = settings.valid_combination_search_multithread_thread_count;
	auto futures = vector<future<void>>();
	auto thread_load = total / thread_count;
	mutex sync;
	for (unsigned i = 0; i < thread_count; i++)
	{
		unsigned int min = thread_load * i;
		unsigned int max = min + thread_load;
		if (i == thread_count - 1)
		{
			max = total;
		}

		futures.emplace_back(thr_pool->push([this, map_size, &m, &results, min, max, &sync](int id)
		{
			validate_predictions(map_size, m, results, min, max, &sync);
		}));
	}

	for(auto& future : futures)
	{
		future.get();
	}
}

int solver_service_separation_combination_finding::find_hamming_weight(int i) const
{
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

void solver_service_separation_combination_finding::get_combination_search_map(solver_map& solver_map, border& border, vector<unsigned char>& m, unsigned char& map_size) const
{
	google::dense_hash_set<int> empty_pts_set;
	empty_pts_set.set_empty_key(-1);
	empty_pts_set.set_deleted_key(-2);
	auto cell_indices = vector<int>(solver_map.cells.size(), -1);
	for (auto i = 0; i < border.cells.size(); i++)
	{
		auto& c = border.cells[i];
		cell_indices[c.pt.x * solver_map.height + c.pt.y] = i;
		auto& entry = solver_map.neighbour_cache_get(c.pt).by_state[cell_state_empty];
		for (auto& cell : entry)
		{
			empty_pts_set.insert(cell->pt.x * solver_map.height + cell->pt.y);
		}
	}

	map_size = static_cast<unsigned char>(empty_pts_set.size());
	m.reserve(map_size * 9);
	for (auto& empty_pt : empty_pts_set)
	{
		auto& c = solver_map.cells[empty_pt];
		auto& entry = solver_map.neighbour_cache_get(c.pt);
		auto& filled_neighbours = entry.by_state[cell_state_filled];

		auto header_byte = static_cast<unsigned char>((c.hint << 4) | filled_neighbours.size());
		m.push_back(header_byte);

		for (auto j = 0; j < 8; j++)
		{
			if (j < filled_neighbours.size())
			{
				auto& neighbour = filled_neighbours[j];
				auto& cell_index = cell_indices[neighbour->pt.x * solver_map.height + neighbour->pt.y];
				auto neighbour_byte = static_cast<unsigned char>(cell_index << 2);
				auto flag = neighbour->state & cell_flags;
				neighbour_byte |= flag >> 2;
				m.push_back(neighbour_byte);
			}
			else
			{
				m.push_back(0xFF);
			}
		}
	}
}

void solver_service_separation_combination_finding::build_border_constraint_matrix(solver_map& map, border& border, vector<vector<int>>& matrix) const
{
	auto border_length = static_cast<int>(border.cells.size());

	point_map<int> border_indices;
	border_indices.resize(border_length);
	for (auto i = 0; i < border_length; i++)
	{
		border_indices[border.cells[i].pt] = i;
	}

	point_set hint_pts;
	for (auto& c : border.cells)
	{
		auto& empty_neighbours = map.neighbour_cache_get(c.pt).by_state[cell_state_empty];
		for (auto& empty_cell : empty_neighbours)
		{
			hint_pts.insert(empty_cell->pt);
		}
	}

	for (auto& hint_pt : hint_pts)
	{
		auto& hint_cell = map.cell_get(hint_pt);
		auto& entry = map.neighbour_cache_get(hint_pt);
		auto& filled_neighbours = entry.by_state[cell_state_filled];

		auto all_unflagged_in_border = true;
		auto flagged_mine_count = 0;
		for (auto& fn : filled_neighbours)
		{
			auto flag = fn->state & cell_flags;
			if (flag == cell_flag_has_mine)
			{
				flagged_mine_count++;
			}
			else if (flag == cell_flag_none)
			{
				if (border_indices.find(fn->pt) == border_indices.end())
				{
					all_unflagged_in_border = false;
					break;
				}
			}
		}
		if (!all_unflagged_in_border)
		{
			continue;
		}

		auto row = vector<int>(border_length + 1, 0);
		for (auto& fn : filled_neighbours)
		{
			auto flag = fn->state & cell_flags;
			if (flag != cell_flag_none)
			{
				continue;
			}
			auto it = border_indices.find(fn->pt);
			if (it != border_indices.end())
			{
				row[it->second] = 1;
			}
		}
		row[border_length] = hint_cell.hint - flagged_mine_count;
		matrix.push_back(row);
	}
}

void solver_service_separation_combination_finding::compute_rref(vector<vector<int>>& matrix, int num_vars, border_reduction_result& result) const
{
	auto num_rows = static_cast<int>(matrix.size());
	int pivot_row = 0;

	for (int col = 0; col < num_vars; col++)
	{
		int found = -1;
		for (int row = pivot_row; row < num_rows; row++)
		{
			if (matrix[row][col] == 1 || matrix[row][col] == -1)
			{
				found = row;
				break;
			}
		}
		if (found == -1)
		{
			result.free_variable_indices.push_back(col);
			continue;
		}

		if (found != pivot_row)
		{
			swap(matrix[found], matrix[pivot_row]);
		}

		if (matrix[pivot_row][col] == -1)
		{
			for (auto& val : matrix[pivot_row])
			{
				val = -val;
			}
		}

		for (int row = 0; row < num_rows; row++)
		{
			if (row == pivot_row) continue;
			auto factor = matrix[row][col];
			if (factor != 0)
			{
				for (int j = 0; j <= num_vars; j++)
				{
					matrix[row][j] -= factor * matrix[pivot_row][j];
				}
			}
		}

		result.pivot_columns.push_back(col);
		pivot_row++;
	}

	result.rank = pivot_row;
	result.free_count = num_vars - result.rank;
	result.valid = result.rank > 0;
	result.matrix = matrix;
}

border_reduction_result solver_service_separation_combination_finding::reduce_border(solver_map& map, border& border) const
{
	border_reduction_result result;
	if (border.cells.size() <= 1)
	{
		return result;
	}

	vector<vector<int>> matrix;
	build_border_constraint_matrix(map, border, matrix);

	if (matrix.empty())
	{
		return result;
	}

	compute_rref(matrix, static_cast<int>(border.cells.size()), result);

	if (result.valid && settings.combination_search_gaussian_backtracking)
	{
		precompute_backtracking_depths(result);
	}

	return result;
}

void solver_service_separation_combination_finding::validate_predictions_reduced(const border_reduction_result& reduction, int border_length, vector<unsigned int>& results, unsigned int min_free, unsigned int max_free, mutex* sync) const
{
	auto num_pivots = static_cast<int>(reduction.pivot_columns.size());
	auto num_free = static_cast<int>(reduction.free_variable_indices.size());
	auto num_vars = border_length;

	for (unsigned int free_val = min_free; free_val < max_free; free_val++)
	{
		unsigned int prediction = 0;
		bool valid = true;

		for (int i = 0; i < num_free; i++)
		{
			if (free_val & (1u << i))
			{
				prediction |= (1u << reduction.free_variable_indices[i]);
			}
		}

		for (int i = 0; i < num_pivots; i++)
		{
			int value = reduction.matrix[i][num_vars];
			for (int k = 0; k < num_free; k++)
			{
				if (free_val & (1u << k))
				{
					value -= reduction.matrix[i][reduction.free_variable_indices[k]];
				}
			}

			if (value != 0 && value != 1)
			{
				valid = false;
				break;
			}
			if (value == 1)
			{
				prediction |= (1u << reduction.pivot_columns[i]);
			}
		}

		if (valid)
		{
			if (sync != nullptr)
			{
				lock_guard<mutex> guard(*sync);
				results.push_back(prediction);
			}
			else
			{
				results.push_back(prediction);
			}
		}
	}
}

void solver_service_separation_combination_finding::precompute_backtracking_depths(border_reduction_result& reduction) const
{
	auto num_free = static_cast<int>(reduction.free_variable_indices.size());
	auto num_pivots = static_cast<int>(reduction.pivot_columns.size());
	auto num_vars = num_free + num_pivots;

	if (num_free == 0)
	{
		return;
	}

	// Build a set of free variable column indices for fast lookup
	vector<int> free_col_to_depth(num_vars + 1, -1);
	for (int k = 0; k < num_free; k++)
	{
		free_col_to_depth[reduction.free_variable_indices[k]] = k;
	}

	reduction.check_at_depth.resize(num_free);

	for (int i = 0; i < num_pivots; i++)
	{
		// Find the deepest free variable this dependent var depends on
		int max_depth = -1;
		for (int k = 0; k < num_free; k++)
		{
			int free_col = reduction.free_variable_indices[k];
			if (reduction.matrix[i][free_col] != 0)
			{
				if (k > max_depth)
				{
					max_depth = k;
				}
			}
		}

		if (max_depth >= 0)
		{
			reduction.check_at_depth[max_depth].push_back(i);
		}
		else
		{
			// Depends on no free variable — determined by constant alone
			// Check at depth 0 (before any assignment)
			reduction.check_at_depth[0].push_back(i);
		}
	}
}

void solver_service_separation_combination_finding::backtrack(const border_reduction_result& reduction, int border_length, int depth, unsigned int free_val, unsigned int prediction, vector<unsigned int>& results) const
{
	auto num_free = static_cast<int>(reduction.free_variable_indices.size());
	auto num_vars = border_length;

	if (depth == num_free)
	{
		results.push_back(prediction);
		return;
	}

	for (unsigned int bit = 0; bit <= 1; bit++)
	{
		unsigned int next_free_val = free_val;
		unsigned int next_prediction = prediction;

		if (bit)
		{
			next_free_val |= (1u << depth);
			next_prediction |= (1u << reduction.free_variable_indices[depth]);
		}

		// Check all dependent variables that become fully determined at this depth
		bool valid = true;
		unsigned int dep_bits = 0;
		for (auto pivot_row : reduction.check_at_depth[depth])
		{
			int value = reduction.matrix[pivot_row][num_vars];
			for (int k = 0; k <= depth; k++)
			{
				if (next_free_val & (1u << k))
				{
					value -= reduction.matrix[pivot_row][reduction.free_variable_indices[k]];
				}
			}

			if (value != 0 && value != 1)
			{
				valid = false;
				break;
			}
			if (value == 1)
			{
				dep_bits |= (1u << reduction.pivot_columns[pivot_row]);
			}
		}

		if (valid)
		{
			backtrack(reduction, border_length, depth + 1, next_free_val, next_prediction | dep_bits, results);
		}
	}
}

void solver_service_separation_combination_finding::validate_predictions_backtracking(const border_reduction_result& reduction, int border_length, vector<unsigned int>& results) const
{
	backtrack(reduction, border_length, 0, 0, 0, results);
}

void solver_service_separation_combination_finding::thr_pool_validate_predictions_reduced(const border_reduction_result& reduction, int border_length, vector<unsigned int>& results, unsigned int total_free) const
{
	auto thread_count = settings.valid_combination_search_multithread_thread_count;
	auto futures = vector<future<void>>();
	auto thread_load = total_free / thread_count;
	mutex sync;
	for (unsigned i = 0; i < static_cast<unsigned>(thread_count); i++)
	{
		unsigned int min = thread_load * i;
		unsigned int max = min + thread_load;
		if (i == static_cast<unsigned>(thread_count) - 1)
		{
			max = total_free;
		}

		futures.emplace_back(thr_pool->push([this, &reduction, border_length, &results, min, max, &sync](int id)
		{
			validate_predictions_reduced(reduction, border_length, results, min, max, &sync);
		}));
	}

	for (auto& future : futures)
	{
		future.get();
	}
}

static ClResultArr global_results = {};

void solver_service_separation_combination_finding::find_valid_border_cell_combinations(solver_map& solver_map, border& border, const border_reduction_result& reduction) const
{
	auto border_length = border.cells.size();
	auto results = vector<unsigned int>();
	auto all_remaining_cells_in_border = solver_map.undecided_count == border_length;

	bool use_reduced = reduction.valid && reduction.free_count < static_cast<int>(border_length);

	if (use_reduced)
	{
		if (settings.combination_search_gaussian_backtracking && !reduction.check_at_depth.empty())
		{
			validate_predictions_backtracking(reduction, static_cast<int>(border_length), results);
		}
		else
		{
			auto total_free = static_cast<unsigned int>(1u << reduction.free_count);

			if (settings.valid_combination_search_multithread && reduction.free_count >= settings.valid_combination_search_multithread_use_from_size)
			{
				thr_pool_validate_predictions_reduced(reduction, static_cast<int>(border_length), results, total_free);
			}
			else
			{
				validate_predictions_reduced(reduction, static_cast<int>(border_length), results, 0, total_free, nullptr);
			}
		}
	}
	else
	{
		auto total = static_cast<unsigned int>(1 << border_length);
		auto combination_search_map = vector<unsigned char>();
		unsigned char map_size;
		get_combination_search_map(solver_map, border, combination_search_map, map_size);

#ifdef ENABLE_OPEN_CL
		if (settings.valid_combination_search_open_cl && border_length >= settings.valid_combination_search_open_cl_use_from_size)
		{
			int result_count = 0;
			cl_validate_predictions(map_size, combination_search_map, global_results, result_count, total);
			results.reserve(result_count);
			for (auto i = 0; i < result_count; i++)
			{
				results.push_back(global_results[i]);
			}
		}
		else
#endif
		if (settings.valid_combination_search_multithread && border_length >= settings.valid_combination_search_multithread_use_from_size)
		{
			thr_pool_validate_predictions(map_size, combination_search_map, results, total);
		}
		else
		{
			validate_predictions(map_size, combination_search_map, results, 0, total, nullptr);
		}
	}

	for (const auto& prediction : results)
	{
		if (solver_map.remaining_mine_count > 0)
		{
			const auto bits_set = find_hamming_weight(prediction);
			if (bits_set > solver_map.remaining_mine_count)
			{
				continue;
			}
			if (all_remaining_cells_in_border && bits_set != solver_map.remaining_mine_count)
			{
				continue;
			}
		}
		point_map<bool> predictions;
		predictions.resize(border_length);
		auto mine_count = 0;
		for (unsigned int j = 0; j < border_length; j++)
		{
			auto& pt = border.cells[j].pt;
			auto has_mine = (prediction & (1 << j)) > 0;
			predictions[pt] = has_mine;
			if (has_mine)
			{
				++mine_count;
			}
		}
		border.valid_combinations.emplace_back(mine_count, predictions);
	}
}