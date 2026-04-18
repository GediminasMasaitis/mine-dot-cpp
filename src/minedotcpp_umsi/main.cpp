// UMSI — Universal Minesweeper Solving Interface reference engine.
//
// A protocol-over-stdio front-end to the minedotcpp solver, modelled after
// chess's UCI. Speaks line-oriented text on stdin (commands from the GUI)
// and stdout (responses). Holds a single solver_settings instance and a
// single most-recently-loaded map; runs the solver synchronously on `go`.
//
// v0 scope:
//   * handshake (`umsi`), liveness (`isready`), settings (`setoption`)
//   * load a map (`position` ... `end`), solve it (`go`)
//   * no `stop` support — solves run to completion
//   * `print_trace` is forced off and not exposed, since its output would
//     corrupt the protocol channel
//
// The map text between `position` and `end` uses the same format as the
// existing text_map_parser / text_map_visualizer (see mapio/).

#include <cstdio>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "common/map.h"
#include "mapio/text_map_parser.h"
#include "solvers/solver.h"
#include "solvers/solver_result.h"
#include "solvers/solver_settings.h"
#include "solvers/solver_verdict.h"

using namespace std;
using namespace minedotcpp::common;
using namespace minedotcpp::mapio;
using namespace minedotcpp::solvers;

// ---------------------------------------------------------------------------
// Engine state (module-global — single engine instance per process)
// ---------------------------------------------------------------------------

static solver_settings g_settings;
static string g_position_text;

struct umsi_option
{
	string name;
	string type;            // "check" or "spin"
	string default_value;
	string min_value;       // spin only
	string max_value;       // spin only
	function<void(const string&)> set_from_string;
};

static vector<umsi_option> g_options;

// ---------------------------------------------------------------------------
// Option registry
//
// Options are registered by pointing at the corresponding solver_settings
// field; the lambda parses the string form and writes through the pointer.
// The pointer is safe to capture because g_settings is a static global with
// a lifetime that matches the process.
// ---------------------------------------------------------------------------

static void register_bool(const char* name, bool* ptr)
{
	umsi_option opt;
	opt.name = name;
	opt.type = "check";
	opt.default_value = *ptr ? "true" : "false";
	opt.set_from_string = [ptr](const string& v)
	{
		*ptr = (v == "true" || v == "1" || v == "yes" || v == "on");
	};
	g_options.push_back(std::move(opt));
}

static void register_int(const char* name, int* ptr, int lo, int hi)
{
	umsi_option opt;
	opt.name = name;
	opt.type = "spin";
	opt.default_value = to_string(*ptr);
	opt.min_value = to_string(lo);
	opt.max_value = to_string(hi);
	opt.set_from_string = [ptr](const string& v) { *ptr = stoi(v); };
	g_options.push_back(std::move(opt));
}

static void init_options()
{
	// Trivial
	register_bool("trivial_solve", &g_settings.trivial_solve);
	register_bool("trivial_stop_on_no_mine_verdict", &g_settings.trivial_stop_on_no_mine_verdict);
	register_bool("trivial_stop_on_any_verdict", &g_settings.trivial_stop_on_any_verdict);
	register_bool("trivial_stop_always", &g_settings.trivial_stop_always);

	// Gaussian
	register_bool("gaussian_solve", &g_settings.gaussian_solve);
	register_bool("gaussian_resolve_on_success", &g_settings.gaussian_resolve_on_success);
	register_bool("gaussian_single_stop_on_no_mine_verdict", &g_settings.gaussian_single_stop_on_no_mine_verdict);
	register_bool("gaussian_single_stop_on_any_verdict", &g_settings.gaussian_single_stop_on_any_verdict);
	register_bool("gaussian_single_stop_always", &g_settings.gaussian_single_stop_always);
	register_bool("gaussian_all_stop_on_no_mine_verdict", &g_settings.gaussian_all_stop_on_no_mine_verdict);
	register_bool("gaussian_all_stop_on_any_verdict", &g_settings.gaussian_all_stop_on_any_verdict);
	register_bool("gaussian_all_stop_always", &g_settings.gaussian_all_stop_always);

	// Separation
	register_bool("separation_solve", &g_settings.separation_solve);
	register_bool("separation_order_borders_by_size", &g_settings.separation_order_borders_by_size);
	register_bool("separation_order_borders_by_size_descending", &g_settings.separation_order_borders_by_size_descending);
	register_bool("separation_single_border_stop_on_no_mine_verdict", &g_settings.separation_single_border_stop_on_no_mine_verdict);
	register_bool("separation_single_border_stop_on_any_verdict", &g_settings.separation_single_border_stop_on_any_verdict);
	register_bool("separation_single_border_stop_always", &g_settings.separation_single_border_stop_always);

	// Partial
	register_bool("partial_solve", &g_settings.partial_solve);
	register_bool("partial_single_stop_on_no_mine_verdict", &g_settings.partial_single_stop_on_no_mine_verdict);
	register_bool("partial_single_stop_on_any_verdict", &g_settings.partial_single_stop_on_any_verdict);
	register_bool("partial_all_stop_on_no_mine_verdict", &g_settings.partial_all_stop_on_no_mine_verdict);
	register_bool("partial_all_stop_on_any_verdict", &g_settings.partial_all_stop_on_any_verdict);
	register_bool("partial_stop_always", &g_settings.partial_stop_always);
	register_int("partial_solve_from_size", &g_settings.partial_solve_from_size, 0, 128);
	register_int("partial_optimal_size", &g_settings.partial_optimal_size, 0, 128);
	register_bool("partial_set_probability_guesses", &g_settings.partial_set_probability_guesses);
	register_bool("partial_solve_only_when_giving_up", &g_settings.partial_solve_only_when_giving_up);

	// Resplit
	register_bool("resplit_on_partial_verdict", &g_settings.resplit_on_partial_verdict);
	register_bool("resplit_on_complete_verdict", &g_settings.resplit_on_complete_verdict);

	// Mine count
	register_bool("mine_count_ignore_completely", &g_settings.mine_count_ignore_completely);
	register_bool("mine_count_solve", &g_settings.mine_count_solve);
	register_bool("mine_count_solve_non_border", &g_settings.mine_count_solve_non_border);
	register_int("give_up_from_size", &g_settings.give_up_from_size, 0, 128);

	// OpenCL
	register_bool("valid_combination_search_open_cl", &g_settings.valid_combination_search_open_cl);
	register_bool("valid_combination_search_open_cl_allow_loop_break", &g_settings.valid_combination_search_open_cl_allow_loop_break);
	register_int("valid_combination_search_open_cl_use_from_size", &g_settings.valid_combination_search_open_cl_use_from_size, 0, 128);
	register_int("valid_combination_search_open_cl_max_batch_size", &g_settings.valid_combination_search_open_cl_max_batch_size, 1, 64);
	register_int("valid_combination_search_open_cl_platform_id", &g_settings.valid_combination_search_open_cl_platform_id, 0, 16);
	register_int("valid_combination_search_open_cl_device_id", &g_settings.valid_combination_search_open_cl_device_id, 0, 16);

	// Multithread
	register_bool("valid_combination_search_multithread", &g_settings.valid_combination_search_multithread);
	register_int("valid_combination_search_multithread_use_from_size", &g_settings.valid_combination_search_multithread_use_from_size, 0, 128);
	register_int("valid_combination_search_multithread_thread_count", &g_settings.valid_combination_search_multithread_thread_count, 1, 256);

	// Combination search
	register_bool("combination_search_gaussian_reduction", &g_settings.combination_search_gaussian_reduction);
	register_bool("combination_search_gaussian_backtracking", &g_settings.combination_search_gaussian_backtracking);

	// Variable mine count probabilities
	register_int("variable_mine_count_borders_probabilities_multithread_use_from", &g_settings.variable_mine_count_borders_probabilities_multithread_use_from, 0, 1048576);
	register_int("variable_mine_count_borders_probabilities_give_up_from", &g_settings.variable_mine_count_borders_probabilities_give_up_from, 0, 16777216);

	// Guessing
	register_bool("guess_if_no_no_mine_verdict", &g_settings.guess_if_no_no_mine_verdict);
	register_bool("guess_if_no_verdict", &g_settings.guess_if_no_verdict);

	// Debug
	register_int("debug_setting_1", &g_settings.debug_setting_1, -1000000, 1000000);
	register_int("debug_setting_2", &g_settings.debug_setting_2, -1000000, 1000000);
	register_int("debug_setting_3", &g_settings.debug_setting_3, -1000000, 1000000);

	// print_trace and its thresholds are intentionally NOT exposed: enabling
	// print_trace would cause the solver to printf() into stdout, corrupting
	// the protocol. Force it off here.
	g_settings.print_trace = false;
}

// ---------------------------------------------------------------------------
// Command handlers
// ---------------------------------------------------------------------------

static void trim_cr(string& s)
{
	if (!s.empty() && s.back() == '\r') s.pop_back();
}

static void handle_umsi()
{
	cout << "id name minedotcpp" << endl;
	cout << "id author Gediminas Masaitis" << endl;
	cout << "id version 0.1" << endl;
	for (const auto& opt : g_options)
	{
		cout << "option name " << opt.name << " type " << opt.type;
		cout << " default " << opt.default_value;
		if (opt.type == "spin")
		{
			cout << " min " << opt.min_value << " max " << opt.max_value;
		}
		cout << endl;
	}
	cout << "umsiok" << endl;
}

static void handle_setoption(const string& line)
{
	// Expected form: setoption name <name> value <value>
	istringstream iss(line);
	string tok;
	iss >> tok;                     // "setoption"
	iss >> tok;
	if (tok != "name")
	{
		cout << "error expected 'name' after 'setoption'" << endl;
		return;
	}
	string name;
	iss >> name;
	iss >> tok;
	if (tok != "value")
	{
		cout << "error expected 'value' after option name" << endl;
		return;
	}
	string value;
	iss >> value;

	for (auto& opt : g_options)
	{
		if (opt.name == name)
		{
			try
			{
				opt.set_from_string(value);
			}
			catch (const exception& e)
			{
				cout << "error failed to set " << name << ": " << e.what() << endl;
			}
			return;
		}
	}
	cout << "error unknown option: " << name << endl;
}

static void handle_position(const string& line)
{
	// Two accepted forms:
	//   1. `position` on its own, followed by rows then a terminating
	//      `end` line. Natural for typing by hand.
	//   2. `position <row>;<row>;...;<row>` on a single line, using `;`
	//      as a row separator. Matches the convention used elsewhere in
	//      this codebase (MapTextVisualizers and the GUI's CLI arg
	//      handler both do the same `;` -> newline swap). Natural for
	//      scripts and protocol traces.
	// Either way we end up with a buffer of newline-separated rows,
	// which is exactly what text_map_parser wants.
	string map_text;

	auto cmd_end = line.find_first_of(" \t");
	string trailing;
	if (cmd_end != string::npos)
	{
		auto start = line.find_first_not_of(" \t", cmd_end);
		if (start != string::npos) trailing = line.substr(start);
	}

	if (!trailing.empty())
	{
		// Single-line form: swap `;` for newlines.
		map_text.reserve(trailing.size() + 1);
		for (char c : trailing) map_text += (c == ';') ? '\n' : c;
		if (map_text.back() != '\n') map_text += '\n';
	}
	else
	{
		// Multi-line form: read rows until `end`.
		string next;
		while (getline(cin, next))
		{
			trim_cr(next);
			if (next == "end") break;
			map_text += next;
			map_text += '\n';
		}
	}

	g_position_text = std::move(map_text);
}

static const char* verdict_to_string(solver_verdict v)
{
	switch (v)
	{
	case verdict_has_mine: return "mine";
	case verdict_doesnt_have_mine: return "safe";
	default: return "unknown";
	}
}

static void handle_go()
{
	if (g_position_text.empty())
	{
		cout << "error no position loaded" << endl;
		cout << "done" << endl;
		return;
	}

	text_map_parser parser;
	map m;
	try
	{
		parser.parse(g_position_text, m);
	}
	catch (const exception& e)
	{
		cout << "error parse failed: " << e.what() << endl;
		cout << "done" << endl;
		return;
	}

	try
	{
		// Construct a fresh solver per `go`: the solver owns a ctpl thread
		// pool sized to hardware_concurrency(), and rebuilding it is cheap
		// compared to the solve itself on anything non-trivial. Settings
		// are held by reference inside the solver, so live changes from
		// prior setoption calls are picked up here.
		solver s(g_settings);
		point_map<solver_result> results;
		s.solve(m, results);

		cout.setf(ios::fixed);
		cout.precision(6);
		for (const auto& kv : results)
		{
			const auto& r = kv.second;
			cout << "result " << r.pt.x << ' ' << r.pt.y
			     << ' ' << r.probability
			     << ' ' << verdict_to_string(r.verdict) << endl;
		}
	}
	catch (const exception& e)
	{
		cout << "error solve failed: " << e.what() << endl;
	}
	cout << "done" << endl;
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------

int main()
{
	// When stdout is redirected (the normal case for a UMSI engine), the
	// C runtime defaults to block-buffering, which deadlocks a parent
	// process waiting for a line. Force unbuffered; combined with endl in
	// every handler this guarantees messages reach the client promptly.
	// (MSVC treats _IOLBF as _IOFBF — fully buffered — so we use _IONBF.)
	setvbuf(stdout, nullptr, _IONBF, 0);

	init_options();

	string line;
	while (getline(cin, line))
	{
		trim_cr(line);
		if (line.empty()) continue;

		istringstream iss(line);
		string cmd;
		iss >> cmd;

		if (cmd == "umsi")           handle_umsi();
		else if (cmd == "isready")   cout << "readyok" << endl;
		else if (cmd == "setoption") handle_setoption(line);
		else if (cmd == "position")  handle_position(line);
		else if (cmd == "go")        handle_go();
		else if (cmd == "quit")      return 0;
		else if (cmd[0] == '#')      continue;       // comment line
		else                          cout << "error unknown command: " << cmd << endl;
	}
	return 0;
}
