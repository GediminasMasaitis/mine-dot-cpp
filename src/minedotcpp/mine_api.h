#pragma once

#ifdef _WIN32
#  ifdef MINE_EXPORTS
#    define MINE_API __declspec(dllexport)
#  else
#    define MINE_API __declspec(dllimport)
#  endif
#else
#  define MINE_API
#endif