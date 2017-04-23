#pragma once
#include <vector>
#include <unordered_map>

namespace minedotcpp
{
	namespace common
	{
		enum cell_param
		{
			// State
			cell_states = 3,

			cell_state_empty = 0,
			cell_state_filled = 1,
			cell_state_wall = 2,

			// Flags
			cell_flags = 3 << 2,

			cell_flag_has_mine = 1 << 2,
			cell_flag_doesnt_have_mine = 2 << 2,
			cell_flag_not_sure = 3 << 2
		};

		struct point
		{
			unsigned short x;
			unsigned short y;

			struct point& operator+=(const point& rhs) { x += rhs.x; y += rhs.y; return *this; }
			struct point& operator+=(const unsigned short& k) { x += k; y += k; return *this; }
		};

		inline point operator+(point lhs, const point& rhs) { return lhs += rhs; }
		inline point operator+(point lhs, const unsigned short k) { return lhs += k; }
		inline point operator+(const unsigned short k, point rhs) { return rhs += k; }

		class cell
		{
		public:
			point point;
			cell_param state;
		};

		class neighbour_cache_entry
		{
		public:
			std::vector<cell*> all_neighbours;
			std::vector<cell*> by_state[16];
			std::vector<cell*> by_flag[16];
			std::vector<cell*> by_param[16];

			neighbour_cache_entry();
			~neighbour_cache_entry();
		};

		class map
		{
		public:
			unsigned short width;
			unsigned short height;
			cell* cells;
			neighbour_cache_entry* neighbour_cache;

			map(unsigned short width, unsigned short height);
			~map();

			cell* cell_get(unsigned short x, unsigned short y);
			cell* cell_get(point pt);
			cell* cell_try_get(unsigned short x, unsigned short y);
			cell* cell_try_get(point pt);
			bool cell_exists(unsigned short x, unsigned short y);
			bool cell_exists(point pt);

			neighbour_cache_entry* neighbour_cache_get(int x, int y);
			neighbour_cache_entry* neighbour_cache_get(point pt);
		private:
			void build_neighbour_cache();
			void calculate_neighbours_of(point pt, std::vector<cell*>& cells, bool include_self);
		};
	}
}