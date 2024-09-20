file(GLOB_RECURSE minedotcpp_testcli_src
    "*.h"
    "*.cpp"
)

add_executable(minedotcpp_testcli ${minedotcpp_testcli_src})

set_property(TARGET minedotcpp_testcli PROPERTY CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD 20)
if(shared)
    add_definitions(-DSHARED_LIB)
endif()

if(USE_OPENCL)
    add_definitions(-DENABLE_OPEN_CL)
    find_package(OpenCL REQUIRED)
    message(STATUS ${OpenCL_INCLUDE_DIR})
    include_directories(${OpenCL_INCLUDE_DIR})
endif()

include_directories(${CMAKE_SOURCE_DIR}/src/minedotcpp)
include_directories(${CMAKE_SOURCE_DIR}/external/ctpl)
include_directories(${CMAKE_SOURCE_DIR}/external/sparsehash)
target_link_libraries(minedotcpp_testcli minedotcpp)

install(TARGETS minedotcpp_testcli RUNTIME DESTINATION bin)