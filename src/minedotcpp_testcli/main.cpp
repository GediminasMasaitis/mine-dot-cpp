#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include "mapio/text_map_parser.h"
#include "mapio/text_map_visualizer.h"
#include "solvers/solver.h"
#include <chrono>
#include "debug/debugging.h"
#include "benchmarking/benchmarker.h"
#include "global_wrappers.h"
#ifdef _WIN32
#include <windows.h>
#endif


using namespace std;
using namespace minedotcpp::common;
using namespace minedotcpp::solvers;

void solve_from_file(int argc, char* argv[])
{
	minedotcpp::mapio::text_map_parser parser;
	//minedotcpp::mapio::text_map_visualizer visualizer;
	string path = "C:/Temp/test_map.txt";
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
		ifstream strm(path);
		parser.parse(strm, test_map);
		strm.close();
	}

	solver_settings settings;
	solver s(settings);
	std::chrono::high_resolution_clock clock;
	auto results = point_map<solver_result>();
	auto start_time = clock.now();
	s.solve(test_map, results);
	auto end_time = clock.now();
	auto diff = end_time - start_time;

	cout << endl << "That took " << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() << " ms" << endl << endl;
	dump_results(results);
	visualize(test_map, results, false);
}

auto total_iterations = 0;

void on_iteration_impl(int benchmark_index, map& m, point_map<solver_result>& results, int iteration, int duration)
{
	printf("Map: %5i, Total iterations: %7i, Iteration: %3i;  Time taken: %4i us\n", benchmark_index, total_iterations++, iteration, duration);
	//visualize(m, results, false);
}

void benchmark()
{
	auto mt = std::mt19937(0);
	auto benchmarker = minedotcpp::benchmarking::benchmarker(mt);
	benchmarker.on_iteration = on_iteration_impl;
	auto settings = solver_settings();
	auto solvr = solver(settings);
	auto group = minedotcpp::benchmarking::benchmark_density_group();
	auto count = 500;
	benchmarker.benchmark_multiple(solvr, 16,16, 56, count, group);
	cout << "Density: " << group.density << endl;
	auto sum = 0;
	auto success_count = 0;
	for (auto& entry : group.entries)
	{
		//cout << entry.total_duration << "; ";
		sum += entry.total_duration;
		if(entry.solved)
		{
			success_count++;
		}
	}
	auto success_rate = (static_cast<double>(success_count) / count) * 100;
	cout << endl << "Total: " << sum << " us" << endl;
	cout << "Success rate: " << success_rate << "%" << endl;
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
	COORD coord = {300,1000};
	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleScreenBufferSize(handle, coord);
	auto wh = GetConsoleWindow();
	RECT rect = { NULL };
	GetWindowRect(wh, &rect);
	MoveWindow(wh, rect.left, 50, 1300, 950, TRUE);
#endif
	solver_settings s;
	s.give_up_from_size = 15;
	init_solver(s);
	//solve_from_file(argc, argv);
	//benchmark();
	
	cout << "Press any key to continue" << endl;
	getc(stdin);
	return 0;
}

