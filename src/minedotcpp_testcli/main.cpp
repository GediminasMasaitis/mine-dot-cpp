#include <iostream>
#include <string>
#include "mine.h"

using namespace std;
using namespace minedotcpp::common;

int main()
{
	auto test_map = new map(50, 50);
	test_map->cell_get(10, 10)->state = cell_state_empty;
	delete test_map;
	cout << "Press any key to continue" << endl;
	getc(stdin);
	return 0;
}
