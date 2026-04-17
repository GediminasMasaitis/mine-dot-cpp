#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <cstdint>
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

// Human-friendly duration formatter: auto-picks us/ms/s.
static std::string format_duration_ns(size_t ns)
{
	std::ostringstream os;
	os << std::fixed << std::setprecision(1);
	if (ns < 1000ULL)
		os << ns << " ns";
	else if (ns < 1000000ULL)
		os << (ns / 1000.0) << " us";
	else if (ns < 1000000000ULL)
		os << (ns / 1000000.0) << " ms";
	else
		os << (ns / 1000000000.0) << " s";
	return os.str();
}

// Print a log-scale histogram of per-game solve times (input is nanoseconds per game).
static void print_histogram(const std::vector<size_t>& durations_ns)
{
	// Bucket boundaries in ns (exclusive upper bound for each bucket except the last).
	struct bucket { size_t upper_ns; const char* label; };
	static const bucket buckets[] = {
		{ 1000000ULL,       "    < 1 ms" },
		{ 2000000ULL,       "  1 -   2 ms" },
		{ 5000000ULL,       "  2 -   5 ms" },
		{ 10000000ULL,      "  5 -  10 ms" },
		{ 25000000ULL,      " 10 -  25 ms" },
		{ 50000000ULL,      " 25 -  50 ms" },
		{ 100000000ULL,     " 50 - 100 ms" },
		{ 250000000ULL,     "100 - 250 ms" },
		{ 500000000ULL,     "250 - 500 ms" },
		{ 1000000000ULL,    "500 ms - 1 s" },
		{ 5000000000ULL,    "  1 -   5 s" },
		{ SIZE_MAX,         "    > 5 s" },
	};
	constexpr size_t bucket_count = sizeof(buckets) / sizeof(buckets[0]);

	std::vector<size_t> counts(bucket_count, 0);
	for (auto ns : durations_ns)
	{
		for (size_t b = 0; b < bucket_count; b++)
		{
			if (ns < buckets[b].upper_ns)
			{
				counts[b]++;
				break;
			}
		}
	}

	size_t max_count = 0;
	for (auto c : counts) if (c > max_count) max_count = c;
	constexpr int bar_width = 40;
	const auto total = durations_ns.size();

	cout << "Histogram:" << endl;
	for (size_t b = 0; b < bucket_count; b++)
	{
		const int bar_len = max_count > 0 ? static_cast<int>((counts[b] * bar_width) / max_count) : 0;
		const double pct = total > 0 ? (100.0 * counts[b] / total) : 0.0;
		cout << "  " << buckets[b].label << "  " << std::string(bar_len, '#');
		for (int i = bar_len; i < bar_width; i++) cout << ' ';
		cout << "  " << counts[b] << " (";
		cout << std::fixed << std::setprecision(1) << pct << "%)" << endl;
	}
}

struct benchmark_summary
{
	std::string label;
	int count;
	size_t total_wall_ns;
	size_t avg_ns;
	size_t p50_ns;
	size_t p90_ns;
	size_t p99_ns;
	size_t max_ns;
	double success_rate;
};

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

	std::vector<benchmark_summary> summaries;

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
		settings.partial_solve_only_when_giving_up = false;
		settings.print_trace = true;

		auto solvr = solver(settings);

		auto mineCount = 99;
		auto group = minedotcpp::benchmarking::benchmark_density_group();
		auto count = 1000;
		const auto width = 30;
		const auto heigth = 16;

		auto wall_start = std::chrono::high_resolution_clock::now();
		benchmarker.benchmark_multiple(solvr, width, heigth, mineCount, count, group);
		auto wall_end = std::chrono::high_resolution_clock::now();
		auto wall_ns = static_cast<size_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(wall_end - wall_start).count());

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
		const auto avg_ns = sum / count;
		auto pct_ns = [&](double p) { return durations[static_cast<size_t>(durations.size() * p)]; };

		size_t p50 = pct_ns(0.50);
		size_t p90 = pct_ns(0.90);
		size_t p99 = pct_ns(0.99);
		size_t max_ns = durations.back();

		cout << endl << "--- Summary: " << config.label << " ---" << endl;
		cout << "Games:        " << count << endl;
		cout << "Density:      " << group.density << endl;
		cout << "Success rate: " << std::fixed << std::setprecision(1) << success_rate << "%"
			 << " (" << success_count << "/" << count << ")" << endl;
		cout << "Total wall:   " << format_duration_ns(wall_ns) << endl;
		cout << "Total solve:  " << format_duration_ns(sum) << endl;
		cout << "Avg:          " << format_duration_ns(avg_ns) << endl;
		cout << "p50:          " << format_duration_ns(p50) << endl;
		cout << "p90:          " << format_duration_ns(p90) << endl;
		cout << "p99:          " << format_duration_ns(p99) << endl;
		cout << "Max:          " << format_duration_ns(max_ns) << endl;
		cout << endl;
		print_histogram(durations);

		summaries.push_back({
			config.label, count, wall_ns, avg_ns, p50, p90, p99, max_ns, success_rate
		});
	}

	if (summaries.size() > 1)
	{
		cout << endl << "=== Comparison ===" << endl;
		cout << std::left
			 << std::setw(42) << "Config"
			 << std::right
			 << std::setw(10) << "Success"
			 << std::setw(12) << "Avg"
			 << std::setw(12) << "p50"
			 << std::setw(12) << "p90"
			 << std::setw(12) << "p99"
			 << std::setw(14) << "Max"
			 << std::setw(14) << "Total wall"
			 << endl;
		for (auto& s : summaries)
		{
			cout << std::left
				 << std::setw(42) << s.label
				 << std::right
				 << std::setw(9) << std::fixed << std::setprecision(1) << s.success_rate << "%"
				 << std::setw(12) << format_duration_ns(s.avg_ns)
				 << std::setw(12) << format_duration_ns(s.p50_ns)
				 << std::setw(12) << format_duration_ns(s.p90_ns)
				 << std::setw(12) << format_duration_ns(s.p99_ns)
				 << std::setw(14) << format_duration_ns(s.max_ns)
				 << std::setw(14) << format_duration_ns(s.total_wall_ns)
				 << endl;
		}
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

