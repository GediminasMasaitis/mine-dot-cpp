#pragma once
namespace minedotcpp
{
	namespace common
	{
		struct point
		{
			int x;
			int y;

			struct point& operator+=(const point& rhs) { x += rhs.x; y += rhs.y; return *this; }
			struct point& operator+=(const int& k) { x += k; y += k; return *this; }
		};

		inline point operator+(point lhs, const point& rhs) { return lhs += rhs; }
		inline point operator+(point lhs, const int k) { return lhs += k; }
		inline point operator+(const int k, point rhs) { return rhs += k; }
	}
}