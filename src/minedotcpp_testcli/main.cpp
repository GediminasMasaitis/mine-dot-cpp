#include <iostream>
#include <string>
#include "mine.h"

using namespace std;

int main()
{
	auto map = new map(50, 50);
	delete map;
	cout << "Press enter to continue" << endl;
	string str;
	cin >> str;
	getc(stdin);
	return 0;
}
