#include "solver.h"
#include "solver_map.h"
#include "services/trivial/solver_service_trivial.h"
#include "services/separation/solver_service_separation.h"
#include "services/guessing/solver_service_guessing.h"
#include "services/gaussian/solver_service_gaussian.h"
#include <chrono>
#include <cstdio>

using namespace minedotcpp;
using namespace solvers;
using namespace services;
using namespace common;
using namespace std;

void solver::solve(const map& base_map, point_map<solver_result>& results) const
{
	auto t_start = settings.print_trace ? chrono::high_resolution_clock::now() : chrono::high_resolution_clock::time_point{};

	solver_map m;
	m.init_from(base_map);
	if(settings.mine_count_ignore_completely)
	{
		m.remaining_mine_count = -1;
	}

	point_map<double> probabilities;
	point_map<bool> verdicts;

	auto t_init = settings.print_trace ? chrono::high_resolution_clock::now() : chrono::high_resolution_clock::time_point{};

	if(settings.trivial_solve)
	{
		auto trivial_service = solver_service_trivial(settings, thr_pool);
		trivial_service.solve_trivial(m, verdicts);
		if(should_stop_solving(verdicts, settings.trivial_stop_on_no_mine_verdict, settings.trivial_stop_on_any_verdict, settings.trivial_stop_always))
		{
			get_final_results(m, probabilities, verdicts, results);
			return;
		}
	}

	auto t_trivial = settings.print_trace ? chrono::high_resolution_clock::now() : chrono::high_resolution_clock::time_point{};

	if(settings.gaussian_solve)
	{
		auto gaussian_service = solver_service_gaussian(settings, thr_pool);
		gaussian_service.solve_gaussian(m, verdicts);
		if(should_stop_solving(verdicts, settings.gaussian_all_stop_on_no_mine_verdict, settings.gaussian_all_stop_on_any_verdict, settings.gaussian_all_stop_always))
		{
			get_final_results(m, probabilities, verdicts, results);
			return;
		}
	}

	auto t_gauss = settings.print_trace ? chrono::high_resolution_clock::now() : chrono::high_resolution_clock::time_point{};

	if (settings.separation_solve)
	{
		separation_service.solve_separation(m, probabilities, verdicts);
	}

	auto t_sep = settings.print_trace ? chrono::high_resolution_clock::now() : chrono::high_resolution_clock::time_point{};

	get_final_results(m, probabilities, verdicts, results);

	if (settings.print_trace)
	{
		auto t_end = chrono::high_resolution_clock::now();
		auto total_us = chrono::duration_cast<chrono::microseconds>(t_end - t_start).count();
		// Gate top-level solve() trace on duration. By the time we get here, the
		// thread-local trace gate used by solve_border has already been reset,
		// so we can't use that; duration is a reasonable proxy. Threshold is
		// configurable via settings.print_trace_min_solve_us (set to 0 to print
		// every solve).
		if (total_us >= settings.print_trace_min_solve_us)
		{
			auto init_us = chrono::duration_cast<chrono::microseconds>(t_init - t_start).count();
			auto trivial_us = chrono::duration_cast<chrono::microseconds>(t_trivial - t_init).count();
			auto gauss_us = chrono::duration_cast<chrono::microseconds>(t_gauss - t_trivial).count();
			auto sep_us = chrono::duration_cast<chrono::microseconds>(t_sep - t_gauss).count();
			auto final_us = chrono::duration_cast<chrono::microseconds>(t_end - t_sep).count();
			printf("[trace] solve() total=%lldus: init=%lldus, trivial=%lldus, gaussian=%lldus, separation=%lldus, final=%lldus\n",
				static_cast<long long>(total_us),
				static_cast<long long>(init_us),
				static_cast<long long>(trivial_us),
				static_cast<long long>(gauss_us),
				static_cast<long long>(sep_us),
				static_cast<long long>(final_us));
		}
	}
}

void solver::get_final_results(solver_map& m, point_map<double>& probabilities, point_map<bool>& verdicts, point_map<solver_result>& results) const
{
	auto guessing_service = solver_service_guessing(settings, thr_pool);
	point guessed_pt;
	auto guessed = guessing_service.guess_verdict(m, probabilities, verdicts, guessed_pt);
	for (auto& probability : probabilities)
	{
		auto& result = results[probability.first];
		result.pt = probability.first;
		result.probability = probability.second;
		result.verdict = verdict_none;
	}
	for (auto& verdict : verdicts)
	{
		auto& result = results[verdict.first];
		result.pt = verdict.first;
		if (!guessed || guessed_pt != result.pt)
		{
			result.probability = verdict.second ? 1 : 0;
		}
		result.verdict = verdict.second ? verdict_has_mine : verdict_doesnt_have_mine;
	}
}