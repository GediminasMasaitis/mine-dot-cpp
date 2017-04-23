#include "cell_param.h"
#include "point.h"
#include "cell.h"

minedotcpp::common::cell::cell()
{
}

minedotcpp::common::cell::cell(point pt, cell_param param, int hint)
{
	this->pt = pt;
	this->state = param;
	this->hint = hint;
}
