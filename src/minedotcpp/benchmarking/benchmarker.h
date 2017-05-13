#pragma once
#include "../solvers/solver.h"
#include <random>
#include "../game/game_map_generator.h"
#include "benchmark_entry.h"
#include <chrono>
#include "../game/game_engine.h"
#include "benchmark_density_group.h"

namespace minedotcpp
{
	namespace benchmarking
	{
		class benchmarker
		{
		public:

			//std::mt19937 random_engine;
			game::game_map_generator generator;

			benchmarker(std::mt19937& mt)
			{
				generator.random_engine = &mt;
			}

			void benchmark_multiple(solvers::solver& s, int width, int height, double density, int tests_to_run, benchmark_density_group& group)
			{
				group.density = density;
				for(auto i = 0; i < tests_to_run; i++)
				{
					auto entry = benchmark_entry();
					benchmark(s, width, height, density, entry);
					group.entries.push_back(entry);
				}
			}

			void benchmark(solvers::solver& s, int width, int height, double density, benchmark_entry& entry)
			{
				common::point starting_pt = { width >> 1, height >> 1 };
				auto engine = game::game_engine(generator);
				engine.start_new(width, height, starting_pt, true, density);
				auto clock = std::chrono::high_resolution_clock();
				auto iteration = 0;
				while(true)
				{
					auto m = common::map();
					engine.gm.to_regular_map(m);
					printf("Iteration: %i\n", iteration++);
					visualize(m, std::vector<std::vector<common::point>>(), false);
					auto results = common::point_map<solvers::solver_result>();
					auto start_time = clock.now();
					s.solve(m, results);
					auto end_time = clock.now();
					auto diff = end_time - start_time;
					auto ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(diff).count());
					entry.solving_durations.push_back(ms);
					entry.total_duration += ms;

					for(auto& res : results)
					{
						switch(res.second.verdict)
						{
						case solvers::verdict_has_mine:
							engine.set_flag(res.first, common::cell_flag_has_mine);
							break;
						case solvers::verdict_doesnt_have_mine:
						{
							auto game_res = engine.open_cell(res.first);
							switch(game_res)
							{
							case game::game_won:
								entry.solved = true;
								return;
							case game::game_lost:
								entry.solved = false;
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
		};
	}
}
