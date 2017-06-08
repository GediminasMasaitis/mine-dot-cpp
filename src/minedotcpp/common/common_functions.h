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

		template<typename TModel, typename TBy>
		struct sort_by_info
		{
		public:
			TModel model;
			TBy by;

			sort_by_info(TModel model, TBy by) : model(model), by(by) { }
		};


		template<typename TModel, typename TBy>
		void vector_sort_by(std::vector<TModel>& vec, TBy(*const selector)(const TModel& model), bool(*const predicate)(const TBy& left, const TBy& right))
		{
			auto infos = std::vector<sort_by_info<TModel, TBy>>();
			infos.reserve(vec.size());
			for(size_t i = 0; i < vec.size(); ++i)
			{
				infos.emplace_back(vec[i], selector(vec[i]));
			}
			std::sort(infos.begin(), infos.end(), [&predicate](const sort_by_info<TModel, TBy>& left, const sort_by_info<TModel, TBy>& right)
			{
				return predicate(left.by, right.by);
			});
			for(size_t i = 0; i < vec.size(); ++i)
			{
				vec[i] = infos[i].model;
			}
		}
	}
}
