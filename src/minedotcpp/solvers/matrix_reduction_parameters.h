#pragma once

namespace minedotcpp
{
	namespace solvers
	{
		class matrix_reduction_parameters
		{
		public:
			bool skip_reduction;
			bool order_columns;
			bool reverse_columns;
			bool reverse_rows;
			bool use_unique_rows;

			matrix_reduction_parameters(bool skip_reduction, bool order_columns, bool reverse_columns, bool reverse_rows, bool use_unique_rows) :
				skip_reduction(skip_reduction), 
				order_columns(order_columns),
				reverse_columns(reverse_columns),
				reverse_rows(reverse_rows),
				use_unique_rows(use_unique_rows)
			{
			}
		};
	}
}

