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

include_directories(${CMAKE_SOURCE_DIR}/external/sparsehash)

install(TARGETS minedotcpp
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)