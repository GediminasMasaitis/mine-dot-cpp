#pragma once
namespace minedotcpp
{
	namespace common
	{
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
	}
}