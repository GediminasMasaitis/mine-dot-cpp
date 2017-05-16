#include "../solver_map.h"
#include "../border.h"
#include "../../debug/debugging.h"
#include <thread>
#include <mutex>
#include <queue>
#include "solver_service_separation_combination_finding.h"
#include <CL/cl.hpp>

using namespace minedotcpp;
using namespace solvers;
using namespace services;
using namespace common;
using namespace std;

void solver_service_separation_combination_finding::thr_find_combos(const solver_map& map, border& border, unsigned int min, unsigned int max, const vector<cell>& empty_cells, const CELL_INDICES_T& cell_indices, mutex& sync) const
{
	auto border_length = border.cells.size();
	auto all_remaining_cells_in_border = map.undecided_count == border_length;
	// TODO: Macro because calling the thread function here directly causes it to slow down about 50% for some reason. Figure out why
#define FIND_COMBOS_BODY(lck) \
for(unsigned int combo = min; combo < max; combo++)\
{\
	if(map.remaining_mine_count > 0)\
	{\
		auto bits_set = SWAR(combo);\
		if(bits_set > map.remaining_mine_count)\
		{\
			continue;\
		}\
		if(all_remaining_cells_in_border && bits_set != map.remaining_mine_count)\
		{\
			continue;\
		}\
	}\
\
	auto prediction_valid = is_prediction_valid(map, combo, empty_cells, cell_indices);\
	if(prediction_valid)\
	{\
		point_map<bool> predictions;\
		predictions.resize(border_length);\
		for(unsigned int j = 0; j < border_length; j++)\
		{\
			auto& pt = border.cells[j].pt;\
			auto has_mine = (combo & (1 << j)) > 0;\
			predictions[pt] = has_mine;\
		}\
		lck;\
		border.valid_combinations.push_back(predictions);\
	}\
}
	FIND_COMBOS_BODY(std::lock_guard<std::mutex> block(sync))
}

void solver_service_separation_combination_finding::find_valid_border_cell_combinations(solver_map& map, border& border) const
{
	auto border_length = border.cells.size();

	const int max_size = 31;
	if(border_length > max_size)
	{
		// TODO: handle too big border
		//throw new InvalidDataException($"Border with {borderLength} cells is too large, maximum {maxSize} cells allowed");
	}
	unsigned int total_combos = 1 << border_length;


	point_set empty_pts;
	//border.cell_indices.resize(border.cells.size());
	CELL_INDICES_T cell_indices;
	CELL_INDICES_RESIZE(cell_indices, border, map);
	//border.cell_indices.resize(map.width * map.height);
	for(auto i = 0; i < border.cells.size(); i++)
	{
		auto& c = border.cells[i];
		CELL_INDICES_ELEMENT(cell_indices, c.pt, map) = i;
		//border.cell_indices[c.pt] = i;
		auto& entry = map.neighbour_cache_get(c.pt).by_state[cell_state_empty];
		for(auto& cell : entry)
		{
			empty_pts.insert(cell.pt);
		}
	}

	auto all_remaining_cells_in_border = map.undecided_count == border_length;
	vector<cell> empty_cells;
	empty_cells.reserve(empty_pts.size());
	for(auto& pt : empty_pts)
	{
		empty_cells.push_back(map.cell_get(pt));
	}

	auto thread_count = thread::hardware_concurrency();
	if(border_length > settings.multithread_valid_combination_search_from_size && thread_count > 1)
	{
		mutex sync;
		auto thread_load = total_combos / thread_count;
		vector<thread> threads;
		for(unsigned i = 0; i < thread_count; i++)
		{
			unsigned int min = thread_load * i;
			unsigned int max = min + thread_load;
			if(i == thread_count - 1)
			{
				max = total_combos;
			}
			threads.emplace_back([this, &map, &border, min, max, &empty_cells, &cell_indices, &sync]()
			{
				thr_find_combos(map, border, min, max, empty_cells, cell_indices, sync);
			});
			//thr_find_combos(map, border, min, max, empty_cells, cell_indices, sync);
		}

		for(auto& thr : threads)
		{
			thr.join();
		}
	}
	else
	{
		unsigned int min = 0;
		unsigned int max = total_combos;
		//thr_find_combos(map, border, 0, total_combos, empty_cells, cell_indices, sync);
		FIND_COMBOS_BODY(;);
	}
}

int solver_service_separation_combination_finding::SWAR(int i) const
{
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

bool solver_service_separation_combination_finding::is_prediction_valid(const solver_map& map, unsigned int prediction, const vector<cell>& empty_cells, const CELL_INDICES_T& cell_indices) const
{
	for(auto& cell : empty_cells)
	{
		auto neighbours_with_mine = 0;
		auto& filled_neighbours = map.neighbour_cache[cell.pt.x * map.height + cell.pt.y].by_state[cell_state_filled];
		for(auto& neighbour : filled_neighbours)
		{
			auto flag = neighbour.state & cell_flags;
			switch(flag)
			{
			case cell_flag_has_mine:
				++neighbours_with_mine;
				break;
			case cell_flag_doesnt_have_mine:
				break;
			default:
				unsigned int i = CELL_INDICES_ELEMENT(cell_indices, neighbour.pt, map);
				auto verdict = (prediction & (1 << i)) > 0;
				if(verdict)
				{
					++neighbours_with_mine;
				}
				break;
			}
		}

		if(neighbours_with_mine != cell.hint)
		{
			return false;
		}
	}
	return true;
}

void do_open_cl_things(vector<unsigned char>& map, vector<int>& empty_pts, vector<int>& results, int max_prediction)
{
	auto platforms = vector<cl::Platform>();
	cl::Device device;
	cl::Platform::get(&platforms);

	for(auto i = 0; i < platforms.size(); ++i)
	{
		auto& platform = platforms[i];
		auto platform_name = platform.getInfo<CL_PLATFORM_NAME>();
		auto platform_version = platform.getInfo<CL_PLATFORM_VERSION>();
		cout << "Platform " << i << endl << platform_name << endl << platform_version << endl << endl;
		auto devices = vector<cl::Device>();
		platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

		for(auto j = 0; j < devices.size(); ++j)
		{
			auto& dev = devices[j];
			if(i == 0 && j == 1)
			{
				device = dev;
			}
			auto device_type = dev.getInfo<CL_DEVICE_TYPE>();
			string device_type_str;
			switch(device_type)
			{
			case CL_DEVICE_TYPE_CPU:
				device_type_str = "CPU";
				break;
			case CL_DEVICE_TYPE_GPU:
				device_type_str = "GPU";
				break;
			case CL_DEVICE_TYPE_ACCELERATOR:
				device_type_str = "ACCELERATOR";
				break;
			default:
				device_type_str = device_type;
			}
			cout << "  Device " << j << ":" << endl;
			cout << "  " << device_type_str << endl;
			cout << "  " << dev.getInfo<CL_DEVICE_NAME>() << endl;
			cout << "  " << dev.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() / (1024 * 1024) << " MB total memory" << endl;
			cout << "  " << dev.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() << " B local memory" << endl;
			cout << "  " << dev.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << " max compute units" << endl;
			cout << "  " << dev.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() << " MHz max clock frequency" << endl;
			cout << endl;
		}

		cout << "-------------------------------" << endl << endl;
	}

	cout << "Selected: " << device.getInfo<CL_DEVICE_NAME>() << endl;

	string cl_code = R"(

//#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

__kernel void ArrayStuff(__global unsigned char* restrict map, __global int* restrict empty_pts, __global int* restrict results, __global int* increment)
{
	//results[10] = get_local_size(0);

	size_t local_id = get_local_id(0);
	size_t global_id = get_global_id(0);
	size_t group_id = get_group_id(0);
	
	int prediction = global_id;
	
	unsigned char prediction_valid = 1;
	for(int i = 0; i < 32; i++)
	{
		int empty_pt = empty_pts[i];
		if(empty_pt == -1)
		{
			break;
		}
		
		int neighbours_with_mine = 0;
		int header_offset = empty_pt * 9;
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
		
		/*if(prediction == 1 && i == 1)
		{
			results[prediction * 8] = prediction;
			results[prediction * 8 + 1] = neighbours_with_mine;
			results[prediction * 8 + 2] = hint;
			results[prediction * 8 + 3] = i;
		}*/

		if(neighbours_with_mine != hint)
		{
			prediction_valid = 0;
			break;
		}
	}
	
	if(prediction_valid)
	{
		results[atomic_inc(increment)] = prediction;
	}

	//results[global_id] = global_id;
}
	
	)";

	auto sources = cl::Program::Sources(1, std::make_pair(cl_code.c_str(), cl_code.length() + 1));
	auto context = cl::Context(device);
	auto program = cl::Program(context, sources);

	cl_int err = 0;
	err = program.build("-cl-std=CL1.2");

	auto offs = 0;
	auto map_buf = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, map.size(), map.data(), &err);
	auto empty_pts_buf = cl::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, sizeof(int) * empty_pts.size(), empty_pts.data(), &err);
	auto results_buf = cl::Buffer(context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * results.size(), results.data(), &err);
	auto inc_buf = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, sizeof(int), &offs, &err);

	auto kernel = cl::Kernel(program, "ArrayStuff");

	err = kernel.setArg(0, map_buf);
	err = kernel.setArg(1, empty_pts_buf);
	err = kernel.setArg(2, results_buf);
	err = kernel.setArg(3, inc_buf);

	auto queue = cl::CommandQueue(context, device);

	auto run_err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(max_prediction), cl::NullRange);
	auto read_err = queue.enqueueReadBuffer(results_buf, CL_TRUE, 0, sizeof(int) * results.size(), results.data());

	cl::finish();
}

void solver_service_separation_combination_finding::cl_find_valid_border_cell_combinations(solver_map& original_map, border& border) const
{
	auto border_length = border.cells.size();
	unsigned int total_combos = 1 << border_length;

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
	for(auto i = empty_pts.size(); i < 32; i++)
	{
		empty_pts.push_back(-1);
	}

	auto map = vector<unsigned char>();
	map.reserve(original_map.cells.size() * 9);
	for(auto i = 0; i < original_map.cells.size(); i++)
	{
		auto& c = original_map.cells[i];
		auto& entry = original_map.neighbour_cache_get(c.pt);
		auto& filled_neighbours = entry.by_state[cell_state_filled];

		//assert(filled_neighbours.size() != 0);

		auto header_byte = static_cast<unsigned char>((c.hint << 4) | filled_neighbours.size());
		map.push_back(header_byte);

		for(auto j = 0; j < 8; j++)
		{
			if(j < filled_neighbours.size())
			{
				auto& neighbour = filled_neighbours[j];
				auto& cell_index = cell_indices[neighbour.pt.x * original_map.height + neighbour.pt.y];
				auto neighbour_byte = static_cast<unsigned char>(cell_index << 2);
				auto flag = neighbour.state & cell_flags;
				neighbour_byte |= flag >> 2;
				map.push_back(neighbour_byte);
			}
			else
			{
				map.push_back(0xFF);
			}
		}
	}

	auto min = 0;
	auto max = total_combos;

	auto results = vector<int>(1024*16,-1);

	do_open_cl_things(map, empty_pts, results, max);

	for(auto& prediction : results)
	{
		if(prediction == -1)
		{
			break;
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
	return;
	for(unsigned int prediction = min; prediction < max; prediction++)
	{
		//bool prediction_valid = cl_is_prediction_valid_fake(map, empty_pts, prediction);
		unsigned char prediction_valid = 1;
		for(int i = 0; i < 32; i++)
		{
			int empty_pt = empty_pts[i];
			if(empty_pt == -1)
			{
				break;
			}
			int neighbours_with_mine = 0;
			int header_offset = empty_pt * 9;
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
				break;
			}
		}
		
		if(prediction_valid)
		{
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
}

bool solver_service_separation_combination_finding::cl_is_prediction_valid_fake(const vector<unsigned char>& map, const vector<int>& empty_pts, unsigned int prediction) const
{
	for(int i = 0; i < 32; i++)
	{
		int pt = empty_pts[i];
		if(pt == -1)
		{
			break;
		}
		auto neighbours_with_mine = 0;
		auto header_offset = pt * 9;
		auto& header = map[header_offset];
		auto neighbour_count = header & 0x0F;
		auto hint = header >> 4;
		for(auto j = 1; j <= neighbour_count; j++)
		{
			auto& neighbour = map[header_offset + j];
			auto flag = neighbour & 0x03;
			switch(flag)
			{
			case cell_flag_has_mine >> 2:
				++neighbours_with_mine;
				break;
			case cell_flag_doesnt_have_mine >> 2:
				break;
			default:
				unsigned int cell_index = neighbour >> 2;
				auto verdict = (prediction & (1 << cell_index)) > 0;
				if(verdict)
				{
					++neighbours_with_mine;
				}
				break;
			}
		}

		if(neighbours_with_mine != hint)
		{
			return false;
		}
	}
	return true;
}