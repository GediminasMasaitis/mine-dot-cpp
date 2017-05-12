#pragma once
#include <sparsehash/dense_hash_map>
#include <sparsehash/dense_hash_set>

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

		inline bool operator!=(const point& lhs, const point& rhs) { return lhs.x != rhs.x || lhs.y != rhs.y; }
		inline bool operator==(const point& lhs, const point& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
		inline point operator+(point lhs, const point& rhs) { return { lhs.x + rhs.x, lhs.y + rhs.y }; }
		inline point operator+(point lhs, const int k) { return { lhs.x + k, lhs.y + k }; }
		inline point operator+(const int k, point rhs) { return { rhs.x + k, rhs.y + k }; }

		struct point_hash
		{
			size_t operator()(const point& pt) const {
				return pt.x * 7919 + pt.y;
			}
		};

		//template<typename T>
		//using point_map = google::dense_hash_map<point, T, point_hash>;//std::unordered_map<point, T, point_hash>;

		template<typename T>
		class point_map : public google::dense_hash_map<const point, T, point_hash>
		{
		public:
			inline point_map()
			{
				point_map::set_empty_key({ -1,-1 });
				point_map::set_deleted_key({ -2,-2 });
			}
		};

		//using point_set = std::unordered_set<point, point_hash>;

		class point_set : public google::dense_hash_set<point, point_hash>
		{
		public:
			inline point_set()
			{
				set_empty_key({ -1,-1 });
				set_deleted_key({ -2,-2 });
			}
		};
	}
}

