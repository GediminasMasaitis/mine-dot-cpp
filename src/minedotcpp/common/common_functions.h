#pragma once
#include <vector>

namespace minedotcpp
{
	namespace common
	{
		template<typename T>
		void vector_erase_index_quick(std::vector<T>& v, size_t index)
		{
			auto last_index = v.size() - 1;
			if (index != last_index)
			{
				v[index] = v[last_index];
			}
			v.pop_back();
		}

		template<typename T>
		void vector_erase_index_safe(std::vector<T>& v, size_t index)
		{
			v.erase(v.begin() + index);
		}

		template<typename T>
		void vector_erase_value_quick(std::vector<T>& v, T& value)
		{
			for (auto i = 0; i < v.size(); i++)
			{
				if (v[i] == value)
				{
					vector_erase_index_quick(v, i);
				}
			}
		}

		template<typename T>
		void vector_erase_value_safe(std::vector<T>& v, T& value)
		{
			for (auto i = 0; i < v.size(); i++)
			{
				if (v[i] == value)
				{
					vector_erase_index_safe(v, i);
				}
			}
		}
	}
}
