#pragma once
#include "../solvers/solver.h"
#include "../game/game_map_generator.h"
#include "benchmark_density_group.h"

namespace minedotcpp
{
	namespace benchmarking
	{
		class MINE_API benchmarker
		{
		public:

			game::game_map_generator generator;

			void (*on_iteration)(int benchmark_index, common::map& m, common::point_map<solvers::solver_result>& results, int iteration, size_t duration) = nullptr;
			void (*on_end)(benchmark_entry& entry) = nullptr;

			explicit benchmarker(std::mt19937& mt)
			{
				generator.random_engine = &mt;
			}

			void benchmark_multiple(solvers::solver& s, int width, int height, int mine_count, int tests_to_run, benchmark_density_group& group);
			void benchmark(int benchmark_index, solvers::solver& s, int width, int height, int mine_count, benchmark_entry& entry);
		};


	}
}
