#pragma once
#include "../common/map.h"
#include "../mapio/text_map_visualizer.h"

static void visualize_external(minedotcpp::common::map& m, std::vector<std::vector<minedotcpp::common::point>> regions)
{
	auto visualizer_path = "C:/Temp/Visualizer/MineDotNet.GUI.exe";
	minedotcpp::mapio::text_map_visualizer visualizer;
	std::vector<minedotcpp::common::map> maps;
	maps.push_back(m);

	for(auto& region : regions)
	{
		minedotcpp::common::map mask_map(m.width, m.height, -1);
		for(auto& coord : region)
		{
			mask_map.cell_get(coord).state = minedotcpp::common::cell_state_filled;
		}
		maps.push_back(mask_map);
	}
	std::string parameter_str = "start \"\" \"";
	parameter_str.append(visualizer_path);
	parameter_str.append("\" ");
	for(auto& mp : maps)
	{
		auto str = visualizer.visualize_to_string(mp);
		str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
		std::replace(str.begin(), str.end(), '\n', ';');
		parameter_str.append(" ");
		parameter_str.append(str);
	}
	system(parameter_str.c_str());
}

static void visualize_external(minedotcpp::common::map& m, std::vector<minedotcpp::solvers::border> borders)
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
	visualize_external(m, pts);
}

static void visualize_external(minedotcpp::common::map& m, minedotcpp::common::point_map<minedotcpp::solvers::solver_result> results)
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
	visualize_external(m, {pts_safe, pts_mine});
}