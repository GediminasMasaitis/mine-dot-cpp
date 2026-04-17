#pragma once

// Tracing gate. A thread-local flag that trace sites AND with settings.print_trace
// so output can be isolated to a specific call path (e.g. only the pathological
// 57-cell border) without touching every call site. solver_service_separation
// flips this on in solve_border when effective_size meets the configured
// threshold; the RAII trace_scope restores the previous value on scope exit.

namespace minedotcpp
{
	namespace debug
	{
		inline thread_local bool trace_active = false;

		class trace_scope
		{
		public:
			explicit trace_scope(bool enable)
				: previous(trace_active)
			{
				if (enable)
				{
					trace_active = true;
				}
			}
			~trace_scope()
			{
				trace_active = previous;
			}
			trace_scope(const trace_scope&) = delete;
			trace_scope& operator=(const trace_scope&) = delete;
		private:
			bool previous;
		};
	}
}
