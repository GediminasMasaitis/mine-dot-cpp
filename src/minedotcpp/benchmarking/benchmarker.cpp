#include "benchmarker.h"
#include "../game/game_engine.h"
#include <chrono>
#include "../debug/debugging.h"

void minedotcpp::benchmarking::benchmarker::benchmark_multiple(solvers::solver& s, int width, int height, int mine_count, int tests_to_run, benchmark_density_group& group)
{
	group.density = static_cast<double>(mine_count) / (width * height);
	for(auto i = 0; i < tests_to_run; i++)
	{
		auto entry = benchmark_entry();
		entry.map_index = i;
		benchmark(i, s, width, height, mine_count, entry);
		group.entries.push_back(entry);
	}
}

void minedotcpp::benchmarking::benchmarker::benchmark(int benchmark_index, solvers::solver& s, int width, int height, int mine_count, benchmark_entry& entry)
{
	s.ResetTable();
	common::point starting_pt = {0,0};
	auto engine = game::game_engine(generator);
	auto initial_result = engine.start_with_mine_count(width, height, starting_pt, false, mine_count);
	if(initial_result == game::game_won)
	{
		entry.solved = true;
		if (on_end != nullptr)
		{
			on_end(entry);
		}
		return;
	}
	
	auto clock = std::chrono::high_resolution_clock();
	auto iteration = 0;
	while (true)
	{
		auto m = common::map();
		engine.gm.to_regular_map(m);
		auto results = common::point_map<solvers::solver_result>();
		auto start_time = clock.now();
		s.solve(m, results);
		auto end_time = clock.now();
		auto diff = end_time - start_time;
		auto ns = static_cast<size_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count());
		if (on_iteration != nullptr)
		{
			on_iteration(benchmark_index, m, results, iteration++, ns);
		}

		entry.solving_durations.push_back(ns);
		entry.total_duration += ns;

		for (auto& res : results)
		{
			switch (res.second.verdict)
			{
			case solvers::verdict_has_mine:
				engine.set_flag(res.first, common::cell_flag_has_mine);
				break;
			case solvers::verdict_doesnt_have_mine:
				{
					auto game_res = engine.open_cell(res.first);
					switch (game_res)
					{
					case game::game_won:
						entry.solved = true;
						if(on_end != nullptr)
						{
							on_end(entry);
						}
						return;
					case game::game_lost:
						entry.solved = false;
						if (on_end != nullptr)
						{
							on_end(entry);
						}
						return;
					default:
						break;
					}
				}
				break;
			default:
				break;
			}
		}
	}
}
