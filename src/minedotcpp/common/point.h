#pragma once
#include <unordered_map>

namespace minedotcpp
{
	namespace common
	{
		struct point
		{
		public:
			int x;
			int y;

			//struct point& operator+=(const point& rhs) { x += rhs.x; y += rhs.y; return *this; }
			//struct point& operator+=(const int& k) { x += k; y += k; return *this; }
		};

		inline bool operator==(const point& lhs, const point& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
		inline point operator+(point lhs, const point& rhs) { return { lhs.x + rhs.x, lhs.y + rhs.y }; }
		inline point operator+(point lhs, const int k) { return { lhs.x + k, lhs.y + k }; }
		inline point operator+(const int k, point rhs) { return { rhs.x + k, rhs.y + k }; }

		struct point_hash
		{
			size_t operator()(const point& pt) const {
				return pt.x << 4 | pt.y;
			}
		};

		template<typename T>
		using point_map = std::unordered_map<point, T, point_hash>;

	}
}

