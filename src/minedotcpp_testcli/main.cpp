#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
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

void solve_from_file(int argc, char* argv[])
{
	minedotcpp::mapio::text_map_parser parser;
	//minedotcpp::mapio::text_map_visualizer visualizer;
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
	settings.valid_combination_search_multithread = true;
	settings.valid_combination_search_open_cl = true;
	settings.valid_combination_search_open_cl_allow_loop_break = false;
	settings.valid_combination_search_open_cl_platform_id = 0;
	settings.valid_combination_search_open_cl_device_id = 0;

	//settings.mine_count_ignore_completely = true;
	//settings.guess_if_no_no_mine_verdict = false;
	//settings.give_up_from_size = 25;
	init_solver(settings);
	//solver s(settings);
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
	//auto fs = ofstream("C:\\Temp\\map_end.txt", ios::app);
	//fs << entry.map_index << " " << entry.solved << endl;
	//fs.close();
	//printf("Map: %5i, Success: %s, Time taken: %4i ms,\n", entry.map_index, entry.solved ? "Y" : "N", entry.total_duration / 1000);
}

void benchmark()
{
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
	settings.partial_solve_from_size = 28;
	settings.partial_optimal_size = 24;
	settings.valid_combination_search_multithread = true;
	settings.valid_combination_search_open_cl = false;
	settings.valid_combination_search_open_cl_allow_loop_break = false;
	
	
	//settings.gaussian_all_stop_always = true;
	//settings.separation_single_border_stop_on_no_mine_verdict = false;
	//settings.guess_if_no_no_mine_verdict = true;
	
	auto solvr = solver(settings);

	auto file = fstream("C:/mine/results.txt", ios_base::in | ios_base::out | ios_base::app);
	//file << "16x16 30-->100" << endl;
	
	for(auto mineCount = 99; mineCount < 100; mineCount++)
	{
		auto group = minedotcpp::benchmarking::benchmark_density_group();
		auto count = 10000;
		const auto width = 30;
		const auto heigth = 16;
		benchmarker.benchmark_multiple(solvr, width, heigth, mineCount, count, group);
		cout << "Density: " << group.density << endl;
		size_t sum = 0;
		auto success_count = 0;
		for (auto& entry : group.entries)
		{
			//cout << entry.total_duration << "; ";
			sum += entry.total_duration;
			if (entry.solved)
			{
				success_count++;
			}
		}
		auto success_rate = (static_cast<double>(success_count) / count) * 100;
		const auto avg = (sum / count);
		cout << endl << "Total: " << (avg / 1000) << " us" << endl;
		cout << "Success rate: " << success_rate << "%" << endl;
		file << std::fixed << std::setprecision(2) << group.density*100 << "\t" << success_rate << "\t" << (avg / 1000) << "\t" << endl;
	}
	file.close();
}

void benchmark2()
{
	auto mt = std::mt19937(0);
	auto benchmarker = minedotcpp::benchmarking::benchmarker(mt);
	benchmarker.on_iteration = on_iteration_impl;
	benchmarker.on_end = on_end_impl;

	auto settings = solver_settings();
	settings.trivial_solve = true;
	settings.trivial_stop_always = true;
	//settings.gaussian_solve = false;
	//settings.separation_solve = false;

	//settings.gaussian_all_stop_always = true;
	//settings.separation_single_border_stop_on_no_mine_verdict = false;
	settings.valid_combination_search_open_cl_platform_id = 0;
	//settings.guess_if_no_no_mine_verdict = true;

	auto solvr = solver(settings);

	auto file = fstream("C:/mine/results2.txt", ios_base::in | ios_base::out | ios_base::app);
	file << "16x16 30-->100" << endl;

	for (auto size = 4; size < 64; size++)
	{
		auto group = minedotcpp::benchmarking::benchmark_density_group();
		auto count = 1000;
		const auto width = size;
		const auto heigth = size;
		const auto area = width * heigth;
		const auto mineCount = (area * 15) / 100;
		benchmarker.benchmark_multiple(solvr, width, heigth, mineCount, count, group);
		cout << "Density: " << group.density << endl;
		size_t sum = 0;
		auto success_count = 0;
		for (auto& entry : group.entries)
		{
			//cout << entry.total_duration << "; ";
			sum += entry.total_duration;
			if (entry.solved)
			{
				success_count++;
			}
		}
		auto success_rate = (static_cast<double>(success_count) / count) * 100;
		const auto avg = (sum / count);
		cout << endl << "Total: " << (avg / 1000) << " us" << endl;
		cout << "Success rate: " << success_rate << "%" << endl;
		file << std::fixed << std::setprecision(2) << size << "\t" << success_rate << "\t" << (avg / 1000) << "\t" << endl;
	}
	file.close();
}

void benchmark3()
{
	auto settings = solver_settings();
	settings.trivial_solve = true;
	settings.gaussian_solve = true;

	settings.separation_solve = true;
	settings.partial_solve = false;
	settings.mine_count_solve = false;
	settings.valid_combination_search_multithread = true;
	settings.valid_combination_search_open_cl = true;

	//settings.gaussian_all_stop_always = true;
	//settings.separation_single_border_stop_on_no_mine_verdict = false;
	//settings.guess_if_no_no_mine_verdict = true;
	auto solvr = solver(settings);
	

	auto file = fstream("C:/mine/results3.txt", ios_base::in | ios_base::out | ios_base::app);
	//file << "16x16 30-->100" << endl;

	for (auto multiThreadFrom = 16; multiThreadFrom <= 30; multiThreadFrom++)
	{
		auto mt = std::mt19937(0);
		auto benchmarker = minedotcpp::benchmarking::benchmarker(mt);
		benchmarker.on_iteration = on_iteration_impl;
		benchmarker.on_end = on_end_impl;
		const auto mineCount = 50;
		//settings.valid_combination_search_multithread_thread_count = multiThreadFrom;
		//solvr.settings.valid_combination_search_multithread_thread_count = multiThreadFrom;
		//solvr.separation_service.settings.valid_combination_search_multithread_thread_count = multiThreadFrom;
		//solvr.separation_service.combination_service.settings.valid_combination_search_multithread_thread_count = multiThreadFrom;
		
		settings.valid_combination_search_open_cl_use_from_size = multiThreadFrom;
		solvr.settings.valid_combination_search_open_cl_use_from_size = multiThreadFrom;
		solvr.separation_service.settings.valid_combination_search_open_cl_use_from_size = multiThreadFrom;
		solvr.separation_service.combination_service.settings.valid_combination_search_open_cl_use_from_size = multiThreadFrom;
		
		auto group = minedotcpp::benchmarking::benchmark_density_group();
		auto count = 1000;
		const auto width = 16;
		const auto heigth = 16;
		benchmarker.benchmark_multiple(solvr, width, heigth, mineCount, count, group);
		cout << "Density: " << group.density << endl;
		size_t sum = 0;
		auto success_count = 0;
		for (auto& entry : group.entries)
		{
			//cout << entry.total_duration << "; ";
			sum += entry.total_duration;
			if (entry.solved)
			{
				success_count++;
			}
		}
		auto success_rate = (static_cast<double>(success_count) / count) * 100;
		const auto avg = (sum / count);
		cout << endl << "Total: " << (avg / 1000) << " us" << endl;
		cout << "Success rate: " << success_rate << "%" << endl;
		file << std::fixed << std::setprecision(2) << multiThreadFrom << "\t" << (avg / 1000) << "\t" << endl;
	}
	file.close();
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

void pattern_search_test()
{
	//auto pt = point{ 4 , 6 };
}

void umsi()
{
	
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
	list_open_cl_devices();
	//pattern_search_test();
	//solve_from_file(argc, argv);
	benchmark();
	//test_global_api();
	cout << "Press any key to continue" << endl;
	getc(stdin);
	return 0;
}

