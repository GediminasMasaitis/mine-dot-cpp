#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include "mapio/text_map_parser.h"
#include "mapio/text_map_visualizer.h"
#include "solvers/solver.h"
#include <chrono>
#include <iomanip>

#include "debug/debugging.h"
#include "benchmarking/benchmarker.h"
#include "global_wrappers.h"
#ifdef _WIN32
#include <windows.h>
#endif


using namespace std;
using namespace minedotcpp::common;
using namespace minedotcpp::solvers;

string resolve_path(const string& path)
{
	if (filesystem::exists(path))
	{
		return path;
	}
#ifdef __linux__
	// Try WSL mount: convert "C:/foo" or "C:\foo" to "/mnt/c/foo"
	if (path.size() >= 2 && path[1] == ':')
	{
		string wsl_path = "/mnt/" + string(1, tolower(path[0])) + "/" + path.substr(2);
		for (auto& c : wsl_path)
		{
			if (c == '\\') c = '/';
		}
		if (filesystem::exists(wsl_path))
		{
			return wsl_path;
		}
	}
#endif
	return path;
}

#ifdef ENABLE_OPEN_CL
void list_open_cl_devices()
{
	auto platforms = vector<cl::Platform>();
	cl::Platform::get(&platforms);
	for(auto i = 0; i < platforms.size(); ++i)
	{
		auto& platform = platforms[i];
		string platform_name;
		string platform_vendor;
		platform.getInfo(CL_PLATFORM_NAME, &platform_name);
		platform.getInfo(CL_PLATFORM_VENDOR, &platform_vendor);

		cout << i << ": " << platform_vendor << " " << platform_name << endl;

		auto devices = vector<cl::Device>();
		platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

		for(auto j = 0; j < devices.size(); ++j)
		{
			auto& device = devices[j];
			string device_name;
			device.getInfo(CL_DEVICE_NAME, &device_name);
			cout << "  " << j << ": " << device_name << endl;
		}
	}
}
#endif

void solve_from_file(int argc, char* argv[])
{
	minedotcpp::mapio::text_map_parser parser;
	string path = "C:/mine/map.txt";
	map test_map;
	if(argc > 1)
	{
		string cmd(argv[1]);
		if(cmd == "--path")
		{
			if(argc > 2)
			{
				path = string(argv[2]);
			}
			else
			{
				cout << "No path supplied" << endl;
				return;
			}
		}
		else if(cmd == "--map")
		{
			if(argc > 2)
			{
				parser.parse(string(argv[2]), test_map);
			}
			else
			{
				cout << "No map supplied" << endl;
				return;
			}
		}
	}

	if(test_map.width == -1 && test_map.height == -1)
	{
		path = resolve_path(path);
		ifstream strm(path);
		parser.parse(strm, test_map);
		strm.close();
	}

	solver_settings settings;
	settings.trivial_solve = false;
	settings.gaussian_solve = false;
	//settings.gaussian_all_stop_always = true;
	//settings.valid_combination_search_open_cl = false;
	//settings.debug_setting_1 = false;
	settings.partial_solve = false;
	settings.mine_count_solve = false;
	settings.give_up_from_size = 100;
	settings.separation_solve = false;
	settings.valid_combination_search_multithread = true;
	settings.valid_combination_search_open_cl = true;
	settings.valid_combination_search_open_cl_allow_loop_break = false;
	settings.valid_combination_search_open_cl_platform_id = 0;
	settings.valid_combination_search_open_cl_device_id = 0;

	//settings.mine_count_ignore_completely = true;
	//settings.guess_if_no_no_mine_verdict = false;
	//settings.give_up_from_size = 25;
	init_solver(settings);
	std::chrono::high_resolution_clock clock;
	auto results = vector<solver_result>(1024);
	auto results_size = static_cast<int>(results.size());
	auto visualizer = minedotcpp::mapio::text_map_visualizer();
	auto str = visualizer.visualize_to_string(test_map);
	auto start_time = clock.now();
	solve(str.c_str(), results.data(), &results_size);
	results.resize(results_size);
	auto end_time = clock.now();
	auto diff = end_time - start_time;

	cout << endl << "That took " << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() << " ms" << endl << endl;
	dump_results(results);
	visualize(test_map, results, false);
}

auto total_iterations = 0;

void on_iteration_impl(int benchmark_index, map& m, point_map<solver_result>& results, int iteration, size_t duration)
{
	//printf("Map: %5i, Total iterations: %7i, Iteration: %3i;  Time taken: %5i us\n", benchmark_index, total_iterations++, iteration, duration);
	//visualize(m, results, false);
}

void on_end_impl(minedotcpp::benchmarking::benchmark_entry& entry)
{
	printf("Map: %5i, Success: %s, Time taken: %8i us\n", entry.map_index, entry.solved ? "Y" : "N", static_cast<int>(entry.total_duration / 1000));
}

void benchmark()
{
	struct benchmark_config
	{
		const char* label;
		bool gaussian_reduction;
		bool gaussian_backtracking;
	};

	benchmark_config configs[] = {
		//{"WITHOUT gaussian reduction",              false, false},
		{"WITH gaussian reduction (flat)",           true, false},
		{"WITH gaussian reduction (backtracking)",   true,  true},
	};

	for (auto& config : configs)
	{
		cout << endl << "=== Benchmark: " << config.label << " ===" << endl;

		auto mt = std::mt19937(0);
		auto benchmarker = minedotcpp::benchmarking::benchmarker(mt);
		benchmarker.on_iteration = on_iteration_impl;
		benchmarker.on_end = on_end_impl;

		auto settings = solver_settings();
		settings.trivial_solve = true;
		settings.gaussian_solve = true;
		settings.separation_solve = true;
		settings.partial_solve = true;
		settings.mine_count_solve = true;
		settings.mine_count_solve_non_border = true;
		settings.give_up_from_size = 28;
		settings.partial_solve_from_size = 24;
		settings.partial_optimal_size = 24;
		settings.valid_combination_search_multithread = true;
		settings.valid_combination_search_open_cl = false;
		settings.valid_combination_search_open_cl_allow_loop_break = false;
		settings.combination_search_gaussian_reduction = config.gaussian_reduction;
		settings.combination_search_gaussian_backtracking = config.gaussian_backtracking;
		settings.print_trace = true;

		auto solvr = solver(settings);

		auto mineCount = 99;
		auto group = minedotcpp::benchmarking::benchmark_density_group();
		auto count = 1000;
		const auto width = 30;
		const auto heigth = 16;
		benchmarker.benchmark_multiple(solvr, width, heigth, mineCount, count, group);
		cout << "Density: " << group.density << endl;
		size_t sum = 0;
		auto success_count = 0;
		std::vector<size_t> durations;
		durations.reserve(group.entries.size());
		for (auto& entry : group.entries)
		{
			sum += entry.total_duration;
			durations.push_back(entry.total_duration);
			if (entry.solved)
			{
				success_count++;
			}
		}
		std::sort(durations.begin(), durations.end());
		auto success_rate = (static_cast<double>(success_count) / count) * 100;
		const auto avg = (sum / count);
		auto pct = [&](double p) { return durations[static_cast<size_t>(durations.size() * p)] / 1000; };
		cout << "Avg:  " << (avg / 1000) << " us" << endl;
		cout << "p50:  " << pct(0.50) << " us" << endl;
		cout << "p90:  " << pct(0.90) << " us" << endl;
		cout << "p99:  " << pct(0.99) << " us" << endl;
		cout << "Max:  " << (durations.back() / 1000) << " us" << endl;
		cout << "Success rate: " << success_rate << "%" << endl;
	}
}


void test_global_api()
{
	solver_settings s;
	s.gaussian_solve = false;
	//s.give_up_from_size = 15;
	cout << sizeof(solver_result) << endl;
	init_solver(s);
	auto map_str = R"(###2###1
########
##3211##
##1..2##
##1..2##
2#2113##
########
###2###2)";
	auto results_size = 1024;
	auto results = vector<solver_result>(results_size);
	solve(map_str, results.data(), &results_size);
	results.resize(results_size);
	dump_results(results);

	map m;
	minedotcpp::mapio::text_map_parser().parse(map_str, m);
	visualize(m, results, false);

}

int main(int argc, char* argv[])
{
#ifdef _WIN32
	auto wh = GetConsoleWindow();
	if (wh != NULL)
	{
		COORD coord = {300,1000};
		auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleScreenBufferSize(handle, coord);
		RECT rect = {};
		GetWindowRect(wh, &rect);
		MoveWindow(wh, rect.left, 50, 1300, 950, TRUE);
	}
#endif
#ifdef ENABLE_OPEN_CL
	list_open_cl_devices();
#endif
	if (argc > 1 && string(argv[1]) == "--benchmark")
	{
		benchmark();
	}
	else
	{
		solve_from_file(argc, argv);
	}
	cout << "Press any key to continue" << endl;
	getc(stdin);
	return 0;
}

