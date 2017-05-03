#pragma once

#ifdef SHARED_LIB
	#ifdef _WIN32
		#ifdef MINE_EXPORTS
			#define MINE_API __declspec(dllexport)
		#else
			#define MINE_API __declspec(dllimport)
		#endif
	#else
		#define MINE_API
	#endif
#else
	#define MINE_API
#endif