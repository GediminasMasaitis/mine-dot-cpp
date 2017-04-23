#include <iostream>
#include <string>
#include <fstream>
#include "mine.h"
#include "mapio/text_map_parser.h"
#include "mapio/text_map_visualizer.h"

using namespace std;
using namespace minedotcpp::common;

int main()
{
	minedotcpp::mapio::text_map_parser parser;
	minedotcpp::mapio::text_map_visualizer visualizer;
	ifstream strm("C:/Temp/test_map.txt");
	auto test_map = parser.parse(&strm);
	auto str = visualizer.visualize_to_string(test_map);
	cout << str << endl;
	strm.close();
	cout << "Press any key to continue" << endl;
	getc(stdin);
	return 0;
}
