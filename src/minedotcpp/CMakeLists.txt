file(GLOB_RECURSE minedotcpp_src
    "*.h"
    "*.cpp"
)
if(shared)
	add_library(minedotcpp SHARED ${minedotcpp_src})
	add_definitions(-DSHARED_LIB)
else()
	add_library(minedotcpp ${minedotcpp_src})
endif()
set_property(TARGET minedotcpp PROPERTY CXX_STANDARD 11)
add_definitions(-DMINE_EXPORTS)
if(USE_OPENCL)
	add_definitions(-DENABLE_OPEN_CL)
endif()

if(USE_SPARSEHASH)
	include_directories(${CMAKE_SOURCE_DIR}/external/sparsehash)
endif()

if(USE_OPENCL)
	find_package(OpenCL REQUIRED)
	include_directories(${OpenCL_INCLUDE_DIR})
	target_link_libraries(minedotcpp ${OpenCL_LIBRARY})
endif()

install(TARGETS minedotcpp
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)