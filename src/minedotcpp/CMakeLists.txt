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
set_property(TARGET minedotcpp PROPERTY CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD 20)
add_definitions(-DMINE_EXPORTS)

include_directories(${CMAKE_SOURCE_DIR}/external/ctpl)

if(USE_SPARSEHASH)
    include_directories(${CMAKE_SOURCE_DIR}/external/sparsehash)
endif()

if(USE_OPENCL)
    add_definitions(-DENABLE_OPEN_CL)
    find_package(
        OpenCL
        #HINTS "C:\\Program Files (x86)\\IntelSWTools\\system_studio_2020\\OpenCL\\sdk"
        REQUIRED
    )
    message(STATUS ${OpenCL_INCLUDE_DIR})
    message(STATUS ${OpenCL_LIBRARY})
    include_directories(${OpenCL_INCLUDE_DIR})
    target_link_libraries(minedotcpp ${OpenCL_LIBRARY})
endif()

install(TARGETS minedotcpp
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)