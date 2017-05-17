#include "../solver_map.h"
#include "../border.h"
#include "../../debug/debugging.h"
#include <thread>
#include <mutex>
#include <queue>
#include "solver_service_separation_combination_finding.h"
#ifdef ENABLE_OPEN_CL
#include <CL/cl.hpp>
#endif

using namespace minedotcpp;
using namespace solvers;
using namespace services;
using namespace common;
using namespace std;

#ifdef ENABLE_OPEN_CL
void solver_service_separation_combination_finding::cl_build_find_combination_program()
{
	if(!settings.valid_combination_search_open_cl)
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

	string cl_code = R"(

//#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable
//#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

__kernel void FindCombination(__constant unsigned char* map, __global int* results, __global int* increment)
{
	size_t prediction = get_global_id(0);

	unsigned char prediction_valid = 1;
	unsigned char empty_pts_count = map[0];
	for(int i = 0; i < empty_pts_count; i++)
	{
		if(!prediction_valid)
		{
			continue;
		}
		int neighbours_with_mine = 0;
		int header_offset = 1 + i * 9;
		unsigned char header = map[header_offset];
		int neighbour_count = header & 0x0F;
		int hint = header >> 4;
		for(int j = 1; j <= neighbour_count; j++)
		{
			unsigned char neighbour = map[header_offset + j];
			unsigned char flag = neighbour & 0x03;
			switch(flag)
			{
			case 1:
				++neighbours_with_mine;
				break;
			case 2:
				break;
			default:
			{
				unsigned char cell_index = neighbour >> 2;
				unsigned char verdict = (prediction & (1 << cell_index)) > 0;
				if(verdict)
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
	auto err = cl_find_combination_program.build("-cl-std=CL1.2");
	auto context2 = cl_find_combination_program.getInfo<CL_PROGRAM_CONTEXT>(&err);
	auto devices2 = context2.getInfo<CL_CONTEXT_DEVICES>(&err);
}

void solver_service_separation_combination_finding::cl_validate_predictions(vector<unsigned char>& map, vector<int>& results, unsigned int total) const
{
	cl_int err;
	auto devices = cl_context.getInfo<CL_CONTEXT_DEVICES>(&err);
	auto device = devices[0];

	auto offs = 0;
	auto map_buf = cl::Buffer(cl_context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, map.size(), map.data(), &err);
	auto results_buf = cl::Buffer(cl_context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * results.size(), results.data(), &err);
	auto inc_buf = cl::Buffer(cl_context, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, sizeof(int), &offs, &err);

	auto kernel = cl::Kernel(cl_find_combination_program, "FindCombination");

	err = kernel.setArg(0, map_buf);
	err = kernel.setArg(1, results_buf);
	err = kernel.setArg(2, inc_buf);

	auto queue = cl::CommandQueue(cl_context, device);

	auto run_err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(total), cl::NullRange/*cl::NDRange(1)*/);
	auto read_err = queue.enqueueReadBuffer(results_buf, CL_TRUE, 0, sizeof(int) * results.size(), results.data());

	cl::finish();
}
#endif
void solver_service_separation_combination_finding::validate_predictions(vector<unsigned char>& m, vector<int>& results, unsigned int min, unsigned int max, mutex* sync) const
{
	for(unsigned int prediction = min; prediction < max; prediction++)
	{
		unsigned char prediction_valid = 1;
		unsigned char empty_pts_count = m[0];
		for(int i = 0; i < empty_pts_count; i++)
		{
			int neighbours_with_mine = 0;
			int header_offset = 1 + i * 9;
			unsigned char header = m[header_offset];
			int neighbour_count = header & 0x0F;
			int hint = header >> 4;
			for(int j = 1; j <= neighbour_count; j++)
			{
				unsigned char neighbour = m[header_offset + j];
				unsigned char flag = neighbour & 0x03;
				switch(flag)
				{
				case 1:
					++neighbours_with_mine;
					break;
				case 2:
					break;
				default:
				{
					unsigned char cell_index = neighbour >> 2;
					unsigned char verdict = (prediction & (1 << cell_index)) > 0;
					if(verdict)
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
				break;
			}
		}

		if(prediction_valid)
		{
			if(sync != nullptr)
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

void solver_service_separation_combination_finding::thr_validate_predictions(vector<unsigned char>& m, vector<int>& results, unsigned int total) const
{
	auto thread_count = thread::hardware_concurrency();
	auto thread_load = total / thread_count;
	auto threads = vector<thread>();
	mutex sync;
	for(unsigned i = 0; i < thread_count; i++)
	{
		unsigned int min = thread_load * i;
		unsigned int max = min + thread_load;
		if(i == thread_count - 1)
		{
			max = total;
		}

		threads.emplace_back([this, &m, &results, min, max, &sync]()
		{
			validate_predictions(m, results, min, max, &sync);
		});
	}

	for(auto& thr : threads)
	{
		thr.join();
	}
}

int solver_service_separation_combination_finding::find_hamming_weight(int i) const
{
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

void solver_service_separation_combination_finding::get_combination_search_map(solver_map& original_map, border& border, vector<unsigned char>& m) const
{
	point_set empty_pts_set;
	auto cell_indices = vector<int>(original_map.cells.size(), -1);
	for(auto i = 0; i < border.cells.size(); i++)
	{
		auto& c = border.cells[i];
		cell_indices[c.pt.x * original_map.height + c.pt.y] = i;
		auto& entry = original_map.neighbour_cache_get(c.pt).by_state[cell_state_empty];
		for(auto& cell : entry)
		{
			empty_pts_set.insert(cell.pt);
		}
	}

	auto empty_pts = vector<int>();
	for(auto& pt : empty_pts_set)
	{
		empty_pts.push_back(pt.x * original_map.height + pt.y);
	}
	
	auto empty_pt_count = static_cast<unsigned char>(empty_pts.size());
	m.reserve(1 + empty_pt_count * 9);
	m.push_back(empty_pt_count);
	for(auto i = 0; i < empty_pts.size(); i++)
	{
		auto& empty_pt = empty_pts[i];
		auto& c = original_map.cells[empty_pt];
		auto& entry = original_map.neighbour_cache_get(c.pt);
		auto& filled_neighbours = entry.by_state[cell_state_filled];

		auto header_byte = static_cast<unsigned char>((c.hint << 4) | filled_neighbours.size());
		m.push_back(header_byte);

		for(auto j = 0; j < 8; j++)
		{
			if(j < filled_neighbours.size())
			{
				auto& neighbour = filled_neighbours[j];
				auto& cell_index = cell_indices[neighbour.pt.x * original_map.height + neighbour.pt.y];
				auto neighbour_byte = static_cast<unsigned char>(cell_index << 2);
				auto flag = neighbour.state & cell_flags;
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

void solver_service_separation_combination_finding::find_valid_border_cell_combinations(solver_map& solver_map, border& border) const
{
	auto border_length = border.cells.size();
	auto total = static_cast<unsigned int>(1 << border_length);
	auto m = vector<unsigned char>();
	auto results = vector<int>();
	get_combination_search_map(solver_map, border, m);
	auto all_remaining_cells_in_border = solver_map.undecided_count == border_length;
	cout << "Border size: " << border_length << endl;
	cout << "All remaining mines in border" << endl;
#ifdef ENABLE_OPEN_CL
	if(settings.valid_combination_search_open_cl && border_length >= settings.valid_combination_search_open_cl_use_from_size)
	{
		results.resize(1024 * 2, -1);
		cl_validate_predictions(m, results, total);
	}
	else
#endif
	if(settings.valid_combination_search_multithread && border_length >= settings.valid_combination_search_multithread_use_from_size)
	{
		thr_validate_predictions(m, results, total);
	}
	else
	{
		validate_predictions(m, results, 0, total, nullptr);
	}

	for(auto& prediction : results)
	{
		if(prediction == -1)
		{
			break;
		}
		if (solver_map.remaining_mine_count > 0)
		{
			auto bits_set = find_hamming_weight(prediction);
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
		for(unsigned int j = 0; j < border_length; j++)
		{
			auto& pt = border.cells[j].pt;
			auto has_mine = (prediction & (1 << j)) > 0;
			predictions[pt] = has_mine;
		}
		border.valid_combinations.push_back(predictions);
	}
}