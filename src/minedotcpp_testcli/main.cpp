#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include "mapio/text_map_parser.h"
#include "mapio/text_map_visualizer.h"
#include "solvers/solver.h"

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
	settings.solve_trivial = false;
	solver s(settings);
	auto results = s.solve(*test_map);
	print_results(results);
	delete results;
	auto str = visualizer.visualize_to_string(test_map);
	cout << str << endl;
	strm.close();
	cout << "Press any key to continue" << endl;
	getc(stdin);
	return 0;
}

