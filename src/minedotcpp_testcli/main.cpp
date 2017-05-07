#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include "mapio/text_map_parser.h"
#include "mapio/text_map_visualizer.h"
#include "solvers/solver.h"
#include <chrono>
#include "debug/debugging.h"
#ifdef _WIN32
#include <windows.h>
#endif


using namespace std;
using namespace minedotcpp::common;
using namespace minedotcpp::solvers;

void print_results(point_map<solver_result>* results)
{
	if(results == nullptr)
	{
		cout << "No results" << endl;
		return;
	}
	for(auto& result : *results)
	{
		cout << "[" << result.first.x << ";" << result.first.y << "] " << result.second.probability * 100 << "%" << endl;
	}
	cout << endl;
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
	COORD coord = {300,1000};
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleScreenBufferSize(handle, coord);

	HWND wh = GetConsoleWindow();
	RECT rect = { NULL };
	GetWindowRect(wh, &rect);
	// Move window to required position
	//MoveWindow(wh, rect.left, rect.top, 640, 900, TRUE);
	MoveWindow(wh, rect.left, 50, 800, 950, TRUE);
#endif

	/* std::chrono::high_resolution_clock cl;
	auto b = cl.now();
	for (int j = 0; j < 20000; j++)
	{
		google::dense_hash_map<point, bool, point_hash> a;
		point empty_pt = { -1,-1 };
		a.set_empty_key(empty_pt);
		//a.resize(32);
		//point_map<bool> a;
		//a.reserve(32);
		for (int i = 1; i < 32; i++)
		{
			point pt = { i, i * 2 };
			a[pt] = true;
		}
	}
	auto e = cl.now();
	auto d = e - b;

	cout << endl << "That took " << std::chrono::duration_cast<std::chrono::milliseconds>(d).count() << " ms" << endl << endl;
	getc(stdin);
	return 0;*/
	minedotcpp::mapio::text_map_parser parser;
	minedotcpp::mapio::text_map_visualizer visualizer;
	string path = "C:/Temp/test_map.txt";
	map* test_map = nullptr;
	if(argc > 1)
	{
		string cmd(argv[1]);
		if(cmd == "--path")
		{
			if (argc > 2)
			{
				path = string(argv[2]);
			}
			else
			{
				cout << "No path supplied" << endl;
				return 1;
			}
		}
		else if (cmd == "--map")
		{
			if (argc > 2)
			{
				test_map = parser.parse(string(argv[2]));
			}
			else
			{
				cout << "No map supplied" << endl;
				return 1;
			}
		}
	}

	ifstream strm(path);
	if(test_map == nullptr)
	{
		test_map = parser.parse(&strm);
	}

	solver_settings settings;
	settings.trivial_solve = false;
	solver s(settings);
	std::chrono::high_resolution_clock clock;
	auto start_time = clock.now();
	auto results = s.solve(*test_map);
	auto end_time = clock.now();
	auto diff = end_time - start_time;
	
	cout << endl << "That took " << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() << " ms" << endl << endl;
	print_results(results);
	//visualize_external(*test_map, *results);
	delete results;
	auto str = visualizer.visualize_to_string(*test_map);
	cout << str << endl;
	strm.close();
	cout << "Press any key to continue" << endl;
	getc(stdin);
	return 0;
}

