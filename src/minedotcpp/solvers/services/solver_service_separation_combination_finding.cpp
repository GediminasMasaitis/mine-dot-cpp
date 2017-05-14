#include "../solver_map.h"
#include "../border.h"
#include "../../debug/debugging.h"
#include <thread>
#include <mutex>
#include <queue>
#include "solver_service_separation_combination_finding.h"

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
	auto prediction_valid = is_prediction_valid(map, border, combo, empty_cells, cell_indices);\
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

bool solver_service_separation_combination_finding::is_prediction_valid(const solver_map& map, const border& b, unsigned int prediction, const vector<cell>& empty_cells, const CELL_INDICES_T& cell_indices) const
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