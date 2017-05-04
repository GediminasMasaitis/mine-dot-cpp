file(GLOB_RECURSE minedotcpp_testcli_src
    "*.h"
    "*.cpp"
)

add_executable(minedotcpp_testcli ${minedotcpp_testcli_src})

set_property(TARGET minedotcpp_testcli PROPERTY CXX_STANDARD 11)
if(shared)
	add_definitions(-DSHARED_LIB)
endif()
include_directories(${CMAKE_SOURCE_DIR}/src/minedotcpp)
target_link_libraries(minedotcpp_testcli minedotcpp)

install(TARGETS minedotcpp_testcli RUNTIME DESTINATION bin)