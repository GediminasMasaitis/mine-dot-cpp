#pragma once
#include "../common/map.h"
#include "../mapio/text_map_visualizer.h"
#include <sstream>

static void visualize(minedotcpp::common::map& m, std::vector<std::vector<minedotcpp::common::point>> regions, bool external)
{
	auto visualizer_path = "C:/Temp/Visualizer/MineDotNet.GUI.exe";
	minedotcpp::mapio::text_map_visualizer visualizer;
	std::vector<std::string> maps;
	if(m.width > 0 && m.height > 0)
	{
		auto str = visualizer.visualize_to_string(m);
		maps.push_back(str);
	}

	for(auto& region : regions)
	{
		minedotcpp::common::map mask_map(m.width, m.height, -1);
		for(auto& coord : region)
		{
			mask_map.cell_get(coord).state = minedotcpp::common::cell_state_filled;
		}
		auto str = visualizer.visualize_to_string(mask_map);
		maps.push_back(str);
	}
	std::ostringstream ss;
	if(external)
	{
		ss << "start \"\" \"" << visualizer_path << "\" ";
	}
	
	//std::istringstream mapss(str);
	if(external)
	{
		for(auto& str : maps)
		{
			str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
			std::replace(str.begin(), str.end(), '\n', ';');
			ss << " ";
			ss << str;
		}
	}
	else
	{
		std::vector<std::istringstream> mapsss;
		for(auto& str : maps)
		{
			mapsss.push_back(std::istringstream(str));
		}

		for(auto i = 0; i < m.height; i++)
		{
			for(auto& mapss : mapsss)
			{
				std::string s;
				if(std::getline(mapss, s, '\n'))
				{
					ss << s << "   ";
				}
			}
			ss << std::endl;
		}
		ss << std::endl << std::endl;
	}
	
	if(external)
	{
		system(ss.str().c_str());
	}
	else
	{
		puts(ss.str().c_str());
	}
}

static void visualize(minedotcpp::common::map& m, std::vector<minedotcpp::solvers::border> borders, bool external)
{
	std::vector<std::vector<minedotcpp::common::point>> pts;
	for(auto& b : borders)
	{
		std::vector<minedotcpp::common::point> bpts;
		bpts.reserve(b.cells.size());
		for(auto& c : b.cells)
		{
			bpts.push_back(c.pt);
		}
		pts.push_back(bpts);
	}
	visualize(m, pts, external);
}

static void visualize(minedotcpp::common::map& m, minedotcpp::common::point_map<minedotcpp::solvers::solver_result> results, bool external)
{
	std::vector<minedotcpp::common::point> pts_safe;
	std::vector<minedotcpp::common::point> pts_mine;
	for(auto& result : results)
	{
		if(result.second.verdict == minedotcpp::solvers::verdict_has_mine)
		{
			pts_mine.push_back(result.second.pt);
		}
		else if(result.second.verdict == minedotcpp::solvers::verdict_doesnt_have_mine)
		{
			pts_safe.push_back(result.second.pt);
		}
	}
	visualize(m, {pts_safe, pts_mine}, external);
}