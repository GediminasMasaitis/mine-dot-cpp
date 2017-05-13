#pragma once
#include "../common/map.h"
#include "../mapio/text_map_visualizer.h"
#include "../solvers/solver_result.h"
#include "../solvers/border.h"
#include <iostream>
#include <sstream>

static void dump_verdicts(minedotcpp::common::point_map<bool>& ps)
{
	for (auto& p : ps)
	{
		printf("[%i, %i]: %s  ", p.first.x, p.first.y, p.second ? "true" : "false");
	}
	printf("\n");
}

static void dump_probabilities(minedotcpp::common::point_map<double>& ps)
{
	for (auto& p : ps)
	{
		printf("[%i, %i]: %f  ", p.first.x, p.first.y, p.second);
	}
	printf("\n");
}

static void dump_ratios(google::dense_hash_map<int, double>& ps)
{
	for (auto& p : ps)
	{
		printf("[%i]: %f  ", p.first, p.second);
	}
	printf("\n");
}

static void dump_point_set(minedotcpp::common::point_set& ps)
{
	for(auto& p : ps)
	{
		printf("[%i, %i]  ", p.x, p.y);
	}
	printf("\n");
}

static void dump_results(minedotcpp::common::point_map<minedotcpp::solvers::solver_result>& results)
{
	if (results.size() == 0)
	{
		std::cout << "No results" << std::endl;
		return;
	}
	for (auto& result : results)
	{
		std::cout << "[" << result.first.x << ";" << result.first.y << "] " << result.second.probability * 100 << "%" << std::endl;
	}
	std::cout << std::endl;
}

static void dump_predictions(std::vector<minedotcpp::common::point_map<bool>>& predictions)
{
	for(auto& prediction : predictions)
	{
		for(auto& entry : prediction)
		{
			putc(entry.second ? '+' : ' ', stdout);
		}
		putc('\n', stdout);
	}
}

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
		minedotcpp::common::map mask_map;
		mask_map.init(m.width, m.height, -1, minedotcpp::common::cell_state_empty);
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
		if(m.remaining_mine_count != -1)
		{
			ss << "m" << m.remaining_mine_count << std::endl;
		}
		ss << std::endl << std::endl;
	}
	
	if(external)
	{
		auto str = ss.str();
		system(str.c_str());
	}
	else
	{
		auto str = ss.str();
		std::replace(str.begin(), str.end(), 'X', ' ');
		puts(str.c_str());
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